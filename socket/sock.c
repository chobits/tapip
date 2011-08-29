#include "socket.h"
#include "sock.h"
#include "lib.h"
#include "list.h"

void sock_add_hash(struct sock *sk, struct hlist_head *head)
{
	get_sock(sk);
	hlist_add_head(&sk->hash_list, head);
}

void sock_del_hash(struct sock *sk)
{
	free_sock(sk);
	/* Must check whether sk is hashed! */
	if (!hlist_unhashed(&sk->hash_list))
		hlist_del(&sk->hash_list);
}

struct sock *get_sock(struct sock *sk)
{
	sk->refcnt++;
	return sk;
}

void free_sock(struct sock *sk)
{
	if (--sk->refcnt <= 0)
		free(sk);
}

/* common sock ops */
void sock_recv_notify(struct sock *sk)
{
	if (!list_empty(&sk->recv_queue))
		wake_up(sk->recv_wait);
}

struct pkbuf *sock_recv_pkb(struct sock *sk)
{
	struct pkbuf *pkb = NULL;

	while (1) {
		if (!list_empty(&sk->recv_queue)) {
			pkb = list_first_entry(&sk->recv_queue, struct pkbuf, pk_list);
			list_del_init(&pkb->pk_list);
			break;
		}
		/* FIXME: sock state handling */
		if (sleep_on(sk->recv_wait) < 0)
			break;
	}
	return pkb;
}

int sock_close(struct sock *sk)
{
	struct pkbuf *pkb;
	list_del_init(&sk->listen_list);
	sk->recv_wait = NULL;
	/* stop receiving packet */
	if (sk->ops->unhash)
		sk->ops->unhash(sk);
	/* clear receive queue */
	while (!list_empty(&sk->recv_queue)) {
		pkb = list_first_entry(&sk->recv_queue, struct pkbuf, pk_list);
		list_del(&pkb->pk_list);
		free_pkb(pkb);
	}
	return 0;
}

int sock_autobind(struct sock *sk)
{
	if (sk->ops->set_port)
		return sk->ops->set_port(sk, 0);
	return -1;
}
