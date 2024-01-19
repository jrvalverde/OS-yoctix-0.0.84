/*
 *  Copyright (C) 2000,2001 by Anders Gavare.  All rights reserved.
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
 *  modules/bus/isa/wdc/wdc.c
 *
 *	TODO: third (0x1e8) and fourth (0x168) controller ???
 *	secondary controller at irq 15 ??
 *
 *  History:
 *	27 Oct 2000	test
 *	2 Jan 2000	reading seems to work okay
 *	5 Jan 2000	simple partition support (PC slices + BSD partitions)
 */



#include "../../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/ports.h>
#include <sys/proc.h>
#include <sys/interrupts.h>
#include <sys/device.h>
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>

#include "disklabel.h"


#define	WDC_PORT_DATA		0
#define	WDC_PORT_ERROR		1
#define	WDC_PORT_SECTORCOUNT	2
#define	WDC_PORT_SECTOR		3
#define	WDC_PORT_CYLINDER_LO	4
#define	WDC_PORT_CYLINDER_HI	5
#define	WDC_PORT_DRIVEHEAD	6
#define	WDC_PORT_STATUS		7	/*  read   */
#define	WDC_PORT_COMMAND	7	/*  write  */

#define	WDC_PORT_DCR		0x206	/*  device control register  */

/*  Commands (sent to the WDC_PORT_COMMAND port):  */
#define	WDC_CMD_READ_WRETRY	0x20
#define	WDC_CMD_DRIVEID		0xEC


#define	WDC_MAXCONTROLLERS	2

struct wdc_controller
      {
	char		*name;
	int		iobase_lo;
	int		iobase_hi;
	u_int16_t	*drive_id [2];		/*  master & slave id   */
	struct device	*dev [2];		/*  master & slave dev  */
      };

struct wdc_controller wdc_controller [WDC_MAXCONTROLLERS] =
      {
	{  "primary",	0x1f0,	0x3f0, { NULL, NULL }, { NULL, NULL }  },
	{  "secondary",	0x170,	0x370, { NULL, NULL }, { NULL, NULL }  }
      };

struct wdc_partitiondev
      {
	struct device	*rawdev;
	daddr_t		start_offset;
	daddr_t		size;
      };


struct module	*wdc_m;
int		wdc_irqnr = 14;
volatile int	wdc_irqexpect = 0;
struct lockstruct wdc_lock;



void wdc_irqhandler ()
  {
/*
    int status;
    status = inb (WDC_PORT_STATUS + wdc_controller[0].iobase_lo);
*/
    if (!wdc_irqexpect)
        printk ("* stray wdc interrupt %i", wdc_irqnr);
/*
    else
        printk ("* wdc interrupt: okay");
*/

    wdc_irqexpect = 0;
    wakeup (wdc_irqhandler);
  }



int wdc_sendcommand (int baseport, int drive, int head,
	int cylinder, int sector, int seccount, int commandnr)
  {
    /*
     *	Return values:
     *	  0:  command sent ok
     *	  1:  BUSY (command not sent)
     *	  2:  Drive not ready (command not sent)
     *	  3:  no interrupt (command aborted?)
     */

    int i, d;

    /*  Wait for BUSY bit to clear:  */
    i = 15;
    while (i>0)
      {
	d = inb (baseport + WDC_PORT_STATUS);
	if ((d & 0x80) == 0x00)
	    i = -1;
	kusleep (1000);
	i--;
      }

    if (i==0)
	return 1;

    /*  Select drive, head, sector, sector count, and cylinder:  */
    outb (baseport + WDC_PORT_DRIVEHEAD, drive*16 + 0xA0 + head);
    outb (baseport + WDC_PORT_SECTORCOUNT, seccount);
    outb (baseport + WDC_PORT_SECTOR, sector);
    outb (baseport + WDC_PORT_CYLINDER_LO, cylinder & 255);
    outb (baseport + WDC_PORT_CYLINDER_HI, cylinder >> 8);

outb (0x3f6, 8);

    /*  Wait for DRDY bit to be set:  */
    i = 15;
    while (i>0)
      {
	d = inb (baseport + WDC_PORT_STATUS);
	if ((d & 0xc0) == 0x40)
	    i = -1;
	kusleep (1000);
	i--;
      }

    if (i==0)
	return 2;

    /*  Send the command:  */
    wdc_irqexpect = 1;
    outb (baseport + WDC_PORT_STATUS, commandnr);

    /*  Wait for command to finnish:  */
    i = 3000;
    while (i>0)
      {
	if ((inb (baseport + WDC_PORT_STATUS) & 0x80)==0)
	  return 0;
	if (wdc_irqexpect == 0)
	    i = -1;
	kusleep (1000);
	i--;
      }

    if (wdc_irqexpect==1)
      {
	wdc_irqexpect = 0;
	return 3;
      }

    return 0;
  }



int wdc_open (struct device *dev)
  {
    return 0;
  }



int wdc_close (struct device *dev)
  {
    return 0;
  }



#include "wdc_read.c"

#include "wdc_partitions.c"


int wdc_write (struct device *dev)
  {
    return ENODEV;
  }



int wdc_seek (struct device *dev)
  {
    return EINVAL;
  }



int wdc_ioctl (struct device *dev)
  {

    return ENOTTY;
  }



void wdc__idtoascii (char *asciistr, u_int16_t *drive_id, int start, int stop)
  {
    int i;
    int a = 0;

    for (i=start; i<=stop; i++)
      {
	asciistr[a++] = drive_id[i] >> 8;
	asciistr[a++] = drive_id[i] & 255;
      }

    asciistr[a] = 0;

    for (i=a-1; i>=0; i--)
	if (asciistr[i]==' ')
	  asciistr[i]=0;
	else
	if (asciistr[i]!=0)
	  i=0;
  }



void wdc_driveprobe (int controller, int unit)
  {
    u_int16_t *drive_id;
    int i;
    int port;

    drive_id = (u_int16_t *) malloc (256 * sizeof(u_int16_t));
    if (!drive_id)
      {
	printk ("wdc_driveprobe: out of memory");
	return;
      }

    port = wdc_controller[controller].iobase_lo;

    outb (port + WDC_PORT_DCR, 8 + 4);
    kusleep (100000);
    outb (port + WDC_PORT_DCR, 8);

    i = wdc_sendcommand (port, unit, 0, 0,0,0, WDC_CMD_DRIVEID);
    if (i != 0)
      {
	free (drive_id);
	return;
      }

    for (i = 0; i<256; i++)
	drive_id [i] = inw (port + WDC_PORT_DATA);

    outb (port + WDC_PORT_DCR, 8 + 4);
    kusleep (100000);
    outb (port + WDC_PORT_DCR, 8);

    wdc_controller[controller].drive_id[unit] = drive_id;
  }



void wdc_regdevices ()
  {
    char buf[120];
    int c, h, s;
    char asciistr [41];
    char asciistr2 [9];
    int cnr, dnr;
    u_int16_t *drive_id;
    struct device *dev;


    for (cnr=0; cnr<WDC_MAXCONTROLLERS; cnr++)
      for (dnr=0; dnr<=1; dnr++)
	if ((drive_id = wdc_controller[cnr].drive_id[dnr]))
	  {
	    c = drive_id [1];
	    h = drive_id [3];
	    s = drive_id [6];

	    /*
	     *  Show drive id. Something like this:
	     *
	     *  wd0 at wdc0 channel 0 drive 0: <Conner Peripherals 170MB - CP30174E>
	     *  wd0: serial nr "AM73W7H", rev "2.35", soft sectored, 32KB buffer
	     *  wd0: 162MB, 903 cyl, 8 head, 46 spt, 332304 sectors
	     */

	    wdc__idtoascii (asciistr, drive_id, 0x1b, 0x2e);
	    snprintf (buf, 120, "wd%i at %s channel %i drive %i: <%s>",
		dnr+2*cnr, wdc_m->shortname, cnr, dnr, asciistr);
	    printk ("%s", buf);

	    wdc__idtoascii (asciistr, drive_id, 0xa, 0x13);
	    wdc__idtoascii (asciistr2, drive_id, 0x17, 0x1a);

	    snprintf (buf, 120, "wd%i: serial nr \"%s\", "
		"rev \"%s\"%s%s%s, %i%sKB buffer",
		dnr+2*cnr, asciistr, asciistr2,
		(drive_id[0] & 0x04)? "" : ", soft sectored",
		(drive_id[0] & 0x08)? "" : ", MFM encoded",
		(drive_id[0] & 0x80)? ", removable" : "",
		drive_id[0x15]/2,
		(drive_id[0x15]==1)? ".5" : ""
		);

	    printk ("%s", buf);
	    printk ("wd%i: %iMB, %i cyl, %i head, %i spt, %i sectors",
		dnr+2*cnr, c*h*s/2048, c,h,s, c*h*s);

	    /*  Allocate the device struct:  */
	    dev = device_alloc (
		cnr==0?
			  (dnr==0? "wd0":"wd1")
			: (dnr==0? "wd2":"wd3"),
		wdc_m->shortname, DEVICETYPE_BLOCK, 0640, 0,0, 512);

	    if (dev)
	      {
		dev->open    = wdc_open;
		dev->close   = wdc_close;
		dev->read    = wdc_read;
		dev->write   = wdc_write;
		dev->seek    = wdc_seek;
		dev->ioctl   = wdc_ioctl;
		dev->readtip = NULL;  /*  wdc_readtip;  */

		wdc_controller[cnr].dev[dnr] = NULL;

		if (device_register (dev))
		  {
		    printk ("wdc_regdevices: could not register wd%i", cnr*2+dnr);
		    device_free (dev);
		  }
		else
		  wdc_controller[cnr].dev[dnr] = dev;

		/*  The "raw" device has been registered. Now, let's see
		    if this drive has partitions on it. If so, then they
		    are registered too:  */
		wdc_read_mbr (dev);
	      }
	    else
	      printk ("wdc: could not alloc device struct");
	  }
  }



void wdc_init (int arg)
  {
    char buf[100];
    int res;

    wdc_m = module_register ("isa0", MODULETYPE_NUMBERED, "wdc", "Harddisk Controller");
    if (!wdc_m)
	return;

    memset (&wdc_lock, 0, sizeof(wdc_lock));

    if (!irq_register (wdc_irqnr, (void *)&wdc_irqhandler, wdc_m->shortname))
      {
	printk ("could not register irq %i for %s already in use",
		wdc_irqnr, wdc_m->shortname);
	module_unregister (wdc_m);
	return;
      }

    res = ports_register (wdc_controller[0].iobase_lo, 8, wdc_m->shortname);
    if (!res)
      {
	printk ("%s: could not register ports 0x%Y-0x%Y", wdc_m->shortname,
		wdc_controller[0].iobase_lo, wdc_controller[0].iobase_lo+7);
	irq_unregister (wdc_irqnr);
	module_unregister (wdc_m);
	return;
      }

    res = ports_register (wdc_controller[1].iobase_lo, 8, wdc_m->shortname);
    if (!res)
      {
	printk ("%s: could not register ports 0x%Y-0x%Y", wdc_m->shortname,
		wdc_controller[1].iobase_lo, wdc_controller[1].iobase_lo+7);
	ports_unregister (wdc_controller[0].iobase_lo);
	irq_unregister (wdc_irqnr);
	module_unregister (wdc_m);
	return;
      }


    /*
     *	Probe for devices. If no devices were found, then we
     *	unregister the entire module.
     */

    wdc_driveprobe (0, 0);
    wdc_driveprobe (0, 1);

    wdc_driveprobe (1, 0);
    wdc_driveprobe (1, 1);

    if (!wdc_controller[0].drive_id[0] && !wdc_controller[0].drive_id[1])
	ports_unregister (wdc_controller[0].iobase_lo);

    if (!wdc_controller[1].drive_id[0] && !wdc_controller[1].drive_id[1])
	ports_unregister (wdc_controller[1].iobase_lo);

    if (!wdc_controller[0].drive_id[0] && !wdc_controller[0].drive_id[1]
	&& !wdc_controller[1].drive_id[0] && !wdc_controller[1].drive_id[1])
      {
	irq_unregister (wdc_irqnr);
	module_unregister (wdc_m);
	return;
      }

    isa_module_nametobuf (wdc_m, buf, 100);
    printk ("%s", buf);

    wdc_regdevices ();
  }

