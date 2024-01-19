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
 *  sys/arch/mac68k/proc.h
 */

#ifndef	__SYS__ARCH__MAC68K__PROC_H
#define	__SYS__ARCH__MAC68K__PROC_H


#include <sys/defs.h>

struct proc;
struct vm_object;


struct mdproc
      {
	int	tmp;
      };



/*  Functions in arch/mac68k/proc.c:  */

void machdep_pagedir_init (struct proc *);
int machdep_proc_init (struct proc *);
int machdep_proc_freemaps (struct proc *);
int machdep_proc_remove (struct proc *);
int machdep_fork (struct proc *parent, struct proc *child);
void machdep_pswitch (struct proc *p);


/*  Functions in arch/mac68k/pmap.c:  */

int pmap_mappage (struct proc *p, size_t virtualaddr, byte *a_page, u_int32_t region_type);
int pmap_markpages_cow (struct proc *p, size_t startaddr, size_t endaddr);
int pmap_unmappages (struct proc *p, size_t startaddr, size_t endaddr);
int pmap_mapped (struct proc *p, size_t virtualaddr);


#endif	/*  __SYS__ARCH__MAC68K__PROC_H  */

