#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>

#define VSER_MAJOR 256  		//主设备号用来区别设备类型
#define VSER_MINOR 0			//次设备号用来区别具体的设备
#define VSER_DEV_CNT 2			//申请设备的个数，连续申请，次设备号从指定的次设备号开始依次增加
#define VSER_DEV_NAME "virtualserial"	//设备的名字


//static struct cdev vsdev;

/**
*define_kfifo-定义和初始化FIFO的宏
*@fifo：声明的fifo数据类型的名称
*@类型：FIFO元素的类型
*@大小：FIFO中的元素数，必须是2的幂
*
*注意：宏可用于全局和本地FIFO数据类型变量。
*/

static DEFINE_KFIFO(vsfifo0, char, 32);
static DEFINE_KFIFO(vsfifo1, char, 32);

struct vsdev {
	struct kfifo *fifo;
	struct cdev cdev;
};

static struct vsdev vsdev[2];

/*
在“struct inode”的开头保留大部分只读和经常访问的字段（尤其是对于rcu路径查找和“stat”数据）。
*/

static int vs_open(struct inode *inode, struct file *filp)
{
#if 0
	switch (MINOR(inode->i_rdev))
	{
		default:
		case 0: 
			filp->private_data = &vsfifo0;
			break;
		case 1:
			filp->private_data = &vsfifo1;
	}
#endif
	filp->private_data = container_of(inode->i_cdev, struct vsdev, cdev); //根据结构成员的地址得到结构的起始地址
	return 0;
}
static int vs_release(struct inode * inode, struct file * filp)
{
	return 0;
}

ssize_t vs_read(struct file * filp, char __user * buf, size_t count, loff_t * ops)
{
	unsigned int copied = 0;
	int ret = 0;
	struct vsdev *dev = filp->private_data;

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

	ret = kfifo_to_user(dev->fifo, buf, count, &copied);
	
	if (!ret)
	{
		goto kfifo_err;
	}
	
	return copied;
kfifo_err:
	return ret;

}
ssize_t vs_write(struct file * filp, const char __user * buf, size_t count, loff_t * ops)
{
	unsigned int copied = 0;
	int ret = 0;

	struct vsdev *dev = filp->private_data;

	ret = kfifo_from_user(dev->fifo, buf, count, &copied);

	if (!ret)
	{
		goto kfifo_err;
	}
	
	return copied;
kfifo_err:
	return ret;
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
	int i = 0;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);//得到设备号VSER_MAJOR << 20 || VSER_MINOR
	ret = register_chrdev_region(dev, VSER_DEV_CNT, VSER_DEV_NAME); //注册字符设备

	if (ret)
		goto reg_err;

	for (i = 0; i < VSER_DEV_CNT; i++)
	{
		cdev_init(&vsdev[i].cdev, &vser_ops);		//初始化结构体，将vser_ops绑定vsdev
		vsdev[i].cdev.owner = THIS_MODULE;
		vsdev[i].fifo = i == 0 ? (struct kfifo *)&vsfifo0 :  (struct kfifo *)&vsfifo1;

		ret = cdev_add(&vsdev[i].cdev, dev + i, 1);		//添加cdev到内核里面
		if (ret)
			goto add_err;
	}

	printk("******** %s ********\n", __FUNCTION__);
	return 0;
add_err:
	for (--i; i > 0; --i)
	{
		cdev_del(&vsdev[i].cdev);
	}
	unregister_chrdev_region(dev, VSER_DEV_CNT);
reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
	dev_t dev;
	int i = 0;
	dev = MKDEV(VSER_MAJOR, VSER_MINOR);

	for (i = 0; i < VSER_DEV_CNT; i++)
	{
		cdev_del(&vsdev[i].cdev);
	}
	unregister_chrdev_region(dev, VSER_DEV_CNT);//注销字符设备
	printk("******** %s ********\n", __FUNCTION__);
}

module_init(vser_int);
module_exit(vser_exit);
MODULE_LICENSE("GPL");


