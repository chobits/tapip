#include "netif.h"
#include "ether.h"
#include "ip.h"

#include "lib.h"
#include "list.h"

static LIST_HEAD(frag_head);	/* head of datagrams */

static inline int full_frag(struct fragment *frag)
{
	return (((frag->frag_flags & FRAG_FL_IN) == FRAG_FL_IN) &&
		(frag->frag_rsize == frag->frag_size));
}

static inline int complete_frag(struct fragment *frag)
{
	return frag->frag_flags & FRAG_COMPLETE;
}

struct fragment *new_frag(struct ip *iphdr)
{
	struct fragment *frag;
	frag = xmalloc(sizeof(*frag));
	frag->frag_ttl = 600;
	frag->frag_id = iphdr->ip_id;
	frag->frag_src = iphdr->ip_src;
	frag->frag_dst = iphdr->ip_dst;
	frag->frag_pro = iphdr->ip_pro;
	frag->frag_hlen = 0;
	frag->frag_size = 0;
	frag->frag_rsize = 0;
	frag->frag_flags = 0;
	list_add(&frag->frag_list, &frag_head);
	list_init(&frag->frag_pkb);
	return frag;
}

void delete_frag(struct fragment *frag)
{
	struct pkbuf *pkb;
	list_del(&frag->frag_list);
	while (!list_empty(&frag->frag_pkb)) {
		pkb = list_first_entry(&frag->frag_pkb, struct pkbuf, pk_list);
		list_del(&pkb->pk_list);
		free_pkb(pkb);
	}
	free(frag);
	/* FIXME: icmp DESTINATION */
}

struct pkbuf *reass_frag(struct fragment *frag)
{
	struct pkbuf *pkb, *fragpkb;
	struct ip *fraghdr;
	unsigned char *p;
	int hlen, len;

	hlen = frag->frag_hlen;
	len = frag->frag_hlen + frag->frag_size;
	if (len > 65535) {
		ipdbg("reassembled packet oversize(%d/%d)", hlen, len);
		goto out;
	}

	/* copy ether header and ip header */
	fragpkb = list_first_entry(&frag->frag_pkb, struct pkbuf, pk_list);
	fraghdr = pkb2ip(fragpkb);

	pkb = alloc_pkb(ETH_HRD_SZ + len);
	pkb->pk_pro = ETH_P_IP;
	p = pkb->pk_data;
	memcpy(p, fragpkb->pk_data, ETH_HRD_SZ + hlen);

	/* adjacent ip header */
	pkb2ip(pkb)->ip_fragoff = 0;
	pkb2ip(pkb)->ip_len = len;

	p += ETH_HRD_SZ + hlen;
	list_for_each_entry(fragpkb, &frag->frag_pkb, pk_list) {
		fraghdr = pkb2ip(fragpkb);
		memcpy(p, (char *)fraghdr + hlen, fraghdr->ip_len - hlen);
		p += fraghdr->ip_len - hlen;
	}
	ipdbg("resassembly success(%d/%d bytes)", hlen, len);
out:
	delete_frag(frag);
	return pkb;
}

/* FIXME: add support to overlap fragment(see linux) */
int insert_frag(struct pkbuf *pkb, struct fragment *frag)
{
	struct pkbuf *fragpkb;
	struct ip *iphdr, *fraghdr;
	struct list_head *pos;
	int off, hlen;

	if (complete_frag(frag)) {
		ipdbg("extra fragment for complete reassembled packet");
		goto frag_drop;
	}

	iphdr = pkb2ip(pkb);
	off = ipoff(iphdr);
	hlen = iphlen(iphdr);

	/* Is last fragment? */
	if ((iphdr->ip_fragoff & IP_FRAG_MF) == 0) {
		if (frag->frag_flags & FRAG_LAST_IN) {
			ipdbg("reduplicate last ip fragment");
			goto frag_drop;
		}
		frag->frag_flags |= FRAG_LAST_IN;
		frag->frag_size = off + iphdr->ip_len - hlen;
		pos = frag->frag_pkb.prev;
		goto frag_out;
	}

	/* normal fragment */
	pos = &frag->frag_pkb;
	/* find the first fraghdr, in which case fraghdr off < iphdr off */
	list_for_each_entry_reverse(fragpkb, &frag->frag_pkb, pk_list) {
		fraghdr = pkb2ip(fragpkb);
		if (off == ipoff(fraghdr)) {
			ipdbg("reduplicate ip fragment");
			goto frag_drop;
		}
		if (off > ipoff(fraghdr)) {
			pos = &fragpkb->pk_list;
			goto frag_found;
		}
	}
	/* not found: pkb is the current min-offset fragment */
	fraghdr = NULL;

frag_found:
	/* Should strict head check? */
	if (frag->frag_hlen && frag->frag_hlen != hlen) {
		ipdbg("error ip fragment");
		goto frag_drop;
	} else {
		frag->frag_hlen = hlen;
	}

	/* error: fraghdr off > iphdr off */
	if (fraghdr && ipoff(fraghdr) + fraghdr->ip_len - hlen > off) {
		ipdbg("error ip fragment");
		goto frag_drop;
	}

	/* Is first one? (And reduplicate ip fragment cannot appear here!) */
	if (off == 0)
		frag->frag_flags |= FRAG_FIRST_IN;

frag_out:
	list_add(&pkb->pk_list, pos);
	frag->frag_rsize += iphdr->ip_len - hlen;
	/* Is full? (If yes, set complete flag) */
	if (full_frag(frag))
		frag->frag_flags |= FRAG_COMPLETE;
	return 0;

frag_drop:
	free_pkb(pkb);
	return -1;
}

struct fragment *lookup_frag(struct ip *iphdr)
{
	struct fragment *frag;
	list_for_each_entry(frag, &frag_head, frag_list)
		if (frag->frag_id == iphdr->ip_id &&
			frag->frag_pro == iphdr->ip_pro &&
			frag->frag_src == iphdr->ip_src &&
			frag->frag_dst == iphdr->ip_dst)
			return frag;
	return NULL;
}

struct pkbuf *ip_reass(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct fragment *frag;

	ipdbg("ID:%d RS:%d DF:%d MF:%d OFF:%d bytes size:%d bytes",
			iphdr->ip_id,
			(iphdr->ip_fragoff & IP_FRAG_RS) ? 1 : 0,
			(iphdr->ip_fragoff & IP_FRAG_DF) ? 1 : 0,
			(iphdr->ip_fragoff & IP_FRAG_MF) ? 1 : 0,
			ipoff(iphdr),
			iphdr->ip_len);

	frag = lookup_frag(iphdr);
	if (frag == NULL)
		frag = new_frag(iphdr);
	if (insert_frag(pkb, frag) < 0)
		return NULL;
	if (complete_frag(frag))
		pkb = reass_frag(frag);
	else
		pkb = NULL;

	return pkb;
}

struct pkbuf *ip_frag(struct ip *orig, int hlen, int dlen, int off,
						unsigned short mf_bit)
{
	struct pkbuf *fragpkb;
	struct ip *fraghdr;
	fragpkb = alloc_pkb(ETH_HRD_SZ + hlen + dlen);
	fragpkb->pk_pro = ETH_P_IP;
	fraghdr = pkb2ip(fragpkb);
	/* copy head */
	memcpy(fraghdr, orig, hlen);
	/* copy data */
	memcpy((void *)fraghdr + hlen, (void *)orig + hlen + off, dlen);
	/* adjacent the head */
	fraghdr->ip_len = htons(hlen + dlen);
	mf_bit |= (off >> 3);
	fraghdr->ip_fragoff = htons(mf_bit);
	ip_setchksum(fraghdr);
	return fragpkb;
}

void ip_send_frag(struct netdev *dev, struct pkbuf *pkb, unsigned int dst)
{
	struct pkbuf *fragpkb;
	struct ip *fraghdr, *iphdr;
	int dlen, hlen, mlen, fraglen, off;

	iphdr = pkb2ip(pkb);
	hlen = iphlen(iphdr);
	dlen = ntohs(iphdr->ip_len) - hlen;
	mlen = (dev->net_mtu - hlen) & ~7;	/* max length */
	off = 0;

	while (dlen > mlen) {
		ipdbg(" [f] ip frag: off %d hlen %d dlen %d", off, hlen, mlen);
		fragpkb = ip_frag(iphdr, hlen, mlen, off, IP_FRAG_MF);
		ip_send_dev(dev, fragpkb, dst);

		dlen -= mlen;
		off += mlen;
	}

	if (dlen) {
		ipdbg(" [f] ip frag: off %d hlen %d dlen %d", off, hlen, dlen);
		fragpkb = ip_frag(iphdr, hlen, dlen, off,
					iphdr->ip_fragoff & IP_FRAG_MF);
		ip_send_dev(dev, fragpkb, dst);
	}
	free_pkb(pkb);
}

/* FIXME: ip_timer test */
void ip_timer(int delta)
{
	struct fragment *frag;
	/* FIXME: mutex-- for frag_head */
	list_for_each_entry(frag, &frag_head, frag_list) {
		/* condition race */
		if (full_frag(frag))
			continue;
		frag->frag_ttl -= delta;
		if (frag->frag_ttl <= 0)
			delete_frag(frag);
	}
	/* mutex++ for frag_head */
}
