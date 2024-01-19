/*
 *  Copyright (C) 2000,2001 by Anders Gavare.  All rights reserved.
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
 *  kern/sys_fork.c  --  the fork() syscall
 *
 *
 *  History:
 *	14 Apr 2000	first version
 *	24 May 2000	continuing, not finished yet
 *	19 Jul 2000	moving vm_region duplication to vm_fork()
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/syscalls.h>
#include <sys/interrupts.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/vm.h>
#include <sys/emul.h>
#include <sys/lock.h>


extern volatile struct proc *procqueue [];

extern volatile int need_to_pswitch;

extern byte *pidbitmap;
extern struct lockstruct pidbitmap_lock;
extern pid_t lastpid;



int remove_pid (pid_t pid)
  {
    /*
     *	remove_pid ()
     *	-------------
     *
     *	Removes 'pid' from the pidbitmap. Returns 0 on success, 1 if
     *	the pid bit already was cleared. 2 if the pid was out of range.
     *
     *	This function should only be called by proc_remove().
     */

    pid_t index;
    int mask;

    if (pid < PID_MIN || pid > PID_MAX)
      return 2;

    lock (&pidbitmap_lock, "remove_pid", LOCK_BLOCKING | LOCK_RW);

    index = pid - PID_MIN;
    mask = 1 << (index & 7);
    index >>= 3;

    if (pidbitmap[index] & mask)
      {
	pidbitmap[index] &= (~mask);
	unlock (&pidbitmap_lock);
	return 0;
      }

    unlock (&pidbitmap_lock);
    return 1;
  }



pid_t get_unused_pid ()
  {
    /*
     *	get_unused_pid ()
     *	-----------------
     *
     *	Return a process ID in the range [PID_MIN, PID_MAX] which is
     *	guaranteed to not be used by any other process. This is done by
     *	finding the first unused bit in the pidbitmap. The bit is set to
     *	1 before we return, so that the caller doesn't need to use any
     *	kind of external locking to set the bit.
     *	On error, 0 (zero) is returned.
     *
     *	(Actually, at first I thought of using random PIDs ala OpenBSD
     *	but I believe that get_unused_pid() would have become very slow
     *	when there are lots of processes (thousands...) running on the
     *	system since we would have called random() until we succeed in
     *	finding an unused PID.  Maybe I'll implement this later...)
     */

    pid_t index, oldlastpid, foundpid = 0;
    int mask;

    lock (&pidbitmap_lock, "get_unused_pid", LOCK_BLOCKING | LOCK_RW);

    if (lastpid > PID_MAX || lastpid < PID_MIN)
	lastpid = PID_MIN;
    oldlastpid = lastpid;

    while (!foundpid)
      {
	index = lastpid - PID_MIN;
	mask = 1 << (index & 7);
	index >>= 3;

	if (pidbitmap[index] & mask)
	  {
	    lastpid++;
	    if (lastpid > PID_MAX)
	      lastpid = PID_MIN;
	    if (lastpid == oldlastpid)
	      {
		printk ("get_unused_pid: no more free PIDs!");
		unlock (&pidbitmap_lock);
		return 0;
	      }
	  }
	else
	  {
	    foundpid = lastpid;
	    pidbitmap[index] |= mask;
	  }
      }

    lastpid++;

    unlock (&pidbitmap_lock);

    return foundpid;
  }



int sys_fork (ret_t *res, struct proc *p)
  {
    /*
     *	sys_fork ()
     *	-----------
     *
     *	The fork() syscall should create a child process which is an identical
     *	copy of its parent process except for the following:
     *
     *		1)  Unique PID
     *		2)  PPID should be set to the PID of the parent
     *		3)  Timer counts (and some other things) reset to zero
     *
     *	Since the parent and child share the same file descriptors, we have to
     *	duplicate the parent's fdesc chain...
     *
     *	If either of the processes writes to a memory page which is in use by
     *	both the processes, a page fault should occur to protect the memory
     *	of the other process. Therefore, we need to turn all writable pages
     *	into Copy-On-Write pages. Read-only pages (or actually entire read-only
     *	vm_objects) can still be shared, though.
     *
     *	The res return value after a successfull fork() is the child's PID
     *	or 0 (to the child).  Otherwise, errno is returned.
     */

    int i, tmp, oldints;
    pid_t child_pid;
    struct proc *child_proc;
    struct vm_region *region, *nextregion;


    /*
     *	Create child process and copy parts of the parent process info.
     *	Also set up "trivial" new values such as PID and PPID.
     */

    child_pid = get_unused_pid ();

    oldints = interrupts (DISABLE);

    child_proc = proc_alloc ();
    if (!child_proc)
      {
	interrupts (oldints);
	remove_pid (child_pid);
	return ENOMEM;
      }

    /*  Copy parts of the Machine Independant process struct from the parent:  */
    memcpy (&child_proc->PROC_MI_COPYFROM, &p->PROC_MI_COPYFROM,
	(size_t) &p->PROC_MI_COPYTO - (size_t) &p->PROC_MI_COPYFROM);

    child_proc->pid = child_pid;
    child_proc->ppid = p->pid;
    child_proc->parent = p;
    p->nr_of_children ++;


    /*
     *	Duplicate the parent's file descriptor chain:
     */

    /*  Free the space occupied by the (emtpy) fdesc array allocated
	by proc_alloc():  */
    if (child_proc->fdesc)
	free (child_proc->fdesc);

    if (child_proc->fcntl_dflag)
	free (child_proc->fcntl_dflag);

    /*  Allocate room for as many pointers as the parent process has:  */
    child_proc->fdesc = (struct fdesc **) malloc (sizeof(struct fdesc *)
	* p->nr_of_fdesc);

    if (!child_proc->fdesc)
      {
	p->nr_of_children --;
	interrupts (oldints);
	proc_remove (child_proc);
	return ENOMEM;
      }

    child_proc->nr_of_fdesc = p->nr_of_fdesc;
    child_proc->first_unused_fdesc = p->first_unused_fdesc;

    child_proc->fcntl_dflag = (u_int16_t *) malloc (sizeof(u_int16_t) * p->nr_of_fdesc);
    if (!child_proc->fcntl_dflag)
      {
	p->nr_of_children --;
	interrupts (oldints);
	proc_remove (child_proc);
	return ENOMEM;
      }


    /*
     *	Duplicate _the pointers_ to all the parent's descriptors, and
     *	the parent's fcntl_dflags.
     */

    memcpy (child_proc->fdesc, p->fdesc, sizeof(struct fdesc *) * child_proc->nr_of_fdesc);
    memcpy (child_proc->fcntl_dflag, p->fcntl_dflag, sizeof(u_int16_t) * child_proc->nr_of_fdesc);

    /*  Increase the refcount of the descriptors:  */
    for (i=0; i<child_proc->nr_of_fdesc; i++)
	if (child_proc->fdesc[i])
	  child_proc->fdesc[i]->refcount++;


    child_proc->rootdir_vnode = p->rootdir_vnode;
    if (child_proc->rootdir_vnode)
      child_proc->rootdir_vnode->refcount ++;

    child_proc->currentdir_vnode = p->currentdir_vnode;
    if (child_proc->currentdir_vnode)
      child_proc->currentdir_vnode->refcount ++;


    /*
     *	Duplicate the parent's virtual memory regions etc.:
     */

    tmp = vm_fork (p, child_proc);
    if (tmp)
      {
	/*  Remove all vm_objects references:  */
	region = child_proc->vmregions;
	while (region)
	  {
	    nextregion = region->next;
	    vm_region_detach (p, region->start_addr);
	    region = nextregion;
	  }

	/*  Decrease refcount of descriptors:  */
	for (i=0; i<child_proc->nr_of_fdesc; i++)
	  if (child_proc->fdesc[i])
	    child_proc->fdesc[i]->refcount--;

	p->nr_of_children --;
	interrupts (oldints);
	proc_remove (child_proc);
	return ENOMEM;
      }


    /*
     *	Machine dependant part of process struct needs to be
     *	copied, and then we need to return to both processes!
     */

    *res = machdep_fork (p, child_proc);

    need_to_pswitch = 1;
    interrupts (oldints);

    return 0;
  }

