obj-m := kernel_netlink.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


default:
	$(MAKE) -C $(KDIR)  M=$(PWD) modules
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers
