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
 *  pic.c  --  8259A Programmable Interrupt Controller routines
 *
 *  History:
 *	24 Oct 1999	first version
 */


#include <sys/arch/i386/pic.h>
#include <sys/arch/i386/pio.h>


void pic_setmask (int mask)
  {
    /*  mask bits 0..15 control IRQ 0..15  */
    outb (0x21, mask & 255);
    outb (0xa1, (mask >> 8) & 255);
  }



void pic_init ()
  {
    /*
     *	8259A initialization.  (First some help from
     *	http://www.acm.uiuc.edu/sigops/roll_your_own/i386/irq.html,
     *	and then some (more correct) from
     *	http://www.nondot.org/sabre/os/H7Misc/8259pic.txt.)
     */

    /*  ICW1  */
    outb (0x20, 0x11);	/*  Master port A  */
    outb (0xa0, 0x11);	/*  Slave port A  */

    /*  ICW2  */
    outb (0x21, 0x20);	/*  Master offset of 0x20 in the IDT  */
    outb (0xa1, 0x28);	/*  Master offset of 0x28 in the IDT  */

    /*  ICW3  */
    outb (0x21, 0x04);	/*  Slaves attached to IR line 2  */
    outb (0xa1, 0x02);	/*  This slave in IR line 2 of master  */

    /*  ICW4  */
    outb (0x21, 0x01);	/*  Set non-buffered  */
    outb (0xa1, 0x01);	/*  Set non-buffered  */

    pic_setmask (0);
  }



void pic_eoi ()
  {
    /*  Send EOI to both master and slave  */
    outb (0x20, 0x20);	/*  master PIC  */
    outb (0xa0, 0x20);	/*  slave PIC  */
  }

