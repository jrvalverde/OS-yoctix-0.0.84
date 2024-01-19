/*
 *  Copyright (C) 1999,2000,2001 by Anders Gavare.  All rights reserved.
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
 *  kern/proc.c  --  process handling functions
 *
 *	Processes are a central concept of any (multitasking) operating system.
 *	In general, a "process" is a kind of "state of execution". There can
 *	be several processes running on a computer, and it is up to the
 *	operating system (with the use of the underlaying hardware if possible)
 *	to let each process get a fair share of the CPU time.
 *
 *	A process can run either in user mode or in kernel mode. Several
 *	processes can be in kernel mode simultaneously. Unfortunately, this
 *	forces us to use locking mechanisms to make sure that accesses to
 *	data structures in memory are atomic.
 *
 *	Also, if a process needs to wait for something to complete (for example
 *	a read from a floppy disk) it will sleep in kernel mode. Sleeping
 *	causes the process to be moved from the run queue to a sleep queue.
 *	A sleeping process can later be woken up with wakeup(), for example
 *	when a hardware interrupt (for example from the floppy drive
 *	controller) is received.
 *
 *	Whenever the operating system has nothing left to do, it calls
 *	pswitch() to switch to another running process.
 *
 *
 *	TODO:	When SMP is to be implemented, there will probably be one
 *		run queue per CPU.
 *
 *
 *	proc_init()
 *		Initialize the process handling functions.
 *
 *	proc_alloc()
 *		Allocate memory for a process structure.
 *
 *	proc_remove()
 *		Remove a process from memory (and the process queue)
 *
 *	superuser()
 *		Returns 1 if the uid of curproc is zero.
 *
 *	pswitch()
 *		Jumps to the next process pointed in the run queue.
 *
 *	sleep()
 *		Causes the current process to sleep on an address.
 *
 *	wakeup_proc()
 *		Moves a specific process from the sleepqueue to the runqueue.
 *
 *	wakeup()
 *		Moves processes sleeping on an address from the sleepqueue
 *		to the runqueue.
 *
 *	find_proc_by_pid()
 *		Return a pointer to the process having a specific pid value.
 *
 *	proc_exit()
 *		Perform neccessary cleanup before removing a process from
 *		all process queues.
 *
 *
 *  History:
 *	27 Dec 1999	first version
 *	28 Dec 1999	adding preliminary dispatcher()
 *	3 Jan 2000	more process queues
 *	13 Jan 2000	refreshing pswitch(), proc_remove() should now
 *			actually remove a process from all run queues...
 *			Removed proc_setcurproc() because that functionality
 *			is in pswitch() now.
 *			Adding sleep() and wakeup()
 *	14 Jan 2000	pswitch() switches to the process pointed to by
 *			runqueue, not runqueue->next
 *	xxx 2000	old behaviour of pswitch() again...
 *	22 Mar 2000	reserving space for filedescriptors in proc_alloc()
 *	3 Nov 2000	adding find_proc_by_pid()
 *	7 Nov 2000	adding proc_exit()
 */


#include "../config.h"
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/std.h>
#include <sys/interrupts.h>
#include <sys/vm.h>
#include <sys/errno.h>
#include <sys/syscalls.h>
#include <string.h>


volatile struct proc	*procqueue [PROC_MAXQUEUES];	/*  Process queues  */
volatile struct proc	*curproc;			/*  Current running process  */
volatile int	need_to_pswitch;
volatile int	idle;
volatile int	nr_of_switches = 0;

byte		*pidbitmap;
struct lockstruct pidbitmap_lock;
pid_t		lastpid;

extern volatile int ticks_until_pswitch;
extern struct timespec system_time;
extern int switchratio;


void proc_init ()
  {
    /*
     *	proc_init ()
     *	------------
     *
     *	Initialize process handling related structures etc.
     *
     *		o)  Process queues (runqueue, ...)
     *		o)  pidbitmap (used to generate unique pids)
     *		o)  No process running yet (curproc)
     */

    int i;
    size_t pdmsize;

    for (i=0; i<PROC_MAXQUEUES; i++)
	procqueue[i] = NULL;

    memset (&pidbitmap_lock, 0, sizeof(pidbitmap_lock));
    pdmsize = (PID_MAX - PID_MIN + 1)/8 + 1;
    pidbitmap = (byte *) malloc (pdmsize);
    if (!pidbitmap)
	panic ("proc_init(): !pidbitmap");
    memset (pidbitmap, 0, pdmsize);
    lastpid = PID_MIN;

    curproc = NULL;
    need_to_pswitch = 0;
    idle = 0;
  }



struct proc *proc_alloc ()
  {
    /*
     *	proc_alloc ()
     *	-------------
     *
     *	Allocate memory for a process structure and return a pointer to it.
     *	The process is NOT placed on any process queue.
     *
     *	Both machine dependant and machine independant data fields are set to
     *	sane default values. Anything done by proc_alloc() is undone by
     *	proc_remove().
     *
     *	This function returns NULL on failure.
     */

    struct proc *p;
    int oldints;

    p = (struct proc *) malloc (sizeof(struct proc));
    if (!p)
	return NULL;


    /*
     *  Machine independant initialization:
     */

    memset (p, 0, sizeof(struct proc));

    oldints = interrupts (DISABLE);
    p->creattime = system_time;
    interrupts (oldints);

    p->ticks = &p->uticks;

    /*  Allocate space for file descriptors:  */
    p->fdesc = (struct fdesc **) malloc (sizeof(struct fdesc *) * NR_OF_FDESC);
    if (!p->fdesc)
      {
	free (p);
	return NULL;
      }

    p->fcntl_dflag = (u_int16_t *) malloc (sizeof(u_int16_t) * NR_OF_FDESC);
    if (!p->fcntl_dflag)
      {
	free (p->fdesc);
	free (p);
	return NULL;
      }

    p->nr_of_fdesc = NR_OF_FDESC;
    p->first_unused_fdesc = 0;
    memset (&p->fdesc[0], 0, sizeof(struct fdesc *) * NR_OF_FDESC);
    memset (p->fcntl_dflag, 0, sizeof(u_int16_t) * NR_OF_FDESC);

    p->umask = 022;


    /*
     *  Machine dependant initialization:
     */

    if (!machdep_proc_init (p))
      {
	free (p->fcntl_dflag);
	free (p->fdesc);
	free (p);
	return NULL;
      }

    return p;
  }



int proc_remove (struct proc *p)
  {
    /*
     *	proc_remove ()
     *	--------------
     *
     *	Removes a process from memory:
     *
     *   (o)  Calls remove_pid() to make the PID available
     *	 (o)  Removes the process from any process queues
     *	 (o)  Frees the memory occupied by the process structure(s)
     *
     *	NOTE: The process should be killed correctly (all filedescriptors
     *	should be closed, child processes killed etc.) BEFORE this function
     *	is called.
     *
     *	Returns 1 on success, 0 on failure.
     */

    struct proc *tmpptr;
    int i;
    int oldints;


    if (!p)
	return 0;


    /*  TODO:  is the result from remove_pid() important here?  */
    remove_pid (p->pid);

    oldints = interrupts (DISABLE);

    /*  Remove p from any process queue(s):  */
    tmpptr = p->next;
    if (tmpptr)
	tmpptr->prev = p->prev;

    tmpptr = p->prev;
    if (tmpptr)
	tmpptr->next = p->next;

    for (i=0; i<PROC_MAXQUEUES; i++)
	if (procqueue[i] == p)
	  {
	    if (procqueue[i]->next == p)
		procqueue[i] = NULL;
	    else
		procqueue[i] = procqueue[i]->next;
	  }


    /*
     *	Free machine dependant parts of the process structure:
     */

    if (!machdep_proc_remove (p))
      {
	printk ("proc_remove(): cannot remove pid %i", p->pid);
	interrupts (oldints);
	return 0;
      }


    /*
     *	Free machine independant parts of the process structure:
     */

    /*	Free memory used by the file descriptor array:  */
    if (p->fdesc)
	free (p->fdesc);

    if (p->fcntl_dflag)
	free (p->fcntl_dflag);

    /*	Free the process structure itself:  */
    free (p);

    interrupts (oldints);

    /*  Return successfully:  */
    return 1;
  }



void pswitch ()
  {
    /*
     *	pswitch ()
     *	----------
     *
     *	Jump to run the NEXT process pointed to in the run queue.
     *
     *	This function is called from the machine dependant system timer
     *	interrupt handler at regular intervals, or from the machine
     *	independant parts whenever the kernel wants to run another user
     *	process.
     */

    int oldints;
    volatile static int in_pswitch = 0;


    oldints = interrupts (DISABLE);
    ticks_until_pswitch = switchratio;


    /*
     *	We don't want pswitch() to be called recursively.
     */

    if (idle || in_pswitch>0)
      {
	interrupts (oldints);
	return;
      }

    in_pswitch ++;


    /*
     *	No process running yet (during kernel initialisation) and
     *	no process on the runqueue to jump to?
     *	Then return without doing anything.
     */

    if (!curproc && !runqueue)
      {
	in_pswitch --;
	interrupts (oldints);
	return;
      }


    /*
     *	No run queue? Then wait (idle loop) until a process is woken
     *	up; wakeup() should set idle = 0.
     */

    if (!runqueue)
      {
#if DEBUGLEVEL>=5
	printk ("(idle)");
#endif

	idle = 1;
	interrupts (ENABLE);

	while (idle)
	    ;

	interrupts (DISABLE);

#if DEBUGLEVEL>=5
	printk ("(running)");
#endif

	if (!runqueue)
	    panic ("pswitch: no runqueue, dangerous... !");
      }


    /*
     *	When the system is starting up and no process has ever run,
     *	we have curproc=NULL. Then we should pswitch() to the first process.
     */

    /*  Step forward in the run queue:  */
    runqueue = runqueue->next;

    in_pswitch --;
    need_to_pswitch = 0;
    nr_of_switches++;		/*  statistics  */


    /*
     *	Call machine dependant routine to actually "launch" the process,
     *	and set curproc = runqueue:
     */

    machdep_pswitch ((struct proc *) runqueue);

    interrupts (oldints);
  }



void sleep (void *addr, char *msg)
  {
    /*
     *	sleep ()
     *	--------
     *
     *	sleep() puts the currently running process on the sleep queue, and
     *	then calls pswitch() to give CPU time to other (running) processes.
     *
     *	A call to wakeup() moves the process back onto the run queue.
     *
     *	A special case is when no process is running on the system (when
     *	curproc == NULL). Then we simply wait until an interrupt_has_occured.
     */

    int oldints;
    oldints = interrupts (DISABLE);


    if (!curproc)
      {
	/*  Sleep when no process has started yet:  */
	idle = 1;
	interrupts (ENABLE);
	while (idle)  ;
	interrupts (oldints);
	return;
      }

    if (curproc->status != P_RUN && curproc->status != P_IDL)
      {
	printk ("sleep(0x%x,\"%s\"): process %i is not running (status=%i)",
		(u_int32_t) addr, msg, curproc->pid, curproc->status);
	interrupts (oldints);
	return;
      }

    curproc->status = P_SLEEP;
    curproc->wchan = addr;
    curproc->wmesg = msg;


    /*
     *	Move the process to the sleep queue:
     */

    if (runqueue != curproc)
      {
	printk ("!!  runqueue=%x curproc=%x", runqueue, curproc);
	curproc=runqueue->next;
	while (curproc!=runqueue)
	  {
	    printk ("    runqueue %x", curproc);
	    curproc=curproc->next;
	  }
	panic ("sleep(0x%x,\"%s\"): inconsistensy in the run queue", (u_int32_t) addr, msg);
      }

    curproc->next->prev = curproc->prev;
    curproc->prev->next = curproc->next;
    if (curproc == curproc->next)
	runqueue = NULL;
    else
	runqueue = runqueue->next;

    if (sleepqueue)
      {
	sleepqueue->prev->next = (struct proc *) curproc;
	curproc->prev = sleepqueue->prev;
	curproc->next = (struct proc *) sleepqueue;
	sleepqueue->prev = (struct proc *) curproc;
	sleepqueue = curproc;
      }
    else
      {
	curproc->next = (struct proc *) curproc;
	curproc->prev = (struct proc *) curproc;
	sleepqueue = curproc;
      }

    pswitch();
    interrupts (oldints);
  }



int wakeup_proc (struct proc *p)
  {
    /*
     *	wakeup_proc ()
     *	--------------
     *
     *	Move p from the sleep queue to the run queue, and clear sleep address
     *	into stored in the proc struct.
     *
     *	Returns errno on error.
     */

    struct proc *tmpp;
    int oldints;

    oldints = interrupts (DISABLE);

    if (!p)
      {
	interrupts (oldints);
	return EINVAL;
      }

    if (p->status != P_SLEEP)
      {
	printk ("wakeup_proc(): trying to wake up non-sleeping process pid=%i", p->pid);
	interrupts (oldints);
	return EINVAL;
      }

    /*  Remove from sleep queue:  */
    tmpp = p->prev;
    if (tmpp)
	tmpp->next = p->next;

    tmpp = p->next;
    if (tmpp)
	tmpp->prev = p->prev;

    if (sleepqueue == p)
      sleepqueue = p->next;

    /*  Add to runqueue:  */
    if (!runqueue)
      {
	p->next = p;
	p->prev = p;
	runqueue = p;
      }
    else
      {
	p->next = runqueue->next;
	p->prev = (struct proc *) runqueue;
	runqueue->next->prev = p;
	runqueue->next = p;
      }

    p->status = P_RUN;
    p->wchan = NULL;
    p->wmesg = NULL;

    interrupts (oldints);

    return 0;
  }



void wakeup (void *addr)
  {
    /*
     *	wakeup ()
     *	---------
     *
     *	Wake up a process which is sleeping on an address. In fact, we wake
     *	up ALL processes sleeping on the address...
     *
     *	(TODO: maybe there should be a wakeup_one() call too?)
     *
     *	Processes that are woken up are moved from the sleep queue to the
     *	run queue. If one or more processes were woken up, we set
     *	need_to_pswitch = 1, so that pswitch() is called as soon as possible.
     *	(For example at the return after an interrupt has been handled.)
     */

    struct proc *p, *next_p;
    int done = 0, any_moved = 0;
    int total_on_sleepqueue, i;
    int oldints;


    oldints = interrupts (DISABLE);


    /*
     *	curproc=NULL before the kernel has started the first process.
     *	There's actually no process to wake up, but if there's an idle
     *	loop currently running in sleep() then we should abort it.
     */

    if (!curproc)
      {
	idle = 0;
	interrupts (oldints);
	return;
      }


    p = (struct proc *) sleepqueue;
    if (!p)
      {
	interrupts (oldints);
	return;
      }


    /*  First round: mark the processes with status=P_RUN  */
    total_on_sleepqueue = 0;
    while (!done)
      {
	if (p->wchan == addr)
	  p->status = P_RUN;

	p = p->next;
	total_on_sleepqueue ++;

	/*  Back where we started?  */
	if (p == sleepqueue)
	  done = 1;
      }

    /*  Second round: move P_RUN-processes to the run queue:  */
    p = (struct proc *) sleepqueue;
    for (i = 0; i<total_on_sleepqueue; i++)
      {
	next_p = p->next;

	/*  This will also catch any processes which have been marked
		runnable but weren't actually sleeping on any address...  */
	if (p->status == P_RUN)
	  {
	    /*  Remove p from the sleep queue:  */
	    p->prev->next = p->next;
	    p->next->prev = p->prev;

	    /*  Was p the only process on the sleep queue? If so, then
		remove this queue completely and abort the for():  */
	    if (p == next_p)
	      {
		sleepqueue = NULL;
		i = total_on_sleepqueue;
	      }
	    else
	      sleepqueue = next_p;

	    /*  Add p to the run queue:  */
	    if (runqueue)
	      {
		p->next = runqueue->next;
		runqueue->next->prev = p;
		runqueue->next = p;
		p->prev = (struct proc *) runqueue;
/*
		runqueue->prev->next = p;
		p->prev = runqueue->prev;
		p->next = (struct proc *) runqueue;
		runqueue->prev = (struct proc *) p;
		runqueue = p;
		runqueue = runqueue->prev;
*/
	      }
	    else
	      {
		p->next = p;
		p->prev = p;
		runqueue = p;
	      }

	    p->wchan = NULL;
	    p->wmesg = NULL;

	    any_moved = 1;
	  }

	if (i<total_on_sleepqueue)
	  p = next_p;
      }

    if (runqueue)
	idle = 0;

    if (any_moved)
	need_to_pswitch = 1;

    interrupts (oldints);
  }



struct proc *find_proc_by_pid (pid_t pid)
  {
    /*
     *	find_proc_by_pid ()
     *	-------------------
     *
     *	Returns a pointer to the process with pid 'pid'. If no process is
     *	found, then NULL is returned.
     *
     *	IMPORTANT!!!  This function must be called with interrupts disabled!
     *
     *	TODO:  This function uses a linear search through all procqueues.
     *	(Needs to be remodelled.)
     */

    struct proc *p;
    int i;

    for (i=0; i<PROC_MAXQUEUES; i++)
      {
	p = (struct proc *) procqueue [i];

	if (p)
	  {
	    if (p->pid == pid)
	      return p;

	    p = p->next;
	    while (p != (struct proc *) procqueue[i])
	      {
		if (p->pid == pid)
		  return p;

		p = p->next;
	      }
	  }
      }

    return NULL;
  }



int proc_exit (struct proc *p, int exitcode)
  {
    /*
     *	proc_exit ()
     *	------------
     *
     *	  o)  Unmap all mapped memory regions.
     *	  o)  Close all (file)descriptors.
     *	  o)  Set exitcode to return to parent.
     *	  o)  Remove the process from the runqueue.
     *	  o)  Send a signal to the parent (?).
     */

    int err, i;
    ret_t tmpres;
    struct proc *tmpp;
    int oldints;
    struct vm_region *region, *nextregion;


printk ("proc_exit(): pid=%i, ppid=%i", p->pid, p->ppid);


    p->status = P_IDL;


    /*
     *	Unmap all mapped memory regions (the entire address space):
     *	(This is not a call to munmap(), because it is too slow.)
     */

    region = p->vmregions;
    while (region)
      {
	nextregion = region->next;
	vm_region_detach (p, region->start_addr);
	region = nextregion;
      }


    /*
     *	Close all file descriptors:  (ignore errors)
     */

    for (i=p->nr_of_fdesc-1; i>=0; i--)
	if (p->fdesc[i])
	  err = sys_close (&tmpres, p, i);

    if (p->currentdir_vnode)
	p->currentdir_vnode->refcount --;
    if (p->rootdir_vnode)
	p->rootdir_vnode->refcount --;


    /*  Set exitcode to return to parent  */
    p->exitcode = exitcode;


    /*
     *	Remove the process from any procqueues:
     */

    oldints = interrupts (DISABLE);
    p->status = P_ZOMBIE;

    for (i=0; i<PROC_MAXQUEUES; i++)
      if ((tmpp = (struct proc *) procqueue[i]))
	{
	  tmpp = tmpp->next;
	  while (tmpp != (struct proc *) procqueue[i] && tmpp!=p)
		tmpp = tmpp->next;
	  if (tmpp == p)
	    {
	      /*  The only process left in this runqueue?  */
	      if (tmpp->next == tmpp)
		  procqueue[i] = NULL;
	      else
		{
		  tmpp->prev->next = tmpp->next;
		  tmpp->next->prev = tmpp->prev;
		  procqueue[i] = tmpp->prev;
		}
	    }
	}


    /*  Place the process on the zombiequeue:  */
    if (!zombiequeue)
      {
	zombiequeue = p;
	p->next = p;
	p->prev = p;
      }
    else
      {
	p->next = zombiequeue->next;
	p->prev = (struct proc *) zombiequeue;
	zombiequeue->next->prev = p;
	zombiequeue->next = p;
      }


    /*  If a parent exists and is waiting for this process to
	become a zombie, then let's awaken it (by sending SIGCHLD):  */
    if (p->parent)
      {

if (p->parent->wchan == &p->parent->nr_of_children)
  wakeup_proc (p->parent);
else
  sig_post (p, p->parent, SIGCHLD);

/*	wakeup (&p->parent->nr_of_children);  */

      }

/*
    else

  TODO: what to do if p->parent = NULL ?  send sig to init??

      printk ("proc_exit(): p->parent = NULL. not yet implemented");
*/

    interrupts (oldints);

    return 0;
  }

