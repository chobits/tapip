#include "lib.h"
#include "netif.h"
#include "tcp.h"
#include "ip.h"

static char *tcp_state_string(struct tcp *tcphdr)
{
	static char ss[32];
	char *ssp = ss;
	if (tcphdr->fin) {
		strcpy(ssp, "FIN|");
		ssp += 4;
	}
	if (tcphdr->syn) {
		strcpy(ssp, "SYN|");
		ssp += 4;
	}
	if (tcphdr->rst) {
		strcpy(ssp, "RST|");
		ssp += 4;
	}
	if (tcphdr->psh) {
		strcpy(ssp, "PSH|");
		ssp += 4;
	}
	if (tcphdr->ack) {
		strcpy(ssp, "ACK|");
		ssp += 4;
	}
	if (tcphdr->urg) {
		strcpy(ssp, "URG|");
		ssp += 4;
	}
	if (ssp == ss)
		ssp[0] = '\0';
	else
		ssp[-1] = '\0';
	return ss;
}

static void tcp_segment_init(struct tcp_segment *seg, struct ip *iphdr, struct tcp *tcphdr)
{
	seg->seq = ntohl(tcphdr->seq);
	seg->dlen = ipdlen(iphdr) - tcphlen(tcphdr);
	seg->len = seg->dlen + tcphdr->syn + tcphdr->fin;
	seg->text = tcptext(tcphdr);
	/* if len is 0, fix lastseq to seq */
	seg->lastseq = seg->len ? (seg->seq + seg->len - 1) : seg->seq;
	seg->ack = tcphdr->ack ? ntohl(tcphdr->ackn) : 0;
	seg->wnd = ntohs(tcphdr->window);
	seg->up = ntohs(tcphdr->urgptr);
	/* precedence value is not used */
	seg->prc = 0;
	seg->iphdr = iphdr;
	seg->tcphdr = tcphdr;

	tcpdbg("from "IPFMT":%d" " to " IPFMT ":%d"
		"\tseq:%u(%d:%d) ack:%u %s",
			ipfmt(iphdr->ip_src), ntohs(tcphdr->src),
			ipfmt(iphdr->ip_dst), ntohs(tcphdr->dst),
			ntohl(tcphdr->seq), seg->dlen, seg->len,
			ntohl(tcphdr->ackn), tcp_state_string(tcphdr));
}

static void tcp_recv(struct pkbuf *pkb, struct ip *iphdr, struct tcp *tcphdr)
{
	struct tcp_segment seg;
	struct sock *sk;

	tcp_segment_init(&seg, iphdr, tcphdr);
	/* Should we use net device to match a connection? */
	sk = tcp_lookup_sock(iphdr->ip_src, iphdr->ip_dst,
				tcphdr->src, tcphdr->dst);
	tcp_process(pkb, &seg, sk);
	if (sk)
		free_sock(sk);
}

void tcp_in(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct tcp *tcphdr = ip2tcp(iphdr);
	int tcplen = ipdlen(iphdr);

	tcpdbg("%d bytes, real %d bytes",tcplen, tcphlen(tcphdr));
	if (tcplen < TCP_HRD_SZ || tcplen < tcphlen(tcphdr)) {
		tcpdbg("tcp length it too small");
		goto drop_pkb;
	}
	if (tcp_chksum(iphdr->ip_src, iphdr->ip_dst,
		tcplen, (unsigned short *)tcphdr) != 0) {
		tcpdbg("tcp packet checksum corrupts");
		goto drop_pkb;
	}
	return tcp_recv(pkb, iphdr, tcphdr);
drop_pkb:
	free_pkb(pkb);
}
