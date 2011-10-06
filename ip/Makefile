OBJS	= ip_in.o ip_out.o ip_forward.o ip_frag.o icmp.o raw.o route.o
SUBDIR	= ip

all:ip_obj.o
ip_obj.o:$(OBJS)
	@echo " [LD] $(SUBDIR)/$@"
	$(Q)$(LD) -r -o $@ $^

%.o:%.c
	@echo " [CC] $(SUBDIR)/$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
