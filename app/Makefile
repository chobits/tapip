OBJS	=	ping.o		\
		snc.o

all:app_obj.o
app_obj.o:$(OBJS)
	$(LD) -r -o $@ $^
ping.o:ping.c
	$(CC) $(CFLAGS) -c -o $@ $<
snc.o:snc.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
