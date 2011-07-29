#ifndef __IP_H
#define __IP_H

#include "netif.h"
#include "list.h"

/* IP Packet Format */
#define IP_ALEN 4

#define IP_VERSION_4	4

#define IP_FRAG_RS	0x8000
#define IP_FRAG_DF	0x4000
#define IP_FRAG_MF	0x2000
#define IP_FRAG_OFF	0x1fff
#define IP_FRAG_MASK	(IP_FRAG_OFF | IP_FRAG_MF)

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

#define IP_HRD_SZ sizeof(struct ip)

#define iphlen(ip) (((ip)->ip_verlen & 0xf) * 4)
#define ipoff(ip) ((((ip)->ip_fragoff) & IP_FRAG_OFF) * 8)
#define pkb2ip(pkb) ((struct ip *)((pkb)->pk_data + ETH_HRD_SZ))

#define IPFMT "%d.%d.%d.%d"
#define ipfmt(ip)\
	(ip) & 0xff, ((ip) >> 8) & 0xff, ((ip) >> 16) & 0xff, ((ip) >> 24) & 0xff

static inline void ip_ntoh(struct ip *iphdr)
{
        iphdr->ip_len = ntohs(iphdr->ip_len);
        iphdr->ip_id = ntohs(iphdr->ip_id);
        iphdr->ip_fragoff = ntohs(iphdr->ip_fragoff);
}
#define ip_hton(ip) ip_ntoh(ip)

/* Fragment */
struct fragment {
	unsigned short frag_id;
	unsigned int frag_src;
	unsigned int frag_dst;
	int frag_ttl;		/* reassembly timer */
	unsigned int frag_rsize;/* size of received fragments */
	unsigned int frag_size;	/* total fragments size(original packet) */
	struct list_head frag_list;
	struct list_head frag_pkb;
};

extern struct pkbuf *ip_reass(struct pkbuf *);

#endif	/* ip */
