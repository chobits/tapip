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
	sock = xzalloc(sizeof(*sock));
	sock->state = SS_UNCONNECTED;
	sock->family = family;
	sock->type = type;
	wait_init(&sock->sleep);
	sock->refcnt = 1;
	return sock;
}

struct socket *_socket(int family, int type, int protocol)
{
	struct socket *sock = NULL;
	/* only support AF_INET */
	if (family != AF_INET)
		goto out;
	/* alloc new socket */
	sock = alloc_socket(family, type);
	if (!sock)
		goto out;
	/* only support AF_INET */
	sock->ops = &inet_ops;
	/* assert sock->ops->socket */
	if (sock->ops->socket(sock, protocol) < 0) {
		free_socket(sock);
		sock = NULL;
	}
	/* only support AF_INET */
out:
	return sock;
}

int _listen(struct socket *sock, int backlog)
{
	int err = -1;
	if (!sock || backlog < 0)
		goto out;
	get_socket(sock);
	if (sock->ops)
		err = sock->ops->listen(sock, backlog);
	free_socket(sock);
out:
	return err;
}

void _close(struct socket *sock)
{
	if (!sock)
		return;
	/*
	 * Maybe _close() is called in interrupt signal handler,
	 * in which case there is no method to notify app the interrupt.
	 * If sock is waited on recv/accept, we wake it up first!
	 */
	wait_exit(&sock->sleep);
	free_socket(sock);
}

int _connect(struct socket *sock, struct sock_addr *skaddr)
{
	int err = -1;
	if (!sock || !skaddr)
		goto out;
	get_socket(sock);
	if (sock->ops) {
		err = sock->ops->connect(sock, skaddr);
	}
	free_socket(sock);
out:
	return err;
}

int _bind(struct socket *sock, struct sock_addr *skaddr)
{
	int err = -1;
	if (!sock || !skaddr)
		goto out;
	get_socket(sock);
	if (sock->ops)
		err = sock->ops->bind(sock, skaddr);
	free_socket(sock);
out:
	return err;
}

struct socket *_accept(struct socket *sock, struct sock_addr *skaddr)
{
	struct socket *newsock = NULL;
	int err = 0;
	if (!sock)
		goto out;
	get_socket(sock);
	/* alloc slave socket */
	newsock = alloc_socket(sock->family, sock->type);
	if (!newsock)
		goto out_free;
	newsock->ops = sock->ops;
	/* real accepting process */
	if (sock->ops)
		err = sock->ops->accept(sock, newsock, skaddr);
	if (err < 0) {
		free(newsock);
		newsock = NULL;
	}
out_free:
	free_socket(sock);
out:
	return newsock;
}

int _send(struct socket *sock, void *buf, int size, struct sock_addr *skaddr)
{
	int err = -1;
	if (!sock || !buf || size <= 0 || !skaddr)
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
	if (!sock)
		goto out;
	/* get reference for _close() safe */
	get_socket(sock);
	if (sock->ops)
		pkb = sock->ops->recv(sock);
	free_socket(sock);
out:
	return pkb;
}

int _write(struct socket *sock, void *buf, int len)
{
	int ret = -1;
	if (!sock || !buf || len <= 0)
		goto out;
	/* get reference for _close() safe */
	get_socket(sock);
	if (sock->ops)
		ret = sock->ops->write(sock, buf, len);
	free_socket(sock);
out:
	return ret;
}

int _read(struct socket *sock, void *buf, int len)
{
	int ret = -1;
	if (!sock || !buf || len <= 0)
		goto out;
	/* get reference for _close() safe */
	get_socket(sock);
	if (sock->ops)
		ret = sock->ops->read(sock, buf, len);
	free_socket(sock);
out:
	return ret;
}

void socket_init(void)
{
	inet_init();
}
