CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../../../../include

OBJS=fdc.o


all: $(OBJS)


clean:
	rm -f $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

