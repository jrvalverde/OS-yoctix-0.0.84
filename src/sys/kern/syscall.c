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
 *  kern/syscall.c  --  syscall entry point
 *
 *	When a process wants the kernel to do something for it, it calls the
 *	kernel via a system call. The exact mechanism may differ between
 *	hardware architectures, but we always end up in syscall() with a
 *	syscall number and a number of arguments.
 *
 *	The call is then redirected to the correct "handler", depending on
 *	which emulation the process is using.  This was set when the
 *	process was first started (probably using execve() or similar).
 *
 *
 *	syscall_init()
 *		Initialize the syscall emulation etc.
 *
 *	syscall()
 *		Machine independant syscall entry point.
 *		Called from the machine dependant (assembly language)
 *		routine which handles low-level syscalls.
 *
 *
 *  History:
 *	24 Oct 1999	first version
 *	13 Jan 2000	working again
 *	20 Jan 2000	adding "emul" module at "virtual"
 */


#include "../config.h"

#include <sys/std.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/emul.h>
#include <sys/interrupts.h>
#include <sys/module.h>
#include <sys/syscalls.h>
#include <sys/signal.h>



extern volatile struct proc *curproc;
extern volatile int need_to_pswitch;
extern volatile int nr_of_switches;


void syscall_init ()
  {
    module_register ("virtual", MODULETYPE_SYSTEM | MODULETYPE_BUILTIN, "emul", "Binary emulation formats");

    /*  See also reg/emul.c  */
  }



ret_t syscall (int *errno, int nr, int r1, int r2, int r3, int r4, int r5, int r6,
	int r7, int r8, int r9, int r10)
  {
    /*
     *	syscall ()  --  Machine independant syscall entry
     *	-------------------------------------------------
     *
     *	The machine dependant routine which calls this function should do
     *	so with interrupts _disabled_ to avoid race conditions regarding
     *	the contents of "curproc".
     *
     *	Then, we turn on interrupts (here) to allow timer ticks and
     *	process switches to occur while we're running in kernel mode.
     *
     *	What we must do here is simply to call the correct syscall
     *	function (which depends on which emulation the process is using).
     */

    ret_t res = 0;
    int (*func)();
    struct emul *e;
    struct proc *p;


    /*
     *	Remember which process we are running, since a process switch
     *	can occur later during the handling of the syscall.
     *
     *	Also, since we're in kernel mode (or "system" mode), we should
     *	make sure that timer ticks update p->sticks instead of p->uticks.
     *
     *	sc_params_ok needs to be reset, since no syscall arguments have
     *	been validated yet.
     */

    p = (struct proc *) curproc;
    p->ticks = &p->sticks;
    interrupts (ENABLE);
    p->sc_params_ok = 0;


    /*
     *	Which emulation is the process using? Is the syscall unknown?
     */

    *errno = 0;
    e = p->emul;

    if (!e)
	panic ("pid %i has no syscall emulation", p->pid);

    if (nr>=e->highest_syscall_nr || nr<0 || !(func=e->syscall[nr]))
      {
#if DEBUGLEVEL>=2
/*
	printk ("syscall(%s#%i,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x): TODO",
		e->name, nr, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10);
*/
	printk ("syscall(%s#%i,%x,%x,%x,%x,%x,%x)",
		e->name, nr, r1, r2, r3, r4, r5, r6);
#endif
	*errno = ENOSYS;
	return 0;
      }

/*
printk ("pid %i #%i,%x,%x,%x,%x,%x,%x)",
	p->pid, nr, r1, r2, r3, r4, r5, r6);
*/

    /*
     *	Do the actual syscall:
     */

    *errno = func (&res, p, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10);

/*
printk ("done%i #%i,%x,%x,%x,%x,%x,%x)",
	p->pid, nr, r1, r2, r3, r4, r5, r6);
*/

    /*
     *	In case the syscall wants us to do a pswitch here, then
     *	we do so. If pswitch() is successful it will clear the
     *	need_to_pswitch variable, so there's no need to do that here.
     *
     *	(For example, the current i386 fork() implementation needs to
     *	do a process switch before returning to the parent process.)
     */

    if (need_to_pswitch)
	pswitch ();


    /*
     *	If there are signals that haven't yet been delivered to the
     *	process, then we do so now:
     */

    if (p->siglist & ~p->sigmask)
      {
	res = 0;
	sig_deliver (p, &res);
	*errno = 0;
      }


    /*
     *	Let future timer ticks update the user tick counter.
     */

    p->ticks = &p->uticks;


    return res;
  }

