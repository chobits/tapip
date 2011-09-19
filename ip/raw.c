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
	/* FIXME: lock for raw lookup */
	sk = raw_lookup_sock(iphdr->ip_src, iphdr->ip_dst, iphdr->ip_pro);
	while (sk) {
		rawpkb = copy_pkb(pkb);
		raw_recv(rawpkb, sk);
		/* for all matched raw sock */
		sk = raw_lookup_sock_next(sk, iphdr->ip_src, iphdr->ip_dst,
							iphdr->ip_pro);
	}
}
