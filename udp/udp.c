#include "netif.h"
#include "sock.h"
#include "list.h"
#include "ether.h"
#include "icmp.h"
#include "ip.h"
#include "udp.h"

static void udp_recv(struct pkbuf *pkb, struct ip *iphdr, struct udp *udphdr)
{
	struct sock *sk;
	sk = udp_lookup_sock(udphdr->dst);
	if (!sk) {
		icmp_send(ICMP_T_DESTUNREACH, ICMP_PORT_UNREACH, 0, pkb);
		goto drop;
	}
	/* FIFO receive queue */
	list_add_tail(&pkb->pk_list, &sk->recv_queue);
	sk->ops->recv_notify(sk);
	/* We have handled the input packet with sock, so release it */
	free_sock(sk);
	return;
drop:
	free_pkb(pkb);
}

void udp_in(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct udp *udphdr = ip2udp(iphdr);
	int udplen = ipdlen(iphdr);

	if (udplen < UDP_HRD_SZ || udplen < ntohs(udphdr->length)) {
		udpdbg("udp length is too small");
		goto drop_pkb;
	}
	/* Maybe ip data has pad bytes. */
	if (udplen > ntohs(udphdr->length))
		udplen = ntohs(udphdr->length);
	if (udphdr->checksum && udp_chksum(iphdr->ip_src, iphdr->ip_dst,
				udplen, (unsigned short *)udphdr) != 0) {
		udpdbg("udp packet checksum corrupts");
		goto drop_pkb;
	}
	/*
	 * Should we check source ip address?
	 * Maybe ip layer does it for us!
	 */

	udpdbg("from "IPFMT":%d" " to " IPFMT ":%d",
			ipfmt(iphdr->ip_src), ntohs(udphdr->src),
			ipfmt(iphdr->ip_dst), ntohs(udphdr->dst));
	udp_recv(pkb, iphdr, udphdr);
	return;
drop_pkb:
	free_pkb(pkb);
}
