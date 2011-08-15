#include <stdio.h>
#include <signal.h>

#include "netif.h"
#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "lib.h"
#include "route.h"
#include "netcfg.h"

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
		if (strcmp(argv[argc], "dev") == 0) {
			net_debug |= NET_DEBUG_DEV;
		} else if (strcmp(argv[argc], "l2") == 0) {
			net_debug |= NET_DEBUG_L2;
		} else if (strcmp(argv[argc], "arp") == 0) {
			net_debug |= NET_DEBUG_ARP;
		} else if (strcmp(argv[argc], "ip") == 0) {
			net_debug |= NET_DEBUG_IP;
		} else if (strcmp(argv[argc], "icmp") == 0) {
			net_debug |= NET_DEBUG_ICMP;
		} else if (strcmp(argv[argc], "udp") == 0) {
			net_debug |= NET_DEBUG_UDP;
		} else if (strcmp(argv[argc], "tcp") == 0) {
			net_debug |= NET_DEBUG_TCP;
		} else if (strcmp(argv[argc], "all") == 0) {
			net_debug |= NET_DEBUG_ALL;
		} else {
			ferr("Usage: debug (dev|l2|arp|ip|icmp|udp|tcp)+\n");
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
		"          mtu %d\n"
		"          RX packet:%u bytes:%u errors:%u\n"
		"          TX packet:%u bytes:%u errors:%u\n",
		veth->net_name,
		macfmt(veth->net_hwaddr),
		ipfmt(veth->net_ipaddr),
		veth->net_mtu,
		veth->net_stats.rx_packets,
		veth->net_stats.rx_bytes,
		veth->net_stats.rx_errors,
		veth->net_stats.tx_packets,
		veth->net_stats.tx_bytes,
		veth->net_stats.tx_errors);
#ifndef CONFIG_TOP1
	printf("---[ Non local for ./net_stack ]------\n"
		"%-10sHWaddr "MACFMT"\n"
		"          IPaddr "IPFMT"\n"
		"          mtu %d\n",
		tap->dev.net_name,
		macfmt(tap->dev.net_hwaddr),
		ipfmt(tap->dev.net_ipaddr),
		tap->dev.net_mtu);
#endif
}

