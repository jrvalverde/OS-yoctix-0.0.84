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
 *  sys/vm.h  --  the virtual file system
 */

#ifndef	__SYS__VFS_H
#define	__SYS__VFS_H


#include <sys/defs.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/device.h>

struct proc;


struct superblock
      {
	u_int32_t	blocksize;		/*  blocksize (512, 8192, etc)  */
	daddr_t		nr_of_blocks;		/*  nr of filesystem (data) blocks  */
	daddr_t		root_inode;		/*  inode number of the root directory  */
	void		*fs_superblock;		/*  fs dependant superblock info  */
      };


struct filesystem
      {
	struct lockstruct lock;

	/*  Pointer to the next filesystem:  */
	struct filesystem *next;

	ref_t		refcount;

	/*
	 *  NOLOCK:
	 *
	 *  The following parts of the filesystem struct should never need to
	 *  be changed during the filesystem's lifetime, so the lock variable
	 *  doesn't need to be used when reading these:
	 */

	/*  Filesystem name:  */
	char		*name;

	/*  Superblock functions:  */
	int		(*read_superblock) ( struct mountinstance *, struct proc *);
	int		(*write_superblock) ( struct mountinstance *, struct proc *);

	/*  Read and write from files:  */
	int		(*read) (struct vnode *, off_t, byte *, off_t, off_t *, struct proc *);
					/*  vnode, offset, buffer, length, &transfered  */
	int		(*write)();	/*  vnode, offset, buffer, length, &transfered  */

	/*  Directory handling functions:  */
	int		(*get_direntries) (struct proc *, struct vnode *, off_t, byte *, size_t, off_t *);
	int		(*namei)();

	/*  Stat a name in a directory, or stat a file using its inode:  */
	/*  (msdosfs for example only supports the first of these)  */
	int		(*namestat) (struct mountinstance *, inode_t, char *, struct stat *, struct proc *);
	int		(*istat) (struct mountinstance *, inode_t, struct stat *, struct proc *);
	int		(*readlink) (struct mountinstance *, inode_t, char *, size_t, struct proc *, size_t *);
      };


struct mountinstance
      {
	struct lockstruct lock;

	struct mountinstance *next;
	struct superblock *superblock;
	u_int32_t	flags;
	ref_t		refcount;

	/*
	 *  NOLOCK:
	 *
	 *  The following parts of the mountinstance struct should never
	 *  need to be changed during the mountinstance's lifetime, so
	 *  the lock variable doesn't need to be used when reading these:
	 */

	struct filesystem *fs;

	/*  The source (device or file) of this mountinstance:  */
	char		*device_name;	/*  device or file name:  "fd1"  */
	struct device	*device;	/*  NULL if source is a file  */

	/*  Where is this mountinstance mounted at?  */
	char		*mount_point;	/*  mount point as a string:  "/home"  */
	struct mountinstance *parent;	/*  ptr to parent mountinstance. NULL if none  */
	inode_t		parent_inode;	/*  inode of parent directory  */
	struct vnode	*mounted_at;	/*  vnode IN THE PARENT where we're mounted (note:
					    this is not the vnode describing the parent dir)  */
      };

/*  where flags can be a combination of:  */

#define	VFSMOUNT_NODEV		1
#define	VFSMOUNT_RW		2
#define	VFSMOUNT_FILE		4
#define	VFSMOUNT_ASYNC		8



#define	BCACHE_HASHSIZE		512

struct bcache_entry
      {
	struct lockstruct	lock;

	struct bcache_entry	*next;

	time_t			last_written;
	int			status;

	/*
	 *  NOLOCK:
	 *  The following parts of the bcache_entry struct should never
	 *  need to be changed during the bcache_entry's lifetime, so
	 *  the lock variable doesn't need to be used when reading these:
	 */

	/*  Hash of mi and blocknr:  */
	hash_t			hash;

	/*  "Physical" location:  */
	struct mountinstance	*mi;
	daddr_t			blocknr;

	/*  Ptr to buffer in memory:  */
	byte			*bufferptr;

	/*  Note: the size of the buffer should be mi->superblock->blocksize  */
      };

/*  where status contains the following bits:  */
#define	BCACHE_DIRTY		1


void vfs_init ();
struct filesystem *vfs_register (char *fstype, char *lockvalue);

int vfs_mount (struct proc *p, char *devname, char *mountpoint, char *fstype, u_int32_t flags);
void vfs_dumpmountinstances();

int vfs_stat (struct proc *p, char *fname, struct mountinstance **mi, struct stat *ss);

int vfs_namei (struct proc *p, char *fname, inode_t *inode, struct mountinstance **mi);

void vfs_bcacheflush ();
int block_read (struct mountinstance *mi, daddr_t blocknr, daddr_t nrofblocks, void *buf, struct proc *p);
int block_write (struct mountinstance *mi, daddr_t blocknr, daddr_t nrofblocks, void *buf, struct proc *p);

struct vnode *vnode_create (struct proc *p, char *filename, int *errno,
	char *lockaddr, struct stat *ss, struct mountinstance *mi,
	hash_t hash_n, hash_t hindex_n, hash_t hash_idev, hash_t hindex_idev);
int vnode_remove (struct vnode *v);
struct vnode *vnode_lookup (struct proc *p, char *filename1, int *errno, char *lockaddr);
void *vnode_pagein ();
int vnode_close (struct vnode *v, struct proc *p);
struct vnode *vnode_create_rootvnode (struct mountinstance *mi, struct proc *p, int *errno, char *lockaddr);

#define	VNODE_CREATE		1
#define	VNODE_DONT_CREATE	0



/*
 *  statfs struct:  NOTE! TODO! This should
 *	     1)  be moved away from here
 *	and  2)  be made machine independant... it should be up to emulation
 *		 code to translate into specific word-length etc!
 */



typedef struct
      {
	s_int32_t	val[2];
      }	fsid_t;


#define	MFSNAMELEN	16
#define	MNAMELEN	32

struct statfs
      {
	u_int32_t	f_flags;	/*  mount flags  */
	s_int32_t	f_bsize;	/*  file system fundamental i/o block size  */
	u_int32_t	f_iosize;	/*  optimal transfer block size  */
	u_int32_t	f_blocks;	/*  total blocks in fs  */
	u_int32_t	f_bfree;	/*  free blocks in fs  */
	s_int32_t	f_bavail;	/*  free blocks avail to non-superuser  */
	u_int32_t	f_files;	/*  total file nodes in fs  */
	u_int32_t	f_ffree;	/*  free file nodes in fs  */
	fsid_t		f_fsid;		/*  file system id  */
	u_int32_t	f_owner;	/*  user that mounted the file system  */ /*  NOTE: should be uid_t  */
	u_int32_t	f_syncwrites;	/*  count of sync writes since mount  */
	u_int32_t	f_asyncwrites;	/*  count of async writes since mount  */
	u_int32_t	f_spare[4];	/*  spare for later  */
	char		f_fstypename[MFSNAMELEN];	/*  fs type name  */
	char		f_mntonname[MNAMELEN];		/*  directory on which mounted  */
	char		f_mntfromname[MNAMELEN];	/*  mounted file system  */
/*	union mount_info mount_info;         per-filesystem mount options
*/
      };


#endif	/*  __SYS__VFS_H  */

