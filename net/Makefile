CC = gcc
LD = ld
CFLAGS = -I../include

OBJS	=	net.o		\
		netdev.o	\
		tap.o

all:net_obj.o
net_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

net.o:net.c
	$(CC) $(CFLAGS) -c $< -o $@
netdev.o:netdev.c
	$(CC) $(CFLAGS) -c $< -o $@
tap.o:tap.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
