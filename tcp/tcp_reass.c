#include "tcp.h"
#include "cbuf.h"
#include "sock.h"
#include "netif.h"

struct tcp_reass_head {
	struct list_head list;	/* list of reassamble tcp segment */
	void *data;		/* memory address of tcp segment text */
	unsigned int seq;	/* sequence number of tcp segment text */
	unsigned int len;	/* data length of tcp segment text */
};

void tcp_free_reass_head(struct tcp_sock *tsk)
{
	struct tcp_reass_head *trh;
	while (!list_empty(&tsk->rcv_reass)) {
		trh = list_first_entry(&tsk->rcv_reass, struct tcp_reass_head, list);
		list_del(&trh->list);
		free_pkb(containof(trh, struct pkbuf, pk_data));
	}
}

void tcp_segment_reass(struct tcp_sock *tsk, struct tcp_segment *seg, struct pkbuf *pkb)
{
	/* reuse ether header as tcp_reass_head magically */
	struct tcp_reass_head *trh, *ctrh, *prev, *next;
	int rlen, len;

	/* TODO: how much text data is cached in reass list */

	list_for_each_entry(trh, &tsk->rcv_reass, list) {
		if (seg->seq < trh->seq) {
			prev = list_last_entry(&trh->list, struct tcp_reass_head, list);
			ADJACENT_SEGMENT_HEAD(prev->seq + prev->len);
			break;
		}
	}

	list_for_each_entry_safe_continue(trh, next, &tsk->rcv_reass, list) {
		if (seg->seq + seg->dlen < trh->seq + trh->len) {
			if (seg->seq + seg->dlen > trh->seq) {
				seg->dlen = trh->seq - seg->seq;
				if (seg->dlen == 0)
					goto out;
				assert(seg->dlen > 0);
			}
			break;
		}
		/* delete duplicate segment from reass list */
		list_del(&trh->list);
		free_pkb(containof(trh, struct pkbuf, pk_data));
	}

	/* insert segment into prev of trh */
	ctrh = (struct tcp_reass_head *)pkb->pk_data;
	list_init(&ctrh->list);
	ctrh->data = seg->text;
	ctrh->seq = seg->seq;
	ctrh->len = seg->dlen;
	list_add_tail(&ctrh->list, &trh->list);
	get_pkb(pkb);

	/* Can it write reass segment to cbuf */
	len = rlen = 0;
	list_for_each_entry_safe(trh, next, &tsk->rcv_reass, list) {
		if (trh->seq > tsk->rcv_nxt)
			break;
		assert(trh->seq == tsk->rcv_nxt);
		rlen = tcp_write_buf(tsk, trh->data, trh->len);
		if (rlen <= 0)
			break;
		len += rlen;
		list_del(&trh->list);
		free_pkb(containof(trh, struct pkbuf, pk_data));
	}

	if (len > 0 && seg->tcphdr->psh)
		tsk->flags |= TCP_F_PUSH;

out:
	return;
}
