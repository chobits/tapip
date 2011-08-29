#ifndef __INET_H
#define __INET_H

#include "socket.h"

struct inet_type {
	struct sock *(*alloc_sock)(int);
	int type;
	int protocol;
};

extern struct socket_ops inet_ops;
extern void inet_init(void);

#endif	/* inet.h */
