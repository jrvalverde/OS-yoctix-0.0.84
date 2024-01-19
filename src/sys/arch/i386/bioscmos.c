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
 *  bioscmos.c  --  BIOS version detection etc.
 *
 *	cmos_read() and cmos_write() reads and writes CMOS data.
 *	bios_init() initializes/detects the BIOS.
 *
 *  TODO:
 *	Store this data somewhere else, so that it is accessible!
 *	BIOS32 stuff (?)
 *
 *
 *  History:
 *	18 Oct 1999	first version
 *	21 Oct 1999	removing BIOS manufacturer strings -- if you
 *			absolutely need to know if you are running
 *			an original IBM PC/XT, then this could be added.
 *			(I don't think Yoctix runs on the XT :)
 *	17 Dec 1999	adding com/lpt stuff
 *	5 Jan 2000	cmos_read() and cmos_write() functions
 *	7 Jan 2000	actually registering bios? at mainbus0
 */


#include <string.h>
#include <stdio.h>
#include <sys/defs.h>
#include <sys/md/machdep.h>
#include <sys/console.h>
#include <sys/std.h>
#include <sys/module.h>
#include <sys/arch/i386/pio.h>
#include <sys/interrupts.h>



int cmos_read (int pos)
  {
    outb (0x70, pos);
    asm ("jmp 1f\n1:");
    return inb (0x71);
  }



void cmos_write (int pos, int value)
  {
    outb (0x70, pos);
    asm ("jmp 1f\n1:");
    outb (0x71, value);
  }



int bcd_to_int (int bcd)
  {
    return 10*((bcd & 255)/16)+(bcd & 15);
  }



void machdep_halt ()
  {
    asm ("cli\nhlt");
  }



void machdep_reboot ()
  {
    volatile int t;
    int t2;
    int p=0x64,d=0xfe;

    interrupts (DISABLE);
    for (t2=10; t2<1000000000; t2 *= 10)
      {
	outb (p, d);
	for (t=1; t<t2; t++)
	  ;
      }

    printk ("machdep_reboot(): not reached");
    machdep_halt ();
  }



void bios_init ()
  {
    struct module *m;
    char	buf [100];
    int		i;
    byte *	ptr=0;
    int		bios32;

    byte	i386_bios_id1 = 0;
    byte	i386_bios_id2 = 0;
    char	i386_bios_date [9];
    int		i386_bios_com1 = 0;
    int		i386_bios_com2 = 0;
    int		i386_bios_com3 = 0;
    int		i386_bios_com4 = 0;
    int		i386_bios_lpt1 = 0;
    int		i386_bios_lpt2 = 0;
    int		i386_bios_lpt3 = 0;
    int		i386_bios_lpt4 = 0;


    /*
     *  Get machine ID bytes and BIOS date:
     */

    i386_bios_id1 = ptr[0xffffe];
    i386_bios_id2 = ptr[0xfffff];

    memcpy (i386_bios_date, (char *) 0xffff5, 8);
    i386_bios_date[8]='\0';
    for (i=0; i<8; i++)
	if (i386_bios_date[i]<32 || i386_bios_date[i]>126)
		i386_bios_date[i]='\0';


    /*
     *	BIOS DATA AREA (at address 0x00000400)
     */

    i386_bios_com1 = ptr[0x400] + ptr[0x401]*256;
    i386_bios_com2 = ptr[0x402] + ptr[0x403]*256;
    i386_bios_com3 = ptr[0x404] + ptr[0x405]*256;
    i386_bios_com4 = ptr[0x406] + ptr[0x407]*256;
    i386_bios_lpt1 = ptr[0x408] + ptr[0x409]*256;
    i386_bios_lpt2 = ptr[0x40a] + ptr[0x40b]*256;
    i386_bios_lpt3 = ptr[0x40c] + ptr[0x40d]*256;
    i386_bios_lpt4 = ptr[0x40e] + ptr[0x40f]*256;


    /*
     *	BIOS32
     *	(TODO: check checksum etc.)
     */

    bios32 = 0;
    for (i=0xe0000; i<0xffff0; i+=16)
	if (!strncmp (ptr+i, "_32_", 4))
	  {
	    bios32 = i;
	    break;
	  }


    /*
     *	Display results, for example:
     *	bios0 at mainbus0: id=fc,f6 date=12/01/94
     */

    m = module_register ("mainbus0", MODULETYPE_SYSTEM | MODULETYPE_BUILTIN |
	MODULETYPE_NUMBERED, "bios", "PC BIOS");
    module_nametobuf (m, buf, 100);

    snprintf (buf+strlen(buf), 100-strlen(buf), ": id=%y,%y, date=%s",
	i386_bios_id1, i386_bios_id2, i386_bios_date);

    if (bios32)
	snprintf (buf+strlen(buf), 100-strlen(buf), ", bios32=0x%x", bios32);

    printk ("%s", buf);
  }

