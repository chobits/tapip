#include "netif.h"
#include "ether.h"
#include "ip.h"

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
		ipdbg("icmp");
		/* pkbdbg(pkb); */
		break;
	case IP_P_TCP:
		ipdbg("tcp");
		break;
	case IP_P_UDP:
		ipdbg("udp");
		break;
	default:
		ipdbg("unknown protocol");
		break;
	}
	free_pkb(pkb);
}

void ip_forward(struct netdev *nd, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	struct ip *iphdr = (struct ip *)ehdr->eth_data;
	ipdbg("no forward, host drops packet");
	free_pkb(pkb);
}

void ip_in(struct netdev *nd, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	struct ip *iphdr = (struct ip *)ehdr->eth_data;
	int hlen;

	if (pkb->pk_len < ETH_HRD_SZ + IP_HRD_SZ) {
		ipdbg("ip packet is too small");
		goto err_free_pkb;
	}

	if ((iphdr->ip_verlen >> 4) != IP_VERSION_4) {
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
	if (iphdr->ip_len < hlen) {
		ipdbg("ip size is unknown");
		goto err_free_pkb;
	}

	ipdbg(IPFMT " -> " IPFMT "(%d/%d bytes)",
				ipfmt(iphdr->ip_src), ipfmt(iphdr->ip_dst),
				hlen, iphdr->ip_len);
	/* Is this packet sent to us? */
	if (iphdr->ip_dst != nd->_net_ipaddr)
		ip_forward(nd, pkb);
	else
		ip_recv(pkb);
	return;

err_free_pkb:
	free_pkb(pkb);
}

