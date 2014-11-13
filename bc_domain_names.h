/*
 * 域名结构的定义文件
 */
#ifndef _BC_DOMAIN_NAME_H_
#define _BC_DOMAIN_NAME_H_

#define DOMAIN_MAX_LENGTH 128
/// 域名类型
enum domain_type{WEBPAGE=0,    ///< 网页类
	BLANK,                     ///< 银行类
	DOWNLOAD,                  ///< 下载类
	MULTIMEDIA,                ///< 多媒体
	INTERNATIONAL,             ///< 国际类
	SHOPPING,                  ///< 购物
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
