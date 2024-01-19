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
 *  interrupts.c
 *
 *  History:
 *	2 Dec 2000
 */


#include "../config.h"
#include <sys/arch/mac68k/machdep.h>
#include <sys/arch/mac68k/psl.h>
#include <sys/defs.h>
#include <sys/std.h>
#include <sys/interrupts.h>



void machdep_interrupts (int state)
  {
    /*
     *	machdep_interrupts ()
     *	---------------------
     *
     *	Turns interrupts on or off.
     */

    switch (state)
      {
	case DISABLE:
		asm ("movw #0x2700, %sr");
		return;
	case ENABLE:
		asm ("movw #0x2000, %sr");
		return;
      }

    printk ("machdep_interrupts: invalid state=%i", state);
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

    int flags = 0;

    asm ("movw sr, d0");
    asm ("movl d0, %0" : "=g" (flags) );

    return (flags & PSL_IPL)? DISABLE : ENABLE;
  }


