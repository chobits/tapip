OBJS	= tcp_in.o tcp_out.o tcp_state.o tcp_sock.o tcp_text.o tcp_timer.o
SUBDIR	= tcp

all:tcp_obj.o
tcp_obj.o:$(OBJS)
	@echo " [LD] $(SUBDIR)/$@"
	$(Q)$(LD) -r -o $@ $^

%.o:%.c
	@echo " [CC] $(SUBDIR)/$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
