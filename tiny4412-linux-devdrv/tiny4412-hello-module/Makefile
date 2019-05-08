KERN_DIR = /home/me/work/tiny4412/tiny4412-linux/linux-3.5

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

obj-m   += tiny4412-hello-module.o

