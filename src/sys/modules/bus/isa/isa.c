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
 *  modules/bus/isa/isa.c
 *
 *	Information about how to program the DMA chips was obtained from
 *	a file written by "Breakpoint of Tank" <nstalker@iag.net>.
 *
 *  History:
 *	5 Jan 2000	first version
 *	7 Jan 2000	adding isadma
 */



#include "../../../config.h"
#include <sys/std.h>
#include <stdio.h>
#include <string.h>
#include <sys/module.h>
#include <sys/ports.h>
#include <sys/interrupts.h>
#include <sys/dma.h>
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>



extern struct portsrange *portsranges;
extern char *irq_handler_name [];
extern char *dma_owner [];


int	isadma_maskreg [MAX_DMA_CHANNELS]	= { 0x0a, 0x0a, 0x0a, 0x0a, 0xd4, 0xd4, 0xd4, 0xd4 };
int	isadma_modereg [MAX_DMA_CHANNELS]	= { 0x0b, 0x0b, 0x0b, 0x0b, 0xd6, 0xd6, 0xd6, 0xd6 };
int	isadma_clearreg [MAX_DMA_CHANNELS]	= { 0x0c, 0x0c, 0x0c, 0x0c, 0xd8, 0xd8, 0xd8, 0xd8 };

int	isadma_pagereg [MAX_DMA_CHANNELS]	= { 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };
int	isadma_addrreg [MAX_DMA_CHANNELS]	= { 0x00, 0x02, 0x04, 0x06, 0xc0, 0xc4, 0xc8, 0xcc };
int	isadma_countreg [MAX_DMA_CHANNELS]	= { 0x01, 0x03, 0x05, 0x07, 0xc2, 0xc6, 0xca, 0xce };

struct module *isa_m;
struct module *isadma_m;

void isadma_init (int);
void isapnp_init (int);



void isa_init (int arg)
  {
    char buf[40];

    isa_m = module_register ("mainbus0", MODULETYPE_NUMBERED, "isa", "ISA bus");

    if (!isa_m)
	return;

    module_nametobuf (isa_m, buf, 40);
    printk ("%s", buf);

    isadma_init (arg);
  }



int isa_module_nametobuf (struct module *m, char *buf, int buflen)
  {
    /*
     *	Create a nice dmesg line:
     *
     *	  module0 at parent0 port XXXX irq Y dma Z
     *
     *	where XXXX, Y, and Z can be a comma-separated list of values.
     *	Return 1 on success, 0 on failure.
     */

    struct portsrange *p;
    int found, i;


    if (!m || !buf || buflen<1)
	return 0;

    module_nametobuf (m, buf, buflen);


    /*  Is the module using any ports?				*/
    /*	In other words, find m->shortname in portsranges:	*/
    found = 0;
    p = portsranges;
    while (p)
      {
	if (!strcmp (m->shortname, p->owner))
	  {
	    if (!found)
	      {
		found = 1;
		snprintf (buf+strlen(buf), buflen-strlen(buf), " port ");
	      }
	    else
		snprintf (buf+strlen(buf), buflen-strlen(buf), ",");

	    if (p->baseport < 0x100)
		snprintf (buf+strlen(buf), buflen-strlen(buf), "0x%y", p->baseport);
	    else
		snprintf (buf+strlen(buf), buflen-strlen(buf), "0x%Y", p->baseport);

	    if (p->nrofports > 1)
	      {
		i = p->baseport + p->nrofports-1;
		if (i < 0x100)
		  snprintf (buf+strlen(buf), buflen-strlen(buf), "-0x%y", i);
		else
		  snprintf (buf+strlen(buf), buflen-strlen(buf), "-0x%Y", i);
	      }
	  }

        p = p->next;
      }


    /*  IRQs?  */
    found = 0;
    for (i=0; i<MAX_IRQS; i++)
      {
	if (irq_handler_name[i] && !strcmp(m->shortname, irq_handler_name[i]))
	  {
	    if (!found)
	      {
		found = 1;
		snprintf (buf+strlen(buf), buflen-strlen(buf), " irq ");
	      }
	    else
		snprintf (buf+strlen(buf), buflen-strlen(buf), ",");

	    snprintf (buf+strlen(buf), buflen-strlen(buf), "%i", i);
	  }
      }


    /*  DMA?  */
    found = 0;
    for (i=0; i<MAX_DMA_CHANNELS; i++)
      {
	if (dma_owner[i] && !strcmp(m->shortname, dma_owner[i]))
	  {
	    if (!found)
	      {
		found = 1;
		snprintf (buf+strlen(buf), buflen-strlen(buf), " dma ");
	      }
	    else
		snprintf (buf+strlen(buf), buflen-strlen(buf), ",");

	    snprintf (buf+strlen(buf), buflen-strlen(buf), "%i", i);
	  }
      }


    return 1;
  }



void isadma_init (int arg)
  {
    char buf[150];
    int res;

    isadma_m = module_register (isa_m->shortname,
	MODULETYPE_NUMBERED, "isadma", "ISA Direct Memory Access");

    if (!isadma_m)
	return;

    res = 0;
    res += ports_register (0x0, 16, isadma_m->shortname);
    res += ports_register (0x80, 16, isadma_m->shortname);
    res += ports_register (0xc0, 32, isadma_m->shortname);

    if (res < 3)
      {
	printk ("%s: could not register dma ports", isadma_m->shortname);
	ports_unregister (0x0);
	ports_unregister (0x80);
	ports_unregister (0xc0);
	module_unregister (isadma_m);
	return;
      }

    isa_module_nametobuf (isadma_m, buf, 150);
    printk ("%s", buf);
  }



void isadma_stopdma (int channelnr)
  {
    /*  Mask the DMA channel:  */
    outb (isadma_maskreg[channelnr], 0x04 + (channelnr & 3));

    /*  Stop any currently executing transfers:  */
    outb (isadma_clearreg[channelnr], 0);

    /*  Unmask the DMA channel:  */
    outb (isadma_maskreg[channelnr], channelnr & 3);
  }



int isadma_startdma (int channelnr, void *addr, size_t len, int mode)
  {
    /*
     *	Sets up everything needed to do DMA data transfer.
     *
     *	channelnr is the DMA channel number
     *	addr is the physical address of the buffer (it may not cross
     *		a physical 64KB boundary!)
     *	len is the length in bytes to send/receive
     *	mode is either ISADMA_READ or ISADMA_WRITE.
     *
     *	Return 1 on success, 0 on failure.
     */

    if (!addr || channelnr>=MAX_DMA_CHANNELS || len<1 || len>65536)
	return 0;

    /*  Mask the DMA channel:  */
    outb (isadma_maskreg[channelnr], 0x04 + (channelnr & 3));

    /*  Stop any currently executing transfers:  */
    outb (isadma_clearreg[channelnr], 0);

    /*  Set the mode of the channel:  */
    outb (isadma_modereg[channelnr], mode + (channelnr & 3));

    /*  Send the address:  */
    outb (isadma_addrreg[channelnr], (int)addr & 255);
    outb (isadma_addrreg[channelnr], ((int)addr >> 8) & 255);
    outb (isadma_pagereg[channelnr], ((int)addr >> 16) & 255);

    /*  Send the length:  */
    outb (isadma_countreg[channelnr], (len-1) & 255);
    outb (isadma_countreg[channelnr], ((len-1) >> 8) & 255);

    /*  Unmask the DMA channel:  */
    outb (isadma_maskreg[channelnr], channelnr & 3);

    return 1;
  }

