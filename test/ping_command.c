#include <stdio.h>
#include <signal.h>

#include "netif.h"
#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "lib.h"
#include "route.h"

extern unsigned int net_debug;

static unsigned short id = 0;
static int size;
static int count;
static int ttl;
static unsigned ipaddr;

static int usage(void)
{
	printf(
		"Usage: ping [OPTIONS] ipaddr\n"
		"OPTIONS:\n"
		"       -s size     icmp echo size\n"
		"       -c count    times(not implemented)\n"
		"       -t ttl      time to live\n"
	);
}

static int parse_args(int argc, char **argv)
{
	int c;

	for (c = 0; c < argc; c++)
		printf("%s ", argv[c]);
	printf("\n");
	if (argc < 2)
		return -1;

	/* init options */
	size = 56;
	count = 1;
	ttl = 64;
	ipaddr = 0;

	optind = 0;	/* reinitialize getopt() */
	opterr = 0;
	while ((c = getopt(argc, argv, "s:t:c:?h")) != -1) {
		switch (c) {
		case 's':
			size = atoi(optarg);
			break;
		case 'c':
			count = atoi(optarg);
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
		printf("packet size %d is too large. Maximum is 65507\n");
		return -2;
	}
	if (ttl < 0 || ttl > 255) {
		printf("ttl %d out of range\n", ttl);
		return -2;
	}
	if (count <= 0)
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

/* raw ping: we should use raw ip instead of it sometime */
void ping(int argc, char **argv)
{
	struct pkbuf *pkb;
	struct icmp *icmphdr;
	struct ip *iphdr;
	int err;

	/* parse args */
	if ((err = parse_args(argc, argv)) < 0) {
		if (err == -1)
			usage();
		return;
	}
	/* alloc packet */
	pkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + ICMP_ECHO_HRD_SZ + size);
	iphdr = pkb2ip(pkb);
	icmphdr = (struct icmp *)iphdr->ip_data;
	/* fill icmp data */
	memset(icmphdr->icmp_d, 'x', size);
	icmphdr->icmp_type = ICMP_T_ECHOREQ;
	icmphdr->icmp_code = 0;
	icmphdr->icmp_data.echo.id = htons(++id);
	icmphdr->icmp_data.echo.seq = htons(0);
	icmphdr->icmp_cksum = 0;
	icmphdr->icmp_cksum = icmp_chksum((unsigned char *)icmphdr,
				ICMP_ECHO_HRD_SZ + size);
	printf("send to "IPFMT" id %d seq %d ttl %d\n",
			ipfmt(ipaddr),
			id,
			ntohs(icmphdr->icmp_data.echo.seq),
			ttl);
	net_debug |= NET_DEBUG_ICMP;
	ip_send_info(pkb, 0, IP_HRD_SZ + ICMP_ECHO_HRD_SZ + size,
				ttl, IP_P_ICMP, ipaddr);
	sleep(1);
	net_debug &= ~NET_DEBUG_ICMP;
}
