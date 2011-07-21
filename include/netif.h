#ifndef __NETIF_H
#define __NETIF_H

#define NETDEV_ALEN	6
#define NETDEV_NLEN	16	/* IFNAMSIZ */

/* network interface device */
struct netdev {
	int net_fd;				/* virtual netif file descriptor */
	int net_mtu;
	unsigned int  net_ipaddr;		/* dev binding ip address */
	unsigned char net_hwaddr[NETDEV_ALEN];	/* hardware address */
	unsigned char net_name[NETDEV_NLEN];	/* device name */
	/* our netstack information */
	unsigned int _net_ipaddr;		/* fake ip address */
	unsigned char _net_hwaddr[NETDEV_ALEN];	/* fake hardware address */
} __attribute__((packed));


#define PK_DROP		0
#define PK_RECV		1

struct pkbuf {
	int pk_len;
	unsigned char pk_data[0];
} __attribute__((packed));

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

extern struct netdev *veth;

extern struct netdev *netdev_alloc(char *dev);
extern void netdev_free(struct netdev *nd);
extern void netdev_fillinfo(struct netdev *nd);
extern void netdev_send(struct netdev *nd, struct pkbuf *pkb, int len);
extern int netdev_recv(struct netdev *nd, struct pkbuf *pkb);
extern void netdev_poll(struct netdev *nd);
extern void netdev_tx(struct netdev *nd, struct pkbuf *pkb, int len,
				unsigned short proto, unsigned char *dst);

extern void net_timer(void);
extern void netdev_interrupt(void);

extern struct pkbuf *alloc_pkb(struct netdev *nd);
extern void free_pkb(struct pkbuf *pkb);

#endif	/* netif.h */
