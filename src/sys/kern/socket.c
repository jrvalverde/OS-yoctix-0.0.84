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
 *  kern/socket.c  --  socket functions
 */


#include "../config.h"
#include <sys/defs.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/errno.h>


int socket_create (struct proc *p, int domain, int type, int protocol)
  {
    /*
     *	socket_create
     *	-------------
     *
     *	Returns a filedesriptor number on success (>0), -1 on error.
     */

    struct fdesc *fp;
    int s;

    fp = fd_alloc (FDESC_TYPE_SOCKET);
    if (!fp)
      return -1;

    fp->so_domain   = domain;
    fp->so_type     = type;
    fp->so_protocol = protocol;

    s = 0;
    while (p->fdesc[s] && s<p->nr_of_fdesc)  s++;
    if (s>=p->nr_of_fdesc)
      {
	free (fp);
	return EMFILE;
      }

    fp->refcount ++;
    p->fdesc[s] = fp;

    return s;
  }

