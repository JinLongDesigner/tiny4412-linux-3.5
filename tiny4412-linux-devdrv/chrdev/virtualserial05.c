#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>


//#define VSER_MAJOR 256  		//主设备号用来区别设备类型
#define VSER_MINOR 0			//次设备号用来区别具体的设备
#define VSER_DEV_CNT 1			//申请设备的个数，连续申请，次设备号从指定的次设备号开始依次增加
#define VSER_DEV_NAME "virtualFIFO"	//设备的名字

#define CLASS_NAME "VirtualFIFO"

//static struct cdev vsdev;
//static dev_t dev;
//static struct class *cls = NULL;

struct virtualfifo
{
	struct cdev cdev;
	dev_t dev;
	struct class * cls;
	wait_queue_head_t read_queue;
	wait_queue_head_t write_queue;
};
static struct virtualfifo *vsdev = NULL;

struct virtualfifo_private_data
{
	struct virtualfifo *virtualfifo;
};

/**
*define_kfifo-定义和初始化FIFO的宏
*@fifo：声明的fifo数据类型的名称
*@类型：FIFO元素的类型
*@大小：FIFO中的元素数，必须是2的幂
*
*注意：宏可用于全局和本地FIFO数据类型变量。
*/


DEFINE_KFIFO(vsfifo, char, 64);

static int vs_open(struct inode *inode, struct file *filp)
{
	struct virtualfifo_private_data *data = NULL;
        struct virtualfifo *device = vsdev;

        printk("%s: major=%d, minor=%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));

        data = kmalloc(sizeof(struct virtualfifo_private_data), GFP_KERNEL);
        if (!data)
                return -ENOMEM;

        data->virtualfifo = device;
        filp->private_data = data;

        return 0;

}
static int vs_release(struct inode * inode, struct file * filp)
{
	struct virtualfifo_private_data *data = filp->private_data;

        kfree(data);
	return 0;
}

static ssize_t vs_read(struct file * filp, char __user * buf, size_t count, loff_t * ppos)
{
	unsigned int actual_readed = 0;
	int ret = 0;
	struct virtualfifo_private_data *data = filp->private_data;
        struct virtualfifo *device = data->virtualfifo;

	if (kfifo_is_empty(&vsfifo))
	{
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		printk("%s : pid=%d, going to sleep.\n", __func__, current->pid);

                ret = wait_event_interruptible(device->write_queue, !kfifo_is_empty(&vsfifo));
                if (ret)
                        return ret;
	}

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

	ret = kfifo_to_user(&vsfifo, buf, count, &actual_readed);
	if (ret)
		return -EIO;

	printk(KERN_INFO "%s, pid = %d, actual_readed = %d, ppos = %lld .\n", __func__, current->pid, actual_readed, *ppos);

	
	return actual_readed;
}
static ssize_t vs_write(struct file * filp, const char __user * buf, size_t count, loff_t * ppos)
{
	unsigned int actual_write = 0;
	int ret = 0;
	
	struct virtualfifo_private_data *data = filp->private_data;
        struct virtualfifo *device = data->virtualfifo;

	if (kfifo_is_full(&vsfifo))
	{
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		printk("%s : pid=%d, going to sleep.\n", __func__, current->pid);
		
		ret = wait_event_interruptible(device->write_queue, !kfifo_is_full(&vsfifo));
		if (ret)
			return ret;
	}

	ret = kfifo_from_user(&vsfifo, buf, count, &actual_write);
	
	if (ret)
		return -EIO;

	if (!kfifo_is_empty(&vsfifo))
	{
		wake_up_interruptible(&device->read_queue);
	}

	printk(KERN_INFO "%s, pid = %d, actual_write = %d, ppos = %lld, ret = %d.\n", __func__, current->pid, actual_write, *ppos, ret);

	return actual_write;
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
	int ret = 0;
	
	vsdev = kmalloc(sizeof(struct virtualfifo), GFP_KERNEL);
	if (!vsdev)
		return -ENOMEM;	

#if 0
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);//得到设备号VSER_MAJOR << 20 || VSER_MINOR
	ret = register_chrdev_region(dev, VSER_DEV_CNT, VSER_DEV_NAME); //注册字符设备	
#endif
	ret = alloc_chrdev_region(&vsdev->dev, VSER_MINOR, VSER_DEV_CNT, VSER_DEV_NAME);
	if (ret)
		goto reg_err;

	cdev_init(&vsdev->cdev, &vser_ops);		//初始化结构体，将vser_ops绑定vsdev
	vsdev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&vsdev->cdev, vsdev->dev, VSER_DEV_CNT);		//添加cdev到内核里面
	if (ret)
		goto add_err;

	vsdev->cls = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(vsdev->cls)) {
		ret = PTR_ERR(vsdev->cls);
		printk(KERN_INFO "Could not create device class (err %d)\n", -ret);
		unregister_chrdev_region(vsdev->dev, VSER_DEV_CNT);
		goto class_create_failed;
	}

	device_create(vsdev->cls, NULL, vsdev->dev, NULL, VSER_DEV_NAME);
	
	init_waitqueue_head(&vsdev->read_queue);
	init_waitqueue_head(&vsdev->write_queue);

	printk(KERN_INFO "******** %s ********\n", __FUNCTION__);
	printk(KERN_INFO "******** %d ********* %d ********\n", MAJOR(vsdev->dev), MINOR(vsdev->dev));
	return 0;

class_create_failed:
	unregister_chrdev_region(vsdev->dev, VSER_DEV_CNT);
	return ret;
	
add_err:
	unregister_chrdev_region(vsdev->dev, VSER_DEV_CNT);
	return ret;
	
reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
//	dev_t dev;
//	dev = MKDEV(VSER_MAJOR, VSER_MINOR);

	device_destroy(vsdev->cls, vsdev->dev);
	class_destroy(vsdev->cls);
	cdev_del(&vsdev->cdev);
	unregister_chrdev_region(vsdev->dev, VSER_DEV_CNT);//注销字符设备
	kfree(vsdev);
	printk(KERN_INFO "******** %s ********\n", __FUNCTION__);
}

module_init(vser_int);
module_exit(vser_exit);
MODULE_LICENSE("GPL");

