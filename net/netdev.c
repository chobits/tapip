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

struct pkbuf *alloc_pkb(struct netdev *nd)
{
	struct pkbuf *pkb;
	pkb = malloc(sizeof(*pkb) + nd->net_mtu + sizeof(struct ether));
	if (!pkb) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	return pkb;
}

void free_pkb(struct pkbuf *pkb)
{
	free(pkb);
}

/* Altough dev is already created, this function is safe! */
struct netdev *netdev_alloc(char *dev)
{
	struct netdev *nd;
	nd = malloc(sizeof(*nd));
	if (nd == NULL) {
		perror("malloc");
		goto err_malloc;
	}

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
err_malloc:
	return NULL;
}

void netdev_free(struct netdev *nd)
{
	if (nd->net_fd > 0)
		delete_tap(nd->net_fd);
	free(nd);
}

void netdev_fillinfo(struct netdev *nd)
{
	getname_tap(nd->net_fd, nd->net_name);
	gethwaddr_tap(nd->net_fd, nd->net_hwaddr);
	getmtu_tap(nd->net_name, &nd->net_mtu);
	getipaddr_tap(nd->net_name, &nd->net_ipaddr);
}

void netdev_send(struct netdev *nd, struct pkbuf *pkb, int len)
{
	int l;
	l = write(nd->net_fd, pkb->pk_data, len);
	if (l < 0)
		perror("write net dev");
	else
		devdbg("write net dev size: %d\n", l);
}

int netdev_recv(struct netdev *nd, struct pkbuf *pkb)
{
	int l;
	l = read(nd->net_fd, pkb->pk_data, nd->net_mtu + sizeof(struct ether));
	if (l < 0)
		perror("read net dev");
	else {
		pkb->pk_len = l;
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
		if (ret <= 0) {
			perror("poll /dev/net/tun");
			exit(EXIT_FAILURE);
		}
		/* get a packet */
		netdev_rx(nd);
	}
}
