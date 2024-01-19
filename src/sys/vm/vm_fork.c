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
 *  vm/vm_fork.c  --  vm code to handle fork()
 *
 *  History:
 *	19 Jul 2000	first version, code moved here from sys_fork()
 */


#include "../config.h"
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/defs.h>
#include <sys/errno.h>



int vm_fork (struct proc *p, struct proc *child_proc)
  {
    /*
     *	vm_fork ()
     *	----------
     *
     *	All virtual memory regions of the parent process should be accessible
     *	from both the parent and the child process. Read-only regions can be
     *	shared by the two processes, but writable regions need to have a
     *	shadow object inserted before the actual vm_object. This is neccessay
     *	to implement copy-on-write.
     */

    struct vm_region *region;
    struct vm_object *tmpobj1, *tmpobj2;
    int datareg;


    /*
     *	For each region in the parent "p", create a region in child_proc:
     */

    region = p->vmregions;
    datareg = 0;
    while (region)
      {
	/*  If this is the data region which sys_break() modifies, then
	    we have to remember to set p->dataregion and child_proc->dataregion:  */
	datareg = (region == p->dataregion)? 1 : 0;

	/*
	 *  Is this a non-writable region? Then simply attach a read-only
	 *  region in the child using the same vm_object and characteristics
	 *  as the parent's region:
	 */
	if (!(region->type & VMREGION_WRITABLE))
	    vm_region_attach (child_proc, region->source, region->srcoffset,
		region->start_addr, region->end_addr, region->type);
	else
	  {
	    /*
	     *	The region is writable in one way or another:
	     *	Create shadow objects for both the parent and the child.
	     *	and make them point to the old object:
	     */

	    tmpobj1 = vm_object_create (VM_OBJECT_SHADOW);
	    if (!tmpobj1)
		goto vm_fork_failed;

	    tmpobj2 = vm_object_create (VM_OBJECT_SHADOW);
	    if (!tmpobj2)
	      {
		free (tmpobj1);
		goto vm_fork_failed;
	      }

	    /*  The shadow objects should refer to the original object:  */
	    tmpobj1->next = region->source;
	    tmpobj2->next = region->source;

	    if (region->source->type == VM_OBJECT_SHADOW)
	      {
		region->source->shadow_ref1 = tmpobj1;
		region->source->shadow_ref2 = tmpobj2;
	      }

	    /*  The parent should now use the new shadow object 'tmpobj1':  */
	    region->source = tmpobj1;

	    /*
	     *	Both the parent's region and the child's region should
	     *	use Copy-on-Write.  We only set this for the parent,
	     *	since we call vm_region_attach() to create a duplicate
	     *	with the same characteristics as the parent's vm_region:
	     */

	    region->type |= VMREGION_COW;
	    vm_region_attach (child_proc, tmpobj2, region->srcoffset,
		region->start_addr, region->end_addr, region->type);

	    /*
	     *	Correct some refcounts:
	     *	The parent is using tmpobj1, so this should be increased
	     *	(to 1), but tmpobj2's refcount has already been increased
	     *	by vm_region_attach(). The original object refered to by
	     *	both tmpobj1 and tmpobj2 (ie. tmpobj2->next) should have
	     *	its refcount increased, since now there is one more referer
	     *	(the child).
	     */
	    tmpobj1->refcount ++;	/*  should now be = 1  */
	    tmpobj2->next->refcount ++;

	    if (datareg)
	      {

if (p->dataregion != region)
	panic ("yoo");

		p->dataregion = region;
		child_proc->dataregion = child_proc->vmregions;

		/*  extra check to make sure that vm_region_attach actually
		    added the region _first_ in the chain:  */
		if (child_proc->dataregion->start_addr != region->start_addr)
		  panic ("vm_fork: vm_region_attach broken");
	      }

	    pmap_markpages_cow (p, region->start_addr, region->end_addr);
	  }

	/*  Next region:  */
	region = region->next;
      }

    return 0;


vm_fork_failed:

    /*
     *	If vm_fork() fails, it is up to the caller to deallocate any
     *	regions attached to the child process.
     *
     *	TODO:  of course, if the fork fails and there has already been
     *	one or more copy-on-write split ups using shadow vm_objects, then
     *	the parent's vmregions will point to _shadow objects_, when before
     *	the fork they would have pointed to the actual objects. A "tree
     *	collapser" needs to be implemented to fix this memory leakage...
     */

    return ENOMEM;		/*  TODO: is this the best error code?  */
  }

