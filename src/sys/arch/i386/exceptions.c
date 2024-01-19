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
 *  exceptions.c  --  i386 exception handlers
 *
 *	When an exception occurs, we need to handle it. Usually, this means
 *	killing the process causing the exception.
 *
 *	Page fault exceptions are handled by vm_fault().
 *
 *  History:
 *	24 Oct 1999	first version
 *	?? 2000		page fault stuff
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/interrupts.h>
#include <sys/syscalls.h>
#include <sys/arch/i386/gdt.h>
#include <sys/arch/i386/idt.h>
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/exceptions.h>


extern byte *idt;
extern volatile struct proc *curproc;
extern size_t userland_startaddr;


void exception0_asm();
void exception1_asm();
void exception2_asm();
void exception3_asm();
void exception4_asm();
void exception5_asm();
void exception6_asm();
void exception7_asm();
void exception8_asm();
void exception9_asm();
void exception10_asm();
void exception11_asm();
void exception12_asm();
void exception13_asm();
void exception14_asm();
void exception15_asm();
void exception16_asm();
void exception17_asm();
void exceptionreserved_asm();
void exceptioninvalid_asm();



void i386exceptions_init ()
  {
    /*
     *	i386exceptions_init ()
     *	----------------------
     *
     *	Initializes the Interrupt Descriptor Table.
     */

    int i;

    i386setidtentry (idt + 8 * 0, (byte *)exception0_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 1, (byte *)exception1_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 2, (byte *)exception2_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 3, (byte *)exception3_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 4, (byte *)exception4_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 5, (byte *)exception5_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 6, (byte *)exception6_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 7, (byte *)exception7_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 8, (byte *)exception8_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 9, (byte *)exception9_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 10, (byte *)exception10_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 11, (byte *)exception11_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 12, (byte *)exception12_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 13, (byte *)exception13_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 14, (byte *)exception14_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 15, (byte *)exception15_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 16, (byte *)exception16_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  
    i386setidtentry (idt + 8 * 17, (byte *)exception17_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);  

    for (i=18; i<0x20; i++)
      i386setidtentry (idt + 8*i, (byte *)exceptionreserved_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);

    for (i=0x20; i<=0xff; i++)
      i386setidtentry (idt + 8*i, (byte *)exceptioninvalid_asm, SEL_CODE, IDT_TYPE_INTERRUPTGATE, 0);
  }



u_int32_t machdep_pagefaulthandler (size_t linearaddr, int errorcode)
  {
    /*
     *	machdep_pagefaulthandler ()
     *	---------------------------
     *
     *	All we have to do is to call vm_fault() with correct parameters.
     *	Interrupts are disabled when the function is entered, but once we
     *	have saved the current value of curproc, they don't need to be
     *	disabled. (Actually, if we don't enable interrupts again, then the
     *	system will only be able to handle one page fault at a time which
     *	would be REALLY slow...)
     *
     *	linearaddr is the linear address. (Yeah, really! :-)
     *	To convert it to a userland address, we subtract the userland
     *	starting address.
     */

    struct proc *p;

    p = (struct proc *) curproc;
    interrupts (ENABLE);

#if DEBUGLEVEL>=8
    printk ("pagefault pid %i linearaddr %x errorcode %x", p->pid, linearaddr, errorcode);
#endif

    vm_fault (p, linearaddr - userland_startaddr,
	((errorcode&2)? VM_PAGEFAULT_WRITE : VM_PAGEFAULT_READ)
	+ ((errorcode&1)? 0 : VM_PAGEFAULT_NOTPRESENT) );

    return p->md.tss->cr3;
  }



void machdep_exceptionhandler (int nr, int a,int b,int c,int d,int e, int f,int g,int h,
	int i, int j, int k, int l, int m)
  {
    /*
     *	machdep_exceptionhandler ()
     *	---------------------------
     *
     *	The generic exception handler. We end up here if no other (specific)
     *	exception handler caught the exception.
     */

    ret_t tmp_result;
    struct proc *p;

    p = (struct proc *) curproc;
    interrupts (ENABLE);

    printk ("exception #%i pid %i cs:eip=%Y:%x stack %x %x %x %x %x %x %x %x %x %x %x %x %x", nr, curproc->pid,
	curproc->md.tss->cs, curproc->md.tss->eip, a,b,c,d,e,f,g,h,i,j,k,l,m);

    sys_exit (&tmp_result, p, 0);

    panic ("machdep_exceptionhandler: not reached");
  }


