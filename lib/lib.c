#include <stdio.h>
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
