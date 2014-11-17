#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "bc_domain_search.h"
#include "bc_domain_names.h"

#define NAME "bc_domain_mem"
#define BC_DOMAIN_MEM_VERSION "v1.0"

MODULE_AUTHOR("hzy(hzy.oop@gmail.com) bingchuan inc");
MODULE_DESCRIPTION("a module support bingchuan domains cache and quick search");
MODULE_LICENSE("GPL");

/// 冰川域名数据库
struct bc_domain_db db;

static int bc_domain_search_init(void)
{
	int err=0;
	/// 初始化bc_domain_db
	if((err=init_bc_domain_db(&db))<0)
		printk(KERN_INFO"init bc_domain db error");

	/// 输出信息
	if(0==err)
		printk(KERN_INFO"%s(%s) load ok\n",NAME,BC_DOMAIN_MEM_VERSION);
	else
		printk(KERN_INFO"%s(%s) load error\n",NAME,BC_DOMAIN_MEM_VERSION);
	return err;
}

static void bc_domain_search_exit(void)
{
	int err=0;
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

