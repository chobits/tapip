#include "netif.h"
#include "ether.h"
#include "ip.h"

#include "lib.h"
#include "list.h"

static LIST_HEAD(frag_head);	/* head of datagrams */

static inline int full_frag(struct fragment *frag)
{
	return frag->frag_rsize == frag->frag_size;
}

struct fragment *new_frag(struct ip *iphdr)
{
	struct fragment *frag;
	frag = malloc(sizeof(*frag));
	if (!frag) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	frag->frag_ttl = 600;
	frag->frag_id = iphdr->ip_id;
	frag->frag_src = iphdr->ip_src;
	frag->frag_dst = iphdr->ip_dst;
	frag->frag_size = 0;
	frag->frag_rsize = 0;
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
	int hlen, dlen;

	pkb = alloc_pkb(frag->frag_size + ETH_HRD_SZ);
	p = pkb->pk_data;
	/* copy ether header and ip header */
	fragpkb = list_first_entry(&frag->frag_pkb, struct pkbuf, pk_list);
	fraghdr = pkb2ip(fragpkb);
	hlen = iphlen(fraghdr);
	memcpy(p, fragpkb->pk_data, ETH_HRD_SZ + hlen);

	/* adjacent ip header */
	pkb2ip(pkb)->ip_fragoff = 0;
	pkb2ip(pkb)->ip_len = frag->frag_size;

	p += ETH_HRD_SZ + hlen;
	list_for_each_entry(fragpkb, &frag->frag_pkb, pk_list) {
		fraghdr = pkb2ip(fragpkb);
		memcpy(p, (char *)fraghdr + hlen, fraghdr->ip_len - hlen);
		p += fraghdr->ip_len - hlen;
	}
	ipdbg("resassembly success(%d/%d bytes)", hlen, frag->frag_size);
	delete_frag(frag);
	return pkb;
}

int insert_frag(struct pkbuf *pkb, struct fragment *frag)
{
	struct pkbuf *fragpkb;
	struct ip *iphdr, *fraghdr;
	struct list_head *pos;
	int off, hlen;

	iphdr = pkb2ip(pkb);
	off = ipoff(iphdr);
	hlen = iphlen(iphdr);

	/* Is last fragment? */
	if ((iphdr->ip_fragoff & IP_FRAG_MF) == 0) {
		if (frag->frag_size) {
			ipdbg("reduplicate last ip fragment");
			goto frag_drop;
		}
		frag->frag_size = off + iphdr->ip_len;
		frag->frag_rsize += iphdr->ip_len;
		list_add_tail(&pkb->pk_list, &frag->frag_pkb);
		return 0;
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
	if (fraghdr && hlen != iphlen(fraghdr)) {
		ipdbg("error ip fragment");
		goto frag_drop;
	}

	/* error: fraghdr off > iphdr off */
	if (fraghdr && ipoff(fraghdr) + fraghdr->ip_len - hlen > off) {
		ipdbg("error ip fragment");
		goto frag_drop;
	}

	list_add(&pkb->pk_list, pos);
	frag->frag_rsize += iphdr->ip_len - hlen;
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
	if (full_frag(frag))
		pkb = reass_frag(frag);
	else
		pkb = NULL;

	return pkb;
}

struct pkbuf *ip_frag(struct ip *orig, int hlen, int dlen, int off, unsigned short mf_bit)
{
	struct pkbuf *fragpkb;
	struct ip *fraghdr;
	fragpkb = alloc_pkb(ETH_HRD_SZ + hlen + dlen);
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
	int dlen, hlen, fraglen, off;
	iphdr = pkb2ip(pkb);
	hlen = iphlen(iphdr);
	dlen = ntohs(iphdr->ip_len) - hlen;
	off = 0;
	while (dlen > dev->net_mtu - hlen) {
		ipdbg(" [f] ip frag: off %d hlen %d dlen %d", off, hlen, dev->net_mtu - hlen);
		fragpkb = ip_frag(iphdr, hlen, dev->net_mtu - hlen, off, IP_FRAG_MF);
		ip_send_dev(dev, fragpkb, dst);

		dlen -= dev->net_mtu - hlen;
		off += dev->net_mtu - hlen;
	}

	if (dlen) {
		ipdbg(" [f] ip frag: off %d hlen %d dlen %d", off, hlen, dlen);
		fragpkb = ip_frag(iphdr, hlen, dlen, off, iphdr->ip_fragoff & IP_FRAG_MF);
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
