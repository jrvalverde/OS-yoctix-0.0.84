CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include
AR=ar

LOCAL_CC=gcc

LIB=libmodules.a
OBJS=modules_init.o


all:
	$(LOCAL_CC) create_minit.c -o create_minit

	rm -f modules_init.c
	./create_minit > modules_init.c
	$(CC) $(CFLAGS) -c modules_init.c

	rm -f $(LIB)
	$(AR) r $(LIB) $(OBJS)

	$(LOCAL_CC) create_modules.c -o create_modules
	./create_modules compile


clean:
	$(LOCAL_CC) create_modules.c -o create_modules
	./create_modules clean

	rm -f $(LIB) $(OBJS) *.core *.o create_minit create_modules modules_init.c


