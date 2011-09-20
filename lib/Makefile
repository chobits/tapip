OBJS	=	lib.o checksum.o

all:lib_obj.o
lib_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

lib.o:lib.c
	$(CC) $(CFLAGS) -c $< -o $@
checksum.o:checksum.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
