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
 *  sys/console.h  --  low-level kernel console functions
 *
 *	Functions and definitions used by the "simple" console driver.
 *	Each port (i386, ...) must at least have a simple console driver.
 *
 *	NOTE:  Some of this should be architecture dependant...
 */

#ifndef	__SYS__CONSOLE_H
#define	__SYS__CONSOLE_H

#include <sys/defs.h>


#define	CONSOLE_KERNEL_COLOR	10		/*  light green on PC  */
#define	CONSOLE_MODULE_COLOR	9		/*  light blue  */
#define	CONSOLE_PANIC_COLOR	12		/*  light red  */
#define	CONSOLE_NORMAL_COLOR	7		/*  white  */


void console_sethardwarecursorpos ();

int console_gethardwarecursorpos ();

void console_init ();

void console_reset ();

void console_putchar (byte c);

void (*console_puts) (char *s, int color);

void console_scrollup (int lines);


#endif	/*  __SYS__CONSOLE_H  */

