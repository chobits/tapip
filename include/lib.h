#ifndef __LIB_H
#define __LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define gettid() syscall(SYS_gettid)


/* colour macro */
#define red(str) "\e[01;31m"#str"\e\[0m"
#define green(str) "\e[01;32m"#str"\e\[0m"
#define yellow(str) "\e[01;33m"#str"\e\[0m"
#define purple(str) "\e[01;35m"#str"\e\[0m"
#define grey(str) "\e[01;30m"#str"\e\[0m"
#define cambrigeblue(str) "\e[01;36m"#str"\e\[0m"
#define navyblue(str) "\e[01;34m"#str"\e\[0m"
#define blue(str) navyblue(str)

#define ferr(fmt, args...) fprintf(stderr, fmt, ##args)
#define dbg(fmt, args...) ferr("[%d]%s " fmt "\n", gettid(), __FUNCTION__, ##args)

#define devdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_DEV)\
		dbg(green(dev)" "fmt, ##args);\
} while (0)

#define l2dbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_L2)\
		dbg(yellow(l2)" "fmt, ##args);\
} while (0)

#define arpdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_ARP)\
		dbg(red(arp)" "fmt, ##args);\
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
		dbg(blue(ip)" "fmt, ##args);\
} while (0)

#define icmpdbg(fmt, args...)\
do {\
	if (net_debug & NET_DEBUG_ICMP)\
		dbg(purple(icmp)" "fmt, ##args);\
} while (0)

#define NET_DEBUG_DEV	0x00000001
#define NET_DEBUG_L2	0x00000002
#define NET_DEBUG_ARP	0x00000004
#define NET_DEBUG_IP	0x00000008
#define NET_DEBUG_ICMP	0x00000010
#define NET_DEBUG_UDP	0x00000020
#define NET_DEBUG_TCP	0x00000040
#define NET_DEBUG_ALL	0xffffffff

extern unsigned int net_debug;
extern void *xmalloc(int);
extern int str2ip(char *str, unsigned int *ip);

#endif	/* lib.h */
