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
 *  sys/vnode.h  --  vnode struct
 *
 *	A vnode (in memory) represents a file in the filesystem. A file
 *	in the filesystem is never represented by two different vnodes
 *	at the same time.
 *
 *	The vnode is then refered to by filedescriptors. Each time a process
 *	opens the file described by the vnode, the vnode's refcount is
 *	increased. A vnode whose refcount is zero may be freed from memory.
 *
 *	Each vnode can have several names. During a lookup, these names are
 *	tried first. If there is no match, a manual stat is done and we have
 *	to search for a vnode with the correct inode/device hash.
 */

#ifndef __SYS__VNODE_H
#define	__SYS__VNODE_H

#include <sys/defs.h>
#include <sys/stat.h>
#include <sys/proc.h>


struct vm_object;


/*
 *  The vnode hash tables:
 *
 *  The NAME hash:
 *  A hashing function calculates a value based on the full filename of a
 *  file. This value is then ANDed by (VNODE_HASHSIZE-1) to get the index
 *  in vnode_chain[] which points to the linked vnode chain where the
 *  vnode should be. (The VNODE_HASHSIZE number must be a power of 2.)
 *
 *  The IDEV hash:
 *  This is used to find a vnode based on its inode and device. Works the
 *  same as the name hash.
 *
 *  Note that the name and idev hashes are two completely different things.
 */

#define	VNODE_NAME_HASHSIZE	256
#define	VNODE_IDEV_HASHSIZE	256



/*
 *  Vnode name entry:
 */

struct vnodename
      {
	struct vnodename *prev;
	struct vnodename *next;

	hash_t		hash;
	char		*name;
	struct vnode	*v;
      };



/*
 *  The vnode struct
 *  ----------------
 *
 *  refcount_exec and refcount_write are used to make sure that noone can
 *  execute a file which is currently being written to, and noone can write
 *  to a file (actually open a file for writing) if the vnode is being used
 *  by a the executable region of a process.
 */

struct vnode
      {
	/*
	 *  This lock should only be used when accessing parts of the
	 *  vnode struct that can be changed, static values don't
	 *  require a lock:
	 */

	struct lockstruct lock;

	struct vnode	*next;
	struct vnode	*prev;

	/*  vm_object pointing to this vnode, if any:  */
	struct vm_object *vmobj;

	/*  _ONE_ of the file's filenames:  (TODO: this is ugly)  */
	struct vnodename *vname;

	/*  Refcounts for any access, exec access, and write access:  */
	ref_t	refcount;
	ref_t	refcount_exec;
	ref_t	refcount_write;

	/*  Status bits (dirty etc.):  */
	u_int32_t	status;

	/*  Time when the vnode became dirty:  */
	time_t		dirty_time;

	/*  stat struct (inode data):  */
	struct stat	ss;

	/*
	 *  If another mountinstance is mounted with its root overlapping
	 *  this vnode, then this is a pointer to that mountinstance.
	 *  (Otherwise NULL.)
	 */
	struct mountinstance *mounted;


	/*
	 *  NOLOCK:
	 *
	 *  The following parts of the vnode struct should never need to
	 *  be changed during the vnode's lifetime, so the lock variable
	 *  doesn't need to be used when reading these:
	 */


	/*  Ptr to the mountinstance that this vnode resides on:  */
	struct mountinstance *mi;

	/*  Ptr to function handlers:  */
	int	(*open)(struct device *, struct proc *);
			/*  device:   dev, p    file:  (unused)  */
	int	(*close)(struct device *, struct proc *);
			/*  device:   dev, p    file:  (unused)  */
	int	(*read)();	/*	(int *, struct device *, byte *, off_t, struct proc *); */
			/*  chardev:  *res, dev, buffer, buflen, p  */
			/*  blockdev: ?  */
			/*  file:     vnode, offset, buffer, len, &transfered  */
	int	(*write)();	/*  chardev:  *res, dev, buffer, buflen, p  */
			/*  blockdev: ?  */
			/*  file:     vnode, offset, buffer, len, &transfered  */
	int	(*ioctl)(struct device *, int *, struct proc *, ...);
			/*  chardev:  dev, *res, p, (unsigned long)req, ...  */
	int	(*lseek)();
      };


/*
 *  vnode status bits
 */

#define	VNODE_DIRTY		1	/*  should be written to disk  */
#define	VNODE_TO_BE_REMOVED	2	/*  used during clean-up  */
#define	VNODE_EXCLUSIVE		4	/*  opened excl. by a process  */


#endif	/*  __SYS__VNODE_H  */
