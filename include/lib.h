#ifndef __LIB_H
#define __LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

/* pthread */
#include <pthread.h>
/* Why does not pthread.h extern this function? */
extern int pthread_mutexattr_settype(pthread_mutexattr_t *, int);
typedef void *(*pfunc_t)(void *);
extern pthread_t threads[3];
extern int newthread(pfunc_t thread_func);

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
#define dbg(fmt, args...) ferr("[%d]%s " fmt "\n", (int)gettid(), __FUNCTION__, ##args)

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
		dbg(purple(udp)" "fmt, ##args);\
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
extern void perrx(char *str);
extern int str2ip(char *str, unsigned int *ip);
extern void printfs(int mlen, const char *fmt, ...);
extern int parse_ip_port(char *, unsigned int *, unsigned short *);

extern unsigned short ip_chksum(unsigned short *data, int size);
extern unsigned short icmp_chksum(unsigned short *data, int size);
extern unsigned short tcp_chksum(unsigned int src, unsigned dst,
		unsigned short len, unsigned short *data);
extern unsigned short udp_chksum(unsigned int src, unsigned int dst,
		unsigned short len, unsigned short *data);
#endif	/* lib.h */
