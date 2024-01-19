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
 *  kern/sys_fd.c  --  Filedescriptor related syscalls
 *
 *	sys_open ()
 *		Open a file, return a descriptor number.
 *
 *	sys_close ()
 *		Close a file descriptor.
 *
 *	sys_read ()
 *		Read from a file descriptor.
 *
 *	sys_write ()
 *		Write to a file descriptor.
 *
 *	sys_writev ()
 *		Write several blocks of data to a file descriptor.
 *
 *	sys_ioctl ()
 *		Perform I/O control stuff on a descriptor.
 *
 *	sys_fstat ()
 *		Stat a file descriptor.
 *
 *	sys_dup2 ()
 *		Duplicate a file descriptor.
 *
 *	sys_fchdir ()
 *		Change the current directory to an open file descriptor.
 *
 *	sys_lseek ()
 *		Move a file descriptor's read/write pointer.
 *
 *	sys_fcntl ()
 *		File [descriptor] control.
 *
 *  History:
 *	8 Mar 2000	first version, sys_write(), sys_ioctl()
 *	25 Mar 2000	adding sys_read() 
 *	2 Jun 2000	adding sys_open(), sys_close()
 *	3 Jun 2000	adding sys_fstat()
 *	7 Jun 2000	sys_open() locks vnode
 *	15 Jun 2000	adding sys_dup2()
 *	26 Jul 2000	sys_fchdir()
 *	27 Jul 2000	sys_lseek()
 *	28 Jul 2000	sys_fcntl()
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
#include <sys/emul.h>
#include <fcntl.h>



extern size_t userland_startaddr;



int sys_open (ret_t *res, struct proc *p, char *filename, int flags,
		mode_t mode)
  {
    /*
     *	sys_open ()
     *	-----------
     *
     *	Try to look up the vnode using "filename". If this is a device, then
     *	we try to open it using its open() function. Regular files don't have
     *	to be opened to be accessed.
     *
     *	If everything was successful, we allocate memory for a file descriptor
     *	which points to the vnode.
     */

    struct vnode *v;
    struct fdesc *fd;
    int err, i, found;
    int rcwrite_increased=0;
    size_t slen, blen;

    if (!res || !p)
	return EINVAL;

    if (!p->sc_params_ok)
      {
	blen = vm_prot_maxmemrange (p, filename, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, filename, blen);
	if (slen==0)
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    filename += userland_startaddr;

#if DEBUGLEVEL>=4
    printk ("sys_open: '%s' flags=%x mode=%x", filename, flags, mode);
#endif

    v = vnode_lookup (p, filename, &err, "sys_open");
    if (!v)
	return err;

/*  TODO: check file access permission stuff  */

    /*  Allocate a filedescriptor struct and point it to the vnode:  */
    fd = fd_alloc (FDESC_TYPE_FILE);
    if (!fd)
      {
	unlock (&v->lock);
	return ENOMEM;
      }

    /*
     *	NOTE: The fd isn't locked, since noone else is using it yet and
     *	therefore no race condition can exist.
     */

    /*  Take care of flags:  */

    fd->mode = 0;

    switch ((flags & O_ACCMODE))
      {
	case O_RDONLY:
		fd->mode |= FDESC_MODE_READ;
		break;
	case O_WRONLY:
		fd->mode |= FDESC_MODE_WRITE;
		break;
	case O_RDWR:
		fd->mode |= FDESC_MODE_READ;
		fd->mode |= FDESC_MODE_WRITE;
		break;
	default:
		unlock (&v->lock);
		free (fd);
#if DEBUGLEVEL>=3
		printk ("sys_open(): invalid flags=%x", flags);
#endif
		return EINVAL;
      }

    flags &= ~O_ACCMODE;

    if (flags & O_NONBLOCK)
      {
	flags &= ~O_NONBLOCK;
	fd->mode |= FDESC_MODE_NONBLOCK;
      }

    /*  Unknown flags remaining?  */
    if (flags)
      {
	unlock (&v->lock);
	free (fd);
#if DEBUGLEVEL>=3
	printk ("sys_open(): unknown flags=%x", flags);
#endif
	return EINVAL;
      }

    if (fd->mode & FDESC_MODE_WRITE)
      {
	/*  We cannot write to a vnode which someone is executing:  */
	if (v->refcount_exec > 0)
	  {
	    unlock (&v->lock);
	    free (fd);
	    return ETXTBSY;
	  }

	rcwrite_increased = 1;
	v->refcount_write ++;
      }

    /*  Find a free fdesc slot in the process fdesc array:  */
    found = p->first_unused_fdesc;
    if (found<0 || found>=p->nr_of_fdesc)
	panic ("sys_open: invalid first_unused_fdesc in pid %i", p->pid);

    if (p->fdesc[found])
      {
	i=0; found=0;
	while (p->fdesc[i] && i<p->nr_of_fdesc) i++;
	if (i>=p->nr_of_fdesc)
	  {
	    printk ("sys_open: no free slots in pid %i. TODO", p->pid);
	    if (rcwrite_increased)
	      v->refcount_write --;
	    free (fd);
	    unlock (&v->lock);
	    return EMFILE;
	  }
	found = i;
      }

    fd->v = v;
    v->refcount ++;

    if (v->open)
      {
	err = v->open (v->ss.st_rdev, p);
	if (err)
	  {
	    if (rcwrite_increased)
	      v->refcount_write --;
	    v->refcount --;
	    free (fd);
	    unlock (&v->lock);
	    return err;
	  }
      }

    unlock (&v->lock);

    fd->refcount ++;
    p->fdesc[found] = fd;
    p->fdesc[found]->sflag = 0;
    p->fcntl_dflag[found] = 0;	/*  will remain open on exec  */
    p->first_unused_fdesc = (found+1) % p->nr_of_fdesc;

    *res = found;
    return 0;
  }



int sys_close (ret_t *res, struct proc *p, int d)
  {
    /*
     *	sys_close ()
     *	------------
     *
     *	Call fd_detach() to remove the file descriptor from the process' file
     *	descriptor array.
     */

    if (!res || !p)
	return EINVAL;

    return fd_detach (p, d);
  }



int sys_read (ret_t *res, struct proc *p, int d, void *buf, size_t len)
  {
    /*
     *	sys_read ()
     *	-----------
     *
     *	Check if descriptor 'd' is opened for reading.
     *	Check address (buf) and length (len).
     *
     *	Return value:	res should be set to the actual number of bytes read.
     */

    struct vnode *v;
    off_t offtres, curofs;
    int erres;

    if (!res || !p)
	return EINVAL;

    if (d<0 || d>=p->nr_of_fdesc)
	return EBADF;

    if (!p->fdesc[d] || !(p->fdesc[d]->mode & FDESC_MODE_READ))
	return EBADF;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, buf, len, VMREGION_WRITABLE))
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    buf = (void *) ((byte *)buf + userland_startaddr);

    if (p->fdesc[d]->type == FDESC_TYPE_FILE)
      {
	v = p->fdesc[d]->v;
	if (!v)
	  panic ("sys_read from a file without a vnode, pid=%i, fd=%i",
		p->pid, d);

	/*  1: Is this a regular file?  */
	if (S_ISREG(v->ss.st_mode))
	  {
#if DEBUGLEVEL>=4
	    printk ("  (read %i, ofs=%i, len=%i, st_size=%i)",
		d, (int)p->fdesc[d]->cur_ofs, (int)len,
		(int)v->ss.st_size);
#endif
	    if (!v->read)
	      return EINVAL;

	    /*  cur_ofs requires a lock:  */
	    lock (&p->fdesc[d]->lock, "sys_read", LOCK_BLOCKING | LOCK_RO);
	    curofs = p->fdesc[d]->cur_ofs;
	    unlock (&p->fdesc[d]->lock);

	    if (len > v->ss.st_size - curofs)
		len = v->ss.st_size - curofs;
	    if (len>0)
	      erres = v->read (v, (off_t) curofs, buf, (off_t) len, &offtres);
	    else
	      {
		offtres = 0;
		erres = 0;
	      }

	    if (erres==0)
		curofs += offtres;

	    lock (&p->fdesc[d]->lock, "sys_read", LOCK_BLOCKING | LOCK_RW);
	    p->fdesc[d]->cur_ofs = curofs;
	    unlock (&p->fdesc[d]->lock);

	    *res = offtres;
	    return erres;
	  }

	/*  2:  Character special file?  */
	if (S_ISCHR(v->ss.st_mode))
	  {
	    if (!v->read)
	      return EINVAL;

	    erres = v->read (&offtres, v->ss.st_rdev, buf, (off_t) len, p);

	    *res = offtres;
	    return erres;
	  }

	/*  3:  Block special file?  */
	if (S_ISBLK(v->ss.st_mode))
	  {
	    if (!v->read)
	      return EINVAL;
	    printk ("sys_read: from _block_ file %i:", d);
	  }
      }
    else
      {
	/*  Not a vnode...  (TODO)  */
	printk ("sys_read from NOT A VNODE %i: (TODO)", d);
      }

    printk ("sys_read: failed (fd=%i)", d);

    *res = len;
    return EINVAL;
  }



int sys_write (ret_t *res, struct proc *p, int d, void *buf, size_t len)
  {
    /*
     *	sys_write ()
     *	------------
     *
     *	Check if descriptor 'd' is opened for writing.
     *	Check address (buf) and length (len).
     *
     *	Return value:	res should be set to the actual number of
     *			bytes written.
     *	TODO: write to a setuid file should clear the setuid flag
     *		if p->uid!=0 (security reasons)
     */

    struct vnode *v;
    off_t offtres, curofs;
    int erres;

    if (!res || !p)
	return EINVAL;

    if (d<0 || d>=p->nr_of_fdesc)
	return EBADF;

    if (!(p->fdesc[d]) || !(p->fdesc[d]->mode & FDESC_MODE_WRITE))
	return EBADF;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, buf, len, VMREGION_READABLE))
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    buf = (void *) ((byte *)buf + userland_startaddr);

    v = p->fdesc[d]->v;
    if (v)
      {
	/*  1: Is this a regular file?  */
	if (S_ISREG(v->ss.st_mode))
	  {
	    printk ("sys_write to regular file %i: buf='%s'", d, buf);

	    /*  TODO:  make file larger...  truncate up  */

	    if (!v->write)
	      return EINVAL;

	    /*  cur_ofs requires a lock:  */
	    lock (&p->fdesc[d]->lock, "sys_write", LOCK_BLOCKING | LOCK_RO);
	    curofs = p->fdesc[d]->cur_ofs;
	    unlock (&p->fdesc[d]->lock);

	    if (len > v->ss.st_size - curofs)
		len = v->ss.st_size - curofs;
	    if (len>0)
	      erres = v->write (v, (off_t)curofs, buf, (off_t)len, &offtres);
	    else
	      {
		offtres = 0;
		erres = 0;
	      }

	    if (erres==0)
		curofs += offtres;

	    lock (&p->fdesc[d]->lock, "sys_write", LOCK_BLOCKING | LOCK_RW);
	    p->fdesc[d]->cur_ofs = curofs;
	    unlock (&p->fdesc[d]->lock);

	    *res = offtres;
	    return erres;
	  }

	/*  2:  Character special file?  */
	if (S_ISCHR(v->ss.st_mode))
	  {
	    if (!v->write)
	      return EINVAL;

	    erres = v->write (&offtres, v->ss.st_rdev, buf, (off_t) len);
	    *res = offtres;
	    return erres;
	  }

	/*  3:  Block special file?  */
	if (S_ISBLK(v->ss.st_mode))
	  {
/*	    if (!v->write)
	      return EINVAL;
*/
printk ("sys_write to _block_ file %i: buf='%s'", d, buf);
	  }
      }
    else
      {
	/*  Not a vnode...  (TODO)  */
printk ("sys_write to NOT A VNODE %i: (TODO) buf='%s'", d, buf);
      }

printk ("sys_write: failed (fd=%i)", d);

    *res = len;
    return 0;
  }



int sys_writev (ret_t *res, struct proc *p, int d,
		struct iovec *iov, int iovcnt)
  {
    /*
     *	sys_writev ()
     *	-------------
     *
     *	Write several buffers at once, according to 'iov'. We only continue
     *	to the next iovec entry if the one before succeeded. sys_write() is
     *	called for each iovec entry.
     */

    int i, r1;
    size_t total_written = 0;
    void *ptr;
    ret_t r2;

    if (!res || !p || iovcnt<0)
	return EINVAL;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, iov, sizeof(struct iovec) * iovcnt,
		VMREGION_READABLE))
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    iov = (struct iovec *) ((byte *)iov + userland_startaddr);

    for (i=0; i<iovcnt; i++)
      {
	ptr = iov[i].iov_base;
	r1 = sys_write (&r2, p, d, ptr, iov[i].iov_len);
	total_written += r2;
	*res = total_written;
	if (r1)
	  return r1;
      }

    return 0;
  }



int sys_ioctl (ret_t *res, struct proc *p, int d, unsigned long req, unsigned long arg1)
  {
    /*
     *	sys_ioctl ()
     *	------------
     *
     *	If the vnode associated with descriptor d has an ioctl() function,
     *	then we call it. Otherwise: return EINVAL.
     */

    struct vnode *v = NULL;
    int r, tmpres = 0;

    if (!res || !p)
	return EINVAL;

    if (d<0 || d>=p->nr_of_fdesc)
	return EBADF;

    if (!(p->fdesc[d]))
	return EBADF;

    v = p->fdesc[d]->v;

    if (v->ioctl)
      {
	/*  Character device?  */
	if (S_ISCHR(v->ss.st_mode))
	  {
	    r = v->ioctl (v->ss.st_rdev, &tmpres, p, (unsigned long)req, (unsigned long)arg1);
	    *res = tmpres;
	    return r;
	  }
      }

    printk ("sys_ioctl (not implemented) d=%i req=%x arg1=%x", d, (int)req, (int)arg1);

    *res = 0;
    return EINVAL;
  }



int sys_fstat (ret_t *res, struct proc *p, int d, struct stat *ss)
  {
    /*
     *	sys_fstat ()
     *	------------
     *
     *	Return stat data for an open file descriptor.
     */

    struct vnode *v;

    if (!res || !p)
      return EINVAL;

    if (d<0 || d>=p->nr_of_fdesc)
      return EBADF;

    if (!p->fdesc[d] || !p->fdesc[d]->v)
      return EBADF;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, ss, sizeof(struct stat),
		VMREGION_WRITABLE))
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    ss = (struct stat *) ((byte *)ss + userland_startaddr);

    v = p->fdesc[d]->v;
    if (!v)
      return EBADF;

    lock (&v->lock, "sys_fstat", LOCK_BLOCKING | LOCK_RO);
    *ss = v->ss;
    unlock (&v->lock);

    *res = 0;
    return 0;
  }



int sys_dup2 (ret_t *res, struct proc *p, int dold, int dnew)
  {
    /*
     *	sys_dup2 ()
     *	-----------
     *
     *	If descriptor dnew is in use, then close it. Then let dnew point to
     *	the same data as dold.
     *
     *	TODO: what happens if dold and dnew are the same???
     */

    if (!res || !p)
      return EINVAL;

    if (dold<0 || dold>=p->nr_of_fdesc ||
	dnew<0 || dnew>=p->nr_of_fdesc)
      return EBADF;

    if (!(p->fdesc[dold]))
      return EBADF;

    if (p->fdesc[dnew])
      sys_close (res, p, dnew);

    p->fdesc[dnew] = p->fdesc[dold];

    lock (&p->fdesc[dnew]->lock, "sys_dup2", LOCK_BLOCKING | LOCK_RW);
    p->fdesc[dnew]->refcount ++;
    unlock (&p->fdesc[dnew]->lock);

    /*  TODO:  should we return 0 or dnew here???
	OpenBSD returns dnew, but the manpage is not 100%
	clear about this...  */
    *res = dnew;

    return 0;
  }



int sys_fchdir (ret_t *res, struct proc *p, int fd)
  {
    /*
     *	sys_fchdir ()
     *	-------------
     *
     *	Set p->currentdir_vnode to the vnode of the filedescriptor fd. The
     *	refcount of the new dir should be increased, and the refcount of the
     *	old dir should de decreased.
     */

    if (!res || !p)
	return EINVAL;

    if (fd<0 || fd>=p->nr_of_fdesc)
	return EBADF;

    if (!p->fdesc[fd])
	return EBADF;

    if (!p->fdesc[fd]->v)
      {
	printk ("fchdir: pid=%i fd=%i, v==NULL", p->pid, fd);
	return EBADF;
      }

    if (p->currentdir_vnode)
      lock (&p->currentdir_vnode->lock, "sys_fchdir", LOCK_BLOCKING | LOCK_RW);

    /*  Already there?  (Same vnode?)  */
    if (p->currentdir_vnode == p->fdesc[fd]->v)
      {
	if (p->currentdir_vnode)
	  unlock (&p->currentdir_vnode->lock);
	return 0;
      }

    if (!S_ISDIR(p->fdesc[fd]->v->ss.st_mode))
      {
	if (p->currentdir_vnode)
	  unlock (&p->currentdir_vnode->lock);
	return ENOTDIR;
      }

    if (p->currentdir_vnode)
      {
	p->currentdir_vnode->refcount --;
	unlock (&p->currentdir_vnode->lock);
      }

    p->currentdir_vnode = p->fdesc[fd]->v;

    lock (&p->currentdir_vnode->lock, "sys_fchdir", LOCK_BLOCKING | LOCK_RW);
    p->currentdir_vnode->refcount ++;
    unlock (&p->currentdir_vnode->lock);

    return 0;
  }



int sys_lseek (off_t *res, struct proc *p, int fd, off_t offset, int whence)
  {
    /*
     *	sys_lseek ()
     *	------------
     *
     *	Move a file descriptors read/write pointer. This only works if the
     *	file descriptor refers to a vnode.
     *
     *	TODO: if the vnode has a seek function, then we should use it here
     */

    off_t tmpo;

    if (!res || !p)
	return EINVAL;

    if (fd<0 || fd>=p->nr_of_fdesc)
	return EBADF;

    if (!p->fdesc[fd])
	return EBADF;

    if (!p->fdesc[fd]->v)
	return EBADF;

    lock (&p->fdesc[fd]->lock, "sys_lseek", LOCK_BLOCKING | LOCK_RW);

    switch (whence)
      {
	case SEEK_SET:
		tmpo = offset;
		break;
	case SEEK_CUR:
		tmpo = p->fdesc[fd]->cur_ofs + offset;
		break;
	case SEEK_END:
		lock (&p->fdesc[fd]->v->lock, "sys_lseek", LOCK_BLOCKING);
		tmpo = p->fdesc[fd]->v->ss.st_size + offset;
		unlock (&p->fdesc[fd]->v->lock);
		break;
	default:
		unlock (&p->fdesc[fd]->lock);
		return EINVAL;
      }

    if (tmpo<0)
      {
	unlock (&p->fdesc[fd]->lock);
	return EINVAL;
      }

    p->fdesc[fd]->cur_ofs = tmpo;
    *res = p->fdesc[fd]->cur_ofs;
    unlock (&p->fdesc[fd]->lock);

    return 0;
  }



int sys_fcntl (ret_t *res, struct proc *p, int fd, int cmd, int arg)
  {
    /*
     *	sys_fcntl ()
     *	------------
     *
     *	File descriptor control.
     *
     *	TODO: maybe the contents of arg should be checked for F_SETFD
     *	and F_SETFL...
     */

    int i;

    if (!res || !p)
	return EINVAL;

    if (fd<0 || fd>=p->nr_of_fdesc)
	return EBADF;

    if (!p->fdesc[fd])
	return EBADF;

    switch (cmd)
      {
	case F_DUPFD:
		/*
		 *  According to OpenBSD's fcntl(2) man page: (12 Jan 1994)
		 *
		 *  "Return a new descriptor as follows:
		 *
		 *   o	Lowest numbered available descriptor greater than or
		 *	equal to arg (interpreted as an int).
		 *   o  Same object references as the original descriptor.
		 *   o  New descriptor shares the same file offset if the ob-
		 *	ject was a file.
		 *   o  Same access mode (read, write or read/write).
		 *   o  Same file status flags (i.e., both file descriptors
		 *	share the same file status flags).
		 *   o  The close-on-exec flag associated with the new file
		 *	descriptor is set to remain open across execv(3)
		 *	calls."
		 */

		/*  Find an available descriptor:  */
		i = arg;
		if (i<0 || i>=p->nr_of_fdesc)
		  return EBADF;
		while (p->fdesc[i] && i<p->nr_of_fdesc)  i++;
		if (i>=p->nr_of_fdesc)
		  return EMFILE;

		/*  Let the i refer the same descriptor as fd:  */
		p->fdesc[i] = p->fdesc[fd];
		/*  Note: No locking neccessary since fdesc[i] cannot be
			used by anyone else yet!  */
		p->fdesc[i]->refcount ++;
		p->fcntl_dflag[i] = 0;		/*  The opposite of FD_CLOEXEC  */
		break;
	case F_GETFD:
		*res = p->fcntl_dflag[fd];
		break;
	case F_SETFD:
		p->fcntl_dflag[fd] = arg;
		break;
	case F_GETFL:
		lock (&p->fdesc[fd]->lock, "sys_fcntl", LOCK_BLOCKING | LOCK_RO);
		*res = p->fdesc[fd]->sflag;
		unlock (&p->fdesc[fd]->lock);
		break;
	case F_SETFL:
		lock (&p->fdesc[fd]->lock, "sys_fcntl", LOCK_BLOCKING | LOCK_RW);
		p->fdesc[fd]->sflag = arg;
		unlock (&p->fdesc[fd]->lock);
		break;
	default:
		printk ("sys_fcntl: unimplemented cmd=%i (pid=%i fd=%i arg=%i)",
			cmd, p->pid, fd, arg);
		return EINVAL;
      }

    return 0;
  }

