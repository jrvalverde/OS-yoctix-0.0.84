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
 *  modules/fs/ffs/stat.c
 *
 *  Included from ffs.c
 */


int ffs_namestat (struct mountinstance *mi, inode_t dirinode, char *name, struct stat *ss, struct proc *p)
  {
    /*
     *	ffs_namestat ()
     *	---------------
     *
     *	Fill 'ss' with stat data from file 'name' in directory 'inode'.
     *	(This is slow. TODO: vfs_stat() should not call namestat for ffs filesystems.
     *	It is neccessary for msdosfs, but for ffs it is really stupid.)
     *
     *	Stupid but working algorithm:
     *
     *	  o)  Stat dirinode (the directory we're about to scan).
     *	  o)  Scan through the directory to find 'name'.
     *	  o)  If 'name' was found, stat its inode (and return data via ss).
     */

    struct ffs_superblock *fsb = (struct ffs_superblock *) mi->superblock->fs_superblock;
    off_t dirlength;
    off_t curpos;
    off_t blnr;
    int res;
    byte *buf;
    struct direct *dptr;

#if DEBUGLEVEL>=4
    printk ("ffs_namestat: name='%s'", name);
#endif

    if (!mi || dirinode<2 || !name || !ss)
      return EINVAL;

    /*  Using ss for "temporary" data, such as the dirinode's stat data,
	should be okay.  */
    res = ffs_istat (mi, dirinode, ss, p);
    if (res)
      return res;

    dirlength = ss->st_size;
    buf = (byte *) malloc (fsb->fs_bsize);

    /*  Find 'name' by scanning the directory:  */
    curpos = 0;
    blnr = 0;
    while (curpos < dirlength)
      {
	/*  Read one (large) block:  */
	res = ffs__readblock (mi, dirinode, blnr, buf, p);
	if (res)
	  {
	    free (buf);
	    return res;
	  }

	dptr = (struct direct *) buf;

/*  TODO: directories are subdivided into DIRBLKSIZ bytes each!!!  */

	while ((byte *)dptr < buf+(dirlength-curpos))
	  {
#if DEBUGLEVEL>=4
	    printk ("  dptr: ino=%i, len=%i, type=%i, namlen=%i, \"%s\"",
		dptr->d_ino, dptr->d_reclen, dptr->d_type,
		dptr->d_namlen, dptr->d_name);
#endif

	    if (!strncmp(name, dptr->d_name, dptr->d_namlen+1))
	      {
		ffs_istat (mi, dptr->d_ino, ss, p);
		free (buf);
		return 0;
	      }

	    dptr = (struct direct *) ((byte *)dptr + dptr->d_reclen);
	  }

	curpos += fsb->fs_bsize;
	blnr ++;
      }

    free (buf);
    return ENOENT;
  }



int ffs_istat (struct mountinstance *mi, inode_t inode, struct stat *ss, struct proc *p)
  {
    /*
     *	ffs_istat ()
     *	------------
     *
     *	Fill 'ss' with stat data from inode number 'inode'.
     */

    struct dinode *di;
    int res;

    if (!mi || inode<2 || !ss)
      return EINVAL;

    di = (struct dinode *) malloc (sizeof(struct dinode));
    if (!di)
      return ENOMEM;

    res = ffs__readdinode (mi, inode, di, p);
    if (res)
      {
	free (di);
	return res;
      }

    /*  Copy di data to ss:  */
    memset (ss, 0, sizeof(struct stat));
    ss->st_dev		= mi->device->vfs_dev;
    ss->st_ino		= inode;
    ss->st_mode		= di->di_mode;
    ss->st_nlink	= di->di_nlink;
    ss->st_uid		= di->di_uid;
    ss->st_gid		= di->di_gid;
    ss->st_rdev		= NULL;
    ss->st_atime	= di->di_atime;
    ss->st_atimensec	= di->di_atimensec;
    ss->st_mtime	= di->di_mtime;
    ss->st_mtimensec	= di->di_mtimensec;
    ss->st_ctime	= di->di_ctime;
    ss->st_ctimensec	= di->di_ctimensec;
    ss->st_size		= di->di_size;
    ss->st_blocks	= di->di_blocks;
    ss->st_flags	= di->di_flags;
    ss->st_gen		= di->di_gen;

    free (di);
    return 0;
  }


