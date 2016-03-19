#ifndef __COMPILE_H
#define __COMPILE_H

#undef _inline
#define _inline inline __attribute__((always_inline))

#define containof(ptr, type, member)\
	((type *)((char *)(ptr) - (size_t)&((type *)0)->member))

#endif	/* complie.h */
