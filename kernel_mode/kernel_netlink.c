#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include "kernel_netlink/self.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQ");
MODULE_DESCRIPTION("Communicate with user");
MODULE_VERSION("1.0");


struct sock *skd = NULL;
int pid;
struct sk_buff *skb_out;

static void send_to_user(char *msg) {
    struct nlmsghdr *nlh;
    int msg_size = strlen(msg);
    int res;
    skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out) {
        printk(KERN_ALERT "Failed to allocate new skb!\n");
        return;
    }
    //__nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq, int type, int len, int flags);
    nlh = nlmsg_put(skb_out, 0, 0, 60, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(NLMSG_DATA(nlh), msg, msg_size);
    //res = netlink_unicast(skd, skb_out, pid, MSG_DONTWAIT);
    res = nlmsg_unicast(skd, skb_out, pid);
    printk(KERN_ALERT "user pid:%d!\n", pid);
    if (res < 0) {
        printk(KERN_ALERT "Error send message to user!\n");
        return;
    }
    printk(KERN_ALERT "%s\nmsg_size:%d\n", NLMSG_DATA(nlh), msg_size);
    return;
}

static void receive_from_user(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid;
    printk(KERN_ALERT "Kernel received message:%s\tlength:%d\n", (char *)NLMSG_DATA(nlh), nlh->nlmsg_len - NLMSG_HDRLEN);
//    char *msg = (char *)kmalloc(sizeof(char)*(25), GFP_ATOMIC);
//    memset(msg, 0, 25);
//    strncpy(msg, "hook_url_filter to user!", 24);
    //msg = "hook_url_filter to user!";
    char msg[20] = "12345678901234567";
    send_to_user(msg);
//    kfree(msg);
}


static void init_mod(void) {
    struct netlink_kernel_cfg cfg = {

        .input = receive_from_user,
    };
    skd = netlink_kernel_create(&init_net, NETLINK_NETFILTER, &cfg);
    if (!skd) {
        printk(KERN_ALERT "Error create sock!\n");
    }
}

static void exit_mod(void) {
    printk("exit\n");
    netlink_kernel_release(skd);
}

module_init(init_mod);
module_exit(exit_mod);
EXPORT_SYMBOL(send_to_user);
