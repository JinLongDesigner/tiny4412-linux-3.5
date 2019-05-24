#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#define MSIC_NAME "tiny4412_wtd"

static 

static int __init tiny4412_wtd_init(void)
{
	return 0;
}

static void __exit tiny4412_wtd_exit(void)
{

}

module_init(tiny4412_wtd_init);
module_exit(tiny4412_wtd_exit);
MODULE_LICENSE("GPL");




