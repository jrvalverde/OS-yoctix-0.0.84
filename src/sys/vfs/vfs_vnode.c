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
 *  vfs/vfs_vnode.c  --  vnode operations
 *
 *	Vnodes are in-core representations of files on mounted filesystems
 *	(on disks, via nfs, ...). A file on a filesystem is represented by
 *	only one vnode, or none at all if it hasn't been referenced.
 *
 *	When the system runs short of memory, unused vnode entries could
 *	be removed from memory (vnode_remove()).
 *
 *
 *	Internal functions:
 *	-------------------
 *
 *	namehash ()
 *		Calculate a hashvalue for a name (for example a filename).
 *
 *	vname_create ()
 *		Creates a vnodename struct.
 *
 *	vname_remove ()
 *		Removes a vnodename struct from the name cache.
 *
 *	vnode_create ()
 *		Creates a vnode given a filename. Only called internally
 *		by vnode_lookup() doesn't find a filename.
 *
 *	vnode_create_rootvnode ()
 *		Called during the first mount (/) to make sure that we have
 *		a vnode representing the root directory.
 *
 *
 *	Functions that may be used by any part of the system:
 *	-----------------------------------------------------
 *
 *	vnode_lookup ()
 *		This function returns a pointer to a vnode containing the
 *		data for a given filename. If the filename is not yet
 *		represented by a vnode, vnode_create() is called.
 *
 *	vnode_remove ()
 *		Remove a vnode from the hashtablechainthing and free the
 *		vnode struct from memory. Called by some cleanup routine.
 *
 *	vnode_pagein ()
 *		Called from the vm_pagefaulthandler() whenever a page is
 *		not present in physical memory.
 *
 *	vnode_close ()
 *		Should be called when the last filedescriptor refering
 *		a vnode is being closed.
 *
 *  History:
 *	8 Feb 2000	first version (vnode_lookup,create,remove, namehash)
 *	25 Feb 2000	adding vnode_pagein()
 *	7 Jun 2000	locking
 *	19 Nov 2000	correct multiple name per vnode stuff
 */


#include "../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/md/machdep.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/defs.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/interrupts.h>
#include <sys/lock.h>



extern struct vnodename **vnode_name_chain;
extern struct vnode **vnode_idev_chain;
extern struct lockstruct vnode_chains_lock;



struct vnodename *vname_create (hash_t hash, char *filename,
	struct vnode *v)
  {
    /*
     *	This function should only be used internally in vfs_vnode.c.
     *	A vnodename struct is allocated and filled with data (filename,
     *	hash, and a pointer to a vnode).
     *	Returns NULL on error, otherwise a pointer to the vnodename struct.
     */

    struct vnodename *tmp;
    int flen;

    tmp = (struct vnodename *) malloc (sizeof(struct vnodename));
    if (!tmp)
      return NULL;

    memset (tmp, 0, sizeof(struct vnodename));
    tmp->v = v;
    tmp->hash = hash;
    flen = strlen (filename);
    tmp->name = (char *) malloc (flen+1);
    if (!tmp->name)
      {
	free (tmp);
	return NULL;
      }
    strlcpy (tmp->name, filename, flen+1);

    return tmp;
  }



int vname_remove (struct vnodename *vnptr, hash_t hindex)
  {
    /*
     *	This function removes a vnodename from the vnodename
     *	cache.  The caller _must_ take care of calling lock()
     *	before calling this function, and unlock() afterwards.
     *
     *	Returns 0 on success, errno on error.
     */

    if (!vnptr)
      return EINVAL;

    if (vnptr->prev)
      vnptr->prev->next = vnptr->next;
    if (vnptr->next)
      vnptr->next->prev = vnptr->prev;

    if (vnode_name_chain[hindex] == vnptr)
      vnode_name_chain[hindex] = vnptr->next;

    free (vnptr->name);

    if (vnptr->v && vnptr->v->vname == vnptr)
      vnptr->v->vname = NULL;

    free (vnptr);

    return 0;
  }



hash_t namehash (unsigned char *name, int len)
  {
    /*
     *	Calculate and return a hashvalue (a kind of weak checksum)
     *	based on the contents of 'name'. If name==NULL, we return 0.
     *
     *	TODO:	This is just something I made up because I don't have
     *		the time to learn about good hash-generating algorithms
     *		right now... but it should be fixed some time...
     */

    int i = 0;
    hash_t hash = 0;

    if (!name || len<0)
	return 0;

    while (i<len)
      {
	hash += (name[i] * 2013491);
	hash ^= ((name[i] & 0x55) * 1013471);
	hash -= ((name[i] & 0xaa) * 1394177);
	i++;
      }

    return hash;
  }



struct vnode *vnode_create (struct proc *p, char *filename, int *errno,
	char *lockaddr, struct stat *ss, struct mountinstance *mi,
	hash_t hash_n, hash_t hindex_n, hash_t hash_idev, hash_t hindex_idev)
  {
    /*
     *	This function should only be called internally by vnode_lookup(),
     *	or when first mounting "/" by vnode_create_rootvnode().
     *
     *	filename must begin with a slash (/).
     *	ss must contain valid stat data.
     */

    struct vnode *v;
    struct vnodename *vname;

    *errno = 0;

    if (!filename || !errno || !p)
	panic ("vnode_create(): NULL");

    v = (struct vnode *) malloc (sizeof(struct vnode));
    if (!v)
      {
	*errno = ENOMEM;
	return NULL;
      }

    memset (v, 0, sizeof(struct vnode));


    /*
     *	We "lock" this vnode manually to make sure that nobody modifies
     *	the contents of the vnode before the caller is finished with it.
     */

    if (lock (&v->lock, lockaddr, LOCK_NONBLOCKING | LOCK_RW) < 0)
      printk ("vnode_create: could not lock ???");


    /*
     *	We fill v->ss with data from ss. (vnode_lookup() should already
     *	have called stat(), otherwise we wouldn't be here.)
     */

    v->mi = mi;
    v->ss = *ss;


    /*
     *	If this is a regular file, we simply direct read/write etc
     *	to the filesystem driver of the mountinstance of the vnode.
     *	If it is a device special file, then let's point those pointers
     *	to the device' functions.
     */

    if (!v->ss.st_rdev)
      {
	/*  Regular files:  */
	if (v->mi && v->mi->fs)
	  {
	    v->read = v->mi->fs->read;
	    v->write = v->mi->fs->write;
	  }
	else
	  printk ("vnode_create: warning: no mountinstance or no fs ???");
      }
    else
      {
	/*  Device files:  (both character and block devices)  */
	v->open  = v->ss.st_rdev->open;
	v->close = v->ss.st_rdev->close;
	v->read  = v->ss.st_rdev->read;
	v->write = v->ss.st_rdev->write;
	v->ioctl = (void *) v->ss.st_rdev->ioctl;
	v->lseek = v->ss.st_rdev->seek;
      }


    /*
     *	Add first in name and inode/dev chains
     */

    lock (&vnode_chains_lock, "vnode_create", LOCK_BLOCKING | LOCK_RW);

    vname = vname_create (hash_n, filename, v);
    if (!vname)
      {
	*errno = ENOMEM;
	unlock (&vnode_chains_lock);
	return NULL;
      }

    v->vname = vname;	/*  the vnode knows about only _one_ of its names  */

    vname->next = vnode_name_chain [hindex_n];
    if (vnode_name_chain[hindex_n])
	vnode_name_chain [hindex_n]->prev = vname;
    vnode_name_chain [hindex_n] = vname;

    v->next = vnode_idev_chain [hindex_idev];
    if (vnode_idev_chain[hindex_idev])
	vnode_idev_chain [hindex_idev]->prev = v;
    vnode_idev_chain [hindex_idev] = v;

    unlock (&vnode_chains_lock);

    return v;
  }



int vnode_remove (struct vnode *v)
  {
    /*
     *	Remove the vnode pointed to by v from memory.
     *	Returns 0 on success, errno on failure.
     *
     *	1)  Remove all vnodenames refering this vnode
     *	2)  Remove the vnode_idev lookup
     *
     *	TODO: This is slow, just a linear search through all
     *	vnodenames in the entire system...
     */

    int i;
    struct vnodename *vnp, *nextvnp;
    byte tmpbuf [sizeof(dev_t) + sizeof(inode_t)];
    hash_t hash, hindex;

    if (!v)
      return EINVAL;

    if (v->refcount > 0)
      {
	panic ("vnode_remove: vnode '%s' refcount=%i",
		v->vname->name);
	return EINVAL;
      }

    lock (&vnode_chains_lock, "vnode_create", LOCK_BLOCKING | LOCK_RW);

    for (i=0; i<VNODE_NAME_HASHSIZE; i++)
      {
	vnp = vnode_name_chain [i];
	while (vnp)
	  {
	    nextvnp = vnp->next;
	    if (vnp->v == v)
		vname_remove (vnp, i);
	    vnp = nextvnp;
	  }
      }

    memcpy (tmpbuf, &v->ss.st_dev, sizeof(dev_t));
    memcpy (tmpbuf+sizeof(dev_t), &v->ss.st_ino, sizeof(inode_t));

    hash = namehash (tmpbuf, sizeof(dev_t) + sizeof(inode_t));
    hindex = hash & (VNODE_IDEV_HASHSIZE - 1);

    /*  Remove the vnode from the cache chain:  */
    if (v->next)
	v->next->prev = v->prev;
    if (v->prev)
	v->prev->next = v->next;

    if (vnode_idev_chain[hindex] == v)
	vnode_idev_chain[hindex] = v->next;

    unlock (&vnode_chains_lock);
    free (v);

    return 0;
  }



struct vnode *vnode_lookup (struct proc *p, char *filename1, int *errno,
	char *lockaddr)
  {
    /*
     *	vnode_lookup()
     *	--------------
     *
     *	On success, this function returns a pointer to a vnode struct for the
     *	filename specified. (NULL is returned on error, and *errno is set
     *	to indicate the error.)
     *
     *	If there was no error, the vnode is locked by setting its lock
     *	value to 'lockaddr'. This way, the caller can be sure to have
     *	exclusive access to the contents of the vnode when vnode_lookup()
     *	returns.  (The called must call unlock() to release the lock.)
     *
     *	How to look up a vnode:
     *
     *	1)  Try to find the file in the name hashtable (using the filename)
     *
     *	2)  Try to find the file in the inode/device hashtable
     *		(use stat(filename) to get inode/device numbers...)
     *
     *	If the vnode still isn't found, vnode_create is called.
     */

    hash_t hash, hindex, hash2, hindex2;
    int len;
    struct vnode *v;
    struct vnodename *vname;
    struct mountinstance *tmp_mi;
    struct stat tmp_ss;
    byte tmpbuf [sizeof(dev_t) + sizeof(inode_t)];
    char *filename;
    char *curdirname;

    *errno = 0;

    if (!filename1)
      {
	printk ("vnode_lookup(): filename1==NULL");
	return NULL;
      }

    if (filename1[0]=='/')
      {
	filename = (char *) malloc (strlen(filename1)+1);
	if (!filename)
	  {
	    *errno = ENOMEM;
	    return NULL;
	  }
	strlcpy (filename, filename1, strlen(filename1)+1);
      }
    else
      {
	/*  First: add name of current directory:  */
	curdirname = p->currentdir_vnode->vname->name;
	if (!curdirname)
	  {
	    *errno = EINVAL;	/*  TODO: better error?  */
	    return NULL;
	  }
	len = strlen(curdirname);
	if (curdirname[len]=='/')
	  len --;

	filename = (char *) malloc (len + 1 + strlen(filename1) + 1);
	if (!filename)
	  {
	    *errno = ENOMEM;
	    return NULL;
	  }
	snprintf (filename, len+1+strlen(filename1)+1,
		"%s/%s", curdirname, filename1);
      }

    /*  Calculate name hash:  (index into vnode_name_chain[])  */
    hash = namehash (filename, strlen (filename));
    hindex = hash & (VNODE_NAME_HASHSIZE - 1);

    lock (&vnode_chains_lock, "vnode_lookup", LOCK_BLOCKING | LOCK_RO);

    /*  See if the name is actually on that chain:  */
    vname = vnode_name_chain[hindex];
    while (vname)
      {
	if (vname->hash == hash)
	  if (!strcmp(vname->name, filename))
	    {
	      unlock (&vnode_chains_lock);
	      free (filename);
	      v = vname->v;
	      lock (&v->lock, lockaddr, LOCK_BLOCKING | LOCK_RW);
	      return v;
	    }

	vname = vname->next;
      }

    unlock (&vnode_chains_lock);


    /*
     *	If we didn't find the vnode using the filename, try finding it
     *	in the inode/device hash:
     */

    *errno = vfs_stat (p, filename, &tmp_mi, &tmp_ss);
    if (*errno)
      {
	free (filename);
	return NULL;
      }

    memcpy (tmpbuf, &tmp_ss.st_dev, sizeof(dev_t));
    memcpy (tmpbuf+sizeof(dev_t), &tmp_ss.st_ino, sizeof(inode_t));

    hash2 = namehash (tmpbuf, sizeof(dev_t) + sizeof(inode_t));
    hindex2 = hash2 & (VNODE_IDEV_HASHSIZE - 1);

    lock (&vnode_chains_lock, "vnode_lookup", LOCK_BLOCKING | LOCK_RW);

    /*  See if the inode/dev is actually on that chain:  */
    v = vnode_idev_chain[hindex2];
    while (v)
      {
	if (v->ss.st_ino == tmp_ss.st_ino)
	  if (v->ss.st_dev == tmp_ss.st_dev)
	    {
	      /*
	       *  If we are here, then it means that the filename wasn't
	       *  found in the filename hash table, but it was found using
	       *  its inode/device numbers.  Let's add the name to the
	       *  name hash table:
	       */

	      /*
	       *  NOTE:  normally, if we're low on memory, we'd return
	       *  ENOMEM here... but since vnode_lookup() has actually
	       *  succeeded, we might just as well return without adding
	       *  the filename to the hash table:
	       */

	      vname = vname_create (hash, filename, v);
	      if (vname)
		{
		  vname->next = vnode_name_chain [hindex];
		  if (vnode_name_chain[hindex])
		    vnode_name_chain [hindex]->prev = vname;
		  vnode_name_chain [hindex] = vname;
		}

	      unlock (&vnode_chains_lock);
	      free (filename);
	      lock (&v->lock, lockaddr, LOCK_BLOCKING | LOCK_RW);
	      return v;
	    }

	v = v->next;
      }

    unlock (&vnode_chains_lock);

    /*  Create the vnode:  */
    v = vnode_create (p, filename, errno, lockaddr, &tmp_ss, tmp_mi,
		hash, hindex, hash2, hindex2);
    free (filename);
    return v;
  }



void *vnode_pagein (int *errno, struct vm_region *vmregion, struct vm_object *vmobject, size_t linearaddr,
	struct proc *p)
  {
    /*
     *	Read a page from a file
     *	-----------------------
     *
     *	(o)  Allocate memory for the page.
     *
     *	(o)  Where should we read from?
     *		(linearaddr - vmregion->start_addr) + vmregion->srcoffset
     *
     *	(o)  Return a pointer to the page.
     *
     *	NOTE: We do NOT lock the vnode. (TODO ?)  If we simply lock the vnode,
     *	then we can get a deadlock. (For example, if we get a page fault while
     *	in sys_execve() then we cannot lock the vnode here.)
     */

    void *pagebuffer;
    int res;
    struct vnode *v;
    off_t actually_read = 0;


    v = vmobject->vnode;

    pagebuffer = (void *) malloc (PAGESIZE);
    if (!pagebuffer)
      {
	*errno = ENOMEM;
	return NULL;
      }

    res = v->read (v, (off_t) ((linearaddr - vmregion->start_addr) + vmregion->srcoffset),
	pagebuffer, (off_t) PAGESIZE, &actually_read, p);

    if (actually_read < PAGESIZE)
      {
	*errno = EIO;
	free (pagebuffer);
	return NULL;
      }
    else
	*errno = res;

    return pagebuffer;
 }



int vnode_close (struct vnode *v, struct proc *p)
  {
    /*
     *	vnode_close ()
     *	--------------
     *
     *	Decrease a vnode's refcount. If there are no more references, and the
     *	vnode has a close() function, then we call it.
     *	Returns 0 on success, errno on error.
     *
     *	Note: this function doesn't actually remove the vnode from memory.
     */

    int err;

    if (!v)
	return EINVAL;

    lock (&v->lock, "vnode_close", LOCK_BLOCKING | LOCK_RW);
    v->refcount --;
    if (v->refcount < 0)
	printk ("vnode_close: WARNING! negative refcount on vnode"
		" %s", v->vname->name);

    if (v->refcount == 0 && v->close)
      {
	err = v->close (v->ss.st_rdev, p);
	if (err)
	  return err;
      }

    unlock (&v->lock);
    return 0;
  }



struct vnode *vnode_create_rootvnode (struct mountinstance *mi,
	struct proc *p, int *errno, char *lockaddr)
  {
    /*
     *	This function should only be called once, to create the root
     *	vnode.
     */

    hash_t hash_n, hash_idev, hindex_n, hindex_idev;
    struct stat tmp_ss;
    byte tmpbuf [sizeof(dev_t) + sizeof(inode_t)];

    if (!mi)
      return NULL;

    if (!mi->fs || !mi->fs->istat || !mi->superblock)
      return NULL;

    *errno = mi->fs->istat (mi, (inode_t) mi->superblock->root_inode,
	&tmp_ss, p);

    if (*errno)
      return NULL;

    hash_n = namehash ("/", 1);
    hindex_n = hash_n & (VNODE_NAME_HASHSIZE - 1);

    memcpy (tmpbuf, &tmp_ss.st_dev, sizeof(dev_t));
    memcpy (tmpbuf+sizeof(dev_t), &tmp_ss.st_ino, sizeof(inode_t));

    hash_idev = namehash (tmpbuf, sizeof(dev_t) + sizeof(inode_t));
    hindex_idev = hash_idev & (VNODE_IDEV_HASHSIZE - 1);

    return vnode_create (p, "/", errno, lockaddr, &tmp_ss, mi,
	hash_n, hindex_n, hash_idev, hindex_idev);
  }

