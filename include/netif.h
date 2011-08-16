#ifndef __NETIF_H
#define __NETIF_H

#define NETDEV_ALEN	6
#define NETDEV_NLEN	16	/* IFNAMSIZ */

#include "list.h"
struct pkbuf;
struct netdev;

struct netstats {
	unsigned int rx_packets;
	unsigned int tx_packets;
	unsigned int rx_errors;
	unsigned int tx_errors;
	unsigned int rx_bytes;
	unsigned int tx_bytes;
};

struct netdev_ops {
	int (*xmit)(struct netdev *, struct pkbuf *);
	int (*init)(struct netdev *);
	void (*exit)(struct netdev *);
};

/* network interface device */
struct netdev {
	/* tap device information */
	int net_mtu;
	unsigned int  net_ipaddr;		/* dev binding ip address */
	unsigned int net_mask;			/* netmask */
	unsigned char net_hwaddr[NETDEV_ALEN];	/* hardware address */
	unsigned char net_name[NETDEV_NLEN];	/* device name */
	struct netdev_ops *net_ops;		/* Nic Operation */
	struct netstats net_stats;		/* protocol independent statistic */
};
#define LOCALNET(dev) ((dev)->net_ipaddr & (dev)->net_mask)

/* tap device */
struct tapdev {
	struct netdev dev;
	int fd;
};

/* packet buf */
struct pkbuf {
	struct list_head pk_list;	/* for ip fragment or arp waiting list */
	unsigned short pk_pro;		/* ethernet packet type ID */
	unsigned short pk_type;		/* packet hardware address type */
	int pk_len;
	int pk_refcnt;
	struct netdev *pk_indev;
	struct rtentry *pk_rtdst;
	unsigned char pk_data[0];
} __attribute__((packed));

/* packet hardware address type */
#define PKT_LOCALHOST	1
#define PKT_OTHERHOST	2
#define PKT_MULTICAST	3
#define PKT_BROADCAST	4

#define HOST_LITTLE_ENDIAN	/* default: little endian machine */
#ifdef HOST_LITTLE_ENDIAN

static inline unsigned short htons(unsigned short host)
{
	return (host >> 8) | (host << 8);
}
#define ntohs(net) htons(net)

static inline unsigned int htonl(unsigned int host)
{
	return ((host & 0x000000ff) << 24) |
		((host & 0x0000ff00) << 8) |
		((host & 0x00ff0000) >> 8) |
		((host & 0xff000000) >> 24);
}
#define ntohl(net) htonl(net)

#endif	/* HOST_LITTLE_ENDIAN */

extern struct tapdev *tap;
extern struct netdev *veth;
extern struct netdev *loop;

extern struct netdev *netdev_alloc(char *dev, struct netdev_ops *);
extern void netdev_free(struct netdev *nd);
extern void netdev_interrupt(void);

extern void net_timer(void);

extern struct pkbuf *alloc_pkb(int size);
extern struct pkbuf *alloc_netdev_pkb(struct netdev *nd);
extern void pkbdbg(struct pkbuf *pkb);
//#define DEBUG_PKB
#ifdef DEBUG_PKB
extern void _free_pkb(struct pkbuf *pkb);
extern void _netdev_tx(struct netdev *nd, struct pkbuf *pkb, int len,
				unsigned short proto, unsigned char *dst);
#define netdev_tx(nd, pkb, len, proto, dst)\
do {\
	dbg("");\
	_netdev_tx(nd, pkb, len, proto, dst);\
} while (0)

#define free_pkb(pkb)\
do {\
	dbg("");\
	_free_pkb(pkb);\
} while (0)
#else
extern void free_pkb(struct pkbuf *pkb);
extern void netdev_tx(struct netdev *nd, struct pkbuf *pkb, int len,
				unsigned short proto, unsigned char *dst);
#endif
extern int free_pkbs;
extern int alloc_pkbs;

#endif	/* netif.h */
