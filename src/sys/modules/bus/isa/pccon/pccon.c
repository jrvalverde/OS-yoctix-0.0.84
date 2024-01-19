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
 *  modules/bus/isa/pccon/pccon.c
 *
 *  History:
 *	5 Jan 2000	first version
 *	14 Mar 2000	virtual consoles
 *	13 Jun 2000	using the general terminal_* interface
 *	19 Oct 2000	minor cleanup; /dev/console = /dev/ttyC0
 */



#include "../../../../config.h"
#include <stdio.h>
#include <string.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/ports.h>
#include <sys/proc.h>
#include <sys/console.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/interrupts.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>
#include <sys/lock.h>

#include "pccon.h"


extern size_t userland_startaddr;

extern byte pccon_keypressed [128];
extern byte pccon_charconvert [];

struct device *pccon_dev [MAX_NR_OF_VCS];	/*  /dev/ttyC*    */
struct device *pccon_devconsole;		/*  /dev/console  */

struct pccon_vc *pccon_vc [MAX_NR_OF_VCS];
int pccon_current_vc;

struct lockstruct pccon_writelock;

struct module *pccon_m;



struct pccon_vc *pccon_vc_init (int rows, int columns, int nr)
  {
    /*
     *	pccon_vc_init ()
     *	----------------
     *
     *	This function allocates memory to hold a "virtual console" and its
     *	data (termios flags, character cell values (including colors), and
     *	location of the cursor), and sets the initial data to relatively
     *	sane values.
     *
     *	Returns a pointer to a pccon_vc struct if successful, NULL otherwise.
     */

    struct pccon_vc *p;
    int i;

    p = (struct pccon_vc *) malloc (sizeof(struct pccon_vc));
    if (!p)
	return NULL;

    memset (p, 0, sizeof(struct pccon_vc));

    p->nrows = rows;
    p->ncols = columns;

    /*  Allocate memory for the character cells.  */
    p->cells = (struct charcell *)
	malloc (sizeof(struct charcell) * rows * columns);
    if (!p->cells)
      {
	free (p);
	return NULL;
      }

    /*  Allocate termstate struct:  */
    p->ts = terminal_open ();
    if (!p->ts)
      {
	free (p->cells);
	free (p);
	return NULL;
      }

    /*  Data needed to use the termios system:  */
    p->ts->id		  = nr;
    p->ts->write	  = pccon_termwrite;
    p->ts->read		  = pccon_termread;
    p->ts->termio.c_oflag = OPOST | ONLCR;
    p->ts->termio.c_iflag = ICRNL;
    p->ts->termio.c_lflag = ECHO | ECHOE | ICANON;

    /*  Clear the screen buffer ...  */
    for (i=rows*columns-1; i>=0; i--)
	p->cells[i].ch = ' ', p->cells[i].color = 7;

    /*  ... and "home" the cursor:  */
    p->cursor_row    = 0;
    p->cursor_column = 0;
    p->cursor_type   = PCCON_CURSOR_LINE;
    p->current_color = 7;

    /*
     *	Set default "lock" mode:  The default is to not have any num/caps/
     *	scroll lock on, but different people like different settings...
     *	Use PCCON_NUMLOCK for numlock etc. Use logical "OR" for multiple locks.
     */

    p->lockmode = 0;
    pccon_update_leds ();

    p->videomode = 0;

    return p;
  }



int pccon_termread ()
  {
    /*
     *	Special termios code to process incomming characters:  none.  :-)
     */

    return 0;
  }



int pccon_termwrite (int id, char *buf, size_t len)
  {
    /*
     *	Termios code which handles outgoing data (ie writes to the screen).
     *	This function is both called when a process (or the kernel) wants
     *	to output data to the vc, but also as a result of key input (if
     *	local echo was enabled).  This funcion should only be called by
     *	the termios subsystem.
     *
     *	TODO: This is slow, the pccon_printchar() stuff should probably
     *	be inlined in some way...
     */

    int oldints;

    if (!buf || id >= MAX_NR_OF_VCS || id < 0)
      return EINVAL;

    oldints = interrupts (DISABLE);
    interrupts (oldints);

    if (oldints == ENABLE)
      lock (&pccon_vc[id]->lock, "pccon_write", LOCK_BLOCKING | LOCK_RW);

    while (len > 0)
      pccon_printchar (id, buf[0]), buf++, len--;

    if (oldints == ENABLE)
      unlock (&pccon_vc[id]->lock);
    else
      interrupts (oldints);

    return 0;
  }



int pccon__get_vc_nr (struct device *dev)
  {
    /*
     *	Get the "device number" from dev->name.
     *
     *	The name is of one of the following three forms:
     *
     *		ttyCx	ttyCyy	console
     *
     *	where x (or y) is a 0-based virtual console number. The name
     *	'console' is always considered to be ttyC0.
     *
     *	Returns -1 on error, 0..MAX_NR_OF_VCS-1 on success.
     */

    int vc_nr;

    if (!dev)
	return -1;

    if (dev->name[0]=='c')				/*  console  */
	vc_nr = 0;
    else
      {
	vc_nr = (dev->name[4]) - 48;			/*  ttyCx    */
	/*  Is there more than one digit?  */
	if (dev->name[5])				/*  ttyCyy   */
	  vc_nr = vc_nr*10 + (dev->name[5])-48;
      }

    if (vc_nr<0 || vc_nr>=MAX_NR_OF_VCS)
        return -1;

    return vc_nr;
  }



int pccon_open (struct device *dev, struct proc *p)
  {
    /*
     *	pccon_open ()
     *	-------------
     */

    int vc_nr;

    vc_nr = pccon__get_vc_nr (dev);
    if (vc_nr<0)
	return ENXIO;

    /*
     *	If this virtual console hasn't been initialized yet, then let's do
     *	so now:
     */

    if (!pccon_vc [vc_nr])
	pccon_vc [vc_nr] = pccon_vc_init (25, 80, vc_nr);

    dev->refcount++;

    return 0;
  }



int pccon_close (struct device *dev, struct proc *p)
  {
    /*
     *	pccon_close ()
     *	--------------
     */

    int vc_nr;

    vc_nr = pccon__get_vc_nr (dev);
    if (vc_nr<0)
	return ENXIO;

    if (dev->refcount == 0)
	panic ("pccon_close: ttyC%i refcount already 0!", vc_nr);

    dev->refcount--;

    return 0;
  }



int pccon_read (off_t *res, struct device *dev, byte *buf, off_t buflen, struct proc *p)
  {
    /*
     *	pccon_read ()
     *	-------------
     */

    int vc_nr;

    vc_nr = pccon__get_vc_nr (dev);
    if (vc_nr<0 || !res || !buf)
	return ENXIO;

    *res = terminal_read (pccon_vc[vc_nr]->ts, buf, buflen);
    return 0;
  }



int pccon_write (off_t *res, struct device *dev, byte *buf, off_t buflen, struct proc *p)
  {
    /*
     *	pccon_write ()
     *	--------------
     */

    size_t len;
    size_t origlen = buflen;
    int vc_nr;

    vc_nr = pccon__get_vc_nr (dev);
    if (vc_nr<0 || !res || !buf)
	return ENXIO;

    /*
     *	Call terminal_write() to convert the data (NL -> CR+NL)
     *	and actually output chars to the screen...
     */

    while (buflen > 0)
      {
	len = terminal_write (pccon_vc[vc_nr]->ts, buf, buflen);
	if (len < 1)
	  buflen = 0;
	else
	  {
	    buflen -= len;
	    buf += len;
	  }
      }

    *res = origlen;

    return 0;
  }



int pccon_ioctl (struct device *dev, int *res, struct proc *p, unsigned long req, unsigned long arg1)
  {
    /*
     *	pccon_ioctl ()
     *	--------------
     *
     *	TODO:  This needs to be less hardcoded... :)
     */

    byte *outaddr, *inaddr;
    struct winsize tmpwinsize;
    int vc_nr;

    vc_nr = pccon__get_vc_nr (dev);
    if (vc_nr<0)
	return ENXIO;

    /*
     *	TODO:  this is extremely ugly...
     *	We should check arguments, userland writability etc
     */

    /*  TIOCGETA:  */
    if (req == 0x402c7413)
      {
	outaddr = (byte *)arg1 + userland_startaddr;
	memcpy (outaddr, &pccon_vc[vc_nr]->ts->termio, sizeof(struct termios));
	*res = 0;
	return 0;
      }

    /*  TIOCGPGRP:  */
    if (req == 0x40047477)
      {
	outaddr = (byte *)arg1 + userland_startaddr;
	memcpy (outaddr, &pccon_vc[vc_nr]->ts->pgrp, sizeof(pid_t));
	*res = 0;
	return 0;
      }

    /*  TIOCSPGRP:  */
    if (req == 0x80047476)
      {
	inaddr = (byte *)arg1 + userland_startaddr;
	memcpy (&pccon_vc[vc_nr]->ts->pgrp, inaddr, sizeof(pid_t));
	*res = 0;
	return 0;
      }

    /*  TIOCGWINSZ:  */
    if (req == 0x40087468)
      {
	outaddr = (byte *)arg1 + userland_startaddr;
	tmpwinsize.ws_row = pccon_vc[vc_nr]->nrows;
	tmpwinsize.ws_col = pccon_vc[vc_nr]->ncols;
	tmpwinsize.ws_xpixel = 0;
	tmpwinsize.ws_ypixel = 0;
	memcpy (outaddr, &tmpwinsize, sizeof(struct winsize));
	*res = 0;
	return 0;
      }

    /*  TIOCSETA or TIOCSETAW:  */
    if (req == 0x802c7414 || req == 0x802c7415)
      {
	/* TODO: TIOCSETAW differs from TIOCSETA in that it wants output to be
	   written first...  this is not implemented yet  */
	/*  actually, "inaddr" would be a better name... TODO  */
	inaddr = (byte *)arg1 + userland_startaddr;
	memcpy (&pccon_vc[vc_nr]->ts->termio, inaddr, sizeof(struct termios));
	*res = 0;
	return 0;
      }

    printk ("pccon_ioctl: unimplemented req=%x arg=%x", (int)req, (int)arg1);

    *res = 0;
    return 0;
  }



void pccon_init (int arg)
  {
    char buf[80];
    int resi1, res60, res64;
    int i, res;
    char tmp_name[20];


    pccon_m = module_register ("isa0", MODULETYPE_NUMBERED, "pccon", "PC Console Driver");
    if (!pccon_m)
      {
	printk ("pccon: could not register module at isa0");
	return;
      }

    memset (&pccon_writelock, 0, sizeof(pccon_writelock));


    /*
     *	Register ports 0x60 and 0x64, and IRQ 1:
     */

    res60 = ports_register (0x60, 1, pccon_m->shortname);
    res64 = ports_register (0x64, 1, pccon_m->shortname);
    resi1 = irq_register (1, (void *)&pccon_irqhandler, pccon_m->shortname);

    if (!resi1 || !res60 || !res64)
      {
	if (!resi1)
	  printk ("Could not register irq 1 for %s", pccon_m->shortname);
	if (!res60)
	  printk ("Could not register port %y for %s", 0x60, pccon_m->shortname);
	if (!res64)
	  printk ("Could not register port %y for %s", 0x64, pccon_m->shortname);
	ports_unregister (0x60);
	ports_unregister (0x64);
	module_unregister (pccon_m);
	return;
      }

    if (!pccon_initkeybuf ())
      {
	printk ("pccon: not enough memory for keybuf");
	irq_unregister (1);
	ports_unregister (0x60);
	ports_unregister (0x64);
	module_unregister (pccon_m);
	return;
      }

    for (i=0; i<MAX_KEYS; i++)
	pccon_keypressed [i] = 0;

    /*  Initialize the keyboardmap (a Swedish one is a good choice  :-)  */
    pccon_keymapinit ();

    /*  Detect color or b/w:  */
    pccon_detect_color ();

    /*  Set up character conversion:  */
    for (i=0; i<256; i++)
	pccon_charconvert [i] = i;

    pccon_charconvert [0xe5] = 0x86;	/*  å  */
    pccon_charconvert [0xe4] = 0x84;	/*  ä  */
    pccon_charconvert [0xf6] = 0x94;	/*  ö  */
    pccon_charconvert [0xc5] = 0x8f;	/*  Å  */
    pccon_charconvert [0xc4] = 0x8e;	/*  Ä  */
    pccon_charconvert [0xd6] = 0x99;	/*  Ö  */

    /*  Register the virtual console devices:  */
    for (i=0; i<MAX_NR_OF_VCS; i++)
      {
	pccon_vc [i] = NULL;

	/*  Names are ttyC0..ttyC9,ttyC10,ttyC11,...  */
	snprintf (tmp_name, 20, "ttyC%i", i);

	pccon_dev [i] = device_alloc (tmp_name,
		pccon_m->shortname, DEVICETYPE_CHAR, 0600, 0, 0, 0);

	if (!pccon_dev[i])
	  printk ("pccon: warning: could not allocate device struct");
	else
	  {
	    pccon_dev[i]->open  = pccon_open;
	    pccon_dev[i]->close = pccon_close;
	    pccon_dev[i]->read  = pccon_read;
	    pccon_dev[i]->write = pccon_write;
	    pccon_dev[i]->ioctl = pccon_ioctl;

	    if ((res = device_register (pccon_dev[i])))
	      {
		printk ("pccon: device_register() = %i (error)", res);
		device_free (pccon_dev[i]);
	      }
	  }
      }

    if (!pccon_dev[0])
      {
	printk ("pccon: WARNING! Could not register device /dev/ttyC0");
      }
    else
      {
	pccon_devconsole = device_alloc ("console",
		pccon_m->shortname, DEVICETYPE_CHAR, 0600, 0, 0, 0);

	if (!pccon_devconsole)
	  printk ("pccon: WARNING! Could not alloc device /dev/console");
	else
	  {
	    pccon_devconsole->open  = pccon_open;
	    pccon_devconsole->close = pccon_close;
	    pccon_devconsole->read  = pccon_read;
	    pccon_devconsole->write = pccon_write;
	    pccon_devconsole->ioctl = pccon_ioctl;

	    if ((res = device_register (pccon_devconsole)))
	      {
		printk ("pccon: device_register() = %i (error)", res);
		device_free (pccon_devconsole);
	      }
	  }
      }


    /*
     *	Manually initialize virtual console 0.  It is neccessary to do it
     *	this way, since we're "taking over" from the simple console (see
     *	md/console.c) and therefore need to copy the current position of
     *	the cursor.
     */

    pccon_current_vc = 0;
    pccon_vc [0] = pccon_vc_init (25, 80, 0);
    pccon_gethwcursorpos (&pccon_vc[0]->cursor_row,
	&pccon_vc[0]->cursor_column);

    console_puts = pccon_slowputs;

    isa_module_nametobuf (pccon_m, buf, 80);
    snprintf (buf+strlen(buf), 80-strlen(buf), ": %i virtual consoles", MAX_NR_OF_VCS);
    printk ("%s", buf);

    pccon_update_leds ();
  }

