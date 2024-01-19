/*
 *  Copyright (C) 1999 by Anders Gavare.  All rights reserved.
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
 *  panic.c  --  print error message and halt
 *
 *  TODO:
 *	Wait for ENTER key and reboot (?) (better than just halt)
 *
 *  History:
 *	18 Oct 1999	first version
 */


#include "../config.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/console.h>
#include <sys/std.h>

#ifdef KDB
#include <sys/md/machdep.h>
#endif


#define	PANIC_BUFSIZE	200		/*  panic's buffer is small  */


void panic (char *fmt, ...)
  {
    char buf [PANIC_BUFSIZE];
    char buf2 [PANIC_BUFSIZE];
    va_list pvar;    

    va_start (pvar, fmt);
    vsnprintf (buf, PANIC_BUFSIZE, fmt, pvar);
    va_end (pvar);

    snprintf (buf2, PANIC_BUFSIZE, "panic: %s", buf);

    /*  No reason to preserve color:  */
    console_puts (buf2, CONSOLE_PANIC_COLOR);

#ifdef KDB
#ifdef KDB_ON_PANIC
    /*  Machine dependant kernel debugger:  */
    kdb ();
#endif
#endif

    machdep_halt ();
  }

