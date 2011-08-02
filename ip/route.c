#include "netif.h"
#include "ip.h"
#include "lib.h"
#include "route.h"
#include "list.h"

static LIST_HEAD(rt_head);

struct rtentry *rt_lookup(unsigned int ipaddr)
{
	struct rtentry *rt;
	ipdbg(IPFMT, ipfmt(ipaddr));
	/* FIXME: lock found route entry, which may be deleted */
	list_for_each_entry(rt, &rt_head, rt_list) {
		if ((rt->rt_netmask & ipaddr) ==
			(rt->rt_netmask & rt->rt_ipaddr))
			return rt;
	}
	return NULL;
}

struct rtentry *rt_alloc(unsigned int ipaddr,
				unsigned int netmask, struct netdev *dev)
{
	struct rtentry *rt;
	rt = malloc(sizeof(*rt));
	rt->rt_ipaddr = ipaddr;
	rt->rt_netmask = netmask;
	rt->rt_dev = dev;
	list_init(&rt->rt_list);
	return rt;
}

void rt_add(unsigned int ipaddr, unsigned int netmask, struct netdev *dev)
{
	struct rtentry *rt, *rte;
	struct list_head *l;

	rt = rt_alloc(ipaddr, netmask, dev);
	/* insert according to netmask descend-order */
	l = &rt_head;
	list_for_each_entry(rte, &rt_head, rt_list) {
		if (rt->rt_netmask >= rte->rt_netmask) {
			l = &rte->rt_list;
			break;
		}
	}
	/* if not found or the list is empty, insert to prev of head*/
	list_add_tail(&rt->rt_list, l);
}

void rt_init(void)
{
	/* next-hop is tap ip */
	rt_add(veth->net_ipaddr, 0x00ffffff, veth);
	/* default route */
	rt_add(veth->net_ipaddr, 0, veth);
}

void rt_traverse(void)
{
	struct rtentry *rt;

	if (list_empty(&rt_head))
		return;
	printf("Destination     Genmask         Iface\n");
	list_for_each_entry(rt, &rt_head, rt_list) {
		printfs(16, IPFMT, ipfmt(rt->rt_ipaddr));
		printfs(16, IPFMT, ipfmt(rt->rt_netmask));
		printf("%s\n", rt->rt_dev->net_name);
	}
}
