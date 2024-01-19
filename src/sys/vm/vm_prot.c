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
 *  vm/vm_prot.c  --  memory range protection checks
 *
 *
 *  History:
 *	24 Nov 2000	test, seems to work
 */


#include "../config.h"
#include <sys/proc.h>
#include <sys/std.h>
#include <sys/vm.h>


extern size_t userland_startaddr;


int vm_prot_accessible (struct proc *p, void *addr, size_t len,
	int accessflag)
  {
    /*
     *	vm_prot_accessible ()
     *	---------------------
     *
     *	Returns 1 if the address range is accessible, 0 if the memory range is
     *	not accessible or if there was some other error.
     *
     *	The memory range (specified by 'addr' and 'len') can encompass more
     *	than one vm_region, so all vm_regions need to be checked.
     *
     *	(The regions must have _at least_ the accessability specified in the
     *	accessflag, ie if accessflag=VM_REGION_WRITABLE, then it will match a
     *	region with VM_REGION_READABLE+WRITABLE, but not one with only the
     *	READABLE bit set.)
     *
     *	Note: addr is a userland address.
     */

    struct vm_region *region;
    int done = 0;
    size_t start = (size_t)addr, stop = (size_t)addr+len-1;

#if DEBUGLEVEL>=5
    printk ("vm_prot_accessible: p=0x%x addr=0x%x len=%i acc=%Y",
	(int)p, (int)addr, (int)len, accessflag);
#endif

    if (!p)
      return 0;

    /*  Loop until the entire range has been checked:  */
    while (!done)
      {
	/*  Find the region where the address range begins:  */
	done = 1;
	region = p->vmregions;
	while (region)
	  {
	    if (region->start_addr <= start &&
		region->end_addr >= start)
	      {
		/*  Are the access flags on this vm_region not okay?  */
		if ((region->type & accessflag) != accessflag)
		  return 0;

		/*  Are we done?  */
		start = region->end_addr + 1;
		if (start > stop)
		  return 1;

		done = 0;
		region = NULL;
	      }
	    else
	      /*  next region  */
	      region = region->next;
	  }
      }

    return 0;
  }



size_t vm_prot_maxmemrange (struct proc *p, void *addr, int accessflag)
  {
    /*
     *	vm_prot_maxmemrange ()
     *	----------------------
     *
     *	Returns the maximum number of bytes that a "string" (a memory
     *	range) at address 'addr' can consist of. If the regions
     *	involved are incompatible with the requested accessflag, or
     *	if addr doesn't point to any region at all, 0 is returned.
     *
     *	Note: addr is a userland address.
     */

    struct vm_region *region;
    int done = 0;
    size_t start, stop;

#if DEBUGLEVEL>=5
    printk ("vm_prot_maxmemrange: p=0x%x addr=0x%x acc=%Y",
	(int)p, (int)addr, accessflag);
#endif

    if (!p)
      return 0;

    start = (size_t)addr;
    stop = start - 1;

    while (!done)
      {
	/*  Find the region where the address range begins:  */
	region = p->vmregions;

	done = 1;

	while (region)
	  {
	    if (region->start_addr <= start &&
		region->end_addr >= start)
	      {
		/*  Are the access flags on this vm_region not okay?  */
		if ((region->type & accessflag) != accessflag)
		  return (stop - (size_t)addr + 1);

		/*  Stop is at least the same as the end of this region:  */
		stop = region->end_addr;
		start = stop + 1;
		done = 0;
		region = NULL;
	      }
	    else
	      /*  next region  */
	      region = region->next;
	  }
      }

    return (stop - (size_t)addr + 1);
  }



size_t vm_prot_stringlen (struct proc *p, byte *addr, size_t buflen)
  {
    /*
     *	vm_prot_stringlen ()
     *	--------------------
     *
     *	This function works like strlen(), but if the length is more than
     *	buflen-1, then 0 is returned.
     *
     *	maxlen should be the total length of a memory range as returned by
     *	vm_prot_maxmemrange().
     *
     *	Note: addr is a userland address.
     */

    size_t r = 0;

    addr = (byte *) (addr + userland_startaddr);

#if DEBUGLEVEL>=5
    printk ("vm_prot_stringlen: p=0x%x addr=0x%x '%s' buflen=%i",
	(int)p, (int)addr, addr, (int)buflen);
#endif

    if (!p || buflen<1)
      return 0;

    while (buflen>=1 && ((*addr)!='\0'))
      buflen--, r++, addr++;

    if (buflen==0 && (*addr)=='\0')
      return 0;

    return r;
  }

