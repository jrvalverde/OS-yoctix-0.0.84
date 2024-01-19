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
 *  arch/i386/pmap.c  --  i386 specific physical memory mapping stuff
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


extern size_t userland_startaddr;


int pmap_mappage (struct proc *p, size_t virtualaddr, byte *a_page, u_int32_t region_type)
  {
    size_t *pagedir;
    size_t *pagetable;
    int pdentrynr, ptentrynr;
    int access;
/* char *tmp;*/

    virtualaddr += userland_startaddr;
    pagedir = p->md.pagedir;
    pdentrynr = (virtualaddr >> 22) & 1023;
    ptentrynr = (virtualaddr >> 12) & 1023;

    /*  If there is no pagetable for this pdentrynr, then let's create one:  */
    if (!pagedir[pdentrynr])
      {
	pagetable = (size_t *) malloc (PAGESIZE);
	if (!pagetable)
	  return ENOMEM;
	memset ((byte *)pagetable, 0, PAGESIZE);
	pagedir[pdentrynr] = (size_t) ((u_int32_t) pagetable + 7);
      }
    else
	pagetable = (size_t *) ((u_int32_t)pagedir[pdentrynr] & 0xfffff000);

/*
printk ("physaddr=%x a=%x pdnr=%i ptnr=%i pdiraddr=%x ptabaddr=%x [%x]", (int)virtualaddr, (int)a_page,
	(int)pdentrynr, (int)ptentrynr, (int)pagedir, (int)pagetable, pagetable[ptentrynr]);

tmp = (char *) ((size_t)a_page & 0xfffff000);
printk ("  dump: %y %y %y %y %y %y %y %y %y %y %y %y %y %y %y %y",
	tmp[0], tmp[1], tmp[2], tmp[3],
	tmp[4], tmp[5], tmp[6], tmp[7],
	tmp[8], tmp[9], tmp[10], tmp[11],
	tmp[12], tmp[13], tmp[14], tmp[15]);

If there IS a ptentry in the pagetable, then there is an error
and this function should not have been called in the first place.
if (pagetable[ptentrynr])
  panic ("pmap_mappage(): virtualaddr=%x is already in the process' pagetable", virtualaddr);
*/


    access = (region_type & VMREGION_WRITABLE)? 7 : 5;
    pagetable[ptentrynr] = ((u_int32_t)a_page & 0xfffff000) + access;

    return 0;
  }



int pmap_markpages (struct proc *p, size_t startaddr, size_t endaddr, int mask)
  {
    /*
     *	Mark all pages from address startaddr upto (and including)
     *	endaddr of process p according to mask. startaddr and endaddr are
     *	userland addresses.
     *
     *	Return 1 on success, 0 on failure.
     */

    size_t *pagedir;
    size_t *pagetable;
    int pdentrynr, ptentrynr;
    unsigned int nr_of_pages;

    if (!p || startaddr>=endaddr)
	return 0;

    if (endaddr >= 0xffffffff-userland_startaddr)
      endaddr = 0xffffffff-userland_startaddr;

    startaddr &= 0xfffff000;
    endaddr   &= 0xfffff000;
    nr_of_pages = ((u_int32_t)endaddr - (u_int32_t)startaddr)/PAGESIZE + 1;

    pagedir = p->md.pagedir;
    startaddr += userland_startaddr;

    while (nr_of_pages > 0)
      {
	pdentrynr = (startaddr >> 22) & 1023;
	ptentrynr = (startaddr >> 12) & 1023;

	pagetable = (size_t *) ((u_int32_t)pagedir[pdentrynr] & 0xfffff000);
	if (pagetable)
	    pagetable[ptentrynr] &= ~mask;

	startaddr += PAGESIZE;
	nr_of_pages --;
      }

    return 1;
  }



int pmap_markpages_cow (struct proc *p, size_t startaddr, size_t endaddr)
  {
    /*
     *	Mark all pages from address startaddr upto (and including)
     *	endaddr of process p as non-writable. startaddr and endaddr are
     *	userland addresses.
     *
     *	Return 1 on success, 0 on failure.
     */

    /*  Remove "write" bit from all pages:  */
    return pmap_markpages (p, startaddr, endaddr, 2);
  }



int pmap_unmappages (struct proc *p, size_t startaddr, size_t endaddr)
  {
    /*
     *	Unmap all pages from address startaddr upto (and including)
     *	endaddr by marking them non-readable, non-writable, non-user.
     *
     *	Return 1 on success, 0 on failure.
     */

    /*  Remove all access bits from the pages:  */
    return pmap_markpages (p, startaddr, endaddr, 7);
  }



int pmap_mapped (struct proc *p, size_t virtualaddr)
  {
    /*
     *	Return 1 if the virtual address was mapped in process p,
     *	0 if it was not mapped.
     */

    size_t *pagedir;
    size_t *pagetable;
    int pdentrynr, ptentrynr;

    virtualaddr += userland_startaddr;
    pagedir = p->md.pagedir;
    pdentrynr = (virtualaddr >> 22) & 1023;
    ptentrynr = (virtualaddr >> 12) & 1023;

    if (!(pagedir[pdentrynr] & 7))
	return 0;

    pagetable = (size_t *) ((u_int32_t)pagedir[pdentrynr] & 0xfffff000);

    if (!(pagetable[ptentrynr] & 7))
	return 0;

    return 1;
  }

