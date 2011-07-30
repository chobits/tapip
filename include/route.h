#ifndef __ROUTE_H
#define __ROUTE_H

#include "netif.h"

struct rtentry {
	struct list_head rt_list;
	unsigned int rt_ipaddr;		/* next-hop address */
	unsigned int rt_netmask;
	struct netdev *rt_dev;
};

extern struct rtentry *rt_lookup(unsigned int);
extern void rt_add(unsigned int, unsigned int, struct netdev *);
extern void rt_init(void);
extern void rt_traverse(void);

#endif	/* route.h */
