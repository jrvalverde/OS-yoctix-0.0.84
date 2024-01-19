CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include -I../../include
AR=ar


#  machdep_main should NOT be included here!
#  It's included when making the entire kernel image!

OBJS=	machdep_main_startup.o \
	timer.o console.o interrupts.o kdb.o proc.o pmap.o \
	signal.o

all: libmd.a machdep_main.o


clean:
	rm -f libmd.a machdep_main.o $(OBJS)


libmd.a: $(OBJS)
	$(AR) r libmd.a $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.S
	$(CC) $(CFLAGS) -c $<


