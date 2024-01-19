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
 *  modules/bus/isa/pccom/pccom.c  --  PC serial port driver
 *
 *  History:
 *	8 Dec 2000	test
 */



#include "../../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/ports.h>
#include <sys/interrupts.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>


#define	PCCOM_BIOSPORTS		0x00000400
#define	PCCOM_MAXPORTS		4


struct module *pccom_m [PCCOM_MAXPORTS];



volatile int pccom_irq4expect = 0;
volatile int pccom_irq3expect = 0;



void pccom_irq4handler ()
  {     
    if (!pccom_irq4expect)
      printk ("* stray pccom interrupt 4");
    else
      printk ("* pccom interrupt 4: okay");

    pccom_irq4expect = 0;
    wakeup (pccom_irq4handler);
  }



void pccom_irq3handler ()
  {     
    if (!pccom_irq3expect)
      printk ("* stray pccom interrupt 3");
    else
      printk ("* pccom interrupt 3: okay");

    pccom_irq3expect = 0;
    wakeup (pccom_irq3handler);
  }



void pccom_init (int arg)
  {
    char buf[100];
    int res;
    int i, p, intr;
    u_int16_t *portptr = (u_int16_t *) PCCOM_BIOSPORTS;


    memset (pccom_m, 0, sizeof(pccom_m));


    /*
     *	Register ports as found by BIOS:   (i386 specific)
     */

    for (i=0; i<4; i++)
      {
	p = portptr[i];
	intr = 4 - (i&1);	/*  4, 3, 4, 3  */

	if (p)
	  {
	    pccom_m[i] = module_register ("isa0", MODULETYPE_NUMBERED,
		"pccom", "PC serial (COM) port");
	    if (!pccom_m[i])
	      {
		printk ("pccom: out of memory?");
		continue;
	      }

	    res = ports_register (p, 8, pccom_m[i]->shortname);
	    if (!res)
	      {
		module_unregister (pccom_m[i]);
		continue;
	      }

	    if (i==0)
	      {
		if (!irq_register (intr, (void *)&pccom_irq4handler,
			pccom_m[i]->shortname))
		  {
		    printk ("%s: irq %i already in use",
			pccom_m[i]->shortname, intr);
		    module_unregister (pccom_m[i]);
		  }
	      }

	    if (i==1)
	      {
		if (!irq_register (intr, (void *)&pccom_irq3handler,
			pccom_m[i]->shortname))
		  {
		    printk ("%s: irq %i already in use",
			pccom_m[i]->shortname, intr);
		    module_unregister (pccom_m[i]);
		  }
	      }


/*  TODO:  Detect UART type etc.  */


	    isa_module_nametobuf (pccom_m[i], buf, sizeof(buf));
	    printk ("%s", buf);
	  }
	else
	  break;
      }
  }

