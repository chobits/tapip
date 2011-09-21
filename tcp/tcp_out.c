#include "lib.h"
#include "netif.h"
#include "route.h"
#include "ip.h"
#include "tcp.h"

static int tcp_init_pkb(struct tcp_sock *tsk, struct pkbuf *pkb,
			unsigned int saddr, unsigned int daddr)
{
	struct ip *iphdr = pkb2ip(pkb);
	/* fill ip head */
	iphdr->ip_hlen = IP_HRD_SZ >> 2;
	iphdr->ip_ver = IP_VERSION_4;
	iphdr->ip_tos = 0;
	iphdr->ip_len = htons(pkb->pk_len - ETH_HRD_SZ);
	iphdr->ip_id = htons(tcp_id);
	iphdr->ip_fragoff = 0;
	iphdr->ip_ttl = TCP_DEFAULT_TTL;
	iphdr->ip_pro = IP_P_TCP;
	iphdr->ip_dst = daddr;
	/* NOTE: tsk maybe NULL, if connect doesnt exist */
	if (tsk && tsk->sk.sk_dst) {
		pkb->pk_rtdst = tsk->sk.sk_dst;
	} else {
		if (rt_output(pkb) < 0)
			return -1;
		if (tsk)
			tsk->sk.sk_dst = pkb->pk_rtdst;
	}
	iphdr->ip_src = saddr;
	return 0;
}

void tcp_send_out(struct tcp_sock *tsk, struct pkbuf *pkb, struct tcp_segment *seg)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct tcp *tcphdr = (struct tcp *)iphdr->ip_data;
	unsigned int saddr, daddr;

	if (seg) {
		daddr = seg->iphdr->ip_src;
		saddr = seg->iphdr->ip_dst;
	} else if (tsk) {
		daddr = tsk->sk.sk_daddr;
		saddr = tsk->sk.sk_saddr;
	} else
		assert(0);

	if (tcp_init_pkb(tsk, pkb, saddr, daddr) < 0) {
		free_pkb(pkb);
		return;
	}
	tcp_set_checksum(iphdr, tcphdr);
	ip_send_out(pkb);
}

/*
 * Reset algorithm is not stated directly in RFC 793,
 * but we can conclude it according to all reset generation.
 * NOTE: maybe @tsk is NULL
 */
void tcp_send_reset(struct tcp_sock *tsk, struct tcp_segment *seg)
{
	struct tcp *otcp, *tcphdr = seg->tcphdr;
	struct pkbuf *opkb;

	if (tcphdr->rst)
		return;
	opkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + TCP_HRD_SZ);
	/* fill tcp head */
	otcp = (struct tcp *)pkb2ip(opkb)->ip_data;
	memset(otcp, 0x0, TCP_HRD_SZ);
	otcp->src = tcphdr->dst;
	otcp->dst = tcphdr->src;
	if (tcphdr->ack) {
		/*
		 * Should we set ack?
		 * -Yes for xinu, it always set ack to seq+len
		 * +No for Linux
		 * +No for tapip
		 */
		otcp->seq = tcphdr->ackn;
	} else {
		otcp->ackn = htonl(seg->seq + seg->len);
		otcp->ack = 1;
	}
	otcp->doff = TCP_HRD_DOFF;
	otcp->rst = 1;
	tcpdbg("send RESET from "IPFMT":%d to "IPFMT":%d",
			ipfmt(seg->iphdr->ip_dst), ntohs(otcp->src),
			ipfmt(seg->iphdr->ip_src), ntohs(otcp->dst));
	tcp_send_out(NULL, opkb, seg);
}

/*
 * Acknowledgment algorithm is not stated directly in RFC 793,
 * but we can conclude it from all acknowledgment situation.
 */
void tcp_send_ack(struct tcp_sock *tsk, struct tcp_segment *seg)
{
	/*
	 * SYN-SENT :
	 *         SEG: SYN, acceptable ACK, no RST   (SND.NXT = SEG.SEQ+1)
	 *         <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
	 * SYN-RECEIVED / ESTABLISHED  / FIN-WAIT-1   / FIN-WAIT-2   /
	 * CLOSE-WAIT   / CLOSING      / LAST-ACK     / TIME-WAIT    :
	 *         SEG: no RST, ??ACK, ??SYN            (segment is not acceptable)
	 *         <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
	 * ESTABLISHED  / FIN-WAIT-1  / FIN-WAIT-2  / process the segment text:
	 *         SEG: ACK, no RST
	 *         <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
	 *         (This acknowledgment should be piggybacked on a segment being
	 *          transmitted if possible without incurring undue delay.)
	 */
	struct tcp *otcp, *tcphdr = seg->tcphdr;
	struct pkbuf *opkb;

	if (tcphdr->rst)
		return;
	opkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + TCP_HRD_SZ);
	/* fill tcp head */
	otcp = (struct tcp *)pkb2ip(opkb)->ip_data;
	memset(otcp, 0x0, TCP_HRD_SZ);
	otcp->src = tcphdr->dst;
	otcp->dst = tcphdr->src;
	otcp->doff = TCP_HRD_DOFF;
	otcp->seq = htonl(tsk->snd_nxt);
	otcp->ackn = htonl(tsk->rcv_nxt);
	otcp->ack = 1;
	otcp->window = 5320;	/* if no windown, the remote will not send fin to us */
	tcpdbg("send ACK(%u) to "IPFMT":%d", ntohl(otcp->ackn),
			ipfmt(seg->iphdr->ip_src), ntohs(otcp->dst));
	tcp_send_out(tsk, opkb, seg);
}

void tcp_send_synack(struct tcp_sock *tsk, struct tcp_segment *seg)
{
	/*
	 * LISTEN :
	 * SYN-SENT:
	 *         SEG: SYN, no ACK, no RST
	 *         <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>
	 *         (ISS == SND.NXT)
	 */
	struct tcp *otcp, *tcphdr = seg->tcphdr;
	struct pkbuf *opkb;

	if (tcphdr->rst)
		return;
	opkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + TCP_HRD_SZ);
	/* fill tcp head */
	otcp = (struct tcp *)pkb2ip(opkb)->ip_data;
	memset(otcp, 0x0, TCP_HRD_SZ);
	otcp->src = tcphdr->dst;
	otcp->dst = tcphdr->src;
	otcp->doff = TCP_HRD_DOFF;
	otcp->seq = htonl(tsk->iss);
	otcp->ackn = htonl(tsk->rcv_nxt);
	otcp->syn = 1;
	otcp->ack = 1;
	otcp->window = 5320;	/* if no windown, the remote will not send fin to us */
	tcpdbg("send SYN(%u)/ACK(%u) to "IPFMT":%d",
			ntohl(otcp->seq), ntohl(otcp->ackn),
			ipfmt(seg->iphdr->ip_dst), ntohs(otcp->dst));
	tcp_send_out(tsk, opkb, seg);
}

void tcp_send_syn(struct tcp_sock *tsk, struct tcp_segment *seg)
{
	/*
	 * SYN-SENT:
	 */
	struct tcp *otcp;
	struct pkbuf *opkb;

	opkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + TCP_HRD_SZ);
	/* fill tcp head */
	otcp = (struct tcp *)pkb2ip(opkb)->ip_data;
	memset(otcp, 0x0, TCP_HRD_SZ);
	otcp->src = tsk->sk.sk_sport;
	otcp->dst = tsk->sk.sk_dport;
	otcp->doff = TCP_HRD_DOFF;
	otcp->seq = htonl(tsk->iss);
	otcp->syn = 1;
	tcpdbg("send SYN(%u) to "IPFMT":%d", ntohl(otcp->seq),
			ipfmt(tsk->sk.sk_daddr), ntohs(otcp->dst));
	tcp_send_out(tsk, opkb, seg);
}

void tcp_send_fin(struct tcp_sock *tsk)
{
	struct tcp *otcp;
	struct pkbuf *opkb;

	opkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + TCP_HRD_SZ);
	/* fill tcp head */
	otcp = (struct tcp *)pkb2ip(opkb)->ip_data;
	memset(otcp, 0x0, TCP_HRD_SZ);
	otcp->src = tsk->sk.sk_sport;
	otcp->dst = tsk->sk.sk_dport;
	otcp->doff = TCP_HRD_DOFF;
	otcp->seq = htonl(tsk->snd_nxt);
	otcp->window = 5320;	/* if no windown, the remote will not send fin to us */
	otcp->fin = 1;
	/*
	 * Should we send an ACK?
	 * Yes, tcp stack will drop packet if it has no ACK bit
	 * according to RFC 793 #SEGMENT RECEIVE
	 */
	otcp->ackn = htonl(tsk->rcv_nxt);
	otcp->ack = 1;
	tcpdbg("send FIN(%u)/ACK(%u) to "IPFMT":%d",
			ntohl(otcp->seq), ntohl(otcp->ackn),
			ipfmt(tsk->sk.sk_daddr), ntohs(otcp->dst));
	tcp_send_out(tsk, opkb, NULL);
}
