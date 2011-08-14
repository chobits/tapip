/*
 * special net device independent L2 code
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>

#include <net/if.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/if_tun.h>

#include "netif.h"
#include "ether.h"
#include "ip.h"
#include "lib.h"
#include "netcfg.h"

extern void net_in(struct netdev *, struct pkbuf *);
struct netdev *veth;	/* virtual ethernet card device */

void netdev_interrupt(void)
{
	netdev_poll(veth);
}

/* only create one virtual net device */
void netdev_init(void)
{
	veth = netdev_alloc("veth0");
	if (!veth) {
		fprintf(stderr, "net device cannot be inited.\n");
		exit(EXIT_FAILURE);
	}
	netdev_fillinfo(veth);
	/* fake information for our netstack */
	veth->_net_ipaddr = FAKE_IPADDR;
	hwacpy(veth->_net_hwaddr, FAKE_HWADDR);
	dbg("network ip address: " IPFMT, ipfmt(veth->_net_ipaddr));
	dbg("network hw address: " MACFMT, macfmt(veth->_net_hwaddr));
	/* init stats */
	veth->net_stats.rx_packets = 0;
	veth->net_stats.tx_bytes = 0;
	veth->net_stats.rx_packets = 0;
	veth->net_stats.tx_bytes = 0;
}

void netdev_exit(void)
{
	if (veth)
		netdev_free(veth);
}

void netdev_rx(struct netdev *dev)
{
	struct pkbuf *pkb;
	pkb = alloc_netdev_pkb(dev);
	if (netdev_recv(dev, pkb) > 0)
		net_in(dev, pkb);
	else
		free_pkb(pkb);
	/*
	 * Dont call free_pkb(pkb), we follow the rule:
	 * Who uses packet, who kills it!
	 */
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
	hwacpy(ehdr->eth_dst, dst);
	hwacpy(ehdr->eth_src, dev->_net_hwaddr);
	ehdr->eth_pro = htons(proto);

	l2dbg(MACFMT " -> " MACFMT "(%s)",
				macfmt(ehdr->eth_src),
				macfmt(ehdr->eth_dst),
				ethpro(proto));

	netdev_send(dev, pkb, len + sizeof(struct ether));
	free_pkb(pkb);
}

/* referred to eth_trans_type() in linux */
static struct ether *eth_init(struct netdev *dev, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	if (pkb->pk_len < ETH_HRD_SZ) {
		free_pkb(pkb);
		dbg("received packet is too small:%d bytes", pkb->pk_len);
		return NULL;
	}
	/* hardware address type */
	if (is_eth_multicast(ehdr->eth_dst)) {
		if (is_eth_broadcast(ehdr->eth_dst))
			pkb->pk_type = PKT_BROADCAST;
		else
			pkb->pk_type = PKT_MULTICAST;
	} else if (!hwacmp(ehdr->eth_dst, dev->_net_hwaddr)) {
			pkb->pk_type = PKT_LOCALHOST;
	} else {
			pkb->pk_type = PKT_OTHERHOST;
	}
	/* packet protocol */
	pkb->pk_pro = ntohs(ehdr->eth_pro);
	return ehdr;
}

/* L2 protocol parsing */
void net_in(struct netdev *dev, struct pkbuf *pkb)
{
	struct ether *ehdr = eth_init(dev, pkb);
	if (!ehdr)
		return;
	l2dbg(MACFMT " -> " MACFMT "(%s)",
				macfmt(ehdr->eth_src),
				macfmt(ehdr->eth_dst),
				ethpro(pkb->pk_pro));
	switch (pkb->pk_pro) {
	case ETH_P_RARP:
//		rarp_in(dev, pkb);
		break;
	case ETH_P_ARP:
		arp_in(dev, pkb);
		break;
	case ETH_P_IP:
		ip_in(dev, pkb);
		break;
	default:
		l2dbg("drop unkown-type packet");
		free_pkb(pkb);
		break;
	}
}

void net_timer(void)
{
	/* timer init if need */
	sleep(1);
	/* timer runs */
	while (1) {
		arp_timer(1);
		ip_timer(1);
		sleep(1);
	}
}
