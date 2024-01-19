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
 *  machdep_main_startup.c  --  do machine dependant low-level startup
 *  (first thing called from main())
 *
 *  Return 0 on success, >0 if failure.
 *
 *  History:
 *	18 Oct 1999	first version
 *	22 Oct 1999	call gdt_init()
 *	24 Oct 1999	call idt_init()
 *	24 Nov 1999	temporary set firstaddr and lastaddr
 *	17 Dec 1999	find available RAM by scanning memory for 0xff bytes
 *	14 Jan 2000	interrupt enable/disable stuff
 */


#include "../config.h"
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/gdt.h>
#include <sys/arch/i386/idt.h>
#include <sys/arch/i386/pic.h>
#include <sys/interrupts.h>
#include <sys/defs.h>
#include <sys/std.h>


extern size_t malloc_firstaddr;
extern size_t malloc_lastaddr;


size_t kernel_image_size = 0;


void *i386_lowalloc__curaddress = 0;
void *i386_lowalloc__lowestaddress = 0;



size_t i386_lowfree ()
  {
    /*
     *	i386_lowfree ()
     *	---------------
     *
     *	Returns the amount of free "conventional" memory.
     */

    return (size_t)i386_lowalloc__curaddress - (size_t)i386_lowalloc__lowestaddress;
  }



void *i386_lowalloc (size_t len)
  {
    /*
     *	i386_lowalloc ()
     *	----------------
     *
     *	Allocate len bytes of conventional memory, and return a pointer to
     *	the allocated block. The memory is not free:able.
     */

    /*  Make sure we allocate in 16-byte chunks, for best performance:  */
    if (len & 15)
	len = (len | 15) + 1;

    i386_lowalloc__curaddress = (void *) ((u_int32_t) i386_lowalloc__curaddress - len);

    if (i386_lowalloc__curaddress < i386_lowalloc__lowestaddress)
	panic ("out of conventional (low) memory");

    return i386_lowalloc__curaddress;
  }



void machdep_interrupts (int state)
  {
    /*
     *	machdep_interrupts ()
     *	---------------------
     *
     *	Turns interrupts on or off.
     */

    if (state == DISABLE)
	asm ("cli");
    else
    if (state == ENABLE)
	asm ("sti");
    else
	panic ("machdep_interrupts(): unknown state %i", state);
  }



int machdep_oldinterrupts ()
  {
    /*
     *	machdep_oldinterrupts ()
     *	------------------------
     *
     *	Returns the current state of the interrupt flag.
     *	(TODO: the word "old" is very misleading here...)
     */

    volatile int flags = 0;

    asm ("pushf\npopl %eax");
    asm ("movl %%eax, %%ebx": "=b" (flags));

    return (flags & 0x200)? ENABLE : DISABLE;
  }



int machdep_main_startup ()
  {
    /*
     *	machdep_main_startup ()
     *	-----------------------
     *
     *	What we do here includes:
     *
     *		o)  Estimate size of kernel in memory
     *		o)  Prepare for i386_lowalloc()
     *		o)  Initialize GDT, PIC, IDT
     *		o)  Calculate how much memory the machine has
     */

    int failure;
    int memincrement;
    byte *p;
    int i;


    /*
     *	Estimate size of kernel in memory:
     *
     *	If the kernel image has an OpenBSD-like a.out header, we
     *	can simply add together the size of the text+data+bss+syms segments
     *	to get the kernel_image_size.
     *
     *	TODO: This assumes that the kernel was loaded at 0x100000 !
     */

    p = (byte *) 0x100000;
    kernel_image_size   = p[4]+p[5]*256+p[6]*65536+p[7]*16777216
			+ p[8]+p[9]*256+p[10]*65536+p[11]*16777216
			+ p[12]+p[13]*256+p[14]*65536+p[15]*16777216
			+ p[16]+p[17]*256+p[18]*65536+p[19]*16777216;

    if (kernel_image_size % PAGESIZE)
	kernel_image_size = ((kernel_image_size/PAGESIZE)+1)*PAGESIZE;


    /*
     *	Prepare for i386_lowalloc():
     *
     *	On some PS/2 systems, the conventional memory is only 639 KB,
     *	not 640 KB, so we begin at 0xa0000 - 1 KB and go downwards when
     *	someone wants a block of conventional memory.
     */

    i386_lowalloc__curaddress = (void *) 0xa0000 - 1024;
    i386_lowalloc__lowestaddress = (void *) KERNPDIRADDR+PAGESIZE;	/*  don't go too far down  */
    /*  This is ugly: the kernel pagedirectory is fixed at physical KERNPDIRADDR (0x10000).  */


    /*
     *	Initialize the global descriptor table (GDT), the 8259A PIC
     *	(interrupt controller), the interrupt descriptor table (IDT)...
     */

    gdt_init ();
    pic_init ();
    idt_init ();


    /*
     *	Calculate available memory:
     *
     *	If an entire page (or whatever) of memory is set to all
     *	0xff, then it might be unavailable.
     *
     *	TODO: there are several other ways to check for available
     *	memory... there should be support for more than one here.
     */

/*   TODO:  This depends on the kernel's load addr:  */
    malloc_firstaddr = 0x100000+kernel_image_size;

    /*  We scan for available RAM in chunks as large as this...  */
    memincrement = 262144;	/*  256 KB segments...  */

    failure = 0;
    malloc_lastaddr = 0x100000;

    while (!failure)
      {
	p = (byte *) malloc_lastaddr;
	if (p[0]==0xff)
	  {
	    /*  Is the entire page filled with 0xff?  */
	    for (i=0; i<memincrement; i++)
		if (p[i]==0xff)
		  failure++;

	    /*  If not all of the bytes in the page were 0xff, then we
		should continue searching...  */
	    if (failure<memincrement)
	      {
		failure = 0;
		malloc_lastaddr += memincrement;
	      }
	  }
	else
	  malloc_lastaddr += memincrement;
      }

    return 0;
  }

