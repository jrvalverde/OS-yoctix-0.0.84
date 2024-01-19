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
 *  arch/i386/signal.c  --  i386 signal trampoline functions
 *
 *  History:
 *	15 Sep 2000	first version
 */


#include "../../config.h"
#include <sys/arch/i386/signal.h>
#include <sys/std.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/interrupts.h>


extern struct proc *curproc;
extern size_t userland_startaddr;

void arch_sigtramp_asmcode ();



int machdep_sigreturn (struct proc *p, struct sigcontext *scp)
  {
    /*
     *	'scp' points to a sigcontext struct with CPU registers and a signal
     *	mask that we should restore.
     *
     *	Returns errno on error. Doesn't return on success, but rather jumps
     *	to the "new" cs:eip.
     */

    u_int32_t *pstack;

printk ("in machdep_sigreturn (p->pid=%i, scp=0x%x)", p->pid, scp);

    scp = (struct sigcontext *) ((byte *)scp + userland_startaddr);
    pstack = (u_int32_t *) p->md.tss->esp0;

    pstack[ -1] = scp->ss;
    pstack[ -2] = scp->esp;
    pstack[ -3] = scp->eflags;
    pstack[ -4] = scp->cs;
    pstack[ -5] = scp->eip;
    pstack[ -6] = scp->eax;
    pstack[ -7] = scp->ebx;
    pstack[ -8] = scp->ecx;
    pstack[ -9] = scp->edx;
    pstack[-10] = scp->ebp;
    pstack[-11] = scp->esi;
    pstack[-12] = scp->edi;
    pstack[-13] = scp->ds;
    pstack[-14] = scp->es;
    pstack[-15] = scp->fs;
    pstack[-16] = scp->gs;

/*
printk ("  cs:eip = %Y:%x  ss:esp = %Y:%x", scp->cs, scp->eip, scp->ss, scp->esp);
printk ("  eflags = %x  eax,ebx,ebp = %x,%x,%x", scp->eflags, scp->eax, scp->ebx, scp->ebp);
*/

    /*
     *	TODO:  all fields need to be permission-checked...
     *	the best thing to do if something is incorrect is probably to
     *	kill the process, although sigreturn(2) on openbsd should
     *	return EFAULT or EINVAL to the caller.  (Then there'll have
     *	to be a suicide call (kill(2)) after sigreturn(2) in the
     *	signal trampoline asm code... just in case sigreturn() fails.)
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

    u_int32_t eax,ebx,ecx,edx,ebp,esi,edi,esp;
    u_int32_t eip,eflags,cs,ds,es,fs,gs,ss;
    int oldints;
    u_int32_t *pstack;
    struct sigcontext *scp;


    oldints = interrupts (DISABLE);

    if (!p)
      {
	interrupts (oldints);
	return EINVAL;
      }

printk ("machdep_sigtramp (p->pid=%i codeptr=0x%x)", p->pid, codeptr);


    /*
     *	Retrieve p's cpu register contents:
     */

    if (p==curproc)
      {
/*
	printk ("  machdep_sigtramp(): p==curproc");
*/

	pstack = (u_int32_t *) p->md.tss->esp0;

	ss  = pstack[-1];
	esp = pstack[-2];
	eflags = pstack[-3];
	cs  = pstack[-4];
	eip = pstack[-5];
	eax = pstack[-6];
	ebx = pstack[-7];
	ecx = pstack[-8];
	edx = pstack[-9];
	ebp = pstack[-10];
	esi = pstack[-11];
	edi = pstack[-12];
	ds  = pstack[-13];
	es  = pstack[-14];
	fs  = pstack[-15];
	gs  = pstack[-16];
      }
    else
      {
	printk ("  machdep_sigtramp(): p!=curproc (not yet implemented, TODO)");
return EINVAL;
	eax = p->md.tss->eax;
	ebx = p->md.tss->ebx;
	ecx = p->md.tss->ecx;
	edx = p->md.tss->edx;
	esi = p->md.tss->esi;
	edi = p->md.tss->edi;
	ebp = p->md.tss->ebp;
	esp = p->md.tss->esp;
	eip = p->md.tss->eip;
	eflags = p->md.tss->eflags;
	cs = p->md.tss->cs;
	ds = p->md.tss->ds;
	es = p->md.tss->es;
	fs = p->md.tss->fs;
	gs = p->md.tss->gs;
	ss = p->md.tss->ss;
      }

/*
printk ("about to trampoline pid=%i, cs:eip=0x%Y:0x%x esp=0x%x ebp=0x%x eflags=0x%x",
	p->pid, cs,eip,esp,ebp,eflags);
*/

    /*  TODO: check esp etc so that we don't overwrite stuff which we're
	not supposed to overwrite...  */


    /*
     *	Setup a sigcontext struct on p's userland stack:
     */

    esp -= sizeof (struct sigcontext);
    scp = (struct sigcontext *) ((byte *)esp + userland_startaddr);
    memset (scp, 0, sizeof(struct sigcontext));

    scp->eax = eax; scp->ebx = ebx; scp->ecx = ecx; scp->edx = edx;
    scp->esi = esi; scp->edi = edi; scp->ebp = ebp; scp->esp = esp+sizeof(struct sigcontext);
    scp->cs = cs; scp->ds = ds; scp->es = es; scp->fs = fs; scp->gs = gs;
    scp->ss = ss; scp->eip = eip; scp->eflags = eflags;

    scp->sigmask = 0;		/*  TODO:  which sigmask??  */


    /*
     *	Copy the trampoline code:
     *	TODO: this assumes that the trampoline code fits within 32 bytes!!
     *	TODO 2: minimal security risk: if we copy too much, we'll get some
     *	of the rest of the kernel text segment too...
     */

    esp -= 32;
    memcpy ((void *)((byte *)esp + userland_startaddr), arch_sigtramp_asmcode, 32);


    /*
     *	Set cpu registers to start the trampoline code:
     *		eax = signr
     *		ebx = addr of sighandler
     */

    if (p==curproc)
      {
/*
	printk ("  machdep_sigtramp(): (2) p==curproc");
*/

	pstack = (u_int32_t *) p->md.tss->esp0;

	pstack[-2] = esp;				/*  esp  */
	pstack[-5] = esp;				/*  eip  */
	pstack[-6] = signr;				/*  eax... but not used...?  */
	pstack[-7] = (u_int32_t) codeptr;		/*  ebx  */
      }
    else
      {
	printk ("  machdep_sigtramp(): (2) p!=curproc (not yet implemented, TODO)");
return EINVAL;
      }

    interrupts (oldints);

/*
printk ("returning from sigtramp()");
*/
    return 0;
  }

