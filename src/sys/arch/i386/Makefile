CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include -I../../include
AR=ar


#  machdep_main should NOT be included here!
#  It's included when making the entire kernel image!

OBJS=	machdep_main_startup.o \
	gdt.o idt.o pic.o \
	exceptions.o exceptions_asm.o \
	syscall_asm.o \
	timer.o timer_asm.o \
	signal.o signal_asm.o \
	interrupts_asm.o \
	machdep_res_init.o proc.o pmap.o task.o \
	console.o bioscmos.o \
	vm.o kdb.o

all: libmd.a machdep_main.o


clean:
	rm -f libmd.a machdep_main.o $(OBJS)


libmd.a: $(OBJS)
	$(AR) r libmd.a $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.S
	$(CC) $(CFLAGS) -c $<


