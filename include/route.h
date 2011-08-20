#ifndef __ROUTE_H
#define __ROUTE_H

#include "netif.h"

struct rtentry {
	struct list_head rt_list;
	unsigned int rt_net;		/* network address */
	unsigned int rt_netmask;	/* mask */
	unsigned int rt_gw;		/* gateway(next-hop) */
	unsigned int rt_flags;		/* route entry flags */
	int rt_metric;			/* distance metric */
	struct netdev *rt_dev;		/* output net device or local net device */
};

#define RT_NONE		0x00000000
#define RT_LOCALHOST	0x00000001
#define RT_DEFAULT	0x00000002

extern struct rtentry *rt_lookup(unsigned int);
extern void rt_add(unsigned int, unsigned int, unsigned int,
			int, unsigned int, struct netdev *);
extern void rt_init(void);
extern int rt_output(struct pkbuf *);
extern int rt_input(struct pkbuf *);
extern void rt_traverse(void);

#endif	/* route.h */
