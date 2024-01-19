CC=gcc
CFLAGS=-pipe -O2 -Wall -Werror -fno-builtin -nostdinc -I../include
AR=ar

LIB=libvfs.a
OBJS=vfs_init.o vfs_mount.o vfs_block.o vfs_namei.o vfs_stat.o vfs_vnode.o


all: $(LIB)


clean:
	rm -f $(LIB) $(OBJS)


$(LIB): $(OBJS)
	$(AR) r $(LIB) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

