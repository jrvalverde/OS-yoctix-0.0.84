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
 *  vm/vm_object.c  --  handle virtual memory objects
 *
 *	vm_anonymous_pagein ()
 *		Pagein function for anonymous vm_objects. (Returns a zero-
 *		filled page.)
 *
 *	vm_object_create ()
 *		Creates an "empty" object.
 *
 *	vm_object_freepages ()
 *		Frees pages (from a vm_object) with page_nr within a
 *		specific range.
 *
 *	vm_object_free ()
 *		Removes a vm_object from memory. Calls vm_object_combine()
 *		if neccessary.
 *
 *	vm_object_combine ()
 *		Combine two vm_objects.
 *
 *  History:
 *	8 Jan 2000	first version, vm_object_free()
 *	18 Feb 2000	added vm_object_create()
 *	25 Feb 2000	added vm_anonymous_pagein()
 *	7 Mar 2000	adding page chains to vm_objects
 *	17 Jul 2000	vm_object_combine()
 *	26 Jul 2000	vm_object_freepages()
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/interrupts.h>
#include <sys/md/machdep.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/errno.h>


extern struct mcb *first_mcb;		/*  physical addr of first mcb  */
extern size_t malloc_firstaddr;



void *vm_anonymous_pagein (int *errno, struct vm_region *vmregion,
	struct vm_object *vmobject, size_t linearaddr)
  {
    /*
     *	Anonymous memory pagein function:
     *  Allocate a page and zerofill it.
     */

    void *p = (void *) malloc (PAGESIZE);
    *errno = 0;

    if (!p)
      {
	*errno = ENOMEM;
	return NULL;
      }

    memset (p, 0, PAGESIZE);
    return p;
  }



struct vm_object *vm_object_create (int type)
  {
    /*
     *	Allocate memory for a vm_object and return the pointer to
     *	it. Return NULL on failure.
     *
     *	The only thing done with the vm_object is to set its type.
     *	Anything else (such as pointing it to a vnode, or increasing
     *	its refcount) must be done by the caller.
     */

    struct vm_object *v;

    v = (struct vm_object *) malloc (sizeof(struct vm_object));
    if (!v)
	return NULL;

    memset (v, 0, sizeof(struct vm_object));

    v->type = type;

    if (type == VM_OBJECT_FILE)
	v->pagein = (void *) vnode_pagein;

    if (type == VM_OBJECT_ANONYMOUS)
	v->pagein = vm_anonymous_pagein;

    return v;
  }



int vm_object_freepages (struct vm_object *obj, size_t firstpage, size_t lastpage)
  {
    /*
     *	vm_object_freepages ()
     *	----------------------
     *
     *	Free any pages with page_nr in the range firstpage..lastpage, by
     *	removing the page from the object's page chain and free()'ing it.
     *
     *	Returns 0 on success, errno on error.
     */

    size_t pageindex;
    struct mcb *a_mcb, *prev_mcb;
    byte *addr;

    if (!obj)
	return EINVAL;

    prev_mcb = NULL;
    pageindex = obj->page_chain_start;
    while (pageindex)
      {
	a_mcb = &first_mcb[pageindex];
	if (a_mcb->size != MCB_VMOBJECT_PAGE)
	  panic ("vm_object_free: inconsist vmobj page size");

	addr = (byte *) (malloc_firstaddr+PAGESIZE*pageindex);

	/*  Remember mcb index to the next page in this chain:  */
	pageindex = a_mcb->next;

	if (a_mcb->page_nr >= firstpage && a_mcb->page_nr <= lastpage)
	  {
	    if (!prev_mcb)
	      obj->page_chain_start = a_mcb->next;
	    else
	      prev_mcb->next = a_mcb->next;

	    /*  Free the page:  (this also clears the mcb data)  */
	    free (addr);
	  }
	else
	  prev_mcb = a_mcb;
      }

    return 0;
  }



int vm_object_free (struct vm_object *obj)
  {
    /*
     *	Decrease the reference count of a vm_object, and ultimately
     *	remove it completely from memory.
     *
     *	Returns 1 on success, 0 on failure.
     */

    int oldints;
    struct vm_object *object3;


    if (!obj)
	return 0;

    lock (&obj->lock, "vm_object_free", LOCK_BLOCKING | LOCK_RW);

    obj->refcount --;

    /*  If others reference the object, then let's leave it in memory.  */
    if (obj->refcount > 0)
      {
	unlock (&obj->lock);
	return 1;
      }


    /*
     *	A shadow object (object1 in the picture below) points to another
     * 	object (object2). If there is no object3, then object2 should also
     *	be freed. Otherwise we should combine object2 and object3 into one.
     *
     *		object1  ---\
     *			     \
     *			      >---  object2
     *			     /
     *		object3  ---/
     *
     *	The pages belonging to object1 will be freed (later in this
     *	function), but if we don't combine object2 and object3 there
     *	might be duplicate pages somewhere which would waste memory.
     *	Therefore, they should be combined into one object.
     */

    if (obj->next)
      {
printk ("()()() refcount=%i vnode=%x type=%i nexttype=%i nextvnode=%x",
	obj->refcount, obj->vnode, obj->type, obj->next->type,
	obj->next->vnode);

	if (obj->next->refcount == 2  &&
		obj->next->type == VM_OBJECT_SHADOW)
	  {
	    object3 = obj->next->shadow_ref1;
	    if (object3 == obj)
	      object3 = obj->next->shadow_ref2;

	    vm_object_combine (obj->next, object3);
	  }
	else
	  /*  Recursively free the referenced object:  */
	  vm_object_free (obj->next);
      }

    /*  If the object was a file object, then let's decrease the refcount
	of the vnode the object used:  */
/*
    if (obj->vnode)
      {
	if (obj != obj->vnode->vmobj)
	  panic ("vm_object_free: multiple vm_objects point to one vnode");

	obj->vnode->refcount --;

	if (obj->flags & VM_OBJECT_EXECUTABLEFILE)
	  obj->vnode->refcount_exec --;

*  TODO:  writable vnodes should be written to disk...  *

      }
*/

    /*
     *	If this was not a file vm_object (describing a vnode), then we
     *	free the pages belonging to it. Pages in file vm_objects act as
     *	a kind of buffer cache, so they should only be freed if there's
     *	a lack of free memory.
     */

    oldints = interrupts (DISABLE);

    if (!obj->vnode)
      vm_object_freepages (obj, 0, (size_t)-1);

    interrupts (oldints);
    unlock (&obj->lock);


    /*
     *	If the object was an anonymous object, then we actually free
     *	the memory occupied by the object itself. If it was a file object
     *	then we don't free it (as it contains cached pages of the contents
     *	of that vnode) but we "close" it.
     */

    if (!obj->vnode)
      free (obj);

    return 1;
  }



int vm_object_combine (struct vm_object *subobj, struct vm_object *newobj)
  {
    /*
     *	vm_object_combine
     *	-----------------
     *
     *	Combine objects subobj and newobj, and then free subobj (including
     *	pages). This algorithm is used:
     *
     *	    For each page in subobj
     *		which is not found in newobj:
     *			add page to newobj
     *		which is found in newobj:
     *			free the page
     *
     *	    Free subobj itself.
     *
     *
     *	Returns 0 on success, errno on error.
     */

    struct mcb *a_mcb;
    struct mcb *new_mcb;
    byte *addr;
    size_t pageindex, newpageindex, foundindex, tmp;
    size_t page_nr;


    if (!subobj || !newobj)
	return EINVAL;

    if (newobj->next != subobj || newobj->type != VM_OBJECT_SHADOW
	  || subobj->type != VM_OBJECT_SHADOW)
	panic ("vm_object_combine: vm_object chain is corrupt");

    lock (&newobj->lock, "vm_object_combine", LOCK_BLOCKING | LOCK_RW);
    lock (&subobj->lock, "vm_object_combine", LOCK_BLOCKING | LOCK_RW);

    /*  Go through all of subobj's pages:  */
    pageindex = subobj->page_chain_start;
    while (pageindex)
      {
	a_mcb = &first_mcb[pageindex];
	page_nr = a_mcb->page_nr;
	addr = (byte *) (malloc_firstaddr+PAGESIZE*pageindex);

	/*  Try to find the page "page_nr" in the newobj pages:  */
	newpageindex = newobj->page_chain_start;
	foundindex = 0;
	while (newpageindex)
	  {
	    new_mcb = &first_mcb[newpageindex];
	    if (page_nr == new_mcb->page_nr)
	      {
		foundindex = newpageindex;
		newpageindex = 0;
	      }
	    else
		newpageindex = new_mcb->next;
	  }
	/*  Was it not found? Then move it from subobj to newobj:  */
	tmp = a_mcb->next;
	if (foundindex==0)
	  {
	    a_mcb->next = newobj->page_chain_start;
	    newobj->page_chain_start = pageindex;
	    pageindex = tmp;
	  }
	else
	  {
	    pageindex = tmp;
	    /*  Free the page...  */
	    if (a_mcb->size != MCB_VMOBJECT_PAGE)
	      panic ("vm_object_combine: inconsistent vmobj page size");
	    free (addr);
	  }
      }


    /*  Free subobj:  */
    newobj->next = subobj->next;

    if (subobj->next->shadow_ref1 == subobj)
	subobj->next->shadow_ref1 = newobj;
    if (subobj->next->shadow_ref2 == subobj)
	subobj->next->shadow_ref2 = newobj;

    free (subobj);

    unlock (&newobj->lock);

    return 0;
  }


