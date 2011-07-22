#include <stdio.h>

#include "ether.h"
#include "arp.h"
#include "lib.h"

static struct arpentry arp_cache[ARP_CACHE_SZ];

int arp_insert(unsigned short pro, unsigned int ipaddr, unsigned char *hwaddr)
{
	static int next;
	int i;
	/* round-robin loop algorithm */
	for (i = 0; i < ARP_CACHE_SZ; i++) {
		if (arp_cache[next].ae_state == ARP_FREE)
			goto found;
		next = (next + 1) % ARP_CACHE_SZ;
	}
	dbg("arp cache is full");
	return -1;
found:
	arp_cache[i].ae_state = ARP_RESOLVED;
	arp_cache[i].ae_pro = pro;
	arp_cache[i].ae_ipaddr = ipaddr;
	hwacpy(arp_cache[i].ae_hwaddr, hwaddr);
	/* for next time inserting */
	next = (next + 1) % ARP_CACHE_SZ;
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
		if (ae->ae_ttl <= 0)
			ae->ae_state = ARP_FREE;
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
