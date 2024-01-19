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
 *  console.c  --  simple mac68k console driver
 *
 *  History:
 *	9 Dec 2000	nothing yet
 */


#include <sys/defs.h>
#include <sys/console.h>


/*
 *  console_puts should point to the best available puts() function.
 *  During bootup, before any modules or memory management stuff has
 *  been initialized, we use console_slowputs(). Later, any other
 *  driver (for example pccon) can set console_puts to point to a
 *  better variant of puts().
 */

void console_slowputs();
void	(*console_puts)() = &console_slowputs;


/*
 *  These variables are supposed to be "relatively" private.  That is, at
 *  the moment the only external routine that is supposed to access any
 *  of these immediately is kdb_print() in arch/mac68k/kdb.c.
 */

int	console_cursor_xpos = 0;
int	console_cursor_ypos = 0;
int	console_cursor_color = 7;		/*  raw color  */
byte	*console_screenbuffer = (byte *) 0;	/*  hercules = 0xb0000, color = 0xb8000 (?)  */
int	console_screenbufferlen = 0;		/*  set by console_reset() !!!  */
int	console_current_offset = 0;		/*  for quicker updates  */
int	console_nrofcolumns = 0;
int	console_nroflines = 0;
int	console_crtcbase = 0x3d0;		/*  0x3d0 for color, 0x3b0 for monochrome  */



void console_reset ()
  {
    /*
     *	console_reset ()
     *	----------------
     *
     *	In case we are unsure about the state of the console (or some of the
     *	console_* variables), this function will reset things to a sane
     *	state.
     *
     *	Called from console_init() and kdb_initconsole().
     */

    int p = console_gethardwarecursorpos();
    console_cursor_xpos = p % console_nrofcolumns;
    console_cursor_ypos = p / console_nrofcolumns;

    if (console_cursor_xpos<0 || console_cursor_ypos<0 ||
	console_cursor_xpos>=console_nrofcolumns || console_cursor_ypos>=console_nroflines)
      {
	console_cursor_xpos = 0;
	console_cursor_ypos = 0;
      }

    console_cursor_color = 7;
    console_screenbufferlen = console_nrofcolumns * console_nroflines * 2;
    console_sethardwarecursorpos ();
  }



void console_init ()
  {
    /*
     *	console_init ()
     *	---------------
     *
     *	This function is called from init_main() before any other console
     *	functions are used. This simple version of the console driver
     *	doesn't really need any initialization except for detection of
     *	the screenbuffer address and crtcbase port (which depend on whether
     *	we're using a color or monochrome graphics card).
     *
     *	See the modules/bus/isa/pccon driver for more details.
     */

    int p = 0x9f9a80, f=9;
    int *q;

    while (1)
{
q = (int *) (p + f % 21888);
f++;
*q = 0x12345678;
}
    
  }



void console_sethardwarecursorpos ()
  {
    /*
     *	console_sethardwarecursorpos ()
     *	-------------------------------
     */

  }



int console_gethardwarecursorpos ()
  {
    /*
     *	console_gethardwarecursorpos ()
     *	-------------------------------
     *
     *	Returns "x+console_nrofcolumns*y"
     */

    int p = console_current_offset / 2;

    return p;
  }



void console_putchar (byte c)
  {
    /*
     *	console_putchar ()
     *	------------------
     *
     *	Puts the character 'c' on the screen and moves the cursor.
     */

    /*  Treat a few char specially:  */
    if (c=='\n')
      {
	console_cursor_xpos = 0;
	console_cursor_ypos ++;
      }
    else
    if (c=='\r')
      {
	console_cursor_xpos = 0;
	console_cursor_ypos ++;
      }
    else
    if (c=='\b')
      {
	if (console_cursor_xpos>0)
		console_cursor_xpos--;
      }
    else
      {
	/*  Any other character:  */
	console_screenbuffer[console_current_offset++] = c;
	console_screenbuffer[console_current_offset++] = console_cursor_color;

	console_cursor_xpos = (console_cursor_xpos + 1) % console_nrofcolumns;
	if (console_cursor_xpos == 0)
	  {
	    console_cursor_ypos ++;
	  }
      }

    if (console_cursor_ypos >= console_nroflines)
      {
	console_cursor_ypos = console_nroflines - 1;
	console_cursor_xpos = 0;
	console_scrollup (1);
      }

    console_sethardwarecursorpos ();
  }



void console_slowputs (char *s, int color)
  {
    /*
     *	console_slowputs ()
     *	-------------------
     *
     *	Output kernel debug messages in a specific color. (NOTE: This
     *	function is only used until a better console driver has been
     *	initialized. See modules/bus/isa/pccon for more info.)
     */

    int preserve_color = console_cursor_color;
    int i = 0;

    console_cursor_color = color;

    while (s[i])
      console_putchar (s[i++]);
    console_putchar ('\n');

    console_cursor_color = preserve_color;
  }



void console_scrollup (int lines)
  {
    /*
     *	console_scrollup ()
     *	-------------------
     */

    int len_to_copy;
    int copy_from, copy_to;
    u_int16_t *p;
    int i;


    /*  Scroll up more than the number of lines on the screen?
	Then blank the screen!  */
    if (lines >= console_nroflines)
      {
	return;
      }

    /*  Move lines to the top of the screen:  */
    len_to_copy = (console_nroflines-lines)*2*console_nrofcolumns;
    copy_from = 2*console_nrofcolumns*lines;
    copy_to = 0;

    /*  Fill the lowest lines with spaces:  */
    p = (u_int16_t *) (console_screenbuffer + console_screenbufferlen - 2*lines*console_nrofcolumns);
    for (i=0; i<lines*console_nrofcolumns; i++)
	p[i] = 0x720;
  }

