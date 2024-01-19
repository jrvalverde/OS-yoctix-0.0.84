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
 *  timer.c  --  mac68k system timer functions
 *
 *  History:
 *	9 Dec 2000	first version
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/timer.h>
#include <sys/interrupts.h>


extern volatile struct proc *curproc;
extern volatile ticks_t system_ticks;

int switchratio = 1;

void timer_asm ();
void timer ();


void machdep_timer_init ()
  {
    /*
     *	machdep_timer_init ()
     *	---------------------
     *
     *	Initialize the i386 system timer:
     *
     *		(o)  Prepare an entry for our timer routine in the IDT.
     *
     *		(o)  Program the 8253 PIT (Programmable Interval
     *			Timer) to trigger HZ times per second.
     *
     *		(o)  Enable interrupts (turn on the system timer).
     */

/*
    u_int32_t v;

*/
    /*  Register IRQ 0:  (machine independant)  */
  /*  irq_register (0, (void *) &timer, "timer");
*/

    /*  Set frequency of the 8253 PIT to HZ:  */
/*    v = 1193180;  or is it 1192500 ???  
    v /= HZ;
    outb (0x43, 0x36);
    outb (0x40, v % 255);
    outb (0x40, v / 256);
    pic_setmask (0);
*/
    switchratio = (int) (HZ / DEFAULT_SWITCH_HZ);

    /*  Let's make sure that interrupts are enabled:  */
    machdep_interrupts (ENABLE);
  }


