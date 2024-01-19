CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include
AR=ar

LIB=libkern.a
OBJS=init_main.o lock.o malloc.o \
	proc.o timer.o filedesc.o terminal.o \
	signal.o syscall.o socket.o kdb.o \
	sys_execve.o sys_proc.o sys_fd.o sys_file.o sys_socket.o \
	sys_time.o sys_sig.o sys_fork.o sys_mmap.o sys_misc.o \

all: $(LIB)


clean:
	rm -f $(LIB) $(OBJS)


$(LIB): $(OBJS)
	$(AR) r $(LIB) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

