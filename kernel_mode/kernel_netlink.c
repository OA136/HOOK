#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/string.h>

#include <linux/netlink.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQ");
MODULE_DESCRIPTION("Communicate with user");
MODULE_VERSION("1.0");

void control() {
    struct net *net = (struct net*)kmalloc(sizeof(struct net), GFP_ATOMIC);
    struct sock *skd = netlink_kernel_create(net, NETLINK_NETFILTER, );

}

static int init(void) {
    control();
    return 0;
}

static int exit(void) {
    printk("exit\n");
    return 0;
}

module_init(init);
module_exit(exit);
