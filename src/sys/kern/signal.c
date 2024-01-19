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
 *  kern/signal.c  --  signal delivery functions
 *
 *  History:
 *	15 Sep 2000	test
 *	30 Oct 2000	more testing... :)
 */


#include "../config.h"
#include <sys/std.h>
#include <sys/errno.h>
#include <sys/interrupts.h>
#include <sys/proc.h>
#include <sys/syscalls.h>
#include <sys/signal.h>



int sig_deliver (struct proc *p, ret_t *retval)
  {
    /*
     *	sig_deliver ()
     *	--------------
     *
     *	This function should be called to deliver a signal as specified in
     *	p->siglist to the process 'p'.
     */

    int signr, found;
    sigset_t siglist;

    if (!p || !retval)
	return EINVAL;

    siglist = p->siglist & ~p->sigmask;

printk ("sig_deliver (p->pid=%i, siglist=0x%x)", p->pid, siglist);

    /*  Figure out which signal nr to post:  */
    signr = 0;
    found = 0;
    while (signr<NSIG)
      {
	if (siglist & (1<<signr))
	  found = signr, signr = NSIG;
	else
	  signr++;
      }
    signr = found;
    if (!signr)
	panic ("signal_post(): no unmasked bit in p->siglist (pid=%i)", p->pid);

    /*  The signal is being delivered), so we remove signr from p->siglist.  */
    p->siglist &= ~(1<<signr);

    if (p->sigacts[signr].sa_handler)
      {
	*retval = signr;
	machdep_sigtramp (p, p->sigacts[signr].sa_handler, signr);
      }
#if DEBUGLEVEL>=3
    else
      printk ("sig_deliver: no handler specified. TODO: default actions");
#endif

    return 0;
  }



int sig_post (struct proc *pfrom, struct proc *pto, int signr)
  {
    /*
     *	sig_post ()
     *	-----------
     *
     *	Post signal nr 'signr' from process 'pfrom' to process 'pto'.
     */

    sigset_t sigval;

printk ("sig_post (pfrom=%x,pto=%x,signr=%i)", pfrom,pto,signr);

    if (!pfrom || !pto || signr<=0 || signr>=NSIG)
	return EINVAL;

    sigval = 1 << signr;

    /*
     *	Is the signal being masked?  (Here we trust p->sigmask to not include
     *	unmaskable signals.)
     */

    sigval &= (~pto->sigmask);
    if (!sigval)
	return 0;


/*  Ugly:  */
if ((int)pto->sigacts[signr].sa_handler < PAGESIZE)
  signr = SIGKILL;
else
  printk ("pto->sigacts[signr].sa_handler = %i",
	pto->sigacts[signr].sa_handler);


    /*
     *	Some signals are handled manually:	TODO
     */

    if (signr == SIGKILL)
      {
	return proc_exit (pto, 0);
      }



    pto->siglist |= sigval;


/*  TODO: already running processes (usermode) ???  */

    if (pto->status == P_SLEEP)
	wakeup_proc (pto);

    return 0;
  }

