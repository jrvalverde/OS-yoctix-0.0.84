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
 *  machdep_main_startup.c  --  do machine dependant low-level startup
 *  (first thing called from main())
 *
 *  Return 0 on success, >0 if failure.
 *
 *  History:
 *	9 Dec 2000	test
 */


#include "../config.h"
#include <sys/arch/mac68k/machdep.h>
#include <sys/defs.h>
#include <sys/std.h>


extern size_t malloc_firstaddr;
extern size_t malloc_lastaddr;

size_t kernel_image_size = 0;

int boothowto = 0;



void machdep_reboot ()
  {
    printk ("machdep_reboot(): failed");
    machdep_halt ();
  }



void machdep_halt ()
  {
    printk ("machdep_halt()");
    for (;;) ;
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

    byte *p;

    printk ("boothowto = 0x%x", boothowto);


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
     *	Calculate available memory:
     */

    malloc_firstaddr = 0x100000;
    malloc_lastaddr = 0x700000;

    return 0;
  }



int machdep_res_init ()
  {
    /*
     *	machdep_res_init ()
     *	-------------------
     */

    return 0;
  }



void machdep_vm_init ()
  {
    printk ("machdep_vm_init ()");
  }




