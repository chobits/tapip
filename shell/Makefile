CC = gcc
LD = ld
CFLAGS = -I../include -Wall

OBJS	=	main.o		\
		net_command.o	\
		ping_command.o	\
		shell.o

all:shell_obj.o
shell_obj.o:$(OBJS)
	$(LD) -r -o $@ $^
main.o:main.c
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
