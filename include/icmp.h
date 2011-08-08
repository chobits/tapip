#ifndef __ICMP_H
#define __ICMP_H

struct icmp {
	unsigned char icmp_type;
	unsigned char icmp_code;
	unsigned short icmp_cksum;
	union {
		struct {
			unsigned short id;	/* identifier */
			unsigned short seq;	/* sequence number */
		} echo;
	} icmp_data;
	unsigned char icmp_d[0];
} __attribute__((packed));

#define ICMP_HRD_SZ 4
#define ICMP_ECHO_HRD_SZ 8

#define ICMP_T_ECHORLY 0
#define ICMP_T_ECHOREQ 8

#endif	/* icmp.h */
