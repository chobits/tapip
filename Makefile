MAKEFLAGS += --no-print-directory
vpath %.c net/:arp/:test/:lib/:ip/

CC = gcc
CFLAGS = -lpthread

NET_STACK_OBJS =	test/test_obj.o	\
			net/net_obj.o	\
			ip/ip_obj.o	\
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
ip/ip_obj.o:ip.c route.c
	@(cd ip/; make)
lib/lib_obj.o:lib.c
	@(cd lib/; make)

clean:
	@(cd net/; make clean)
	@(cd test/; make clean)
	@(cd arp/; make clean)
	@(cd ip/; make clean)
	@(cd lib/; make clean)
	rm -f net_stack

lines:
	wc -l `find |grep "\.[ch]$$"`
