#include "tcp.h"
#include "ip.h"
#include "ether.h"
#include "netif.h"
#include "route.h"
#include "sock.h"
#include "cbuf.h"

void tcp_free_buf(struct tcp_sock *tsk)
{
	if (tsk->rcv_buf) {
		free_cbuf(tsk->rcv_buf);
		tsk->rcv_buf = NULL;
	}
}

/*
 * Situation is here:
 *  1. PUSH and segment text
 *  2. PUSH without segment text
 *  3. segment text without PUSH
 */
void tcp_recv_text(struct tcp_sock *tsk, struct tcp_segment *seg)
{
	struct cbuf *cbuf = tsk->rcv_buf;
	int rlen;
	if (seg->tcphdr->psh)
		tsk->flags |= TCP_F_PUSH;
	if (!tsk->rcv_wnd)
		goto out;
	/* first text */
	if (!cbuf) {
		cbuf = alloc_cbuf(tsk->rcv_wnd);
		tsk->rcv_buf = cbuf;
	}

	rlen = write_cbuf(cbuf, (char *)seg->text, seg->dlen);
	tsk->rcv_wnd -= rlen;	/* assert rlen >= 0 */
	tsk->rcv_nxt += rlen;
out:
	tsk->sk.ops->recv_notify(&tsk->sk);
	tcp_send_ack(tsk, seg);
}

static void tcp_init_text(struct tcp_sock *tsk, struct pkbuf *pkb,
						void *buf, int size)
{
	struct tcp *tcphdr = pkb2tcp(pkb);
	tcphdr->src = tsk->sk.sk_sport;
	tcphdr->dst = tsk->sk.sk_dport;
	tcphdr->doff = TCP_HRD_DOFF;
	tcphdr->seq = _htonl(tsk->snd_nxt);
	tcphdr->ackn = _htonl(tsk->rcv_nxt);
	tcphdr->ack = 1;
	tcphdr->window = _htons(tsk->rcv_wnd);
	memcpy(tcphdr->data, buf, size);
	tsk->snd_nxt += size;
	tsk->snd_wnd -= size;
	tcpsdbg("send TEXT(%u:%d) [WIN %d] to "IPFMT":%d",
			_ntohl(tcphdr->seq), size, _ntohs(tcphdr->window),
			ipfmt(tsk->sk.sk_daddr), _ntohs(tcphdr->dst));
}

int tcp_send_text(struct tcp_sock *tsk, void *buf, int len)
{
	struct pkbuf *pkb;
	int slen = 0;
	int segsize = tsk->sk.sk_dst->rt_dev->net_mtu - IP_HRD_SZ - TCP_HRD_SZ;
	len = min(len, (int)tsk->snd_wnd);
	while (slen < len) {
		segsize = min(segsize, len - slen);
		pkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + TCP_HRD_SZ + segsize);
		tcp_init_text(tsk, pkb, buf + slen, segsize);
		slen += segsize;
		if (slen >= len)
			pkb2tcp(pkb)->psh = 1;
		tcp_send_out(tsk, pkb, NULL);
	}
	/* zero window: request window update */
	if (!slen) {
		tcp_send_ack(tsk, NULL);
		slen = -1;
	}
	return slen;
}

