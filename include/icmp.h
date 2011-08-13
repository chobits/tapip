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
	} icmp_un;
	unsigned char icmp_data[0];
} __attribute__((packed));

#define icmp_id icmp_un.echo.id
#define icmp_seq icmp_un.echo.id

#define ICMP_HRD_SZ sizeof(struct icmp)

#define ICMP_T_ECHORLY		0
#define ICMP_T_DESTUNREACH	3
#define ICMP_T_REDIRECT		5
#define ICMP_T_ECHOREQ		8
#define ICMP_T_TIMEEXCEED	11

#endif	/* icmp.h */
