/*
 * ping - raw ip version
 *        It is implemented via our defined _socket apis!
 */
#include "lib.h"
#include "netif.h"
#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "route.h"
#include "socket.h"
#include "sock.h"

static unsigned short id;
static unsigned short seq;
static int size;
static int count;
static int recv;
static int psend, precv;
static int finite;
static int ttl;
static unsigned ipaddr;
static struct socket *sock;
static struct sock_addr skaddr;
static char *buf;

static void usage(void)
{
	printf(
		"Usage: ping [OPTIONS] ipaddr\n"
		"OPTIONS:\n"
		"       -s size     icmp echo size\n"
		"       -c count    times(not implemented)\n"
		"       -t ttl      time to live\n"
	);
}

void init_options(void)
{
	buf = NULL;
	sock = NULL;
	size = 56;
	finite = 0;
	recv = count = 0;
	ttl = 64;
	ipaddr = 0;
	seq = 0;
	psend = precv = 0;
	memset(&skaddr, 0x0, sizeof(skaddr));
	id++;
}

static int parse_args(int argc, char **argv)
{
	int c;

	if (argc < 2)
		return -1;

	/* reinitialize getopt() */
	optind = 0;
	opterr = 0;
	while ((c = getopt(argc, argv, "s:t:c:?h")) != -1) {
		switch (c) {
		case 's':
			size = atoi(optarg);
			break;
		case 'c':
			recv = count = atoi(optarg);
			finite = 1;
			break;
		case 't':
			ttl = atoi(optarg);
			break;
		case 'h':
		case '?':
		default:
			return -1;
		}
	}
	if (size < 0 || size > 65507) {
		printf("Packet size %d is too large. Maximum is 65507\n", size);
		return -2;
	}
	if (ttl < 0 || ttl > 255) {
		printf("ttl %d out of range\n", ttl);
		return -2;
	}
	if (finite && count <= 0)
		printf("bad number of packets to transmit\n");

	argc -= optind;
	argv += optind;
	if (argc != 1)
		return -1;

	if (str2ip(*argv, &ipaddr) < 0) {
		printf("bad ip address %s\n", *argv);
		return -2;
	}
	return 0;
}

static void close_socket(void)
{
	struct socket *tmp;
	if (sock) {
		/*
		 * set sock to NULL before close it
		 * Otherwise shell thread close it when I enter Ctrl + C to
		 * send an interrupt signal to main thread,
		 * and then ping thread will wake up to close it also,
		 * which cause a segment fault for double freeing.
		 * FIXME: send tty signal to ping thread
		 */
		tmp = sock;
		sock = NULL;
		_close(tmp);
	}
}

static void send_packet(void)
{
	if (!buf)
		buf = xmalloc(size + ICMP_HRD_SZ);
	struct icmp *icmphdr = (struct icmp *)buf;
	static int first = 1;
	if (first) {
		printf("PING "IPFMT" %d(%d) bytes of data\n",
			ipfmt(ipaddr),
			size,
			size + ICMP_HRD_SZ + IP_HRD_SZ);
		first = 0;
	}


	/* fill icmp data */
	memset(icmphdr->icmp_data, 'x', size);
	icmphdr->icmp_type = ICMP_T_ECHOREQ;
	icmphdr->icmp_code = 0;
	icmphdr->icmp_id = htons(id);
	icmphdr->icmp_seq = htons(seq);
	icmphdr->icmp_cksum = 0;
	icmphdr->icmp_cksum = icmp_chksum((unsigned short *)icmphdr,
			ICMP_HRD_SZ + size);
	seq++;
	/* socket apis */
	_send(sock, buf, ICMP_HRD_SZ + size, &skaddr);
	psend++;
}

static void sigalrm(int num)
{
	send_packet();
	if (!finite || --count > 0) {
		alarm(1);
	} else if (count == 0) {
		alarm(1);
		count--;
	} else {
		close_socket();
	}
}

static void ping_stat(void)
{
	printf("\n"
		"--- " IPFMT " ping statistics ---\n"
		"%d packets transmitted, %d received, %d%% packet loss\n",
		ipfmt(ipaddr), psend, precv, (psend - precv) * 100 / psend);
}

static void sigint(int num)
{
	alarm(0);
	/* if we dont close socket, then recv still block socket!! */
	close_socket();
}

static void recv_packet(void)
{
	struct pkbuf *pkb;
	struct ip *iphdr;
	struct icmp *icmphdr;
	while (!finite || recv > 0) {
		pkb = _recv(sock);
		if (!pkb)
			break;
		iphdr = pkb2ip(pkb);
		icmphdr = ip2icmp(iphdr);
		if (iphdr->ip_pro == IP_P_ICMP &&
			ntohs(icmphdr->icmp_id) == id &&
			icmphdr->icmp_type == ICMP_T_ECHORLY) {
			recv--;
			printf("%d bytes from " IPFMT ": icmp_seq=%d ttl=%d\n",
							ipdlen(iphdr),
							ipfmt(iphdr->ip_src),
							ntohs(icmphdr->icmp_seq),
							iphdr->ip_ttl);
			precv++;
		}
		free_pkb(pkb);
	}
}

void ping(int argc, char **argv)
{
	int err;

	/* init options */
	init_options();

	/* parse args */
	if ((err = parse_args(argc, argv)) < 0) {
		if (err == -1)
			usage();
		return;
	}

	/* signal install */
	signal(SIGALRM, sigalrm);
	signal(SIGINT, sigint);

	/* address */
	skaddr.dst_addr = ipaddr;
	sock = _socket(AF_INET, SOCK_RAW, IP_P_ICMP);
	/* send request */
	sigalrm(SIGALRM);
	/* receive reply */
	recv_packet();

	alarm(0);
	close_socket();
	if (buf)
		free(buf);
	ping_stat();
}
