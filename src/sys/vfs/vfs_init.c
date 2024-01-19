/*
 *  Copyright (C) 1999,2000 by Anders Gavare.  All rights reserved.
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
 *  vfs/vfs_init.c  --  initialize the virtual file system
 *
 *	vfs_init()
 *		Initialize the virtual file system
 *
 *	vfs_register()
 *		Register a filesystem driver
 *
 *	TODO:  vfs_unregister()
 *
 *
 *  History:
 *	27 Dec 1999	first version
 *	20 Jan 2000	adding vfs_register()
 *	8 Feb 2000	adding vnode stuff
 */


#include "../config.h"
#include <string.h>
#include <sys/malloc.h>
#include <sys/vfs.h>
#include <sys/std.h>
#include <sys/vnode.h>
#include <sys/module.h>



struct filesystem *firstfilesystem = NULL;
struct mountinstance *firstmountinstance = NULL;

struct vnodename **vnode_name_chain = NULL;
struct vnode **vnode_idev_chain = NULL;
struct lockstruct vnode_chains_lock;		/*  This is the lock used for
					    both the name and idev chains */

struct bcache_entry **bcache_chain = NULL;



void vfs_init ()
  {
    int i;

    memset (&vnode_chains_lock, 0, sizeof(struct lockstruct));

    /*
     *	Register the VFS:
     */
    module_register ("virtual", MODULETYPE_SYSTEM | MODULETYPE_BUILTIN,
		     "vfs", "Virtual File System");


    /*
     *	Create the vnode linked list hashtables:
     */

    vnode_name_chain = (struct vnodename **)
		malloc (VNODE_NAME_HASHSIZE * sizeof(struct vnodename *));
    if (!vnode_name_chain)
	panic ("vfs_init(): could not malloc vnode_name_chain[]");

    for (i=0; i<VNODE_NAME_HASHSIZE; i++)
	vnode_name_chain [i] = NULL;

    vnode_idev_chain = (struct vnode **)
		malloc (VNODE_IDEV_HASHSIZE * sizeof(struct vnode *));
    if (!vnode_idev_chain)
	panic ("vfs_init(): could not malloc vnode_idev_chain[]");

    for (i=0; i<VNODE_IDEV_HASHSIZE; i++)
	vnode_idev_chain [i] = NULL;


    /*
     *	Create the buffer cache linked list hashtable:
     */

    bcache_chain = (struct bcache_entry **)
		malloc (BCACHE_HASHSIZE * sizeof(struct bcache_entry *));
    if (!bcache_chain)
	panic ("vfs_init(): could not malloc bcache_chain[]");

    for (i=0; i<BCACHE_HASHSIZE; i++)
	bcache_chain [i] = NULL;


    /*
     *	There is nothing to flush to disk yet, but this will cause
     *	vfs_bcacheflush() to set up a timer to call itself every now
     *	and then.
     */

    vfs_bcacheflush ();
  }



struct filesystem *vfs_register (char *fstype, char *lockvalue)
  {
    /*
     *	Try to register a filesystem driver.
     *	Returns a pointer to a filesystem on success, NULL on failure.
     *
     *	It is up to the caller to initialize the data in the filesystem
     *	struct, and then unlock() it.
     */

    struct filesystem *tmp, *tmp2;

    tmp = (struct filesystem *) malloc (sizeof(struct filesystem));
    if (!tmp)
	return NULL;

    memset (tmp, 0, sizeof(struct filesystem));

    tmp->name = fstype;

    /*  Add tmp last in the chain:  */
    tmp2 = firstfilesystem;
    if (!tmp2)
      firstfilesystem = tmp;
    else
      {
	while (tmp2->next)
	  tmp2 = tmp2->next;
	tmp2 -> next = tmp;
      }

    if (lock (&tmp->lock, lockvalue, LOCK_NONBLOCKING | LOCK_RW) < 0)
      printk ("vfs_register: could not lock ???");

    return tmp;
  }


