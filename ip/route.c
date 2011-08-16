#include "netif.h"
#include "ip.h"
#include "icmp.h"
#include "lib.h"
#include "route.h"
#include "list.h"

#include "netcfg.h"

static LIST_HEAD(rt_head);

struct rtentry *rt_lookup(unsigned int ipaddr)
{
	struct rtentry *rt;
	/* FIXME: lock found route entry, which may be deleted */
	list_for_each_entry(rt, &rt_head, rt_list) {
		if ((rt->rt_netmask & ipaddr) ==
			(rt->rt_netmask & rt->rt_net))
			return rt;
	}
	return NULL;
}

struct rtentry *rt_alloc(unsigned int net, unsigned int netmask,
	unsigned int gw, int metric, unsigned int flags, struct netdev *dev)
{
	struct rtentry *rt;
	rt = malloc(sizeof(*rt));
	rt->rt_net = net;
	rt->rt_netmask = netmask;
	rt->rt_gw = gw;
	rt->rt_metric = metric;
	rt->rt_flags = flags;
	rt->rt_dev = dev;
	list_init(&rt->rt_list);
	return rt;
}

void rt_add(unsigned int net, unsigned int netmask, unsigned int gw,
			int metric, unsigned int flags, struct netdev *dev)
{
	struct rtentry *rt, *rte;
	struct list_head *l;

	rt = rt_alloc(net, netmask, gw, metric, flags, dev);
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
	/* loopback */
	rt_add(LOCALNET(loop), loop->net_mask, 0, 0, RT_LOCALHOST, loop);
	/* local host */
	rt_add(veth->net_ipaddr, 0xffffffff, 0, 0, RT_LOCALHOST, loop);
	/* local net */
	rt_add(LOCALNET(veth), veth->net_mask, 0, 0, RT_NONE, veth);
	/* default route: next-hop is tap ipaddr */
	rt_add(0, 0, veth->net_ipaddr, 0, RT_DEFAULT, veth);
	dbg("route table init");
}

/* Assert pkb is host-order */
int rt_input(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct rtentry *rt = rt_lookup(iphdr->ip_dst);
	if (!rt) {
#ifndef CONFIG_TOP1
		/*
		 * RFC 1812 #4.3.3.1
		 * If a router cannot forward a packet because it has no routes
		 * at all (including no default route) to the destination
		 * specified in the packet, then the router MUST generate a
		 * Destination Unreachable, Code 0 (Network Unreachable) ICMP
		 * message.
		 */
		ip_hton(iphdr);
		icmp_send(ICMP_T_DESTUNREACH, ICMP_NET_UNREACH, 0, pkb);
#endif
		free_pkb(pkb);
		return -1;
	}
	pkb->pk_rtdst = rt;
	return 0;
}

/* Assert pkb is net-order */
int rt_output(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct rtentry *rt = rt_lookup(iphdr->ip_dst);
	if (!rt) {
		/* FIXME: icmp dest unreachable to localhost */
		ipdbg("No route entry to "IPFMT, ipfmt(iphdr->ip_dst));
		free_pkb(pkb);
		return -1;
	}
	pkb->pk_rtdst = rt;
	iphdr->ip_src = rt->rt_dev->net_ipaddr;
	return 0;
}

void rt_traverse(void)
{
	struct rtentry *rt;

	if (list_empty(&rt_head))
		return;
	printf("Destination     Gateway         Genmask         Metric Iface\n");
	list_for_each_entry(rt, &rt_head, rt_list) {
		if (rt->rt_flags & RT_LOCALHOST)
			continue;
		if (rt->rt_flags & RT_DEFAULT)
			printf("default         ");
		else
			printfs(16, IPFMT, ipfmt(rt->rt_net));
		if (rt->rt_gw == 0)
			printf("*               ");
		else
			printfs(16, IPFMT, ipfmt(rt->rt_gw));
		printfs(16, IPFMT, ipfmt(rt->rt_netmask));
		printf("%-7d", rt->rt_metric);
		printf("%s\n", rt->rt_dev->net_name);
	}
}

