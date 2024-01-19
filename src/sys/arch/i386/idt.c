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
 *  idt.c  --  i386 low-level IDT (Interrupt Descriptor Table) routines
 *
 *  History:
 *	24 Oct 1999	first version
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/arch/i386/gdt.h>
#include <sys/arch/i386/idt.h>
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/exceptions.h>
#include <sys/arch/i386/syscall.h>


byte idt_address [6];
byte *idt = 0;



void irq0_asm();
void irq1_asm();
void irq2_asm();
void irq3_asm();
void irq4_asm();
void irq5_asm();
void irq6_asm();
void irq7_asm();
void irq8_asm();
void irq9_asm();
void irqa_asm();
void irqb_asm();
void irqc_asm();
void irqd_asm();
void irqe_asm();
void irqf_asm();



void i386setidtentry (void *addr, byte *offset, int segmentselector, int type, int dpl)
  {
    /*
     *	Set up a Interrupt Descriptor Table entry according to
     *	"Intel486 Microprocessor Family Programmer's Reference Manual"
     *	without checking the arguments (should be correct, or the
     *	kernel is bad anyway)
     */

    byte *p = (byte *) addr;
    int i;

    /*  Zero everything at first, so we can OR values into the structure:  */
    for (i=0; i<8; i++)
	p[i] = 0;

    p[0] = (size_t)offset & 255;
    p[1] = ((size_t)offset >> 8) & 255;
    p[2] = segmentselector & 255;
    p[3] = (segmentselector >> 8) & 255;
    p[4] = 0;
    p[5] = 128 + dpl * 32 + type;
    p[6] = ((size_t)offset >> 16) & 255;
    p[7] = ((size_t)offset >> 24) & 255;
  }



void idt_init ()
  {
    /*
     *	idt_init ()  --  Initialize the IDT
     *	-----------------------------------
     *
     *	Entry 0..17:	Exceptions
     *	Entry 18..31:	Reserved
     *	Entry 32..47:	Hardware IRQs
     *	Entry 48..127:	Nothing
     *	Entry 128:	syscall entry
     *	Entry 129..255:	Nothing
     */

    size_t sz;


    sz = 8 * 256;
    idt = i386_lowalloc (sz);

    i386exceptions_init ();

/*  Should this be trapgate or interruptgate?  (TODO) */
/*    i386setidtentry (idt + 8*128, (byte *) &arch_syscallasm, SEL_CODE, IDT_TYPE_TRAPGATE, 3); */
    i386setidtentry (idt + 8*128, (byte *) &arch_syscallasm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 3);

    i386setidtentry (idt + 8*0x20, (byte *) &irq0_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x21, (byte *) &irq1_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x22, (byte *) &irq2_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x23, (byte *) &irq3_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x24, (byte *) &irq4_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x25, (byte *) &irq5_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x26, (byte *) &irq6_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x27, (byte *) &irq7_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x28, (byte *) &irq8_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x29, (byte *) &irq9_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x2a, (byte *) &irqa_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x2b, (byte *) &irqb_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x2c, (byte *) &irqc_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x2d, (byte *) &irqd_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x2e, (byte *) &irqe_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
    i386setidtentry (idt + 8*0x2f, (byte *) &irqf_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);


    /*
     *	Use the new idt:
     */

    idt_address [0] = (sz-1) & 255;	/*  Low byte of limit  */
    idt_address [1] = (sz-1) / 256;	/*  High byte of limit  */
    idt_address [2] = (u_int32_t)idt & 255;	/*  Low byte of address  */
    idt_address [3] = ((u_int32_t)idt >> 8) & 255;
    idt_address [4] = ((u_int32_t)idt >> 16) & 255;
    idt_address [5] = ((u_int32_t)idt >> 24) & 255;  /*  Highest byte  */

    asm ("lidt (%0)" : : "r" (&idt_address[0]));
  }

