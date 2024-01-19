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
 *  kern/sys_execve.c  --  replace a process' contents by new contents
 *			read from a file
 *
 *  History:
 *	20 Jan 2000	test (nothing)
 *	18 Feb 2000	continuing
 *	25 Feb 2000	can execute /sbin/init using openbsd_aout emulation,
 *			but it's far from perfect
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
#include <fcntl.h>


#define	HEADER_SIZE	16

extern size_t userland_startaddr;

extern volatile int need_to_pswitch;

extern volatile struct proc *curproc;
extern volatile struct proc *procqueue[];

extern struct emul *first_emul;

extern struct timespec system_time;



int sys_execve (ret_t *retval, struct proc *p, char *path, char *argv[],
		char *envp[])
  {
    /*
     *	sys_execve ()
     *	-------------
     *
     *	Get a pointer to a vnode that represents the file's inode in memory.
     *	(vnode_lookup() automatically calls vnode_create() if it is not
     *	already in memory, so a NULL return from vnode_lookup() is a really
     *	serious error.)
     *
     *	Is the file executable?  (If not, return error.)
     *	Is the file opened for writing by someone?  (Then return an error.)
     *
     *	Remember the old flags of the vnode, and then set the EXECUTING flag.
     *
     *	Figure out what the file's exec format is (openbsd_aout, linux_elf etc).
     *	If the format is unknown, restore the flags and return an error.
     *
     *	Create an empty vm_object and point it to the file. Increase the
     *	vnode's refcount.
     *
     *	Copy the original process' argv and envp data into kernel memory.
     *
     *	Call the exec format handler to read the vnode's header and map
     *	text and data regions to the process. (Each of these attachments of
     *	regions automatically increase the vm_object's refcount, so there's
     *	no need to do it manually.)  The original process' vmregion chain
     *	is "backed up", so that it can be restored in case the new region
     *	mapping fails. If everything is successfull, the backed up chain
     *	is freed from memory.
     *
     *	---  Before this point, the old process still exists, and should
     *	     get the error code produced by an error. Below, the old process
     *	     is no more...
     *
     *	Next, a stack region and the argv/envp data should be setup.
     *	Also, any hardware registers should be setup. (This is machine dependant.)
     *
     *	Free the argv_backup and envp_backup chains.
     *	Reset iticks, sticks, uticks of the process.
     *	Place the process on the run queue.
     *
     *	Now, the "new" process should be ready to run.
     */

    int e=0, res, n, i;
    struct vnode *programvnode;
    byte program_header [HEADER_SIZE];
    off_t actually_read;
    struct emul *emul, *found;
    struct vm_object *vmobj;
    struct vm_region *region_backup;
    struct vm_region *region_to_free;
    int oldints;
    char **argv_backup = NULL;
    int nr_argv_backup;
    char **envp_backup = NULL;
    int nr_envp_backup;
    char *tmpcharp;
    int tmplen;


    if (!p || !retval)
	return EINVAL;

    p->status = P_IDL;
    *retval = 0;


/*  TODO:  check parameters:  */

    path = (char *)  ((size_t)path + userland_startaddr);
    argv = (char **) ((size_t)argv + userland_startaddr);
    envp = (char **) ((size_t)envp + userland_startaddr);


    /*
     *	Get a pointer to a vnode that represents the file's inode in memory.
     *	(vnode_lookup() automatically calls vnode_create() if it is not
     *	already in memory, so a NULL return from vnode_lookup() is a really
     *	serious error.)
     */

/*  TODO:  full path?  */
    programvnode = vnode_lookup (p, path, &e, "sys_execve");
    if (!programvnode)
	return e;


    /*
     *	Is the file executable?  (If not ==> return errorcode)
     *	Is the file opened for writing by someone?  (If not ==> return errorcode)
     *	Or is it opened for exclusive access?
     */

/*  TODO: is this a regular file? if not, then abort...  */

/*  TODO:  Check for all users! Now we only check if the OWNER-X flag is set!!!!!!!  */
    if ((programvnode->ss.st_mode & S_IXUSR) == 0)
      {
	unlock (&programvnode->lock);
	return EACCES;
      }

    if (programvnode->refcount_write > 0 || (programvnode->status & VNODE_EXCLUSIVE))
      {
	unlock (&programvnode->lock);
	return ETXTBSY;
      }

    programvnode->refcount_exec ++;


    /*
     *	Figure out what the file's exec format is (openbsd_aout, linux_elf etc).
     *	If the format is unknown, restore the flags and return an error.
     */

    /*  Read the file's header into the 'program_header' buf:  */
    res = programvnode->read (programvnode, (off_t) 0, program_header, (off_t) HEADER_SIZE, &actually_read);
    if (res || actually_read < HEADER_SIZE)
      {
	programvnode->refcount_exec --;
	unlock (&programvnode->lock);
	return res;
      }


    /*  Find an emulation which understands the header:  */
    emul = first_emul;
    found = NULL;
    while (emul)
      {
	if (emul->check_magic (program_header, HEADER_SIZE))
	  {
	    found = emul;
	    emul = NULL;
	  }
	else
	  emul = emul->next;
      }
    emul = found;
    if (!emul)
      {
	programvnode->refcount_exec --;
	unlock (&programvnode->lock);
	return ENOEXEC;
      }

#if DEBUGLEVEL>=4
    printk ("  execve(): emulation found: '%s'", emul->name);
#endif


    /*
     *	Create an empty vm_object and point it to the file. Increase the
     *	vnode's refcount.
     */

    if (programvnode->vmobj)
      {
	vmobj = programvnode->vmobj;
      }
    else
      {
	vmobj = vm_object_create (VM_OBJECT_FILE);
	if (!vmobj)
	  {
	    programvnode->refcount_exec --;
	    unlock (&programvnode->lock);
	    return ENOMEM;
	  }
	vmobj->vnode = programvnode;
	programvnode->vmobj = vmobj;
      }

    programvnode->refcount++;

#if DEBUGLEVEL>=4
    printk ("  execve(): vnode: filename='%s' hash=%x refcount=%i",
	programvnode->filename, programvnode->hash, programvnode->refcount);
#endif


    /*
     *	Copy the original process' argv and envp data into kernel memory.
     */

    /*  How many argv entries shall we copy?  */
    n = 0;
    while (argv[n])
	n++;

    if (n>0)
      {
	argv_backup = (char **) malloc (sizeof(char *) * (n+1));
	if (!argv_backup)
	  panic ("sys_execve: TODO  (malloc problem 1)");
	argv_backup[n] = NULL;

	/*  Copy each argument to kernel memory:  */
	for (i=0; i<n; i++)
	  {
	    tmpcharp = argv[i];

	    /*  If no process is running yet, then we're in pure kernel land,
		otherwise the addresses are userland-ish:  (curproc is NULL
		before any user process has started)   [ugly hack]  */
	    if (curproc)
		tmpcharp = (char *) ((size_t)tmpcharp + userland_startaddr);

	    tmplen = strlen(tmpcharp);
	    argv_backup[i] = (char *) malloc (tmplen+1);
	    if (!argv_backup[i])
		panic ("sys_execve: TODO  (malloc problem 2)");
	    memcpy (argv_backup[i], tmpcharp, tmplen+1);
/*	    argv_backup[i][tmplen]=0; */
	  }
      }

    nr_argv_backup = n;

    /*  How many envp entries shall we copy?  */
    n = 0;
    while (envp[n])
	n++;

    if (n>0)
      {
	envp_backup = (char **) malloc (sizeof(char *) * (n+1));
	if (!envp_backup)
	  panic ("sys_execve: TODO  (malloc problem 3)");
	envp_backup[n] = NULL;

	/*  Copy each environment string to kernel memory:  */
	for (i=0; i<n; i++)
	  {
	    tmpcharp = envp[i];

	    /*  If no process is running yet, then we're in pure kernel land,
		otherwise the addresses are userland-ish:  (curproc is NULL
		before any user process has started)   [ugly hack]  */
	    if (curproc)
		tmpcharp = (char *) ((size_t)tmpcharp + userland_startaddr);

	    tmplen = strlen(tmpcharp);
	    envp_backup[i] = (char *) malloc (tmplen+1);
	    if (!envp_backup[i])
		panic ("sys_execve: TODO  (malloc problem 4)");
	    memcpy (envp_backup[i], tmpcharp, tmplen+1);
	  }
      }

    nr_envp_backup = n;


    /*
     *	Call the exec format handler to read the vnode's header and map
     *	text and data regions to the process. (Each of these attachments of
     *	regions automatically increase the vm_object's refcount, so there's
     *	no need to do it manually.)  The original process' vmregion chain
     *	is "backed up", so that it can be restored in case the new region
     *	mapping fails. If everything is successfull, the backed up chain
     *	is freed from memory.
     */

    region_backup = p->vmregions;
    p->vmregions = NULL;


    /*  BEFORE THIS, the old process can be returned to...
	but if emul->loadexec() succeeds, then the process
	address space is (or should be) mapped to nothing...  */

    res = emul->loadexec (p, vmobj);

    if (res)
      {
	/*  Undo everything so that we can return to the old process:  */

printk ("  execve(): loadexec() failed: %i", res);

	p->vmregions = region_backup;
	programvnode->refcount_exec --;
	programvnode->refcount --;

	i=0;
	while (argv_backup[i])  free (argv_backup[i++]);
	free (argv_backup);
	i=0;
	while (envp_backup[i])  free (envp_backup[i++]);
	free (envp_backup);

	free (vmobj);
	unlock (&programvnode->lock);
	return res;
      }


    /*
     *	Free the region_backup chain from memory.
     */

    while (region_backup)
      {
	vm_object_free (region_backup->source);
	region_to_free = region_backup;
	region_backup = region_backup->next;
	free (region_to_free);
      }


    unlock (&programvnode->lock);


    /*
     *	Close any file descriptors that have the close-on-exec flag set:
     */

    for (i=0; i<p->nr_of_fdesc; i++)
      if (p->fcntl_dflag[i] & FD_CLOEXEC)
	fd_detach (p, i);


    /*
     *	Next, a stack region and the argv/envp data should be setup.
     *	Also, any hardware registers should be setup.
     *	(This is machine dependant.)
     *
     *	If we fail here, there is no process to return to so we will
     *	just have to kill what's left of the process.
     */

    res = emul->setupstackandhwregisters (p);
    if (res)
      {
	panic ("sys_execve: fail 1. TODO: kill the process");
      }

    res = emul->setupargvenvp (p, argv_backup, envp_backup);
    if (res)
      {
	panic ("sys_execve: fail 2. TODO: kill the process");
      }

    p->emul = emul;


    /*
     *	Free the argv_backup and envp_backup chains:
     */

    i=0;
    while (argv_backup[i])  free (argv_backup[i++]);
    free (argv_backup);
    i=0;
    while (envp_backup[i])  free (envp_backup[i++]);
    free (envp_backup);


    /*
     *	Reset iticks, sticks, uticks of the process.
     *	Place the process on the run queue.
     */

    oldints = interrupts (DISABLE);

    p->iticks = 0;
    p->sticks = 0;
    p->uticks = 0;
    p->ticks = &p->uticks;


    /*
     *	Special case if this is the first execve call the kernel makes:
     *	manually put the process on the runqueue. If there is already a run-
     *	queue then we must be in it (since we're running).
     */
    if (!runqueue)
      {
	runqueue = p;
	p->next = p;
	p->prev = p;
      }

    p->status = P_RUN;
    p->flags |= PFLAG_CALLEDEXEC;

    /*  TODO: maybe this shouldn't be here... the process already
	existed, we have only changed its executable image...  */
    /*  also note that interrupts need to be disabled for system_time  */
    p->creattime = system_time;

/*    interrupts (oldints); */

    p->emul->startproc (p);

    panic ("sys_execve: not reached");
    return 0;
  }

