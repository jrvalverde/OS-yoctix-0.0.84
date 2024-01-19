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
 *  kern/sys_sig.c  --  signal handling related syscalls
 *
 *
 *  History:
 *	20 Jul 2000	empty sys_sigsuspend()
 *	24 Jul 2000	adding sys_sigprocmask(), sys_sigaction()
 *	15 Sep 2000	sys_sigreturn()
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/md/signal.h>
#include <sys/errno.h>
#include <sys/syscalls.h>
#include <sys/interrupts.h>



extern size_t userland_startaddr;



int sys_sigsuspend (ret_t *res, struct proc *p, int sigmask)
  {
    /*
     *	sys_sigsuspend ()
     *	-----------------
     *
     *	TODO: implement this function.
     *
     *	NOTE: This function should be implemented with sigmask
     *	as a _pointer_ to the sigmask, not the sigmask itself. The
     *	openbsd_aout emulation on the other hand receives just the
     *	sigmask.
     */

    *res = 0;

printk ("sigsuspend pid=%i: sigmask=%x", p->pid, sigmask);

sleep (&p->siglist, "sigsuspend");

printk ("           pid=%i (sigsuspend awakened)", p->pid);

    /*  Always return EINTR:  */
    return EINTR;
  }



int sys_sigprocmask (ret_t *res, struct proc *p, int how, sigset_t *set, sigset_t *oset)
  {
    /*
     *	sys_sigprocmask ()
     *	------------------
     *
     *	Set or unset the blocking bits of p->sigmask.
     *	Returns 0 on success, errno on error.
     */

    if (!res || !p)
	return EINVAL;

/*  TODO: check set and oset  */

    set  = (sigset_t *) ((byte *)set  + userland_startaddr);
    oset = (sigset_t *) ((byte *)oset + userland_startaddr);

    *oset = p->sigmask;

    switch (how)
      {
	case SIG_BLOCK:		p->sigmask |=  (*set); break;
	case SIG_UNBLOCK:	p->sigmask &= ~(*set); break;
	case SIG_SETMASK:	p->sigmask  =  (*set); break;
	default:
	  return EINVAL;
      }

    p->sigmask &= ~SIG_UNMASKABLE;

    return 0;
  }



int sys_sigaction (ret_t *res, struct proc *p, int sig, struct sigaction *act, struct sigaction *oact)
  {
    /*
     *	sys_sigaction ()
     *	----------------
     *
     *	Sets (and returns) the sigaction for signal sig.
     *	Returns 0 on success, errno on error.
     */

    if (!res || !p)
	return EINVAL;

    if (sig<0 || sig>=NSIG)
	return EINVAL;

#if DEBUGLEVEL>=4
    printk ("sys_sigaction (pid=%i, sig=%i)", p->pid, sig);
#endif

/*  TODO: check act and oact  */

    act  = (struct sigaction *) ((byte *)act  + userland_startaddr);
    if (oact != NULL)
	oact = (struct sigaction *) ((byte *)oact + userland_startaddr);

    if (oact)
	*oact = p->sigacts [sig];

/*  TODO: check act for allowed values, handle flags etc.  */

    p->sigacts [sig] = *act;

    return 0;
  }



int sys_sigreturn (ret_t *res, struct proc *p, struct sigcontext *scp)
  {
    /*
     *	sys_sigreturn ()
     *	----------------
     *
     *	The sigcontext struct pointed to by scp contains CPU registers
     *	and the value of the old signal mask which we should restore.
     *
     *	If successful, this function never returns to the caller.
     *
     *	NOTE:  We're giving a userland address to machdep_sigreturn()!
     */

    if (!res || !p)
	return EINVAL;

    return machdep_sigreturn (p, scp);
  }



int sys_kill (ret_t *res, struct proc *p, pid_t pid, int sig)
  {
    /*
     *	sys_kill ()
     *	-----------
     */

    int oldints;
    struct proc *tmpp;

    if (!res || !p || sig<1 || sig>=NSIG)
	return EINVAL;

printk ("sys_kill(): from pid=%i to pid=%i, sig=%i", p->pid, pid, sig);

    if (pid<=0)
      {
	printk ("sys_kill: pid=%i (process group kill TODO)", pid);
	return EINVAL;
      }

    oldints = interrupts (DISABLE);

    tmpp = find_proc_by_pid (pid);
    if (!tmpp)
      {
	interrupts (oldints);
	return ESRCH;
      }

    if (sig==0)
      {
	interrupts (oldints);
	return 0;
      }

    /* TODO:  SIGCONT may be sent by any process to any of its
		descendants!  */
    if (p->cred.uid != 0 &&
	tmpp->cred.ruid != p->cred.uid &&
	tmpp->cred.ruid != p->cred.ruid &&
	tmpp->cred.ruid != p->cred.suid &&
	tmpp->cred.uid != p->cred.uid &&
	tmpp->cred.uid != p->cred.ruid &&
	tmpp->cred.uid != p->cred.suid)
      {
	interrupts (oldints);
	return EPERM;
      }

    /*  TODO: how to make sure that the process isn't killed by
	two different processes at the same time, and one "gets
	first"...  */
    interrupts (oldints);
    sig_post (p, tmpp, sig);
    return 0;
  }

