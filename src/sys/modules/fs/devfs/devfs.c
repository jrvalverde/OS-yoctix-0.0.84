/*
 *  Copyright (C) 2000,2001 by Anders Gavare.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

/*
 *  modules/fs/devfs/devfs.c  --  /dev filesystem driver
 *
 *	The /dev filesystem driver is a way of accessing the registered devices
 *	in the system (in the first_device chain) through standard vfs
 *	operations.
 *
 *
 *  History:
 *	22 Mar 2000	test
 *	24 Nov 2000	adding get_direntries()
 *	5 Jan 2000	stat returns {a,m,c}time
 */



#include "../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/module.h>
#include <sys/interrupts.h>
#include <sys/device.h>


extern struct device *first_device;

struct module *devfs_m;
struct filesystem *devfs_fs;

extern struct timespec system_time;
struct timespec devfs_ctime;



int devfs_read_superblock (struct mountinstance *mi, struct proc *p)
  {
    mi->superblock->blocksize = 1024;		/*  Doesn't matter  */
    mi->superblock->nr_of_blocks = 1;		/*  ---   ''   ---  */
    mi->superblock->root_inode = 1;
    mi->superblock->fs_superblock = NULL;

    return 0;
  }



int devfs_namestat (struct mountinstance *mi, inode_t dirinode, char *name, struct stat *ss, struct proc *p)
  {
    /*
     *	Scan the directory 'dirinode' for a file named 'name' and if found
     *	we fill the stat struct 'ss' with its data (creation time, uid,gid etc).
     *
     *	If 'dirinode' == 1, then we read the root directory, otherwise
     *	we read the directory beginning at cluster 'dirinode'.
     *
     *	Returns 0 on success, errno on failure.
     */

    struct device *d=NULL, *found = NULL;

    if (dirinode != mi->superblock->root_inode)
	return ENOENT;

    memset (ss, 0, sizeof(struct stat));

    if (!strcmp(name, ".") || !strcmp(name, ".."))
      {
	ss->st_ino = mi->superblock->root_inode;
	ss->st_mode = S_IFDIR + 0755;
	ss->st_nlink = 1;
	ss->st_uid = 0;
	ss->st_gid = 0;
	ss->st_rdev = mi->device;
	ss->st_blksize = mi->superblock->blocksize;
	ss->st_flags = 0;
	ss->st_gen = 1;
	return 0;
      }

    /*  Find 'name' in the device chain:  */
    d = first_device;
    while (d)
      {
	if (!strcmp(d->name, name))
	  {
	    found = d;
	    d = NULL;
	  }
	if (d)
	  d = d->next;
      }

    d = found;
    if (!d)
	return ENOENT;

    ss->st_dev = (dev_t) mi;		/*  Hmm...  */
    ss->st_ino = (inode_t) ((int)d);	/*  Hmm again...  */
    ss->st_mode = d->vfs_mode;
    if (d->type == DEVICETYPE_CHAR)
	ss->st_mode |= S_IFCHR;
    if (d->type == DEVICETYPE_BLOCK)
	ss->st_mode |= S_IFBLK;
    ss->st_nlink = 1;
    ss->st_uid = d->vfs_uid;
    ss->st_gid = d->vfs_gid;
    ss->st_rdev = d;
    ss->st_atime = d->vfs_atime.tv_sec;
    ss->st_atimensec = d->vfs_atime.tv_nsec;
    ss->st_mtime = d->vfs_mtime.tv_sec;
    ss->st_mtimensec = d->vfs_mtime.tv_nsec;
    ss->st_ctime = d->vfs_ctime.tv_sec;
    ss->st_ctimensec = d->vfs_ctime.tv_nsec;
    ss->st_size = 0;
    ss->st_blocks = 0;
    ss->st_blksize = mi->superblock->blocksize;
    ss->st_flags = 0;
    ss->st_gen = 1;
    return 0;
  }



int devfs_istat (struct mountinstance *mi, inode_t inode, struct stat *ss, struct proc *p)
  {
    if (inode != mi->superblock->root_inode)
      {
	printk ("devfs_istat(): can only istat the root directory");
	return EINVAL;
      }

    /*  /dev is always owned by root.wheel, rwxr-xr-x  */

    ss->st_dev = (dev_t) mi;	/*  Hm...  */
    ss->st_ino = inode;
    ss->st_mode = 0755 + S_IFDIR;
    ss->st_nlink = 1;
    ss->st_uid = 0;
    ss->st_gid = 0;
    ss->st_rdev = NULL;
    ss->st_mtime = devfs_ctime.tv_sec;
    ss->st_mtimensec = devfs_ctime.tv_nsec;
    ss->st_atime = ss->st_mtime;
    ss->st_atimensec = ss->st_mtimensec;
    ss->st_ctime = ss->st_mtime;
    ss->st_ctimensec = ss->st_mtimensec;
    ss->st_size = 0;
    ss->st_blocks = 0;
    ss->st_blksize = mi->superblock->blocksize;
    ss->st_flags = 0;
    ss->st_gen = 1;
    return 0;
  }



int devfs_get_direntries (struct proc *p, struct vnode *v,
	off_t curofs, byte *buf, size_t buflen, off_t *offtres)
  {
    /*
     *	This is a semi-ugly hack until I have time to write something more
     *	optimized and correct:
     *
     *	TODO: race conditions, and rewrite for optimized space usage...
     */

    struct device *d;
    off_t fakeoffset = 0;
    u_int32_t *p32;
    u_int16_t *p16;
    int len;

    *offtres = 0;

    if (buflen < 64)
	return 0;

    d = first_device;
    while (d)
      {
	if (curofs == fakeoffset)
	  {
	    /*  fill in one entry:  */
	    memset (buf, 0, 64);

	    p32 = (u_int32_t *)((byte *)(buf));
	    *p32 = (long) d;

	    p16 = (u_int16_t *)((byte *)(buf + 4));
	    *p16 = 64;

	    len = strlen(d->name);
	    if (len > 50)
		len=50;
	    p16 = (u_int16_t *)((byte *)(buf + 6));
	    *p16 = len;

	    strlcpy (buf+8, d->name, 50);

	    buf += 64;

	    curofs += 64;
	    *offtres += 64;
	    if (buflen - *offtres < 64)
		return 0;
	  }

	/*  next:  */
	fakeoffset += 64;
	d = d->next;
      }
    
    return 0;
  }



void devfs_init (int arg)
  {
    int oldints;
    devfs_m = module_register ("vfs", 0, "devfs", "Device (/dev) filesystem");

    if (!devfs_m)
	return;

    devfs_fs = vfs_register ("devfs", "devfs_init");
    if (!devfs_fs)
      {
	module_unregister (devfs_m);
	return;
      }

    /*  Init devfs_fs:  */
    oldints = interrupts (DISABLE);
    devfs_ctime = system_time;
    interrupts (oldints);

    devfs_fs->read_superblock = devfs_read_superblock;
    devfs_fs->namestat = devfs_namestat;
    devfs_fs->istat = devfs_istat;
    devfs_fs->get_direntries = devfs_get_direntries;

    unlock (&devfs_fs->lock);
  }

