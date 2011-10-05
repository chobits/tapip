#include "netif.h"
#include "ether.h"
#include "arp.h"
#include "ip.h"
#include "raw.h"
#include "udp.h"
#include "tcp.h"
#include "icmp.h"
#include "route.h"
#include "lib.h"

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

	/* copy pkb to raw */
	raw_in(pkb);

	/* pass to upper-level */
	switch (iphdr->ip_pro) {
	case IP_P_ICMP:
		icmp_in(pkb);
		break;
	case IP_P_TCP:
		tcp_in(pkb);
		break;
	case IP_P_UDP:
		udp_in(pkb);
		break;
	default:
		free_pkb(pkb);
		ipdbg("unknown protocol");
		break;
	}
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

