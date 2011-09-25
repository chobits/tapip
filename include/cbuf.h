#ifndef __CBUF_H
#define __CBUF_H

/*
 * circul buffer:
 *   .<------------size-------------->.
 *   |                                |
 *   |<-x->|<-CBUFUSED->|<-----y----->|
 *   |     |            |<-CBUFRIGHT->|
 *   +-----++++++++++++++-------------+
 *         |            |
 *         CBUFTAIL     CBUFHEAD
 *   (CBUFFREE = x+y)
 */
struct cbuf {
	int head;	/* write point */
	int tail;	/* read point */
	int size;
	char buf[0];
};

#define _CBUFHEAD(cbuf) ((cbuf)->head % (cbuf)->size)
#define _CBUFTAIL(cbuf) ((cbuf)->tail % (cbuf)->size)
#define CBUFUSED(cbuf) ((cbuf)->head - (cbuf)->tail)
#define CBUFFREE(cbuf) ((cbuf)->size - CBUFUSED(cbuf))
#define CBUFHEAD(cbuf) &(cbuf)->buf[_CBUFHEAD(cbuf)]
#define CBUFTAIL(cbuf) &(cbuf)->buf[_CBUFTAIL(cbuf)]
/* head right space for writing */
#define CBUFHEADRIGHT(cbuf)\
	((CBUFHEAD(cbuf) >= CBUFTAIL(cbuf)) ?\
		((cbuf->size - _CBUFHEAD(cbuf))) :\
			(_CBUFTAIL(cbuf) - _CBUFHEAD(cbuf)))
/* tail righ space for reading */
#define CBUFTAILRIGHT(cbuf)\
	((CBUFHEAD(cbuf) > CBUFTAIL(cbuf)) ?\
		(CBUFUSED(cbuf)) :\
			((cbuf)->size - _CBUFTAIL(cbuf)))

extern int read_cbuf(struct cbuf *cbuf, char *buf, int size);
extern int write_cbuf(struct cbuf *cbuf, char *buf, int size);
extern struct cbuf *alloc_cbuf(int size);
extern void free_cbuf(struct cbuf *cbuf);

extern int alloc_cbufs;
extern int free_cbufs;

#endif	/* cbuf.h */
