CC = gcc
LD = ld
CFLAGS = -I../include

OBJS	=	lib.o

all:lib_obj.o
lib_obj.o:$(OBJS)
	$(LD) -r -o $@ $^

lib.o:lib.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
