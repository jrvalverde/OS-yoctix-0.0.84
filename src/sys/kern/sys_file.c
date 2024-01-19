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
 *  kern/sys_file.c  --  File related syscalls
 *
 *	sys_access ()
 *		Get accessability for a filename.
 *
 *	sys_chdir ()
 *
 *	sys_chroot ()
 *
 *	sys_stat ()
 *		Stat a filename.
 *
 *	sys_fstatfs ()
 *
 *	sys_umask ()
 *
 *	sys_getdirentries ()
 *
 *	sys_mount ()  --  TODO: move somewhere else?
 *
 *
 *  History:
 *	20 Jul 2000	sys_stat(), empty sys_access()
 *	26 Jul 2000	sys_chdir(), sys_chroot()
 *	17 Sep 2000	sys_umask()
 *	7 Nov 2000	sys_fstatfs() skeleton, sys_getdirentries()
 *	8 Jan 2001	sys_mount()
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/syscalls.h>
#include <sys/interrupts.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/vm.h>
#include <sys/stat.h>
#include <sys/mount.h>



extern size_t userland_startaddr;



int sys_access (ret_t *res, struct proc *p, char *name, int mode)
  {
    /*
     *	sys_access ()
     *	-------------
     *
     *	TODO:  this function is not yet implemented
     */

    size_t slen, blen;

printk ("sys_access: not yet implemented");

    if (!p->sc_params_ok)
      {
	blen = vm_prot_maxmemrange (p, name, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, name, blen);

	p->sc_params_ok = 1;
      }


    *res = 0;
    return 0;
  }



int sys_chdir (ret_t *res, struct proc *p, char *name)
  {
    /*
     *	sys_chdir ()
     *	------------
     *
     *	Find the new directory 'name' using vnode_lookup(). Set
     *	p->currentdir_vnode to the new directory's vnode. The refcount of
     *	the new dir should be increased, and the refcount of the old dir
     *	should de decreased.
     */

    struct vnode *v;
    int e;
    size_t slen, blen;

    if (!res || !p || !name)
	return EINVAL;

    if (!p->sc_params_ok)
      {
	blen = vm_prot_maxmemrange (p, name, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, name, blen);
	if (slen==0)
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    name += userland_startaddr;

    v = vnode_lookup (p, name, &e, "sys_chdir");
    if (!v)
      return e;

    if (!S_ISDIR(v->ss.st_mode))
      {
	unlock (&v->lock);
	return ENOTDIR;
      }

    if (p->currentdir_vnode && p->currentdir_vnode!=v)
      {
	lock (&p->currentdir_vnode->lock, "sys_chdir", LOCK_BLOCKING | LOCK_RW);
	p->currentdir_vnode->refcount --;
	unlock (&p->currentdir_vnode->lock);
      }

    p->currentdir_vnode = v;
    v->refcount ++;
    unlock (&v->lock);
    return 0;
  }



int sys_chroot (ret_t *res, struct proc *p, char *name)
  {
    /*
     *	sys_chroot ()
     *	-------------
     *
     *	Find the directory 'name' using vnode_lookup(). Set
     *	p->rootdir_vnode to the directory's vnode. The refcount of the
     *	new rootdir should be increased, and the refcount of the old
     *	rootdir should de decreased.
     */

    struct vnode *v;
    int e;
    size_t slen, blen;

    if (!res || !p || !name)
	return EINVAL;

    if (!p->sc_params_ok)
      {
	blen = vm_prot_maxmemrange (p, name, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, name, blen);
	if (slen==0)
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    name += userland_startaddr;
printk ("sys_chroot (%s)", name);

    v = vnode_lookup (p, name, &e, "sys_chroot");
    if (!v)
      return e;

    if (!S_ISDIR(v->ss.st_mode))
      {
	unlock (&v->lock);
	return EINVAL;
      }

    if (p->rootdir_vnode && p->rootdir_vnode!=v)
      {
	lock (&p->rootdir_vnode->lock, "sys_chroot", LOCK_BLOCKING | LOCK_RW);
	p->rootdir_vnode->refcount --;
	unlock (&p->rootdir_vnode->lock);
      }

    p->rootdir_vnode = v;
    v->refcount ++;
    unlock (&v->lock);
    return 0;
  }



int sys_stat (ret_t *res, struct proc *p, char *name, struct stat *ss)
  {
    /*
     *	sys_stat ()
     *	-----------
     *
     *	Stat a file in the virtual filesystem. Returns errno.
     */

/*     struct mountinstance *mi; */
    size_t slen, blen;
    struct vnode *v;
    int e;

    if (!res || !p)
	return EINVAL;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, ss, sizeof(struct stat),
				VMREGION_WRITABLE))
	  return EFAULT;

	blen = vm_prot_maxmemrange (p, name, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, name, blen);
	if (slen==0)
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    name += userland_startaddr;
    ss = (struct stat *) ((byte *)ss + userland_startaddr);
    *res = 0;

/*
    return vfs_stat (p, name, &mi, ss);
*/


    v = vnode_lookup (p, name, &e, "sys_stat");
    if (!v)
      return e;

    *ss = v->ss;
    unlock (&v->lock);

    return 0;
  }



int sys_fstatfs (ret_t *res, struct proc *p, int fd, struct statfs *ss)
  {
    /*
     *	sys_fstatfs ()
     *	--------------
     *
     *	TODO:  this is not very complete yet...
     */

    struct mountinstance *mi;

    if (!res || !p)
	return EINVAL;

    if (fd<0 || fd>=p->nr_of_fdesc)
	return EBADF;

    if (!p->fdesc[fd])
	return EBADF;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, ss, sizeof(struct stat),
				VMREGION_WRITABLE))
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    ss = (struct statfs *) ((byte *)ss + userland_startaddr);
    memset (ss, 0, sizeof(struct statfs));

    mi = p->fdesc[fd]->v->mi;

    lock (&mi->lock, "sys_fstatfs", LOCK_BLOCKING | LOCK_RO);

    ss->f_flags   = mi->flags;
    ss->f_bsize   = mi->device->bsize;
    ss->f_iosize  = mi->device->bsize;	/*  TODO  */
    ss->f_blocks  = mi->superblock->nr_of_blocks;
    ss->f_bfree   = 0;
    ss->f_bavail  = 0;
    ss->f_files   = 0;
    ss->f_ffree   = 0;
    strlcpy (ss->f_fstypename, (char *)mi->fs->name, MFSNAMELEN);
    strlcpy (ss->f_mntonname, mi->mount_point, MNAMELEN);
    strlcpy (ss->f_mntfromname, mi->device_name, MNAMELEN);

    unlock (&mi->lock);

    return 0;
  }



int sys_umask (ret_t *res, struct proc *p, int newmask)
  {
    /*
     *	sys_umask ()
     *	------------
     *
     *	Set the process' umask to newmask, and return the old value.
     *	TODO: newmask should be mode_t... but is this neccessary?
     */

    mode_t oldv;

    if (!res || !p)
	return EINVAL;

    oldv = p->umask;
    p->umask = newmask & ACCESSPERMS;
    return oldv;
  }



int sys_getdirentries (ret_t *res, struct proc *p, int fd, char *buf,
	int nbytes, long *basep)
  {
    /*
     *	sys_getdirentries ()
     *	--------------------
     */

    int erres = 0;
    off_t offtres = 0;
    off_t curofs;
    struct vnode *v;
    int setbasep = 0;

    if (!res || !p)
	return EINVAL;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, buf, nbytes, VMREGION_WRITABLE))
	  return EFAULT;

	if (basep)
	  {
	    if (!vm_prot_accessible (p, basep, sizeof(long),
		VMREGION_WRITABLE | VMREGION_READABLE))
	      return EFAULT;
	  }

	p->sc_params_ok = 1;
      }

    if (fd<0 || fd>=p->nr_of_fdesc)
      return EBADF;

    if (!p->fdesc[fd])
      return EBADF;

    v = p->fdesc[fd]->v;
    if (!v)
      return EINVAL;
    if (!v->mi)
      return EINVAL;
    if (!v->mi->fs)
      return EINVAL;

    if (!S_ISDIR(v->ss.st_mode))
      return EINVAL;

    buf = (char *) ((byte *)buf + userland_startaddr);

    lock (&p->fdesc[fd]->lock, "sys_getdirentries", LOCK_BLOCKING | LOCK_RW);
    if (basep)
      {
	setbasep = 1;
	basep = (long *) ((byte *)basep + userland_startaddr);
	p->fdesc[fd]->cur_ofs = *basep;
      }

    curofs = p->fdesc[fd]->cur_ofs;
    unlock (&p->fdesc[fd]->lock);


    /*
     *	Semi-ugly hack:  the size of "virtual" directories may be zero, but
     *	there can still be directory entries in them. Physical directories
     *	always have a non-zero size.  (TODO: fix this some other way ?)
     *
     *	Anyway, this is a check to see if we're beyond the end of the
     *	directory. If so, we decrease the size of the buffer to only include
     *	what's left of the directory.
     */

    if (v->ss.st_size)
      {
	if (nbytes > v->ss.st_size - curofs)
	    nbytes = v->ss.st_size - curofs;

	if (nbytes<=0)
	  {
	    *res = 0;
	    return 0;
	  }
      }


    /*
     *	Use the get_direntries() function of the specific filesystem on
     *	which the vnode is located:
     */

    if (!v->mi)
      return EINVAL;
    if (!v->mi->fs)
      return EINVAL;

    if (v->mi->fs->get_direntries)
      {
	erres = v->mi->fs->get_direntries (p, v, curofs,
		buf, (size_t)nbytes, &offtres);

	lock (&p->fdesc[fd]->lock, "sys_getdirentries",
		LOCK_BLOCKING | LOCK_RW);

	if (erres==0)
	    p->fdesc[fd]->cur_ofs += offtres;

	if (setbasep)
	  *basep = p->fdesc[fd]->cur_ofs;

	unlock (&p->fdesc[fd]->lock);

	*res = offtres;
	return erres;
      }
    else
      printk ("sys_getdirentries: filesystem has no get_direntries function");

    return EINVAL;
  }



int sys_readlink (ret_t *res, struct proc *p, char *path, char *buf,
	size_t bufsize)
  {
    /*
     *	sys_readlink ()
     *	---------------
     *
     *	Return contents of a symbolic link.
     */

    size_t slen, blen;
    size_t realsize;
    int e;
    struct vnode *v;
    inode_t i;
    struct mountinstance *mi;

    if (!res || !p)
      return EINVAL;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, buf, bufsize, VMREGION_WRITABLE))
	  return EFAULT;

	blen = vm_prot_maxmemrange (p, path, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, path, blen);
	if (slen==0)
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    buf = (char *) ((char *)buf + userland_startaddr);
    path = (char *) ((char *)path + userland_startaddr);

    v = vnode_lookup (p, path, &e, "sys_readlink");
    if (!v)
      return ENOENT;

    i = v->ss.st_ino;
    mi = v->mi;
    unlock (&v->lock);

    if (!mi)
      return EINVAL;
    if (!mi->fs)
      return EINVAL;
    if (!mi->fs->readlink)
      return EOPNOTSUPP;

    e = mi->fs->readlink (mi, (inode_t)i, buf, (size_t)bufsize, p, &realsize);
    *res = realsize;

    return e;
  }



int sys_mount (ret_t *res, struct proc *p,
	char *type, char *dir, int flags, void *data)
  {
    /*
     *	sys_mount ()
     *	------------
     *
     *	Mount a filesystem object into the current filesystem tree.
     */

    size_t slen, blen, datasize;
    int vfs_mount_flags, f;
    char *devname;

    if (!res || !p)
      return EINVAL;

    if (!p->sc_params_ok)
      {
	blen = vm_prot_maxmemrange (p, type, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, type, blen);
	if (slen==0)
	  return EFAULT;

	blen = vm_prot_maxmemrange (p, dir, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, dir, blen);
	if (slen==0)
	  return EFAULT;

	/*  NOTE: p->sc_params_ok is still 0, since data has to
		be checked too...  */
      }

    type = (char *) ((char *)type + userland_startaddr);
    dir  = (char *) ((char *)dir + userland_startaddr);

    /*  Different types have different datasize:  */
    /*  TODO:  */
    if (!strcmp (type, "ffs"))
      datasize = sizeof (struct ufs_args);
    else
      {
#if DEBUGLEVEL>=2
	printk ("sys_mount: unknown filesystem type '%s'", type);
#endif
	return EOPNOTSUPP;
      }

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, data, datasize, VMREGION_READABLE))
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    devname = NULL;
    if (!strcmp (type, "ffs"))
      {
	devname = ((struct ufs_args *)data) -> fspec;

	blen = vm_prot_maxmemrange (p, devname, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, devname, blen);
	if (slen==0)
	  return EFAULT;

	devname = (char *) ((char *)devname + userland_startaddr);
      }

    /*  Handle the flags too:  */
    f = flags;
    vfs_mount_flags = 0;
    /*  TODO: VFSMOUNT_NODEV, VFSMOUNT_RW, VFSMOUNT_FILE, VFSMOUNT_ASYNC  */

#if DEBUGLEVEL>=2
    if (f)
      printk ("sys_mount: unrecognized flags 0x%x for type '%s', dir '%s'",
		flags, type, dir);
#endif

    return vfs_mount (p, devname, dir, type, vfs_mount_flags);
  }

