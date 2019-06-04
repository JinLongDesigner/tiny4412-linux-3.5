ifneq ($(KERNELRELEASE),)
obj-m := virtualserial05.o
else
KERNELDIR := /work/RK3399/rk3399_linux/kernel
PWD := $(shell pwd)
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers
