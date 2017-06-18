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
#include "kernel_netlink.h"
#include "common.h"
#include "send_newpage.c"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQ");
MODULE_DESCRIPTION("filter url");
MODULE_VERSION("1.0");
//

char code[] = "HTTP/1.1 200 OK\r\n"\
"Content-Type: text/html\r\n"\
"Content-Length: 142\r\n"\
"Connection: close\r\n"\
"\r\n\r\n"\
"＜html＞\r\n"\
"＜head＞\r\n"\
"＜title＞Wrox Homepage＜/title＞\r\n"\
"＜/head＞\r\n"\
"＜body＞\r\n"\
"＜!-- body goes here --＞\r\n"\
"＜/body＞\r\n"\
"＜/html＞\r\n";


int build_dev_xmit_tcp (struct net_device* dev,
			 u_char * smac, u_char * dmac,
             u_char * pkt, int pkt_len,
             u_long sip, u_long dip,
             u_short sport, u_short dport,
             u_long seq, u_long ack_seq,
             u_char syn, u_char ack, u_char psh, u_char fin)
{
	struct sk_buff * skb = NULL;
	// struct net_device * dev = NULL;
	struct ethhdr * ethdr = NULL;
	struct iphdr * iph = NULL;
	struct tcphdr * tcph = NULL;
	u_char * pdata = NULL;
	int nret = 1;

	if (NULL == smac || NULL == dmac) goto out;

	// dev = dev_get_by_name(&init_net, eth);

	if (NULL == dev) goto out;

	skb = alloc_skb (pkt_len + sizeof (struct iphdr) + sizeof (struct tcphdr) + LL_RESERVED_SPACE (dev), GFP_ATOMIC);
	/*
	LL_RESERVED_SPACE(dev) = 16
	alloc_skb返回以后，skb->head = skb_data = skb->tail = alloc_skb分配的内存区首地址,skb->len = 0;
	skb->end = skb->tail + size;
	注：我的机子是32位x86机器，所以没有定义NET_SKBUFF_DATA_USES_OFFSET，因而，
	skb->tail,skb->mac_header,skb->network_header,skb->transport_header这几个成员都是指针
	*/
	if (NULL == skb)
		goto out;
	skb_reserve (skb, LL_RESERVED_SPACE (dev));//add data and tail
	skb->dev = dev;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = __constant_htons(ETH_P_IP);
	skb->ip_summed = CHECKSUM_NONE;
	skb->priority = 0;
	//skb->nh.iph = (struct iphdr*)skb_put(skb, sizeof (struct iphdr));
	//skb->h.th = (struct tcphdr*)skb_put(skb, sizeof (struct tcphdr));
	skb_set_network_header(skb, 0); //skb->network_header = skb->data + 0;
	skb_put(skb, sizeof (struct iphdr)); //add tail and len
	skb_set_transport_header(skb, sizeof (struct iphdr));//skb->transport_header = skb->data + sizeof (struct iphdr)
	skb_put(skb, sizeof (struct tcphdr));
	pdata = skb_put (skb, pkt_len);
	{
	if (NULL != pkt)
        memcpy (pdata, pkt, pkt_len);
	}

	{
		tcph = tcp_hdr(skb);
		memset (tcph, 0, sizeof (struct tcphdr));
		tcph->source = sport;
		tcph->dest = dport;
		tcph->seq = seq;
		tcph->ack_seq = ack_seq;
		tcph->doff = 5;
		tcph->psh = psh;
		tcph->fin = fin;
		tcph->syn = syn;
		tcph->ack = ack;
		tcph->window = __constant_htons (5840);
		skb->csum = 0;
		tcph->check = 0;
	}

	{
		iph = ip_hdr(skb);
		iph->version = 4;
		iph->ihl = sizeof(struct iphdr)>>2;
		iph->frag_off = 0;
		iph->protocol = IPPROTO_TCP;
		iph->tos = 0;
		iph->daddr = dip;
		iph->saddr = sip;
		iph->ttl = 0x40;
		iph->tot_len = __constant_htons(skb->len);
		iph->check = 0;
	}
	skb->csum = skb_checksum (skb, iph->ihl*4, skb->len - iph->ihl * 4, 0);
	tcph->check = csum_tcpudp_magic (sip, dip, skb->len - iph->ihl * 4, IPPROTO_TCP, skb->csum);
	{
		ethdr = (struct ethhdr*)skb_push (skb, 14);//reduce data and add len
		// memcpy (ethdr->h_dest, dmac, ETH_ALEN);
		// memcpy (ethdr->h_source, smac, ETH_ALEN);
		ethdr->h_proto = __constant_htons (ETH_P_IP);
	}
	nret = dev_queue_xmit(skb);
	if (0 > nret) goto out;
	printk("+++ack ret: %d\n", nret);

	out:
	if (0 != nret && NULL != skb)
	{
		dev_put (dev);
		kfree_skb (skb);
	}
	return (nret);
}

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
    //	控制变量
    if (con_url == 0) {

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
                if (strcmp(url, "www.jyb.cn") == 0 || strcmp(url, "www.dgqxjy.com") == 0) {
                    send_to_user(url);
                    printk(KERN_ALERT "Find:%s\n", url);
                    _http_send_redirect(skb,iph,tcph);
                    return NF_DROP;
                }

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
    }
	return NF_ACCEPT;
}

// 钩子函数注册
static struct nf_hook_ops http_hooks[] =
{
	{
		.hook 			= hook_func,
		.pf 			= NFPROTO_IPV4,
		//.hooknum 		= NF_INET_POST_ROUTING,
		.hooknum 		= NF_INET_FORWARD,
		.priority 		= NF_IP_PRI_MANGLE,
		.owner			= THIS_MODULE
	}
};

// 模块加载
static int init_hook_module(void)
{
    redirect_url_init();
	nf_register_hooks(http_hooks, ARRAY_SIZE(http_hooks));
	printk(KERN_ALERT "hook_url_filter: insmod\n");
	return 0;
}

// 模块卸载
static void cleanup_hook_module(void)
{
	nf_unregister_hooks(http_hooks, ARRAY_SIZE(http_hooks));
	redirect_url_fini();
	printk(KERN_ALERT "hook_url_filter: rmmod\n");
}

module_init(init_hook_module);
module_exit(cleanup_hook_module);
