#ifndef __ETHER_H
#define __ETHER_H

#include <string.h>

#define ETH_HRD_SZ	sizeof(struct ether)
#define ETH_ALEN	6	/* ether address len */

#define ETH_P_IP	0x0800
#define ETH_P_ARP	0x0806
#define ETH_P_RARP	0x8035

struct ether {
	unsigned char eth_dst[ETH_ALEN];	/* destination ether addr */
	unsigned char eth_src[ETH_ALEN];	/* source ether addr */
	unsigned short eth_pro;			/* packet type ID */
	unsigned char eth_data[0];		/* data field */
} __attribute__((packed));

static inline void hwacpy(void *dst, void *src)
{
	memcpy(dst, src, ETH_ALEN);
}

static inline void hwaset(void *dst, int val)
{
	memset(dst, val, ETH_ALEN);
}

static inline int hwacmp(void *hwa1, void *hwa2)
{
	return memcmp(hwa1, hwa2, ETH_ALEN);
}

#define macfmt(ha) (ha)[0], (ha)[1], (ha)[2], (ha)[3], (ha)[4], (ha)[5]
#define MACFMT "%02x:%02x:%02x:%02x:%02x:%02x"

static inline char *ethpro(unsigned short proto)
{
	if (proto == ETH_P_IP)
		return "IP";
	else if (proto == ETH_P_ARP)
		return "ARP";
	else if (proto == ETH_P_RARP)
		return "RARP";
	else
		return "unknown";
}

static inline int is_eth_multicast(unsigned char *hwa)
{
	return (hwa[0] & 0x01);
}

static inline int is_eth_broadcast(unsigned char *hwa)
{
	/*
	 * fast compare method
	 * ethernet mac broadcast is FF:FF:FF:FF:FF:FF
	 */
	return (hwa[0] & hwa[1] & hwa[2] & hwa[3] & hwa[4] & hwa[5]) == 0xff;
}

#endif	/* ether.h */
