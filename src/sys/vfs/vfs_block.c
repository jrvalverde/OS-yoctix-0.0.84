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
 *  vfs/vfs_block.c  --  block read/write on devices and files
 *
 *	block_read() and block_write() may be called by vfs code (or other
 *	code) to read/write one or more blocks from a mountinstance.
 *
 *	Internal functions:
 *
 *	block_readwrite ()
 *		Calls buffercache_read() or buffercache_write().
 *
 *	buffercache_read ()
 *
 *	buffercache_write ()
 *
 *
 *  History:
 *	5 Feb 2000	first version
 *	25 Feb 2000	cached read
 *	15 Mar 2000	uses device' readtip() if it exists to read
 *			the rest of a track from a disk into the cache
 *	26 Dec 2000	vfs_bcacheflush() called by timer every 15th sec.
 */


#include "../config.h"
#include <string.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/timer.h>
#include <sys/vfs.h>
#include <sys/std.h>



extern struct bcache_entry **bcache_chain;



void vfs_bcacheflush ()
  {
    /*
     *	vfs_bcacheflush ()
     *	------------------
     *
     */

    struct timespec ts;

    ts.tv_sec = 15;
    ts.tv_nsec = 0; 
    timer_ksleep (&ts, vfs_bcacheflush, 0);
  }



hash_t buffercache_hash (int a, int b)
  {
    /*
     *	Calculate buffercache hash value.
     *
     *	TODO:  Actually implement some kind of hashing-algorithm.
     *
     *	Right now, this is really stupid. The value of "a" should be
     *	an address in memory, for example to the mountinstance which
     *	we are reading to (or from, depending on how you see it).
     *	The value of "b" should be a block number.
     *
     *	Even if this is a really poor hash algorithm, it could
     *	work, since all we want is that the lowest bits of the
     *	hash value is "different enough for most blocks on most
     *	mountinstances". (I'm not good at describing this...)
     */

    return ((a>>12) + b);
  }



int buffercache_write (struct mountinstance *mi, daddr_t blocknr, void *buf,
		      struct proc *p)
  {
    /*
     *	buffercache_write ()
     *	--------------------
     *
     *	TODO:	readtip / writetip ??
     *		use buffercache!
     *		async writes!
     */

    int res;

    if (!mi || !buf)
      return EINVAL;

    lock (&mi->lock, "buffercache_write", LOCK_BLOCKING | LOCK_RW);

    if ((mi->flags & VFSMOUNT_RW)==0)
      {
	/*  Read only filesystem... actually, sys_open() should return
	    EROFS, not the actual write...  */
	unlock (&mi->lock);
	return EROFS;
      }

    if (mi->flags & VFSMOUNT_ASYNC)
      printk ("buffercache_write: async");

    unlock (&mi->lock);

    if (!mi->device)
      {
	printk ("buffercache_write: no device for mi '%s'",
		mi->mount_point);
	return EINVAL;
      }
    if (!mi->device->write)
      {
	printk ("buffercache_write: no device->write for mi '%s'",
		mi->mount_point);
	return EINVAL;
      }

    res = mi->device->write (mi->device, (daddr_t)blocknr, (daddr_t)1, buf, p);

printk ("buffercache_write: mi->device->write res = %i", res);

    return res;
  }



int buffercache_read (struct mountinstance *mi, daddr_t blocknr, void *buf,
		      struct proc *p)
  {
    /*
     *	buffercache_read ()
     *	-------------------
     *
     *	This is an internal function called by block_read(). First, we check
     *	to see if the specific blocknr-and-mountinstance is already in
     *	memory (in the cache). If that is the case, then we simply memcpy()
     *	the cached block to buf.
     *
     *	If we didn't find the block in the buffer cache, we have to read it
     *	from the underlaying device (often a physical disk drive). In practice,
     *	we ask the device how many blocks it believes to be effective to read
     *	at once. For example, reading one single sector from a rotating disk
     *	may cause the next sector to be "missed", and we'll have to wait for
     *	the next revolution of the disk before we can read it.
     *
     *	If the device doesn't provide us with information about the number of
     *	blocks to read, then we assume that reading one (1) block will be okay.
     */

    hash_t hash;
    struct bcache_entry *bptr, *found;
    int res;
    byte *large_buf;
    daddr_t blocks_to_read, startblock, i;
    daddr_t wanted_block;


    /*  Calculate an extremely simple and stupid "hash" of 'mi' and 'blocknr':  */
    hash = buffercache_hash ((int)mi, (int)blocknr);

    /*  Scan the 'hash' branch of bcache_chain[] to find our block:  */
    bptr = bcache_chain [hash & (BCACHE_HASHSIZE-1)];
    found = NULL;
    while (bptr)
      {
	if (bptr->hash == hash && bptr->blocknr == blocknr && bptr->mi == mi)
	  {
	    found = bptr;
	    bptr = NULL;
	  }
	else
	  bptr = bptr->next;
      }

    lock (&mi->lock, "buffercache_read", LOCK_BLOCKING | LOCK_RW);

    /*  Found the block in the cache? Then memcpy() it to buf and return:  */
    if (found)
      {
	memcpy (buf, found->bufferptr, mi->superblock->blocksize);
	unlock (&mi->lock);
	return 0;
      }


    /*
     *	We are here if the block was not found in the cache.
     *
     *	First, we ask the device how many blocks it thinks is effective to
     *	read in one call, and where to start. (For example, a floppy
     *	disk driver would give us the tip to read an entire track at once.)
     *	If it doesn't provide us with that information, we assume that
     *	reading one (1) block will be okay.
     */

    startblock = blocknr;
    if (mi->device->readtip)
      {
	res = mi->device->readtip (mi->device, blocknr, &blocks_to_read,
		&startblock);
	if (blocks_to_read < 1 || res>0)
	  {
	    printk ("block_read(): bad readtip from device %s: "
		"blocks_to_read=%i startblock=%i res=%i",
		mi->device->name, (int)blocks_to_read,
		(int)startblock, res);
	    unlock (&mi->lock);
	    return EINVAL;
	  }
      }
    else
      blocks_to_read = 1;


    large_buf = (byte *) malloc (blocks_to_read * mi->superblock->blocksize);
    if (!large_buf)
      {
	unlock (&mi->lock);
	return ENOMEM;
      }

    /*  wanted_block is "index" into the read buffer to the block
	we want... (0-based)  */
    wanted_block = blocknr - startblock;
    blocknr = startblock;

    /*  Read from the device (non-cached access):  */
    res = mi->device->read (mi->device, blocknr, blocks_to_read, large_buf, p);
    if (res)
      {
	free (large_buf);
	unlock (&mi->lock);
	return res;
      }

    /*  Add _ALL_ the read blocks to the buffer cache:  */
    for (i=0; i<blocks_to_read; i++)
      {
	/*  ... but only if it was not already in the buffer cache!  */
	hash = buffercache_hash ((int)mi, (int)(blocknr + i));

	bptr = bcache_chain [hash & (BCACHE_HASHSIZE-1)];
	found = NULL;
	while (bptr)
	  {
	    if (bptr->hash == hash && bptr->blocknr == blocknr && bptr->mi == mi)
	      {
		found = bptr;
		bptr = NULL;
	      }
	    else
	      bptr = bptr->next;
	  }

	if (!found)
	  {
	    found = (struct bcache_entry *) malloc (sizeof(struct bcache_entry));
	    if (!found)
	      {
		free (large_buf);
		unlock (&mi->lock);
		return ENOMEM;
	      }
	    memset (found, 0, sizeof(struct bcache_entry));
	    found->mi = mi;
	    found->blocknr = blocknr + i;
	    found->hash = hash;
	    found->bufferptr = (byte *) malloc (mi->superblock->blocksize);
	    if (!found->bufferptr)
	      {
		free (found);
		free (large_buf);
		unlock (&mi->lock);
		return ENOMEM;
	      }
	    found->next = bcache_chain[hash & (BCACHE_HASHSIZE-1)];
	    bcache_chain[hash & (BCACHE_HASHSIZE-1)] = found;

	    /*  Copy data from large_buf to the buffer cache:  */
	    memcpy (found->bufferptr, large_buf + i*mi->superblock->blocksize,
		mi->superblock->blocksize);
	  }
      }

    /*  Copy data to "buf" from large_buf:  */
    memcpy (buf, large_buf + wanted_block*mi->superblock->blocksize,
	mi->superblock->blocksize);

    free (large_buf);

    unlock (&mi->lock);
    return 0;
  }



int block_readwrite (struct mountinstance *mi, daddr_t blocknr,
	daddr_t nrofblocks, void *buf, struct proc *p,
	int writeflag)
  {
    /*
     *	block_readwrite ()
     *	------------------
     *
     *	Read/write blocks to/from the buffer at 'buf'. The function used
     *	depends on writeflag (0=read, 1=write).
     *
     *	Information about the device to be read from is included in "mi".
     *	This function is only used internally in vfs_block.c.
     *
     *	Returns 0 on success, errno on error.
     */

    int res;
    daddr_t i;

    if (!mi || !buf)
	return EINVAL;

    /*  Should we read/write from/to a file?  */
    if ((mi->flags & VFSMOUNT_FILE) || !mi->device)
      {
	printk ("block_readwrite(): only device access is supported, "
		"not files yet (TODO)");
	return ENOTBLK;
      }

    /*  Read/write all the blocks:  */
    for (i=0; i<nrofblocks; i++)
      {
	if (writeflag)
	  res = buffercache_write (mi, blocknr+i, buf, p);
	else
	  res = buffercache_read (mi, blocknr+i, buf, p);

	if (res)
	  return res;

	buf += mi->device->bsize;   /*  or mi->superblock->blocksize ???  */
      }

    return 0;
  }



int block_read (struct mountinstance *mi, daddr_t blocknr, daddr_t nrofblocks,
	void *buf, struct proc *p)
  {
    /*
     *	block_read ()
     *	-------------
     */

    return block_readwrite (mi, blocknr, nrofblocks, buf, p, 0);
  }



int block_write (struct mountinstance *mi, daddr_t blocknr, daddr_t nrofblocks,
	void *buf, struct proc *p)
  {
    /*
     *	block_write ()
     *	--------------
     */

    return block_readwrite (mi, blocknr, nrofblocks, buf, p, 1);
  }

