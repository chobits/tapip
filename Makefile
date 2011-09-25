#### User configure  ###############
CONFIG_DEBUG = n
CONFIG_DEBUG_WAIT = n
CONFIG_DEBUG_ARP_LOCK = n
CONFIG_DEBUG_ICMPEXCFRAGTIME = n
CONFIG_TOPLOGY = 2
#### End of User configure #########

MAKEFLAGS += --no-print-directory

LD = ld
CC = gcc
CFLAGS = -Wall -I../include
LFLAGS = -lpthread
export LD CC CFLAGS

ifeq ($(CONFIG_DEBUG), y)
	CFLAGS += -g
endif

ifeq ($(CONFIG_DEBUG_ICMPEXCFRAGTIME), y)
	CFLAGS += -DICMP_EXC_FRAGTIME_TEST
endif

ifeq ($(CONFIG_DEBUG_WAIT), y)
	CFLAGS += -DWAIT_DEBUG
endif

ifeq ($(CONFIG_DEBUG_ARP_LOCK), y)
	CFLAGS += -DDEBUG_ARPCACHE_LOCK
endif

ifeq ($(CONFIG_TOPLOGY), 1)
	CFLAGS += -DCONFIG_TOP1
else
	CFLAGS += -DCONFIG_TOP2
endif

NET_STACK_OBJS =	shell/shell_obj.o	\
			net/net_obj.o		\
			arp/arp_obj.o		\
			ip/ip_obj.o		\
			socket/socket_obj.o	\
			udp/udp_obj.o		\
			tcp/tcp_obj.o		\
			app/app_obj.o		\
			lib/lib_obj.o

all:tapip
tapip:$(NET_STACK_OBJS)
	$(CC) $(LFLAGS) $^ -o $@

shell/shell_obj.o:shell/main.c shell/shell.c\
		shell/net_command.c shell/ping_command.c
	@make -C shell/
net/net_obj.o:net/net.c net/netdev.c net/tap.c net/pkb.c net/veth.c net/loop.c
	@make -C net/
arp/arp_obj.o:arp/arp.c arp/arp_cache.c
	@make -C arp/
ip/ip_obj.o:ip/ip.c ip/route.c ip/ip_frag.c ip/icmp.c ip/raw.c
	@make -C ip/
udp/udp_obj.o:udp/udp.c udp/udp_sock.c
	@make -C udp/
tcp/tcp_obj.o:tcp/tcp.c tcp/tcp_sock.c tcp/tcp_out.c tcp/tcp_state.c\
		tcp/tcp_text.c
	@make -C tcp/
lib/lib_obj.o:lib/lib.c lib/checksum.c lib/cbuf.c
	@make -C lib/
socket/socket_obj.o:socket/socket.c socket/sock.c socket/raw_sock.c\
		socket/inet.c
	@make -C socket/
app/app_obj.o:app/ping.c app/udp_test.c app/tcp_test.c
	@make -C app/

test:cbuf
cbuf:lib/cbuf.c lib/lib.c
	$(CC) -DCBUF_TEST -Iinclude/ $^ -o $@

tag:
	ctags -R *

clean:
	@make -C net/ clean
	@make -C shell/ clean
	@make -C arp/ clean
	@make -C ip/ clean
	@make -C udp/ clean
	@make -C tcp/ clean
	@make -C lib/ clean
	@make -C socket/ clean
	@make -C app/ clean
	rm -f tapip cbuf

lines:
	@echo "code lines:"
	@wc -l `find . -name \*.[ch]` | sort -n

