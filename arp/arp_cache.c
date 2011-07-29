#include <stdio.h>

#include "ether.h"
#include "arp.h"
#include "lib.h"
#include "list.h"

static struct arpentry arp_cache[ARP_CACHE_SZ];

struct arpentry *arp_alloc(void)
{
	static int next = 0;
	int i;
	struct arpentry *ae;
	/* round-robin loop algorithm */
	for (i = 0; i < ARP_CACHE_SZ; i++) {
		if (arp_cache[next].ae_state == ARP_FREE)
			break;
		next = (next + 1) % ARP_CACHE_SZ;
	}
	/* not found */
	if (i >= ARP_CACHE_SZ) {
		dbg("arp cache is full");
		return NULL;
	}
	/* init */
	ae = &arp_cache[next];
	ae->ae_dev = NULL;
	ae->ae_retry = ARP_REQ_RETRY;
	ae->ae_ttl = ARP_WAITTIME;
	ae->ae_state = ARP_WAITING;
	ae->ae_pro = ETH_P_IP;		/* default protocol */
	list_init(&ae->ae_list);
	/* for next time allocation */
	next = (next + 1) % ARP_CACHE_SZ;
	return ae;
}

int arp_insert(struct netdev *nd, unsigned short pro,
		unsigned int ipaddr, unsigned char *hwaddr)
{
	struct arpentry *ae;
	ae = arp_alloc();
	if (!ae)
		return -1;
	ae->ae_dev = nd;
	ae->ae_pro = pro;
	ae->ae_ttl = ARP_TIMEOUT;
	ae->ae_ipaddr = ipaddr;
	ae->ae_state = ARP_RESOLVED;
	hwacpy(ae->ae_hwaddr, hwaddr);
	return 0;
}

struct arpentry *arp_lookup(unsigned short pro, unsigned int ipaddr)
{
	int i;
	for (i = 0; i < ARP_CACHE_SZ; i++) {
		if (arp_cache[i].ae_state == ARP_FREE)
			continue;
		if (arp_cache[i].ae_pro == pro &&
			arp_cache[i].ae_ipaddr == ipaddr)
			return &arp_cache[i];
	}
	return NULL;
}

void arp_timer(int delta)
{
	struct arpentry *ae;
	int i;

	for (ae = &arp_cache[0], i = 0; i < ARP_CACHE_SZ; i++, ae++) {
		if (ae->ae_state == ARP_FREE)
			continue;
		ae->ae_ttl -= delta;
		if (ae->ae_ttl <= 0) {
			if ((ae->ae_state == ARP_WAITING && --ae->ae_retry < 0)
				|| ae->ae_state == ARP_RESOLVED) {
				ae->ae_state = ARP_FREE;
			} else {
				/* retry arp request */
				ae->ae_ttl = ARP_WAITTIME;
				arp_request(ae);
			}
		}
	}
}

void arp_cache_init(void)
{
	int i;
	for (i = 0; i < ARP_CACHE_SZ; i++)
		arp_cache[i].ae_state = ARP_FREE;
	dbg("ARP INIT");
}

static char *__arpstate[] = {
	NULL,
	"Free",
	"Waiting",
	"Resolved"
};

static inline char *arpstate(struct arpentry *ae)
{
	return __arpstate[ae->ae_state];
}

char *ipnfmt(unsigned int ipaddr)
{
	static char ipbuf[16];
	snprintf(ipbuf, 16, IPFMT, ipfmt(ipaddr));
	return ipbuf;
}

void arp_cache_traverse(void)
{
	struct arpentry *ae;
	int i, first;
	first = 1;
	for (ae = &arp_cache[0], i = 0; i < ARP_CACHE_SZ; i++, ae++) {
		if (ae->ae_state == ARP_FREE)
			continue;
		if (first) {
			printf("State    Timeout(s)  HWaddress         Address\n");
			first = 0;
		}
		printf("%-9s%-12d" MACFMT " %s\n",
			arpstate(ae), ((ae->ae_ttl < 0) ? 0 : ae->ae_ttl),
			macfmt(ae->ae_hwaddr), ipnfmt(ae->ae_ipaddr));
	}
}
