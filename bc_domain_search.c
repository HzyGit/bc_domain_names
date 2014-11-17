#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
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
	/// 创建proc文件
	if(create_mem_proc(PROC_NAME)<0)
	{
		err=-ENOMEM;
		goto clean_mem;
	}
	printk(KERN_INFO "/proc/%s create ok",PROC_NAME);
	/// 设置read_buf
	if((err=dump_bigmem(&db.mem,&read_buf))<0)
	{
		read_buf=NULL;
		printk(KERN_ERR "dump bigmem to read_buf error");
		goto clean_mem;
	}
	temp=strlen(read_buf);
	/// 输出信息
	printk(KERN_INFO"%s(%s) load ok\n",NAME,BC_DOMAIN_MEM_VERSION);
	return 0;
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

