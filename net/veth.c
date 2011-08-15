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

#include "netif.h"
#include "ether.h"
#include "ip.h"
#include "lib.h"
#include "list.h"
#include "netcfg.h"

struct tapdev *tap;
struct netdev *veth;

static int tap_dev_init(void)
{
	tap = xmalloc(sizeof(*tap));
	tap->fd = alloc_tap("tap0");
	if (tap->fd < 0)
		goto free_tap;
	if (setpersist_tap(tap->fd) < 0)
		goto close_tap;
	/* set tap information */
	set_tap();
	getname_tap(tap->fd, tap->dev.net_name);
	getmtu_tap(tap->dev.net_name, &tap->dev.net_mtu);
#ifndef CONFIG_TOP1
	gethwaddr_tap(tap->fd, tap->dev.net_hwaddr);
	setipaddr_tap(tap->dev.net_name, FAKE_TAP_ADDR);
	getipaddr_tap(tap->dev.net_name, &tap->dev.net_ipaddr);
	setnetmask_tap(tap->dev.net_name, FAKE_TAP_NETMASK);
	setup_tap(tap->dev.net_name);
#endif
	unset_tap();
	return 0;

close_tap:
	close(tap->fd);
free_tap:
	free(tap);
	return -1;
}

static void veth_dev_exit(struct netdev *dev)
{
	if (dev != veth)
		perrx("Net Device Error");
	delete_tap(tap->fd);
}

static int veth_dev_init(struct netdev *dev)
{
	/* init tap: out network nic */
	if (tap_dev_init() < 0)
		return -1;
	/* init veth: information for our netstack */
	dev->net_mtu = tap->dev.net_mtu;
	dev->net_ipaddr = FAKE_IPADDR;
	dev->net_mask = FAKE_NETMASK;
	hwacpy(dev->net_hwaddr, FAKE_HWADDR);
	dbg("network ip address: " IPFMT, ipfmt(dev->net_ipaddr));
	dbg("network hw address: " MACFMT, macfmt(dev->net_hwaddr));
	/* net stats have been zero */
	return 0;
}

static int veth_xmit(struct netdev *dev, struct pkbuf *pkb)
{
	int l;
	l = write(tap->fd, pkb->pk_data, pkb->pk_len);
	if (l != pkb->pk_len) {
		devdbg("write net dev");
		dev->net_stats.tx_errors++;
	} else {
		dev->net_stats.tx_packets++;
		dev->net_stats.tx_bytes += l;
		devdbg("write net dev size: %d\n", l);
	}
	return l;
}

static struct netdev_ops veth_ops = {
	.init = veth_dev_init,
	.xmit = veth_xmit,
	.exit = veth_dev_exit,
};

static int veth_recv(struct pkbuf *pkb)
{
	int l;
	l = read(tap->fd, pkb->pk_data, pkb->pk_len);
	if (l <= 0) {
		devdbg("read net dev");
		veth->net_stats.rx_errors++;
	} else {
		devdbg("read net dev size: %d\n", l);
		veth->net_stats.rx_packets++;
		veth->net_stats.rx_bytes += l;
		pkb->pk_len = l;
	}
	return l;
}

static void veth_rx(void)
{
	struct pkbuf *pkb = alloc_netdev_pkb(veth);
	if (veth_recv(pkb) > 0)
		net_in(veth, pkb);	/* pass to upper */
	else
		free_pkb(pkb);
}

void veth_poll(void)
{
	struct pollfd pfd = {};
	int ret;
	while (1) {
		pfd.fd = tap->fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		/* one event, infinite time */
		ret = poll(&pfd, 1, -1);
		if (ret <= 0)
			perrx("poll /dev/net/tun");
		/* get a packet and handle it */
		veth_rx();
	}
}

void veth_init(void)
{
	veth = netdev_alloc("veth", &veth_ops);
}

void veth_exit(void)
{
	netdev_free(veth);
}
