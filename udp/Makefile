OBJS	=	udp.o udp_sock.o

all:udp_obj.o
udp_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

udp.o:udp.c
	$(CC) $(CFLAGS) -c $< -o $@
udp_sock.o:udp_sock.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
