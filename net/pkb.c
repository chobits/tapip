#include <stdio.h>
#include <ctype.h>

#include "netif.h"
#include "ether.h"
#include "lib.h"

#define MAX_PKBS 200
int free_pkbs = 0;
int alloc_pkbs = 0;

#define pkb_safe() \
do {\
	if ((alloc_pkbs - free_pkbs) > MAX_PKBS) {\
		dbg("oops: too many pkbuf");\
		exit(EXIT_FAILURE);\
	}\
} while (0)

/* referred from linux-2.6: handing packet l2 padding */
void pkb_trim(struct pkbuf *pkb, int len)
{
	pkb->pk_len = len;
	if (realloc(pkb, sizeof(*pkb) + len) == NULL)
		perrx("realloc");
}

struct pkbuf *alloc_pkb(int size)
{
	struct pkbuf *pkb;
	pkb = xzalloc(sizeof(*pkb) + size);
	pkb->pk_len = size;
	pkb->pk_pro = 0xffff;
	pkb->pk_type = 0;
	pkb->pk_refcnt = 1;
	pkb->pk_indev = NULL;
	pkb->pk_rtdst = NULL;
	list_init(&pkb->pk_list);
	alloc_pkbs++;
	pkb_safe();
	return pkb;
}

struct pkbuf *alloc_netdev_pkb(struct netdev *nd)
{
	return alloc_pkb(nd->net_mtu + ETH_HRD_SZ);
}

struct pkbuf *copy_pkb(struct pkbuf *pkb)
{
	struct pkbuf *cpkb;
	cpkb = xmalloc(pkb->pk_len);
	memcpy(cpkb, pkb, pkb->pk_len);
	cpkb->pk_refcnt = 1;
	list_init(&cpkb->pk_list);
	alloc_pkbs++;
	pkb_safe();
	return cpkb;
}

#ifdef DEBUG_PKB
void _free_pkb(struct pkbuf *pkb)
{
	dbg("%p %d", pkb, pkb->pk_refcnt);
#else
void free_pkb(struct pkbuf *pkb)
{
#endif
	if (--pkb->pk_refcnt <= 0) {
		free_pkbs++;
		free(pkb);
	}
}

void get_pkb(struct pkbuf *pkb)
{
	pkb->pk_refcnt++;
}

void pkbdbg(struct pkbuf *pkb)
{
	int i;
	ferr("packet size: %d bytes\n", pkb->pk_len);
	ferr("packet buffer(ascii):\n");
	for (i = 0; i < pkb->pk_len; i++) {
		if ((i % 16) == 0)
			ferr("%08x: ", i);
		if (isprint(pkb->pk_data[i]))
			ferr("%c", pkb->pk_data[i]);
		else
			ferr(".");
		if ((i % 16) == 15)
			ferr("\n");
	}
	if ((i % 16) != 0)
		ferr("\n");
	ferr("packet buffer(raw):\n");
	for (i = 0; i < pkb->pk_len; i++) {
		if ((i % 16) == 0)
			ferr("%08x: ", i);
		if ((i % 2) == 0)
			ferr(" ");
		ferr("%02x", pkb->pk_data[i]);
		if ((i % 16) == 15)
			ferr("\n");
	}
	if ((i % 16) != 0)
		ferr("\n");
}

