#include "socket.h"
#include "sock.h"
#include "inet.h"
#include "list.h"
#include "lib.h"
#include "wait.h"

/*
 * TODO:
 * socket state manage!
 */

LIST_HEAD(listen_head);	/* listening sock list */

static void __free_socket(struct socket *sock)
{
	if (sock->ops) {
		sock->ops->close(sock);
		/* Other socket apis will not work! */
		sock->ops = NULL;
	}
	/*
	 * Now sock has nothing to do with net stack,
	 * we can free it directly !
	 */
	free(sock);
}

static void free_socket(struct socket *sock)
{
	if (--sock->refcnt <= 0)
		__free_socket(sock);
}

static struct socket* get_socket(struct socket *sock)
{
	sock->refcnt++;
	return sock;
}

static struct socket *alloc_socket(int family, int type)
{
	struct socket *sock;
	sock = xmalloc(sizeof(*sock));
	memset(sock, 0x0, sizeof(*sock));
	sock->state = SS_UNCONNECTED;
	sock->family = family;
	sock->type = type;
	wait_init(&sock->sleep);
	sock->refcnt = 1;
	return sock;
}

struct socket *_socket(int family, int type, int protocol)
{
	struct socket *sock;
	/* only support AF_INET */
	if (family != AF_INET)
		return NULL;

	sock = alloc_socket(family, type);
	/* only support AF_INET */
	sock->ops = &inet_ops;

	/* assert sock->ops->socket */
	if (sock->ops->socket(sock, protocol) < 0) {
		free_socket(sock);
		sock = NULL;
	}

	return sock;
}

int _listen(struct socket *sock, int backlog)
{
	/*
	if (backlog < 0 || backlog > MAX_BACKLOG)
		return -1;
	if (sock->state != SS_UNCONNECTED)
		return -1;
	sock->state = SS_LISTEN;
	sock->sk->backlog = backlog;
	list_add(&sock->sk->listen_list, &listen_head);
	*/
	return 0;
}

void _close(struct socket *sock)
{
	/* If sock is waited on recv/send, we wake it up first! */
	wait_exit(&sock->sleep);
	free_socket(sock);
}

int _bind(struct socket *sock, struct sock_addr *skaddr)
{
	int err = -1;
	if (!skaddr)
		goto out;
	get_socket(sock);
	if (sock->ops)
		err = sock->ops->bind(sock, skaddr);
	free_socket(sock);
out:
	return err;
}

struct socket *_accept(struct socket *sock)
{
	return NULL;
}

int _send(struct socket *sock, void *buf, int size, struct sock_addr *skaddr)
{
	int err = -1;
	if (!sock || !buf || !skaddr)
		goto out;
	get_socket(sock);
	if (sock->ops)
		err = sock->ops->send(sock, buf, size, skaddr);
	free_socket(sock);
out:
	return err;
}

struct pkbuf *_recv(struct socket *sock)
{
	struct pkbuf *pkb = NULL;
	/* get reference for _close() safe */
	get_socket(sock);
	if (sock->ops)
		pkb = sock->ops->recv(sock);
	free_socket(sock);
	return pkb;
}

void socket_init(void)
{
	inet_init();
}
