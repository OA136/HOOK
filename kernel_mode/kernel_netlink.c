#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include "common.h"
#include "kernel_ac.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQ");
MODULE_DESCRIPTION("Communicate with user");
MODULE_VERSION("1.0");

//控制信息
int con_url = 1;


struct sock *skd = NULL;
int pid;
//struct sk_buff *skb_out;

static void send_to_user(char *msg) {
    struct nlmsghdr *nlh;
    int msg_size = strlen(msg);
    int res;
    struct sk_buff *skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out) {
        printk(KERN_ALERT "Failed to allocate new skb!\n");
        return;
    }
    //__nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq, int type, int len, int flags);
    nlh = nlmsg_put(skb_out, 0, 0, 60, msg_size, 0);
   // printk(KERN_ALERT "msg_size:%d\t%d\n", msg_size, nlh->nlmsg_len-NLMSG_HDRLEN);
    NETLINK_CB(skb_out).dst_group = 0;

    strncpy(nlmsg_data(nlh), msg, msg_size);
    //res = netlink_unicast(skd, skb_out, pid, MSG_DONTWAIT);
    res = nlmsg_unicast(skd, skb_out, pid);
    //printk(KERN_ALERT "user pid:%d!\n", pid);
    if (res < 0) {
        printk(KERN_ALERT "Error send message to user!\n");
        return;
    }
    //printk(KERN_ALERT "%s\nmsg_size:%d\n", NLMSG_DATA(nlh), msg_size);
    return;
}

static void receive_from_user(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid;
    printk(KERN_ALERT "Kernel received message:%s\tlength:%d\n", nlmsg_data(nlh), strlen(nlmsg_data(nlh)));
//    char *msg = (char *)kmalloc(sizeof(char)*(25), GFP_ATOMIC);
//    memset(msg, 0, 25);
//    strncpy(msg, "hook_url_filter to user!", 24);
    //msg = "hook_url_filter to user!";
    //控制url过滤启动
    if (nlh->nlmsg_type == CON_START) {
        con_url = 0;
        char msg[20] = "start url_filter!";
        send_to_user(msg);
    }
    //增加模式串
    if (nlh->nlmsg_type == ADD_PATTERN) {
        printk(KERN_ALERT "add pattern:%s\n", nlmsg_data(nlh));
        add_pattern(nlmsg_data(nlh));
        char msg[20] = "add_pattern!";
        send_to_user(msg);
    }

//    kfree(msg);
}


static int init_mod(void) {
    struct netlink_kernel_cfg cfg = {

        .input = receive_from_user,
    };
    skd = netlink_kernel_create(&init_net, NETLINK_NETFILTER, &cfg);
    if (!skd) {
        printk(KERN_ALERT "Error create sock!\n");
    }
    return 0;
}

static void exit_mod(void) {
    printk("exit\n");
    netlink_kernel_release(skd);
}

module_init(init_mod);
module_exit(exit_mod);

EXPORT_SYMBOL(con_url);
EXPORT_SYMBOL(send_to_user);
