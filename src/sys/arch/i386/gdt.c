/*
 *  Copyright (C) 1999 by Anders Gavare.  All rights reserved.
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
 *  gdt.c  --  i386 low-level GDT (Global Descriptor Table) routines
 *
 *  History:
 *	22 Oct 1999	first version
 *	24 Oct 1999	minor changes
 *	28 Dec 1999	fixed minor bugs
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/arch/i386/gdt.h>
#include <sys/arch/i386/machdep.h>


byte gdt_address [6];
byte *gdt = 0;



void i386setgdtentry (void *addr, size_t limit, size_t baseaddr, int segtype,
	int desctype, int dpl, int presentbit, int avl, int opsize, int dbit,
	int granularity)
  {
    /*
     *	Set up a Segment Descriptor according to
     *	"Intel486 Microprocessor Family Programmer's Reference Manual"
     *	without checking the arguments (should be correct, or the
     *	kernel is bad anyway)
     */

    byte *p = (byte *) addr;
    int i;

    /*  Zero everything at first, so we can OR values into the structure:  */
    for (i=0; i<8; i++)
	p[i] = 0;

    p[0] = limit & 255;			/*  Lowest 16 bits are the segment limit:  */
    p[1] = (limit >> 8) & 255;
    p[2] = baseaddr & 255;		/*  Bits 16..31 are baseaddress bits 0..15  */
    p[3] = (baseaddr >> 8) & 255;
    p[4] = (baseaddr >> 16) & 255;	/*  Bits 32..39 are baseaddr bits 16..23  */
    p[5] |= segtype;			/*  Bits 40..43 are segment Type  */
    p[5] |= (desctype * 16);		/*  Bit 44 is descriptor Type  */
    p[5] |= (dpl * 32);			/*  Bits 45..46 are DPL  */
    p[5] |= (presentbit * 128);		/*  Bit 47 is present bit  */
    p[6] |= ((limit >> 16) & 15);	/*  Bits 48..51 are limit bits 16..19  */
    p[6] |= (avl * 16);			/*  Bit 52 is available for system use  */
    p[6] |= (opsize * 32);		/*  Bit 53 is opsize  */
    p[6] |= (dbit * 64);		/*  Bit 54 is dbit  */
    p[6] |= (granularity * 128);	/*  Bit 55 is granularity  */
    p[7] = (baseaddr >> 24) & 255;	/*  Bits 56..63 are baseaddr bits 24..31  */
  }



void gdt_init ()
  {
    /*
     *	gdt_init ()  --  Initialize the GDT
     *	-----------------------------------
     *
     *	Each GDT entry is an 8 byte descriptor.
     *
     *		0: Null descriptor
     *		1: Kernel CODE descriptor
     *		2: Kernel DATA descriptor
     *		3: User CODE descriptor
     *		4: User DATA descriptor
     *		5: Dummy TSS descriptor
     *		6: ...		(one TSS descriptor for each process)
     */

    size_t sz;

    sz = 8 * (6 + MAX_PROCESSES);
    if (sz > 65536)
	panic ("gdt_init(): MAX_PROCESSES set too high");
    gdt = i386_lowalloc (sz);


    /*  0:  Null descriptor (we'll just zero out everything in the entire gdt...)  */
    memset (gdt+0, 0, sz);

    /*  1:  Kernel CODE descriptor  */
    i386setgdtentry (gdt + 8 * 1, 0xfffff, 0x0, GDT_SEGTYPE_EXEC | GDT_SEGTYPE_READ_CODE,
	GDT_DESCTYPE_CODEDATA, 0, 1, 0, 1, 1, 1);

    /*  2:  Kernel DATA descriptor  */
    i386setgdtentry (gdt + 8 * 2, 0xfffff, 0x0, GDT_SEGTYPE_WRITE_DATA,
	GDT_DESCTYPE_CODEDATA, 0, 1, 0, 1, 1, 1);

    /*  3:  User CODE descriptor  */
    i386setgdtentry (gdt + 8 * 3, 0xfffff, 0x0, GDT_SEGTYPE_EXEC |
	GDT_SEGTYPE_READ_CODE, GDT_DESCTYPE_CODEDATA, 3, 1, 0, 1, 1, 1);

    /*  4:  User DATA desctiptor  */
    i386setgdtentry (gdt + 8 * 4, 0xfffff, 0x0, GDT_SEGTYPE_WRITE_DATA,
	GDT_DESCTYPE_CODEDATA, 3, 1, 0, 1, 1, 1);


    /*
     *	Use the new gdt:
     */

    gdt_address [0] = (sz-1) & 255;	/*  Low byte of limit  */
    gdt_address [1] = (sz-1) / 256;	/*  High byte of limit  */
    gdt_address [2] = (u_int32_t)gdt & 255;	/*  Low byte of address  */
    gdt_address [3] = ((u_int32_t)gdt >> 8) & 255;
    gdt_address [4] = ((u_int32_t)gdt >> 16) & 255;
    gdt_address [5] = ((u_int32_t)gdt >> 24) & 255;  /*  Highest byte  */

    asm ("lgdt (%0)" : : "r" (&gdt_address[0]));

    asm ("movl $0x10, %eax\n"
	"\tmovl %ax, %ds\n"
	"\tmovl %ax, %es\n"
	"\tmovl %ax, %ss\n"
	"\tmovl $0, %eax\n"
	"\tmovl %ax, %fs\n"
	"\tmovl %ax, %gs");
  }

