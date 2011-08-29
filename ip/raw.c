#include "netif.h"
#include "socket.h"
#include "list.h"
#include "raw.h"
#include "ip.h"

static void raw_recv(struct pkbuf *pkb, struct sock *sk)
{
	/* FIFO queue */
	list_add_tail(&pkb->pk_list, &sk->recv_queue);
	/* Should we get sk? */
	pkb->pk_sk = sk;
	sk->ops->recv_notify(sk);
}

void raw_in(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct pkbuf *rawpkb;
	struct sock *sk;
	sk = raw_lookup_sock(iphdr->ip_src, iphdr->ip_dst, iphdr->ip_pro);
	while (sk) {
		rawpkb = copy_pkb(pkb);
		raw_recv(rawpkb, sk);
		/* for all matched raw sock */
		sk = raw_lookup_sock_next(sk, iphdr->ip_src, iphdr->ip_dst,
							iphdr->ip_pro);
	}
}

struct wait raw_send_wait;
struct list_head raw_send_queue;

void raw_out(void)
{
	struct pkbuf *pkb;
	while (1) {
		while (list_empty(&raw_send_queue))
			sleep_on(&raw_send_wait);

		pkb = list_first_entry(&raw_send_queue, struct pkbuf, pk_list);
		list_del_init(&pkb->pk_list);
		ip_send_out(pkb);
	}
}
