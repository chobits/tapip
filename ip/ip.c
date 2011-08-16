#include "netif.h"
#include "ether.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "route.h"

#include "lib.h"
#include "netcfg.h"

unsigned short ip_chksum(unsigned short *data, int size)
{
	unsigned int sum = 0;

	while (size > 0) {
		sum += *data++;
		size -= 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return ~sum & 0xffff;
}

void ip_setchksum(struct ip *iphdr)
{
	iphdr->ip_cksum = 0;
	iphdr->ip_cksum = ip_chksum((unsigned short *)iphdr, iphlen(iphdr));
}

void ip_recv_local(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);

	/* fragment reassambly */
	if (iphdr->ip_fragoff & (IP_FRAG_OFF | IP_FRAG_MF)) {
		if (iphdr->ip_fragoff & IP_FRAG_DF) {
			ipdbg("error fragment");
			free_pkb(pkb);
			return;
		}
		pkb = ip_reass(pkb);
		if (!pkb)
			return;
		iphdr = pkb2ip(pkb);
	}

	/* pass to upper-level */
	switch (iphdr->ip_pro) {
	case IP_P_ICMP:
		icmp_in(pkb);
		break;
	case IP_P_TCP:
		free_pkb(pkb);
		ipdbg("tcp");
		break;
	case IP_P_UDP:
		free_pkb(pkb);
		ipdbg("udp");
		break;
	default:
		free_pkb(pkb);
		ipdbg("unknown protocol");
		break;
	}
}

void ip_send_dev(struct netdev *dev, struct pkbuf *pkb)
{
	struct arpentry *ae;
	unsigned int dst;
	struct rtentry *rt = pkb->pk_rtdst;

	if (rt->rt_flags & RT_LOCALHOST) {
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
	if (rt_output(pkb) < 0)
		return;
	ip_setchksum(iphdr);
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
	pkb->pk_pro = ETH_P_IP;
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

/* Assert pkb is net-order */
void ip_forward(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct rtentry *rt = pkb->pk_rtdst;
	struct netdev *indev = pkb->pk_indev;
	unsigned int dst;
#ifdef CONFIG_TOP1
	ipdbg("host doesnt support forward!");
	goto drop_pkb;
#endif
	ipdbg(IPFMT " -> " IPFMT "(%d/%d bytes) forwarding",
				ipfmt(iphdr->ip_src), ipfmt(iphdr->ip_dst),
				iphlen(iphdr), ntohs(iphdr->ip_len));

	if (iphdr->ip_ttl <= 1) {
		icmp_send(ICMP_T_TIMEEXCEED, ICMP_EXC_TTL, 0, pkb);
		goto drop_pkb;
	}

	/* FIXME: ajacent checksum for decreased ttl */
	iphdr->ip_ttl--;
	ip_setchksum(iphdr);

	/* default route or remote dst */
	if ((rt->rt_flags & RT_DEFAULT) || rt->rt_metric > 0)
		dst = rt->rt_gw;
	else
		dst = iphdr->ip_dst;
	ipdbg("forward to next-hop "IPFMT, ipfmt(dst));
	if (indev == rt->rt_dev) {
		/*
		 * ICMP REDIRECT conditions(RFC 1812):
		 * 1. The packet is being forwarded out the same physical
		 *    interface that it was received from.
		 * 2. The IP source address in the packet is on the same Logical IP
		 *    (sub)network as the next-hop IP address.
		 * 3. The packet does not contain an IP source route option.
		 *    (Not implemented)
		 */
		struct rtentry *srt = rt_lookup(iphdr->ip_src);
		if (srt && srt->rt_metric == 0 &&
			equsubnet(srt->rt_netmask, iphdr->ip_src, dst)) {
			if (srt->rt_dev != indev) {
				ipdbg("Two NIC are connected to the same LAN");
			}
			icmp_send(ICMP_T_REDIRECT, ICMP_REDIRECT_HOST, dst, pkb);
		}
	}
	/* ip fragment */
	if (ntohs(iphdr->ip_len) > rt->rt_dev->net_mtu) {
		if (iphdr->ip_fragoff & htons(IP_FRAG_DF)) {
			icmp_send(ICMP_T_DESTUNREACH, ICMP_FRAG_NEEDED, 0, pkb);
			goto drop_pkb;
		}
		ip_send_frag(rt->rt_dev, pkb);
	} else {
		ip_send_dev(rt->rt_dev, pkb);
	}
	return;
drop_pkb:
	free_pkb(pkb);
}

void ip_recv_route(struct pkbuf *pkb)
{
	if (rt_input(pkb) < 0)
		return;
	/* Is this packet sent to us? */
	if (pkb->pk_rtdst->rt_flags & RT_LOCALHOST) {
		ip_recv_local(pkb);
	} else {
		ip_hton(pkb2ip(pkb));
		ip_forward(pkb);
	}
}

void ip_in(struct netdev *dev, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	struct ip *iphdr = (struct ip *)ehdr->eth_data;
	int hlen;

	/* Fussy sanity check */
	if (pkb->pk_type == PKT_OTHERHOST) {
		ipdbg("ip(l2) packet is not for us");
		goto err_free_pkb;
	}

	if (pkb->pk_len < ETH_HRD_SZ + IP_HRD_SZ) {
		ipdbg("ip packet is too small");
		goto err_free_pkb;
	}

	if (ipver(iphdr) != IP_VERSION_4) {
		ipdbg("ip packet is not version 4");
		goto err_free_pkb;
	}

	hlen = iphlen(iphdr);
	if (hlen < IP_HRD_SZ) {
		ipdbg("ip header is too small");
		goto err_free_pkb;
	}

	if (ip_chksum((unsigned short *)iphdr, hlen) != 0) {
		ipdbg("ip checksum is error");
		goto err_free_pkb;
	}

	ip_ntoh(iphdr);
	if (iphdr->ip_len < hlen ||
		pkb->pk_len < ETH_HRD_SZ + iphdr->ip_len) {
		ipdbg("ip size is unknown");
		goto err_free_pkb;
	}

	if (pkb->pk_len > ETH_HRD_SZ + iphdr->ip_len)
		pkb_trim(pkb, ETH_HRD_SZ + iphdr->ip_len);

	/* Now, we can take care of the main ip processing safely. */
	ipdbg(IPFMT " -> " IPFMT "(%d/%d bytes)",
				ipfmt(iphdr->ip_src), ipfmt(iphdr->ip_dst),
				hlen, iphdr->ip_len);
	ip_recv_route(pkb);
	return;

err_free_pkb:
	free_pkb(pkb);
}

