#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>

#define VSER_MAJOR 256  		//主设备号用来区别设备类型
#define VSER_MINOR 0			//次设备号用来区别具体的设备
#define VSER_DEV_CNT 1			//申请设备的个数，连续申请，次设备号从指定的次设备号开始依次增加
#define VSER_DEV_NAME "vser"	//设备的名字

static int __init vser_int(void)
{
	int ret ;
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);//得到设备号VSER_MAJOR << 20 || VSER_MINOR
	ret = register_chrdev_region(dev, VSER_DEV_CNT, VSER_DEV_NAME); //注册字符设备

	if(ret)
		goto reg_err;

	printk("******** %s ********\n", __FUNCTION__);
	return 0;
reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
	dev_t dev;
	dev = MKDEV(VSER_MAJOR, VSER_MINOR);

	unregister_chrdev_region(dev, VSER_DEV_CNT);//注销字符设备
	printk("******** %s ********\n", __FUNCTION__);
}

module_init(vser_int);
module_exit(vser_exit);
MODULE_LICENSE("GPL");

