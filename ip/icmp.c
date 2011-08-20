/*
 * Reference:
 *  RFC 792
 *  RFC 1812
 *  Linux 2.6.11.12
 *  Xinu
 */
#include "netif.h"
#include "ip.h"
#include "icmp.h"
#include "route.h"
#include "lib.h"

static void icmp_drop_reply(struct icmp_desc *, struct pkbuf *);
static void icmp_dest_unreach(struct icmp_desc *, struct pkbuf *);
static void icmp_echo_request(struct icmp_desc *, struct pkbuf *);
static void icmp_echo_reply(struct icmp_desc *, struct pkbuf *);
static void icmp_redirect(struct icmp_desc *, struct pkbuf *);

static struct icmp_desc icmp_table[ICMP_T_MAXNUM + 1] = {
	[ICMP_T_ECHORLY] = {
		.error = 0,
		.handler = icmp_echo_reply,
	},
	[ICMP_T_DUMMY_1] = ICMP_DESC_DUMMY_ENTRY,
	[ICMP_T_DUMMY_2] = ICMP_DESC_DUMMY_ENTRY,
	[ICMP_T_DESTUNREACH] = {
		.error = 1,
		.handler = icmp_dest_unreach,
	},
	[ICMP_T_SOURCEQUENCH] = {
		.error = 1,
		.info = "icmp source quench",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_REDIRECT] = {
		.error = 1,
		.info = "icmp redirect",
		.handler = icmp_redirect,
	},
	[ICMP_T_DUMMY_6] = ICMP_DESC_DUMMY_ENTRY,
	[ICMP_T_DUMMY_7] = ICMP_DESC_DUMMY_ENTRY,
	[ICMP_T_ECHOREQ] = {
		.error = 0,
		.handler = icmp_echo_request,
	},
	[ICMP_T_DUMMY_9] = ICMP_DESC_DUMMY_ENTRY,
	[ICMP_T_DUMMY_10] = ICMP_DESC_DUMMY_ENTRY,
	[ICMP_T_TIMEEXCEED] = {
		.error = 1,
		.info = "icmp time exceeded",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_PARAMPROBLEM] = {
		.error = 1,
		.info = "icmp parameter problem",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_TIMESTAMPREQ] = {
		.error = 0,
		.info = "icmp timestamp request",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_TIMESTAMPRLY] = {
		.error = 0,
		.info = "icmp timestamp rely",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_INFOREQ] = {
		.error = 0,
		.info = "icmp infomation request",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_INFORLY] = {
		.error = 0,
		.info = "icmp infomation reply",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_ADDRMASKREQ] = {
		.error = 0,
		.info = "icmp address mask request",
		.handler = icmp_drop_reply,
	},
	[ICMP_T_ADDRMASKRLY] = {
		.error = 0,
		.info = "icmp address mask reply",
		.handler = icmp_drop_reply,
	}
};

static void icmp_dest_unreach(struct icmp_desc *icmp_desc, struct pkbuf *pkb)
{
	/* FIXME: report error to upper application layer */
	icmpdbg("dest unreachable");
	free_pkb(pkb);
}

static char *redirectstr[4] = {
	[ICMP_REDIRECT_NET]	= "net redirect",
	[ICMP_REDIRECT_HOST]	= "host redirect",
	[ICMP_REDIRECT_TOSNET]	= "type of serice and net redirect",
	[ICMP_REDIRECT_TOSHOST]	= "type of service and host redirect"
};

static void icmp_redirect(struct icmp_desc *icmp_desc, struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct icmp *icmphdr = ip2icmp(iphdr);

	if (icmphdr->icmp_code > 4)
		icmpdbg("Redirect code %d is error", icmphdr->icmp_code);
	else
		icmpdbg("from " IPFMT " %s(new nexthop "IPFMT")",
					ipfmt(iphdr->ip_src),
					redirectstr[icmphdr->icmp_code],
					ipfmt(icmphdr->icmp_gw));
	free_pkb(pkb);
}

static void icmp_echo_reply(struct icmp_desc *icmp_desc, struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct icmp *icmphdr = ip2icmp(iphdr);
	icmpdbg("from "IPFMT" id %d seq %d ttl %d",
			ipfmt(iphdr->ip_src),
			ntohs(icmphdr->icmp_id),
			ntohs(icmphdr->icmp_seq),
			iphdr->ip_ttl);
	free_pkb(pkb);
}

static void icmp_echo_request(struct icmp_desc *icmp_desc, struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct icmp *icmphdr = ip2icmp(iphdr);
	icmpdbg("echo request data %d bytes",
			iphdr->ip_len - iphlen(iphdr) - ICMP_HRD_SZ);
	if (icmphdr->icmp_code) {
		icmpdbg("echo request packet corrupted");
		free_pkb(pkb);
		return;
	}
	icmphdr->icmp_type = ICMP_T_ECHORLY;
	/*
	 * adjacent the checksum:
	 * If  ~ >>> (cksum + x + 8) >>> == 0
	 * let ~ >>> (cksum` + x ) >>> == 0
	 * then cksum` = cksum + 8
	 */
	if (icmphdr->icmp_cksum >= 0xffff - ICMP_T_ECHOREQ)
		icmphdr->icmp_cksum += ICMP_T_ECHOREQ + 1;
	else
		icmphdr->icmp_cksum += ICMP_T_ECHOREQ;
	iphdr->ip_dst = iphdr->ip_src;	/* ip_src is set by ip_send_out() */
	ip_hton(iphdr);
	ip_send_out(pkb);
}

static void icmp_drop_reply(struct icmp_desc *icmp_desc, struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct icmp *icmphdr = ip2icmp(iphdr);
	icmpdbg("icmp type %d code %d (dropped)",
			icmphdr->icmp_type, icmphdr->icmp_code);
	if (icmp_desc->info)
		icmpdbg("%s", icmp_desc->info);
	free_pkb(pkb);
}

void icmp_in(struct pkbuf *pkb)
{
	struct ip *iphdr = pkb2ip(pkb);
	struct icmp *icmphdr = ip2icmp(iphdr);
	int icmplen, type;
	/* sanity check */
	icmplen = ipdlen(iphdr);
	icmpdbg("%d bytes", icmplen);
	if (icmplen < ICMP_HRD_SZ) {
		icmpdbg("icmp header is too small");
		goto drop_pkb;
	}

	if (icmp_chksum((unsigned short *)icmphdr, icmplen) != 0) {
		icmpdbg("icmp checksum is error");
		goto drop_pkb;
	}

	type = icmphdr->icmp_type;
	if (type > ICMP_T_MAXNUM) {
		icmpdbg("unknown icmp type %d code %d",
					type, icmphdr->icmp_code);
		goto drop_pkb;
	}
	/* handle icmp type */
	icmp_table[type].handler(&icmp_table[type], pkb);
	return;
drop_pkb:
	free_pkb(pkb);
}

/* NOTE: icmp dont drop @ipkb */
void icmp_send(unsigned char type, unsigned char code,
		unsigned int data, struct pkbuf *pkb_in)
{
	struct pkbuf *pkb;
	struct ip *iphdr = pkb2ip(pkb_in);
	struct icmp *icmphdr;
	int paylen = ntohs(iphdr->ip_len);	/* icmp payload length */
	if (paylen < iphlen(iphdr) + 8)
		return;
	/*
	 * RFC 1812 Section 4.3.2.7 for sanity check
	 * An ICMP error message MUST NOT be sent as the result of receiving:
	 * 1. A packet sent as a Link Layer broadcast or multicast
	 * 2. A packet destined to an IP broadcast or IP multicast address
	 *[3] A packet whose source address has a network prefix of zero or is an
	 *      invalid source address (as defined in Section [5.3.7])
	 * 4. Any fragment of a datagram other then the first fragment (i.e., a
	 *      packet for which the fragment offset in the IP header is nonzero).
	 * 5. An ICMP error message
	 */
	if (pkb_in->pk_type != PKT_LOCALHOST)
		return;
	if (MULTICAST(iphdr->ip_dst) || BROADCAST(iphdr->ip_dst))
		return;
	if (iphdr->ip_fragoff & htons(IP_FRAG_OFF))
		return;

	if (icmp_type_error(type) && iphdr->ip_pro == IP_P_ICMP) {
		icmphdr = ip2icmp(iphdr);
		if (icmphdr->icmp_type > ICMP_T_MAXNUM || icmp_error(icmphdr))
			return;
	}
	/* build icmp packet and send */
	/* ip packet size must be smaller than 576 bytes */
	if (IP_HRD_SZ + ICMP_HRD_SZ + paylen > 576)
		paylen = 576 - IP_HRD_SZ - ICMP_HRD_SZ;
	pkb = alloc_pkb(ETH_HRD_SZ + IP_HRD_SZ + ICMP_HRD_SZ + paylen);
	icmphdr = (struct icmp *)(pkb2ip(pkb)->ip_data);
	icmphdr->icmp_type = type;
	icmphdr->icmp_code = code;
	icmphdr->icmp_cksum = 0;
	icmphdr->icmp_undata = data;
	memcpy(icmphdr->icmp_data, (unsigned char *)iphdr, paylen);
	icmphdr->icmp_cksum =
		icmp_chksum((unsigned short *)icmphdr, ICMP_HRD_SZ + paylen);
	icmpdbg("to "IPFMT"(payload %d) [type %d code %d]\n",
		ipfmt(iphdr->ip_src), paylen, type, code);
	ip_send_info(pkb, 0, IP_HRD_SZ + ICMP_HRD_SZ + paylen,
						0, IP_P_ICMP, iphdr->ip_src);
}

