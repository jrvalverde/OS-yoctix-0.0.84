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
 *  arch/i386/proc.c  --  i386 specific process handling functions
 *
 *	The i386 processor family has a multitasking concept based around
 *	something called "tasks". Each task is described by a "tss" -- task
 *	state segment. If you are not familiar with the tss concept, you
 *	should read one of Intel's books on the subject, for example
 *	"80386 System Software Writers Guide".
 *
 *	Unix processes can be mapped into the i386 "task" representation --
 *	this is the way I do it right now -- but this is not neccessary. A
 *	different approach would be to use "software processes" where the CPU's
 *	register copying functions etc have to be emulated in software. There
 *	are good and bad aspects on both approaches.
 */


#include "../config.h"
#include <sys/proc.h>
#include <string.h>
#include <sys/malloc.h>
#include <sys/std.h>
#include <sys/errno.h>
#include <sys/vm.h>
#include <sys/interrupts.h>
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/proc.h>
#include <sys/arch/i386/gdt.h>


extern volatile struct proc *procqueue [];
extern volatile struct proc *curproc;
extern u_int32_t *i386_kernel_pagedir;
extern size_t userland_startaddr;
extern byte *gdt;


/*  See task.c  */
void i386_settask (int nr, struct i386tss *tss);
void i386_ltr (int nr);
void i386_jmptss (int nr);


/*  A hint for finding the next free TSS slot in the GDT:  */
int next_free_tss = I386_FIRSTTSS;



void machdep_pagedir_init (struct proc *p)
  {
    /*
     *	machdep_pagedir_init ()
     *	-----------------------
     *
     *
     *	Initialize the pagedir of a process. The first entries of the pagedir
     *	should be mapped to the physical RAM.
     */

    size_t *pagedir;
    int i;

    if (!p)
	return;

    if (!(pagedir = p->md.pagedir))
	return;

    memset (pagedir, 0, PAGESIZE);
    i = userland_startaddr/(4*1024*1024);
    while (i>0)
      {
	i--;
	pagedir[i] = i386_kernel_pagedir[i];
      }
  }



int machdep_proc_init (struct proc *p)
  {
    /*
     *	machdep_proc_init ()
     *	--------------------
     *
     *	Allocate i386 specific process structures, and "insert" them
     *	into the process 'p'.
     *
     *	Returns 1 on success, 0 on failure.
     */

    struct i386tss *tss;
    u_int32_t *pagedir;
    void *kstack;
    int i, found;


    /*	Is there actually any speed increase to use a
	whole page for the TSS??? Probably not... */
    tss = (struct i386tss *) malloc (sizeof(struct i386tss));
    if (!tss)
	return 0;

    pagedir = (u_int32_t *) malloc (PAGESIZE);
    if (!pagedir)
      {
	free (tss);
	return 0;
      }

    kstack = (void *) malloc (KSTACK_SIZE);
    if (!kstack)
      {
	free (pagedir);
	free (tss);
	return 0;
      }

    p->md.tss = tss;
    p->md.pagedir = pagedir;
    p->md.kstack = kstack;


    /*
     *  Initialize the tss
     *	==================
     *
     *	Flags need to have IF (Interrupts Enable) turned on when switching
     *	between multiple tasks.
     *
     *	cs,ds,es,ss actually use the same address range, but different
     *	segments because data is not executable and so on. The "+3" in cs
     *	and ss are there to allow the process to run at all. (This is one
     *	of the worst "bugs" I have encountered when dealing with this stuff.)
     *
     *	The process' cr3 register should point to its pagedir.
     *
     *	ss0:esp0 should point to the process' kernel stack, which is used
     *	whenever the process needs to jump into kernel mode (syscalls,
     *	timer interrupts etc).
     */

    memset (tss, 0, sizeof (struct i386tss));
    tss->eflags = 0x00000202;
    tss->ds = SEL_USERDATA;
    tss->es = SEL_USERDATA;
    tss->ss = SEL_USERDATA + 3;
    tss->cs = SEL_USERCODE + 3;
    tss->cr3 = (u_int32_t) pagedir;
    tss->ss0 = SEL_DATA;
    tss->esp0 = (u_int32_t) kstack + KSTACK_SIZE - KSTACK_MARGIN;


    /*
     *  Initialize the page directory.
     */

    machdep_pagedir_init (p);


    /*  Find a free TSS slot (present bit == 0):  */
    i = next_free_tss;
    found = 0;
    while (!found)
      {
	if ((gdt[8*i+5] & 128) == 0)
	  found = i;
	else
	  {
	    i++;
	    if (i >= MAX_PROCESSES+I386_FIRSTTSS)
		i = I386_FIRSTTSS;
	    if (i == next_free_tss)
		panic ("machdep_proc_init(): no free TSS slots in GDT");
	  }
      }

/*
    printk ("using tss %i", found);
*/

    p->md.tss_nr = found;
    i386_settask (found, tss);

    next_free_tss = found + 1;
    if (next_free_tss >= MAX_PROCESSES+I386_FIRSTTSS)
	next_free_tss = I386_FIRSTTSS;

    return 1;
  }



int machdep_proc_freemaps (struct proc *p)
  {
    /*
     *	Free memory occupied by process p's pagetables
     *	and page directory.
     *	Returns 1 on success, 0 on failure.
     */

    int i;
    size_t *pagedir, *pagetable;

    pagedir = p->md.pagedir;
    if (pagedir)
      {
	for (i=(userland_startaddr/(4*1024*1024)); i<1024; i++)
	  {
	    pagetable = (size_t *) ((u_int32_t)pagedir[i] & 0xfffff000);
	    if (pagetable)
		free (pagetable);
	  }
	free (pagedir);
	p->md.pagedir = NULL;
      }

    return 1;
  }



int machdep_proc_remove (struct proc *p)
  {
    /*
     *	machdep_proc_remove ()
     *	----------------------
     *
     *	Free the 'md' part of p.
     *	Returns 1 on success, 0 on failure.
     */

    if (!p)
	return 0;

    /*  Free the Task State Segment:  */
    free (p->md.tss);

    /*  Free pagedir and all pagetables:  */
    machdep_proc_freemaps (p);

    /*  Free the kernel stack associated with this process:  */
    free (p->md.kstack);

    /*  Mark the TSS entry in the GDT as not-present:  */
    gdt [8*p->md.tss_nr+5] &= 127;

    return 1;
  }



void machdep_setkerneltaskaddr (void *kernelcode)
  {
    /*
     *	Ugly hack: this function sets up a dummy tss with
     *	kernel privileges (using eip == kernelcode) and jumps
     *	to it.
     *
     *	TODO:  This function is probably not used anymore...
     */

    static struct i386tss *dummy_tss = NULL;
    static byte *dummy_stack = NULL;

    if (!dummy_tss)
	dummy_tss = (struct i386tss *) malloc (sizeof(struct i386tss));

    if (!dummy_stack)
	dummy_stack = (byte *) malloc (KSTACK_SIZE);

    memset (dummy_tss, 0, sizeof(struct i386tss));
    dummy_tss->eflags = 0x00000002;
    dummy_tss->cs   = SEL_CODE;
    dummy_tss->ds   = SEL_DATA;
    dummy_tss->es   = SEL_DATA;
    dummy_tss->ss   = SEL_DATA;
    dummy_tss->esp  = (u_int32_t) dummy_stack + KSTACK_SIZE - KSTACK_MARGIN;
    dummy_tss->ss0  = SEL_DATA;
    dummy_tss->esp0 = (u_int32_t) dummy_stack + KSTACK_SIZE - KSTACK_MARGIN;
    dummy_tss->cr3  = (u_int32_t) i386_kernel_pagedir;
    dummy_tss->eip  = (u_int32_t) kernelcode;

    i386_settask (SEL_DUMMYTSS/8, dummy_tss);
    i386_jmptss (SEL_DUMMYTSS/8);
  }



void machdep_pswitch (struct proc *p)
  {
    /*
     *	machdep_pswitch ()
     *	------------------
     *
     *	Set curproc to the new process 'p' and jump to it.
     *	(Only switch to the new process if it is not already running.)
     */

    if (curproc != p)
      {
	curproc = p;
	i386_jmptss (p->md.tss_nr);
      }
  }



int machdep_fork (struct proc *parent, struct proc *child)
  {
    /*
     *	machdep_fork ()
     *	---------------
     *
     *	Super-ugly hack, because I don't know how to do this any other
     *	way. Anyway, the point of this function is to do the actual
     *	low-level split-up of the parent process and return the child PID
     *	to the parent and zero to the child.
     *
     *	Should be called with interrupts disabled.
     */

    u_int32_t *parentstack;

    /*
     *	Re-setup child's tss to point to child's userland; restore
     *	registers to what they were when the parent fork() syscall was
     *	issued, but set eax to zero.
     */

    memset (child->md.tss, 0, sizeof(struct i386tss));
    parentstack = (u_int32_t *) parent->md.tss->esp0;

    child->md.tss->esp0 = (u_int32_t) child->md.kstack + KSTACK_SIZE - KSTACK_MARGIN;
    child->md.tss->ss0 = SEL_DATA;
    child->md.tss->cr3 = (u_int32_t) child->md.pagedir;
    child->md.tss->eip = parentstack[-5];
    child->md.tss->eflags = parentstack[-3] & 0xfffffffe;
    child->md.tss->eax = 0;	/*  parent's eax is at parentstack[-6], but   */
				/*  we want to return 0 to the child process  */
    child->md.tss->ecx = parentstack[-8];
    child->md.tss->edx = parentstack[-9];
    child->md.tss->ebx = parentstack[-7];
    child->md.tss->esp = parentstack[-2];
    child->md.tss->ebp = parentstack[-10];
    child->md.tss->esi = parentstack[-11];
    child->md.tss->edi = parentstack[-12];
    child->md.tss->ds = (u_int16_t) parentstack[-13];
    child->md.tss->es = (u_int16_t) parentstack[-14];
    child->md.tss->fs = (u_int16_t) parentstack[-15];
    child->md.tss->gs = (u_int16_t) parentstack[-16];
    child->md.tss->cs = (u_int16_t) parentstack[-4];
    child->md.tss->ss = (u_int16_t) parentstack[-1];


    /*  Add child to runqueue:  */
    if (!runqueue)
      {
	runqueue = child;
	child->next = child;
	child->prev = child;
      }
    else
      {
	child->next = runqueue->next;
	child->prev = (struct proc *) runqueue;
	runqueue->next->prev = child;
	runqueue->next = child;
      }

    return child->pid;
  }


