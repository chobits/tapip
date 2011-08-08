CC = gcc
LD = ld
CFLAGS = -I../include

OBJS	=	test.o		\
		net_command.o	\
		ping_command.o	\
		shell.o

all:test_obj.o
test_obj.o:$(OBJS)
	$(LD) -r -o $@ $^
test.o:test.c
	$(CC) $(CFLAGS) -c -o $@ $<
net_command.o:net_command.c
	$(CC) $(CFLAGS) -c -o $@ $<
ping_command.o:ping_command.c
	$(CC) $(CFLAGS) -c -o $@ $<
shell.o:shell.c
	$(CC) $(CFLAGS) -c -o $@ $<

test_shell:shell.c
	$(CC) $(CFLAGS) -DTEST_SHELL $< -o $@

clean:
	rm -f *.o
