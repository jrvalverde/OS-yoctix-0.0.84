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
 *  vm/vm_fault.c  --  page fault handler
 *
 *	vm_fault ()
 *		The page fault handler, called from machine dependant
 *		low level code.  Should try to make the faulting page
 *		available, or kill the process.
 *
 *  History:
 *	25 Feb 2000	first version
 *	16 Apr 2000	rewriting most of it...
 *	24 May 2000	finnishing rewrite begun on 16 Apr
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/md/machdep.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/defs.h>
#include <sys/interrupts.h>
#include <sys/lock.h>



extern struct mcb *first_mcb;		/*  physical addr of first mcb  */
extern size_t malloc_firstaddr;

volatile static int invmfault = 0;
struct lockstruct vm_fault_lock;



void vm_fault (struct proc *p, size_t virtualaddr, int action)
  {
    /*
     *	vm_fault:  The page fault handler
     *	---------------------------------
     *
     *	1.  Go through the process' vm_region chain to find a region
     *	    where the virtualaddr belongs.
     *
     *	2.  Find the page offset within the correct region.
     *
     *	3.  Use the page offset to find the mcb describing the page
     *	    by traversing the vm_region's sources (vm_objects).
     *
     *	Either a page was found in a vm_object's page chain, or no
     *	such page (and therefore no vm_object) was found.
     *
     *	Depending on the region's type and on the action wanted (read,
     *	write, ...) we'll do one of the following:
     *
     *	A:  If the page was not available, it has to be made available.
     *	(NOTE! Copy-on-write means that the page will be marked read-only!)
     *
     *	    1)	If the lowest-level object is a FILE object, then page-in
     *		at the lowest-level object (the FILE object) and mark the
     *		page as read-only or read-write depending on the region's
     *		access bits. Return.
     *
     *	    2)	If the lowest-level object is anonymous, then page-in the
     *		page at the highest-level object, mark the page as
     *		read-only or read-write depending on the region's
     *		access bits.
     *
     *	B:  The page exists. If the region is Copy-On-Write, and the
     *	    action was to 'write' then this means that we will have to
     *	    duplicate the page, and insert the new page in the
     *	    page chain of the vm_region's first source. This first
     *	    source should be a shadow vm_object. If it isn't (doesn't
     *	    exist) then we'll have to create it.
     */


    struct vm_region *region, *found_region;
    struct vm_object *vmobj, *found_vmobj, *new_vmobj;
    size_t offset_within_region, pagenumber;
    byte *a_page = NULL, *b_page = NULL;
    int res;
    int pmapflags;
    size_t mcb_index, found_mcb_index;
    struct mcb *a_mcb;
    size_t i;


    invmfault ++;

/*  MEGA-LOCK:  */
lock (&vm_fault_lock, "vm_fault", LOCK_BLOCKING | LOCK_RW);


    /*
     *	1.  Go through the process' vm_region chain to find a region
     *	    where the virtualaddr belongs.
     */

    region = p->vmregions;
    if (!region)
	panic ("vm_fault(): current process has no vmregions");

    found_region = NULL;
    while (region)
      {
	if (virtualaddr >= region->start_addr && virtualaddr <= region->end_addr)
	  {
	    found_region = region;
	    region = NULL;
	  }
	else
	  region = region->next;
      }

    region = found_region;

    if (!region)
      {
	unlock (&vm_fault_lock);
	printk ("vm_fault(): no matching region for %x...", virtualaddr);
	sig_post (p, p, SIGSEGV);
pswitch();
	goto vm_fault_return;
      }

#if DEBUGLEVEL>=5
    printk ("f addr %x region %x..%x type=%i action=%i invmfault=%i",
	virtualaddr, region->start_addr, region->end_addr, region->type, action, invmfault);
#endif

    if ((action & VM_PAGEFAULT_READ) && (region->type & VMREGION_READABLE)==0)
      {
	unlock (&vm_fault_lock);
	printk ("vm_fault(): trying to read from read protected region");
	sig_post (p, p, SIGSEGV);
pswitch();
	goto vm_fault_return;
      }

    if ((action & VM_PAGEFAULT_WRITE) && (region->type & VMREGION_WRITABLE)==0
		&& (region->type & VMREGION_COW)==0)
      {
	unlock (&vm_fault_lock);
	printk ("vm_fault(): trying to write to write protected region");
	sig_post (p, p, SIGSEGV);
pswitch();
	goto vm_fault_return;
      }


    /*
     *	2.  Find the offset within the correct region.
     */

    offset_within_region = virtualaddr - region->start_addr + region->srcoffset;
    pagenumber = offset_within_region / PAGESIZE;


    /*
     *	3.  Use the page offset to find the mcb describing the page
     *	    by traversing the vm_region's source chain (vm_objects).
     */

    /*  Start with the first source for this vm_region ...  */
    vmobj = region->source;
    found_vmobj = NULL;
    found_mcb_index = 0;

    /*  ... and scan through vm_objects until we find the page:  */
    while (vmobj)
      {
	/*
	 *  Scan through all the pages of this vm_object:
	 *  (TODO:  this is extremely inefficient, should be
	 *	    hashed or something...)
	 */
	mcb_index = vmobj->page_chain_start;
	while (mcb_index != 0)
	  {
	    a_mcb = &first_mcb[mcb_index];

	    /*  Is this the page we're looking for?  */
	    if (a_mcb->size == MCB_VMOBJECT_PAGE)
	      {
		if (a_mcb->page_nr == pagenumber)
		  {
		    found_vmobj = vmobj;
		    found_mcb_index = mcb_index;
		    a_mcb = NULL;	/*  (to break the scan)  */
		    vmobj = NULL;
		  }
	      }
	    else
	      panic ("vm_fault(): non-vm_object mcbs in vm_object page chain!");

	    /*  go to the next mcb... until the end of the chain (0):  */
	    if (a_mcb)
		mcb_index = a_mcb->next;
	    else
		mcb_index = 0;
	  }

	/*  Try next vmobj, if there are any more...  */
	if (vmobj)
	  vmobj = vmobj->next;
      }

    vmobj = found_vmobj;


    /*
     *	A:  If the page was not available, it has to be made available.
     *	(NOTE! Copy-on-write means that the page will be marked read-only!)
     *
     *	    1)	If the lowest-level object is a FILE object, then page-in
     *		at the lowest-level object (the FILE object) and mark the
     *		page as read-only or read-write depending on the region's
     *		access bits. Return.
     *
     *	    2)	If the lowest-level object is anonymous, then page-in the
     *		page at the highest-level object, mark the page as
     *		read-only or read-write depending on the region's
     *		access bits. Return.
     */

    if (!vmobj)
      {
	/*  Find the last vm_object in the vm_region's source chain:  */
	vmobj = region->source;
	while (vmobj->next)
	  vmobj = vmobj->next;

	/*  Call the vm_object's pagein function:  */
	a_page = (byte *) vmobj->pagein (&res, region, vmobj,
			virtualaddr & ~(PAGESIZE-1));

	if (!a_page)
	  panic ("vm_fault(): !a_page  TODO");

#if DEBUGLEVEL>=5
    printk ("res = %i, a_page:  %y%y%y%y %y%y%y%y %y%y%y%y %y%y%y%y", res,
	a_page[0], a_page[1], a_page[2], a_page[3],
	a_page[4], a_page[5], a_page[6], a_page[7],
	a_page[8], a_page[9], a_page[10], a_page[11],
	a_page[12], a_page[13], a_page[14], a_page[15]);
#endif

	/*  Is this an ANONYMOUS object, ie not a FILE object?  */
	if (vmobj->type == VM_OBJECT_ANONYMOUS)
	  {
	    /*  Then we shall insert the newly paged-in page as
		"close" to the process as possible.  */
	    vmobj = region->source;
	  }

	/*  Insert this into the vmobj's page chain:  */
	/*  (first, let's find the address to "a_page"'s mcb)  */
	i = ((size_t)a_page - malloc_firstaddr) / PAGESIZE;
	a_mcb = &first_mcb[i];

	if (a_mcb->size != PAGESIZE)
	    panic ("vm_fault(): pagein didn't allocate %i bytes as expected", PAGESIZE);

	a_mcb->size = MCB_VMOBJECT_PAGE;
	a_mcb->bitmap = 0;
	a_mcb->page_nr = pagenumber;
	a_mcb->next = vmobj->page_chain_start;

	vmobj->page_chain_start = i;

	/*
	 *  If the region is Copy-on-write, then we DON'T set the writable
	 *  flag:
	 */
	pmapflags = region->type & (VMREGION_READABLE | VMREGION_WRITABLE);
	if (region->type & VMREGION_COW)
	  pmapflags = region->type & VMREGION_READABLE;

	res = pmap_mappage (p, virtualaddr, a_page, pmapflags);

	goto vm_fault_return;
      }


    /*  here:  vmobj and found_mcb_index should contain legal values...  */


    /*
     *	B:  The page exists. If the region is Copy-On-Write, and the
     *	    action was to 'write' then this means that we will have to
     *	    duplicate the page, and insert the new page in the
     *	    page chain of the vm_region's first source. This first
     *	    source should be a shadow vm_object. If it isn't (doesn't
     *	    exist) then we'll have to create it.
     */

    a_page = (byte *) (found_mcb_index*PAGESIZE + malloc_firstaddr);

    if ((action & VM_PAGEFAULT_WRITE) && (region->type & VMREGION_COW))
      {
	b_page = (byte *) malloc (PAGESIZE);
	memcpy (b_page, a_page, PAGESIZE);

	i = ((size_t)b_page - malloc_firstaddr)/PAGESIZE;
	first_mcb[i].size = MCB_VMOBJECT_PAGE;
	first_mcb[i].bitmap = 0;
	first_mcb[i].page_nr = pagenumber;
	first_mcb[i].next = 0;

	/*  region->source must be a shadow object. Create one if we have to:  */
	if (region->source->type != VM_OBJECT_SHADOW)
	  {
	    new_vmobj = vm_object_create (VM_OBJECT_SHADOW);
	    if (!new_vmobj)
		panic ("vm_fault(): ENOMEM... TODO: kill(SIGSEGV) or something?");

	    new_vmobj->next = region->source;
	    new_vmobj->page_chain_start = i;
	    new_vmobj->refcount = 1;
	    region->source = new_vmobj;
	  }
	else
	  {
	    first_mcb[i].next = region->source->page_chain_start;
	    region->source->page_chain_start = i;
	  }

	res = pmap_mappage (p, virtualaddr, b_page, region->type &
		(VMREGION_WRITABLE | VMREGION_READABLE));
	if (res)
	  panic ("vm_fault(): pmap_mappage() failed 1");

	goto vm_fault_return;
      }

    if (!pmap_mapped (p, virtualaddr))
      {
	/*
	 *  If the region is Copy-on-write, then we DON'T set the writable
	 *  flag:
	 */
	pmapflags = region->type & (VMREGION_READABLE | VMREGION_WRITABLE);
	if (region->type & VMREGION_COW)
	  pmapflags = region->type & VMREGION_READABLE;

	res = pmap_mappage (p, virtualaddr, a_page, pmapflags);
	if (res)
	  panic ("vm_fault(): pmap_mappage() failed 2, TODO: sigsegv ...");
      }
    else
	panic ("vm_fault(): unknown page fault. TODO: kill sigsegv");


vm_fault_return:

    unlock (&vm_fault_lock);
    invmfault --;
  }

