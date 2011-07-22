#include <stdio.h>
#include <signal.h>

#include "netif.h"
#include "arp.h"
#include "lib.h"

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
	if (strcmp(argv[1], "dev") == 0) {
		net_debug |= NET_DEBUG_DEV;
		ferr("l2 debug mode\n");
	} else if (strcmp(argv[1], "l2") == 0) {
		net_debug |= NET_DEBUG_L2;
		ferr("l2 debug mode\n");
	} else if (strcmp(argv[1], "arp") == 0) {
		net_debug |= NET_DEBUG_ARP;
		ferr("arp debug mode\n");
	} else if (strcmp(argv[1], "ip") == 0) {
		net_debug |= NET_DEBUG_IP;
		ferr("ip debug mode\n");
	} else if (strcmp(argv[1], "udp") == 0) {
		net_debug |= NET_DEBUG_UDP;
		ferr("udp debug mode\n");
	} else if (strcmp(argv[1], "tcp") == 0) {
		net_debug |= NET_DEBUG_TCP;
		ferr("tcp debug mode\n");
	} else {
		ferr("Usage: debug dev|l2|arp|ip|udp|tcp\n");
		return;
	}
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
