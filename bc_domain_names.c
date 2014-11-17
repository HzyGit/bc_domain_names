/**
 * @file bc_domain_names.cpp
 * @brief 冰川域名操作程序，支持域名内存结构的建立，
 *       添加,读取,删除,搜索
 *       调用格式
 *       bc_domain_names [--build|--add|--del|--serach|--read] ...
 *
 * @author hzy.oop@gmail.com
 * @date 2014-11-14
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <getopt.h>

#include <bigmem.h>
#include "bc_domain_names.h"
#define MAX_PATH 512
const char *g_program="bc_domain_name";

/// 定义输出,考虑后面的cgi输出
#ifndef CGI
#define DB_PRINT(fmt,args...) printf(fmt,##args)
#else
#define DB_PRINT(fmt,args...) cgiprint(fmt,##args)
#endif

/// 调试输出
#define DEBUG_PRINT(err,fmt,args...)\
	do{\
		if(g_argu.is_debug)\
			error_at_line(0,err,__FILE__,__LINE__,fmt,##args);\
	}\
	while(0)
struct option g_opts[]={
	{"add",required_argument,NULL,'a'},
	{"del",required_argument,NULL,'d'},
	{"read",required_argument,NULL,'r'},
	{"search",required_argument,NULL,'s'},
	{"build",required_argument,NULL,'b'},
	{"clean",required_argument,NULL,'c'},
	{"debug",no_argument,NULL,'e'},
	{"help",no_argument,NULL,'h'},
	{NULL,0,NULL,0}
};

/// 域名类型名称
const char *g_domain_type[DOMAIN_TYPE_NUM]={
	"webpage",
	"blank",
	"download",
	"multimedia",
	"international",
	"shopping"
};

/// 命令行参数结构
enum handle_type{ADD_HANDLE=0,DEL_HANDLE,BUILD_HANDLE,SEARCH_HANDLE,READ_HANDLE
	,CLEAN_HANDLE,NUM_HANDLE};

struct argument
{
	enum handle_type handle;   ///< 处理类型
	bool is_debug;             ///< 是否开启debug
	union{
		/// domain,type结构
		struct {
			char name[DOMAIN_MAX_LENGTH];
			enum domain_type type;
		}domain;
		/// path,type结构
		struct {
			char path[MAX_PATH];
			enum domain_type type;
		}dbfile;
	}argu;
}g_argu;


static void usage(int err)
{
	if(EXIT_SUCCESS!=err)
		printf("Trye %s -h|--help for more information\n",g_program);
	else
	{
		printf("%s [--add|--del|--read|--search|--build|--clean|--help] ...\n\t 冰川域名数据库操作程序\n",g_program);
		printf("\n\t-a|--add domain_name,type 向type类别中增加域名\n");
		printf("\t-d|--del domain_name,type 在type中删除域名\n");
		printf("\t-r|--read type 显示数据库存储的域名\n");
		printf("\t-s|--search domain_name,type 在type类别中搜索域名\n");
		printf("\t-b|--build path,type 依据文件path内容重建type数据库");
		printf("\t-c|--clean type 清除数据库中的域名\n");
		printf("\t-h|--help 显示本信息\n");
		printf("\t-e|--debug 显示调试信息\n");
		printf("\n目前支持的type:\n");
		printf("\t%s:网页新闻类\n",g_domain_type[WEBPAGE_DOMAIN]);
		printf("\t%s:下载类\n",g_domain_type[DOWNLOAD_DOMAIN]);
		printf("\t%s:多媒体类\n",g_domain_type[MULTIMEDIA_DOMAIN]);
		printf("\t%s:国际域名类\n",g_domain_type[INTERNATIONAL_DOMAIN]);
		printf("\t%s: 银行类\n",g_domain_type[BLANK_DOMAIN]);
		printf("\t%s: 购物类\n",g_domain_type[SHOPPING_DOMAIN]);
	}
	exit(err);
}

/// @brief 将字符串解析为domain_type类型
/// @retval 成功domain_type值, 失败错误代码负值
static int parse_domain_type(const char *str)
{
	int i=0;
	if(NULL==str)
		return -EINVAL;
	for(i=0;i<DOMAIN_TYPE_NUM;i++)
	{
		if(strcasecmp(str,g_domain_type[i])==0)
			break;
	}
	return i<DOMAIN_TYPE_NUM?i:-EINVAL;
}

/// @brief 将字符串参数解析为argument中的union
/// @retval 成功0,失败错误代码的负值
static int parse_string(const char *str,struct argument *argu)
{
	if(NULL==str)
		return -EINVAL;
	/// 复制str
	char *buf=(char*)malloc(strlen(str)+1);
	if(NULL==buf)
		return -ENOMEM;
	memcpy(buf,str,strlen(str)+1);
	/// 以','为分隔进行str解析
	int err=0;
	char *saveptr=NULL;
	char *tok=NULL;
	do{
		/// 解析string
		if((tok=strtok_r(buf,",",&saveptr))==NULL)
		{
			err=-EINVAL;
			break;
		}
		if(argu->handle==BUILD_HANDLE)
		{
			strncpy(argu->argu.domain.name,tok,DOMAIN_MAX_LENGTH-1);
			argu->argu.domain.name[DOMAIN_MAX_LENGTH-1]='\0';
		}
		else
		{
			strncpy(argu->argu.dbfile.path,tok,MAX_PATH-1);
			argu->argu.dbfile.path[MAX_PATH-1]='\0';
		}
		/// 解析域名类型
		if((tok=strtok_r(NULL,",",&saveptr))==NULL)
		{
			err=-EINVAL;
			break;
		}
		if((err=parse_domain_type(tok))<0)
			break;
		if(argu->handle==BUILD_HANDLE)
			argu->argu.domain.type=(enum domain_type)err;
		else
			argu->argu.dbfile.type=(enum domain_type)err;

	}while(0);
	free(buf);
	return err;
}

/// @brief 解析程序的命令行参数
/// @param[in] argc,argv 命令行参数
static void parse_argument(int argc,char **argv)
{
	g_argu.is_debug=false;
	int ch;
	int err=0;
	bool no_argu=true;
	while((ch=getopt_long(argc,argv,":a:d:b:s:r:c:he",g_opts,NULL))!=-1)
	{
		switch(ch)
		{
			case 'a':
				g_argu.handle=ADD_HANDLE;
				if((err=parse_string(optarg,&g_argu))<0)
				{
					error_at_line(0,-err,__FILE__,__LINE__,"parse string for ADD error");
					usage(EXIT_FAILURE);
				}
				break;
			case 'd':
				g_argu.handle=DEL_HANDLE;
				if((err=parse_string(optarg,&g_argu))<0)
				{
					error_at_line(0,-err,__FILE__,__LINE__,"parse string for DEL error");
					usage(EXIT_FAILURE);
				}
				break;
			case 'b':
				g_argu.handle=BUILD_HANDLE;
				if((err=parse_string(optarg,&g_argu))<0)
				{
					error_at_line(0,-err,__FILE__,__LINE__,"parse string for build error");
					usage(EXIT_FAILURE);
				}
				break;
			case 'e':
				g_argu.is_debug=true;
				break;
			case 's':
				g_argu.handle=SEARCH_HANDLE;
				if((err=parse_string(optarg,&g_argu))<0)
				{
					error_at_line(0,-err,__FILE__,__LINE__,"parse string for SEARCH error");
					usage(EXIT_FAILURE);
				}
				break;
			case 'r':
				g_argu.handle=READ_HANDLE;
				if((err=parse_domain_type(optarg))<0)
				{
					error_at_line(0,-err,__FILE__,__LINE__,"parse string for READ error:%s",optarg);
					usage(EXIT_FAILURE);
				}
				g_argu.argu.domain.name[0]='\0';
				g_argu.argu.domain.type=(enum domain_type)err;
				break;
			case 'c':
				g_argu.handle=CLEAN_HANDLE;
				if((err=parse_domain_type(optarg))<0)
				{
					error_at_line(0,-err,__FILE__,__LINE__,"parse string for READ error:%s",optarg);
					usage(EXIT_FAILURE);
				}
				g_argu.argu.domain.name[0]='\0';
				g_argu.argu.domain.type=(enum domain_type)err;
				break;
			case 'h':
				usage(EXIT_SUCCESS);
			case '?':
				fprintf(stderr,"no support options:%c\n",optopt);
				usage(EXIT_FAILURE);
			case ':':
				fprintf(stderr,"no argument find for %c\n",optopt);
				usage(EXIT_FAILURE);
			default:
				fprintf(stderr,"argument error\n");
				usage(EXIT_FAILURE);
		};
		no_argu=false;
	};
	if(no_argu)
		usage(EXIT_SUCCESS);
}

/// @breif 校验type取值是否正确
/// @retval 0失败 1成功
static int check_type(enum domain_type type)
{
	if(type<0||type>=DOMAIN_TYPE_NUM)
	{
		error_at_line(0,EINVAL,__FILE__,__LINE__,"type error");
		return 0;
	}
	return 1;
}

/// @breif 打开本地file
static FILE* local_open(const char *path)
{
	return fopen(path,"r");
}

/// @breif 打开远程文件
static FILE * remote_open(const char *url)
{
	return NULL;
}

/// @brief 依据path获取fp指针, path为域名文件，或远程url域名文件
/// @retval 成功fp指针  失败返回NULL
static FILE* domain_file_open(const char *path)
{
	const char *str_id="http://";
	if(strncasecmp(path,str_id,strlen(str_id))==0)
		return remote_open(path);
	return local_open(path);
}

/// @breif 向bc_domain数据库中添加数据
static int add_bc_domain(const struct argument *argu,struct bc_domain_db *db)
{
	enum domain_type type=argu->argu.domain.type;
	if(check_type(type)!=1)
		return -EINVAL;
	DEBUG_PRINT(0,"add db for %s,%s",
			argu->argu.domain.name,g_domain_type[type]);
	/// 判断是否需要整理内存碎片
	if(db->domain_names.domain_type_len[type]>=
			db->domain_names.domain_type_max_len[type])
		defrag_mentation(db,type);
	if(db->domain_names.domain_type_len[type]>=
			db->domain_names.domain_type_max_len[type])
	{
		DEBUG_PRINT(ENOMEM,"no mem use");
		return -ENOMEM;
	}
	int index=db->domain_names.domain_type_len[type]++;
	/// 写入新域名
	struct domain_name name;
	size_t len=strlen(argu->argu.domain.name);
	const char *buf=argu->argu.domain.name;
	if(len>=DOMAIN_MAX_LENGTH-1)
		len=DOMAIN_MAX_LENGTH-1;
	memcpy(name.name,buf,len);
	name.name[len]='\0';
	name.is_vaild=true;
	int err=0;
	if((err=set_domain_name(&name,db,type,index))<0)
	{
		DEBUG_PRINT(-err,"write %s into type(%d) error",name.name,index);
		goto clean_len;
	}
	/// 保存bc_domain_names
	if((err=save_bc_domain_names(db))<0)
	{
		DEBUG_PRINT(-err,"save bc_domain_names error");
		goto clean_len;
	}
	return 0;
clean_len:
	db->domain_names.domain_type_len[type]=index;
	return err;
}

/// @breif 向bc_domain数据库中删除数据
static int del_bc_domain(const struct argument *argu,struct bc_domain_db *db)
{
	enum domain_type type=argu->argu.domain.type;
	if(check_type(type)!=1)
		return -EINVAL;
	DEBUG_PRINT(0,"del db for %s,%s",
			argu->argu.domain.name,g_domain_type[type]);
	/// 设置domain_name对象
	struct domain_name name;
	const char *buf=argu->argu.domain.name;
	size_t len=strlen(buf);
	len=len>=DOMAIN_MAX_LENGTH-1?DOMAIN_MAX_LENGTH-1:len;
	memcpy(name.name,buf,len);
	name.name[len]='\0';
	name.is_vaild=false;
	/// 查找
	size_t sum=get_domain_name_num(db,type);
	int i=0;
	int err=0;
	for(i=0;i<sum;++i)
	{
		struct domain_name n;
		if((err=get_domain_name(&n,db,type,i))<0)
		{
			DEBUG_PRINT(0,"get domain index %d in %s error",
					i,g_domain_type[type]);
			continue;
		}
		if(strcasecmp(n.name,name.name)!=0)
			continue;
		if((err=set_domain_name(&name,db,type,i))<0)
		{
			DEBUG_PRINT(0,"set domain index %d in %s error",
					i,g_domain_type[type]);
			continue;
		}
		DEBUG_PRINT(0,"del domain %s in index %d ok",
				name.name,i);
	}
	return 0;
}

/// @breif 搜索bc_domain数据库数据
static int search_bc_domain(const struct argument *argu,struct bc_domain_db *db)
{
	enum domain_type type=argu->argu.domain.type;
	if(check_type(type)!=1)
		return -EINVAL;
	DEBUG_PRINT(0,"search db for %s,%s",
			argu->argu.domain.name,g_domain_type[type]);
	/// 设置domain_name对象
	struct domain_name name;
	const char *buf=argu->argu.domain.name;
	size_t len=strlen(buf);
	len=len>=DOMAIN_MAX_LENGTH-1?DOMAIN_MAX_LENGTH-1:len;
	memcpy(name.name,buf,len);
	name.name[len]='\0';
	name.is_vaild=true;
	/// 查找
	size_t sum=get_domain_name_num(db,type);
	int i=0;
	int err=0;
	for(i=0;i<sum;++i)
	{
		struct domain_name n;
		if((err=get_domain_name(&n,db,type,i))<0)
		{
			DEBUG_PRINT(0,"get domain index %d in %s error",
					i,g_domain_type[type]);
			continue;
		}
		if(strcasestr(n.name,name.name)=='\0')
			continue;
		DB_PRINT("%d %s %s\n",i,n.name,n.is_vaild?"vaild":"no_vaild");
	}
	return 0;
}

/// @brief 重建bc_domain数据库中数据
static int build_bc_domain_db(const struct argument *argu,struct bc_domain_db *db)
{
	enum domain_type type=argu->argu.dbfile.type;
	if(check_type(type)!=1)
		return -EINVAL;
	DEBUG_PRINT(0,"build db for %s,%s",
			argu->argu.dbfile.path,g_domain_type[type]);
	/// 依据path/url获取fp
	FILE *fp=domain_file_open(argu->argu.dbfile.path);
	if(NULL==fp)
	{
		error_at_line(0,errno,__FILE__,__LINE__,"open %s failed",argu->argu.dbfile.path);
		return -errno;
	}
	/// 重置bc_domain_names
	db->domain_names.domain_type_len[type]=0;
	/// 读取文件重建db
	char *line=NULL;
	size_t n=0;
	int err=0;
	while(getline(&line,&n,fp)!=-1)
	{
		size_t index=db->domain_names.domain_type_len[type]++;
		if(index>=db->domain_names.domain_type_max_len[type])
		{
			DEBUG_PRINT(0,"domain in file is too max,MAX:",
					db->domain_names.domain_type_max_len[type]);
			err=-EFAULT;
			break;
		}
		/// 写入db
		size_t len=strlen(line)-1;
		struct domain_name name;
		len=len>=DOMAIN_MAX_LENGTH-1?DOMAIN_MAX_LENGTH-1:len;
		memcpy(name.name,line,len);
		name.name[len]='\0';
		name.is_vaild=true;
		if((err=set_domain_name(&name,db,type,index))<0)
		{
			db->domain_names.domain_type_len[type]--;
			DEBUG_PRINT(-err,"set domain name %s in %d error",
					name.name,index);
			err=0;
			continue;
		}
	}
	/// 保存bc_domain_name
	if((err=save_bc_domain_names(db))<0)
		DEBUG_PRINT(-err,"save bc_domain_names error");
file_close:
	if(NULL!=fp)
	{
		fclose(fp);
		fp=NULL;
	}
	return err;
}

/// @brief 读取bc_domain数据库中
static int clean_bc_domain_db(const struct argument *argu,struct bc_domain_db *db)
{
	enum domain_type type=argu->argu.domain.type;
	if(check_type(type)!=1)
		return -EINVAL;
	DEBUG_PRINT(0,"read db for %s",
			g_domain_type[type]);
	/// 清除type数据库
	db->domain_names.domain_type_len[type]=0;
	/// 保存
	int err=0;
	if((err=save_bc_domain_names(db))<0)
		DEBUG_PRINT(-err,"save bc_domain_names error");
	return err;
}

/// @brief 读取bc_domain数据库中
static int read_bc_domain_db(const struct argument *argu,struct bc_domain_db *db)
{
	enum domain_type type=argu->argu.domain.type;
	if(check_type(type)!=1)
		return -EINVAL;
	DEBUG_PRINT(0,"read db for %s",
			g_domain_type[type]);
	/// 获取数据数目
	int err=0;
	int i=0;
	size_t sum=get_domain_name_num(db,type);
	for(i=0;i<sum;++i)
	{
		struct domain_name name;
		if((err=get_domain_name(&name,db,type,i))<0)
		{
			DEBUG_PRINT(-err,"read domain %d of %s error",
					i,g_domain_type[type]);
			continue;
		}
		DB_PRINT("%d %s %s\n",i,name.name,
				name.is_vaild?"valid":"no_valid");
	}
	DB_PRINT("-------------------\ntotal:%d\n",sum);
	return 0;
}

/// @brief 依据struct argument完成对bc_domain数据库的处理
static int bc_domain_handle(const struct argument *argu,struct bc_domain_db *db)
{
	bool is_update=false;   ///< 是否设置db更新标志
	int err=0;
	if(NULL==db||NULL==argu)
		return -EINVAL;
	switch(argu->handle)
	{
		case ADD_HANDLE:
			DEBUG_PRINT(0,"begin add handle for %s,%s",
					argu->argu.domain.name,
					g_domain_type[argu->argu.domain.type]);
			err=add_bc_domain(argu,db);
			is_update=true;
			break;
		case DEL_HANDLE:
			DEBUG_PRINT(0,"begin del handle for %s,%s",
					argu->argu.domain.name,
					g_domain_type[argu->argu.domain.type]);
			err=del_bc_domain(argu,db);
			is_update=true;
			break;
		case SEARCH_HANDLE:
			DEBUG_PRINT(0,"begin search handle for %s,%s",
					argu->argu.domain.name,
					g_domain_type[argu->argu.domain.type]);
			err=search_bc_domain(argu,db);
			is_update=false;
			break;
		case READ_HANDLE:
			DEBUG_PRINT(0,"%s","begin read handle");
			err=read_bc_domain_db(argu,db);
			is_update=false;
			break;
		case CLEAN_HANDLE:
			DEBUG_PRINT(0,"%s","begin clean handle");
			err=clean_bc_domain_db(argu,db);
			is_update=true;
			break;
		case BUILD_HANDLE:
			DEBUG_PRINT(0,"begin build handle for %s,%s",
					argu->argu.dbfile.path,
					g_domain_type[argu->argu.dbfile.type]);
			err=build_bc_domain_db(argu,db);
			is_update=true;
			break;
		default:
			fprintf(stderr,"cannot support current handle,and exit!\n");
			err=-EINVAL;
			break;
	}
	if(0!=err)
		return err;
	/// 设置更新标志
	if(is_update)
	{
		if((err=set_update_domain_db(db,is_update))<0)
			error_at_line(0,-err,__FILE__,__LINE__,"set update domain error");
		DEBUG_PRINT(0,"set update flags");
	}
	return err;
}

int main(int argc,char **argv)
{
	int err=0;

	/// 载入冰川域名数据库
	struct bc_domain_db db; 
	if((load_bc_domain_db(PROC_PATH,&db))<0)
	{
		error_at_line(0,-err,__FILE__,__LINE__,"bc_domain_db load error");
		return err;
	}
	/// 解析命令行参数
	parse_argument(argc,argv);
	/// 处理结果
	if((err=bc_domain_handle(&g_argu,&db))<0)
		error_at_line(0,-err,__FILE__,__LINE__,"bc_domain_handle error");
	else
		DEBUG_PRINT(0,"bc_domain_handle ok!");
	return 0;
}
