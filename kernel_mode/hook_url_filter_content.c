// 测试内核：3.13.0-32-generic
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/if_packet.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include "kernel_ac.h"
#include "zlibTool.c"
#include "kernel_netlink.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQ");
MODULE_DESCRIPTION("filter url content");
MODULE_VERSION("1.0");


struct node
{
	int seq;
	struct node *next;
	struct node *pre;
};

struct node *head = NULL;

// 发出请求时删除请求头中Accept-Encoding字段，防止收到gzip压缩包
int delete_accept_encoding(char *pkg)
{
	char *pK = NULL;
	char *pV = NULL;
	int len = 0;

	pK = strstr(pkg,"Accept: text/html");
	if(pK == NULL)	return 0;

	pK = strstr(pkg,"Accept-Encoding:");
	if(pK == NULL)	return 0;

	pV = strstr(pK,"\r\n");
	if(pV == NULL)	return 0;

	len = (long long)pV - (long long)pK;

	// 用空格覆盖Accept-Encoding:xxx内存
	memset(pK,' ',len + 2);
//	printk(KERN_ALERT "---Delete Accept-Encoding---\n");

	return 1;
}

void fix_checksum(struct sk_buff *skb){
	struct iphdr *iph = ip_hdr(skb);
	struct tcphdr *tcph = tcp_hdr(skb);
	// 重新计算校验和
	//-------------------------------------
	int datalen = skb->len - iph->ihl*4;
	//只能是ip报文长度小于mtu的数据报(没有分片的报文)。
	skb->ip_summed = CHECKSUM_PARTIAL;//CHECKSUM_PARTIAL表示使用硬件checksum ，L4层的伪头的校验已经完毕，并且已经加入uh->check字段中，此时只需要设备计算整个头4层头的校验值。
	skb->csum_start = skb_headroom(skb) + skb_network_offset(skb) + iph->ihl * 4;
	skb->csum_offset = offsetof(struct tcphdr, check);//校验和字段在tcp头部的偏移
	// ip
	iph->check = 0;
	iph->check = ip_fast_csum(iph, iph->ihl);
	// tcp
	tcph->check = 0;
	tcph->check = ~csum_tcpudp_magic(iph->saddr, iph->daddr,
					datalen, iph->protocol, 0);//伪首部校验和
	//-------------------------------------
}



char *strsub(char *dest, const char *src, size_t count)
{
    char *tmp = dest;

    while (count) {
        if ((*tmp = *src) != 0)
            src++;
        tmp++;
        count--;
    }
    *tmp = '\0';
    return dest;
}



unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	// IP数据包frag合并
//	 if (0 != skb_linearize(skb)) {
//        printk(KERN_ALERT "Frag error!\n");
//	 	return NF_ACCEPT;
//	 }
    //	控制变量
//    if (con_url == 0) {

        struct iphdr *iph = ip_hdr(skb);

        unsigned int tot_len = ntohs(iph->tot_len);
        unsigned int iph_len = ip_hdrlen(skb);

        unsigned int saddr = (unsigned int)iph->saddr;
        unsigned int daddr = (unsigned int)iph->daddr;

        int ack_seq = 0;
        if (iph->protocol == IPPROTO_TCP)
        {
            // struct tcphdr *tcph = (void *)iph + iph->ihl * 4;
            struct tcphdr *tcph = tcp_hdr(skb);
            unsigned int tcplen = skb->len - (iph->ihl*4) - (tcph->doff*4);

            unsigned int sport = (unsigned int)ntohs(tcph->source);
            unsigned int dport = (unsigned int)ntohs(tcph->dest);

            int seq_ack = ntohs(tcph->ack_seq);
            int seq = ntohs(tcph->seq);
            char *pkg = (char *)((long long)tcph + ((tcph->doff) * 4));

    		if (sport == 80)
            {

                // 处理HTTP请求且请求返回200
                if (memcmp(pkg,"HTTP/1.1 200", 12) == 0 || memcmp(pkg,"HTTP/1.0 200", 12) == 0)
                {

                    char *p, *page, *chset_start, *chset_end;
                    p = strstr(pkg,"Content-Type: text/html");
                    if(p == NULL)	return NF_ACCEPT;

                    page = strstr(pkg,"\r\n\r\n");
                    if(page == NULL)	return NF_ACCEPT;

                    page += 4;
                    char *ppage = page;

                    p = strstr(pkg,"Transfer-Encoding: chunked");
                    if(p != NULL){
                        ppage = strstr(page, "\r\n");
                        if(ppage == NULL) return NF_ACCEPT;

                        ppage += 2;
                    }

                    p = strstr(ppage,"<html");
                    if(p == NULL)	return NF_ACCEPT;

                    ppage = strstr(p, "charset=");
                    if(ppage == NULL)	return NF_ACCEPT;

                    //获取编码
                    chset_start = ppage + 8;
                    chset_end = strstr(chset_start, "\"");
                    if (chset_end == NULL) return NF_ACCEPT;
                    char *charset = (char *)kmalloc(sizeof(char)*(chset_end-chset_start+1), GFP_ATOMIC);
                    charset = strsub(charset, chset_start, chset_end-chset_start);

                    ppage = strstr(ppage, "<");
                    if(ppage == NULL)	return NF_ACCEPT;

                    int http_hdrlen = (long long)page-(long long)pkg;
                    int pageLen = tcplen-http_hdrlen;

                    // 检查是否是ajax请求的回复，若是则不处理
                    // ==========================================

                    //获取标题
                    char *title_start, *title_end;
                    title_start = strstr(page, "<title>");
                    if (title_start == NULL) return NF_ACCEPT;
                    title_start = title_start + 7;
                    title_end = strstr(title_start, "</title>");
                    if (title_end ==NULL) return NF_ACCEPT;
                    int title_len = title_end-title_start;
                    char *title = (char *)kmalloc(sizeof(char)*(title_len + 1), GFP_ATOMIC);
                    strsub(title, title_start, title_len);


                    struct node *ptr = head;
                    while(ptr != NULL){
                        if(ptr->seq == seq){
                            ptr->seq = ptr->seq + tcplen;

//                            if(ptr->pre != NULL) ptr->pre->next = ptr->next;
//                            if(ptr->next != NULL) ptr->next->pre = ptr->pre;
//                            kfree(ptr);
//                            if(ptr == head)
//                                head = NULL;

                            printk(KERN_ALERT "s1:%d len:%d seq:%d\n", tcplen + seq, tcplen, seq);

                            AC_match(page);
                            send_to_user(pkg);
                            return NF_ACCEPT;
                        }
                        ptr = ptr->next;
                    }
                    fix_checksum(skb);
                }
                else
                {
                    char *http = strstr(pkg, "HTTP/1.");
                    if (http == NULL) return NF_ACCEPT;
                    char *status = strstr(http, "\r\n");
                    if (status ==   NULL) return NF_ACCEPT;
                    int sta_len = status - http;
                    char *sta = kmalloc(sizeof(char)*(sta_len + 1), GFP_ATOMIC);
                   // printk(KERN_ALERT "status: %s ", strsub(sta, http, sta_len));
                    kfree(sta);

                    printk(KERN_ALERT "s2:%d len:%d seq:%d\n", tcplen + seq, tcplen, seq);

                    AC_match(pkg);
                    send_to_user(pkg);
                    fix_checksum(skb);
                    return NF_ACCEPT;
                }
            }

            else if(dport == 80)
            {
                char *p, *url_start, *url_end, *url;
                // 只处理GET请求
                if (memcmp(pkg,"GET",strlen("GET")) != 0) return NF_ACCEPT;

                p = strstr(pkg,"HTTP/1.1");
                if(p == NULL) return NF_ACCEPT;
                memcpy(p, "HTTP/1.0", 8);

                url_start = strstr(pkg, "Host: ");
                if (url_start == NULL) return NF_ACCEPT;
                url_end = strstr(url_start, "\r\n");
                if (url_end == NULL) return NF_ACCEPT;
                int url_len = url_end - (url_start + 6);
                url =  (char *)kmalloc(sizeof(char)*(url_len + 1), GFP_ATOMIC);
                strsub(url, url_start + 6, url_len);

//                printk(KERN_ALERT "%s\n", url);
                //   url匹配成功
                if (strcmp(url, "www.southcn.com") == 0 || strcmp(url, "www.jyb.cn") == 0){
                    printk(KERN_ALERT "url:%s ", url);

                    struct node *newnode = (struct node*)kmalloc(sizeof(struct node), GFP_ATOMIC);
                    struct node *ptr = head;

                    newnode->seq = ntohs(tcph->ack_seq);
                    newnode->next = NULL;
                    newnode->pre = NULL;
                    printk(KERN_ALERT "seq:\t\t%d\n", newnode->seq);
                    if(head == NULL) {
                        head = newnode;
                    }else{
                        while(ptr->next != NULL) {
                            ptr = ptr->next;
                        }
                        ptr->next = newnode;
                        newnode->pre = ptr;
                    }
                }
                kfree(url);

                // 发出请求时删除请求头中Accept-Encoding字段，防止收到gzip压缩包
                delete_accept_encoding(pkg);
                fix_checksum(skb);
            }
            else {
                printk(KERN_ALERT "s333:%d len:%d seq:%d ack_seq:%d\n", tcplen + seq, tcplen, seq, seq_ack);
                struct node *ptr = head;
                while(ptr != NULL){
                    if(ptr->seq == seq){
                        ptr->seq = ptr->seq + tcplen;

//                        if(ptr->pre != NULL) ptr->pre->next = ptr->next;
//                        if(ptr->next != NULL) ptr->next->pre = ptr->pre;
//                        kfree(ptr);
//                        if(ptr == head)
//                            head = NULL;

                        printk(KERN_ALERT "s3:%d len:%d seq:%d ack_seq:%d\n", tcplen + seq, tcplen, seq, seq_ack);
                        AC_match(pkg);
                        send_to_user(pkg);
                        fix_checksum(skb);
                        return NF_ACCEPT;
                    }
                    ptr = ptr->next;
                }
                return NF_ACCEPT;
            }

        }
//    }
	return NF_ACCEPT;
}
void free_head(struct node *head) {
    struct node *ptr = NULL;
    while (head != NULL) {
        ptr = head->next;
        kfree(head);
        if (NULL != ptr)
            ptr->pre = NULL;
        head = ptr;
    }
}
// 钩子函数注册
static struct nf_hook_ops http_hooks[] =
{
	{
		.hook 			= hook_func,
		.pf 			= NFPROTO_IPV4,
		//.hooknum 		= NF_INET_FORWARD,
		.hooknum 		= NF_INET_POST_ROUTING,
		.priority 		= NF_IP_PRI_MANGLE,
		.owner			= THIS_MODULE
	}
};

// 模块加载
static int init_hook_module(void)
{
	nf_register_hooks(http_hooks, ARRAY_SIZE(http_hooks));
	printk(KERN_ALERT "hook_url_filter_content: insmod\n");
	return 0;
}

// 模块卸载
static void cleanup_hook_module(void)
{
    free_head(head);
	nf_unregister_hooks(http_hooks, ARRAY_SIZE(http_hooks));
	printk(KERN_ALERT "hook_url_filter_content: rmmod\n");
}

module_init(init_hook_module);
module_exit(cleanup_hook_module);
