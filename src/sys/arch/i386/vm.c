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
 *  vm.c  --  i386 virtual memory management functions
 *
 *  History:
 *	3 Jan 2000	first version
 */


#include "../config.h"
#include <string.h>
#include <sys/malloc.h>
#include <sys/arch/i386/machdep.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/arch/i386/gdt.h>
#include <sys/arch/i386/idt.h>


extern	size_t	malloc_lastaddr;
extern	byte	*gdt;


/*  The kernel page directory (at a fixed address):  */
u_int32_t *i386_kernel_pagedir = NULL;

/*  Size of kernel in physical RAM rounded up to nearest 4MB:  */
size_t	userland_startaddr;



void machdep_vm_init ()
  {
    /*
     *	machdep_vm_init ()  --  Initialize paging
     *	-----------------------------------------
     *
     *	  1.  Map physical memory to the kernel's virtual address space.
     *	  2.  Turn on some bits in the cr0 register to enable paging.
     */

    int n, i, i2;
    u_int32_t *tmp_pt;
    size_t physaddr;
    size_t seglength;


    /*  Clear the kernel's page directory (fixed at phys. addr. KERNPDIRADDR)  */
    i386_kernel_pagedir = (u_int32_t *) KERNPDIRADDR;
    memset (i386_kernel_pagedir, 0, PAGESIZE);

    /*  Calculate how many pagetables we need (each one can hold 1024 pages
	== 4 MB memory)  */
    n = malloc_lastaddr / 0x400000;
    if (malloc_lastaddr & 0x3fffff)
	n++;

#if DEBUGLEVEL>=8
    printk ("malloc_lastaddr == %i  ==>  n = %i page tables",
		malloc_lastaddr, n);
#endif

    /*
     *	Allocate the pagetables, and fill them with pointers to all
     *	available pages:
     */

    physaddr = 0;
    for (i=0; i<n; i++)
      {
	/*  Allocate a page for a pagetable:  */
	tmp_pt = (u_int32_t *) malloc (PAGESIZE);

	/*  Fill the pagetable:  (if the available physical memory is not
		a multiple of 4 MB, we might accidentally get too much, but
		who cares?)  */
	for (i2=0; i2<1024; i2++)
	  {
	    /*  User/Supervisor:  0  (superv.)  */
	    /*  Read/Write:       2  (write)    */
	    /*  Present or not:   1  (present)  */
	    tmp_pt[i2] = physaddr + 3;
	    physaddr += PAGESIZE;
	  }

	/*  Insert entry for this pagetable into the pagedirectory:  */
	/*  (the 3 is the same as in the pte, ie supervisor+write+present)  */
	i386_kernel_pagedir[i] = (size_t)tmp_pt + 3;
      }


    /*
     *	Set the user CODE and DATA descriptors to point to directly after
     *	the last kernel page.
     */

    seglength = 0xfffff - physaddr/PAGESIZE;

    i386setgdtentry (gdt + SEL_USERCODE, seglength, physaddr, GDT_SEGTYPE_EXEC | GDT_SEGTYPE_READ_CODE,
	GDT_DESCTYPE_CODEDATA, 3, 1, 0, 1, 1, 1);

    i386setgdtentry (gdt + SEL_USERDATA, seglength, physaddr, GDT_SEGTYPE_WRITE_DATA,
	GDT_DESCTYPE_CODEDATA, 3, 1, 0, 1, 1, 1);

    userland_startaddr = physaddr;


    /*
     *	Let's turn the paging on:
     */

    asm ("movl %%eax, %%cr3": : "a" (i386_kernel_pagedir));
    asm ("movl %cr0, %eax");
    asm ("orl $0x80010033, %eax");
    asm ("movl %eax, %cr0");
    asm ("jmp 1f\n1:");
  }

