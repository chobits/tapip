#include "netif.h"
#include "ether.h"
#include "arp.h"
#include "ip.h"
#include "route.h"

#include "lib.h"

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

void ip_recv(struct pkbuf *pkb)
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

void ip_send_dev(struct netdev *dev, struct pkbuf *pkb, unsigned int dst)
{
	struct arpentry *ae;
	ae = arp_lookup(ETH_P_IP, dst);
	if (!ae) {
		ipdbg("not found arp entry");
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
		ipdbg("arp entry is waiting");
		list_add_tail(&pkb->pk_list, &ae->ae_list);
	} else {
		netdev_tx(dev, pkb, pkb->pk_len - ETH_HRD_SZ,
				ETH_P_IP, ae->ae_hwaddr);
	}
}

static unsigned short ipid = 0;

/* pkb data is net-order */
void ip_send(struct pkbuf *pkb, int fwd)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct rtentry *rt;
	unsigned int dst;

	ipdbg(IPFMT " -> " IPFMT "(%d/%d bytes) %s",
				ipfmt(iphdr->ip_src), ipfmt(iphdr->ip_dst),
				iphlen(iphdr), ntohs(iphdr->ip_len),
				fwd ? "forwarding" : "");
	/* ip routing */
	rt = rt_lookup(iphdr->ip_dst);
	if (!rt) {
		ipdbg("No route entry");
		free_pkb(pkb);
		/* FIXME: if (fwd) icmp dest unreachable */
		return;
	}
	if (rt->rt_net == 0 || rt->rt_metric > 0)	/* default route or remote dst */
		dst = rt->rt_gw;
	else
		dst = iphdr->ip_dst;
	ipdbg("routing to next-hop: " IPFMT, ipfmt(dst));
	if (!fwd) {
		iphdr->ip_src = rt->rt_dev->_net_ipaddr;
		ip_setchksum(iphdr);
	}
	/* ip fragment */
	if (ntohs(iphdr->ip_len) > rt->rt_dev->net_mtu)
		ip_send_frag(rt->rt_dev, pkb, dst);
	else
		ip_send_dev(rt->rt_dev, pkb, dst);
}

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

	ip_send(pkb, 0);
}

void ip_forward(struct netdev *nd, struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct rtentry *rt;
	if (--iphdr->ip_ttl <= 0) {
		free_pkb(pkb);
		/* FIXME: icmp timeout */
		return;
	}
	/* FIXME: ajacent checksum for decreased ttl */
	ip_setchksum(iphdr);
	ip_send(pkb, 1);
}

void ip_in(struct netdev *nd, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	struct ip *iphdr = (struct ip *)ehdr->eth_data;
	int hlen;

	/* Fussy sanity check */
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
	/* Is this packet sent to us? */
	if (iphdr->ip_dst != nd->_net_ipaddr) {
		ip_hton(iphdr);
		ip_forward(nd, pkb);
	} else
		ip_recv(pkb);
	return;

err_free_pkb:
	free_pkb(pkb);
}

