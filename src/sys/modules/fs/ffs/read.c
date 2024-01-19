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
 *  modules/fs/ffs/read.c
 *
 *  Included from ffs.c
 */


int ffs__readblock (struct mountinstance *mi, inode_t i, daddr_t blnr, byte *buf, struct proc *p)
  {
    /*
     *	ffs__readblock ()
     *	-----------------
     *
     *	As slow as it gets  ;-)
     *
     *	'blnr' is a block number. Get the dinode data for inode 'i' to see
     *	which block(s) on disk this block refers to. (Yeah, this needs
     *	a rewrite... I know :)
     *	buf must be large enough to hold fsb->fs_bsize bytes.
     */

    struct ffs_superblock *fsb = (struct ffs_superblock *) mi->superblock->fs_superblock;
    int res;
    daddr_t localbn;		/*  "local" block number... ie within an indirection block  */
    daddr_t indlimit;
    struct dinode *di;
    daddr_t physaddr;

    if (!mi || i<2 || blnr<0 || !buf)
	return EINVAL;

    di = (struct dinode *) malloc (sizeof(struct dinode));
    if (!di)
	return ENOMEM;

    res = ffs__readdinode (mi, i, di, p);
    if (res)
      {
	free (di);
	return res;
      }

    if (blnr > di->di_blocks)
      {
	printk ("ffs__readblock(): blnr=%i, di->di_blocks=%i",
			(int)blnr, (int)di->di_blocks);
	free (di);
	return EINVAL;	/*  TODO: better error code  */
      }


    /*
     *	FFS disk blocks are either addressed directly or indirectly (via one,
     *	two, or three indirection blocks). Here's how we find out how many nr
     *	of indirections we need to read the current block (blnr) into memory:
     *
     *	  1)  The first NDADDR blocks are directly pointed to from the dinode
     *	  2)  After that, the first indirect block contains pointers for the
     *	      next fsb->fs_bsize / sizeof(ufs_daddr_t) number of blocks
     *	  3)  The second indirect block contains pointers to the next
     *	      (fsb->fs_bsize / sizeof(ufs_daddr_t))^2 number of blocks
     *	  4)  -- " --  (fsb->fs_bsize / sizeof(ufs_daddr_t))^3 blocks...
     *
     *	TODO:  these "limits" should be held in some cache variables... it's
     *	pretty stupid to calculate these for each block we read...
     */

    if (blnr < NDADDR)
      physaddr = di->di_db [blnr];
    else
      {
	/*  Block number to lookup:  */
	localbn = blnr - NDADDR;

	/*  Nr of blocks addressable from the first indirection block:  */
	indlimit = fsb->fs_bsize / sizeof(ufs_daddr_t);

	/*  Physical address of first indirection block:  */
	physaddr = di->di_ib [0];

	/*  Will first indirection do, or do we need to "indirect further"?  */
	if (localbn >= indlimit)
	  {
	    /*  Further indirection:  */
	    localbn -= indlimit;

panic ("ffs__readblock(): more than one level of indirection (TODO)");

	  }

	/*
	 *  Here:  physaddr should be the addr to the "last" indirection
	 *  block which we need to read, and localbn should be the block
	 *  number within that indirection block.
	 */

	res = block_read (mi, fsbtodb (fsb, physaddr), fsbtodb (fsb, 1 << fsb->fs_fragshift), buf, p);
	if (res)
	  {
	    free (di);
	    return res;
	  }

	/*  Get the final physaddr:  */
	physaddr = *((ufs_daddr_t *)buf + localbn);
      }

#if DEBUGLEVEL>=5
  printk ("  blnr=%i ==> physaddr=%i.  di_db={%i,%i,%i,%i,%i,%i,..}", (int)blnr, (int)physaddr,
	di->di_db[0], di->di_db[1], di->di_db[2], di->di_db[3], di->di_db[4], di->di_db[5]);
#endif

    res = block_read (mi, fsbtodb (fsb, physaddr), fsbtodb (fsb, 1 << fsb->fs_fragshift), buf, p);
    if (res)
      {
	free (di);
	return res;
      }

    free (di);
    return 0;
  }



int ffs_read (struct vnode *v, off_t offset, byte *buffer, off_t length, off_t *transfered, struct proc *p)
  {
    /*
     *	ffs_read ()
     *	-----------
     */

    struct ffs_superblock *fsb = (struct ffs_superblock *) v->mi->superblock->fs_superblock;
    int res;
    off_t blnr;
    off_t len_to_copy;
    int offset_within_block;
    byte *tmp;

    if (!v || !buffer || !transfered || length<0)
      return EINVAL;
/*
printk ("ffs_read: offset=%i length=%i", (int)offset, (int)length);
*/
    *transfered = 0;

    if (length==0)
      return 0;

    tmp = (byte *) malloc (fsb->fs_bsize);
    if (!tmp)
      return ENOMEM;

    /*  TODO:  int --> daddr_t or off_t... or even better: bitshift  */
    blnr = (int)offset / fsb->fs_bsize;
    offset_within_block = (int)(offset) % fsb->fs_bsize;

    while (*transfered < length)
      {
	res = ffs__readblock (v->mi, v->ss.st_ino, blnr, tmp, p);
	if (res)
	  {
	    free (tmp);
	    return res;
	  }
/*
printk ("DUMP a: %X %X ofswithinbl=%i", *((u_int64_t *)tmp), *((u_int64_t *)tmp+512),
		(int)offset_within_block);
*/
	/*  Copy data from tmp to buffer:  */
	len_to_copy = length - *transfered;
	if (len_to_copy+offset_within_block > fsb->fs_bsize)
		len_to_copy = fsb->fs_bsize-offset_within_block;
	memcpy (buffer, tmp+offset_within_block, len_to_copy);
/*
printk ("DUMP b: %X %X len2c=%i", *((u_int64_t *)buffer), *((u_int64_t *)buffer+1),
		(int)len_to_copy);
*/
	buffer += len_to_copy;
	(*transfered) += len_to_copy;
	offset_within_block = 0;
	blnr ++;
      }

    free (tmp);
    return 0;
  }


