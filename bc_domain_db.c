/*
 * @file bc_domain_db.cpp
 * @breif 域名结构的常用操作定义文件
 * @author hzy.oop@gmail.com
 * @date 2014-11-14
 */

#include "bc_domain_names.h"

#ifndef USER_SPACE

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#else  /// USER_SPACE

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#endif /// USER_SPACE

#include "bc_domain_names.h"

#ifndef USER_SPACE
/// @brief 内核函数，初始化bc_domain_db
int init_bc_domain_db(struct bc_domain_db *db)
{
	int i=0;
	int err=0;
	size_t sum=0;
	size_t max_len[DOMAIN_TYPE_NUM]={
		WEBPAGE_DOMAIN_MAX_COUNT,
		BLANK_DOMAIN_MAX_COUNT,
		DOWNLOAD_DOMAIN_MAX_COUNT,
		MULTIMEDIA_DOMAIN_MAX_COUNT,
		INTERNATIONAL_DOMAIN_MAX_COUNT,
		SHOPPING_DOMAIN_MAX_COUNT
	};
	/// 初始化bc_domain_names
	db->domain_names.is_update=false;
	memset(db->domain_names.domain_type_len,0,
			sizeof(db->domain_names.domain_type_len));
	for(i=0;i<DOMAIN_TYPE_NUM;i++)
		db->domain_names.domain_type_max_len[i]=max_len[i];
	sum=sizeof(struct bc_domain_names);
	for(i=0;i<DOMAIN_TYPE_NUM;i++)
	{
		db->domain_names.domain_type_start[i]=sum;
		sum+=max_len[i];
	}
	/// 初始化bigmem
	if((err=init_bigmem(&db->mem,
					DOMAIN_MAX_COUNT*sizeof(struct domain_name)+sizeof(struct bc_domain_names),
					GFP_KERNEL))<0)
	{
		printk(KERN_ERR "init_bigmem error\n");
		return err;
	}
	/// bc_domain_name
	if((err=save_bc_domain_names(db))<0)
	{
		printk(KERN_ERR "write bc_domain_names error\n");
		clean_bigmem(&db->mem);
	}
	return err;
}

/// @brief 内核函数, 清除bc_domain_db，并释放内存
int clean_bc_domain_db(struct bc_domain_db *db)
{
	memset(&db->domain_names,0,sizeof(struct bc_domain_names));
	/// 清除内存
	clean_bigmem(&db->mem);
	return 0;
}

#else
/// @brief 用户函数,从文件中反序列化db结构
/// @retval 成功0 错误返回错误代码的负值
int load_bc_domain_db(const char *path,struct bc_domain_db *db)
{
	if(NULL==db)
		return -EINVAL;
	/// 从文件中读取全部内容
	char buf[1024]="";
	const int buf_len=sizeof(buf)/sizeof(buf[0]);
	FILE *fp=NULL;
	if((fp=fopen(path,"r"))==NULL)
	{
		error_at_line(0,errno,__FILE__,__LINE__,"open %s error",path);
		return -errno;
	}
	int err=0;
	int left_len=buf_len;
	char *tmp=buf;
	while(left_len>1)
	{
		if(fgets(tmp,left_len,fp)==NULL)
			break;
		left_len-=strlen(tmp);
		tmp+=strlen(tmp);
	}
	fclose(fp);
	/// 反序列化bigmem
	if((err=load_bigmem(&db->mem,buf))<0)
	{
		error_at_line(0,-err,__FILE__,__LINE__,"load bigmem error");
		return err;
	}
	/// 映射内存
	int fd=0;
	if((fd=open("/dev/mem",O_RDWR))<0)
	{
		error_at_line(0,errno,__FILE__,__LINE__,"load bigmem error");
		return -errno;
	}
	if((err=mmap_bigmem(&db->mem,fd,PROT_READ|PROT_WRITE,MAP_SHARED))<0)
	{
		error_at_line(0,-err,__FILE__,__LINE__,"load bigmem error");
		return err;
	}
	/// 设置db属性
	if((err=read_bigmem(&db->mem,0,&db->domain_names,sizeof(db->domain_names)))<0)
	{
		error_at_line(0,-err,__FILE__,__LINE__,"load bigmem error");
		return err;
	}
	return 0;
}

void unload_bc_domain_db(struct bc_domain_db *db)
{
	unmmap_clean_bigmem(&db->mem);
}

/// @breif 整理数据结构中的内存，避免碎片
void defrag_mentation(struct bc_domain_db *db,enum domain_type type)
{
}

#endif

/// @brief 设置更新标识
int set_update_domain_db(struct bc_domain_db *db,bool isupdate)
{
	int err=0;

	if(NULL==db)
		return -EINVAL;
	db->domain_names.is_update=isupdate;
	/// 写入内存
#ifdef USER_SPACE
	if((err=write_bigmem(&db->mem,0,&db->domain_names,sizeof(struct bc_domain_names)))<0)
#else
	if((err=write_bigmem_bh(&db->mem,0,&db->domain_names,sizeof(struct bc_domain_names)))<0)
#endif
	{
#ifndef USER_SPACE
		printk(KERN_ERR "write_bigmem error");
#else
		error_at_line(0,-err,__FILE__,__LINE__,"write_bigmem error");
#endif
	}
	return err;
}


/// @brief 返回db中域名个数
size_t get_domain_name_num(struct bc_domain_db *db,enum domain_type type)
{
	if(NULL==db)
		return 0;
	if(DOMAIN_TYPE_NUM<=type)
		return -EFAULT;
	return db->domain_names.domain_type_len[type];
}

/// @brief 返回db中第i个域名
/// @retval 成功0 失败错误代码的负值
int get_domain_name(struct domain_name *name,struct bc_domain_db *db,enum domain_type type,size_t index)
{
	int err=0;
	/// 判断参数
	if(NULL==db)
		return -EINVAL;
	if(DOMAIN_TYPE_NUM==type)
		return -EFAULT;
	if(index>=db->domain_names.domain_type_len[type])
		return -EFAULT;
	/// 获取内存
#ifdef USER_SPACE
	if((err=read_bigmem(&db->mem,
					db->domain_names.domain_type_start[type]+index*sizeof(struct domain_name),
					name,sizeof(struct domain_name)))<0)
#else
	if((err=read_bigmem_bh(&db->mem,
					db->domain_names.domain_type_start[type]+index*sizeof(struct domain_name),
					name,sizeof(struct domain_name)))<0)
#endif
	{
#ifdef USER_SPACE
		error_at_line(0,-err,__FILE__,__LINE__,"read_bigmem error");
#else
		printk(KERN_ERR "read_bigmem error");
#endif
		return err;
	}
	return 0;
}

/// @brief 设置db中第i个域名
/// @retval 成功0 失败错误代码的负值
int set_domain_name(const struct domain_name *name,struct bc_domain_db *db,enum domain_type type,size_t index)
{
	int err=0;

	if(NULL==db)
		return -EINVAL;
	if(DOMAIN_TYPE_NUM==type)
		return -EFAULT;
	if(index>=db->domain_names.domain_type_len[type])
		return -EFAULT;
	/// 设置内存
#ifdef USER_SPACE
	if((err=write_bigmem(&db->mem,
				db->domain_names.domain_type_start[type]+index*sizeof(struct domain_name),
				name,sizeof(struct domain_name)))<0)
#else
	if((err=write_bigmem_bh(&db->mem,
				db->domain_names.domain_type_start[type]+index*sizeof(struct domain_name),
				name,sizeof(struct domain_name)))<0)
#endif
	{
#ifdef USER_SPACE
		error_at_line(0,-err,__FILE__,__LINE__,"write_bigmem error");
#else
		printk(KERN_ERR "write_bigmem error");
#endif
		return err;
	}
	return 0;
}

/// @brief 保存bc_domain_names结构
/// @retval 成功返回0 失败错误代码负值
int save_bc_domain_names(struct bc_domain_db *db)
{
	if(NULL==db)
		return 0;
#ifndef USER_SPACE
	return write_bigmem_bh(&db->mem,0,&db->domain_names,sizeof(struct bc_domain_names));
#else
	return write_bigmem(&db->mem,0,&db->domain_names,sizeof(struct bc_domain_names));
#endif
}

