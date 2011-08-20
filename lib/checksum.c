#include "lib.h"

#include "netif.h"
#include "ip.h"

static _inline unsigned int sum(unsigned short *data, int size,
		unsigned int origsum)
{
	while (size > 1) {
		origsum += *data++;
		size -= 2;
	}
	if (size)
		origsum += ntohs(((*(unsigned char *)data) & 0xff) << 8);
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

unsigned short tcp_chksum(unsigned int src, unsigned dst,
		unsigned short len, unsigned short *data)
{
	unsigned int sum;
	/* caculate sum of tcp pseudo header */
	sum = htons(IP_P_TCP) + htons(len);
	sum += (src & 0xffff) + (src >> 16);
	sum += (dst & 0xffff) + (dst >> 16);

	/* caculate sum of tcp data(tcp head and data) */
	return checksum(data, len, sum);
}
