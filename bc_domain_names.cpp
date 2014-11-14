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

#include "bc_domain_names.h"
#define MAX_PATH 512
const char *g_program="bc_domain_name";

struct option g_opts[]={
	{"add",required_argument,NULL,'a'},
	{"del",required_argument,NULL,'d'},
	{"read",no_argument,NULL,'r'},
	{"search",required_argument,NULL,'s'},
	{"build",required_argument,NULL,'b'},
	{"debug",no_argument,NULL,'e'},
	{"help",no_argument,NULL,'h'},
	{NULL,0,NULL,0}
};

/// 域名类型名称
const char *g_domain_type[DOMAIN_TYPE_NUM]={
	"webpath",
	"blank",
	"download",
	"multimedia",
	"international",
	"shopping"
};

/// 命令行参数结构
enum handle_type{ADD_HANDLE=0,DEL_HANDLE,BUILD_HANDLE,SEARCH_HANDLE,READ_HANDLE,NUM_HANDLE};

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

/// 调试输出
#define DEBUG_PRINT(err,fmt,args...)\
	do{\
		if(g_argu.is_debug)\
			error_at_line(0,err,__FILE__,__LINE__,fmt,##args);\
	}\
	while(0)

static void usage(int err)
{
	if(EXIT_SUCCESS!=err)
		printf("Trye %s -h|--help for more information\n",g_program);
	else
	{
		printf("%s [--add|--del|--read|--search|--build|--help] ...\n\t 冰川域名数据库操作程序\n",g_program);
		printf("\n\t-a|--add domain_name,type 向type类别中增加域名\n");
		printf("\t-d|--del domain_name,type 在type中删除域名\n");
		printf("\t-r|--read 显示数据库存储的域名");
		printf("\t-s|--search domain_name,type 在type类别中搜索域名\n");
		printf("\t-b|--build path,type 依据文件path内容重建type数据库");
		printf("\t-h|--help 显示本信息\n");
		printf("\t-e|--debug 显示调试信息\n");
		printf("\n目前支持的type:\n");
		printf("\t%s:网页新闻类\n",g_domain_type[WEBPAGE_DOMAIN]);
		printf("\t%s:下载类\n",g_domain_type[BLANK_DOMAIN]);
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
	while((ch=getopt_long(argc,argv,":a:d:b:s:rh",g_opts,NULL))!=-1)
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
				if((err=parse_string(optarg,&g_argu))<0)
				{
					error_at_line(0,-err,__FILE__,__LINE__,"parse string for READ error");
					usage(EXIT_FAILURE);
				}
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

/// @breif 向bc_domain数据库中添加数据
static int add_bc_domain(const struct argument *argu)
{
	return 0;
}

/// @breif 向bc_domain数据库中删除数据
static int del_bc_domain(const struct argument *argu)
{
	return 0;
}

/// @breif 搜索bc_domain数据库数据
static int search_bc_domain(const struct argument *argu)
{
	return 0;
}

/// @brief 重建bc_domain数据库中数据
static int build_bc_domain_db(const struct argument *argu)
{
	return 0;
}

/// @brief 读取bc_domain数据库中
static int read_bc_domain_db()
{
	return 0;
}

/// @brief 依据struct argument完成对bc_domain数据库的处理
static int bc_domain_handle(const struct argument *argu)
{
	int err=0;
	switch(argu->handle)
	{
		case ADD_HANDLE:
			DEBUG_PRINT(0,"begin add handle for %s,%s",
					argu->argu.domain.name,
					g_domain_type[argu->argu.domain.type]);
			err=add_bc_domain(argu);
			break;
		case DEL_HANDLE:
			DEBUG_PRINT(0,"begin del handle for %s,%s",
					argu->argu.domain.name,
					g_domain_type[argu->argu.domain.type]);
			err=del_bc_domain(argu);
			break;
		case SEARCH_HANDLE:
			DEBUG_PRINT(0,"begin search handle for %s,%s",
					argu->argu.domain.name,
					g_domain_type[argu->argu.domain.type]);
			err=search_bc_domain(argu);
			break;
		case READ_HANDLE:
			DEBUG_PRINT(0,"%s","begin read handle");
			err=read_bc_domain_db();
			break;
		case BUILD_HANDLE:
			DEBUG_PRINT(0,"begin build handle for %s,%s",
					argu->argu.dbfile.path,argu->argu.dbfile.type);
			err=build_bc_domain_db(argu);
			break;
		default:
			fprintf(stderr,"cannot support current handle,and exit!\n");
			err=-EINVAL;
			break;
	}
	return err;
}

int main(int argc,char **argv)
{
	int err=0;
	parse_argument(argc,argv);
	if((err=bc_domain_handle(&g_argu))<0)
		error_at_line(0,-err,__FILE__,__LINE__,"bc_domain_handle error");
	DEBUG_PRINT(0,"bc_domain_handle ok!");
	return 0;
}
