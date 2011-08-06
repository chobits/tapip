#include "netif.h"
#include "ip.h"
#include "icmp.h"
#include "route.h"
#include "lib.h"

unsigned short icmp_chksum(unsigned short *data, int size)
{
	return ip_chksum(data, size);
}

void icmp_in(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct icmp *icmphdr = (struct icmp *)iphdr->ip_data;
	unsigned int tmp;
	int icmplen;

	icmplen = iphdr->ip_len - iphlen(iphdr);
	icmpdbg("%d bytes", icmplen);
	if (icmplen < ICMP_HRD_SZ) {
		icmpdbg("icmp header is too small");
		free_pkb(pkb);
		return;
	}

	if (icmp_chksum((unsigned short *)icmphdr, icmplen) != 0) {
		icmpdbg("icmp checksum is error");
		free_pkb(pkb);
		return;
	}
	switch (icmphdr->icmp_type) {
	case ICMP_T_ECHOREQ:
		icmpdbg("echo request data %d bytes", icmplen - ICMP_HRD_SZ - 4);
		if (icmphdr->icmp_code) {
			icmpdbg("echo request packet corrupted");
			free_pkb(pkb);
			return;
		}
		if (icmplen < 4 + ICMP_HRD_SZ) {
			icmpdbg("echo request packet is too small");
			free_pkb(pkb);
			return;
		}
		icmphdr->icmp_type = ICMP_T_ECHORLY;
		/*
		 * adjacent the checksum:
		 * If  ~ >>> (cksum + x + 8) >>> == 0
		 * let ~ >>> (cksum` + x ) >>> == 0
		 * then cksum` = cksum + 8
		 */
		if (icmphdr->icmp_cksum > 0xffff - ICMP_T_ECHOREQ)
			icmphdr->icmp_cksum += ICMP_T_ECHOREQ + 1;
		else
			icmphdr->icmp_cksum += ICMP_T_ECHOREQ;
		tmp = iphdr->ip_src;
		iphdr->ip_src = iphdr->ip_dst;
		iphdr->ip_dst = tmp;
		ip_hton(iphdr);
		ip_send(pkb, 0);
		break;
	default:
		icmpdbg("unknown type");
		free_pkb(pkb);
		break;
	}
}
