ifneq ($(KERNELRELEASE),)
obj-m := VirtualDisk.o
else
KERNELDIR := /work/RK3399_Linux/kernel
PWD := $(shell pwd)
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers
