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
 *  modules/bus/isa/pccon/pccon_keyb.c
 *
 *  History:
 *	5 Jan 2000	first version
 *	2 Jun 2000	now, CAPS-lock only shifts alpha keys (assuming
 *			Swedish keyboard layout...)
 */



#include "../../../../config.h"
#include <stdio.h>
#include <string.h>
#include <sys/std.h>
#include <sys/defs.h>
#include <sys/malloc.h>
#include <sys/interrupts.h>
#include <sys/arch/i386/pio.h>
#include <sys/terminal.h>

#include "pccon.h"


extern struct pccon_vc *pccon_vc[];
extern int pccon_current_vc;

extern size_t pccon_videoaddr;

/*  If a key is down, it's entry in pccon_keypressed[] is 1, otherwise 0.  */
byte pccon_keypressed [MAX_KEYS];

/*  Convert scancode to ASCII (or whatever)  */
byte pccon_keymap [MAX_KEYS];
byte pccon_keymap_altgr [MAX_KEYS];
byte pccon_keymap_shift [MAX_KEYS];
byte pccon_keymap_shift_altgr [MAX_KEYS];


byte	*pccon_keybuf = NULL;
int	pccon_keybufsize;
int	pccon_keybufreadptr;
int	pccon_keybufwriteptr;



int pccon_initkeybuf ()
  {
    /*
     *	Initialize the keyboard read buffer. Returns 1 on success, 0 on
     *	failure.
     */

    pccon_keybuf = (byte *) malloc (KEYBOARD_BUFFER_LEN);
    if (!pccon_keybuf)
	return 0;

    pccon_keybufsize	 = KEYBOARD_BUFFER_LEN;
    pccon_keybufreadptr	 = 0;
    pccon_keybufwriteptr = 0;

    return 1;
  }



void pccon_update_leds ()
  {
    /*
     *	Update the LEDs on the keyboard according to the current
     *	virtual console's lockmode.
     */

    volatile byte a;


    /*
     *	TODO: Is this good? Hopefully it doesn't take too much time,
     *	but what if the keyboard breaks before we exit the loop? Then
     *	the system would hang (if interrupts are disabled).
     */

    do a = inb (0x64); while (a & 0x02);	/*  wait until ready  */

    /*  Command to set the LEDs:  */
    outb (0x60, 0xed);

    do a = inb (0x64); while (a & 0x02);

    /*  Calculate value to send to the keyboard controller:  */
    a = 0;
    if (pccon_vc[pccon_current_vc]->lockmode & PCCON_SCROLLLOCK) a += 1;
    if (pccon_vc[pccon_current_vc]->lockmode & PCCON_NUMLOCK)	 a += 2;
    if (pccon_vc[pccon_current_vc]->lockmode & PCCON_CAPSLOCK)	 a += 4;

    outb (0x60, a);
  }



void pccon_addchartokeybuf (char c)
  {
    pccon_keybuf [pccon_keybufwriteptr++] = c;
    if (pccon_keybufwriteptr >= pccon_keybufsize)
	pccon_keybufwriteptr = 0;
    if (pccon_keybufwriteptr == pccon_keybufreadptr)
	printk ("pccon0: keyboard buffer overflow");
  }



int pccon_readcharfromkeybuf ()
  {
    /*
     *	Read a character from the keyboard input buffer, and return
     *	its value.  If there was no character available, return -1.
     */

    int retval;

    if (pccon_keybufwriteptr == pccon_keybufreadptr)
	return -1;

    retval = pccon_keybuf [pccon_keybufreadptr++];
    if (pccon_keybufreadptr >= pccon_keybufsize)
	pccon_keybufreadptr = 0;

    return retval;
  }



int data_is_alphakey (int d)
  {
    /*
     *	Return 1 if d is a scancode for an alpha key.
     *	TODO: This includes the Swedish åäö-keys, which
     *	will cause [ on US keyboard maps to become { whenever
     *	capslock is active. This should probably be fixed.
     */

    if ((d>=0x10 && d<=0x1a) || (d>=0x1e && d<=0x28) ||
	(d>=0x2c && d<=0x32))
      return 1;

    return 0;
  }



void pccon_irqhandler ()
  {
    byte a, status, data, data2, data3=0;
    int fkey, newvc;
    int shifted, ctrled;

    status = inb (0x64);
    if (status & 1)
      {
	data = inb (0x60);

	if (data == 0xe0)
	  {
	    /*  Wait for next byte.  */
	    do a = inb (0x64); while (! (a & 0x01));

	    data2 = inb (0x60);

	    switch (data2)
	      {
		case 0x38:
			pccon_keypressed [SPECIAL_RIGHTALT] = 1;
			break;
		case 0xb8:
			pccon_keypressed [SPECIAL_RIGHTALT] = 0;
			break;
		case 0x1d:
			pccon_keypressed [SPECIAL_RIGHTCTRL] = 1;
			break;
		case 0x9d:
			pccon_keypressed [SPECIAL_RIGHTCTRL] = 0;
			break;
		case 0x5b:
			pccon_keypressed [SPECIAL_LEFTWINDOWS] = 1;
			break;
		case 0xdb:
			pccon_keypressed [SPECIAL_LEFTWINDOWS] = 0;
			break;
		case 0x5c:
			pccon_keypressed [SPECIAL_RIGHTWINDOWS] = 1;
			break;
		case 0xdc:
			pccon_keypressed [SPECIAL_RIGHTWINDOWS] = 0;
			break;
		case 0x5d:
			pccon_keypressed [SPECIAL_WINDOWSMENU] = 1;
			break;
		case 0xdd:
			pccon_keypressed [SPECIAL_WINDOWSMENU] = 0;
			break;
		case 0x48:		/*  Cursor UP  */
		case 0x50:		/*  Cursor DOWN  */
		case 0x4b:		/*  Cursor LEFT  */
		case 0x4d:		/*  Cursor RIGHT  */
		case 0x47:		/*  Cursor HOME  */
		case 0x4f:		/*  Cursor END  */
			data3='\0';
			switch (data2)
			  {
			    case 0x48:	data3='A'; break;	/*  Up    */
			    case 0x50:	data3='B'; break;	/*  Down  */
			    case 0x47:	data3='H'; break;	/*  Home  */
			    case 0x4f:	data3='F'; break;	/*  End   */
			    case 0x4b:				/*  Left  */
				if ( pccon_keypressed [SPECIAL_LEFTALT]  &&
					(pccon_keypressed [SPECIAL_LEFTCTRL] ||
					 pccon_keypressed [SPECIAL_RIGHTCTRL]) )
				  {
				    newvc = pccon_nextvc (pccon_current_vc, -1);
				    pccon_switch_vc (newvc);	/*  TODO: locks  */
				  }
				else
				  data3='D';
				break;
			    case 0x4d:				/*  Right  */
				if ( pccon_keypressed [SPECIAL_LEFTALT]  &&
					(pccon_keypressed [SPECIAL_LEFTCTRL] ||
					 pccon_keypressed [SPECIAL_RIGHTCTRL]) )
				  {
				    newvc = pccon_nextvc (pccon_current_vc, 1);
				    pccon_switch_vc (newvc);	/*  TODO: locks  */
				  }
				else
				  data3='C';
				break;
			  }
			if (data3)
			  {
			    terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, 27);
			    terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, '[');
			    terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, data3);
			  }
			break;
		case 0x52:
			terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, 27);
			terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, '[');
			terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, '2');
			terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, '~');
			break;
		case 0x53:
			/*  Should this be backspace or 127 (delete) ?  (TODO)  */
			terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, '\b');
			break;
		case 0xc8:	/*  release up  */
		case 0xd0:	/*  down  */
		case 0xcb:	/*  left  */
		case 0xcd:	/*  right  */
		case 0xc7:	/*  home  */
		case 0xcf:	/*  end  */
		case 0xd2:	/*  insert  */
		case 0xd3:	/*  delete  */
			break;
		default:
		    printk ("pccon: unknown scancode sequence: e0 %y", data2);
	      }

	    return;
	  }


	if (data == 0xe1)
	  {
	    /*  Wait for next byte.  */
	    do a = inb (0x64); while (! (a & 0x01));
	    data2 = inb (0x60);
	    do a = inb (0x64); while (! (a & 0x01));
	    data3 = inb (0x60);

	    printk ("pccon: unknown scancode sequence: e1 %y %y", data2, data3);

	    return;
	  }


	if (data == 0x38) pccon_keypressed [SPECIAL_LEFTALT] = 1;
	if (data == 0xb8) pccon_keypressed [SPECIAL_LEFTALT] = 0;

	if (data == 0x1d) pccon_keypressed [SPECIAL_LEFTCTRL] = 1;
	if (data == 0x9d) pccon_keypressed [SPECIAL_LEFTCTRL] = 0;

	if (data == 0x2a) pccon_keypressed [SPECIAL_LEFTSHIFT] = 1;
	if (data == 0xaa) pccon_keypressed [SPECIAL_LEFTSHIFT] = 0;

	if (data == 0x36) pccon_keypressed [SPECIAL_RIGHTSHIFT] = 1;
	if (data == 0xb6) pccon_keypressed [SPECIAL_RIGHTSHIFT] = 0;

	/*
	 *  {Num,Caps,Scroll}-lock:
	 *
	 *  Only switch lockmode if this scancode
	 *  arrived when the key was not pressed.
	 */

	if (data == 0x45)
	  {
	    if (!pccon_keypressed [SPECIAL_NUMLOCK])
		pccon_vc[pccon_current_vc]->lockmode ^= PCCON_NUMLOCK;

	    pccon_keypressed [SPECIAL_NUMLOCK] = 1;
	    pccon_update_leds ();
	  }
	if (data == 0xc5) pccon_keypressed [SPECIAL_NUMLOCK] = 0;

	if (data == 0x46)
	  {
	    if (!pccon_keypressed [SPECIAL_SCROLLLOCK])
		pccon_vc[pccon_current_vc]->lockmode ^= PCCON_SCROLLLOCK;

	    pccon_keypressed [SPECIAL_SCROLLLOCK] = 1;
	    pccon_update_leds ();
	  }
	if (data == 0xc6) pccon_keypressed [SPECIAL_SCROLLLOCK] = 0;

	if (data == 0x3a)
	  {
	    if (!pccon_keypressed [SPECIAL_CAPSLOCK])
		pccon_vc[pccon_current_vc]->lockmode ^= PCCON_CAPSLOCK;

	    pccon_keypressed [SPECIAL_CAPSLOCK] = 1;
	    pccon_update_leds ();
	  }
	if (data == 0xba) pccon_keypressed [SPECIAL_CAPSLOCK] = 0;


	/*  Switch virtual console?  */
	if ((data >= 0x3b && data <= 0x44) || data==0x57 || data==0x58)
	  {
	    if (data==0x57 || data==0x58)
		fkey = 11 + data - 0x57;
	    else
		fkey = 1 + data - 0x3b;

	    /*
	     *	To switch virtual console, we need the left Alt key
	     *	and at least one of the two Ctrl keys to be pressed.
	     *
	     *	Also, the new virtual console must exist, and be different
	     *	from the current virtual console.
	     */
	    if ( pccon_keypressed [SPECIAL_LEFTALT]  &&
		(pccon_keypressed [SPECIAL_LEFTCTRL] ||
		 pccon_keypressed [SPECIAL_RIGHTCTRL])  &&
		fkey <= MAX_NR_OF_VCS  &&  fkey-1 != pccon_current_vc
		&&  pccon_vc[fkey-1] != NULL )
	      {
		/*
		 *  Both the vc we are switching from and to must not be
		 *  locked.  TODO
		 */
/*
		lock (&pccon_vc[pccon_current_vc]->lock, "pccon_irqhandler", LOCK_BLOCKING | LOCK_RW);
		lock (&pccon_vc[fkey-1]->lock, "pccon_irqhandler", LOCK_BLOCKING | LOCK_RW);
*/
		pccon_switch_vc (fkey-1);
/*
		unlock (&pccon_vc[fkey-1]->lock);
		unlock (&pccon_vc[pccon_current_vc]->lock);
*/
	      }
	  }

#ifdef KDB

	/*  Drop into the kernel debugger on LEFTCTRL + LEFTALT + ESC:  */
	if (pccon_keypressed [SPECIAL_LEFTCTRL] &&
	    pccon_keypressed [SPECIAL_LEFTALT] && data==1)
	  {
	    kdb ();

	    /*  Assume that the keys have been released:  */
	    pccon_keypressed [SPECIAL_LEFTCTRL] = 0;
	    pccon_keypressed [SPECIAL_LEFTALT] = 0;
	    pccon_keypressed [1] = 0;		/*  escape  */
	    return;
	  }

#endif	/*  KDB  */


	if (data < 128 && pccon_keymap[data])
	  {
	    if (pccon_keypressed [SPECIAL_LEFTCTRL] ||
		pccon_keypressed [SPECIAL_RIGHTCTRL])
	      ctrled = 1;
	    else
	      ctrled = 0;

	    if (pccon_keypressed [SPECIAL_LEFTSHIFT] ||
		pccon_keypressed [SPECIAL_RIGHTSHIFT])
	      shifted = 1;
	    else
	      shifted = 0;

	    if ( (pccon_vc[pccon_current_vc]->lockmode & PCCON_CAPSLOCK)
		&& (data_is_alphakey(data)) )
	      shifted = 1;

	    /*  General case:  */
	    data2 = pccon_keymap[data];

	    if (!pccon_keypressed [SPECIAL_RIGHTALT] && !shifted)
		data2 = pccon_keymap[data];
	    else
	    if (!pccon_keypressed [SPECIAL_RIGHTALT] && shifted)
		data2 = pccon_keymap_shift[data];
	    else
	    if (pccon_keypressed [SPECIAL_RIGHTALT] && !shifted)
		data2 = pccon_keymap_altgr[data];
	    else
	    if (pccon_keypressed [SPECIAL_RIGHTALT] && !shifted)
		data2 = pccon_keymap_altgr[data];

	    if (ctrled)
		data2 = pccon_keymap[data] & 31;

	    if (pccon_keypressed [SPECIAL_LEFTALT])
		data2 ^= 0x80;

	    terminal_add_to_inputbuf (pccon_vc[pccon_current_vc]->ts, data2);
/*	    pccon_addchartokeybuf (data2);  */
	  }
      }
  }



void pccon__setsubmap (byte *s, byte *map, int startindex)
  {
    int j;

    j = 0;
    while (s[j])
	map[startindex+j] = s[j++];
  }



void pccon_keymapinit ()
  {
    /*  Connect scancodes with ASCII values:  */

    int i;
    char *s;

    for (i=0; i<MAX_KEYS; i++)
	pccon_keymap [i] = 0;

    pccon_keymap[1] = 27;

    s = "1234567890";	pccon__setsubmap (s, pccon_keymap, 2);
    s = "!\"#$%&/()=";	pccon__setsubmap (s, pccon_keymap_shift, 2);
    s = "¹@£$½¾{[]}";	pccon__setsubmap (s, pccon_keymap_altgr, 2);

    pccon_keymap		[0x0c] = '+';
    pccon_keymap_shift		[0x0c] = '?';
    pccon_keymap_altgr		[0x0c] = '\\';
    pccon_keymap_shift_altgr	[0x0c] = '¿';

    pccon_keymap		[0x0d] = '´';
    pccon_keymap_shift		[0x0d] = '`';

    pccon_keymap		[0x0e] = '\b';
    pccon_keymap_shift		[0x0e] = '\b';
    pccon_keymap_altgr		[0x0e] = 127;

    pccon_keymap		[0x0f] = '\t';
    pccon_keymap_shift		[0x0f] = '\t';

    s = "qwertyuiopå¨";	pccon__setsubmap (s, pccon_keymap, 0x10);
    s = "QWERTYUIOPÅ^";	pccon__setsubmap (s, pccon_keymap_shift, 0x10);
    s = "@ e¶ ¥  øþ°~";	pccon__setsubmap (s, pccon_keymap_altgr, 0x10);
    s = "  E® ¥  ØÞ°¯";	pccon__setsubmap (s, pccon_keymap_shift_altgr, 0x10);

    pccon_keymap		[0x1c] = '\r';		/*  TODO:  \n or \r ???  */
    pccon_keymap_shift		[0x1c] = '\r';
    pccon_keymap_altgr		[0x1c] = '\n';

    s = "asdfghjklöä";	pccon__setsubmap (s, pccon_keymap, 0x1e);
    s = "ASDFGHJKLÖÄ";	pccon__setsubmap (s, pccon_keymap_shift, 0x1e);
    s = "æßð        ";	pccon__setsubmap (s, pccon_keymap_altgr, 0x1e);
    s = "Æ§Ðª       ";	pccon__setsubmap (s, pccon_keymap_shift_altgr, 0x1e);

    pccon_keymap		[0x29] = '§';
    pccon_keymap_shift		[0x29] = '½';

    pccon_keymap		[0x2b] = '\'';
    pccon_keymap_shift		[0x2b] = '*';
    pccon_keymap_altgr		[0x2b] = '`';

    s = "zxcvbnm,.-";	pccon__setsubmap (s, pccon_keymap, 0x2c);
    s = "ZXCVBNM;:_";	pccon__setsubmap (s, pccon_keymap_shift, 0x2c);
    s = "«»¢   µ · ";	pccon__setsubmap (s, pccon_keymap_altgr, 0x2c);
    s = "<>©`' º×÷ ";	pccon__setsubmap (s, pccon_keymap_shift_altgr, 0x2c);

    pccon_keymap		[0x37] = '*';
    pccon_keymap_shift		[0x37] = '*';

    pccon_keymap		[0x39] = ' ';
    pccon_keymap_altgr		[0x39] = ' ';
    pccon_keymap_shift		[0x39] = ' ';
    pccon_keymap_shift_altgr	[0x39] = ' ';

    pccon_keymap		[0x56] = '<';
    pccon_keymap_altgr		[0x56] = '|';
    pccon_keymap_shift		[0x56] = '>';
    pccon_keymap_shift_altgr	[0x56] = '¦';
  }

