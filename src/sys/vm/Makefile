CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include
AR=ar

LIB=libvm.a
OBJS=vm_init.o vm_region.o vm_object.o vm_fault.o vm_fork.o vm_prot.o


all: $(LIB)


clean:
	rm -f $(LIB) $(OBJS)


$(LIB): $(OBJS)
	$(AR) r $(LIB) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

