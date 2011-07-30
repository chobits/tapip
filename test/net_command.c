#include <stdio.h>
#include <signal.h>

#include "netif.h"
#include "arp.h"
#include "lib.h"
#include "route.h"

unsigned int net_debug = 0;

void signal_wait(int signum)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, signum);
	sigsuspend(&mask);
}

void netdebug(int argc, char **argv)
{
	do {
		argc--;
		dbg("%s", argv[argc]);
		if (strcmp(argv[argc], "dev") == 0) {
			net_debug |= NET_DEBUG_DEV;
		} else if (strcmp(argv[argc], "l2") == 0) {
			net_debug |= NET_DEBUG_L2;
		} else if (strcmp(argv[argc], "arp") == 0) {
			net_debug |= NET_DEBUG_ARP;
		} else if (strcmp(argv[argc], "ip") == 0) {
			net_debug |= NET_DEBUG_IP;
		} else if (strcmp(argv[argc], "udp") == 0) {
			net_debug |= NET_DEBUG_UDP;
		} else if (strcmp(argv[argc], "tcp") == 0) {
			net_debug |= NET_DEBUG_TCP;
		} else {
			ferr("Usage: debug (dev|l2|arp|ip|udp|tcp)+\n");
			return;
		}
	} while (argc > 1);
	ferr("enter ^C to exit debug mode\n");

	/* waiting for interrupt signal */
	signal_wait(SIGQUIT);

	net_debug = 0;	
	ferr("\nexit debug mode\n");
}

void arpcache(int argc, char **argv)
{
	arp_cache_traverse();
}

void route(int argc, char **argv)
{
	rt_traverse();
}

void ifconfig(int argc, char **argv)
{
	printf("%-10sHWaddr "MACFMT"\n"
		"          IPaddr "IPFMT"\n"
		"          mtu %d\n",
		veth->net_name,
		macfmt(veth->net_hwaddr),
		ipfmt(veth->net_ipaddr),
		veth->net_mtu);
	printf("netstack  HWaddr "MACFMT"\n"
		"          IPaddr "IPFMT"\n"
		"          RX packet:%u bytes:%u errors:%u\n"
		"          TX packet:%u bytes:%u errors:%u\n",
		macfmt(veth->_net_hwaddr),
		ipfmt(veth->_net_ipaddr),
		veth->net_stats.rx_packets,
		veth->net_stats.rx_bytes,
		veth->net_stats.rx_errors,
		veth->net_stats.tx_packets,
		veth->net_stats.tx_bytes,
		veth->net_stats.tx_errors);
}
