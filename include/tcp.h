#ifndef __TCP_H
#define __TCP_H

#include "sock.h"
#include "list.h"

#define TCP_LITTLE_ENDIAN

#define TCP_DEFAULT_TTL 64

struct tcp {
	unsigned short src;	/* source port */
	unsigned short dst;	/* dest port */
	unsigned int seq;	/* sequence number */
	unsigned int ackn;	/* acknowledgment number */
#ifdef TCP_LITTLE_ENDIAN
	unsigned short	reserved:4,
			doff:4,	/* data offset(head length)in 32 bits long */
				/* control bits */
			fin:1,
			syn:1,
			rst:1,
			psh:1,	/* push */
			ack:1,	/* acknowlegment */
			urg:1,	/* urgent */
			ece:1,	/* See RFC 3168 */
			cwr:1;	/* See RFC 3168 */

#else
	unsigned short	doff:4,	/* data offset(head length)in 32 bits long */
			reserved:4,
				/* control bits */
			cwr:1,	/* See RFC 3168 */
			ece:1,	/* See RFC 3168 */
			urg:1,	/* urgent */
			ack:1,	/* acknowlegment */
			psh:1,	/* push */
			rst:1,
			syn:1,
			fin:1;
#endif
	unsigned short window;
	unsigned short checksum;
	unsigned short urgptr;		/* urgent pointer */
	unsigned char data[0];
} __attribute__((packed));

#define ip2tcp(ip) ((struct tcp *)ipdata(ip))
#define TCP_HRD_SZ (sizeof(struct tcp))
#define TCP_HRD_DOFF (TCP_HRD_SZ >> 2)
#define tcphlen(tcp) ((tcp)->doff << 2)

struct tcp_checksum_head {
	unsigned int src;	/* source address */
	unsigned int dst;	/* dest address */
	unsigned char zero;
	unsigned char ptcl;	/* tcp protocol */
	unsigned short tcplen;	/* tcp header length plus data lenght */
} __attribute__((packed));

enum tcp_state {
	TCP_CLOSED = 1,
	TCP_LISTEN,
	TCP_SYN_RECV,
	TCP_SYN_SENT,
	TCP_ESTABLISHED,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_FIN_WAIT1,
	TCP_FIN_WAIT2,
	TCP_CLOSING,
	TCP_TIME_WAIT
};

struct tcp_sock {
	struct sock sk;
	struct hlist_node bhash_list;	/* for bind hash table, e/lhash node is in sk */
	unsigned int bhash;		/* bind hash value */
	int accept_backlog;		/* current entries of accept queue */
	int backlog;			/* size of accept queue */
	struct list_head listen_queue;	/* waiting for second SYN+ACK of three-way handshake */
	struct list_head accept_queue;	/* waiting for third ACK of three-way handshake */
	struct list_head list;
	struct wait *wait_accept;
	struct wait *wait_connect;
	struct tcp_sock *parent;
	/* transmission control block (RFC 793) */
	unsigned int snd_una;	/* send unacknowledged */
	unsigned int snd_nxt;	/* send next */
	unsigned int snd_wnd;	/* send window */
	unsigned int snd_up;	/* send urgent pointer */
	unsigned int snd_wl1;	/* seq for last window update */
	unsigned int snd_wl2;	/* ack for last window update */
	unsigned int iss;	/* initial send sequence number */
	unsigned int rcv_nxt;	/* receive next */
	unsigned int rcv_wnd;	/* receive window */
	unsigned int rcv_up;	/* receive urgent pointer */
	unsigned int irs;	/* initial receive sequence number */
	unsigned int state;	/* connection state */
};
#define tcpsk(sk) ((struct tcp_sock *)sk)
#define TCP_MAX_BACKLOG 128

/* host-order tcp current segment (RFC 793) */
struct tcp_segment {
	unsigned int seq;	/* segment sequence number */
	unsigned int ack;	/* segment acknowledgment number */
	unsigned int lastseq;	/* segment last sequence number */
	unsigned int len;	/* segment length */
	unsigned int wnd;	/* segment window */
	unsigned int up;	/* segment urgent point */
	unsigned int prc;	/* segment precedence value(no used) */
	struct ip *iphdr;
	struct tcp *tcphdr;
};

static _inline int tcp_accept_queue_full(struct tcp_sock *tsk)
{
	return (tsk->accept_backlog >= tsk->backlog);
}

static _inline void tcp_accept_enqueue(struct tcp_sock *tsk)
{
	/* move it from listen queue to accept queue */
	if (!list_empty(&tsk->list))
		list_del(&tsk->list);
	list_add(&tsk->list, &tsk->parent->accept_queue);
	tsk->accept_backlog++;
}

static _inline struct tcp_sock *tcp_accept_dequeue(struct tcp_sock *tsk)
{
	struct tcp_sock *newtsk;
	newtsk = list_first_entry(&tsk->accept_queue, struct tcp_sock, list);
	list_del_init(&newtsk->list);
	tsk->accept_backlog--;
	return newtsk;
}

extern void tcp_in(struct pkbuf *pkb);
extern struct sock *tcp_lookup_sock(unsigned int src, unsigned int dst,
				unsigned int src_port, unsigned int dst_port);
extern void tcp_process(struct pkbuf *pkb, struct tcp_segment *seg, struct sock *sk);
extern struct sock *tcp_alloc_sock(int protocol);
extern int tcp_hash(struct sock *sk);
extern void tcp_init(void);
extern struct tcp_sock *get_tcp_sock(struct tcp_sock *tsk);
extern void tcp_send_reset(struct tcp_sock *tsk, struct tcp_segment *seg);
extern void tcp_send_synack(struct tcp_sock *tsk, struct tcp_segment *seg);
extern void tcp_send_ack(struct tcp_sock *tsk, struct tcp_segment *seg);
extern void tcp_send_syn(struct tcp_sock *tsk, struct tcp_segment *seg);
extern unsigned int alloc_new_iss(void);
extern int tcp_id;
#define for_each_tcp_sock(tsk, node, head)\
	hlist_for_each_entry(tsk, node, head, bhash_list)

#endif	/* tcp.h */
