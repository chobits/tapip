MAKEFLAGS += --no-print-directory
vpath %.c net/:arp/:test/:lib/

CC = gcc
CFLAGS = -lpthread

NET_STACK_OBJS =	test/test_obj.o	\
			net/net_obj.o	\
			arp/arp_obj.o	\
			lib/lib_obj.o

all:net_stack
net_stack:$(NET_STACK_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test/test_obj.o:shell.c test.c net_command.c
	@(cd test/; make)
net/net_obj.o:net.c netdev.c tap.c
	@(cd net/; make)
arp/arp_obj.o:arp.c arp_cache.c
	@(cd arp/; make)
lib/lib_obj.o:lib.c
	@(cd lib/; make)

clean:
	@(cd net/; make clean)
	@(cd test/; make clean)
	@(cd arp/; make clean)
	@(cd lib/; make clean)
	rm -f net_stack
