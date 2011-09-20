OBJS	=	ping.o		\
		udp_test.o	\
		tcp_test.o

all:app_obj.o
app_obj.o:$(OBJS)
	$(LD) -r -o $@ $^
ping.o:ping.c
	$(CC) $(CFLAGS) -c -o $@ $<
udp_test.o:udp_test.c
	$(CC) $(CFLAGS) -c -o $@ $<
tcp_test.o:tcp_test.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
