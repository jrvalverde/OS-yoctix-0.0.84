CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../../../include

OBJS=openbsd_aout.o


all: $(OBJS)


clean:
	rm -f $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

openbsd_aout.o: openbsd_aout.c openbsd_aout_syscalls.c
	$(CC) $(CFLAGS) -c $<
