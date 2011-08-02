#include "netif.h"
#include "ether.h"
#include "arp.h"
#include "lib.h"

#define BRD_HWADDR "\xff\xff\xff\xff\xff\xff"

void arp_queue_send(struct arpentry *ae)
{
	struct pkbuf *pkb;
	while (!list_empty(&ae->ae_list)) {
		pkb = list_first_entry(&ae->ae_list, struct pkbuf, pk_list);
		list_del(ae->ae_list.next);
		arpdbg("send pending packet");
		netdev_tx(ae->ae_dev, pkb, pkb->pk_len - ETH_HRD_SZ,
				pkb->pk_pro, ae->ae_hwaddr);
	}
}

void arp_request(struct arpentry *ae)
{
	struct pkbuf *pkb;
	struct ether *ehdr;
	struct arp *ahdr;

	pkb = alloc_pkb(ETH_HRD_SZ + ARP_HRD_SZ);
	ehdr = (struct ether *)pkb->pk_data;
	ahdr = (struct arp *)ehdr->eth_data;
	/* normal arp information */
	ahdr->arp_hrd = htons(ARP_HRD_ETHER);
	ahdr->arp_pro = htons(ETH_P_IP);
	ahdr->arp_hrdlen = ETH_ALEN;
	ahdr->arp_prolen = IP_ALEN;
	ahdr->arp_op = htons(ARP_OP_REQUEST);
	/* address */
	ahdr->arp_sip = ae->ae_dev->_net_ipaddr;
	hwacpy(ahdr->arp_sha, ae->ae_dev->_net_hwaddr);
	ahdr->arp_tip = ae->ae_ipaddr;
	hwacpy(ahdr->arp_tha, BRD_HWADDR);
	netdev_tx(ae->ae_dev, pkb, pkb->pk_len - ETH_HRD_SZ, ETH_P_ARP, BRD_HWADDR);
}

/*
 * The algorithm is strict based on RFC 826
 * ARP Packet Reception
 */
void arp_in(struct netdev *nd, struct pkbuf *pkb)
{
	struct ether *ehdr = (struct ether *)pkb->pk_data;
	struct arp *ahdr = (struct arp *)ehdr->eth_data;
	struct arpentry *ae;

	if (pkb->pk_len < ETH_HRD_SZ + ARP_HRD_SZ) {
		arpdbg("arp packet is too small");
		goto err_free_pkb;
	}

	/* safe check for arp cheating */
	if (hwacmp(ahdr->arp_sha, ehdr->eth_src) != 0) {
		arpdbg("error sender hardware address");
		goto err_free_pkb;
	}
	arp_hton(ahdr);

#if defined(ARP_ETHERNET) && defined(ARP_IP)
	/* ethernet/ip arp only */
	if (ahdr->arp_hrd != ARP_HRD_ETHER || ahdr->arp_pro != ETH_P_IP ||
		ahdr->arp_hrdlen != ETH_ALEN || ahdr->arp_prolen != IP_ALEN) {
		arpdbg("unsupported L2/L3 protocol");
		goto err_free_pkb;
	}
#endif
	if (ahdr->arp_op != ARP_OP_REQUEST && ahdr->arp_op != ARP_OP_REPLY) {
		arpdbg("unknown arp operation");
		goto err_free_pkb;
	}

	/* real arp process */
	arpdbg(IPFMT " -> " IPFMT, ipfmt(ahdr->arp_sip), ipfmt(ahdr->arp_tip));
	if (ae = arp_lookup(ahdr->arp_pro, ahdr->arp_sip)) {
		/* update old arp entry in cache */
		hwacpy(ae->ae_hwaddr, ahdr->arp_sha);
		/* send waiting packet (maybe we receive arp reply) */
		if (ae->ae_state = ARP_WAITING)
			arp_queue_send(ae);
		ae->ae_state = ARP_RESOLVED;
		ae->ae_ttl = ARP_TIMEOUT;
	}

	if (ahdr->arp_tip != nd->_net_ipaddr)
		goto err_free_pkb;

	if (ae == NULL)
		arp_insert(nd, ahdr->arp_pro, ahdr->arp_sip, ahdr->arp_sha);

	if (ahdr->arp_op == ARP_OP_REQUEST) {
		arpdbg("replying arp request");
		/* arp field */
		ahdr->arp_op = ARP_OP_REPLY;
		hwacpy(ahdr->arp_tha, ahdr->arp_sha);
		ahdr->arp_tip = ahdr->arp_sip;
		hwacpy(ahdr->arp_sha, nd->_net_hwaddr);
		ahdr->arp_sip = nd->_net_ipaddr;
		arp_ntoh(ahdr);
		/* ether field */
		netdev_tx(nd, pkb, ARP_HRD_SZ, ETH_P_ARP, ehdr->eth_src);
	}

	/* arp reply has been handled! */
	return;
err_free_pkb:
	free_pkb(pkb);
}
