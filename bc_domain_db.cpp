/*
 * @file bc_domain_db.cpp
 * @breif 域名结构的常用操作定义文件
 * @author hzy.oop@gmail.com
 * @date 2014-11-14
 */

#ifndef USER_SPACE
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
extern "C" {
#include "bc_domain_names.h"
};

#endif /// USER_SPACE

#ifndef USER_SPACE
/// @brief 内核函数，初始化bc_domain_db
int init_bc_domain_db(struct bc_domain_db *db)
{
	return 0;
}

/// @brief 内核函数, 清除bc_domain_db，并释放内存
int clean_bc_domain_db(struct bc_domain_db *db)
{
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

#endif

/// @brief 返回db中域名个数
size_t get_domain_name_num(struct bc_domain_db *db,enum domain_type type)
{
	if(NULL==db)
		return 0;
	if(DOMAIN_TYPE_NUM==type)
		return -EFAULT;
	return db->domain_names.domain_type_len[type];
}

/// @brief 返回db中第i个域名
/// @retval 成功0 失败错误代码的负值
int get_domain_name(struct domain_name *name,struct bc_domain_db *db,enum domain_type type,size_t index)
{
	/// 判断参数
	if(NULL==db)
		return -EINVAL;
	if(DOMAIN_TYPE_NUM==type)
		return -EFAULT;
	if(index>=db->domain_names.domain_type_len[type])
		return -EFAULT;
	/// 获取内存
	int err=0;
	if((err=read_bigmem(&db->mem,
					db->domain_names.domain_type_start[type]+index*sizeof(struct domain_name),
					name,sizeof(struct domain_name)))<0)
	{
		error_at_line(0,-err,__FILE__,__LINE__,"read_bigmem error");
		return err;
	}
	return 0;
}

/// @brief 设置db中第i个域名
/// @retval 成功0 失败错误代码的负值
int set_domain_name(const struct domain_name *name,struct bc_domain_db *db,enum domain_type type,size_t index)
{
	if(NULL==db)
		return -EINVAL;
	if(DOMAIN_TYPE_NUM==type)
		return -EFAULT;
	if(index>=db->domain_names.domain_type_len[type])
		return -EFAULT;
	/// 设置内存
	int err=0;
	if(err=write_bigmem(&db->mem,
				db->domain_names.domain_type_start[type]+index*sizeof(struct domain_name),
				name,sizeof(struct domain_name))<0)
	{
		error_at_line(0,-err,__FILE__,__LINE__,"write_bigmem error");
		return err;
	}
	return 0;
}

