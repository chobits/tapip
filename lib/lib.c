#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "lib.h"

void perrx(char *str)
{
	if (errno)
		perror(str);
	else
		ferr("ERROR:%s\n", str);
	exit(EXIT_FAILURE);
}

void *xmalloc(int size)
{
	void *p = malloc(size);
	if (!p)
		perrx("malloc");
	return p;
}

/* format and print mlen-max-size data (spaces will fill the buf) */
static char *_space = "                                              ";
void printfs(int mlen, const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	int slen;
	va_start(ap, fmt);
	slen = vsprintf(buf, fmt, ap);
	va_end(ap);
	printf("%.*s", mlen, buf);
	if (mlen > slen)
		printf("%.*s", mlen - slen, _space);
}
