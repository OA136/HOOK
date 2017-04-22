obj-m := hook_url_filter_content.o zlibTool.o hook_url_filter.o kernel_netlink.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
#KBUILD_EXTRA_SYMBOLS += $(PWD)/kernel_netlink/Module.symvers
#export KBUILD_EXTRA_SYMBOLS

default:
	$(MAKE) -C $(KDIR)  M=$(PWD) modules
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *~ *.order
