#include "netif.h"
#include "ether.h"
#include "arp.h"
#include "lib.h"

/*
 * The algorithm is strict based on RFC 826
 * ARP Packet Reception
 */
int arp_in(struct netdev *nd, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	struct arp *ahdr = (struct arp *)ehdr->eth_data;
	struct arpentry *ae;
	int ret = PK_DROP;

	arpdbg(IPFMT " -> " IPFMT, ipfmt(ahdr->arp_sip), ipfmt(ahdr->arp_tip));

	if (pkb->pk_len < ETH_HRD_SZ + ARP_HRD_SZ)
		goto err_free_pkb;

	/* safe check for arp cheating */
	if (hwacmp(ahdr->arp_sha, ehdr->eth_src) != 0)
		goto err_free_pkb;
	arp_hton(ahdr);

#if defined(ARP_ETHERNET) && defined(ARP_IP)
	if (ahdr->arp_hrd != ARP_HRD_ETHER)
		goto err_free_pkb;

	/* ethernet arp only */
	if (ahdr->arp_hrdlen != ETH_ALEN || ahdr->arp_prolen != IP_ALEN)
		goto err_free_pkb;
#endif
	
	arpdbg("lookup");
	if (ae = arp_lookup(ahdr->arp_pro, ahdr->arp_sip)) {
		/* update old arp entry in cache */
		ae->ae_ttl = ARP_TIMEOUT;
		ae->ae_state = ARP_RESOLVED;
		hwacpy(ae->ae_hwaddr, ahdr->arp_sha);
	}

	arpdbg("network ip address: "IPFMT , ipfmt(nd->_net_ipaddr));
	if (ahdr->arp_tip != nd->_net_ipaddr)
		goto err_free_pkb;

	if (ae == NULL)
		arp_insert(ahdr->arp_pro, ahdr->arp_sip, ahdr->arp_sha);

	if (ahdr->arp_op == ARP_OP_REQUEST) {
		arpdbg("reply");
		/* arp field */
		ahdr->arp_op = ARP_OP_REPLY;
		hwacpy(ahdr->arp_tha, ahdr->arp_sha);
		ahdr->arp_tip = ahdr->arp_sip;
		hwacpy(ahdr->arp_sha, nd->_net_hwaddr);
		ahdr->arp_sip = nd->_net_ipaddr;
		arp_ntoh(ahdr);
		/* ether field */
		netdev_tx(nd, pkb, ARP_HRD_SZ, ETH_P_ARP, ehdr->eth_src);
		return PK_RECV;
	} else if (ahdr->arp_op == ARP_OP_REPLY) {
		arpdbg("recv reply");
		ret = PK_RECV;
	}

err_free_pkb:
	free_pkb(pkb);
	return ret;
}
