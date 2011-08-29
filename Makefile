MAKEFLAGS += --no-print-directory

CC = gcc
CFLAGS = -lpthread -Wall

NET_STACK_OBJS =	shell/shell_obj.o		\
			net/net_obj.o		\
			arp/arp_obj.o		\
			ip/ip_obj.o		\
			socket/socket_obj.o	\
			udp/udp_obj.o		\
			app/app_obj.o		\
			lib/lib_obj.o

all:net_stack
net_stack:$(NET_STACK_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

shell/shell_obj.o:shell/main.c shell/shell.c\
		shell/net_command.c shell/ping_command.c
	@(cd shell/; make)
net/net_obj.o:net/net.c net/netdev.c net/tap.c net/pkb.c net/veth.c net/loop.c
	@(cd net/; make)
arp/arp_obj.o:arp/arp.c arp/arp_cache.c
	@(cd arp/; make)
ip/ip_obj.o:ip/ip.c ip/route.c ip/ip_frag.c ip/icmp.c ip/raw.c
	@(cd ip/; make)
udp/udp_obj.o:udp/udp.c udp/udp_sock.c
	@(cd udp/; make)
lib/lib_obj.o:lib/lib.c lib/checksum.c lib/wait.c
	@(cd lib/; make)
socket/socket_obj.o:socket/socket.c socket/sock.c socket/raw_sock.c socket/inet.c
	@(cd socket/; make)
app/app_obj.o:app/ping.c app/udp_test.c
	@(cd app/; make)

tag:
	ctags -R *

clean:
	@(cd net/; make clean)
	@(cd shell/; make clean)
	@(cd arp/; make clean)
	@(cd ip/; make clean)
	@(cd udp/; make clean)
	@(cd tcp/; make clean)
	@(cd lib/; make clean)
	@(cd socket/; make clean)
	@(cd app/; make clean)
	rm -f net_stack

lines:
	@echo "code lines:"
	@wc -l `find . -name \*.[ch]` | sort -n

