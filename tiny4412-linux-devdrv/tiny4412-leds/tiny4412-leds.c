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
#include <linux/errno.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "io_cmd.h"

//每一个cdev至少要一个设备号：主设备号+次设备号
/*这两个主次设备号，在静态申请设备号时使用*/
//#define TINY4412_LEDS_MAJOR 220
//#define TINY4412_LEDS_MINOR 0
#define DEVICENAME "Tiny4412-leds"

struct led_protocal
{
	int led_num;
	int light;
	
	int status;
};

//定义一个led设备结构体
struct leddev  
{
	unsigned int count;											//被调用的次数
	unsigned int status;										//LED的状态

	unsigned int LEDCON;										//LED控制寄存器
	unsigned int LEDDAT;										//LED数据寄存器
	void * __iomem 	peripdev;									//定义的指针指向外设
	
	dev_t dev_num;												//LED设备号
	struct cdev dev;											//字符设备结构体
	struct class *class;										
	struct device *device;										

	int (*leds_on)(struct leddev * leddev, int num);			//led打开函数
	int (*leds_off)(struct leddev * leddev, int num);			//led关闭函数
};

struct leddev * tiny4412_leds;

/*************硬件相关*********************/
void exynos4412_leds_init(struct leddev * leddev)
{
	leddev->LEDCON = readl(leddev->peripdev + 0x2e0);
	leddev->LEDCON &= ~0xffff;
	leddev->LEDCON |= 0x1111;
	writel(leddev->LEDCON, leddev->peripdev + 0x2e0);

	
	leddev->LEDDAT = readl(leddev->peripdev + 0x2e4);
	leddev->LEDCON |= 0xf;
	writel(leddev->LEDCON, leddev->peripdev + 0x2e4);
}
void exynos4412_leds_exit(struct leddev * leddev)
{
	leddev->LEDDAT = readl(leddev->peripdev + 0x2e4);
	leddev->LEDCON |= 0xf;
	writel(leddev->LEDCON, leddev->peripdev + 0x2e4);
}

int exynos4412_leds_on(struct leddev * leddev, int num)
{
	if(num > 3 && num < 0)
	{
		return -EINVAL;
	}

	leddev->LEDDAT = readl(leddev->peripdev + 0x2e4);
	leddev->LEDCON &= ~(1 << num);
	writel(leddev->LEDCON, leddev->peripdev + 0x2e4);
	return 0;
	
}
int exynos4412_leds_off(struct leddev * leddev, int num)
{
	if(num > 3 && num < 0)
	{
		return -EINVAL;
	}

	leddev->LEDDAT = readl(leddev->peripdev + 0x2e4);
	leddev->LEDCON |= (1 << num);
	writel(leddev->LEDCON, leddev->peripdev + 0x2e4);
	return 0;
}
/*************硬件相关*********************/

/*************硬件无相关*********************/
int tiny4412_leds_open(struct inode * node, struct file * fp)
{
	struct leddev * tmpleds = NULL;
	fp->private_data = tiny4412_leds;

//	tmpleds = fp->private_data;
	tmpleds = container_of(node->i_cdev, struct leddev, dev);

	//加锁
	if (tmpleds->count)
	{
		return -EBUSY;
	}

	tmpleds->count = 1;
	//解锁
	return 0;
}

ssize_t tiny4412_leds_read(struct file * fp, char __user * buff, size_t size, loff_t * off_set)
{
	int ret = 0;

	struct leddev * tmpleds = fp->private_data;
	struct led_protocal led_p;

	led_p.status = tmpleds->status;
	if (size != sizeof(led_p))
	{
		return -EINVAL;
	}
	ret = copy_to_user(buff, &led_p, sizeof(led_p));
	if(ret)
	{
		return -EFAULT;
	}
	return size;
}

ssize_t tiny4412_leds_write(struct file *fp, const char __user *buff, size_t size, loff_t *off_set)
{
	int ret = 0;

	struct leddev * tmpleds = fp->private_data;
	struct led_protocal led_p;

	if (size != sizeof(led_p))
	{
		return -EINVAL;
	}
	ret = copy_from_user(&led_p, buff, sizeof(led_p));
	if(ret)
	{
		return -EFAULT;
	}

	if(led_p.light)
	{
		tmpleds->leds_on(tmpleds, led_p.led_num);
		tmpleds->status |= 1 << (led_p.led_num);
	}
	else
	{
		tmpleds->leds_off(tmpleds, led_p.led_num);
		tmpleds->status &= ~(1 << (led_p.led_num));
	}
	
	return size;
}



long tiny4412_leds_ioctl(struct file * fp, unsigned int cmd, unsigned long arg)
{
	struct leddev * tmpleds = fp->private_data;
	
	switch (cmd)
	{
		case IOCTL_LED_ALLON:
			tmpleds->leds_on(tmpleds, 0);
			tmpleds->leds_on(tmpleds, 1);
			tmpleds->leds_on(tmpleds, 2);
			tmpleds->leds_on(tmpleds, 3);
			break;
		case IOCTL_LED_ONEON:
			tmpleds->leds_on(tmpleds, arg);
			break;
		case IOCTL_LED_ONEOFF:
			tmpleds->leds_off(tmpleds, arg);
			break;
		case IOCTL_LED_ALLOFF:
			tmpleds->leds_off(tmpleds, 0);
			tmpleds->leds_off(tmpleds, 1);
			tmpleds->leds_off(tmpleds, 2);
			tmpleds->leds_off(tmpleds, 3);
			break;
		default:
			tmpleds->leds_off(tmpleds, 0);
			tmpleds->leds_off(tmpleds, 1);
			tmpleds->leds_off(tmpleds, 2);
			tmpleds->leds_off(tmpleds, 3);
			break;
	}
	return 0;
}

int tiny4412_leds_release(struct inode * node, struct file * fp)
{
	struct leddev * tmpleds = NULL;
	tmpleds = container_of(node->i_cdev, struct leddev, dev);

	//加锁
	if (!tmpleds->count)
	{
		return -ENODEV;
	}
	tmpleds->count = 0;
	//解锁
	return 0;
}




//字符设备操作函数集
struct file_operations tiny4412_leds_fops = {
		.owner = THIS_MODULE,
		.open = tiny4412_leds_open,
		.read = tiny4412_leds_read,
		.write = tiny4412_leds_write,
		.unlocked_ioctl = tiny4412_leds_ioctl,
		.release = tiny4412_leds_release,
};


//初始化函数，模块加载的时候执行
static __init int tiny4412_leds_init(void)
{
	int ret = 0;

	//申请tiny4412_leds设备空间
	tiny4412_leds = kmalloc(sizeof(struct leddev), GFP_KERNEL);
	if (tiny4412_leds == NULL)
	{
		printk("kmalloc memory faild.\n");
		goto kzalloc_failed;
	}
	memset(tiny4412_leds, 0,sizeof(struct leddev));
	
	//生成字符设备设备号
	//tiny4412_leds->dev_num = MKDEV(TINY4412_LEDS_MAJOR, TINY4412_LEDS_MINOR);
	
	//注册字符设备号
	/*
		静态注册设备号，先到/proc/devices 查看没有使用的设备号，然后随意指定没有使用的设备号
		register_chrdev_region(dev_t from, unsigned count, const char * name)
					参数说明：1.设备号 2. 注册设备号的个数 3.设备的名字
		动态分配设备号，就不需要考虑这些。
		alloc_chrdev_region(dev_t * dev, unsigned baseminor, unsigned count, const char * name):动态注册设备号
					参数说明：1.设备号 2.次设备号（要指定） 3. 注册设备号的个数 4.设备的名字
	*/
	//register_chrdev_region(dev_num, 1, DEVICENAME);

	ret = alloc_chrdev_region(&tiny4412_leds->dev_num, 0, 1, DEVICENAME);
	if (ret < 0)
	{
		printk("alloc chrdev failed.\n");
		goto alloc_chrdev_failed;
	}


	//初始化设备结构
	cdev_init(&tiny4412_leds->dev, &tiny4412_leds_fops);
	tiny4412_leds->dev.owner = THIS_MODULE;
	tiny4412_leds->dev.ops = &tiny4412_leds_fops;

	//注册字符设备到内核
	ret = cdev_add(&tiny4412_leds->dev, tiny4412_leds->dev_num, 1);
	if (ret)
	{
		printk("Adding tiny4412_leds to kernel is failed.\n");
		goto cdev_add_failed;
	}
	/*
	  下面两行是创建了一个总线类型，会在/sys/class下生成tiny4412_leds目录
	  这里的还有一个主要作用是执行device_create后会在/dev/下自动生成
	  tiny4412_leds设备节点。而如果不调用此函数，如果想通过设备节点访问设备
	  需要手动mknod来创建设备节点后再访问。
	*/
	tiny4412_leds->class = class_create(THIS_MODULE, DEVICENAME);
	if (tiny4412_leds->class == NULL)
	{
		printk("Creating tiny4412 leds class is failed.\n");
		goto class_create_failed;
	}

	tiny4412_leds->device = device_create(tiny4412_leds->class, NULL, tiny4412_leds->dev_num, NULL, DEVICENAME);
	if (tiny4412_leds->device == NULL)
	{
		printk("Creating tiny4412 leds device is failed.\n");
		goto device_create_failed;
	}


	tiny4412_leds->peripdev = ioremap(0x11000000, SZ_4K);
	if (tiny4412_leds->peripdev == NULL)
	{
		goto ioremap_failed;
	}

	tiny4412_leds->leds_on = &exynos4412_leds_on;
	tiny4412_leds->leds_off = &exynos4412_leds_off;
	
	exynos4412_leds_init(tiny4412_leds);

	printk("********** %d ******* %d ********\n", MAJOR(tiny4412_leds->dev_num), MINOR(tiny4412_leds->dev_num));
	printk("***************tiny4412_leds inits.******************\n");
	return 0;

ioremap_failed:
	device_del(tiny4412_leds->device);
	return -ENOMEM;

device_create_failed:
	class_destroy(tiny4412_leds->class);
	return -ENOMSG;

class_create_failed:
	return -ENOMSG;
	
cdev_add_failed:
	unregister_chrdev_region(tiny4412_leds->dev_num, 1);
	kfree(tiny4412_leds);
	return ret;
	
alloc_chrdev_failed:
	kfree(tiny4412_leds);
	return ret;

kzalloc_failed:
	return -ENOMEM;

}
//卸载函数，模块卸载时执行
static __exit void tiny4412_leds_exit(void)
{
	exynos4412_leds_exit(tiny4412_leds);
	device_del(tiny4412_leds->device);
	class_destroy(tiny4412_leds->class);
	cdev_del(&tiny4412_leds->dev);
	unregister_chrdev_region(tiny4412_leds->dev_num, 1);
	kfree(tiny4412_leds);
	printk("***************tiny4412_leds exits.******************\n");
}


module_init(tiny4412_leds_init);
module_exit(tiny4412_leds_exit);
MODULE_LICENSE("GPL");


