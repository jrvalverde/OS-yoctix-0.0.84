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
 *  vfs/vfs_namei.c  --  get a file's inode number
 *
 *	Different file systems work differently. In msdosfs, getting the
 *	inode number of a file would mean something like "getting the
 *	first cluster number".  Since stat() returns the inode number,
 *	we simply call stat().
 *
 *	For other file systems with a more Unix-ish approach to inode
 *	numbers, we should be able to get the inode number without also
 *	stating the inode. But this is not yet implemented.   TODO
 *
 *
 *  History:
 *	5 Feb 2000	first version
 */


#include "../config.h"
#include <string.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/std.h>
#include <sys/stat.h>



int vfs_namei (struct proc *p, char *fname, inode_t *inode, struct mountinstance **mi)
  {
    /*
     *	Use stat() to get a stat struct for the fname.
     *
     *	TODO: maybe we should use vnode_lookup() instead, for speed.
     *
     *	Returns 0 on success, errno on failure.
     */

    struct stat ss;
    int res;


    if (!fname || !inode || !mi)
	return EINVAL;

    res = vfs_stat (p, fname, mi, &ss);
    if (res)
	return res;

    (*inode) = ss.st_ino;
    return 0;
  }

