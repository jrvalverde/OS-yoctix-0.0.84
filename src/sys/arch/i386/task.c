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
 *  arch/i386/task.c  --  i386 task related functions
 */


#include "../config.h"
#include <string.h>
#include <sys/std.h>
#include <sys/arch/i386/gdt.h>
#include <sys/proc.h>
#include <sys/arch/i386/machdep.h>



extern byte *gdt;



void i386_settask (int nr, struct i386tss *tss)
  {
    /*
     *	i386_settask ()
     *	---------------
     *
     *	Set a GDT entry 'nr' to the type GDT_SEGTYPE_AVAILABLETSS and let it
     *	point to the specified tss struct.
     */

    i386setgdtentry (gdt + 8 * nr, sizeof(struct i386tss), (size_t) tss,
	GDT_SEGTYPE_AVAILABLETSS, GDT_DESCTYPE_SYSTEM, 0, 1, 0, 1, 1, 0);
  }



void i386_ltr (int nr)
  {
    /*
     *	i386_ltr ()
     *	-----------
     *
     *	The ltr instruction loads the task register with a specific value.
     *	It can be used before any process is started to tell the CPU where
     *	to save away the current cpu registers on the first task switch.
     */

    nr *= 8;
    asm ("ltr %%ax": : "a" (nr));
  }



void i386_jmptss (int nr)
  {
    /*
     *	i386_jmptss ()
     *	--------------
     *
     *	Switch to task nr 'nr'. Whenever "this" task (the one that is
     *	executing the ljmp instruction) is restarted, cs:eip points to
     *	the instruction immediately following the ljmp. (In other words,
     *	we return from i386_jmptss()...)
     */

    unsigned char buf[6];

    nr *= 8;
    buf[0]=0;
    buf[1]=0;
    buf[2]=0;
    buf[3]=0;
    buf[4]=nr & 255;
    buf[5]=nr >> 8;

    asm ("ljmp (%%eax)": : "a" (&buf));
  }

