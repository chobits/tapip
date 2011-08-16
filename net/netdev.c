/*
 *  Lowest net device code:
 *    independent net device layer
 */
#include <string.h>

#include "netif.h"
#include "ether.h"
#include "lib.h"
#include "list.h"
#include "netcfg.h"

/* Altough dev is already created, this function is safe! */
struct netdev *netdev_alloc(char *devstr, struct netdev_ops *netops)
{
	struct netdev *dev;
	dev = xmalloc(sizeof(*dev));
	memset(dev, 0x0, sizeof(*dev));
	dev->net_name[NETDEV_NLEN - 1] = '\0';
	strncpy(dev->net_name, devstr, NETDEV_NLEN - 1);
	dev->net_ops = netops;
	if (netops && netops->init)
		netops->init(dev);
	return dev;
}

void netdev_free(struct netdev *dev)
{
	if (dev->net_ops && dev->net_ops->exit)
		dev->net_ops->exit(dev);
	free(dev);
}

void netdev_interrupt(void)
{
	veth_poll();
}

/* create veth and lo */
void netdev_init(void)
{
	loop_init();
	veth_init();
}

void netdev_exit(void)
{
	veth_exit();
	loop_exit();
}

#ifdef DEBUG_PKB
void _netdev_tx(struct netdev *dev, struct pkbuf *pkb, int len,
		unsigned short proto, unsigned char *dst)
#else
void netdev_tx(struct netdev *dev, struct pkbuf *pkb, int len,
		unsigned short proto, unsigned char *dst)
#endif
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;

	/* first copy to eth_dst, maybe eth_src will be copied to eth_dst */
	ehdr->eth_pro = htons(proto);
	hwacpy(ehdr->eth_dst, dst);
	hwacpy(ehdr->eth_src, dev->net_hwaddr);

	l2dbg(MACFMT " -> " MACFMT "(%s)",
				macfmt(ehdr->eth_src),
				macfmt(ehdr->eth_dst),
				ethpro(proto));

	pkb->pk_len = len + ETH_HRD_SZ;
	/* real transmit packet */
	dev->net_ops->xmit(dev, pkb);
	free_pkb(pkb);
}

