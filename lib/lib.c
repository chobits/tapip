#include "lib.h"
#include "ip.h"

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

void *xzalloc(int size)
{
	void *p = calloc(1, size);
	if (!p)
		perrx("calloc");
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

int str2ip(char *str, unsigned int *ip)
{
	unsigned int a, b, c, d;
	if (sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
		return -1;
	if ((a < 0 || a > 255) ||
		(b < 0 || b > 255) ||
		(c < 0 || c > 255) ||
		(d < 0 || d > 255))
		return -1;
	*ip = a | (b << 8) | (c << 16) | (d << 24);
	return 0;
}

int parse_ip_port(char *str, unsigned int *addr, unsigned short *nport)
{
	char *port;
	if ((port = strchr(str, ':')) != NULL) {
		*nport = _htons(atoi(&port[1]));
		*port = '\0';
	}
	if (str2ip(str, addr) < 0)
		return -1;
	if (port)
		*port = ':';
	return 0;
}

