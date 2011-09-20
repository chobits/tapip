#ifndef __SOCK_H
#define __SOCK_H

#include "netif.h"
#include "list.h"
#include "wait.h"

/* used for _bind() argument */
struct sock_addr {
	/* no field `int family` because it only support PF_INET */
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned short src_port;	/* net order port */
	unsigned short dst_port;
} __attribute__((packed));

struct sock;
struct sock_ops {
	void (*recv_notify)(struct sock *);
	void (*send_notify)(struct sock *);
	int (*send_pkb)(struct sock *, struct pkbuf *);
	int (*send_buf)(struct sock *, void *, int, struct sock_addr *);
	struct pkbuf *(*recv)(struct sock *);
	int (*hash)(struct sock *);
	void (*unhash)(struct sock *);
	int (*bind)(struct sock *, struct sock_addr *);
	int (*connect)(struct sock *, struct sock_addr *);
	int (*set_port)(struct sock *, unsigned short);
	int (*close)(struct sock *);
	int (*listen)(struct sock *, int);
	struct sock *(*accept)(struct sock *);
};

/* PF_INET family sock structure */
struct sock {
	unsigned char protocol;	/* l4 protocol: tcp, udp */
	struct sock_addr sk_addr;
	struct socket *sock;
	struct sock_ops *ops;
	struct rtentry *sk_dst;
	/*
	 * FIXME: lock for recv_queue, or
	 *        Should we add recv_queue into recv_wait,
	 *        just using one mutex?
	 */
	struct list_head recv_queue;
	struct wait *recv_wait;
	unsigned int hash;	/* hash num for sock hash table lookup */
	struct hlist_node hash_list;
	int refcnt;
} __attribute__((packed));

#define sk_saddr sk_addr.src_addr
#define sk_daddr sk_addr.dst_addr
#define sk_sport sk_addr.src_port
#define sk_dport sk_addr.dst_port

#define DEFAULT_BACKLOG		1
#define MAX_BACKLOG		1024

#define hlist_for_each_sock(sk, node, head)\
	hlist_for_each_entry(sk, node, head, hash_list)

extern void sock_add_hash(struct sock *, struct hlist_head *);
extern void sock_del_hash(struct sock *);
extern struct sock *get_sock(struct sock *sk);
extern void free_sock(struct sock *sk);

extern void sock_recv_notify(struct sock *sk);
extern struct pkbuf *sock_recv_pkb(struct sock *sk);
extern int sock_close(struct sock *sk);
extern int sock_autobind(struct sock *);

#endif	/* sock.h */
