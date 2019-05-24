#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <uapi/linux/types.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>

#define VIRTUALDISK_SIZE 0x2000  	//全局内存最大8K字节
#define MEM_CLEAR 0x1				//全局内存清零
#define PORT1_SET 0x2				//将port1端口清零
#define PORT2_SET 0x3				//将port2端口清零

#define VIRTUALDISK_MAJOR 200		//主设备号

#define DEVICE_NAME "VirtualDisk"	//设备名

//VirtualDisk设备结构体
struct VirtualDisk_Dev 
{
	dev_t dev_num;
	struct cdev dev;
	unsigned char mem[VIRTUALDISK_SIZE];
	int port1;
	long port2;
	long count;
};
//创建一个VirtualDisk设备
struct VirtualDisk_Dev *virtualdisk_dev;

//VirtualDisk打开函数
int virtualdisk_open(struct inode *inode, struct file *fp)
{
//	printk("**************** %s ***************\n", __FUNCTION__);
	struct VirtualDisk_Dev *devp = NULL;	
	//struc file 结构里面的private_data是给文件系统和驱动使用的。
	fp->private_data = virtualdisk_dev;
	devp = fp->private_data; 		//获得VirtualDisk设备结构体指针
	devp->count++;					//VirtualDisk设备打开次数
	return 0;
}

//VirtualDisk读函数
ssize_t virtualdisk_read(struct file *fp, char __user *buffer, size_t size, loff_t *off_set)
{
//	printk("**************** %s ***************\n", __FUNCTION__);
	unsigned long p = *off_set;							//记录文件指针偏移位置
	unsigned int count = size;							//记录需要读取的字节数
	int ret = 0;
	
	struct VirtualDisk_Dev *devp = fp->private_data;	//获得VirtualDisk设备结构体指针
	if (p >= VIRTUALDISK_SIZE)							//读取的偏移大于VirtualDisk设备的内存空间
		return count ? -ENXIO: 0;						//读取地址错误
	
	if (count > VIRTUALDISK_SIZE - p)					//要读取的字节大于VirtualDisk设备的内存空间
		count = VIRTUALDISK_SIZE - p;					//将要读取的字节数设为剩余的字节数
	
	//内核空间-->用户空间
	if (copy_to_user((char *)buffer, (void  __user *)(devp->mem + p), count))
	{
		ret = -EFAULT;
	}
	else
	{
		*off_set += count;
		ret = count;
		printk("read %d bytes(s) from %ld .\n", count, p);
	}
	
	return ret;
}
ssize_t virtualdisk_write(struct file *fp, const char __user * buffer, size_t size, loff_t *off_set)
{
//	printk("**************** %s ***************\n", __FUNCTION__);
	unsigned long p = *off_set;							//记录文件指针偏移位置
	unsigned int count = size;							//记录需要读取的字节数
	int ret = 0;
	
	struct VirtualDisk_Dev * devp = fp->private_data;	//获得VirtualDisk设备结构体指针
	if (p >= VIRTUALDISK_SIZE)							//读取的偏移大于VirtualDisk设备的内存空间
		return count ? -ENXIO: 0;						//读取地址错误
	
	if (count > VIRTUALDISK_SIZE - p)					//要读取的字节大于VirtualDisk设备的内存空间
		count = VIRTUALDISK_SIZE - p;					//将要读取的字节数设为剩余的字节数
	
	//用户空间-->内核空间
	if (copy_from_user((char *)(devp->mem + p), buffer, count))
	{
		ret = -EFAULT;
	}
	else
	{
		*off_set += count;
		ret = count;
		printk("written %d bytes(s) from %ld .\n", count, p);
	}
	
	return ret;
}
long virtualdisk_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
//	printk("**************** %s ***************\n", __FUNCTION__);
	struct VirtualDisk_Dev *devp = fp->private_data;	//获得VirtualDisk设备结构体指针
	
	switch (cmd)
	{
		case MEM_CLEAR:									//VirtualDisk设备清零
			memset(devp->mem, 0, VIRTUALDISK_SIZE);
			printk("VirtualDisk is set to zero.\n");
			break;
		case PORT1_SET:
			devp->port1 = 0;							//VirtualDisk设备port1置0		
			break;
		case PORT2_SET:
			devp->port2 = 0;							//VirtualDisk设备port2置0
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

loff_t virtualdisk_llseek(struct file *fp, loff_t off_set, int dat)
{
//	printk("**************** %s ***************\n", __FUNCTION__);
	loff_t ret = 0;											//返回位置的偏移
	switch (dat)
	{
		case SEEK_SET:										//相对于VirtualDisk设备内存空间开始位置的偏移							
			if (off_set  < 0)								//off_set 不合法
			{
				ret = -EINVAL;								//无效的指针
				break;
			}
			if ((unsigned int )off_set > VIRTUALDISK_SIZE)	//偏移大于VirtualDisk设备内存空间
			{
				ret = -EINVAL;								//无效的指针
				break;
			}
			fp->f_pos = (unsigned int)off_set;				//更新指针位置
			ret = fp->f_pos;								//返回的位置偏移
			break;
		case SEEK_CUR:										//相对于VirtualDisk设备内存空间当前位置的偏移
			if((fp->f_pos + off_set) > VIRTUALDISK_SIZE)	//偏移大于VirtualDisk设备内存空间
			{
				ret = -EINVAL;								//无效的指针
				break;
			}
			if ((fp->f_pos + off_set) < 0)					//指针不合法
			{
				ret = -EINVAL;
				break;
			}
			fp->f_pos += off_set;							//更新指针位置
			ret = fp->f_pos;								//无效的指针
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

int virtualdisk_release(struct inode *inode, struct file *fp)
{
//	printk("**************** %s ***************\n", __FUNCTION__);
	struct VirtualDisk_Dev *devp = fp->private_data;		//获得VirtualDisk设备结构体指针
	devp->count--;											//减少VirtualDisk设备打开次数
	return 0;
}

struct file_operations virtualdisk_fops = {
	.owner = THIS_MODULE,
	.open = virtualdisk_open,
	.read = virtualdisk_read,
	.write = virtualdisk_write,
	.unlocked_ioctl = virtualdisk_ioctl,
	.llseek = virtualdisk_llseek,
	.release = virtualdisk_release,
};

static __init int virtualdisk_init(void)
{
	int ret = 0;
	int virtualdisk_major = VIRTUALDISK_MAJOR;
	
	//动态申请设备结构体的内存空间
	virtualdisk_dev = kmalloc(sizeof(struct VirtualDisk_Dev), GFP_KERNEL);
	if (!virtualdisk_dev)
	{
		printk(" kzalloc memory failed.\n");
		ret = -ENOMEM;
		goto kmalloc_failed;
	}
	//申请设备结构体的内存空间清零
	memset(virtualdisk_dev, 0, sizeof(struct VirtualDisk_Dev));
	
	//申请设备号吗，静态申请设备号，使用cat /proc/devices 查看设备号有没有被使用
	virtualdisk_dev->dev_num = MKDEV(virtualdisk_major, 0);

	ret = register_chrdev_region(virtualdisk_dev->dev_num, 1, DEVICE_NAME);
	/*
	if (virtualdisk_major)
	{
		ret = register_chrdev_region(virtualdisk_dev->dev_num, 1, DEVICE_NAME);
	}
	else
	{
		ret = alloc_chrdev_region(&virtualdisk_dev->dev_num, 0, 1, DEVICE_NAME);
		virtualdisk_major = MAJOR(virtualdisk_dev->dev_num);
	}
	*/
	if (ret < 0)
	{
		printk("register chrdev num is failed.\n");
		goto chrdev_num_failed;
	}
	
	//virtualdisk初始化操作函数集
	cdev_init(&virtualdisk_dev->dev, &virtualdisk_fops);
	virtualdisk_dev->dev.owner = THIS_MODULE;
	
	//cdec链接file_operations指针，这样virtualdisk_dev->dev.ops就把设置成了文件操作函数集
	virtualdisk_dev->dev.ops = &virtualdisk_fops;
	
	//virtualdisk注册到内核
	ret = cdev_add(&virtualdisk_dev->dev, virtualdisk_dev->dev_num, 1);
	if (ret < 0)
	{
		printk("adding chrdev to kernel is failed.\n");
		goto cdev_add_failed;
	}
	
	printk("adding virtualdisk device to kernel is ok.\n");
	return 0;
	
cdev_add_failed:
	unregister_chrdev_region(virtualdisk_dev->dev_num, 1);
	kfree(virtualdisk_dev);
	return ret;

chrdev_num_failed:
	return ret;

kmalloc_failed:
	return ret;
}


static __exit void virtualdisk_exit(void)
{
	printk("exiting virtualdisk device from kernel is ok.\n");
	cdev_del(&virtualdisk_dev->dev);
	kfree(virtualdisk_dev);
	unregister_chrdev_region(virtualdisk_dev->dev_num, 1);
}

module_init(virtualdisk_init);
module_exit(virtualdisk_exit);
MODULE_LICENSE("GPL");
