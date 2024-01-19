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
 *  kern/sys_misc.c  --  misc. kernel syscalls
 *
 *
 *  History:
 *	7 Nov 2000	test
 */


#include "../config.h"
#include <sys/std.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/interrupts.h>
#include <sys/md/machdep.h>



extern volatile struct proc *procqueue [PROC_MAXQUEUES];



int sys_reboot (int *res, struct proc *p, int how)
  {
    /*
     *	sys_reboot ()
     *	-------------
     *
     *	Reboot or halt the system. First, the following things
     *	need to be done:
     *
     *		1. All processes killed
     *		   a)   try SIGTERM first, and wait at most 10 seconds
     *			for processes to finnish their stuff
     *		   b)   if any processes are left, then use sys_exit()
     *
     *		2. All mounted filesystems need to be synched and
     *		   unmounted.
     *
     *	Then, machine dependant code is called to halt or reboot the
     *	machine.
     */

    if (!res || !p)
	return EINVAL;

    if (p->cred.uid != 0)
	return EPERM;


    /*  1a:  TODO  (here interrupts need to be temporarily enabled)  */


    /*
     *	1b:  Kill all remaining processes by using proc_exit().
     */



    /*  2:  TODO  */


    /*
     *	Reboot or halt:
     *	TODO:  This is OpenBSD specific... should be generalized
     */

    if (how & 8)
      {
	printk ("The system has halted.");
	machdep_halt ();
      }
    else
      {
	printk ("Rebooting...");
	machdep_reboot ();
      }

    return 0;
  }

