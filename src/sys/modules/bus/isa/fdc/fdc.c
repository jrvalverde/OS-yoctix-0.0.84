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
 *  modules/bus/isa/fdc/fdc.c
 *
 *	TODO:
 *		Support for write/format/...
 *		Finally fix the 765A/B problems
 *		Fix interrupt problems when also using the harddisk (?)
 *
 *  History:
 *	5 Jan 2000	first version
 *	5 Feb 2000	read single sector works
 *	15 Mar 2000	read multiple sectors in one call
 *	29 Jul 2000	motors are turned off after 3 seconds of inactivity
 */



#include "../../../../config.h"
#include <string.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/errno.h>
#include <sys/module.h>
#include <sys/ports.h>
#include <sys/proc.h>
#include <sys/interrupts.h>
#include <sys/dma.h>
#include <sys/device.h>
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>
#include <sys/timer.h>
#include <sys/time.h>



extern size_t i386_lowalloc__curaddress;
extern ticks_t system_ticks;
extern struct timespec system_time;

/*  Max nr of physical drives (could be increased to 4)  */
#define	MAX_DRIVES		2

/*  Using BIOS floppy drive IDs  */
#define	MAX_FDIDS		6

/*  Nr of retries on errors:  */
#define	NR_RETRIES		3


char *fdc_fdid[MAX_FDIDS] =
      {
	"none",				/*  0  */
	"360 KB, 5.25 inch",		/*  1  */
	"1.2 MB, 5.25 inch",		/*  2  */
	"720 KB, 3.5 inch",		/*  3  */
	"1.44 MB, 3.5 inch",		/*  4  */
	"2.88 MB, 3.5 inch",		/*  5  */
      };

int	fdc_nrcyls[MAX_FDIDS]	= { 0,   40,   80,   80,   80,   80 };
int	fdc_nrheads[MAX_FDIDS]	= { 0,    2,    2,    2,    2,    2 };
int	fdc_nrsec[MAX_FDIDS]	= { 0,    9,   15,    7,   18,   36 };
int	fdc_gap3[MAX_FDIDS]	= { 0,    0,   27,   27,   27,   27 };
int	fdc_stephl[MAX_FDIDS]	= { 0, 0xdf, 0xdf, 0xcf, 0xcf, 0xcf };


struct fdc_drivestatus
      {
	int	type;
	int	motor_enabled;
	int	cur_track;
	int	needs_recalibration;
	ticks_t	time_of_last_access;
      };

struct fdc_drivestatus fdc_drivestatus [MAX_DRIVES];


/*  These are offsets from fdc_portbase:  */
#define	FDCPORT_DOR		2		/*  Digital Output Register  */
#define	FDCPORT_STATUS		4		/*  Status register  */
#define	FDCPORT_COMMAND		5		/*  Command register  */
#define	FDCPORT_SETRATE		7		/*  Set data rate  */


/*  FDC commands:  */
#define	FDCCMD_SPECIFY		0x03		/*  Specify step & head load time  */
#define	FDCCMD_SENSE		0x04		/*  Sense Drive Status  */
#define	FDCCMD_READ		0x06 + 0xe0	/*  Read Data  */
#define	FDCCMD_WRITE		0x05 + 0xc0	/*  Write Data  */
#define	FDCCMD_RECALIBRATE	0x07		/*  Recalibrate drive  */
#define	FDCCMD_SENSEI		0x08		/*  Sense Interrupt Status  */
#define	FDCCMD_SEEK		0x0f		/*  Seek track  */
#define	FDCCMD_VERSION		0x10		/*  Get FDC version number  */


/*  Status bits:  */
#define	STATUS_IOREADY		128
#define	STATUS_FDCTOCPU		64
#define	STATUS_NONDMA		32
#define	STATUS_BUSY		16


int fdc_portbase;
int fdc_portmax;
int fdc_irqnr;
int fdc_dmanr;
int fdc_type;
size_t fdc_bufferaddr;
int fdc_controllertype;
volatile int fdc_irqexpect = 0;

struct lockstruct fdc_lock;

struct module *fdc_m;

char fdc_skewexperiment [80] =
      {
	 3,  4,  5,  5,  5,  5,  6,  6,  7,  7,
	 8,  8,  8,  9,  9,  9, 10, 10, 11, 11,
	11, 11, 12, 12, 13, 13, 14, 14, 14, 15,
	15, 15, 16, 16, 16, 17, 17,  0,  0,  0,
	 0,  1,  1,  1,  2,  2,  2,  3,  3,  4,
	 4,  4,  5,  5,  5,  6,  6,  7,  7,  8,
	 8,  8,  9,  9,  9, 10, 10, 10, 11, 11,
	12, 12, 13, 13, 13, 13, 14, 14, 15, 15
      };



void fdc_irqhandler ()
  {
    if (!fdc_irqexpect)
	printk ("* stray fdc interrupt %i", fdc_irqnr);
/*
    else
	printk ("* fdc interrupt: okay");
*/
    fdc_irqexpect = 0;
    wakeup (fdc_irqhandler);
  }



int fdc_whichsector (int abssector, int drive, int *cyl, int *head, int *sector)
  {
    /*  Convert absolute sector number to cylinder, head, and sector (1-based)     */
    /*  abssector = (sector-1) + (nrheads * secpertracj) * cyl + secpertrack * head    */

    drive = fdc_drivestatus[drive].type;

    /*  printk ("fdc_whichsector: abssector == %i", abssector);  */

    *sector = abssector % fdc_nrsec[drive];
    (*sector) ++;
    abssector /= fdc_nrsec[drive];
    *head = abssector % fdc_nrheads[drive];
    *cyl = abssector / fdc_nrheads[drive];

/*
    printk (" ==> cyl=%i head=%i sector=%i ==> new absolute = %i",
	*cyl, *head, *sector, (*sector-1)+(fdc_nrheads[drive]*fdc_nrsec[drive])*(*cyl)+
	fdc_nrsec[drive]*(*head));
*/

    if (*cyl >= fdc_nrcyls[drive])
	return 0;
    return 1;
  }



int fdc_sendcommand (int value)
  {
    int q = 1000;
    byte d;

    while (q>0)
      {
	/*  Is the FDC ready to receive data from the CPU?  */
	d = inb (fdc_portbase+FDCPORT_STATUS) & (STATUS_IOREADY + STATUS_FDCTOCPU);
	if (d == STATUS_IOREADY)
	  {
	    /*  Then send the value and return:  */
	    outb (fdc_portbase+FDCPORT_COMMAND, value);
	    return 1;
	  }

	kusleep (1000);
	q--;
      }

    /*  failed:  */
printk ("fdc_sendcommand failed");
    return 0;
  }



int fdc_readdata ()
  {
    int q = 1000;
    byte d;

    while (q>0)
      {
	/*  Is the FDC ready to send data to the CPU?  */
	d = inb (fdc_portbase+FDCPORT_STATUS) & (STATUS_IOREADY
		+ STATUS_FDCTOCPU + STATUS_BUSY);

	/*  FDC waiting for command?  */
	if (d == STATUS_IOREADY)
	    return -1;

	if (d == STATUS_IOREADY + STATUS_FDCTOCPU + STATUS_BUSY)
		return inb (fdc_portbase+FDCPORT_COMMAND);

	kusleep (1000);
	q--;
      }

    /*  failed:  */
printk ("fdc_readdata failed");
    return 0;
  }



int fdc_enable_motors ()
  {
    /*
     *	Enable drive motors by sending a command to the DOR of the FDC.
     *	(Actually, this function both enables and disables the motors,
     *	so it should probably be renamed.)
     */

    int val, i;

    val = 12;
    for (i=0; i<MAX_DRIVES; i++)
	if (fdc_drivestatus[i].motor_enabled)
		val += (16 << i);

    outb (fdc_portbase + FDCPORT_DOR, val);

    return 1;
  }



void fdc_disablemotors ()
  {
    /*  TODO: Actually, this turns off both drive one and
	drive two...  should be fixed (maybe have two
	timers running?  */

    if (lock (&fdc_lock, "fdc_disablemotors", LOCK_NONBLOCKING |
	LOCK_NOINTERRUPTS | LOCK_RW) < 0)
      return;

    fdc_drivestatus[0].motor_enabled = 0;
    fdc_drivestatus[1].motor_enabled = 0;
    fdc_enable_motors ();

    unlock (&fdc_lock);
  }



void fdc_set_disablemotor ()
  {
    /*
     *	Set up a kernel alarm to disable the drive motors if a specific
     *	number of seconds pass without any drive activity.
     */

    struct timespec ts;

    ts.tv_sec = 3;
    ts.tv_nsec = 0;

    timer_ksleep (&ts, &fdc_disablemotors, 1);
  }



int fdc_recalibrate_drive (int drive)
  {
    /*
     *	Recalibrates a drive (if it needs to be recalibrated)
     *	Returns 1 on success, 0 on failure.
     *	If this function is called when a drive does not need
     *	to be recalibrated, 0 is returned (failure).
     */

    int res, t;

    if (!fdc_drivestatus[drive].needs_recalibration)
	return 0;

    fdc_irqexpect = 1;
/*    interrupt_to_wait_for = fdc_irqnr; */
    fdc_sendcommand (FDCCMD_RECALIBRATE);
    fdc_sendcommand (drive);
    sleep (fdc_irqhandler, fdc_m->shortname);

    fdc_irqexpect = 1;
/*    interrupt_to_wait_for = fdc_irqnr; */
    fdc_sendcommand (FDCCMD_SENSE);
    fdc_sendcommand (drive);
    res = fdc_readdata();
    t = 1000;
    while (t>0 && fdc_irqexpect)
      {
	kusleep (200);
	t--;
      }
    if (fdc_irqexpect)
      {
	printk ("WARNING: no interrupt from FDCCMD_SENSE");
      }

    if (res >= 0x80)
	return 0;

    fdc_drivestatus[drive].needs_recalibration = 0;

    return 1;
  }



int fdc_seek_to_cylinder (int drive, int cylinder, int head)
  {
    /*
     *	If the drive head is already positioned at the right cylinder,
     *	we don't need to do this.  If we actually have to move the
     *	drive head, we send a FDCCMD_SEEK and sleep until the
     *	command has completed.
     *
     *	Returns 1 on success, 0 on failure.
     */

    int r1,r2;
    int retries = NR_RETRIES;
    int hardretries = NR_RETRIES;

seek_retry:

    if (fdc_drivestatus[drive].motor_enabled &&
	!fdc_drivestatus[drive].needs_recalibration
	&& fdc_drivestatus[drive].cur_track == cylinder)
	return 1;

    if (!fdc_drivestatus[drive].motor_enabled)
      {
	fdc_drivestatus[drive].motor_enabled = 1;

	/*  Enable motors:  */
	fdc_enable_motors ();

	/*  Wait for drive to spin up:  */
	kusleep (250000);
      }

    if (fdc_drivestatus[drive].needs_recalibration)
	fdc_recalibrate_drive (drive);

    fdc_irqexpect = 1;
/*    interrupt_to_wait_for = fdc_irqnr;*/
    fdc_sendcommand (FDCCMD_SEEK);
    fdc_sendcommand (head*4 + drive);
    fdc_sendcommand (cylinder);
    sleep (fdc_irqhandler, fdc_m->shortname);

    fdc_sendcommand (FDCCMD_SENSEI);
    r1 = fdc_readdata();
    r2 = fdc_readdata();

    /*  r1 is status register 0  */
    if ((r1 & 0xf8)==0x20)
      {
	/*  Command ok.  */

	/*  Was it the correct cylinder?  */
	if (r2 != cylinder)
	  return 0;

	fdc_drivestatus[drive].cur_track = r2;

	return 1;
      }

/*    printk ("fd%i: soft error: seek failed (r1=%y r2=%y)", drive,r1,r2);  */

    if (retries-- > 0)
	goto seek_retry;

    printk ("fd%i: hard error: seek failed, drive=%i cyl=%i head=%i (r1=%y r2=%y)",
	drive, drive,cylinder,head, r1,r2);
    fdc_drivestatus[drive].needs_recalibration = 1;

    retries = NR_RETRIES;
    if (hardretries-- > 0)
	goto seek_retry;

    return 0;
  }



int fdc_read_sectors (int drive, int abssector, int nrofsectors, byte *buf)
  {
    /*
     *	Read sectors starting at abssector into buf.
     *
     *	Return 1 on success, 0 on failure.
     */

    int retries = NR_RETRIES;
    int hardretries = NR_RETRIES;
    int c,h,s, i, res[7];

    if (nrofsectors<1)
      {
	printk ("fdc_read_sectors(): nrofsectors=%i", nrofsectors);
	return 0;
      }

read_retry:

    /*  Prepare the DMA for reading a sector:  */
    isadma_startdma (fdc_dmanr, (void *) fdc_bufferaddr,
		(size_t)(512*nrofsectors), 0x44);

    /*  Convert the absolute sector number to CHS:  */
    fdc_whichsector (abssector, drive, &c, &h, &s);

    /*  Seek:  */
    i = fdc_seek_to_cylinder (drive, c, h);
    if (i==0)
	return 0;

    /*  Specify head load time, step rate time:  */
    fdc_sendcommand (FDCCMD_SPECIFY);
    fdc_sendcommand (fdc_stephl[fdc_drivestatus[drive].type]);
    fdc_sendcommand (6);

    outb (fdc_portbase+FDCPORT_SETRATE, 0);

    fdc_irqexpect = 1;
/*    interrupt_to_wait_for = fdc_irqnr;*/
    fdc_sendcommand (FDCCMD_READ);
    fdc_sendcommand (h*4 + drive);
    fdc_sendcommand (c);		/*  Cylinder  */
    fdc_sendcommand (h);		/*  Head  */
    fdc_sendcommand (s);		/*  Sector  */
    fdc_sendcommand (2);		/*  0=128, 1=256, 2=512, 3=1024, ...  */
    fdc_sendcommand (s+nrofsectors-1);	/*  Last sector in track  */
    fdc_sendcommand (fdc_gap3[fdc_drivestatus[drive].type]);
    fdc_sendcommand (0xff);
    sleep (fdc_irqhandler, fdc_m->shortname);
    for (i=0; i<=6; i++)
	res[i] = fdc_readdata();

    if ((res[0]&0xf8)==0 && res[1]==0 && res[2]==0)
      {
	/*  Copy the resulting data to 'buf':  */
	memcpy (buf, (void *) fdc_bufferaddr, 512*nrofsectors);

	return 1;
      }


/*    printk ("fd%i: soft error: read failed (r0=%y, r1=%y r2=%y)", drive,res[0],res[1],res[2]);  */
    fdc_drivestatus[drive].needs_recalibration = 1;

    if (retries-- > 0)
	goto read_retry;

    printk ("fd%i: hard error: read failed abssec=%i (chs=%i,%i,%i) (r0=%y, r1=%y r2=%y)",
	drive, abssector, c,h,s, res[0],res[1],res[2]);

    retries = NR_RETRIES;
    if (hardretries-- > 0)
	goto read_retry;

    return 0;
  }



int fdc_write_sectors (int drive, int abssector, int nrofsectors, byte *buf)
  {
    /*
     *	Write sectors starting at abssector. Data is read from buf.
     *
     *	Return 1 on success, 0 on failure.
     */

    int retries = NR_RETRIES;
    int hardretries = NR_RETRIES;
    int c,h,s, i, res[7];

    if (nrofsectors<1)
      {
	printk ("fdc_write_sectors(): nrofsectors=%i", nrofsectors);
	return 0;
      }

write_retry:

    /*  Prepare the DMA for writinging a sector:  */
    isadma_startdma (fdc_dmanr, (void *) fdc_bufferaddr,
		(size_t)(512*nrofsectors), 0x48);

    memcpy ((void *) fdc_bufferaddr, buf, 512*nrofsectors);

    /*  Convert the absolute sector number to CHS:  */
    fdc_whichsector (abssector, drive, &c, &h, &s);

    /*  Seek:  */
    i = fdc_seek_to_cylinder (drive, c, h);
    if (i==0)
	return 0;

    /*  Specify head load time, step rate time:  */
    fdc_sendcommand (FDCCMD_SPECIFY);
    fdc_sendcommand (fdc_stephl[fdc_drivestatus[drive].type]);
    fdc_sendcommand (6);

    outb (fdc_portbase+FDCPORT_SETRATE, 0);

    fdc_irqexpect = 1;
/*    interrupt_to_wait_for = fdc_irqnr;*/
    fdc_sendcommand (FDCCMD_WRITE);
    fdc_sendcommand (h*4 + drive);
    fdc_sendcommand (c);		/*  Cylinder  */
    fdc_sendcommand (h);		/*  Head  */
    fdc_sendcommand (s);		/*  Sector  */
    fdc_sendcommand (2);		/*  0=128, 1=256, 2=512, 3=1024, ...  */
    fdc_sendcommand (s+nrofsectors-1);	/*  Last sector in track  */
    fdc_sendcommand (fdc_gap3[fdc_drivestatus[drive].type]);
    fdc_sendcommand (0xff);
    sleep (fdc_irqhandler, fdc_m->shortname);
    for (i=0; i<=6; i++)
	res[i] = fdc_readdata();

    if ((res[0]&0xf8)==0 && res[1]==0 && res[2]==0)
      {
	/*  Write went okay...  */
	return 1;
      }

    printk ("fd%i: soft error: write failed (r0=%y, r1=%y r2=%y)", drive,res[0],res[1],res[2]);
    fdc_drivestatus[drive].needs_recalibration = 1;

    if (retries-- > 0)
	goto write_retry;

    printk ("fd%i: hard error: write failed abssec=%i (chs=%i,%i,%i) (r0=%y, r1=%y r2=%y)",
	drive, abssector, c,h,s, res[0],res[1],res[2]);

    retries = NR_RETRIES;
    if (hardretries-- > 0)
	goto write_retry;

    return 0;
  }



int fdc_open (struct device *dev)
  {
    int drive, oldints;

    if (dev->refcount > 0)
	return EBUSY;

    drive = (dev->name[2]) - 48;
    if (drive<0 || drive>=MAX_DRIVES)
	return ENXIO;

    fdc_drivestatus [drive].motor_enabled = 0;
    fdc_drivestatus [drive].cur_track = 0;
    fdc_drivestatus [drive].needs_recalibration = 1;
    oldints = interrupts (DISABLE);
    fdc_drivestatus [drive].time_of_last_access = system_ticks;
    interrupts (oldints);

    dev->refcount = 1;
    return 0;
  }



int fdc_close (struct device *dev)
  {
    int drive;

    if (dev->refcount == 0)
	return ENODEV;

    dev->refcount = 0;

    drive = (dev->name[2]) - 48;
    if (drive<0 || drive>=MAX_DRIVES)
	return ENXIO;

    fdc_drivestatus [drive].motor_enabled = 0;
    fdc_drivestatus [drive].cur_track = 0;
    fdc_drivestatus [drive].needs_recalibration = 1;
    fdc_drivestatus [drive].time_of_last_access = 0;

    return 0;
  }



int fdc_read (struct device *dev, daddr_t blocknr, daddr_t nrofblocks,
	      byte *buf, struct proc *p)
  {
    int drive, res, oldints;

    if (!dev || !buf)
	return EINVAL;

    if (dev->refcount < 1)
	return EINVAL;

    drive = (dev->name[2]) - 48;

    lock (&fdc_lock, (void *) "fdc_read", LOCK_BLOCKING | LOCK_RW);
    res = fdc_read_sectors (drive, (int) blocknr, (int) nrofblocks, buf);
    unlock (&fdc_lock);

    /*  Set kernel timer "alarm" to turn off the drive's motor:  */
    oldints = interrupts (DISABLE);
    fdc_drivestatus[drive].time_of_last_access = system_ticks;
    interrupts (oldints);
    fdc_set_disablemotor ();

    if (!res)
	return EIO;

    return 0;
  }



int fdc_write (struct device *dev, daddr_t blocknr, daddr_t nrofblocks,
		byte *buf, struct proc *p)
  {
    int drive, res, oldints;

    if (!dev || !buf)
	return EINVAL;

    if (dev->refcount < 1)
	return EINVAL;

    drive = (dev->name[2]) - 48;

    lock (&fdc_lock, (void *) "fdc_write", LOCK_BLOCKING | LOCK_RW);
    res = fdc_write_sectors (drive, (int) blocknr, (int) nrofblocks, buf);
    unlock (&fdc_lock);

    /*  Set kernel timer "alarm" to turn off the drive's motor:  */
    oldints = interrupts (DISABLE);
    fdc_drivestatus[drive].time_of_last_access = system_ticks;
    interrupts (oldints);
    fdc_set_disablemotor ();

    if (!res)
	return EIO;

    return 0;
  }



int fdc_seek (struct device *dev)
  {

    return EINVAL;
  }



int fdc_ioctl (struct device *dev)
  {

    return ENOTTY;
  }



int fdc_readtip (struct device *dev, daddr_t blocknr, daddr_t *blocks_to_read,
		 daddr_t *startblock)
  {
    /*
     *	Return a tip (in blocks_to_read) specifying the number of 512-byte
     *	blocks to read in one call to fdc_read().  Set *startblock to the
     *	block nr where to start reading.
     *
     *	On floppies, the tip is always to read the full track (on one side
     *	of the disk).
     */

    int drive, c,h,s;
    int diff;

    if (!dev || !blocks_to_read)
	return EINVAL;

    if (dev->refcount < 1)
	return EINVAL;

    drive = (dev->name[2]) - 48;

    /*  Get cyl, head, sector. Sector is 1-based!!!  */
    fdc_whichsector ((int)blocknr, drive, &c, &h, &s);

    diff = fdc_drivestatus [drive].cur_track - c;
    if (diff<0)
	diff = -diff;
/*

    if (diff>=20)
      {
	*blocks_to_read = fdc_nrsec [fdc_drivestatus[drive].type] - s + 1;
      }
    else
      {
*/
	*blocks_to_read = fdc_nrsec [fdc_drivestatus[drive].type];
	*startblock = (fdc_nrheads[fdc_drivestatus[drive].type] * c + h)
		* fdc_nrsec[fdc_drivestatus[drive].type];
/*
      }
*/
/*
    s--;
    if (s > fdc_skewexperiment[diff]+1)
	s = fdc_skewexperiment[diff]+1;
    *blocks_to_read = fdc_nrsec [fdc_drivestatus[drive].type] - s;
    *startblock = (fdc_nrheads[fdc_drivestatus[drive].type] * c + h)
	* fdc_nrsec[fdc_drivestatus[drive].type] + s;
*/

#if DEBUGLEVEL>=5
    printk ("  fdc readtip: %i => chs %i %i %i, blocks_to_read=%i startblock=%i",
	(int)blocknr, c,h,s, (int) *blocks_to_read, (int) *startblock);
#endif

    return 0;
  }



void fdc_driveprobe (int unit, int type)
  {
    struct device *dev;
    char buf[120];
    int c,h,s;

    fdc_drivestatus [unit].type = type;
    fdc_drivestatus [unit].motor_enabled = 0;
    fdc_drivestatus [unit].cur_track = 0;
    fdc_drivestatus [unit].needs_recalibration = 1;

    /*  TODO:  true non-BIOS detection  */
    if (type < 1)
	return;


    c = fdc_nrcyls [type];
    h = fdc_nrheads [type];
    s = fdc_nrsec [type];

    snprintf (buf, 120, "fd%i at %s drive %i: %iKB %i cyl, %i head, %i sec",
		unit, fdc_m->shortname, unit, 512*c*h*s/1024, c,h,s);
    printk ("%s", buf);

    /*  TODO: be able to register fd2 and fd3 too  */
    dev = device_alloc (unit==0? "fd0":"fd1",
	fdc_m->shortname, DEVICETYPE_BLOCK, 0640, 0,0, 512);

    if (!dev)
      {
	printk ("fdc: could not alloc device struct");
	return;
      }

    dev->open  = fdc_open;
    dev->close = fdc_close;
    dev->read  = fdc_read;
    dev->write = fdc_write;
    dev->seek  = fdc_seek;
    dev->ioctl = fdc_ioctl;
    dev->readtip = fdc_readtip;

    if (device_register (dev))
      {
	printk ("fdc: trouble registering device");
	device_free (dev);
      }
  }



int fdc_probecontroller (int baseport)
  {
    /*  Return 1 if controller found, 0 otherwise.  */
    int t=0, type;

    fdc_portbase = baseport;  fdc_portmax = baseport + 7;

    /*  reset fdc  */
    fdc_irqexpect = 1;
    outb (baseport + FDCPORT_DOR, 0);
    kusleep (100000);

    fdc_irqexpect = 1;
    outb (baseport + FDCPORT_DOR, 12);
    kusleep (50000);

    /*  Get controller type:  */
    fdc_irqexpect = 1;
    if (!fdc_sendcommand (FDCCMD_VERSION))
	return 0;

    type = fdc_readdata ();
    fdc_controllertype = type;

    if (type==0x90)
      {
	t = 500;
	while (t>0 && fdc_irqexpect)
	  {
	    kusleep (1000);
	    t--;
	  }

	if (fdc_irqexpect)
	  {
	    printk ("%s: warning: sense failed (type 0x%y portbase 0x%Y irq %i)",
		fdc_m->shortname, type, fdc_portbase, fdc_irqnr);
	  }
      }

    fdc_irqexpect = 0;

    fdc_sendcommand (FDCCMD_SENSEI);
    t = fdc_readdata ();
    t = fdc_readdata ();

    return 1;
  }



void fdc_init (int arg)
  {
    char buf[100];
    int cmosdata;
    int res;
    int tmp;

    fdc_m = module_register ("isa0", MODULETYPE_NUMBERED, "fdc", "Floppy Disk Controller");

    if (!fdc_m)
	return;

    memset (&fdc_lock, 0, sizeof(fdc_lock));


    /*
     *	Probe for Floppy Driver Controller
     *
     *	IRQ must be 6, DMA must be 2 (?)
     *	but ports can be either 0x3f0-0x3f7 or 0x370-0x377
     */

    fdc_irqnr = 6;
    fdc_dmanr = 2;

    if (!irq_register (fdc_irqnr, (void *)&fdc_irqhandler, fdc_m->shortname))
      {
	printk ("could not register fdc0: irq %i already in use", fdc_irqnr);
	module_unregister (fdc_m);
	return;
      }


    if (!fdc_probecontroller (0x3f0) && !fdc_probecontroller (0x370))
      {
	printk ("fdc not present or unknown model");
	irq_unregister (fdc_irqnr);
	module_unregister (fdc_m);
	return;
      }

    res = ports_register (fdc_portbase, 8, fdc_m->shortname);
    if (!res)
      {
	printk ("%s: could not register ports 0x%Y-0x%Y", fdc_m->shortname, fdc_portbase, fdc_portbase+7);
	irq_unregister (fdc_irqnr);
	module_unregister (fdc_m);
	return;
      }

    res = dma_register (fdc_dmanr, fdc_m->shortname);
    if (!res)
      {
	printk ("%s: could not register dma %i", fdc_m->shortname, fdc_dmanr);
	ports_unregister (fdc_portbase);
	irq_unregister (fdc_irqnr);
	module_unregister (fdc_m);
	return;
      }


    isa_module_nametobuf (fdc_m, buf, 100);
    if (fdc_controllertype == 0x80 || fdc_controllertype == 0x90)
	snprintf (buf+strlen(buf), 100-strlen(buf),
		fdc_controllertype==0x80? ": uPD765A" : ": uPD765B");
    else
	snprintf (buf+strlen(buf), 100-strlen(buf), ": unknown type 0x%y",
			fdc_controllertype);
    printk ("%s", buf);


    /*  Allocate 512*36 bytes for the floppy DMA buffer:  */
    fdc_bufferaddr = (size_t) i386_lowalloc (512*36);
    tmp = fdc_bufferaddr & 65535;
    if (tmp > 65535-512*36)
	fdc_bufferaddr = (size_t) i386_lowalloc (512*36);


    /*
     *	Probe for floppy devices
     */

    cmosdata = cmos_read (0x10) & 255;

    fdc_driveprobe (0, cmosdata / 16);
    fdc_driveprobe (1, cmosdata & 15);
/*  fdc_driveprobe (2, -1);  */
/*  fdc_driveprobe (3, -1);  */

return;

{
int res, base, cyldiff;
s_int64_t t1,t2,diff;
char buf[512];
s_int64_t diffs[18];
s_int64_t bestdiff;	/*  shortest time  */
int bestskew;
int bestskews [80];


for (cyldiff=0; cyldiff<80; cyldiff++)
  {

    bestdiff = 10000000;
    bestskew = -1;

    for (base=0; base<18; base++)
      {
	res=fdc_read_sectors (0, 18*2* 0 + 17, 1, buf);
	t1 = system_time.tv_sec*1000000 + system_time.tv_nsec/1000;

	res=fdc_read_sectors (0, 18*2* cyldiff + base, 1, buf);
	t2 = system_time.tv_sec*1000000 + system_time.tv_nsec/1000;

	diff = t2-t1;
/*
	printk ("%i cyls away, %i sectors:  time diff = %i usec",
		cyldiff, base, diff);
*/
	diffs[base] = diff;
	if (diff < bestdiff)
	  {
	    bestdiff = diff;
	    bestskew = base;
	  }
      }

    bestskews[cyldiff] = bestskew;
    printk ("For %i cyls away: bestskew = %i (bestdiff=%i usec)",
		cyldiff, bestskew, bestdiff);

  }

panic ("yo");
}

  }

