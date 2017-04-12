#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include < net/sock.h>
#include <linux/netlink.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQ");
MODULE_DESCRIPTION("Communicate with user");
MODULE_VERSION("1.0");


struct sock *skd = NULL;

void receive(struct sk_buff *skb) {
    printk(KERN_ALERT "Kernel received message!\n");
}



static int init(void) {
    struct netlink_kernel_cfg cfg = {

        .input = receive,
    };
    skd = netlink_kernel_create(net, NETLINK_NETFILTER, &cfg);
    if (!skd) {
        printk(KERN_ALERT "Error create sock!\n");
        return -1;
    }
    return 0;
}

static int exit(void) {
    printk("exit\n");
    return 0;
}

module_init(init);
module_exit(exit);
