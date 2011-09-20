OBJS	=	arp.o		\
		arp_cache.o

all:arp_obj.o
arp_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

arp.o:arp.c
	$(CC) $(CFLAGS) -c $< -o $@
arp_cache.o:arp_cache.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
