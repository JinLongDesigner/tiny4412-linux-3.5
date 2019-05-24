#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>

#define VSER_MAJOR 256  		//主设备号用来区别设备类型
#define VSER_MINOR 0			//次设备号用来区别具体的设备
#define VSER_DEV_CNT 1			//申请设备的个数，连续申请，次设备号从指定的次设备号开始依次增加
#define VSER_DEV_NAME "virtualserial"	//设备的名字

static struct cdev vsdev;
/**
*define_kfifo-定义和初始化FIFO的宏
*@fifo：声明的fifo数据类型的名称
*@类型：FIFO元素的类型
*@大小：FIFO中的元素数，必须是2的幂
*
*注意：宏可用于全局和本地FIFO数据类型变量。
*/

DEFINE_KFIFO(vsfifo, char, 32);

static int vs_open(struct inode *inode, struct file *filp)
{
	return 0;
}
static int vs_release(struct inode * inode, struct file * filp)
{
	return 0;
}

ssize_t vs_read(struct file * filp, char __user * buf, size_t count, loff_t * ops)
{
	unsigned int copied = 0;

/**
*Kfifo-to-用户-将数据从FIFO复制到用户空间
*@FIFO:要使用的FIFO地址
*@到用户：必须复制数据的位置
*@len：目标缓冲区的大小
*@复制：指向输出变量的指针，用于存储复制的字节数
*
*此宏最多将@len字节从FIFO复制到
*用户内存空间，成功返回地址，失败返回0。
*
*注意，只有一个并发读卡器和一个并发读卡器
*编写器，使用这些宏不需要额外的锁定。
*/

	kfifo_to_user(&vsfifo, buf, count, &copied);
	
	return copied;
}
ssize_t vs_write(struct file * filp, const char __user * buf, size_t count, loff_t * ops)
{
	unsigned int copied = 0;

	kfifo_from_user(&vsfifo, buf, count, &copied);
	
	return copied;
}



static struct file_operations vser_ops = {
	.owner = THIS_MODULE,
	.open = vs_open,
	.release = vs_release,
	.read = vs_read,
	.write = vs_write,
};

static int __init vser_int(void)
{
	int ret ;
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);//得到设备号VSER_MAJOR << 20 || VSER_MINOR
	ret = register_chrdev_region(dev, VSER_DEV_CNT, VSER_DEV_NAME); //注册字符设备

	if (ret)
		goto reg_err;

	cdev_init(&vsdev, &vser_ops);		//初始化结构体，将vser_ops绑定vsdev
	vsdev.owner = THIS_MODULE;

	ret = cdev_add(&vsdev, dev, VSER_DEV_CNT);		//添加cdev到内核里面
	if (ret)
		goto add_err;

	printk("******** %s ********\n", __FUNCTION__);
	return 0;
add_err:
	unregister_chrdev_region(dev, VSER_DEV_CNT);
reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
	dev_t dev;
	dev = MKDEV(VSER_MAJOR, VSER_MINOR);


	cdev_del(&vsdev);
	unregister_chrdev_region(dev, VSER_DEV_CNT);//注销字符设备
	printk("******** %s ********\n", __FUNCTION__);
}

module_init(vser_int);
module_exit(vser_exit);
MODULE_LICENSE("GPL");


