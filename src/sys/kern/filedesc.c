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
 *  kern/fdesc.c  --  file descriptor handling
 *
 *	fd_detach()
 *		"Close" a filedescriptor.
 *
 *	fd_alloc()
 *		Allocate memory for a file descriptor, and zero it.
 *
 *  History:
 *	22 Mar 2000	test
 *	2 Jun 2000	adding fd_alloc()
 *	9 Dec 2000	cleaning up
 */


#include "../config.h"
#include <string.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/std.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/errno.h>
#include <sys/lock.h>



int fd_detach (struct proc *p, int fdesc_nr)
  {
    /*
     *	fd_detach ()
     *	------------
     *
     *	Remove a reference to a file descriptor from a process. If this was
     *	the last reference to the descriptor, then we also attempt to close
     *	the vnode.
     *
     *	Returns 0 on success, errno on error.
     */

    int err = 0;

    if (!p)
	return EINVAL;

    if (fdesc_nr<0 || fdesc_nr>=p->nr_of_fdesc)
	return EBADF;

    if (!p->fdesc[fdesc_nr])
	return EBADF;

    lock (&p->fdesc[fdesc_nr]->lock, "fd_detach", LOCK_BLOCKING | LOCK_RW);

    if (p->fdesc[fdesc_nr]->refcount<=0)
	panic ("fd_detach: pid=%i fd=%i refcount=%i, should be >0",
		p->pid, fdesc_nr, p->fdesc[fdesc_nr]->refcount);

    if (--p->fdesc[fdesc_nr]->refcount == 0)
      {
	if (p->fdesc[fdesc_nr]->v)
	  err = vnode_close (p->fdesc[fdesc_nr]->v, p);

	free (p->fdesc[fdesc_nr]);
      }
    else
      unlock (&p->fdesc[fdesc_nr]->lock);

    p->fdesc[fdesc_nr] = NULL;

    /*  Update the first_unused_fdesc "hint":  */
    if (fdesc_nr < p->first_unused_fdesc)
	p->first_unused_fdesc = fdesc_nr;

    return err;
  }



struct fdesc *fd_alloc (int type)
  {
    /*
     *	fd_alloc ()
     *	-----------
     *
     *	Allocate memory for a file descriptor and zero it. Return a
     *	pointer to the allocated memory area. (NULL if malloc failed.)
     */

    struct fdesc *fd;

    fd = (struct fdesc *) malloc (sizeof (struct fdesc));
    if (!fd)
	return NULL;

    memset (fd, 0, sizeof(struct fdesc));
    fd->type = type;
    return fd;
  }

