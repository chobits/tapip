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

extern void net_in(struct netdev *, struct pkbuf *);
struct netdev *veth;	/* virtual ethernet card device */
#define FAKE_IPADDR 0x0100000a	/* 10.0.0.1 */
#define FAKE_HWADDR "\x12\x34\x45\x67\x89\xab"

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
	printf("network ip address: "IPFMT"\n", ipfmt(veth->_net_ipaddr));
	printf("network hw address: "MACFMT"\n", macfmt(veth->_net_hwaddr));
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

void netdev_rx(struct netdev *nd)
{
	struct pkbuf *pkb;
	pkb = alloc_netdev_pkb(nd);
	if (netdev_recv(nd, pkb) > 0)
		net_in(nd, pkb);
	else
		free_pkb(pkb);
	/*
	 * Dont call free_pkb(pkb), we follow the rule:
	 * Who uses packet, who kills it!
	 */
}

#ifdef DEBUG_PKB
void _netdev_tx(struct netdev *nd, struct pkbuf *pkb, int len,
		unsigned short proto, unsigned char *dst)
#else
void netdev_tx(struct netdev *nd, struct pkbuf *pkb, int len,
		unsigned short proto, unsigned char *dst)
#endif
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;

	/* first copy to eth_dst, maybe eth_src will be copied to eth_dst */
	hwacpy(ehdr->eth_dst, dst);
	hwacpy(ehdr->eth_src, nd->_net_hwaddr);
	ehdr->eth_pro = htons(proto);

	l2dbg(MACFMT " -> " MACFMT "(%s)",
				macfmt(ehdr->eth_src),
				macfmt(ehdr->eth_dst),
				ethpro(proto));

	netdev_send(nd, pkb, len + sizeof(struct ether));
	free_pkb(pkb);
}

/* L2 protocol parsing */
void net_in(struct netdev *nd, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	int proto, len;

	len = pkb->pk_len;
	if (pkb->pk_len < ETH_HRD_SZ) {
		free_pkb(pkb);
		dbg("received packet is too small:%d bytes"), pkb->pk_len;
		return;
	}
	proto = ntohs(ehdr->eth_pro);

	l2dbg(MACFMT " -> " MACFMT "(%s)",
				macfmt(ehdr->eth_src),
				macfmt(ehdr->eth_dst),
				ethpro(proto));
	pkb->pk_pro = proto;
	switch (proto) {
	case ETH_P_RARP:
//		rarp_in(nd, pkb);
		break;
	case ETH_P_ARP:
		arp_in(nd, pkb);
		break;
	case ETH_P_IP:
		ip_in(nd, pkb);
		break;
	default:
//		dbg("unkown ether packet type");
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
