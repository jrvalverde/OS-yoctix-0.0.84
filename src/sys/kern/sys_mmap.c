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
 *  kern/sys_mmap.c  --  memory mapping etc.
 *
 *	sys_mmap ()
 *		Maps files or anonymous memory into a process' virtual
 *		memory.
 *
 *	sys_munmap ()
 *		Unmaps regions (or parts of regions) from a process.
 *
 *	sys_break ()
 *		Traditional Unix syscall to set the size of the process'
 *		data segment.
 *
 *
 *  History:
 *	27 Jun 2000	test
 *	19 Jul 2000	adding sys_munmap(), sys_mmap()
 *	20 Jul 2000	sys_break() (doesn't free pages if break addr is lowered)
 *	26 Jul 2000
 */


#include "../config.h"
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/syscalls.h>
#include <sys/interrupts.h>
#include <sys/vm.h>
#include <sys/mman.h>
#include <sys/md/machdep.h>


extern size_t userland_startaddr;


int sys_mmap (ret_t *res, struct proc *p, void *addr, size_t len, int prot,
	int flags, int fd, off_t offset)
  {
    /*
     *	sys_mmap ()
     *	-----------
     *
     *	Map a file (or anonymous memory), shared or private, into the process
     *	virtual memory address space.
     *
     *	First, any mappings already existing in the range addr to addr+len-1
     *	should be removed. We use sys_munmap() for this.
     *
     *	If addr!=NULL and on a page boundary, then we use that address.
     *	Otherwise, we look up a free memory range.
     *
     *	Returns the address of the mmap:ed region on success, MAP_FAILED
     *	on failure.
     */

    int tmp;
    struct vm_object *new_object;
    struct vnode *v;
    int anonymous = 0;
    int filemap = 0;


    *res = (size_t) MAP_FAILED;

    if (!res || !p)
	return EINVAL;

#if DEBUGLEVEL>=3
    printk ("sys_mmap: addr=%x len=%i prot=%i flags=%i fd=%i offset=%i",
	(int)addr, len, prot, flags, fd, (int)offset);
#endif

    /*  Page align len:  */
    len = round_up_to_page (len);

    if (flags==MAP_PRIVATE+MAP_ANON && fd==-1)
	anonymous = 1;

    if (flags==MAP_COPY+MAP_ANON && fd==-1)
	anonymous = 1;

    if (flags==MAP_COPY+MAP_FIXED+MAP_ANON && fd==-1)
	anonymous = 1;

//    if (flags==MAP_SHARED && prot==PROT_READ)
    if (!(flags & MAP_ANON) && (prot&(PROT_READ|PROT_WRITE))==PROT_READ)
	filemap = 1;

    if (!(flags & MAP_ANON) && (flags & MAP_COPY))
	filemap = 1;

    if (!anonymous && !filemap)
      {
	printk ("sys_mmap: unimplemented addr=%x len=%i prot=%i flags=%i fd=%i offset=%i",
		(int)addr, len, prot, flags, fd, (int)offset);
	return EINVAL;
      }

    if (filemap && (fd<0 || fd>=p->nr_of_fdesc))
	return EBADF;


    /*
     *	If addr==NULL or addr is not on a page boundary, then we use
     *	vm_region_findfree() to find a free memory range.
     */

    if (!addr || ((int)addr & (PAGESIZE-1)))
      addr = vm_region_findfree (p, len);

    if (!addr)
	return ENOMEM;


    if (len==0)
      {
	printk ("sys_mmap: len=%i, addr=0x%x", len, addr);
	*res = (size_t) addr;
	return 0;
      }


    /*
     *	Do an munmap() on the address range before we create our new mapping:
     */

    tmp = sys_munmap (res, p, addr, len);
    if (tmp)
	return EINVAL;


    /*
     *	Create the new object (anonymous or file vm_object), and attach
     *	it to the process:
     *
     *	(TODO: maybe this should be allocated earlier, to make sure
     *	that as little damage as possible is done in case we run
     *	out of memory...)
     */

    if (anonymous)
      {
	new_object = vm_object_create (VM_OBJECT_ANONYMOUS);
	if (!new_object)
	  return ENOMEM;
      }
    else
      {
	v = p->fdesc[fd]->v;
	if (!v)
	  return EINVAL;

	lock (&v->lock, "sys_mmap", LOCK_BLOCKING | LOCK_RW);

printk ("! v->vmobj=%x refc=%i name='%s'", (int)v->vmobj, v->refcount,
	v->vname->name);

	/*  Is there already a vm_object for this vnode?  */
	if (v->vmobj)
	  {
	    /*  Then let's use it:  */
	    new_object = v->vmobj;
	  }
	else
	  {
	    /*  Create a new vm_object:  */
	    new_object = vm_object_create (VM_OBJECT_FILE);
	    if (!new_object)
	      {
		unlock (&v->lock);
		return ENOMEM;
	      }
	    new_object->vnode = v;
	    v->refcount ++;
	    v->vmobj = new_object;


	  }

//	new_object->refcount ++;
	unlock (&v->lock);
      }

/*
	    tmp = vm_region_attach (p, new_object, offset,
			(size_t)addr, (size_t)addr+len-1,
			  ((prot & PROT_READ)? VMREGION_READABLE : 0)
			+ ((prot & PROT_WRITE)? VMREGION_WRITABLE : 0)
			+ (((flags & MAP_COPY) && (prot & PROT_WRITE))? VMREGION_COW : 0)
			);
*/

    tmp = vm_region_attach (p, new_object, offset,
		(size_t)addr, (size_t)addr+len-1,
		  ((prot & PROT_READ)? VMREGION_READABLE : 0)
		+ (((flags & MAP_COPY) || (prot & PROT_WRITE))? VMREGION_WRITABLE : 0)
		+ (((flags & MAP_COPY) || (prot & PROT_WRITE))? VMREGION_COW : 0)
		);

printk (" (2)mmap: addr=%x attach=%i obj->vnode=%x", addr, tmp, new_object->vnode);

    if (!tmp)
      {
//	if (filemap)
//	  new_object->vnode->refcount --;

	vm_object_free (new_object);
	return ENOMEM;
      }

    *res = (size_t) addr;
    return 0;
  }



int sys_munmap (ret_t *res, struct proc *p, void *addr, size_t len)
  {
    /*
     *	sys_munmap ()
     *	-------------
     *
     *	Remove any vmregions of p that are completely covered by the address
     *	range addr to addr+len-1.
     *
     *				   A:			   B:
     *	vmregion:		--------		--------
     *	addr..addr+len-1:	    ++++++++	    ++++++++
     *
     *	Any "partly" covered vmregion (like figure A or B) should be truncated.
     *	If the truncation involves increasing the start_addr (case B), then the
     *	srcoffset must also be increased.
     *
     *				   C:
     *	vmregion:		--------
     *	addr..addr+len-1:	  ++++
     *
     *	Case C is handled by splitting up the vmregion in two halves.
     *	The srcoffset of the second half needs to be increased accordingly.
     *
     *	NOTE: This implementation of munmap() is NOT the same as most other
     *	implementations. A correct implementation of munmap() should return
     *	error if an attempt was made to unmap a non-existing region. So, this
     *	implementation is not correct but it is probably a bit simpler and will
     *	hopefully function as expected in other aspects. It ought to be
     *	rewritten some day. (TODO)
     *
     *	On success, 0 is returned, otherwise errno.
     */

    size_t start_addr, end_addr;
    size_t oldregionend;
    struct vm_region *region;
    struct vm_region *nextregion;


    if (!p || !res)
	return EINVAL;

    if (((size_t)addr & (PAGESIZE-1)) != 0)
	return EINVAL;

    /*  Page align len:  */
    len = round_up_to_page (len);

    start_addr = (size_t)addr;
    end_addr = start_addr + len - 1;

    if (end_addr < start_addr)
	return EINVAL;


    /*
     *	"Physically" unmap all the pages, nomatter if we
     *	succeed in unmapping the actual regions:
     */

    pmap_unmappages (p, start_addr, end_addr);


    /*
     *	Step through the vmregions chain:
     */

    region = p->vmregions;
    while (region)
      {
	nextregion = region->next;

	/*  Is this region totally covered by start_addr..end_addr?  */
	if (region->start_addr >= start_addr && region->end_addr <= end_addr)
	  {
	    /*  ... then let's remove it completely.  */
	    vm_region_detach (p, region->start_addr);
	  }
	else
	/*  Case A?  (See explanation above.)  */
	if (region->start_addr < start_addr &&
	    region->end_addr > start_addr && region->end_addr <= end_addr)
	  {
	    /*
	     *	The part of A with addresses ranging from start_addr to
	     *	region->end_addr should be removed. We do this by simply
	     *	truncating the region.
	     */
	    region->end_addr = start_addr - 1;
	  }
	else
	/*  Case B?  (See explanation above.)  */
	if (region->start_addr >= start_addr && region->start_addr < end_addr
	    && region->end_addr > end_addr)
	  {
	    /*
	     *	The part of B with addresses ranging from region->start_addr to
	     *	end_addr should be removed. We do this by truncating the region
	     *	from the lefthand side and increasing region->srcoffset.
	     */
	    region->srcoffset += (end_addr + 1 - region->start_addr);
	    region->start_addr = end_addr + 1;		/*  Should be page aligned...  */
	  }
	else
	/*  Case C?  (See explanation above.)  */
	if (region->start_addr < start_addr && region->end_addr > end_addr)
	  {
	    /*
	     *	This case is handled by splitting up the vmregion in two halves.
	     *	The srcoffset of the second half needs to be increased accordingly.
	     */
	    oldregionend = region->end_addr;
	    region->end_addr = start_addr - 1;

	    vm_region_attach (p, region->source, region->srcoffset +
		(end_addr+1 - region->start_addr), end_addr+1, oldregionend, region->type);
	  }

	/*  Next region:  */
	region = nextregion;
      }

    return 0;
  }



int sys_mprotect (ret_t *res, struct proc *p, void *addr, size_t len, int prot)
  {
    /*
     *	sys_mprotect ()
     *	---------------
     */

    /*  TODO  */
printk ("sys_mprotect: TODO");

    return 0;
  }



int sys_break (ret_t *res, struct proc *p, char *nsize)
  {
    /*
     *	sys_break ()
     *	------------
     *
     *	Set the process' break address. If nsize is below
     *	p->dataregion->end_addr, then we munmap() from nsize-1 to end_addr
     *	and decrease the length of the dataregion.
     *	If nsize is above end_addr then we munmap() from end_addr to
     *	nsize-1 and then increase the length of the dataregion.
     *
     *	Returns nsize in res if successful.
     */

    ssize_t diff;
    ret_t tmp;
    int tmp2;
    size_t endaddraligned;

    if (!p || !res)
	return EINVAL;

    /*  Page align nsize and end_addr:  */
    nsize = (char *)round_up_to_page (nsize);
    endaddraligned = round_up_to_page (p->dataregion->end_addr-1);

    *res = (ret_t) ((long int)nsize);

    if ((size_t)nsize < p->dataregion->start_addr)
	return ENOMEM;

    diff = (size_t)nsize - endaddraligned;
    tmp = 0;
    tmp2 = 0;
    if (diff < 0)
	tmp2 = sys_munmap (&tmp, p, (void *)nsize, -diff);
    else
    if (diff > 0)
	tmp2 = sys_munmap (&tmp, p, (void *)endaddraligned, diff);

    if (tmp2)
	return tmp2;

    p->dataregion->end_addr = (size_t)nsize - 1;
    return 0;
  }

