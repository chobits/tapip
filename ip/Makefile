CC = gcc
LD = ld
CFLAGS = -I../include

OBJS	=	ip.o		\
		ip_frag.o

all:ip_obj.o
ip_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

ip.o:ip.c
	$(CC) $(CFLAGS) -c $< -o $@
ip_frag.o:ip_frag.c
	$(CC) $(CFLAGS) -c $< -o $@
route.o:route.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
