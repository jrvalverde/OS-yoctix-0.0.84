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
 *  modules/fs/ffs/dinode.c
 *
 *  Included from ffs.c
 */


int ffs__readdinode (struct mountinstance *mi, inode_t inode, struct dinode *di, struct proc *p)
  {
    /*
     *	ffs__readdinode ()
     *	------------------
     *
     *	Read the dinode struct of inode 'inode' into the memory pointed to by 'di'.
     *	(Used internally by ffs_istat() and others.)
     *	Returns errno on error, 0 on success.
     */

    struct ffs_superblock *fsb;
    int cg_nr;
    int inode_within_cg;
    int dinodes_per_block;
    int inode_within_buf;
    daddr_t b;				/*  block address  */
    daddr_t bdev;			/*  device address  */
    struct dinode *buf;
    int res;

    if (!mi || inode<2 || !di)
      return EINVAL;

    fsb = (struct ffs_superblock *) mi->superblock->fs_superblock;

/*  TODO: remove (int) (req. external libc stuff... ugly) Use ino_to_cg instead! */
    cg_nr = (int)inode / fsb->fs_ipg;		/*  Cyl. group number  */
    inode_within_cg = (int)inode % fsb->fs_ipg;	/*  inode within this group  */

    /*  Which filesystem block within the cylinder group?  */
    dinodes_per_block = fsb->fs_fsize/sizeof(struct dinode);
    b = cgimin (fsb, cg_nr) + (int)(inode_within_cg / dinodes_per_block);
    inode_within_buf = inode_within_cg % dinodes_per_block;
    bdev = fsbtodb (fsb, b);

    /*  Read one block:  (this block contains the dinode data)  */
    buf = (struct dinode *) malloc (fsb->fs_fsize);
    if (!buf)
	return ENOMEM;
    res = block_read (mi, bdev, fsbtodb (fsb, 1), buf, p);
    if (res)
      {
	free (buf);
	return res;
      }

    /*  buf[inode_within_buf] contains our inode. Let's copy it to di:  */
    *di = buf[inode_within_buf];

    free (buf);
    return 0;
  }



int ffs_readlink (struct mountinstance *mi, inode_t i, char *buf,
		size_t bufsize, struct proc *p, size_t *realsize)
  {
    /*
     *	ffs_readlink ()
     *	---------------
     *
     *	Read the contents of a symbolic link by reading its dinode. If the
     *	file is indeed a symbolic link, then the contents of the link is
     *	placed at di_db[0] as ASCII.
     *
     *	Returns errno on error, 0 on success.
     */

    int res;
    struct dinode di;
    char *j;
    int out = 0;


    res = ffs__readdinode (mi, i, &di, p);
    if (res)
      return res;

    if ((di.di_mode & IFMT) != IFLNK)
      return EINVAL;


    /*
     *	Copy out the link contents:   (Actually, we could have a simple
     *	strlcpy (buf, (char *) &di.di_db[0], bufsize);  here, but readlink()
     *	is not supposed to put a trailing null byte on buf, so we copy
     *	byte by byte manually instead.)
     */

    *realsize = 0;

    for (j = (char *) &di.di_db[0];
	 j < (char *) (&di + sizeof(struct dinode));
	 j++)
      {
	if (out >= bufsize)
	  break;
	buf[out++] = *j;
	(*realsize)++;
      }

    return 0;
  }

