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
 *  sys/filedesc.h  --  file descriptor structure
 *
 *	Processes access files through descriptors. From the program's
 *	perspective, a descriptor is a small integer value. In the kernel,
 *	each process has a small array of file descriptors.
 */

#ifndef	__SYS__FILEDESC_H
#define	__SYS__FILEDESC_H


#include <sys/defs.h>
#include <sys/lock.h>
#include <sys/vnode.h>


/*  The default nr of file descriptors on process creation:  */

#define	NR_OF_FDESC	64


struct fdesc
      {
	/*  Access mode, type:  */
	u_int32_t	mode;
	u_int32_t	type;

	/*  Lock for values that might be modified
		(refcount, cur_ofs, sflag):  */
	struct lockstruct lock;

	/*  sflag:  */
	u_int16_t	sflag;

	/*  Several processes may use the same descriptor:  */
	ref_t		refcount;

	/*  Current offset within the vnode, if applicable:  */
	off_t		cur_ofs;

	/*  For files:  A pointer to the vnode, or NULL:  */
	struct vnode	*v;

	/*  For sockets:  */
	int		so_domain;
	int		so_type;
	int		so_protocol;
      };


#define	FDESC_MODE_READ		1
#define	FDESC_MODE_WRITE	2
#define	FDESC_MODE_NONBLOCK	4

#define	FDESC_TYPE_FILE		0
#define	FDESC_TYPE_SOCKET	1



struct iovec
      {
	void	*iov_base;
	size_t	iov_len;
      };


struct proc;


int fd_detach (struct proc *p, int fdesc_nr);
struct fdesc *fd_alloc (int type);



/*  lseek() "flag":  (TODO: move to unistd.h ???)  */

#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2


#endif	/*  __SYS__FILEDESC_H  */

