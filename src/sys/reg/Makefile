CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include
AR=ar

LIB=libreg.a
OBJS=module.o interrupts.o ports.o dma.o device.o emul.o


all: $(LIB)


clean:
	rm -f $(LIB) $(OBJS)


$(LIB): $(OBJS)
	$(AR) r $(LIB) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

