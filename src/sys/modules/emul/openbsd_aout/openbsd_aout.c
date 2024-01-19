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
 *  modules/exec/openbsd_aout/openbsd_aout.c  --  OpenBSD a.out exec format
 *
 *  History:
 *	20 Jan 2000	test (nothing)
 *	18 Feb 2000	beginning
 */



#include "../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/errno.h>
#include <sys/emul.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/vnode.h>
#include <sys/syscalls.h>
#include <sys/interrupts.h>
#include <sys/os.h>


#include <sys/md/machdep.h>
#include <sys/md/gdt.h>

/*  TODO:  move to arch/i386:  */
extern size_t userland_startaddr;
extern volatile struct proc *curproc;
extern byte *gdt;
extern struct mcb *first_mcb;
extern size_t malloc_firstaddr;
struct module *openbsd_aout_m;
struct emul *openbsd_aout_emul;
extern char nodename[];
extern size_t default_stack_size;


struct openbsd_aout_header
      {
	u_int32_t	midmag;
	u_int32_t	text;
	u_int32_t	data;
	u_int32_t	bss;
	u_int32_t	syms;
	u_int32_t	entry;
	u_int32_t	trsize;
	u_int32_t	drsize;
      };

#define	NR_OF_SYSCALLS		264



int openbsd_aout_check_magic (byte *buffer, int length)
  {
    if (length<4)
	return 0;

    /*  ZMAGIC (?)  */
    if (buffer[0]==0 && buffer[1]==0x86 && buffer[2]==1 && buffer[3]==0xb)
	return 1;

    /*  Dynamic (??)  */
    if (buffer[0]==0x80 && buffer[1]==0x86 && buffer[2]==1 && buffer[3]==0xb)
	return 1;

    return 0;
  }



int openbsd_aout_loadexec (struct proc *p, struct vm_object *vmobj)
  {
    struct openbsd_aout_header header;
    off_t actually_read;
    int res;
    size_t text_begin, data_begin, bss_begin;
    struct vm_object *bss_object;


    if (!p || !vmobj || !vmobj->vnode)
	return EINVAL;

    res = vmobj->vnode->read (vmobj->vnode, (off_t) 0, &header, (off_t) sizeof(header),
	&actually_read);

    if (res || actually_read < sizeof(header))
	return res;

/*
printk ("openbsd_aout_loadexec: header dump:");
printk ("  midmag = %x", header.midmag);
printk ("  text   = %x", header.text);
printk ("  data   = %x", header.data);
printk ("  bss    = %x", header.bss);
printk ("  syms   = %x", header.syms);
printk ("  entry  = %x", header.entry);
printk ("  trsize = %x", header.trsize);
printk ("  drsize = %x", header.drsize);
*/

    if (header.entry != 0x1020)
      {
	printk ("openbsd_aout_loadexec(): header.entry=0x%x (should be 0x1020)",
		header.entry);
	return ENOEXEC;
      }


    /*
     *	Call vm_region_attach() to attach different parts of the vm_object to
     *	the process' address space:
     */

    text_begin = header.entry - (header.entry & (PAGESIZE-1));
    res = vm_region_attach (p, vmobj, 0, text_begin, text_begin+header.text-1,
	VMREGION_READABLE+VMREGION_EXECUTABLE);
    if (!res)
	return ENOMEM;

    data_begin = text_begin + header.text;
    res = vm_region_attach (p, vmobj, header.text, data_begin, data_begin+header.data-1,
	VMREGION_READABLE + VMREGION_WRITABLE + VMREGION_COW);
    if (!res)
	return ENOMEM;


    bss_begin = data_begin + header.data;
    bss_object = vm_object_create (VM_OBJECT_ANONYMOUS);
    if (!bss_object)
	return ENOMEM;

    res = vm_region_attach (p, bss_object, 0, bss_begin, bss_begin+header.bss-1,
	VMREGION_READABLE+VMREGION_WRITABLE);
    if (!res)
      {
	vm_object_free (bss_object);
	return ENOMEM;
      }

    /*  p->dataregion should be set to the bss region, so that
	sys_break() can increase its size later:  */
    p->dataregion = p->vmregions;
    if (p->dataregion->start_addr != bss_begin)
	panic ("openbsd_aout_loadexec: vm_region_attach didn't add region as expected");

    /*  Remove old pagedir and pagetable, and allocate memory for a new pagedir:  */
    if (!machdep_proc_freemaps (p))
      {
	vm_object_free (bss_object);
	return ENOMEM;
      }

    p->md.pagedir = (size_t *) malloc (PAGESIZE);
    if (!p->md.pagedir)
      {
	vm_object_free (bss_object);
	return ENOMEM;
      }
    machdep_pagedir_init (p);

    p->md.tss->cr3 = (u_int32_t) p->md.pagedir;

    return 0;
  }



int openbsd_aout_setupstackandhwregisters (struct proc *p)
  {
/*  TODO: fix, this is machine dependant and should be in arch/i386 somewhere  */

    int res;
    struct vm_object *stackobj;
    size_t stackaddr;

    /*  Create a stack region:  */
    stackobj = vm_object_create (VM_OBJECT_ANONYMOUS);
    if (!stackobj)
	return ENOMEM;

    /*  Where to place the stack in the process' virtual memory?  */
    stackaddr = (size_t) ((u_int64_t)0x100000000 - userland_startaddr);
    stackaddr -= default_stack_size;

    res = vm_region_attach (p, stackobj, 0, stackaddr, stackaddr+default_stack_size-1,
	VMREGION_READABLE+VMREGION_WRITABLE+VMREGION_STACK);
    if (!res)
      {
	vm_object_free (stackobj);
	return ENOMEM;
      }

    p->md.argvenvp_stackobj = stackobj;

    return 0;
  }



byte *openbsd_aout__addstackpage (struct vm_object *stackobj, size_t page_nr)
  {
    byte *a_page;
    struct mcb *a_mcb;
    size_t i;

    a_page = (byte *) malloc (PAGESIZE);
    if (!a_page)
	return NULL;
    memset (a_page, 0, PAGESIZE);
    i = ((size_t)a_page - malloc_firstaddr) / PAGESIZE;
    a_mcb = &first_mcb[i];

    a_mcb->size = MCB_VMOBJECT_PAGE;
    a_mcb->bitmap = 0;
    a_mcb->page_nr = page_nr;
    a_mcb->next = stackobj->page_chain_start;

    stackobj->page_chain_start = i;
    return a_page;
  }



int openbsd_aout__copystr (struct vm_object *stackobj, size_t *page_nr,
		int *offset_within_page, byte **a_page, char *str)
  {
    /*
     *	Copy the strings 'str' into the page referenced by a_page.
     *
     */

    int done=0;
    int strlength;
    int avail_on_curpage;

    /*  Let's count the trailing \0 as a char in the string:  */
    strlength = strlen (str) + 1;

    while (!done)
      {
	/*
	 *  Copy as much as we can in one memcpy:
	 */

	/*  strlength = how much of the string is left to be copied.  */
	/*  How much fits in the current page?  */
	avail_on_curpage = PAGESIZE - *offset_within_page;
	if (avail_on_curpage > strlength)
	  {
	    memcpy (*a_page + *offset_within_page, str, strlength);
	    (*offset_within_page) += strlength;
	    return 0;
	  }

	/*  Only copy as much as we can fit on this page:  */
	memcpy (*a_page + *offset_within_page, str, avail_on_curpage);
	(*offset_within_page) += avail_on_curpage;
	strlength -= avail_on_curpage;

	/*  We need to allocate another page:  */
	(*page_nr)++;
	(*a_page) = openbsd_aout__addstackpage (stackobj, *page_nr);
	*offset_within_page = 0;

	/*  Nothing more to copy on this string?  */
	if (strlength < 1)
	  done = 1;
      }

    return 0;
  }



int openbsd_aout_setupargvenvp (struct proc *p, char *argv[], char *envp[])
  {
    /*
     *	The arguments and environment strings are stored like this:
     *
     *		| argc | argptrs | envptrs | argstrs | envstrs |
     *		+------+---------+---------+---------+---------+
     *		(low address)			  (high address)
     *
     *	The esp register should point at 'argc' when the program starts.
     *
     *	where:	int argc is the number of argptrs-1 (because the last argptr
     *			should be NULL)
     *		argptrs is a number of char *-ers pointing to individual
     *			strings in argstrs.
     *		envptrs is a number of char *-ers pointing to individual
     *			strings in envstrs.
     *
     *	Both argptrs and envptrs end with a NULL entry.
     *
     *	So, the total length of all the data we need to put on the stack
     *	is 4 + 4*(nr_of_argptrs) + 4*(nr_of_envptrs) +
     *		+ length_of_argstrs + length_of_envstrs.
     *
     *	The total length of all data is rounded up to the nearest dword
     *	(4 byte boundary).
     *
     *	We need to allocate new stack pages and insert them into the
     *	process' stack vm_object's pagechain, and we need to fill them
     *	with the data.
     */

    int i, argc, envcount;
    int ae_length, length_of_argstrs, length_of_envstrs;
    int nr_of_pages;
    int offset_within_page;
    size_t page_nr;
    u_int32_t address;
    byte *a_page;


    if (!argv || !p || !envp)
	return EINVAL;


    /*
     *	Calculate length_of_argstrs, length_of_envstrs, and the
     *	total length ae_length:
     */

    i = 0; length_of_argstrs = 0;
    while (argv[i])
      {
	length_of_argstrs += (strlen(argv[i]) + 1);
	i++;
      }
    argc = i;

    i = 0; length_of_envstrs = 0;
    while (envp[i])
      {
	length_of_envstrs += (strlen(envp[i]) + 1);
	i++;
      }
    envcount = i;

/*
printk ("length_of_env=%i arg=%i", length_of_envstrs, length_of_argstrs);
*/

    ae_length = 4 + 4*(argc+1) + 4*(envcount+1) + length_of_argstrs + length_of_envstrs
	+ 4;   /*  the last +4 is to have one dword marginal...  */
    if (ae_length & 3)
	ae_length |= 3, ae_length++;

/*
printk ("ae_length = %i", ae_length);
*/

    /*  ae_length is in bytes. Convert to # of pages:  */
    nr_of_pages = ae_length/PAGESIZE;
    if (ae_length & (PAGESIZE-1))
	nr_of_pages++;

    /*  Offset within starting page:  */
    offset_within_page = nr_of_pages*PAGESIZE - ae_length;

    page_nr = (default_stack_size/PAGESIZE)-nr_of_pages;
    a_page = openbsd_aout__addstackpage (p->md.argvenvp_stackobj, page_nr);


    /*
     *	Insert the data into a_page. If the end of it is reached,
     *	we simply create "the next page" and continue there.
     */

    /***  argc  ***/
    memcpy (a_page+offset_within_page, &argc, 4);
    offset_within_page += 4;
    if (offset_within_page >= PAGESIZE)
      {
	page_nr++;
	a_page = openbsd_aout__addstackpage (p->md.argvenvp_stackobj, page_nr);
	offset_within_page = 0;
      }

    /***  argv[i] pointers, all i  ***/
    address = 0 - userland_startaddr - ae_length + 4
	+ 4*(argc+1) + 4*(envcount+1);
    for (i=0; i<=argc; i++)
      {
	if (i==argc)
	  address = 0;
	memcpy (a_page+offset_within_page, &address, 4);
	offset_within_page += 4;
	if (offset_within_page >= PAGESIZE)
	  {
	    page_nr++;
	    a_page = openbsd_aout__addstackpage (p->md.argvenvp_stackobj, page_nr);
	    offset_within_page = 0;
	  }
	if (argv[i])
	  address += (strlen(argv[i]) + 1);
      }

    /***  envp[i] pointers, all i  ***/
    address = 0 - userland_startaddr - ae_length + 4
	+ 4*(argc+1) + 4*(envcount+1) + length_of_argstrs;
    for (i=0; i<=envcount; i++)
      {
	if (i==envcount)
	  address = 0;
	memcpy (a_page+offset_within_page, &address, 4);
	offset_within_page += 4;
	if (offset_within_page >= PAGESIZE)
	  {
	    page_nr++;
	    a_page = openbsd_aout__addstackpage (p->md.argvenvp_stackobj, page_nr);
	    offset_within_page = 0;
	  }
	if (envp[i])
	  address += (strlen(envp[i]) + 1);
      }

    /*  All argv and envp strings:  */
    for (i=0; i<argc; i++)
	openbsd_aout__copystr (p->md.argvenvp_stackobj, &page_nr,
		&offset_within_page, &a_page, argv[i]);
    for (i=0; i<envcount; i++)
	openbsd_aout__copystr (p->md.argvenvp_stackobj, &page_nr,
		&offset_within_page, &a_page, envp[i]);

    p->md.size_of_argvenvp = ae_length;
    return 0;
  }



int openbsd_aout_startproc (struct proc *p)
  {
    /*
     *	Hardcoded jump to new process...
     */

    interrupts (DISABLE);

    /*  Turn off busy bit of dummy process:  */
    gdt [SEL_DUMMYTSS + 5] &= 0xf0;
    gdt [SEL_DUMMYTSS + 5] |= 9;

    i386_ltr (SEL_DUMMYTSS/8);

    /*  Turn off busy bit of process p:  */
    gdt [p->md.tss_nr*8 + 5] &= 0xf0;
    gdt [p->md.tss_nr*8 + 5] |= 9;

    p->md.tss->eax = 0;
    p->md.tss->ebx = 0;
    p->md.tss->ecx = 0;
    p->md.tss->edx = 0;
    p->md.tss->ebp = 0;
    p->md.tss->esi = 0;
    p->md.tss->edi = 0;
    p->md.tss->eip = 0x1020;
    p->md.tss->eflags = 0x00000202;
    p->md.tss->cs = SEL_USERCODE + 3;
    p->md.tss->ss = SEL_USERDATA + 3;
    p->md.tss->ds = SEL_USERDATA;
    p->md.tss->es = SEL_USERDATA;
    p->md.tss->esp = (size_t) ((u_int64_t)0x100000000 -
	userland_startaddr) - p->md.size_of_argvenvp;

    curproc = NULL;
    machdep_pswitch (p);

printk ("openbsd_aout_startproc: not reached?");
    return 0;
  }



#include "openbsd_aout_syscalls.c"


void openbsd_aout_init (int arg)
  {
    void **s;

    openbsd_aout_m = module_register ("emul", 0, "openbsd_aout", "OpenBSD a.out exec format");

    if (!openbsd_aout_m)
	return;

    openbsd_aout_emul = emul_register ("openbsd_aout");
    if (!openbsd_aout_emul)
      {
	module_unregister (openbsd_aout_m);
	return;
      }


    openbsd_aout_emul->check_magic = openbsd_aout_check_magic;
    openbsd_aout_emul->loadexec = openbsd_aout_loadexec;
    openbsd_aout_emul->setupstackandhwregisters = openbsd_aout_setupstackandhwregisters;
    openbsd_aout_emul->setupargvenvp = openbsd_aout_setupargvenvp;
    openbsd_aout_emul->startproc = openbsd_aout_startproc;

    openbsd_aout_emul->highest_syscall_nr = NR_OF_SYSCALLS;
    s = (void **) malloc (sizeof(void *)*NR_OF_SYSCALLS);
    memset (s, 0, sizeof(void *)*NR_OF_SYSCALLS);
    openbsd_aout_emul->syscall = s;
    openbsd_aout_syscalls_init (s);

    openbsd_aout_emul->active = 1;
  }

