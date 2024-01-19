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


#include "../../../../config.h"
#include <stdio.h>
#include <string.h>
#include <sys/std.h>
#include <sys/module.h>
#include <sys/interrupts.h>
#include <sys/arch/i386/pio.h>
#include <sys/terminal.h>
#include <sys/lock.h>


/*
 *  Maximum nr of virtual consoles:   (Note: it's actually possible to have
 *  this value set higher than 12, but then you'll need to use CTRL+ALT+
 *  LEFT/RIGHT to switch to that vc since there isn't more than 12 F-keys.)
 */

#define	MAX_NR_OF_VCS		12


#define	SPECIAL_LEFTSHIFT	128
#define	SPECIAL_RIGHTSHIFT	129
#define	SPECIAL_LEFTCTRL	130
#define	SPECIAL_RIGHTCTRL	131
#define	SPECIAL_LEFTALT		132
#define	SPECIAL_RIGHTALT	133
#define	SPECIAL_LEFTWINDOWS	134
#define	SPECIAL_RIGHTWINDOWS	135
#define	SPECIAL_WINDOWSMENU	136
#define	SPECIAL_NUMLOCK		137
#define	SPECIAL_CAPSLOCK	138
#define	SPECIAL_SCROLLLOCK	139

#define	MAX_KEYS		140

#define	KEYBOARD_BUFFER_LEN	512


/*
 *  A textmode character cell consists of character
 *  code and color code:
 *
 *	7 6 5 4 3 2 1 0
 *	| |---| | |---|
 *	A   B   C   D
 *
 *	A:  foreground blink
 *	B:  background (R G B from left to right)
 *	C:  foreground intensity
 *	D:  foreground (R G B from left to right)
 */

struct charcell
      {
	byte	ch;
	byte	color;
      };


/*
 *  pccon_vc:  Definition of a Virtual Console
 */

struct pccon_vc
      {
	/*  Screen size  */
	int	nrows;
	int	ncols;

	/*  Character cell data  */
	struct charcell *cells;
	int	current_color;

	/*  Cursor position and type  */
	int	cursor_row;
	int	cursor_column;
	int	cursor_type;

	/*  Lock-modes / LED mode  */
	int	lockmode;

	/*  Video mode  */
	int	videomode;

	/*  Output (write) lock:  */
	struct lockstruct lock;

	/*  Termstate:  */
	struct termstate *ts;

	/*  EGA/VGA font data  */
		/*  TODO  */
      };


#define	PCCON_NUMLOCK		1
#define	PCCON_CAPSLOCK		2
#define	PCCON_SCROLLLOCK	4

#define	PCCON_CURSOR_NONE	0
#define	PCCON_CURSOR_LINE	1
#define	PCCON_CURSOR_BLOCK	2



struct winsize
      {
	u_int16_t	ws_row;		/*  nr of rows  */
	u_int16_t	ws_col;		/*  nr of columns  */
	u_int16_t	ws_xpixel;	/*  nr of pixels (x)  */
	u_int16_t	ws_ypixel;	/*  nr of pixels (y)  */
      };



/*
 *  pccon_  functions
 */

void pccon_detect_color ();

int pccon_termread ();
int pccon_termwrite ();

int pccon_initkeybuf ();
void pccon_update_leds ();
void pccon_addchartokeybuf (char c);
int pccon_readcharfromkeybuf ();	/*  -1 when no key available  */
void pccon_irqhandler ();
void pccon_keymapinit ();

void pccon_gethwcursorpos (int *row, int *col);
void pccon_sethwcursorpos (int row, int col);
int pccon_nextvc (int oldvc, int direction);
void pccon_switch_vc (int newvc);
void pccon_clearscreen (int vc_nr);
void pccon_checkscroll (int vc_nr);
void pccon_printchar (int vc_nr, char ch);
void pccon_slowputs (char *s, int color);

