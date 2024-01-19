
# BIN=misc
# OBJS=misc.o
# INCLUDES=
# MAN=arch.1 pwd.1 sleep.1 sync.1
# MANCHAPTER=1

# .include "../bin_make.inc"


misc: misc.c ps.c hdump.c cat.c mkdir.c
	gcc misc.c -o misc -static -O2 -Wall -s

clean:
	rm -f misc *core

