CC = gcc
LD = ld
CFLAGS = -I../include -Wall

OBJS	=	ping.o		\
		udp_test.o

all:app_obj.o
app_obj.o:$(OBJS)
	$(LD) -r -o $@ $^
ping.o:ping.c
	$(CC) $(CFLAGS) -c -o $@ $<
udp_test.o:udp_test.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
