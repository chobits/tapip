#include "lib.h"
#include "netif.h"
#include "ip.h"
#include "udp.h"
#include "route.h"
#include "socket.h"
#include "sock.h"

#define debug(fmt, args...) \
do {\
	if (flags & F_DEBUG)\
		dbg(fmt "\n", ##args);\
} while (0)

static struct socket *sock;
static struct socket *csock;
static struct sock_addr skaddr;
static int quit = 0;
static void close_socket(void)
{
	struct socket *tmp;
	if (sock) {
		printf("close server\n");
		tmp = sock;
		sock = NULL;
		_close(tmp);
	}
	if (csock) {
		printf("close client\n");
		tmp = csock;
		csock = NULL;
		_close(tmp);
	}

}

static void sigint(int num)
{
        close_socket();
	quit = 1;
}

static void init_options(void)
{
	memset(&skaddr, 0x0, sizeof(skaddr));
	sock = NULL;
	csock = NULL;
	quit = 0;
}

static int init_signal(void)
{
        struct sigaction act = { };     /* zero */
        act.sa_flags = 0;               /* No restart */
        act.sa_handler = sigint;
        if (sigaction(SIGINT, &act, NULL) < 0)
                return -1;
        return 0;
}

static int connect_test(char *dst)
{
	int err;
	err = parse_ip_port(dst, &skaddr.dst_addr, &skaddr.dst_port);
	if (err)
		return err;
	if ((err = _connect(sock, &skaddr)) == 0) {
		printf("Three-way handshake is established\n");
	} else {
		printf("Three-way error\n");
	}
	return err;
}


void tcp_test(int argc, char **argv)
{
	char buf[4096];
	int len;
	if (argc > 2) {
		printf("argc != 2\n");
		return;
	}
	/* init arguments */
	init_options();
	/* signal install */
	if (init_signal() < 0)
		goto out;
	/* init socket */
	sock = _socket(AF_INET, SOCK_STREAM, 0);
	if (!sock)
		goto out;
	if (argc == 2) {
		if (connect_test(argv[1]) < 0)
			goto out;
		goto send;
	}
	skaddr.src_addr = 0;
	skaddr.src_port = 0;
	if (_bind(sock, &skaddr) < 0) {
		printf("bind error");
		goto out;
	}
	printf("bind "IPFMT":%d\n", ipfmt(sock->sk->sk_saddr),
					ntohs(sock->sk->sk_sport));
	if (_listen(sock, 10) < 0) {
		printf("listen error\n");
		goto out;
	}
	csock =_accept(sock, &skaddr);
	if (csock) {
		printf("Three-way handshake successes: from "IPFMT":%d\n\n",
				ipfmt(skaddr.src_addr), ntohs(skaddr.src_port));
	} else {
		printf("Three-way error\n");
		goto out;
	}
	while ((len = _read(csock ? csock : sock, buf, 16)) > 0) {
		printf("%.*s", len, buf);
		fflush(stdout);
	}
	printf("last _read() return %d\n", len);
	goto out;
send:
	while ((len = read(1, buf, 4096)) > 0) {
		if (quit)
			break;
		if (_write(sock, buf, len) < 0) {
			printf("_write error\n");
			break;
		}
	}
out:	/* close and out */
	close_socket();
}
