/*
 * @file bc_domain_names.h
 * @breif 域名结构的定义文件
 * @author hzy.oop@gmail.com
 * @date 2014-11-14
 */

#ifndef _BC_DOMAIN_NAME_H_
#define _BC_DOMAIN_NAME_H_

/// bigmem的内存proc映射
#define PROC_NAME "bc_domain_mem"
#define PROC_PATH "/proc/"PROC_NAME

/// 域名的最大长度
#define DOMAIN_MAX_LENGTH 128

/// 每类域名的最大个数
#define WEBPAGE_DOMAIN_MAX_COUNT 2100
#define BLANK_DOMAIN_MAX_COUNT 600
#define DOWNLOAD_DOMAIN_MAX_COUNT 400
#define MULTIMEDIA_DOMAIN_MAX_COUNT 700
#define INTERNATIONAL_DOMAIN_MAX_COUNT 300
#define SHOPPING_DOMAIN_MAX_COUNT 800

/// 域名最大个数
#define DOMAIN_MAX_COUNT (WEBPAGE_DOMAIN_MAX_COUNT+\
		BLANK_DOMAIN_MAX_COUNT +\
		DOWNLOAD_DOMAIN_MAX_COUNT+\
		MULTIMEDIA_DOMAIN_MAX_COUNT +\
		INTERNATIONAL_DOMAIN_MAX_COUNT+\
		SHOPPING_DOMAIN_MAX_COUNT)

/// 域名类型
enum domain_type{WEBPAGE_DOMAIN=0,    ///< 网页类
	BLANK_DOMAIN,                     ///< 银行类
	DOWNLOAD_DOMAIN,                  ///< 下载类
	MULTIMEDIA_DOMAIN,                ///< 多媒体
	INTERNATIONAL_DOMAIN,             ///< 国际类
	SHOPPING_DOMAIN,                  ///< 购物
	DOMAIN_TYPE_NUM
};

/// 域名
struct domain_name
{
	bool is_vaild;                 ///< 是否有效
	char name[DOMAIN_MAX_LENGTH];  ///< 域名
};

/// 域名集 分类结构
struct bc_domain_names
{
	size_t domain_type_start[DOMAIN_TYPE_NUM];   ///< 各类 域名集合 起始索引
	size_t domain_type_len[DOMAIN_TYPE_NUM];     ///< 各类 域名集合 的长度
	size_t domain_type_max_len[DOMAIN_TYPE_NUM];  ///< 各类域名 集合最大长度

	struct domain_name names[];     ///< 域名数组
};

#endif // _BC_DOMAIN_NAME_H_
