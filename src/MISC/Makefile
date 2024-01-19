#
#  Makefile for biosboot (i386)
#

LD=ld
LDFLAGS=-nostdlib -Ttext 0 -N -x -Bstatic


all: biosboot burnkernel


clean:
	rm -f biosboot crash testprog burnkernel *.o *.core

burnkernel: burnkernel.c
	gcc burnkernel.c -o burnkernel -s

biosboot: biosboot.S
	gcc -c biosboot.S
	ld $(LDFLAGS) biosboot.o -o biosboot

#testprog: testprog.c testprog_main.c
#	gcc testprog_main.c -c
#	gcc testprog.c -c
#	ld -nostdlib testprog.o -o testprog -s
#	ld -nostdlib -Bstatic -Ttext 0x40000 -z -x -o testprog testprog_main.o testprog.o -s

crash: crash.c
	gcc crash.c -o crash -static

