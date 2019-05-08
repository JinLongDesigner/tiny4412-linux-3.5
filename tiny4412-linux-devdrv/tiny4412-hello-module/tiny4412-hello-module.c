#include <linux/kernel.h>
#include <linux/module.h>

/*__init宏最常用的地方是驱动模块初始化函数的定义处，其目的是讲驱动模块的初始化函数放入
名叫.init.text的输入段。当内核启动完毕后，这个段中的内存会被释放掉供其他使用。
有个问题：这个段中的内存是谁做处理的？
*/
static __init int hello_init()
{
	printk(KERN_ALERT "Hello, world\n");
	return 0;
}

static __exit void hello_exit()
{
//调用内核打印函数
/*内核设置了打印的级别，目前系统有八个级别
#define KERN_EMERG	"<0>"	//system is unusable			
#define KERN_ALERT	"<1>"	// action must be taken immediately	
#define KERN_CRIT	"<2>"	// critical conditions			
#define KERN_ERR	"<3>"	// error conditions			
#define KERN_WARNING	"<4>"	// warning conditions			
#define KERN_NOTICE	"<5>"	// normal but significant condition	
#define KERN_INFO	"<6>"	// informational		
#define KERN_DEBUG	"<7>"	// debug-level messages			

*/
	printk(KERN_ALERT "Goodbye, world\n");	
}

//驱动程序的入口函数
module_init(hello_init);
module_exit(hello_exit);
//遵循开源协议
MODULE_LICENSE("Dual BSD/GPL");



