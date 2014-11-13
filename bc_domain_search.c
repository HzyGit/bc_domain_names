#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "bc_domain_search.h"

MODULE_AUTHOR("hzy(hzy.oop@gmail.com) bingchuan inc");
MODULE_DESCRIPTION("a module support bingchuan domains quick search");
MODULE_LICENSE("GPL");

static int bc_domain_search_init(void)
{
}

static void bc_domain_search_exit(void)
{
}

module_init(bc_domain_search_init);
module_exit(bc_domain_search_exit);

