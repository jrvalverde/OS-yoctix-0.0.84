/*
 *  Copyright (C) 2000 by Anders Gavare.  All rights reserved.
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
 *  modules/fs/ffs/ffs.c  --  Fast filesystem (ffs) driver
 *
 *	Filesystem driver to read BSD filesystems.
 *
 *  History:
 *	15 Aug 2000	test
 *	1 Sep 2000	_istat(), _namestat()
 *	5 Sep 2000	using cgstart() instead of cgbase() :-)
 */



#include "../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/modules/fs/ffs.h>
#include <sys/modules/fs/ffs_dinode.h>


extern struct device *first_device;

struct module *ffs_m;
struct filesystem *ffs_fs;



#include "super.c"
#include "dinode.c"
#include "read.c"
#include "stat.c"



int ffs_get_direntries (struct proc *p, struct vnode *v,
	off_t curofs, byte *buf, size_t buflen, off_t *offtres)
  {
    int erres;
    byte *bp;

    erres = v->read (v, (off_t) curofs, buf, (off_t) buflen, offtres);
    if (erres)
      return erres;

    /*  namelen is short, and type should be removed:  */
    bp = buf;
    while ((byte *)bp < (byte *)(buf+(int)*offtres))
      {
	/*  remove type, etc:  */
	bp[6] = bp[7];
	bp[7] = 0;
	bp += (bp[4] + bp[5]*256);
      }

    return erres;
  }



void ffs_init (int arg)
  {
    /*  TODO: handle arg correctly  */

    ffs_m = module_register ("vfs", 0, "ffs", "Fast filesystem (ffs) driver");

    if (!ffs_m)
	return;

    ffs_fs = vfs_register ("ffs", "ffs_init");
    if (!ffs_fs)
      {
	module_unregister (ffs_m);
	return;
      }

    /*  Init ffs_fs:  */
    ffs_fs->read_superblock = &ffs_read_superblock;
    ffs_fs->namestat = &ffs_namestat;
    ffs_fs->istat = &ffs_istat;
    ffs_fs->read = &ffs_read;
    ffs_fs->readlink = &ffs_readlink;
    ffs_fs->get_direntries = &ffs_get_direntries;

    unlock (&ffs_fs->lock);
  }

