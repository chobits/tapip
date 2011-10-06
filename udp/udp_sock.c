#include "lib.h"
#include "socket.h"
#include "sock.h"
#include "udp.h"
#include "ip.h"
#include "netif.h"
#include "route.h"

#define UDP_PORTS	0x10000
#define UDP_HASH_SIZE	128
#define UDP_HASH_SLOTS	(UDP_PORTS / UDP_HASH_SIZE)
#define UDP_HASH_MASK	(UDP_HASH_SIZE - 1)
#define UDP_BEST_UPDATE	10	/* udpate every 10 times */
/* default prot range [UDP_PORT_MIN, UDP_PORT_MAX)*/
#define UDP_PORT_MIN	0x8000
#define UDP_PORT_MAX	0xf000
#define BEST_PORT_MIN	UDP_PORT_MIN
/* slot */
#define udp_hash_slot(hash)	(&udp_table.slot[hash])
#define udp_slot(port)		udp_hash_slot(port & UDP_HASH_MASK)
#define udp_best_slot()		udp_hash_slot(udp_table.best_slot)
/* slot head */
#define udp_hash_head(hash)	(&udp_hash_slot(hash)->head)
#define udp_slot_head(port)	(&udp_slot(port)->head)
#define udp_best_head()		(&udp_best_slot()->head)

struct hash_slot {
	struct hlist_head head;
	int used;
};

struct hash_table {
	struct hash_slot slot[UDP_HASH_SIZE];
	int best_slot;		/* index of best hash slot */
	int best_update;	/* when down to zero, update best field */
	pthread_mutex_t mutex;
};

static struct hash_table udp_table;
static unsigned short udp_id;

static _inline void udp_htable_lock(void)
{
	pthread_mutex_lock(&udp_table.mutex);
}

static _inline void udp_htable_unlock(void)
{
	pthread_mutex_unlock(&udp_table.mutex);
}

static _inline int __port_used(unsigned short port, struct hlist_head *head)
{
	struct hlist_node *node;
	struct sock *sk;
	hlist_for_each_sock(sk, node, head)
		if (sk->sk_sport == _htons(port))
			return 1;
	return 0;
}

static _inline int port_used(unsigned short port)
{
	return __port_used(port, udp_slot_head(port));
}

static _inline unsigned short udp_get_best_port(void)
{
	unsigned short port = udp_table.best_slot + BEST_PORT_MIN;
	struct hash_slot *best = udp_best_slot();

	/* find unused */
	if (best->used) {
		while (port < UDP_PORT_MAX) {
			if (!__port_used(port, &best->head))
				break;
			port += UDP_HASH_SIZE;
		}
		if (port >= UDP_PORT_MAX)
			return 0;
	}
	best->used++;
	return port;
}

static _inline void udp_update_best(int hash)
{
	/* update hash table best slot dynamically */
	if (udp_table.best_slot != hash) {
		if (udp_hash_slot(hash)->used < udp_best_slot()->used) {
			udp_table.best_slot = hash;
			/*
			 * How to select udpate value?
			 * 1. no change (we select this one)
			 * 2. best_update += best->used - slot->used;
			 */
		}
	}
}

static int udp_get_port_slow(void)
{
	int best_slot = udp_table.best_slot;
	int best_used = udp_best_slot()->used;
	int i;
	/* find the best one */
	for (i = 0; i < UDP_HASH_SIZE; i++) {
		if (udp_hash_slot(i)->used < best_used)	{
			best_used = udp_hash_slot(i)->used;
			best_slot = i;
		}
	}
	/* Not found, table is full */
	if (best_slot == udp_table.best_slot)
		return 0;
	/* update best slot */
	udp_table.best_slot = best_slot;
	udp_table.best_update = UDP_BEST_UPDATE;
	/* Assert that we can get one valid port! */
	return udp_get_best_port();
}

/* return net order port */
static unsigned short udp_get_port(void)
{
	struct hash_slot *best = udp_best_slot();
	unsigned short port;

	port = udp_get_best_port();
	if (port == 0)
		return udp_get_port_slow();

	/* update hash table best slot */
	if (--udp_table.best_update <= 0) {
		int i;
		udp_table.best_update = UDP_BEST_UPDATE;
		for (i = 0; i < UDP_HASH_SIZE; i++) {
			/*
			 * Should we find the minimum(best) one?
			 * No.
			 * We find the first, smaller(better) than current best!
			 */
			if (udp_table.slot[i].used < best->used) {
				udp_table.best_slot = i;
				break;
			}
		}
	}
	return _htons(port);
}

static int udp_set_sport(struct sock *sk, unsigned short nport)
{
	int hash, err = -1;

	udp_htable_lock();
	/*
	 * Order cannot be reversed:
	 *  1. check used port
	 *  2. select a number automatically and check it
	 */
	if ((nport && port_used(_ntohs(nport))) ||
		(!nport && !(nport = udp_get_port())))
		goto unlock;

	err = 0;
	/* update best slot */
	hash = _ntohs(nport) & UDP_HASH_MASK;
	udp_update_best(hash);

	/* add sock int udp hash table */
	sk->hash = hash;
	sk->sk_sport = nport;
	if (sk->ops->hash)
		sk->ops->hash(sk);
unlock:
	udp_htable_unlock();
	return err;
}

static void udp_unset_sport(struct sock *sk)
{
	struct hash_slot *slot = udp_hash_slot(sk->hash);
	slot->used--;
	/* update hash table best slot dynamically */
	udp_update_best(sk->hash);
}

static void udp_unhash(struct sock *sk)
{
	udp_unset_sport(sk);
	sock_del_hash(sk);
}

/* If user donnot call bind(), this cannot be called! */
static int udp_hash(struct sock *sk)
{
	sock_add_hash(sk, udp_hash_head(sk->hash));
	return 0;
}

static int udp_send_pkb(struct sock *sk, struct pkbuf *pkb)
{
	ip_send_out(pkb);
	return pkb->pk_len - ETH_HRD_SZ - IP_HRD_SZ - UDP_HRD_SZ;
}

static int udp_init_pkb(struct sock *sk, struct pkbuf *pkb,
		void *buf, int size, struct sock_addr *skaddr)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct udp *udphdr = (struct udp *)iphdr->ip_data;
	/* fill ip head */
	iphdr->ip_hlen = IP_HRD_SZ >> 2;
	iphdr->ip_ver = IP_VERSION_4;
	iphdr->ip_tos = 0;
	iphdr->ip_len = _htons(pkb->pk_len - ETH_HRD_SZ);
	iphdr->ip_id = _htons(udp_id);
	iphdr->ip_fragoff = 0;
	iphdr->ip_ttl = UDP_DEFAULT_TTL;
	iphdr->ip_pro = sk->protocol;	/* IP_P_UDP */
	iphdr->ip_dst = skaddr->dst_addr;
	/* FIXME:use the sk->rt_dst */
	if (rt_output(pkb) < 0)		/* fill ip src */
		return -1;
	/* fill udp */
	udphdr->src = sk->sk_sport;	/* bound local address */
	udphdr->dst = skaddr->dst_port;
	udphdr->length = _htons(size + UDP_HRD_SZ);
	memcpy(udphdr->data, buf, size);
	udpdbg(IPFMT":%d" "->" IPFMT":%d(proto %d)",
			ipfmt(iphdr->ip_src), _ntohs(udphdr->src),
			ipfmt(iphdr->ip_dst), _ntohs(udphdr->dst),
			iphdr->ip_pro);
	udp_set_checksum(iphdr, udphdr);
	return 0;
}

static int udp_send_buf(struct sock *sk, void *buf, int size,
				struct sock_addr *skaddr)
{
	struct sock_addr sk_addr;
	struct pkbuf *pkb;

	/* destination address check */
	if (size <= 0 || size > UDP_MAX_BUFSZ)
		return -1;
	if (skaddr) {
		sk_addr.dst_addr = skaddr->dst_addr;
		sk_addr.dst_port = skaddr->dst_port;
	} else if (sk->sk_dport) {
		sk_addr.dst_addr = sk->sk_daddr;
		sk_addr.dst_port = sk->sk_dport;
	}
	if (!sk_addr.dst_addr || !sk_addr.dst_port)
		return -1;
	if (!sk->sk_sport && sock_autobind(sk) < 0)
		return -1;
	/* udp packet send */
	pkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + UDP_HRD_SZ + size);
	if (udp_init_pkb(sk, pkb, buf, size, &sk_addr) < 0) {
		free_pkb(pkb);
		return -1;
	}
	if (sk->ops->send_pkb)
		return sk->ops->send_pkb(sk, pkb);
	else
		return udp_send_pkb(sk, pkb);
}

static struct sock_ops udp_ops = {
	.recv_notify = sock_recv_notify,
	.recv = sock_recv_pkb,
	.send_buf = udp_send_buf,
	.send_pkb = udp_send_pkb,
	.hash = udp_hash,
	.unhash = udp_unhash,
	.set_port = udp_set_sport,
	.close = sock_close,
};

/* @nport: net order port */
struct sock *udp_lookup_sock(unsigned short nport)
{
	struct hlist_head *head = udp_slot_head(_ntohs(nport));
	struct hlist_node *node;
	struct sock *sk;
	/* FIXME: lock for udp hash table */
	if (hlist_empty(head))
		return NULL;
	hlist_for_each_sock(sk, node, head)
		if (sk->sk_sport == nport)
			return get_sock(sk);
	return NULL;
}

void udp_init(void)
{
	struct hash_slot *slot;
	int i;
	/* init udp hash table */
	for (slot = udp_hash_slot(i = 0); i < UDP_HASH_SIZE; i++, slot++) {
		hlist_head_init(&slot->head);
		slot->used = 0;
	}
	udp_table.best_slot = 0;
	udp_table.best_update = UDP_BEST_UPDATE;
	pthread_mutex_init(&udp_table.mutex, NULL);
	/* udp ip id */
	udp_id = 0;
}

struct sock *udp_alloc_sock(int protocol)
{
	struct udp_sock *udp_sk;
	if (protocol && protocol != IP_P_UDP)
		return NULL;
	udp_sk = xzalloc(sizeof(*udp_sk));
	alloc_socks++;
	udp_sk->sk.ops = &udp_ops;
	udp_id++;
	return &udp_sk->sk;
}
