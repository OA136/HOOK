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
#include "kernel_netlink/self.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("songboyu");
MODULE_DESCRIPTION("filter url");
MODULE_VERSION("1.0");

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
	// if (0 != skb_linearize(skb)) {
	// 	return NF_ACCEPT;
	// }
	struct iphdr *iph = ip_hdr(skb);

	unsigned int tot_len = ntohs(iph->tot_len);
	unsigned int iph_len = ip_hdrlen(skb);

	unsigned int saddr = (unsigned int)iph->saddr;
	unsigned int daddr = (unsigned int)iph->daddr;

	if (iph->protocol == IPPROTO_TCP)
	{
		// struct tcphdr *tcph = (void *)iph + iph->ihl * 4;
		struct tcphdr *tcph = tcp_hdr(skb);
		unsigned int tcplen = skb->len - (iph->ihl*4) - (tcph->doff*4);

		unsigned int sport = (unsigned int)ntohs(tcph->source);
		unsigned int dport = (unsigned int)ntohs(tcph->dest);

		char *pkg = (char *)((long long)tcph + ((tcph->doff) * 4));

		if(dport == 80)
		{
            char *p, *url_start, *url_end, *url;
            url_start = strstr(pkg, "Host: ");
            if (url_start == NULL) return NF_ACCEPT;
            url_end = strstr(url_start, "\r\n");
            if (url_end == NULL) return NF_ACCEPT;
            int url_len = url_end - (url_start + 6);
            url =  (char *)kmalloc(sizeof(char)*(url_len + 1), GFP_ATOMIC);
            strsub(url, url_start + 6, url_len);

            printk(KERN_ALERT "%s\n", url);
            if (strcmp(url, "www.dgqxjy.com") == 0)
                return NF_DROP;
            kfree(url);
			// 只处理GET请求
			if (memcmp(pkg,"GET",strlen("GET")) != 0)
			{
				return NF_ACCEPT;
			}

			p = strstr(pkg,"HTTP/1.1");
			if(p == NULL) return NF_ACCEPT;

			// 发出请求时删除请求头中Accept-Encoding字段，防止收到gzip压缩包
			delete_accept_encoding(pkg);
			fix_checksum(skb);
		}
	}
	return NF_ACCEPT;
}

// 钩子函数注册
static struct nf_hook_ops http_hooks[] =
{
	{
		.hook 			= hook_func,
		.pf 			= NFPROTO_IPV4,
		.hooknum 		= NF_INET_POST_ROUTING,
		.priority 		= NF_IP_PRI_MANGLE,
		.owner			= THIS_MODULE
	}
};

// 模块加载
static int init_hook_module(void)
{
	nf_register_hooks(http_hooks, ARRAY_SIZE(http_hooks));
	printk(KERN_ALERT "hook_url_filter: insmod\n");
    send_to_user("yes");
	return 0;
}

// 模块卸载
static void cleanup_hook_module(void)
{
	nf_unregister_hooks(http_hooks, ARRAY_SIZE(http_hooks));
	printk(KERN_ALERT "hook_url_filter: rmmod\n");
}

module_init(init_hook_module);
module_exit(cleanup_hook_module);

