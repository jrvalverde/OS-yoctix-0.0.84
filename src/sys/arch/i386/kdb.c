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
 *  arch/i386/kdb.c  --  i386 MD code for the built-in kernel debugger
 *
 *	See kern/kdb.c for more information about what the kdb_* functions
 *	are supposed to do.
 *
 *
 *  History:
 *	8 Dec 2000	working (print and getch). One cmd so far: dumpcmos
 */


#include "../config.h"
#include <sys/arch/i386/machdep.h>
#include <sys/arch/i386/pio.h>
#include <sys/std.h>
#include <string.h>
#include <sys/console.h>



extern int console_cursor_color;
extern int console_cursor_xpos;



struct kdb_command kdb_machdep_cmds [] =
      {
	{  "dumpcmos",	"Dump cmos data",	kdb_machdep_dumpcmos  },
	{  NULL,	NULL,			NULL  }
      };



void kdb_initconsole ()
  {
    console_reset ();
    if (console_cursor_xpos!=0)
      kdb_print ("\n");
  }



void kdb_machdep_dumpcmos (char *cmdline)
  {
    char cmos [128];
    int i;

    for (i=0; i<128; i++)
      cmos[i] = cmos_read (i);

    for (i=0; i<128; i+=16)
      kdb_dumpmem (i, 16, cmos+i);
  }



void kdb_print (char *s)
  {
    if (!s)
      s = "(null)";

    console_cursor_color = CONSOLE_PANIC_COLOR;

    while (*s)
	console_putchar (*(s++));
  }



int kdb_getch ()
  {
    /*
     *	This is extremely ugly:  poll the keyboard port (0x60) until we
     *	get a char.  Only care about letters and digits and a few other
     *	neccessary characters.  ENTER and Backspace need to work too.
     */

    int k;
    static int oldk = 0;

    /*  Wait for new valid key:  */
    while (1)
      {
	/*  Wait for last key if any to be released:  */
	while ((k = inb(0x60)) < 128 && k==oldk)  ;

	while ((k = inb(0x60)) >= 128)  ;

	oldk = k;

	if (k==28)
	  return '\n';
	if (k==14 || k==83)
	  return '\b';
	if (k==57)
	  return ' ';

	if (k>=2 && k<=12)
	  return "1234567890+"[k-2];
	if (k>=16 && k<=25)
	  return "qwertyuiop"[k-16];
	if (k>=30 && k<=38)
	  return "asdfghjkl"[k-30];
	if (k>=44 && k<=50)
	  return "zxcvbnm"[k-44];

#if DEBUGLEVEL>=4
	{
	  char buf [15];
	  snprintf (buf, 15, "[0x%y]\b\b\b\b\b\b", k);
	  kdb_print (buf);
	}
#endif
      }
  }


