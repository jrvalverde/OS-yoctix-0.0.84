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
 *  arch/mac68k/signal.c
 *
 *  History:
 *	9 Dec 2000	empty
 */


#include "../../config.h"
#include <sys/std.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/interrupts.h>



int machdep_sigreturn (struct proc *p, struct sigcontext *scp)
  {
    /*
     *	'scp' points to a sigcontext struct with CPU registers and a signal
     *	mask that we should restore.
     *
     *	Returns errno on error. Doesn't return on success, but rather jumps
     *	to the "new" cs:eip.
     */


    return 0;
  }



int machdep_sigtramp (struct proc *p, void *codeptr, int signr)
  {
    /*
     *	machdep_sigtramp ()
     *	-------------------
     *
     *	Set up signal trampoline code for process p; that is, modify
     *	p's userland stack so that the code at 'codeptr' is executed
     *	and when that code returns, machdep_sigreturn() should be
     *	called.
     *
     *	There are two cases we have to handle -- if p is curproc, then we
     *	need to get cpu register values from p's kernel stack, otherwise
     *	we'll find them in p->md.tss.
     *
     *	Read signal_asm.S for more info on arch_sigtramp_asmcode.
     *
     *	TODO:  altstack?  not assume size of arch_sigtramp_asmcode?
     */


    return 0;
  }

