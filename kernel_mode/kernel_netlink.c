#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <net/sock.h>
#include <linux/netlink.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQ");
MODULE_DESCRIPTION("Communicate with user");
MODULE_VERSION("1.0");


struct sock *skd = NULL;

void receive(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    nlh = (struct nlmsghdr *)skb->data;
    printk(KERN_ALERT "Kernel received message:%s\n", (char *)NLMSG_DATA(nlh));
}



static void init_mod(void) {
    struct netlink_kernel_cfg cfg = {

        .input = receive,
    };
    skd = netlink_kernel_create(&init_net, NETLINK_NETFILTER, &cfg);
    if (!skd) {
        printk(KERN_ALERT "Error create sock!\n");
    }
}

static void exit_mod(void) {
    printk("exit\n");
}

module_init(init_mod);
module_exit(exit_mod);
