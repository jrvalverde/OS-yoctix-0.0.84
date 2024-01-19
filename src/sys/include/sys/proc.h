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
 *  sys/proc.h  --  process description
 */

#ifndef	__SYS__PROC_H
#define	__SYS__PROC_H


#include <sys/limits.h>
#include <sys/defs.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/emul.h>
#include <sys/lock.h>
#include <sys/filedesc.h>
#include <sys/md/proc.h>

struct vm_region;
struct vnode;


/*
 *  Process queues:
 */

#define	PROC_MAXQUEUES		4
#define	NO_QUEUE		0
#define	RUN_QUEUE		1
#define	SLEEP_QUEUE		2
#define	ZOMBIE_QUEUE		3
#define	runqueue		procqueue[RUN_QUEUE]
#define	sleepqueue		procqueue[SLEEP_QUEUE]
#define	zombiequeue		procqueue[ZOMBIE_QUEUE]



/*
 *  Process status:
 */

#define	P_NOTHING		0
#define	P_RUN			1
#define	P_SLEEP			2
#define	P_ZOMBIE		3
#define	P_IDL			4



/*
 *  Process credentials.  Note that uid/gid are the Effective uid and
 *  gid values, and that ruid/rgid are the real uid and gid values.
 */

struct	pcred
      {
	uid_t		ruid;			/*  Real UID  */
	gid_t		rgid;			/*  Real GID  */
	uid_t		uid;			/*  Effective UID  */
	gid_t		gid;			/*  Effective GID  */
	uid_t		suid;			/*  Saved UID  */
	gid_t		sgid;			/*  Saved GID  */
      };



/*
 *  The process structure
 *  ---------------------
 *
 *  There is one instance of the process structure for each process in the
 *  system. Instead of having a large array containing all the processes that
 *  the system can hold, the system maintains linked lists of processes.
 *  These lists are (usually) the "runqueue" and the "sleepqueue".
 *
 *  When SMP is to be implemented (in the future), one runqueue for each CPU
 *  will probably be a good approach.  Also, there could be several
 *  sleepqueues (even on single-CPU systems) to increase performance on
 *  very busy systems. This is not fully implemented yet, but will not be
 *  too hard to do.
 *
 *  PROC_MI_COPYFROM and PROC_MI_COPYTO surround the Machine Independant
 *  data which will be copied on a fork(). The rest of the data needs to
 *  be zero-filled (initially) and set-up manually.
 */

struct proc
      {
	/*  Queue links:  */
	struct proc	*next;			/*  Next process in queue  */
	struct proc	*prev;			/*  Previous in queue  */

	struct lockstruct lock;			/*  Struct lock  */

	/*  Process creation time:  (set by proc_alloc())  */
	struct timespec	creattime;

	/*  CPU time usage (measured in ticks_t):  */
	ticks_t		uticks;			/*  User ticks  */
	ticks_t		sticks;			/*  System ticks  */
	ticks_t		iticks;			/*  Interrupt ticks  */
	ticks_t		*ticks;			/*  points to one of [usi]ticks  */

	/*  Process identification:  */
	pid_t		pid;			/*  Process ID  */
	pid_t		ppid;			/*  ID of Parent (or init)  */
	pid_t		pgid;			/*  Process Group ID  */

	int		flags;			/*  Process flags  */

	int		sc_params_ok;		/*  Memory ranges for syscall parameters
						    has been checked? (0 or 1)  */

	/*  Child/parent relationship:  */
	struct proc	*parent;		/*  parent. (NULL if orphaned)  */
	int		nr_of_children;		/*  nr of children  */
	int		exitcode;		/*  exitcode for parent  */

#define	PROC_MI_COPYFROM cred

	/*  Process credentials:  */
	struct pcred	cred;			/*  uid, gid, ...  */

	/*  Syscall emulation:  */
	struct emul	*emul;			/*  binary (syscall) emulation  */

	/*  Scheduling:  */
	u_int32_t	status;			/*  Status (P_ZOMBIE, P_RUN, P_SLEEP)  */
	pri_t		priority;		/*  Priority  */
	pri_t		nice;			/*  Nice value  */
	void *		wchan;			/*  Sleep address  */
	char *		wmesg;			/*  Sleep message  */

	/*  Signal masks:  */
	sigset_t	sigmask;		/*  Blocked signal mask  */
	sigset_t	sigignore;		/*  Ignored signal mask  */
	sigset_t	sigcatch;		/*  Signals being caught by user  */

#define	PROC_MI_COPYTO vmregions

	/*  Virtual Memory regions:  */
	struct vm_region *vmregions;		/*  Ptr to first vm region  */
	struct vm_region *dataregion;		/*  Ptr to data region, used by sys_break()  */

	/*  File descriptors etc.:  */
	int		nr_of_fdesc;		/*  Total nr of descriptor pointers  */
	int		first_unused_fdesc;	/*  Index of the first unused fdesc  */
	struct fdesc	**fdesc;		/*  Array of descriptor pointers  */
	u_int16_t	*fcntl_dflag;		/*  fcntl descriptor flags  */
	mode_t		umask;			/*  umask for file creation  */

	struct vnode	*rootdir_vnode;		/*  Root directory  */
	struct vnode	*currentdir_vnode;	/*  Current directory  */

	/*  Signal handlers:  */
	struct sigaction sigacts[NSIG];		/*  Signal actions  */
	sigset_t	siglist;		/*  Signals to be delivered  */


/*  TODO:  limits, capabilities, timers  */


	/*  Machine dependant info:  */
	struct mdproc	md;
      };



#define	PFLAG_CALLEDEXEC		1



/*
 *  proc related functions:
 */

void proc_init ();
struct proc *proc_alloc ();
int proc_remove (struct proc *);
int superuser ();
void pswitch ();
void sleep (void *, char *);
int wakeup_proc (struct proc *p);
void wakeup (void *);
struct proc *find_proc_by_pid (pid_t pid);
int proc_exit (struct proc *p, int exitcode);

#endif	/*  __SYS__PROC_H  */

