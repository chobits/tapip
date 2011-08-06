#ifndef __ROUTE_H
#define __ROUTE_H

#include "netif.h"

struct rtentry {
	struct list_head rt_list;
	unsigned int rt_net;		/* network address */
	unsigned int rt_netmask;	/* mask */
	unsigned int rt_gw;		/* gateway(next-hop) */
	int rt_metric;			/* distance metric */
	struct netdev *rt_dev;
};

extern struct rtentry *rt_lookup(unsigned int);
extern void rt_add(unsigned int, unsigned int, unsigned int,
					int, struct netdev *);
extern void rt_init(void);
extern void rt_traverse(void);

#endif	/* route.h */
