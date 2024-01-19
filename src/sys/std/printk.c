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
 *  printk.c  --  kernel printf()
 *
 *  TODO:
 *	Inline, ie not call snprintf() ?
 *	Place output in a dmesg buffer!!!
 *
 *  History:
 *	18 Oct 1999	first version
 *	13 Jan 2000	disables printk if DEBUGLEVEL<0
 *	4 Feb 2000	disabling interrupts
 */


#include <stdarg.h>
#include <stdio.h>
#include <sys/std.h>
#include <sys/console.h>
#include <sys/interrupts.h>
#include "../config.h"


extern int	console_cursor_color;

int	printk_enabled = 1;

#define	PRINTK_BUFSIZE	2000


int printk (char *fmt, ...)
  {
    va_list argp;
    char buf[PRINTK_BUFSIZE];
    int res;

    va_start (argp, fmt);
    res = vsnprintf (buf, PRINTK_BUFSIZE, fmt, argp);
    va_end (argp);

    if (printk_enabled)
	console_puts (buf, CONSOLE_KERNEL_COLOR);

#if DEBUGLEVEL<0
    /*  Disable printk if DEBUGLEVEL<0:  */
    printk_enabled = 0;
#endif

    return res;
  }

