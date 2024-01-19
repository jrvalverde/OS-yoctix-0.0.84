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
 *  sys/terminal.h
 */


#ifndef	__SYS__TERMINAL_H
#define	__SYS__TERMINAL_H

#include <termios.h>
#include <sys/defs.h>


#define	MAX_INPUT		1024
#define	MAX_OUTPUT		512
#define	OUTPUT_MARGIN		4

struct termstate
      {
	/*  A termios struct stores control flags etc.:  */
	struct termios	termio;

	/*  Process group ID for this tty  */
	pid_t	pgrp;

	/*  inputhead is the index of the newest added
	    character in the inputbuf.  */
	char		inputbuf [MAX_INPUT];
	size_t		inputhead, inputtail;

	/*  Linemode "sleep": (a canonical read should sleep
	    on this address... wakeup if NL, EOL, or EOF is typed)
	    TODO: since this is also used for non-canonical sleep,
		it should be renamed!  */
	int		linemode_sleep;

	/*  outputlen is the number of chars in outputbuf
	    containing actual output characters.  */
	char		outputbuf [MAX_OUTPUT];
	size_t		outputlen;

	/*  Stopped output:  (xon/xoff stuff)  */
	int		stopped;

	/*  The terminal (device) read/write functions.
	    These are used for example for local echo.
	    The id is used as a "help" for the read/write function
	    to see the difference between for example virtual
	    consoles or multiple devices.  */
	int		(*read)();
	int		(*write)();		/*  int id, char *buf, size_t len  */
	int		id;
      };


struct termstate *terminal_open ();
int terminal_close (struct termstate *ts);
size_t terminal_write (struct termstate *ts, char *buf, size_t len);
int terminal_add_to_inputbuf (struct termstate *ts, char ch);
size_t terminal_read (struct termstate *ts, char *buf, size_t maxlen);


#endif	/*  __SYS__TERMINAL_H  */
