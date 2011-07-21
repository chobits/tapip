#ifndef __LIB_H
#define __LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ferr(fmt, args...) fprintf(stderr, fmt, ##args)
#define dbg(fmt, args...) ferr("%s " fmt "\n", __FUNCTION__, ##args)

#define devdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_DEV)\
		dbg(fmt, ##args);\
} while (0)

#define l2dbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_L2)\
		dbg(fmt, ##args);\
} while (0)

#define arpdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_ARP)\
		dbg(fmt, ##args);\
} while (0)

#define udpdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_UDP)\
		dbg(fmt, ##args);\
} while (0)

#define tcpdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_TCP)\
		dbg(fmt, ##args);\
} while (0)

#define ipdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_IP)\
		dbg(fmt, ##args);\
} while (0)

#define NET_DEBUG_DEV	0x00000001
#define NET_DEBUG_L2	0x00000002
#define NET_DEBUG_ARP	0x00000004
#define NET_DEBUG_IP	0x00000008
#define NET_DEBUG_UDP	0x00000010
#define NET_DEBUG_TCP	0x00000020

extern unsigned int net_debug;

#endif	/* lib.h */
