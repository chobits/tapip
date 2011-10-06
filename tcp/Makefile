OBJS	=	tcp_in.o tcp_out.o tcp_state.o tcp_sock.o tcp_text.o tcp_timer.o

all:tcp_obj.o
tcp_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

tcp_in.o:tcp_in.c
	$(CC) $(CFLAGS) -c $< -o $@
tcp_out.o:tcp_out.c
	$(CC) $(CFLAGS) -c $< -o $@
tcp_state.o:tcp_state.c
	$(CC) $(CFLAGS) -c $< -o $@
tcp_sock.o:tcp_sock.c
	$(CC) $(CFLAGS) -c $< -o $@
tcp_text.o:tcp_text.c
	$(CC) $(CFLAGS) -c $< -o $@
tcp_timer.o:tcp_timer.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
