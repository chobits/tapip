/*
 * circul buffer implementation:
 *  Although the code is clean and simple,
 *  the culculation is complex (CBUFHEADRIGHT).
 */
#include "lib.h"
#include "cbuf.h"

int alloc_cbufs = 0;
int free_cbufs = 0;

void free_cbuf(struct cbuf *cbuf)
{
	free(cbuf);
	free_cbufs++;
}

struct cbuf *alloc_cbuf(int size)
{
	struct cbuf *cbuf;
	cbuf = xzalloc(sizeof(*cbuf) + size);
	cbuf->head = cbuf->tail = 0;
	cbuf->size = size;
	alloc_cbufs++;
	return cbuf;
}

int write_cbuf(struct cbuf *cbuf, char *buf, int size)
{
	int len , wlen, onelen;
	if (!cbuf)
		return 0;
	len = wlen = min(CBUFFREE(cbuf), size);
	while (len > 0) {
		onelen = min(CBUFHEADRIGHT(cbuf), len);
#ifdef CBUF_TEST
		dbg("[%d]%.*s", onelen, onelen, buf);
#endif
		memcpy(CBUFHEAD(cbuf), buf, onelen);
		buf += onelen;
		len -= onelen;
		cbuf->head += onelen;
	}
	return wlen;
}

int read_cbuf(struct cbuf *cbuf, char *buf, int size)
{
	int len , rlen , onelen;
	if (!cbuf)
		return 0;
	len = rlen = min(CBUFUSED(cbuf), size);
	while (len > 0) {
		onelen = min(CBUFTAILRIGHT(cbuf), len);
#ifdef CBUF_TEST
		dbg("[%d]%.*s", onelen, onelen, CBUFTAIL(cbuf));
#endif
		memcpy(buf, CBUFTAIL(cbuf), onelen);
		buf += onelen;
		len -= onelen;
		cbuf->tail += onelen;
	}
	/* update I/O point */
	if (cbuf->tail >= cbuf->size) {
		cbuf->head -= cbuf->size;
		cbuf->tail -= cbuf->size;
	}
	return rlen;
}

#ifdef CBUF_TEST

#define die(str)\
	do {\
		printf("%d: %s\n", __LINE__, str);\
		exit(0);\
	} while(0);

int main(void)
{
	struct cbuf *cbuf;
	char buf[1024];
	char buf2[1024];
	int len;
	int n, i;
	/* test for 1 byte cbuf */

	cbuf = alloc_cbuf(1);
	n = 10000;
	while (n > 0) {
		if (write_cbuf(cbuf, "h", 1) != 1)
			die("write_cbuf");
		if (read_cbuf(cbuf, buf, 1) != 1)
			die("read_cbuf");
		if (buf[0] != 'h')
			die("Not `h`");
		if (read_cbuf(cbuf, buf, 1) != 0)
			die("read_cbuf");
		n--;
	}
	free_cbuf(cbuf);
	dbg("1 byte cbuf is ok!");

	/* test 16 bytes */
	cbuf = alloc_cbuf(16);
	if (write_cbuf(cbuf, "0123456789abcdefXYZ", 19) != 16)
		die("write_cbuf");
	if (read_cbuf(cbuf, buf, 16) != 16)
		die("read_cbuf");
	if (strncmp(buf, "0123456789abcdefXYZ", 16))
		die("data error");
	if (cbuf->head != 0 || cbuf->tail != 0)
		die("cbuf point corrupts");
	/* buffer wrap */
	if (write_cbuf(cbuf, "XXXXXXXX", 8) != 8)
		die("write_cbuf");
	if (read_cbuf(cbuf, buf, 4) != 4)
		die("read_cbuf");
	if (write_cbuf(cbuf, "0123456789abcdefXYZ", 12) != 12)
		die("write_cbuf");
	if (read_cbuf(cbuf, buf, 16) != 16)
		die("read_cbuf");
	if (strncmp(buf, "XXXX0123456789abcdefXYZ", 16))
		die("data error");
	free_cbuf(cbuf);
	dbg("16 byte cbuf is ok!");
	/* test for 9999 byte cbuf */
	cbuf = alloc_cbuf(9999);
	i = 10;
	n = 9999;
	while (i > 0) {
		buf2[i] = 'k';
		if (write_cbuf(cbuf, buf2, i * 7) != i * 7)
			die("write_cbuf");
		if (read_cbuf(cbuf, buf, i * 7) != i * 7)
			die("read_cbuf");
		if (strncmp(buf, buf2, i * 7))
			die("data error");
		n--;
		if (n <= 0) {
			n = 99999*10;
			i--;
		}
	}
	free_cbuf(cbuf);
	dbg("9999 byte cbuf is ok!");

	return 0;
}

#endif
