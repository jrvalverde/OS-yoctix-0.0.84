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
 *  kern/sys_proc.c  --  Process related syscalls
 *
 *	sys_exit ()
 *		Terminate the process
 *
 *	sys_wait4 ()
 *
 *	sys_getpid ()
 *	sys_getppid ()
 *
 *	sys_getprgp ()
 *	sys_setpgid ()
 *
 *	sys_getuid ()
 *	sys_geteuid ()
 *	sys_getgid ()
 *	sys_getegid ()
 *
 *	sys_setuid ()
 *	sys_seteuid ()
 *	sys_setgid ()
 *	sys_setegid ()
 *
 *	sys_issetugid ()
 *
 *	sys_nanosleep ()
 *
 *
 *  History:
 *	8 Mar 2000	first version
 *	...
 *	15 Jun 2000	adding sys_nanosleep()
 */


#include "../config.h"
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/syscalls.h>
#include <sys/interrupts.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/vm.h>
#include <sys/emul.h>
#include <sys/time.h>
#include <sys/timer.h>


extern volatile struct proc *procqueue [];
extern size_t userland_startaddr;


extern size_t malloc_totalfreememory;



int sys_exit (ret_t *res, struct proc *p, int exitcode)
  {
    /*
     *	sys_exit ()
     *	-----------
     *
     *	This call should never return.
     */

    if (!res || !p)
	return EINVAL;

    proc_exit (p, exitcode);

    /*  Switch to another running process:  */
    pswitch ();

    panic ("sys_exit: Not reached (called from other than self???)");
    return 0;
  }



int sys_wait4 (ret_t *res, struct proc *p, pid_t wpid, int *status, int options,
	       void *rusage)
  {
    /*
     *	sys_wait4 ()
     *	------------
     *
     *	Wait for a child process to terminate, and return its exitcode.
     */

    int oldints;
    struct proc *tmpp;
    struct proc *found;


#if DEBUGLEVEL>=4
    printk ("sys_wait4 wpid=%i status=%x options=%x rusage=%x",
		(int)wpid, (int)status, (int)options, (int)rusage);
#endif

    if (!res || !p)
	return EINVAL;

    if (p->nr_of_children == 0)
	return ECHILD;

/*  TODO: check addresses!  EFAULT if illegal...  */

    status = (int *) ((byte *)status + userland_startaddr);
    rusage = (void *) ((byte *)rusage + userland_startaddr);

/*  TODO:  options (WNOHANG etc)  */

    oldints = interrupts (DISABLE);

    while (1)
      {
	/*  Scan the zombiequeue for children of p:  */
	tmpp = (struct proc *) zombiequeue;
	found = NULL;
	while (tmpp)
	  {
	    if (tmpp->parent == p)
	      {
		found = tmpp;
		tmpp = NULL;
	      }

	    /*  next process in the queue:  */
	    if (tmpp)
	      {
		tmpp = tmpp->next;
		if (tmpp == zombiequeue)
		  tmpp = NULL;
	      }
	  }

	if (found)
	  {
	    *res = found->pid;

	    /*  TODO:  generalize, include/sys/wait.h  */
	    *status = (found->exitcode) << 8;

	    p->nr_of_children --;
	    interrupts (oldints);


	    /*
	     *	Remove the process from the zombiequeue and
	     *	free the memory it occupied when it was alive:
	     */

	    proc_remove (found);

	    return 0;
	  }

	/*  Wait until a child has become a zombie:  */
	sleep (&p->nr_of_children, "wait4");
      }

    /*  not reached  */
    return 0;
  }



int sys_getpid (ret_t *res, struct proc *p)
  {
    /*
     *	sys_getpid ()
     *	-------------
     */

    *res = p->pid;
    return 0;
  }



int sys_getppid (ret_t *res, struct proc *p)
  {
    /*
     *	sys_getppid ()
     *	--------------
     */

    *res = p->ppid;
    return 0;
  }



int sys_getpgrp (ret_t *res, struct proc *p)
  {
    /*
     *	sys_getpgrp ()
     *	--------------
     */

    *res = p->pgid;
    return 0;
  }



int sys_setpgid (ret_t *res, struct proc *p, pid_t pid, pid_t grp)
  {
    /*
     *	sys_setpgid ()
     *	--------------
     */

    struct proc *p2;
    int oldints;

    if (grp < 0)
      return EINVAL;

    oldints = interrupts (DISABLE);

    if (pid==0)
      p2 = p;
    else
      p2 = find_proc_by_pid (pid);

    interrupts (oldints);

    if (!p2)
      return ESRCH;

    /*
     *	setpgid() is allowed if at least one of the following is true:
     *
     *	  1)  caller is superuser
     *	  2)  caller's effective uid is same as that of 'pid'
     *	  3)  'pid' is a descendant of caller (p).  (TODO)
     */

    if (p->cred.uid!=0 && p->cred.uid!=p2->cred.uid)
      return EPERM;

    /*  TODO: this is maybe incorrect... what if p2 is not a child
	process of p, should this check really be done then???  */
    if (p!=p2)
      if (p2->flags & PFLAG_CALLEDEXEC)
	return EACCES;

    p->pgid = grp;
    return 0;
  }



int sys_getuid (ret_t *res, struct proc *p)
  {
    /*
     *	sys_getuid ()
     *	-------------
     */

    *res = p->cred.ruid;
    return 0;
  }



int sys_geteuid (ret_t *res, struct proc *p)
  {
    /*
     *	sys_geteuid ()
     *	--------------
     */

    *res = p->cred.uid;
    return 0;
  }



int sys_getgid (ret_t *res, struct proc *p)
  {
    /*
     *	sys_getgid ()
     *	-------------
     */

    *res = p->cred.rgid;
    return 0;
  }



int sys_getegid (ret_t *res, struct proc *p)
  {
    /*
     *	sys_getegid ()
     *	--------------
     */

    *res = p->cred.gid;
    return 0;
  }



int sys_setuid (ret_t *res, struct proc *p, uid_t u)
  {
    /*
     *	sys_setuid ()
     *	-------------
     *
     *	According to the OpenBSD man page for setuid():
     *	"The setuid() function sets the real and effective user IDs and
     *	 the saved set-user-ID of the current process to the specified value.
     *	 The setuid() function is permitted if the effective user ID is that
     *	 of the super user, or if the specified user ID is the same as the
     *	 effective user ID.  If not, but the specified user ID is the same as
     *	 the real user ID, setuid() will set the effective user ID to the real
     *	 user ID."
     */

    if (p->cred.uid==0 || u==p->cred.uid)
      {
	p->cred.ruid = u;
	p->cred.uid  = u;
	p->cred.suid = u;
	return 0;
      }

    if (u==p->cred.ruid)
      {
	p->cred.uid = u;
	return 0;
      }

    return EACCES;
  }



int sys_seteuid (ret_t *res, struct proc *p, uid_t u)
  {
    /*
     *	sys_seteuid ()
     *	--------------
     *
     *	According to the OpenBSD seteuid man page, the effective uid may
     *	only be set to either the real uid or the saved-uid of the process.
     */

    if (u==p->cred.ruid || u==p->cred.suid)
      {
	p->cred.uid = u;
	return 0;
      }

    return EACCES;
  }



int sys_setgid (ret_t *res, struct proc *p, gid_t g)
  {
    /*
     *	sys_setgid ()
     *	-------------
     *
     *	Similar to setuid(), but note that being superuser means uid=0,
     *	not gid=0.
     */

    if (p->cred.uid==0 || g==p->cred.gid)
      {
	p->cred.rgid = g;
	p->cred.gid  = g;
	p->cred.sgid = g;
	return 0;
      }

    if (g==p->cred.rgid)
      {
	p->cred.gid = g;
	return 0;
      }

    return EACCES;
  }



int sys_setegid (ret_t *res, struct proc *p, gid_t g)
  {
    /*
     *	sys_setegid ()
     *	--------------
     *
     *	Similar to seteuid().
     */

    if (g==p->cred.rgid || g==p->cred.sgid)
      {
	p->cred.gid = g;
	return 0;
      }

    return EACCES;
  }



int sys_issetugid (ret_t *res, struct proc *p)
  {
    /*
     *	sys_issetugid ()
     *	----------------
     *
     *	TODO: implement this... ala OpenBSD.
     */

    return 0;
  }



int sys_nanosleep (ret_t *res, struct proc *p, const struct timespec *rqtp,
	struct timespec *rmtp)
  {
    /*
     *	sys_nanosleep()
     *	---------------
     *
     *	Sleep for a specific number of nanoseconds.
     */

    struct timespec *ts;


    if (!res || !p)
	return EINVAL;

/*  TODO: check addresses  */

    ts = (struct timespec *) ((byte *)rqtp + userland_startaddr);
    rmtp = (struct timespec *) ((byte *)rmtp + userland_startaddr);

    if (ts->tv_sec<0 || ts->tv_nsec<0 || ts->tv_nsec>1000000000)
	return EINVAL;

    *res = timer_sleep (p, ts, "nanosleep");

/*  TODO:  set rmtp to the unslept time!!!  */

    return 0;
  }


