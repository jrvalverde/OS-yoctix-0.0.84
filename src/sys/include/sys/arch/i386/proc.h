/*
 *  Copyright (C) 1999,2000 by Anders Gavare.  All rights reserved.
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
 *  sys/arch/i386/proc.h  --  i386 machine dependant part of process description
 */

#ifndef	__SYS__ARCH__I386__PROC_H
#define	__SYS__ARCH__I386__PROC_H


#include <sys/defs.h>

struct proc;
struct vm_object;

struct i386tss
      {
	u_int16_t	backlink;
	u_int16_t	reserved0;

	u_int32_t	esp0;
	u_int16_t	ss0;
	u_int16_t	reserved1;

	u_int32_t	esp1;
	u_int16_t	ss1;
	u_int16_t	reserved2;

	u_int32_t	esp2;
	u_int16_t	ss2;
	u_int16_t	reserved3;

	u_int32_t	cr3;
	u_int32_t	eip;
	u_int32_t	eflags;
	u_int32_t	eax;
	u_int32_t	ecx;
	u_int32_t	edx;
	u_int32_t	ebx;
	u_int32_t	esp;
	u_int32_t	ebp;
	u_int32_t	esi;
	u_int32_t	edi;

	u_int16_t	es;
	u_int16_t	reserved4;
	u_int16_t	cs;
	u_int16_t	reserved5;
	u_int16_t	ss;
	u_int16_t	reserved6;
	u_int16_t	ds;
	u_int16_t	reserved7;
	u_int16_t	fs;
	u_int16_t	reserved8;
	u_int16_t	gs;
	u_int16_t	reserved9;
	u_int16_t	ldt;
	u_int16_t	reserved10;
	u_int16_t	trap;
	u_int16_t	iobase;
      };



struct mdproc
      {
	/*  Task State Segment:  */
	struct	i386tss		*tss;

	u_int16_t		tss_nr;

	/*  Pointer to the process' page table directory:  */
	u_int32_t		*pagedir;

	/*  Pointer to the process' kernel stack:  */
	void			*kstack;

	/*  "Temporary" pointer to the process' stack vm_object.
	    Used to create pages containing argc,argv,envp, and
	    all the environment and argument strings and pointers...  */
	struct vm_object	*argvenvp_stackobj;

	size_t			size_of_argvenvp;
      };



/*  Functions in arch/i386/proc.c:  */

void machdep_pagedir_init (struct proc *);
int machdep_proc_init (struct proc *);
int machdep_proc_freemaps (struct proc *);
int machdep_proc_remove (struct proc *);
int machdep_fork (struct proc *parent, struct proc *child);

void machdep_setkerneltaskaddr (void *);
void machdep_pswitch (struct proc *p);


/*  Functions in arch/i386/pmap.c:  */

int pmap_mappage (struct proc *p, size_t virtualaddr, byte *a_page, u_int32_t region_type);
int pmap_markpages_cow (struct proc *p, size_t startaddr, size_t endaddr);
int pmap_unmappages (struct proc *p, size_t startaddr, size_t endaddr);
int pmap_mapped (struct proc *p, size_t virtualaddr);


#endif	/*  __SYS__ARCH__I386__PROC_H  */

