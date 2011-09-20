OBJS	=	net.o		\
		netdev.o	\
		veth.o		\
		loop.o		\
		pkb.o		\
		tap.o

all:net_obj.o
net_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

net.o:net.c
	$(CC) $(CFLAGS) -c $< -o $@
netdev.o:netdev.c
	$(CC) $(CFLAGS) -c $< -o $@
pkb.o:pkb.c
	$(CC) $(CFLAGS) -c $< -o $@
tap.o:tap.c
	$(CC) $(CFLAGS) -c $< -o $@
veth.o:veth.c
	$(CC) $(CFLAGS) -c $< -o $@
loop.o:loop.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
