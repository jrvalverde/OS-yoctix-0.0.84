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
 *  modules/bus/isa/pccon/pccon_video.c
 *
 *  History:
 *	14 Jan 2000	first version
 *	13 Jun 2000	fixed TAB -> space...
 */



#include "../../../../config.h"
#include <stdio.h>
#include <string.h>
#include <sys/std.h>
#include <sys/console.h>
#include <sys/module.h>
#include <sys/ports.h>
#include <sys/interrupts.h>
#include <sys/arch/i386/pio.h>
#include <sys/modules/bus/isa/isa.h>
#include <sys/lock.h>

#include "pccon.h"



extern struct module *pccon_m;

struct pccon_vc *pccon_vc [MAX_NR_OF_VCS];
int pccon_current_vc;

byte pccon_charconvert [256];

/*  Locks, used by lock() and unlock()  */
struct lockstruct pccon_lock;
struct lockstruct pccon_crtclock;


int		pccon_crtcbase = 0x3d0;
size_t		pccon_videoaddr = 0xb8000;



void pccon_detect_color ()
  {
    /*
     *	Check for color vs b/w video adapter:
     *
     *	This is probably not very stable, but will do for now. According to
     *	HELPPC, the byte at physical address 0x410 contains "Equipment list
     *	flags", where bits 5 and 4 tell us the "initial video mode". If this
     *	value is 11 (binary) then we have b/w, otherwise assume color.
     */

    int *p, res, tot;

    p = (int *) 0x410;
    if ((p[0] & 0x30) == 0x30)
      {
	pccon_crtcbase = 0x3b0;
	pccon_videoaddr = 0xb0000;
	tot = 12;
      }
    else
      {
	pccon_crtcbase = 0x3d0;
	pccon_videoaddr = 0xb8000;
	tot = 16;
      }

    res = ports_register (pccon_crtcbase, tot, pccon_m->shortname);
    if (!res)
	printk ("%s: warning: could not register ports %Y-%Y",
		pccon_m->shortname, pccon_crtcbase, pccon_crtcbase+tot-1);
  }



void pccon_gethwcursorpos (int *row, int *col)
  {
    /*
     *	Return the current (hardware) row and column of the cursor.
     */

    int p;

    outb (pccon_crtcbase + 4, 0xe);
    p = inb (pccon_crtcbase + 5);
    outb (pccon_crtcbase + 4, 0xf);
    p = p*256 + inb (pccon_crtcbase + 5);

    *row = p / pccon_vc[pccon_current_vc]->ncols;
    *col = p % pccon_vc[pccon_current_vc]->ncols;
  }



void pccon_sethwcursorpos (int row, int col)
  {
    /*
     *	Set the current (hardware) row and column of the cursor.
     */

    int p;

    p = row * pccon_vc[pccon_current_vc]->ncols + col;
    outb (pccon_crtcbase + 4, 0xe);
    outb (pccon_crtcbase + 5, p / 256);
    outb (pccon_crtcbase + 4, 0xf);
    outb (pccon_crtcbase + 5, p & 255);
  }



int pccon_nextvc (int oldvc, int direction)
  {
    /*
     *	oldvc = old vc number, direction = 1 or -1.
     *	This functions will find the next vc in use (going left (-1)
     *	or right (1) from the old vc).
     */

    int tst = oldvc + direction;

    while (tst != oldvc)
      {
	if (tst<0)
	  tst = MAX_NR_OF_VCS-1;
	else
	if (tst>=MAX_NR_OF_VCS)
	  tst = 0;

	if (pccon_vc[tst])
	  return tst;

	tst += direction;
      }

    return oldvc;
  }



void pccon_switch_vc (int newvc)
  {
    int videolen;
    struct pccon_vc *p;
    int oldints;


/*  TODO: check if the lock is on... hm...  */


    oldints = interrupts (DISABLE);

    /*  Simple check:  (we don't need to switch)  */
    if (newvc == pccon_current_vc)
      {
	interrupts (oldints);
        return;
      }

    p = pccon_vc [pccon_current_vc];

    /*  Copy video memory to the "cells" of the old vc:  */
    videolen = sizeof(struct charcell) * p->nrows * p->ncols;
    memcpy (p->cells, (void *)pccon_videoaddr, videolen);

    /*  Change video mode to what the new vc uses:  */
    p = pccon_vc [newvc];

/*  TODO  */

    /*  Copy new vc's cells to video memory:  */
    videolen = sizeof(struct charcell) * p->nrows * p->ncols;
    memcpy ((void *)pccon_videoaddr, p->cells, videolen);

    pccon_current_vc = newvc;

    pccon_sethwcursorpos (p->cursor_row, p->cursor_column);

    /*  Update keyboard leds:  */
    pccon_update_leds ();

    interrupts (oldints);
  }



void pccon_clearscreen (int vc_nr)
  {
    /*
     *	Clear the character cells (using the _current_ cursor color),
     *	and "home" the cursor.
     *
     *	TODO:  locking?
     */

    int i;
    struct pccon_vc *p = pccon_vc [vc_nr];
    struct charcell *cc;

    if (vc_nr != pccon_current_vc)
      cc = p->cells;
    else
      cc = (struct charcell *) pccon_videoaddr;

    for (i = p->ncols*p->nrows - 1; i>=0; i--)
      {
	cc[i].ch = ' ';
	cc[i].color = p->current_color;
      }

    p->cursor_row = 0;
    p->cursor_column = 0;

    if (vc_nr == pccon_current_vc)
	pccon_sethwcursorpos (0, 0);
  }



void pccon_checkscroll (vc_nr)
  {
    /*
     *	Check if the cursor is beyond the end of the last line
     *	and in that case scroll up.
     *
     *	TODO:  locking instead of disabled interrupts?
     */

    struct pccon_vc *p = pccon_vc [vc_nr];
    struct charcell *cc;
    int oldints;
    int i, base;

    oldints = interrupts (DISABLE);

    if (p->cursor_row < p->nrows)
      {
	interrupts (oldints);
	return;
      }

    p->cursor_row = p->nrows - 1;

    if (vc_nr != pccon_current_vc)
      cc = p->cells;
    else
      cc = (struct charcell *) pccon_videoaddr;

    memcpy ((void *)cc, (void *)cc + sizeof(struct charcell)*p->ncols,
	sizeof(struct charcell)*p->ncols*(p->nrows-1));

    /*  base = base offset to the last line  */
    base = p->ncols*(p->nrows-1);

    for (i=0; i<p->ncols; i++)
      {
	cc[i+base].ch = ' ';
	cc[i+base].color = p->current_color;
      }

    interrupts (oldints);
  }



void pccon_printchar (int vc_nr, char ch)
  {
    /*
     *	Print a character on the virtual console
     *	----------------------------------------
     *
     *	Some characters are special:
     *
     *		\0	Nothing
     *		\b	Backspace (non-destructive)
     *		\f	Form feed (clear screen)
     *		\r	Carriage return
     *		\n	Newline
     *		\t	Tab (nearest 8-space)
     *		\e	Escape codes
     *
     *	Escape codes are treated as ANSI escape codes.
     *
     *	All other characters are printed after being converted
     *	in the charconvert filter.
     */

    int i;
    struct pccon_vc *p = pccon_vc [vc_nr];
    struct charcell *cc;

    if (ch == '\0')
	return;

    if (ch == '\a')
      {
	/*  Beep.  (TODO)  */
	return;
      }

    if (ch == '\b')
      {
	if (pccon_vc[vc_nr]->cursor_column > 0)
		pccon_vc[vc_nr]->cursor_column --;
	if (vc_nr == pccon_current_vc)
		pccon_sethwcursorpos (pccon_vc[vc_nr]->cursor_row,
			pccon_vc[vc_nr]->cursor_column);
	return;
      }

    if (ch == '\f')
      {
	pccon_clearscreen (vc_nr);
	return;
      }

    if (ch == '\r')
      {
	pccon_vc[vc_nr]->cursor_column = 0;
	if (vc_nr == pccon_current_vc)
		pccon_sethwcursorpos (pccon_vc[vc_nr]->cursor_row,
			pccon_vc[vc_nr]->cursor_column);
	return;
      }

    if (ch == '\n')
      {
	pccon_vc[vc_nr]->cursor_row ++;
	pccon_checkscroll (vc_nr);
	if (vc_nr == pccon_current_vc)
		pccon_sethwcursorpos (pccon_vc[vc_nr]->cursor_row,
			pccon_vc[vc_nr]->cursor_column);
	return;
      }

    if (ch == '\t')
      {
	i = 8 - (pccon_vc[vc_nr]->cursor_column % 8);

	while (i-- > 0)
	  pccon_printchar (vc_nr, ' ');

	return;
      }

/*  TODO: escape codes  */


    ch = pccon_charconvert [(unsigned char)ch];

    if (vc_nr != pccon_current_vc)
      cc = p->cells;
    else
      cc = (struct charcell *) pccon_videoaddr;

    if (p->cursor_column >= p->ncols)
      {
	p->cursor_column = 0;
	p->cursor_row ++;
	pccon_checkscroll (vc_nr);
      }

    /*  Now we actually output the char:  */
    i = p->ncols * p->cursor_row + p->cursor_column;
    cc[i].ch = ch;
    cc[i].color = p->current_color;

    p->cursor_column ++;

    if (vc_nr == pccon_current_vc)
      {
	if (p->cursor_column == p->ncols)
	  pccon_sethwcursorpos (p->cursor_row, p->cursor_column-1);
	else
	  pccon_sethwcursorpos (p->cursor_row, p->cursor_column);
      }
  }



void pccon_slowputs (char *s, int color)
  {
    /*
     *	Slow puts() used for console output of kernel messages
     *
     *	Note: NOT for normal userlevel output...
     *	Switches to virtual console 0 if color=CONSOLE_PANIC_COLOR.
     */

    int backupcolor = pccon_vc[0]->current_color;
    int i=0;
    char ch;
    int oldints;

    oldints = interrupts (DISABLE);

    pccon_vc[0]->current_color = color;

    while ((ch = s[i++]))
	pccon_printchar (0, ch);

    pccon_printchar (0, '\r');
    pccon_printchar (0, '\n');
    pccon_vc[0]->current_color = backupcolor;

    interrupts (oldints);


    /*
     *	If the system panics (or if the panic color is used at all)
     *	then we switch to virtual console 0 so the user sees the panic
     *	message.
     */

    if (color == CONSOLE_PANIC_COLOR)
	pccon_switch_vc (0);
  }


