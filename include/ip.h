#ifndef __IP_H
#define __IP_H

#define IP_ALEN 4

#define IP_P_ICMP	1
#define IP_P_IGMP	2
#define IP_P_TCP	6
#define IP_P_EGP	8
#define IP_P_UDP	17
#define IP_P_OSPF	89

struct ip {
	unsigned char ip_verlen;	/* vertion(4bit), iphdr length(4bit) */
	unsigned char ip_tos;		/* type of service */
	unsigned short ip_len;		/* total ip packet data lenth */
	unsigned short ip_id;		/* datagram id */
	unsigned short ip_fragoff;	/* fragment offset(in 8-octet's) */
	unsigned char ip_ttl;		/* time to live, in gateway hops */
	unsigned char ip_pro;		/* IP protocol */
	unsigned short ip_cksum;	/* header checksum */
	unsigned int ip_src;		/* source address */
	unsigned int ip_dst;		/* dest address */
	unsigned char ip_data[0];	/* data field */
} __attribute__((packed));

#define IPFMT "%d.%d.%d.%d"
#define ipfmt(ip)\
	(ip) & 0xff, ((ip) >> 8) & 0xff, ((ip) >> 16) & 0xff, ((ip) >> 24) & 0xff

#endif	/* ip */
