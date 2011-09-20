#ifndef __NETCFG_H
#define __NETCFG_H

//#define ICMP_EXC_FRAGTIME_TEST

/* see doc/brcfg.sh & doc/net_topology # TOP1 */
#ifdef CONFIG_TOP1
#define FAKE_TAP_ADDR 0x00000000	/* 0.0.0.0 */
#define FAKE_IPADDR 0x1585140a		/* 10.20.133.21 */
/* veth mac cannot be eth0 mac, otherwise eth0 will received arp packet */
#define FAKE_HWADDR "\x00\x34\x45\x67\x89\xab"
#define DEFAULT_GW 0x1485140a		/* 10.20.133.20 */
#endif

/* doc/net_topology # TOP2 */
#ifdef CONFIG_TOP2
#define FAKE_TAP_ADDR 0x0200000a	/* 10.0.0.2 */
#define FAKE_IPADDR 0x0100000a		/* 10.0.0.1 */
#define FAKE_HWADDR "\x00\x34\x45\x67\x89\xab"
#endif

#define FAKE_TAP_NETMASK 0x00ffffff	/* 255.255.255.0 */
#define FAKE_NETMASK 0x00ffffff		/* 255.255.255.0 */

#endif	/* netcfg.h */
