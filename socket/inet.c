#include "socket.h"
#include "sock.h"
#include "netif.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"
#include "raw.h"
#include "inet.h"
#include "lib.h"
#include "route.h"

static struct inet_type inet_type_table[SOCK_MAX] = {
	[0] = {},
	[SOCK_STREAM] = {
		.type = SOCK_STREAM,
		.protocol = IP_P_TCP,
		.alloc_sock = tcp_alloc_sock,
	},
	[SOCK_DGRAM] = {
		.type = SOCK_DGRAM,
		.protocol = IP_P_UDP,
		.alloc_sock = udp_alloc_sock,
	},
	[SOCK_RAW] = {
		.type = SOCK_RAW,
		.protocol = IP_P_IP,	/* for all IP_P_XXX */
		.alloc_sock = raw_alloc_sock,
	}
};

static int inet_socket(struct socket *sock, int protocol)
{
	struct inet_type *inet;
	struct sock *sk;
	int type;

	type = sock->type;
	if (type >= SOCK_MAX)
		return -1;

	inet = &inet_type_table[type];
	/* alloc sock and check protocol */
	sk = inet->alloc_sock(protocol);
	if (!sk)	/* protocol error or other error */
		return -1;
	/* If wildchar protocol is allowed, we set protocol defaultly */
	if (!protocol)
		protocol = inet->protocol;
	sock->sk = get_sock(sk);
	/* RawIp uses it for filtering packet. */
	list_init(&sk->recv_queue);
	hlist_node_init(&sk->hash_list);
	sk->protocol = protocol;
	sk->sock = sock;
	/* only used by raw ip */
	if (sk->hash && sk->ops->hash)
		sk->ops->hash(sk);
	return 0;
}

static int inet_close(struct socket *sock)
{
	struct sock *sk = sock->sk;
	int err = -1;
	/*
	 * 1. Double close is safe!
	 * 2. User cannot touch sk after closing,
	 *    but sk can still be handled by net stack.
	 */
	if (sk) {
		err = sk->ops->close(sk);
		free_sock(sk);
		sock->sk = NULL;
	}
	return err;
}

static int inet_accept(struct socket *sock,
		struct socket *newsock, struct sock_addr *skaddr)
{
	struct sock *sk = sock->sk;
	struct sock *newsk;
	int err = -1;
	if (!sk)
		goto out;
	newsk = sk->ops->accept(sk);
	if (newsk) {
		/* this reference for inet_close() */
		newsock->sk = get_sock(newsk);
		if (skaddr) {
			skaddr->src_addr = newsk->sk_daddr;
			skaddr->src_port = newsk->sk_dport;
		}
		err = 0;
	}
out:
	return err;
}

static int inet_listen(struct socket *sock, int backlog)
{
	struct sock *sk = sock->sk;
	int err = -1;

	if (sock->type != SOCK_STREAM)
		return -1;
	if (sk)
		err = sk->ops->listen(sk, backlog);
	return err;
}

static int inet_bind(struct socket *sock, struct sock_addr *skaddr)
{
	struct sock *sk = sock->sk;
	int err = -1;
	/* protocol defined bind (e.g: raw bind) */
	if (sk->ops->bind)
		return sk->ops->bind(sock->sk, skaddr);
	/* duplicate bind is error */
	if (sk->sk_sport)
		goto err_out;
	/* only bind local ip address */
	if (!local_address(skaddr->src_addr))
		goto err_out;
	/* bind address */
	sk->sk_saddr = skaddr->src_addr;
	/* bind port */
	if (sk->ops->set_port) {
		err = sk->ops->set_port(sk, skaddr->src_port);
		if (err < 0) {
			/* error clear */
			sk->sk_saddr = 0;
			goto err_out;
		}
	} else {
		sk->sk_sport = skaddr->src_port;
	}
	/* bind success */
	err = 0;
	/* clear connection */
	sk->sk_daddr = 0;
	sk->sk_dport = 0;
err_out:
	return err;
}

static int inet_connect(struct socket *sock, struct sock_addr *skaddr)
{
	struct sock *sk = sock->sk;
	int err = -1;
	/* sanity check */
	if (!skaddr->dst_port || !skaddr->dst_addr)
		goto out;
	/* Not allow double connection */
	if (sk->sk_dport)
		goto out;
	/* if not bind, we try auto bind */
	if (!sk->sk_sport && sock_autobind(sk) < 0)
		goto out;
	/* ROUTE */
	{
		struct rtentry *rt = rt_lookup(skaddr->dst_addr);
		if (!rt)
			goto out;
		sk->sk_dst = rt;
		sk->sk_saddr = sk->sk_dst->rt_dev->net_ipaddr;
	}
	/* protocol must support its own connect */
	if (sk->ops->connect)
		err = sk->ops->connect(sk, skaddr);
	/* if connect error happen, it will auto unbind */
out:
	return err;
}

static int inet_read(struct socket *sock, void *buf, int len)
{
	struct sock *sk = sock->sk;
	int ret = -1;
	if (sk) {
		sk->recv_wait = &sock->sleep;
		ret = sk->ops->recv_buf(sock->sk, buf, len);
		sk->recv_wait = NULL;
	}
	return ret;
}

static int inet_write(struct socket *sock, void *buf, int len)
{
	struct sock *sk = sock->sk;
	int ret = -1;
	if (sk)
		ret = sk->ops->send_buf(sock->sk, buf, len, NULL);
	return ret;
}

static int inet_send(struct socket *sock, void *buf, int size,
			struct sock_addr *skaddr)
{
	struct sock *sk = sock->sk;
	if (sk)
		return sk->ops->send_buf(sock->sk, buf, size, skaddr);
	return -1;
}

static struct pkbuf *inet_recv(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct pkbuf *pkb = NULL;
	if (sk) {
		sk->recv_wait = &sock->sleep;
		pkb = sk->ops->recv(sock->sk);
		sk->recv_wait = NULL;
	}
	return pkb;
}

struct socket_ops inet_ops = {
	.socket = inet_socket,
	.close = inet_close,
	.listen = inet_listen,
	.bind = inet_bind,
	.accept = inet_accept,
	.connect = inet_connect,
	.read = inet_read,
	.write = inet_write,
	.send = inet_send,
	.recv = inet_recv,
};

void inet_init(void)
{
	raw_init();
	udp_init();
	tcp_init();
}

