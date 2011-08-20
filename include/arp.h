#ifndef __ARP_H
#define __ARP_H

#include "ether.h"
#include "ip.h"
#include "list.h"

#define ARP_ETHERNET
#define ARP_IP

/* arp cache */
#define ARP_CACHE_SZ	20
#define ARP_TIMEOUT	600	/* 10 minutes */
#define ARP_WAITTIME	1

/* arp entry state */
#define ARP_FREE	1
#define ARP_WAITING	2
#define ARP_RESOLVED	3

#define ARP_REQ_RETRY	4

struct arpentry {
	struct list_head ae_list;		/* packet pending for hard address */
	struct netdev *ae_dev;			/* associated net interface */
	int ae_retry;				/* arp reuqest retrying times */
	int ae_ttl;				/* entry timeout */
	unsigned int ae_state;			/* entry state */
	unsigned short ae_pro;			/* L3 protocol supported by arp */
	unsigned int ae_ipaddr;			/* L3 protocol address(ip) */
	unsigned char ae_hwaddr[ETH_ALEN];	/* L2 protocol address(ethernet) */
};

/* arp format */
#define ARP_HRD_ETHER		1

#define ARP_OP_REQUEST		1	/* ARP request */
#define ARP_OP_REPLY		2	/* ARP reply */
#define ARP_OP_RREQUEST		3	/* RARP request */
#define ARP_OP_RREPLY		4	/* RARP reply */
#define ARP_OP_INREQUEST	8	/* InARP request */
#define ARP_OP_INREPLY		9	/* InARP reply */

#define ARP_HRD_SZ sizeof(struct arp)

struct arp {
	unsigned short arp_hrd;		/* hardware address type */
	unsigned short arp_pro;		/* protocol address type */
	unsigned char arp_hrdlen;	/* hardware address lenth */
	unsigned char arp_prolen;	/* protocol address lenth */
	unsigned short arp_op;		/* ARP opcode(command) */
#if defined(ARP_ETHERNET) && defined(ARP_IP)	/* only support ethernet & ip */
	unsigned char arp_sha[ETH_ALEN];	/* sender hw addr */
	unsigned int arp_sip;			/* send ip addr */
	unsigned char arp_tha[ETH_ALEN];	/* target hw addr */
	unsigned int arp_tip;			/* target ip addr */
#else
	unsigned char arp_data[0];		/* arp data field */
#endif
} __attribute__((packed));


static inline void arp_hton(struct arp *ahdr)
{
	ahdr->arp_hrd = htons(ahdr->arp_hrd);
	ahdr->arp_pro = htons(ahdr->arp_pro);
	ahdr->arp_op = htons(ahdr->arp_op);
}
#define arp_ntoh(ahdr) arp_hton(ahdr)

extern void arp_cache_traverse(void);
extern void arp_cache_init(void);
extern void arp_timer(int delta);
extern void arp_proc(int);

extern struct arpentry *arp_alloc(void);
extern struct arpentry *arp_lookup(unsigned short, unsigned int);
extern struct arpentry *arp_lookup_resolv(unsigned short, unsigned int);
extern int arp_insert(struct netdev *, unsigned short, unsigned int, unsigned char *);

extern void arp_queue_drop(struct arpentry *);
extern void arp_queue_send(struct arpentry *);
extern void arp_request(struct arpentry *);
extern void arp_in(struct netdev *dev, struct pkbuf *pkb);

#endif	/* arp.h */

