#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "bc_domain_search.h"
#include "bc_domain_names.h"

#define NAME "bc_domain_mem"
#define BC_DOMAIN_MEM_VERSION "v1.0"

MODULE_AUTHOR("hzy(hzy.oop@gmail.com) bingchuan inc");
MODULE_DESCRIPTION("a module support bingchuan domains cache and quick search");
MODULE_LICENSE("GPL");

/// 冰川域名数据库
struct bc_domain_db db;
/// proc文件
struct proc_dir_entry *mem_proc=NULL;
struct proc_dir_entry *dir_proc=NULL;
/// proc文件内容
char *read_buf=NULL;
size_t temp=0;

/// hash表
struct domain_hash
{
	struct hlist_head *head;
	size_t len;
};

/// hash node
struct domain_hash_entry
{
	struct hlist_node node;
	size_t index;
};

/// @breif 域名db的hash结构
struct domain_db_hash
{
	struct domain_hash hashs[DOMAIN_TYPE_NUM];
	spinlock_t lock;
}db_hash;


/// @breif 初始化domain_hash结构
/// @param[in] len初始哈希大小
/// @retval 成功0 错误返回错误码的负值
static int init_domain_hash(struct domain_hash *hash,size_t len)
{
	int i=0;

	if(NULL==hash)
		return 0;
	if(0==len)
	{
		hash->len=0;
		hash->head=NULL;
	}
	/// 分配内存
	hash->head=(struct hlist_head*)kmalloc(sizeof(struct domain_hash)*len,GFP_ATOMIC);
	if(NULL==hash->head)
	{
		hash->len=0;
		return -ENOMEM;
	}
	hash->len=len;
	/// 初始化
	for(i=0;i<hash->len;i++)
		INIT_HLIST_HEAD(hash->head+i);
	return 0;
}

/// @breif 添加index到hash
/// @param[in] key hash函数求得的key值
/// @param[in] index 哈希内容
static int add_domain_hash(struct domain_hash *hash,int key,size_t index)
{
	struct domain_hash_entry *entry=NULL;
	if(NULL==hash||0==hash->len||NULL==hash->head)
		return -EINVAL;
	/// 加入hash
	key=key%hash->len;
	entry=(struct domain_hash_entry*)kmalloc(sizeof(struct domain_hash_entry),GFP_ATOMIC);
	if(NULL==entry)
		return -ENOMEM;
	entry->index=index;
	hlist_add_head(&entry->node,hash->head+key);
	return 0;
}

/// @breif 清除hash表中内容
static void clean_domain_hash(struct domain_hash *hash)
{
	int i;
	if(NULL==hash)
		return;
	/// 清除哈希内容
	for(i=0;i<hash->len;++i)
	{
		struct hlist_head *p=hash->head+i;
		while(!hlist_empty(p))
		{
			struct hlist_node *n=p->first;
			hlist_del(n);
			kfree(hlist_entry_safe(n,struct domain_hash_entry,node));
		}
	}
}

/// @brief 销毁domain_hash结构
static void destory_domain_hash(struct domain_hash *hash)
{
	clean_domain_hash(hash);
	/// 销毁head数组
	kfree(hash->head);
	hash->head=NULL;
	hash->len=0;
}

/// @brief 初始化db_hash
static int init_domain_db_hash(void)
{
	const int HASH_MAX_SIZE=2048;    /// 哈希最大大小
	size_t hash_len[DOMAIN_TYPE_NUM];
	int i=0;
	int cur_hash=0;
	int err=0;
	/// 计算哈希表大小
	for(i=0;i<DOMAIN_TYPE_NUM;i++)
	{
		hash_len[i]=db.domain_names.domain_type_max_len[i];
		hash_len[i]=hash_len[i]>HASH_MAX_SIZE?HASH_MAX_SIZE:hash_len[i];
	}
	/// 初始化哈希
	for(cur_hash=0;cur_hash<DOMAIN_TYPE_NUM;cur_hash++)
	{
		if((err=init_domain_hash(db_hash.hashs+cur_hash,hash_len[cur_hash]))<0)
		{
			printk(KERN_ERR "%s: init hash %d error\n",NAME,cur_hash);
			goto clean_hash;
		}
	}
	/// 初始化锁
	spin_lock_init(&db_hash.lock);
	return 0;
clean_hash:
	for(i=0;i<cur_hash;i++)
		destory_domain_hash(db_hash.hashs+i);
	return err;
}

/// @brief 销毁db_hash,并释放内存
static void destory_domain_db_hash(void)
{
	int i=0;
	spin_lock_bh(&db_hash.lock);
	for(i=0;i<DOMAIN_TYPE_NUM;++i)
		destory_domain_hash(db_hash.hashs+i);
	spin_unlock_bh(&db_hash.lock);
}

/// @brief 计算哈希值
static size_t hash_key_str(const char *str)
{
	unsigned int hash=1315423911;
	while(*str)
	{
		hash^=((hash<<5)+(*str++)+(hash>>2));
	}
	return hash;
}

/// @brief 依据bc_domain_db建立 哈希表
static void build_domain_db_hash(void)
{
	int i=0,j=0;
	int err=0;
	for(i=0;i<DOMAIN_TYPE_NUM;++i)
	{
		/// 读取域名
		spin_lock_bh(&db_hash.lock);
		for(j=0;j<get_domain_name_num(&db,i);j++)
		{
			struct domain_name name;
			size_t key=0;
			if((err=get_domain_name(&name,&db,i,j))<0)
				continue;
			if(!name.is_vaild)
				continue;
			key=hash_key_str(name.name);
			add_domain_hash(db_hash.hashs+i,key,j);
		}
		spin_unlock_bh(&db_hash.lock);
	}
}

/// @brief 清除bc_domain_hash中内容
static void clean_domain_db_hash(void)
{
	int i=0;
	for(i=0;i<DOMAIN_TYPE_NUM;++i)
	{
		spin_lock_bh(&db_hash.lock);
		clean_domain_hash(db_hash.hashs+i);
		spin_unlock_bh(&db_hash.lock);
	}
}

/// @breif 依据hash结构对域名进行查找
/// @retval 匹配成功1 匹配失败-1 出错-1
int bc_domain_match(const char *domain,enum domain_type type)
{
	int err=0;
	size_t key=0;
	struct hlist_head *head=NULL;
	struct domain_hash_entry *entry;
	//struct hash_node *n;
	size_t hash_len=0;

	/// 判断type是否合法
	if(type<0||type>=DOMAIN_TYPE_NUM)
		return -EINVAL;
	/// 判断是否需要重建hash
	if((err=read_bigmem_bh(&db.mem,0,&db.domain_names,sizeof(db.domain_names)))<0)
		return err;
	if(db.domain_names.is_update)
	{
		clean_domain_db_hash();
		build_domain_db_hash();
	}
	/// 查找是否匹配
	err=0;    ///< 默认不匹配
	key=hash_key_str(domain);
	spin_lock_bh(&db_hash.lock);
	hash_len=db_hash.hashs[type].len;
	head=db_hash.hashs[type].head;
	if(0==hash_len||NULL==head)
		goto unlock;
	key%=hash_len;
	head+=key;
	hlist_for_each_entry(entry,head,node)
	{
		size_t index=entry->index;
		struct domain_name name;
		if((err=get_domain_name(&name,&db,type,index))<0)
			goto unlock;
		if(!name.is_vaild)
			continue;
		if(strcasecmp(name.name,domain)==0)
		{
			err=1;                     ///< 匹配
			break;
		}
	}
unlock:
	spin_unlock_bh(&db_hash.lock);
	return err;
}
EXPORT_SYMBOL(bc_domain_match);


/// @brief mem proc文件的读函数
ssize_t proc_mem_read(struct file *f,char *buf,size_t count,loff_t *offp)
{
	if(NULL==read_buf)
		return 0;
	if(count>temp)
		count=temp;
	temp-=count;
	copy_to_user(buf,read_buf,count);
	if(0==count)
		temp=strlen(read_buf);
	return count;
}

static const struct file_operations fops={
	.owner=THIS_MODULE,
	.read=proc_mem_read,
};


/// @brief 创建proc文件
static int create_mem_proc(const char *proc_name)
{
	mem_proc=proc_create(proc_name,0444,NULL,&fops);
	if(NULL==mem_proc)
	{
		printk(KERN_ERR"count not initialize /proc/%s",PROC_NAME);
		return -1;
	}
	return 0;
}

/// @brief 删除proc文件 
static void clean_mem_proc(void)
{
	remove_proc_entry(PROC_NAME,NULL);
}

static int bc_domain_search_init(void)
{
	int err=0;
	/// 初始化bc_domain_db
	if((err=init_bc_domain_db(&db))<0)
	{
		printk(KERN_INFO"init bc_domain db error");
		goto err_back;
	}
	/// 初始化hash
	if((err=init_domain_db_hash())<0)
	{
		printk(KERN_INFO "init domain db_hash error");
		goto clean_mem;
	}

	/// 创建proc文件
	if(create_mem_proc(PROC_NAME)<0)
	{
		err=-ENOMEM;
		goto destory_hash;
	}
	printk(KERN_INFO "/proc/%s create ok",PROC_NAME);
	/// 设置read_buf
	if((err=dump_bigmem(&db.mem,&read_buf))<0)
	{
		read_buf=NULL;
		printk(KERN_ERR "dump bigmem to read_buf error");
		goto destory_hash;
	}
	temp=strlen(read_buf);
	/// 输出信息
	printk(KERN_INFO"%s(%s) load ok\n",NAME,BC_DOMAIN_MEM_VERSION);
	return 0;
destory_hash:
	destory_domain_db_hash();
clean_mem:
	clean_bc_domain_db(&db);
err_back:
	printk(KERN_INFO"%s(%s) load error\n",NAME,BC_DOMAIN_MEM_VERSION);
	return err;

}

static void bc_domain_search_exit(void)
{
	int err=0;
	/// 清除read_buf
	if(NULL!=read_buf)
	{
		kfree(read_buf);
		temp=0;
	}
	/// 清除proc_file
	clean_mem_proc();
	/// 清除hash
	destory_domain_db_hash();
	/// 清除bc_domain_db
	if((err=clean_bc_domain_db(&db))<0)
	{
		printk(KERN_ERR"clean bc_domain db error");
		printk(KERN_ERR"unload %s(%s) error",NAME,BC_DOMAIN_MEM_VERSION);
	}
	else
		printk(KERN_INFO"%s(%s) unload ok\n",NAME,BC_DOMAIN_MEM_VERSION);
}

module_init(bc_domain_search_init);
module_exit(bc_domain_search_exit);

