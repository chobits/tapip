#ifndef __NETCFG_H
#define __NETCFG_H

#define CONFIG_TOP2

/* see doc/brcfg.sh & doc/net_topology # TOP1 */
#ifdef CONFIG_TOP1
#define FAKE_TAP_ADDR 0x1585140a	/* 10.20.133.21 */
#define FAKE_TAP_NETMASK 0x00ffffff	/* 255.255.255.0 */
#define FAKE_IPADDR FAKE_TAP_ADDR	/* Now we are the same one! */
#define FAKE_HWADDR "\x12\x34\x45\x67\x89\xab"
#endif

/* doc/net_topology # TOP2 */
#ifdef CONFIG_TOP2
#define FAKE_TAP_ADDR 0x0200000a	/* 10.0.0.2 */
#define FAKE_TAP_NETMASK 0x00ffffff	/* 255.255.255.0 */
#define FAKE_IPADDR 0x0100000a	/* 10.0.0.1 */
#define FAKE_HWADDR "\x12\x34\x45\x67\x89\xab"
#endif

#endif	/* netcfg.h */
