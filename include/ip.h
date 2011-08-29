#ifndef __IP_H
#define __IP_H

#include "netif.h"
#include "ether.h"
#include "list.h"

/* IP Packet Format */
#define IP_ALEN 4

#define IP_VERSION_4	4

#define IP_FRAG_RS	0x8000
#define IP_FRAG_DF	0x4000
#define IP_FRAG_MF	0x2000
#define IP_FRAG_OFF	0x1fff
#define IP_FRAG_MASK	(IP_FRAG_OFF | IP_FRAG_MF)

#define IP_P_IP		0
#define IP_P_ICMP	1
#define IP_P_IGMP	2
#define IP_P_TCP	6
#define IP_P_EGP	8
#define IP_P_UDP	17
#define IP_P_OSPF	89
#define IP_P_RAW	255
#define IP_P_MAX	256

/* default: host order is little-endian */
struct ip {
	unsigned char	ip_hlen:4,	/* header length(in 4-octet's) */
			ip_ver:4;	/* version */
	unsigned char ip_tos;		/* type of service */
	unsigned short ip_len;		/* total ip packet data length */
	unsigned short ip_id;		/* datagram id */
	unsigned short ip_fragoff;	/* fragment offset(in 8-octet's) */
	unsigned char ip_ttl;		/* time to live, in gateway hops */
	unsigned char ip_pro;		/* L4 protocol */
	unsigned short ip_cksum;	/* header checksum */
	unsigned int ip_src;		/* source address */
	unsigned int ip_dst;		/* dest address */
	unsigned char ip_data[0];	/* data field */
} __attribute__((packed));

#define IP_HRD_SZ sizeof(struct ip)

#define ipver(ip) ((ip)->ip_ver)
#define iphlen(ip) ((ip)->ip_hlen << 2)
#define ipdlen(ip) ((ip)->ip_len - iphlen(iphdr))
#define ipdata(ip) ((unsigned char *)(ip) + iphlen(ip))
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
	unsigned short frag_pro;
	unsigned int frag_hlen;
	unsigned int frag_rsize;/* size of received fragments */
	unsigned int frag_size;	/* total fragments payload size(not contain ip header) */
	int frag_ttl;		/* reassembly timer */
	unsigned int frag_flags;/* fragment flags */
	struct list_head frag_list;
	struct list_head frag_pkb;
};

#define FRAG_COMPLETE	0x00000001
#define FRAG_FIRST_IN	0x00000002
#define FRAG_LAST_IN	0x00000004
#define FRAG_FL_IN	0x00000006	/* first and last in*/

#define FRAG_TIME	30	/* exceed defragment time: 30 sec */

#define frag_head_pkb(frag)\
	list_first_entry(&(frag)->frag_pkb, struct pkbuf, pk_list)

#define MULTICAST(netip) ((0x000000f0 & (netip)) == 0x000000e0)
#define BROADCAST(netip) (((0xff000000 & (netip)) == 0xff000000) ||\
				((0xff000000 & (netip)) == 0x00000000))
static inline int equsubnet(unsigned int mask, unsigned int ip1, unsigned int ip2)
{
	return ((mask & ip1) == (mask & ip2));
}

extern struct pkbuf *ip_reass(struct pkbuf *);
extern void ip_send_dev(struct netdev *dev, struct pkbuf *pkb);
extern void ip_send_out(struct pkbuf *pkb);
extern void ip_send_info(struct pkbuf *pkb, unsigned char tos, unsigned short len,
		unsigned char ttl, unsigned char pro, unsigned int dst);
extern void ip_send_frag(struct netdev *dev, struct pkbuf *pkb);
extern void ip_setchksum(struct ip *iphdr);
extern void ip_in(struct netdev *dev, struct pkbuf *pkb);
extern void ip_timer(int delta);

#endif	/* ip */
