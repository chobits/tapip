OBJS	= socket.o sock.o raw_sock.o inet.o

all:socket_obj.o
socket_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

socket.o:socket.c
	$(CC) $(CFLAGS) -c $< -o $@
sock.o:sock.c
	$(CC) $(CFLAGS) -c $< -o $@
raw_sock.o:raw_sock.c
	$(CC) $(CFLAGS) -c $< -o $@
inet_sock.o:inet_sock.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
