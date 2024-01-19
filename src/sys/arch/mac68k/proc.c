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
 *  arch/mac68k/proc.c
 */


#include "../config.h"
#include <sys/proc.h>



extern volatile struct proc *procqueue [];
extern volatile struct proc *curproc;

size_t userland_startaddr;



void machdep_pagedir_init (struct proc *p)
  {
    /*
     *	machdep_pagedir_init ()
     *	-----------------------
     *
     *	Initialize the pagedir of a process. The first entries of the pagedir
     *	should be mapped to the physical RAM.
     */
  }



int machdep_proc_init (struct proc *p)
  {
    /*
     *	machdep_proc_init ()
     *	--------------------
     *
     *	Allocate mac68k specific process structures, and "insert" them
     *	into the process 'p'.
     *
     *	Returns 1 on success, 0 on failure.
     */

    return 0;
  }



int machdep_proc_freemaps (struct proc *p)
  {
    /*
     *	Free memory occupied by process p's pagetables
     *	and page directory.
     *	Returns 1 on success, 0 on failure.
     */

    return 1;
  }



int machdep_proc_remove (struct proc *p)
  {
    /*
     *	machdep_proc_remove ()
     *	----------------------
     *
     *	Free the 'md' part of p.
     *	Returns 1 on success, 0 on failure.
     */

    if (!p)
	return 0;

    /*  Free pagedir and all pagetables:  */
    machdep_proc_freemaps (p);

    /*  Free the kernel stack associated with this process:  */


    return 1;
  }



void machdep_pswitch (struct proc *p)
  {
    /*
     *	machdep_pswitch ()
     *	------------------
     *
     *	Set curproc to the new process 'p' and jump to it.
     *	(Only switch to the new process if it is not already running.)
     */

    if (curproc != p)
      {
	curproc = p;
/*	i386_jmptss (p->md.tss_nr); */
      }
  }



int machdep_fork (struct proc *parent, struct proc *child)
  {
    /*
     *	machdep_fork ()
     *	---------------
     *
     *	Should be called with interrupts disabled.
     */

    return 0;
  }


