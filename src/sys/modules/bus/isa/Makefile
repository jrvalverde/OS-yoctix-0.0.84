CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../../../include

OBJS=isa.o


all: $(OBJS)


clean:
	rm -f $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

