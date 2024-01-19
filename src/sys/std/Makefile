CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include
AR=ar


OBJS=panic.o printk.o string.o snprintf.o kusleep.o random.o


all: libstd.a


clean:
	rm -f libstd.a $(OBJS)


libstd.a: $(OBJS)
	$(AR) r libstd.a $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

