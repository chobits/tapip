#ifndef __RAW_H
#define __RAW_H

#include "socket.h"
#include "list.h"
#include "sock.h"

struct sock;
struct raw_sock {
	struct sock sk;
	struct list_head list;		/* raw sock hash table node */
};

extern void raw_in(struct pkbuf *pkb);
extern struct sock *raw_lookup_sock(unsigned int, unsigned int, int);
extern struct sock *raw_lookup_sock_next(struct sock *,
			unsigned int, unsigned int, int);
extern void raw_init(void);
extern struct sock *raw_alloc_sock(int);

extern struct tapip_wait raw_send_wait;
extern struct list_head raw_send_queue;

extern void raw_out(void);

#define RAW_DEFAULT_TTL 64
#define RAW_MAX_BUFSZ	65536

#endif	/* raw.h */
