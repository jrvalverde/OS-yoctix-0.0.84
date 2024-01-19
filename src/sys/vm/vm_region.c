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
 *  vm/vm_region.c  --  handle virtual memory regions for processes
 *
 *	Each process has a number of memory regions. Each memory region
 *	points to a vm_object (these objects can be shared by several
 *	processes).  Regions are not shared between processes.
 *
 *	When a region is added, its vm_object's reference count is increased.
 *	When a region is removed, its vm_object's ref count is decreased,
 *	and if it becomes zero then nobody is using the object anymore and
 *	it is freed completely. (See vm_object_free() for the details.)
 *
 *
 *	vm_region_attach ()
 *		Attach a vm_object to a process' vm_region chain.
 *		Increases the vm_object's refcount.
 *
 *	vm_region_detach ()
 *		Detach a vm_object from a process' vm_region chain.
 *		Decreases the vm_object's refcount (by calling
 *		vm_object_free()).
 *
 *	vm_region_findfree ()
 *		Find a free memory area with as high address as possible.
 *		(Used my mmap() when no hint address was given by the
 *		process.)
 *
 *
 *  History:
 *	8 Jan 2000	first version
 *	20 Jan 2000	removing vm_region_init(), since it didn't do
 *			anything
 *	19 Jul 2000	adding vm_region_findfree()
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/defs.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/md/machdep.h>



int vm_region_attach (struct proc *p, struct vm_object *obj, off_t objoffset,
	size_t start_addr, size_t end_addr, u_int32_t type)
  {
    /*
     *	Add a region to a process.
     *
     *	TODO:  Sort according to starting address... So far we simply
     *	add the region first in the process' vmregions chain.
     *
     *	Returns 1 on success, 0 on failure.
     */

    struct vm_region *region;

    if (!p || !obj)
	return 0;

if (type==9)
  panic ("Yo, addr=%x..%x", start_addr, end_addr);

    region = (struct vm_region *) malloc (sizeof(struct vm_region));
    if (!region)
	return 0;
    memset (region, 0, sizeof(struct vm_region));

    region->next = NULL;
    region->prev = NULL;
    region->start_addr = start_addr;
    region->end_addr = end_addr;
    region->type = type;
    region->source = obj;
    region->srcoffset = objoffset;

    /*  Let's tell the object that we are using it:  */
    obj->refcount ++;

    /*  Put this region first in the process' vmregions chain:  */
    region->next = p->vmregions;
    if (region->next)
	region->next->prev = region;
    p->vmregions = region;

    return 1;
  }



int vm_region_detach (struct proc *p, size_t start_addr)
  {
    /*
     *	vm_region_detach ()
     *	-------------------
     *
     *	Removes a region from a process. The object which the region
     *	referenced is freed.
     *
     *	Returns 1 on success, 0 on failure.
     */

    struct vm_region *tmp, *region;

    if (!p)
	return 0;

    /*  Scan the vm_region chain for a region with the correct start_addr:  */
    tmp = NULL;
    region = p->vmregions;
    while (region)
      {
	if (region->start_addr == start_addr)
	  {
	    tmp = region;
	    region = NULL;
	  }

	if (region)
	  region = region->next;
      }

    /*  No such region found? Then abort. */
    if (!tmp)
	return 0;

    /*  We are not using the source vm_object anymore:  */
    vm_object_free (tmp->source);

    /*  Remove the region from the process' vmregions chain:  */
    if (tmp->prev)
	tmp->prev->next = tmp->next;
    if (tmp->next)
	tmp->next->prev = tmp->prev;

    if (tmp == p->vmregions)
      p->vmregions = tmp->next;

    free (tmp);

    return 1;
  }



void *vm_region_findfree (struct proc *p, size_t len)
  {
    /*
     *	vm_region_findfree ()
     *	---------------------
     *
     *	Try to find an unmapped memory area of size "len" just below another
     *	vmregion, with the highest possible address. Called by mmap(), when a
     *	free memory area of a specific length is needed.
     *
     *	len does not need to be page aligned.
     *
     *	Returns the address to the memory area on success, NULL on failure.
     */

    struct vm_region *tmp, *region;
    size_t addr1, addr2;
    void *addr;
    int found;

    if (!p)
	return NULL;

if (len==0)
	len = PAGESIZE;

    /*  Page align len:  */
    len = round_up_to_page (len);

    addr = NULL;
    region = p->vmregions;
    while (region)
      {
	/*  We should look for a free memory area between these two addresses:  */
	addr1 = region->start_addr-len;
	addr2 = region->start_addr-1;

	if (region->start_addr > len)
	  {
	    /*
	     *	If any existing region "touches" the address range addr1..addr2,
	     *	then it means that we cannot accept this memory range.
	     */

	    found = 0;		/*  set to 1 if addr1..2 and the tmp region collide  */
	    tmp = p->vmregions;
	    while (tmp)
	      {
		if (    (tmp->start_addr < addr1 && tmp->end_addr < addr1)
		     || (tmp->start_addr > addr2 && tmp->end_addr > addr2))
		  tmp = tmp->next;
		else
		  found = 1, tmp = NULL;
	      }

	    if (!found && addr1 > (size_t)addr)
		addr = (void *)addr1;
	  }

	region = region->next;
      }

    return addr;
  }

