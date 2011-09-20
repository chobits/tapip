#ifndef __TCP_HASH_H
#define __TCP_HASH_H

#include "sock.h"
#include "tcp.h"

/* How to select hash size? or hash algorithm? */
#define TCP_EHASH_SIZE	0x40
#define TCP_EHASH_MASK	(TCP_EHASH_SIZE - 1)

#define TCP_LHASH_SIZE	0x20
#define TCP_LHASH_MASK	(TCP_LHASH_SIZE - 1)

#define TCP_BHASH_SIZE	0x100
#define TCP_BHASH_MASK	(TCP_BHASH_SIZE - 1)
#define TCP_BPORT_MIN	0x8000
#define TCP_BPORT_MAX	0xf000

#define tcp_ehash_head(hash) (&tcp_table.etable[hash])
#define tcp_bhash_head(hash) (&tcp_table.btable[hash])
#define tcp_lhash_head(hash) (&tcp_table.ltable[hash])

struct tcp_hash_table {
	struct hlist_head etable[TCP_EHASH_SIZE];	/* establish hash table */
	struct hlist_head ltable[TCP_LHASH_SIZE];	/* listen hash table */
	struct hlist_head btable[TCP_BHASH_SIZE];	/* bind hash table */
	int bfree;					/* [bmin, bmax] */
	/* FIXME: rw lock */
};

/* reference to Linux */
static __inline unsigned int tcp_ehashfn(unsigned int src, unsigned int dst,
				unsigned short src_port, unsigned short dst_port)
{
	unsigned int hash;
	hash = (src ^ src_port) ^ (dst ^ dst_port);
	/* make all bits xor to lowest 8 bit */
	hash ^= hash >> 16;
	hash ^= hash >> 8;
	return hash & TCP_EHASH_MASK;
}

static _inline int tcp_ehash_conflict(struct hlist_head *head, struct sock *sk)
{
	struct hlist_node *node;
	struct sock *tmpsk;
	hlist_for_each_sock(tmpsk, node, head) {
		if (sk->sk_saddr == tmpsk->sk_saddr &&
			sk->sk_daddr == tmpsk->sk_daddr &&
			sk->sk_sport == tmpsk->sk_sport &&
			sk->sk_dport == tmpsk->sk_dport)
			return 1;
	}
	return 0;

}

#endif	/* tcp_hash.h */
