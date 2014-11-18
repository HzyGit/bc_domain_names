#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "bc_domain_search.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(" a test for bc_domain_mem");

static int type=0;
static char *test_domain="www.baidu.com";
module_param(test_domain,charp,0000);
MODULE_PARM_DESC(test_domain,"test domain string");
module_param(type,int,0644);
MODULE_PARM_DESC(type,"type for domain");

static int __init test_init(void)
{
	int err=0;
	if(type<0)
		type=0;
	if(type>=DOMAIN_TYPE_NUM)
		type=DOMAIN_TYPE_NUM-1;
	if((err=bc_domain_match(test_domain,type))<0)
		printk(KERN_INFO "test %s in %d error\n",test_domain,type);
	else if(err==0)
		printk(KERN_INFO "test %s in %d not match\n",test_domain,type);
	else
		printk(KERN_INFO "test %s in %d match\n",test_domain,type);
	return 0;
}

static void __exit test_exit(void)
{
}

module_init(test_init);
module_exit(test_exit);
