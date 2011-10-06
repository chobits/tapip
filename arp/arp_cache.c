#include "ether.h"
#include "arp.h"
#include "lib.h"
#include "list.h"
#include "compile.h"

#define arp_cache_head (&arp_cache[0])
#define arp_cache_end (&arp_cache[ARP_CACHE_SZ])
static struct arpentry arp_cache[ARP_CACHE_SZ];

#ifdef LOCK_SEM

#include <semaphore.h>
static sem_t arp_cache_sem;	/* arp cache lock */
static _inline void arp_cache_lock_init(void)
{
	if (sem_init(&arp_cache_sem, 0, 1) == -1)
		perrx("sem_init");
}

#ifdef DEBUG_ARPCACHE_LOCK
#define arp_cache_lock() do { dbg("lock"); sem_wait(&arp_cache_sem); } while(0)
#define arp_cache_unlock() do { dbg("unlock"); sem_post(&arp_cache_sem); } while(0)
#else	/* !DEBUG_ARPCACHE_LOCK */
static _inline void arp_cache_lock(void)
{
	sem_wait(&arp_cache_sem);
}

static _inline void arp_cache_unlock(void)
{
	sem_post(&arp_cache_sem);
}
#endif	/* end DEBUG_ARPCACHE_LOCK */

#else	/* !DEBUG_SEM */

/* It is evil to init pthread mutex dynamically X< */
#ifdef STATIC_MUTEX
pthread_mutex_t arp_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
pthread_mutex_t arp_cache_mutex;
#ifndef PTHREAD_MUTEX_NORMAL
#define	PTHREAD_MUTEX_NORMAL PTHREAD_MUTEX_TIMED_NP
#endif

#endif
static _inline void arp_cache_lock_init(void)
{
#ifndef STATIC_MUTEX
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) != 0)
		perrx("pthread_mutexattr_init");
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL) != 0)
		perrx("pthread_mutexattr_settype");
	if (pthread_mutex_init(&arp_cache_mutex, &attr) != 0)
		perrx("pthread_mutex_init");
#endif
}

#ifdef DEBUG_ARPCACHE_LOCK
#define arp_cache_lock() do { dbg("lock"); pthread_mutex_lock(&arp_cache_mutex); } while(0)
#define arp_cache_unlock() do { dbg("unlock"); pthread_mutex_unlock(&arp_cache_mutex); } while(0)
#else
static _inline void arp_cache_lock(void)
{
	pthread_mutex_lock(&arp_cache_mutex);
}

static _inline void arp_cache_unlock(void)
{
	pthread_mutex_unlock(&arp_cache_mutex);
}
#endif	/* end DEBUG_ARPCACHE_LOCK */
#endif	/* end LOCK_SEM */

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

void arp_queue_drop(struct arpentry *ae)
{
	struct pkbuf *pkb;
	while (!list_empty(&ae->ae_list)) {
		pkb = list_first_entry(&ae->ae_list, struct pkbuf, pk_list);
		list_del(ae->ae_list.next);
		arpdbg("drop pending packet");
		free_pkb(pkb);
	}
}

struct arpentry *arp_alloc(void)
{
	static int next = 0;
	int i;
	struct arpentry *ae;

	arp_cache_lock();
	/* round-robin loop algorithm */
	for (i = 0; i < ARP_CACHE_SZ; i++) {
		if (arp_cache[next].ae_state == ARP_FREE)
			break;
		next = (next + 1) % ARP_CACHE_SZ;
	}
	/* not found */
	if (i >= ARP_CACHE_SZ) {
		arpdbg("arp cache is full");
		arp_cache_unlock();
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
	arp_cache_unlock();

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
	struct arpentry *ae, *ret = NULL;
	arp_cache_lock();
	arpdbg("pro:%d "IPFMT, pro, ipfmt(ipaddr));
	for (ae = arp_cache_head; ae < arp_cache_end; ae++) {
		if (ae->ae_state == ARP_FREE)
			continue;
		if (ae->ae_pro == pro && ae->ae_ipaddr == ipaddr) {
			ret = ae;
			break;
		}
	}
	arp_cache_unlock();
	return ret;
}

struct arpentry *arp_lookup_resolv(unsigned short pro, unsigned int ipaddr)
{
	struct arpentry *ae;
	ae = arp_lookup(pro, ipaddr);
	if (ae && ae->ae_pro == ARP_RESOLVED)
		return ae;
	return NULL;
}

void arp_timer(int delta)
{
	struct arpentry *ae;

	arp_cache_lock();
	for (ae = arp_cache_head; ae < arp_cache_end; ae++) {
		if (ae->ae_state == ARP_FREE)
			continue;
		ae->ae_ttl -= delta;
		if (ae->ae_ttl <= 0) {
			if ((ae->ae_state == ARP_WAITING && --ae->ae_retry < 0)
				|| ae->ae_state == ARP_RESOLVED) {
				if (ae->ae_state == ARP_WAITING)
					arp_queue_drop(ae);
				ae->ae_state = ARP_FREE;
			} else {
				/* retry arp request */
				ae->ae_ttl = ARP_WAITTIME;
				arp_cache_unlock();
				arp_request(ae);
				arp_cache_lock();
			}
		}
	}
	arp_cache_unlock();
}

void arp_cache_init(void)
{
	int i;
	for (i = 0; i < ARP_CACHE_SZ; i++)
		arp_cache[i].ae_state = ARP_FREE;
	dbg("ARP CACHE INIT");
	arp_cache_lock_init();
	dbg("ARP CACHE SEMAPHORE INIT");
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
	int first;

	arp_cache_lock();
	first = 1;
	for (ae = arp_cache_head; ae < arp_cache_end; ae++) {
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
	arp_cache_unlock();
}
