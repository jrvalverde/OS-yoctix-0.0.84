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
 *  arch/mac68k/kdb.c  --  mac68k MD code for the built-in kernel debugger
 *
 *	See kern/kdb.c for more information about what the kdb_* functions
 *	are supposed to do.
 *
 *
 *  History:
 *	9 Dec 2000	nothing
 */


#include "../config.h"
#include <sys/kdb.h>
#include <sys/std.h>
#include <string.h>
#include <sys/console.h>



extern int console_cursor_color;
extern int console_cursor_xpos;



struct kdb_command kdb_machdep_cmds [] =
      {
/*	{  "dumpcmos",	"Dump cmos data",	kdb_machdep_dumpcmos  },  */
	{  NULL,	NULL,			NULL  }
      };



void kdb_initconsole ()
  {
    console_reset ();
    if (console_cursor_xpos!=0)
      kdb_print ("\n");
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

while (1);

    return 0;

  }


