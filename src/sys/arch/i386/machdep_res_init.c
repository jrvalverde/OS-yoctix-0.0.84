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
 *  arch/i386/machdep_res_init.c  --  initialize machine depentant resources
 *	(NOT devices). Console output is possible using printk() or panic()
 *
 *	(*)	CPU identification
 *	(*)	BIOS identification
 *	(*)	...
 *
 *  I tried reading the CMOS, but the information there is worthless, so
 *  I removed that part.
 *
 *  Return 0 on success, >0 if failure.
 *
 *  History:
 *	18 Oct 1999	first version
 *	21 Oct 1999	removed call to cmos_init()
 *	5 Jan 2000	actually registering cpu0 at mainbus0, but no
 *			detection of cpu type yet...
 */


#include "../config.h"
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/pio.h>
#include <sys/arch/i386/gdt.h>
#include <sys/std.h>
#include <sys/timer.h>
#include <sys/interrupts.h>
#include <sys/proc.h>
#include <string.h>
#include <stdio.h>
#include <sys/malloc.h>
#include <sys/module.h>


extern struct timespec system_time;


void i386_settask (int nr, struct i386tss *tss);
void i386_ltr (int nr);



void machdep_ignore ()
  {
    return;
  }



int machdep_res_init ()
  {
    int sec,min,hour,day,month,year;
    time_t totalseconds;


    /*
     *	machdep_res_init ()
     *	-------------------
     *
     *		o)  Identify the CPU
     *		o)  Identify the BIOS
     *		o)  Setup a dummy TSS
     *		o)  Setup system_time
     */

    struct module *m;
    char buf [80];
    struct i386tss *dummytss;


    /*
     *	Identify the CPU:
     *
     *	TODO:  actually perform a true cpu identification...
     */

    m = module_register ("mainbus0", MODULETYPE_SYSTEM | MODULETYPE_BUILTIN |
		MODULETYPE_NUMBERED, "cpu", "Processor");
    module_nametobuf (m, buf, 80);
    snprintf (buf+strlen(buf), 80-strlen(buf), ": i386");
    printk ("%s", buf);


    /*
     *	Identify the BIOS:
     */

    bios_init ();


    /*
     *	Set up a dummy TSS:  (The i386 needs to have an "old" TSS when
     *	we switch tasks, so that the current processor state can be
     *	saved away somewhere.)
     */

    dummytss = (struct i386tss *) malloc (sizeof(struct i386tss));
    memset (dummytss, 0, sizeof(struct i386tss));
    i386_settask (5, (struct i386tss *) dummytss);
    i386_ltr (5);


    /*
     *	Set system_time according to CMOS data:
     */

    sec   = cmos_read (0);
    min   = cmos_read (2);
    hour  = cmos_read (4);
    day   = cmos_read (7);
    month = cmos_read (8);
    year  = cmos_read (9);

    /*  Binary or BCD format?  */
    if ((cmos_read(0xb) & 4)==0)
      {
	sec   = bcd_to_int (sec);
	min   = bcd_to_int (min);
	hour  = bcd_to_int (hour);
	day   = bcd_to_int (day);
	month = bcd_to_int (month);
	year  = bcd_to_int (year);
      }
    else
      printk ("cmos: date/time in binary format, not bcd (?)");

    /*  Century always in BCD format??? (TODO)  */
    year += (100 * bcd_to_int (cmos_read (0x32)));

    totalseconds = time_rawtounix (year, month, day, hour, min, sec);
    system_time.tv_sec = totalseconds;
    system_time.tv_nsec = 0;


    /*
     *	Ignore IRQ7:	(these occur all the time on some of my boxes...)
     *			(TODO: figure out why... :-)
     */

    irq_register (7, (void *) machdep_ignore, "ignore_irq7");

    return 0;
  }

