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
 *  sys/lock.h  --  resource locks
 */

#ifndef	__SYS__LOCK_H
#define	__SYS__LOCK_H


#include <sys/defs.h>

struct proc;


/*  See kern/lock.c for more info about how this works:  */

struct lockstruct
      {
	char		*writelock_value;
	ref_t		readlock_refcount;
	struct proc	*waiting_proc;
      };


int lock (struct lockstruct *lockaddr, char *lockvalue, int flags);
int unlock (struct lockstruct *lockaddr);

/*  Should lock() block until we get the lock?  */
#define	LOCK_NONBLOCKING	0
#define	LOCK_BLOCKING		1

/*  Should lock() allow interrupts to be disabled?  */
#define	LOCK_NOINTERRUPTS	2

/*  Read-only lock or read/write?  */
#define	LOCK_RO			0
#define	LOCK_RW			4


#endif	/*  __SYS__LOCK_H  */

