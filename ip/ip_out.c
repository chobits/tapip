#include "netif.h"
#include "ether.h"
#include "arp.h"
#include "ip.h"
#include "raw.h"
#include "icmp.h"
#include "route.h"
#include "lib.h"

void ip_send_dev(struct netdev *dev, struct pkbuf *pkb)
{
	struct arpentry *ae;
	unsigned int dst;
	struct rtentry *rt = pkb->pk_rtdst;

	if (rt->rt_flags & RT_LOCALHOST) {
		ipdbg("To loopback");
		netdev_tx(dev, pkb, pkb->pk_len - ETH_HRD_SZ,
					ETH_P_IP, dev->net_hwaddr);
		return;
	}

	/* next-hop: default route or remote dst */
	if ((rt->rt_flags & RT_DEFAULT) || rt->rt_metric > 0)
		dst = rt->rt_gw;
	else
		dst = pkb2ip(pkb)->ip_dst;

	ae = arp_lookup(ETH_P_IP, dst);
	if (!ae) {
		arpdbg("not found arp cache");
		ae = arp_alloc();
		if (!ae) {
			ipdbg("arp cache is full");
			free_pkb(pkb);
			return;
		}
		ae->ae_ipaddr = dst;
		ae->ae_dev = dev;
		list_add_tail(&pkb->pk_list, &ae->ae_list);
		arp_request(ae);
	} else if (ae->ae_state == ARP_WAITING) {
		arpdbg("arp entry is waiting");
		list_add_tail(&pkb->pk_list, &ae->ae_list);
	} else {
		netdev_tx(dev, pkb, pkb->pk_len - ETH_HRD_SZ,
				ETH_P_IP, ae->ae_hwaddr);
	}
}

/* Assert pkb is net-order & pkb->pk_pro == ETH_P_IP */
void ip_send_out(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	pkb->pk_pro = ETH_P_IP;
	if (!pkb->pk_rtdst && rt_output(pkb) < 0) {
		free_pkb(pkb);
		return;
	}
	ip_set_checksum(iphdr);
	ipdbg(IPFMT " -> " IPFMT "(%d/%d bytes)",
			ipfmt(iphdr->ip_src), ipfmt(iphdr->ip_dst),
			iphlen(iphdr), ntohs(iphdr->ip_len));
	/* ip fragment */
	if (ntohs(iphdr->ip_len) > pkb->pk_rtdst->rt_dev->net_mtu)
		ip_send_frag(pkb->pk_rtdst->rt_dev, pkb);
	else
		ip_send_dev(pkb->pk_rtdst->rt_dev, pkb);
}

static unsigned short ipid = 0;
void ip_send_info(struct pkbuf *pkb, unsigned char tos, unsigned short len,
		unsigned char ttl, unsigned char pro, unsigned int dst)
{
	struct ip *iphdr = pkb2ip(pkb);
	/* fill header information */
	iphdr->ip_ver = IP_VERSION_4;
	iphdr->ip_hlen = IP_HRD_SZ / 4;
	iphdr->ip_tos = tos;
	iphdr->ip_len = htons(len);
	iphdr->ip_id = htons(ipid++);
	iphdr->ip_fragoff = 0;
	iphdr->ip_ttl = ttl;
	iphdr->ip_pro = pro;
	iphdr->ip_dst = dst;

	ip_send_out(pkb);
}
