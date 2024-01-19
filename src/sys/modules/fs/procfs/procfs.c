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
 *  modules/fs/procfs/procfs.c  --  /proc filesystem driver
 *
 *	The /proc filesystem driver lets userland programs get information
 *	about processes running on the system, without the need of parsing
 *	/dev/mem or such, through standard vfs operations.
 *
 *
 *  History:
 *	25 Nov 2000	test
 */



#include "../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/limits.h>
#include <sys/stat.h>
#include <sys/interrupts.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/module.h>
#include <sys/device.h>


struct module *procfs_m;
struct filesystem *procfs_fs;
extern struct proc *procqueue [];

extern volatile struct timespec system_time;
struct timespec procfs_ctime;


int procfs_read_superblock (struct mountinstance *mi, struct proc *p)
  {
    mi->superblock->blocksize = 1024;		/*  Doesn't matter  */
    mi->superblock->nr_of_blocks = 0;		/*  ---   ''   ---  */
    mi->superblock->root_inode = PID_MAX+1;
    mi->superblock->fs_superblock = NULL;

    return 0;
  }



int procfs_namestat (struct mountinstance *mi, inode_t dirinode, char *name,
		struct stat *ss, struct proc *p)
  {
    /*
     *	Scan the directory 'dirinode' for a file named 'name' and if found
     *	we fill the stat struct 'ss' with its data (creation time, uid,gid etc).
     *
     *	TODO:  this function needs some fixing...
     *
     *	Returns 0 on success, errno on failure.
     */

    int a, b, pos;
    struct proc *tmpp;
    int oldints;

    if (dirinode != mi->superblock->root_inode)
	return ENOENT;

    memset (ss, 0, sizeof(struct stat));

    if (!strcmp(name, ".") || !strcmp(name, ".."))
      {
	ss->st_ino = mi->superblock->root_inode;
	ss->st_mode = S_IFDIR + 0555;
	ss->st_nlink = 1;
	ss->st_uid = 0;
	ss->st_gid = 0;
	ss->st_rdev = mi->device;
	ss->st_blksize = mi->superblock->blocksize;
	ss->st_flags = 0;
	ss->st_gen = 1;
	return 0;
      }

    /*  Find process named 'name':  */
    /*  TODO:  fix this, use atoi or something  */
    b = 0;
    for (pos=0; pos<5; pos++)
      {
	a = name[pos];
	if (a!=0)
	  b = b*10 + (a - 48);
	else
	  break;
      }

    oldints = interrupts (DISABLE);
    tmpp = find_proc_by_pid (b);
    if (!tmpp)
      {
	interrupts (oldints);
	return ENOENT;
      }

    ss->st_dev = (dev_t) mi;
    ss->st_ino = (inode_t) tmpp->pid;
    ss->st_mode = 0511 | S_IFDIR;
    ss->st_nlink = 1;
    ss->st_uid = tmpp->cred.uid;
    ss->st_gid = tmpp->cred.gid;
    ss->st_rdev = NULL;
    ss->st_atime = 0;
    ss->st_atimensec = 0;
    ss->st_mtime = tmpp->creattime.tv_sec;
    ss->st_mtimensec = tmpp->creattime.tv_nsec;
    ss->st_ctime = ss->st_mtime;
    ss->st_ctimensec = ss->st_mtimensec;
    ss->st_size = 0;
    ss->st_blocks = 0;
    ss->st_blksize = mi->superblock->blocksize;
    ss->st_flags = 0;
    ss->st_gen = 1;

    interrupts (oldints);
    return 0;
  }



int procfs_istat (struct mountinstance *mi, inode_t inode, struct stat *ss,
	struct proc *p)
  {
    if (inode != mi->superblock->root_inode)
      {
	printk ("devfs_istat(): can only istat the root directory");
	return EINVAL;
      }

    /*  /proc is always owned by root.wheel, r-xr-xr-x  */

    ss->st_dev = (dev_t) mi;	/*  Hm...  */
    ss->st_ino = inode;
    ss->st_mode = 0555 + S_IFDIR;
    ss->st_nlink = 1;
    ss->st_uid = 0;
    ss->st_gid = 0;
    ss->st_rdev = NULL;
    ss->st_atime = 0;
    ss->st_atimensec = 0;
    ss->st_mtime = procfs_ctime.tv_sec;
    ss->st_mtimensec = procfs_ctime.tv_nsec;
    ss->st_ctime = ss->st_mtime;
    ss->st_ctimensec = ss->st_mtimensec;
    ss->st_size = 0;
    ss->st_blocks = 0;
    ss->st_blksize = mi->superblock->blocksize;
    ss->st_flags = 0;
    ss->st_gen = 1;
    return 0;
  }



int procfs_get_direntries (struct proc *p, struct vnode *v,
	off_t curofs, byte *buf, size_t buflen, off_t *offtres)
  {
    /*
     *	TODO:  this is very ugly
     */

    struct proc *tmpp;
    off_t fakeoffset = 0;
    u_int32_t *p32;
    u_int16_t *p16;
    int len;
    int i;
    int oldints;

    *offtres = 0;

    if (buflen < 32)
	return 0;

    oldints = interrupts (DISABLE);

    for (i=0; i<PROC_MAXQUEUES; i++)
      {
	tmpp = procqueue [i];
	while (tmpp)
	  {
	    if (curofs == fakeoffset)
	      {
		/*  fill in one entry:  */
		memset (buf, 0, 32);

		p32 = (u_int32_t *)((byte *)(buf));
		*p32 = (long) tmpp->pid;

		p16 = (u_int16_t *)((byte *)(buf + 4));
		*p16 = 32;

		snprintf (buf+8, 20, "%i", tmpp->pid);

		len = strlen(buf+8);
		if (len > 19)
			len=19;
		p16 = (u_int16_t *)((byte *)(buf + 6));
		*p16 = len;

		buf += 32;

		curofs += 32;
		*offtres += 32;
		if (buflen - *offtres < 32)
			return 0;
	      }

	    /*  next:  */
	    fakeoffset += 32;
	    tmpp = tmpp->next;
	    if (tmpp == procqueue[i])
		tmpp = NULL;
	  }
      }

    interrupts (oldints);    

    return 0;
  }



void procfs_init (int arg)
  {
    /*
     *	Initialize procfs:
     */

    int oldints;
    procfs_m = module_register ("vfs", 0, "procfs",
		"Process tree (/proc) filesystem");

    if (!procfs_m)
	return;

    procfs_fs = vfs_register ("procfs", "procfs_init");
    if (!procfs_fs)
      {
	module_unregister (procfs_m);
	return;
      }

    oldints = interrupts (DISABLE);
    procfs_ctime = system_time;
    interrupts (oldints);

    procfs_fs->read_superblock = procfs_read_superblock;
    procfs_fs->namestat = procfs_namestat;
    procfs_fs->istat = procfs_istat;
    procfs_fs->get_direntries = procfs_get_direntries;

    unlock (&procfs_fs->lock);
  }

