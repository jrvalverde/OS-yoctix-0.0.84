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
 *  sys/arch/i386/signal.h  --  i386 signal trampoline things...
 */

#ifndef	__SYS__ARCH__I386__SIGNAL_H
#define	__SYS__ARCH__I386__SIGNAL_H


#include <sys/defs.h>

struct proc;

struct sigcontext
      {
	u_int32_t	eax;
	u_int32_t	ecx;
	u_int32_t	edx;
	u_int32_t	ebx;

	u_int32_t	ebp;
	u_int32_t	esp;
	u_int32_t	esi;
	u_int32_t	edi;

	u_int32_t	cs;
	u_int32_t	ds;
	u_int32_t	es;
	u_int32_t	fs;
	u_int32_t	gs;
	u_int32_t	ss;

	u_int32_t	eip;
	u_int32_t	eflags;

	sigset_t	sigmask;
      };


int machdep_sigreturn (struct proc *p, struct sigcontext *scp);
int machdep_sigtramp (struct proc *p, void *codeptr, int signr);


#endif	/*  __SYS__ARCH__I386__PROC_H  */

