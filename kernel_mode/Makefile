obj-m := kernel_ac.o kernel_netlink.o hook_url_filter_content.o zlibTool.o

#obj-m := hook_ipv4_replace_html.o zlibTool.o
#obj-m := hook_ipv4.o zlibTool.o
#obj-m := hook_ipv4_fix_seq.o zlibTool.o
#obj-m := kernel_ac.o hook_url_filter.o kernel_netlink.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
#KBUILD_EXTRA_SYMBOLS += $(PWD)/kernel_netlink/Module.symvers
#export KBUILD_EXTRA_SYMBOLS

default:
	$(MAKE) -C $(KDIR)  M=$(PWD) modules
routing:
	sudo iptables -t nat -A POSTROUTING -j MASQUERADE
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *~ *.order
