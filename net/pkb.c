#include <stdio.h>
#include <ctype.h>

#include "netif.h"
#include "ether.h"
#include "lib.h"

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
	pkb = xmalloc(sizeof(*pkb) + size);
	pkb->pk_len = size;
	pkb->pk_pro = 0xffff;
	list_init(&pkb->pk_list);
	return pkb;
}

struct pkbuf *alloc_netdev_pkb(struct netdev *nd)
{
	return alloc_pkb(nd->net_mtu + ETH_HRD_SZ);
}

#ifdef DEBUG_PKB
void _free_pkb(struct pkbuf *pkb)
{
	dbg("%p", pkb);
#else
void free_pkb(struct pkbuf *pkb)
{
#endif
	free(pkb);
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

