#include "lib.h"
#include "netif.h"
#include "ip.h"
#include "udp.h"
#include "route.h"
#include "socket.h"
#include "sock.h"

#define F_BIND		1
#define F_CONNECT	2
#define F_DEBUG		4

#define debug(fmt, args...) \
do {\
	if (flags & F_DEBUG)\
		dbg(fmt "\n", ##args);\
} while (0)

static unsigned int flags;
static struct socket *sock;
static struct sock_addr skaddr;

static void sigint(int num)
{
	/* if we dont close socket, then recv still block socket!! */
	if (sock) {
		_close(sock);
		sock = NULL;
	}
}

static void usage(void)
{
	printf(
		"udp_test - simple arbitrary UDP connections and listens\n"
		"Usage: udp_test [OPTIONS]\n"
		"OPTIONS:\n"
		"      -d             display extra debug information\n"
		"      -b addr:port   listen model: bind addr:port\n"
		"      -c addr:port   send model: connect addr:port\n"
		"      -h             display help information\n"
	);
}

static void send_packet(void)
{
	char buf[512];
	int len;

	while (1) {
		/*
		 * FIXME: I have set SIGINT norestart,
		 *  Why cannot read return from interrupt at right!
		 */
		len = read(0, buf, 512);
		if (len <= 0) {
			debug("read error");
			break;
		}
		if (_send(sock, buf, len, &skaddr) != len) {
			debug("send error\n");
			break;
		}
	}
}

static void recv_packet(void)
{
	struct pkbuf *pkb;
	struct ip *iphdr;
	struct udp *udphdr;
	int len, quit;
	/* 
	 * Dont define local variable: struct sock_addr skaddr,
	 * which conflicts with global variable: skaddr!
	 */

	/* bind */
	if (_bind(sock, &skaddr) < 0) {
		ferr("bind error\n");
		return;
	}
	debug("bind " IPFMT ":%d", ipfmt(sock->sk->sk_saddr),
					ntohs(sock->sk->sk_sport));
	/* recv loop */
	quit = 0;
	while (!quit) {
		pkb = _recv(sock);
		if (!pkb) {
			ferr("recv no pkb\n");
			break;
		}
		iphdr = pkb2ip(pkb);
		udphdr = ip2udp(iphdr);
		debug("ip: %d bytes from " IPFMT ":%d", ipdlen(iphdr),
						ipfmt(iphdr->ip_src),
						ntohs(udphdr->src));
		/* output stdin */
		len = ntohs(udphdr->length) - UDP_HRD_SZ;
		if (write(1, udphdr->data, len) != len) {
			perror("write");
			quit = 1;
		}
		free_pkb(pkb);
	}
}

static int parse_args(int argc, char **argv)
{
	int c, err = 0;
	/* reinitialize getopt() */
	optind = 0;
	opterr = 0;
	while ((c = getopt(argc, argv, "b:c:d?h")) != -1) {
		switch (c) {
		case 'd':
			flags |= F_DEBUG;
			break;
		case 'b':
			err = parse_ip_port(optarg, &skaddr.src_addr,
						&skaddr.src_port);
			flags |= F_BIND;
			break;
		case 'c':
			err = parse_ip_port(optarg, &skaddr.dst_addr,
						&skaddr.dst_port);
			flags |= F_CONNECT;
			break;
		case 'h':
		case '?':
		default:
			return -1;
		}
		if (err < 0) {
			printf("%s:address format is error\n", optarg);
			return -2;
		}
	}

	if ((flags & (F_BIND|F_CONNECT)) == (F_BIND|F_CONNECT))
		return -1;
	if ((flags & (F_BIND|F_CONNECT)) == 0)
		return -1;

	argc -= optind;
	argv += optind;
	if (argc > 0)
		return -1;

	return 0;
}

static void init_options(void)
{
	memset(&skaddr, 0x0, sizeof(skaddr));
	flags = 0;
	sock = NULL;
}

void udp_test(int argc, char **argv)
{
	struct sigaction act = { };
	int err;
	/* init arguments */
	init_options();
	/* parse arguments */
	if ((err = parse_args(argc, argv)) < 0) {
		if (err == -1)
			usage();
		return;
	}
	/* signal install */
	act.sa_flags = 0;	/* No restart */
	act.sa_handler = sigint;
	if (sigaction(SIGINT, &act, NULL) < 0)
		goto out;
	/* init socket */
	sock = _socket(AF_INET, SOCK_DGRAM, 0);
	if (!sock)
		goto out;
	/* receive reply */
	if (flags & F_BIND)
		recv_packet();
	else
		send_packet();
	/* close and out */
out:
	if (sock) {
		_close(sock);
		sock = NULL;
	}
}
