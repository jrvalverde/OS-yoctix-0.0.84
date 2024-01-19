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
 *  modules/fs/ffs/super.c
 *
 *  Included from ffs.c
 */



void ffs__superblockdump (struct ffs_superblock *buf)
  {
    /*
     *	ffs__superblockdump ()
     *	----------------------
     *
     *	Used while debuging the driver...
     */

    printk ("ffs__superblockdump:");
    printk ("  sblkno=%i, cblkno=%i iblkno=%i dblkno=%i,",
	buf->fs_sblkno, buf->fs_cblkno, buf->fs_iblkno, buf->fs_dblkno);
    printk ("  size=%i, dsize=%i, ncg=%i, fsize=%i, bsize=%i, frag=%i,",
	buf->fs_size, buf->fs_dsize, buf->fs_ncg, buf->fs_fsize, buf->fs_bsize, buf->fs_frag);
    printk ("  id=%x;%x, csaddr=%i cssize=%i cgsize=%i ntrak=%i,",
	buf->fs_id[0], buf->fs_id[1], buf->fs_csaddr, buf->fs_cssize, buf->fs_cgsize, buf->fs_ntrak);
    printk ("  nsect=%i spc=%i ncyl=%i cpg=%i ipg=%i fpg=%i",
	buf->fs_nsect, buf->fs_spc, buf->fs_ncyl, buf->fs_cpg, buf->fs_ipg, buf->fs_fpg);
    printk ("  fsmnt=\"%s\", inodefmt=%i, maxfilesize=%X, fsbtodb=%i",
	buf->fs_fsmnt, buf->fs_inodefmt, (u_int64_t)buf->fs_maxfilesize, buf->fs_fsbtodb);
    printk ("  bmask=%x fmask=%x bshift=%i fshift=%i, fragshift=%i",
	buf->fs_bmask, buf->fs_fmask, buf->fs_bshift, buf->fs_fshift, buf->fs_fragshift);
  }



int ffs_read_superblock (struct mountinstance *mi, struct proc *p)
  {
    /*
     *	ffs_read_superblock ()
     *	----------------------
     *
     *	Read the ffs superblock data from mountinstance mi.
     */

    struct ffs_superblock *buf;		/*  superblock buffer  */
    int bsize;				/*  device block size  */
    int res;


    /*
     *	 Figure out blocksize:
     */

    /*  If we're reading from a file, then assume blocksize 512 for now... (TODO)  */
    if (!mi->device)
	bsize = 512;
    else
	bsize = mi->device->bsize;
    mi->superblock->blocksize = bsize;

    buf = (struct ffs_superblock *) malloc (SBSIZE);
    if (!buf)
	return ENOMEM;

    res = block_read (mi, (daddr_t) ((int)SBOFF/bsize), (daddr_t) ((int)SBSIZE/bsize), buf, p);
    if (res)
      {
	free (buf);
	printk ("ffs_read_superblock: block_read: res=%i", res);
	return res;
      }

    mi->superblock->fs_superblock = buf;
    mi->superblock->root_inode = 2;			/*  Legacy stuff  */
    mi->superblock->nr_of_blocks = buf->fs_size;

    if (buf->fs_dsize<0 || buf->fs_size<buf->fs_dsize || buf->fs_ncg<1
	|| buf->fs_fsize<1 || buf->fs_bsize<buf->fs_fsize || buf->fs_frag<1)
      {
	ffs__superblockdump (buf);
	free (buf);
	printk ("ffs_read_superblock: invalid superblock");
	return EINVAL;
      }

#if DEBUGLEVEL>=4
    ffs__superblockdump (buf);
#endif

    if (buf->fs_magic!=FS_MAGIC || buf->fs_inodefmt!=FS_44INODEFMT ||
	buf->fs_bsize!=buf->fs_fsize*buf->fs_frag)
      {
	if (buf->fs_magic!=FS_MAGIC)
	  printk ("ffs_read_superblock: fs_magic=0x%x, should be 0x%x", buf->fs_magic, FS_MAGIC);
	if (buf->fs_inodefmt!=FS_44INODEFMT)
	  printk ("ffs_read_superblock: fs_inodefmt=%i, only %i supported so far", buf->fs_inodefmt, FS_44INODEFMT);
	if (buf->fs_bsize!=buf->fs_fsize*buf->fs_frag)
	  printk ("ffs_read_superblock: fs_bsize=%i fsize=%i frag=%i (weird)",
		buf->fs_bsize, buf->fs_fsize, buf->fs_frag);
	free (buf);
	return EINVAL;
      }

    return 0;
  }


