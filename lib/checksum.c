#include "lib.h"
#include "netif.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

static _inline unsigned int sum(unsigned short *data, int size,
		unsigned int origsum)
{
	while (size > 1) {
		origsum += *data++;
		size -= 2;
	}
	if (size)
		origsum += _ntohs(((*(unsigned char *)data) & 0xff) << 8);
	return origsum;
}

static _inline unsigned short checksum(unsigned short *data, int size,
					unsigned int origsum)
{
	origsum = sum(data, size, origsum);
	origsum = (origsum & 0xffff) + (origsum >> 16);
	origsum = (origsum & 0xffff) + (origsum >> 16);
	return (~origsum & 0xffff);
}

unsigned short ip_chksum(unsigned short *data, int size)
{
	return checksum(data, size, 0);
}

unsigned short icmp_chksum(unsigned short *data, int size)
{
	return checksum(data, size, 0);
}

static _inline unsigned short tcp_udp_chksum(unsigned int src, unsigned int dst,
		unsigned short proto, unsigned short len, unsigned short *data)
{
	unsigned int sum;
	/* caculate sum of tcp pseudo header */
	sum = _htons(proto) + _htons(len);
	sum += src;	/* checksum will move high short to low short */
	sum += dst;
	/* caculate sum of tcp data(tcp head and data) */
	return checksum(data, len, sum);
}

unsigned short tcp_chksum(unsigned int src, unsigned int dst,
		unsigned short len, unsigned short *data)
{
	return tcp_udp_chksum(src, dst, IP_P_TCP, len, data);
}

unsigned short udp_chksum(unsigned int src, unsigned int dst,
		unsigned short len, unsigned short *data)
{
	return tcp_udp_chksum(src, dst, IP_P_UDP, len, data);
}

void udp_set_checksum(struct ip *iphdr, struct udp *udphdr)
{
	udphdr->checksum = 0;
	udphdr->checksum = tcp_udp_chksum(iphdr->ip_src, iphdr->ip_dst,
		IP_P_UDP, _ntohs(udphdr->length), (unsigned short *)udphdr);
	/*
	 * 0 is for no checksum
	 * So use 0xffff instead of 0xffff,
	 * becasue check(sum + 0xffff) == check(sum).
	 */
	if (!udphdr->checksum)
		udphdr->checksum = 0xffff;
}

void tcp_set_checksum(struct ip *iphdr, struct tcp *tcphdr)
{
	tcphdr->checksum = 0;
	tcphdr->checksum = tcp_udp_chksum(iphdr->ip_src, iphdr->ip_dst,
		IP_P_TCP, ipndlen(iphdr), (unsigned short *)tcphdr);
}

void ip_set_checksum(struct ip *iphdr)
{
	iphdr->ip_cksum = 0;
	iphdr->ip_cksum = ip_chksum((unsigned short *)iphdr, iphlen(iphdr));
}
