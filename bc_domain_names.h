/*
 * @file bc_domain_names.h
 * @breif 域名结构的定义文件
 * @author hzy.oop@gmail.com
 * @date 2014-11-14
 */

#ifndef _BC_DOMAIN_NAME_H_
#define _BC_DOMAIN_NAME_H_

/// 头文件


#ifndef USER_SPACE
#include <bingchuan/bigmem.h>
#else
#include <stdbool.h>
#include <bigmem.h>
#endif

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
	bool is_update;                               ///< 设置更新标识
	struct domain_name names[];     ///< 域名数组
};

/// 域名集 数据结构
struct bc_domain_db
{
	struct bc_domain_names domain_names;  
	struct big_mem mem;
};

#ifndef USER_SPACE

/// @brief 内核函数，初始化bc_domain_db
int init_bc_domain_db(struct bc_domain_db *db);
/// @brief 内核函数, 清除bc_domain_db，并释放内存
int clean_bc_domain_db(struct bc_domain_db *db);

#else   ///USE_SPACE

/// @brief 用户函数,从文件中反序列化db结构
int load_bc_domain_db(const char *path,struct bc_domain_db *db);
void unload_bc_domain_db(struct bc_domain_db *db);
/// @breif 整理数据结构中的内存，避免碎片
void defrag_mentation(struct bc_domain_db *db,enum domain_type type);


#endif  /// USER_SPACE

/// @brief 设置更新标识
int set_update_domain_db(struct bc_domain_db *db,bool isupdate);

/// @brief 返回db中域名个数
size_t get_domain_name_num(struct bc_domain_db *db,enum domain_type type);

/// @brief 返回db中第i个域名
/// @retval 成功0 失败错误代码的负值
int get_domain_name(struct domain_name *name,struct bc_domain_db *db,enum domain_type type,size_t index);

/// @brief 设置db中第i个域名
/// @retval 成功0 失败错误代码的负值
int set_domain_name(const struct domain_name *name,struct bc_domain_db *db,enum domain_type type,size_t index);

/// @briefe 保存bc_domain_names结构
/// @retval 成功返回0 失败返回错误代码负值
int save_bc_domain_names(struct bc_domain_db *db);
#endif // _BC_DOMAIN_NAME_H_
