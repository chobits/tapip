/*
 *  Lowest net device code:
 *    virtual net device driver based on tap device
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <poll.h>

#include <net/if.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/if_tun.h>

#include "netif.h"
#include "ether.h"
#include "lib.h"
#include "list.h"

/* Altough dev is already created, this function is safe! */
struct netdev *netdev_alloc(char *dev)
{
	struct netdev *nd;
	nd = xmalloc(sizeof(*nd));

	/* create virtual device: tap */
	nd->net_fd = alloc_tap(dev);
	if (nd->net_fd < 0) {
		perror("alloc_tap");
		goto err_alloc_tap;
	}
	/* if EBUSY, we donot set persist to tap */
	if (!errno && ioctl(nd->net_fd, TUNSETPERSIST, 1) < 0) {
		perror("ioctl TUNSETPERSIST");
		goto err_alloc_tap;
	}
	return nd;
err_alloc_tap:
	free(nd);
	return NULL;
}

void netdev_free(struct netdev *nd)
{
	if (nd->net_fd > 0)
		delete_tap(nd->net_fd);
	free(nd);
}

#define FAKE_TAP_ADDR 0x0200000a	/* 10.0.0.2 */
#define FAKE_TAP_NETMASK 0x00ffffff	/* 255.255.255.0 */
void netdev_fillinfo(struct netdev *nd)
{
	getname_tap(nd->net_fd, nd->net_name);
	gethwaddr_tap(nd->net_fd, nd->net_hwaddr);
	getmtu_tap(nd->net_name, &nd->net_mtu);
	setipaddr_tap(nd->net_name, FAKE_TAP_ADDR);
	getipaddr_tap(nd->net_name, &nd->net_ipaddr);
	setnetmask_tap(nd->net_name, FAKE_TAP_NETMASK);
	setup_tap(nd->net_name);
}

void netdev_send(struct netdev *nd, struct pkbuf *pkb, int len)
{
	int l;
	l = write(nd->net_fd, pkb->pk_data, len);
	if (l != len) {
		dbg("write net dev");
		nd->net_stats.tx_errors++;
	} else {
		nd->net_stats.tx_packets++;
		nd->net_stats.tx_bytes += l;
		devdbg("write net dev size: %d\n", l);
	}
}

int netdev_recv(struct netdev *nd, struct pkbuf *pkb)
{
	int l;
	l = read(nd->net_fd, pkb->pk_data, nd->net_mtu + sizeof(struct ether));
	if (l < 0) {
		perror("read net dev");
		nd->net_stats.rx_errors++;
	} else {
		pkb->pk_len = l;
		nd->net_stats.rx_packets++;
		nd->net_stats.rx_bytes += l;
		devdbg("read net dev size: %d\n", l);
	}

	return l;
}

void netdev_poll(struct netdev *nd)
{
	struct pollfd pfd = {};
	int ret;

	while (1) {
		pfd.fd = nd->net_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;

		/* one event, infinite time */
		ret = poll(&pfd, 1, -1);
		if (ret <= 0)
			perrx("poll /dev/net/tun");
		/* get a packet */
		netdev_rx(nd);
	}
}
