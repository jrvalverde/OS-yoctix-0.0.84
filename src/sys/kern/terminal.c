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
 *  kern/terminal.c  --  general terminal driver
 *
 *
 *  The general terminal driver (termios)
 *  -------------------------------------
 *
 *  The purpose of the general terminal driver is to provide input/output
 *  conversion required by physical terminal devices (for example NL
 *  to CR+NL conversion), and an input line featuring very simple editing
 *  functions (erase last character, kill whole line).
 *
 *  A specific terminal driver's read() and write() routines should call
 *  the generic routines. The state of a terminal (including for example
 *  the current input line) is fully described by the termstate structure.
 *
 *  When a terminal is opened, the general terminal_open() should first be
 *  called, and then specific settings can be applied.
 */


#include "../config.h"
#include <sys/terminal.h>
#include <sys/std.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/interrupts.h>


struct termstate *terminal_open ()
  {
    /*
     *	terminal_open ()
     *	----------------
     *
     *	Allocate memory for a termstate struct and return a pointer
     *	to it. Return NULL if unsuccessful.
     *
     *	Some default termios values are also filled in.
     */

    struct termstate *ts;

    ts = (struct termstate *) malloc (sizeof(struct termstate));
    if (!ts)
      return NULL;

    memset (ts, 0, sizeof(struct termstate));

    /*  Set default termios values:  */
    ts->termio.c_cc[VEOF] = 0;	/*  TODO  */
    ts->termio.c_cc[VEOL] = 0;	/*  TODO  */
    ts->termio.c_cc[VERASE] = 8;	/*  Backspace  */
    ts->termio.c_cc[VINTR] = 3;		/*  CTRL-C (?)  */
    ts->termio.c_cc[VKILL] = 11;	/*  CTRL-K (?)  */
    ts->termio.c_cc[VQUIT] = 0;		/*  ?  */
    ts->termio.c_cc[VSUSP] = 26;	/*  CTRL-Z (?)  */
    ts->termio.c_cc[VSTART] = 17;	/*  CTRL-Q  */
    ts->termio.c_cc[VSTOP] = 19;	/*  CTRL-S  */
    ts->termio.c_cc[VMIN] = 0;
    ts->termio.c_cc[VTIME] = 0;

    return ts;
  }



int terminal_close (struct termstate *ts)
  {
    /*
     *	terminal_close ()
     *	-----------------
     *
     *	Free the termstate struct from memory.
     */

    if (!ts)
	return EINVAL;

    free (ts);

    return 0;
  }



size_t terminal_write (struct termstate *ts, char *buf, size_t len)
  {
    /*
     *	terminal_write ()
     *	-----------------
     *
     *	Try to convert len bytes from buf to ts->outputbuf. Return the
     *	actual number of converted bytes (which may be smaller than or
     *	equal to len). The reason for this is that if a specific terminal
     *	driver wishes to convert a very large buffer for output, then
     *	it needs to call terminal_write() several times. The value we
     *	return from this function tells the caller how many bytes to
     *	skip in the original buffer when doing the next call to this
     *	function.
     *
     *	After a successful call to this function, ts->outputbuf is filled
     *	with valid characters and ts->outputlen is set appropriately.
     *	If ts->write is not NULL, that function is called with ts->outputbuf
     *	as the buffer to write.
     *
     *	The function returns 0 on failure, or if nothing was written.
     *
     *	Since some input characters may be converted to more than one
     *	output character (for example NL -> CR+NL) we have to have a small
     *	"margin" at the end of the outputbuf. This margin is called
     *	OUTPUT_MARGIN.
     */

    tcflag_t oflag;
    size_t chars_read = 0;
    char ch;
    int res;


    if (!ts || !buf)
	return 0;

    oflag = ts->termio.c_oflag;
    ts->outputlen = 0;

    while (len > 0)
      {
	ch = buf[0];
	chars_read++;

	if (oflag & OPOST)
	  {
	    /*  Perform output processing:  */

	    /*  OLCUC = convert lower- to upper-case:  */
	    if ((oflag & OLCUC) && ch >= 'a' && ch <= 'z')
		ch -= 32;

	    /*  ONLCR = transmit NL as CR+NL:  */
	    if ((oflag & ONLCR) && ch == '\n')
	      {
		/*  TODO:  is this correct behaviour??  */
		/*  OCRNL = transmit CR as NL:  */
		if (oflag & OCRNL)
		  ts->outputbuf[ts->outputlen++] = '\n';
		else
		  ts->outputbuf[ts->outputlen++] = '\r';
	      }

	    /*  OCRNL = transmit CR as NL:  */
	    if ((oflag & OCRNL) && ch == '\r')
		ch = '\n';

	    ts->outputbuf[ts->outputlen++] = ch;
	  }
	else
	  {
	    /*  Don't perform output processing, just copy
		input chars to the output buffer:  */
	    ts->outputbuf[ts->outputlen++] = ch;
	  }

	/*  Have we "almost" filled the outputbuf? Then return.  */
	if (ts->outputlen >= MAX_OUTPUT-OUTPUT_MARGIN)
	  return chars_read;

	/*  Go to the next char in the input buf:  */
	len--;
	buf++;
      }

    if (ts->write)
      {
	res = ts->write (ts->id, ts->outputbuf, (size_t) ts->outputlen);
	if (res)
	  return 0;
      }

    return chars_read;
  }



int terminal_add_to_inputbuf (struct termstate *ts, char ch)
  {
    /*
     *	terminal_add_to_inputbuf ()
     *	---------------------------
     *
     *	This function tries to add ch to ts->inputbuf. If no more
     *	space is available (if inputhead is just before inputtail)
     *	then we simply return -1.  (This is okay according to the
     *	Single Unix Specification's documentation about the General
     *	Terminal Interface.)
     *
     *	If local ECHO is on, then we write the character using
     *	terminal_write().
     *
     *	If we are running in canonical mode, we also handle ERASE
     *	and KILL characters.
     *
     *	TODO: wakeup a process sleeping on input from the terminal
     *	whenever an EOF, EOL, or newline arrives (??)
     */

    tcflag_t iflag;
    size_t lastindex, next;
    int to_remove;
    size_t res;
    char minibuf[10];


    if (!ts)
	return -1;

    iflag = ts->termio.c_iflag;

    if (iflag & ISTRIP)
	ch &= 0x7f;

    if (iflag & IGNCR)
	return 0;

    if ((iflag & INLCR) && ch == '\n')
	ch = '\r';
    else
    if ((iflag & ICRNL) && ch == '\r')
	ch = '\n';

    if ((iflag & IUCLC) && ch >= 'A' && ch >= 'Z')
	ch += 32;


    if (ts->termio.c_lflag & ICANON)
      {
	/*
         *  Canonical mode input processing:
	 */

	/*  Backspace/delete/erase, or kill line?  */
	if (ch == ts->termio.c_cc[VERASE] ||
		ch == ts->termio.c_cc[VKILL])
	  {
	    /*  Anything there to remove?  */
	    /*  TODO:  implement "beep" if we are unable
		to remove any char...  */
	    if (ts->inputhead == ts->inputtail)
		return -1;

	    lastindex = ts->inputhead - 1;
	    if (lastindex < 0)
	      lastindex = MAX_INPUT - 1;

	    if (ts->inputbuf[lastindex] == ts->termio.c_cc[VEOF] ||
		ts->inputbuf[lastindex] == ts->termio.c_cc[VEOL] ||
		ts->inputbuf[lastindex] == '\n')
	      {
		/*  TODO:  should we check ECHOK here too? probably...  */
	        return -1;
	      }

	    /*  Remove one char, or as many as possible, depending
		on the value of ch:  */
	    to_remove = 1;

	    while (to_remove > 0)
	      {
		/*  Remove one char:  */
		ts->inputhead = lastindex;
		to_remove --;

		if ((ts->termio.c_lflag & ECHOE)
		    && (ts->termio.c_lflag & ECHO))
		  {
		    minibuf[0] = '\b';
		    minibuf[1] = ' ';
		    minibuf[2] = '\b';
		    res = terminal_write (ts, minibuf, 3);
		    /*  Don't care about res...  */
		  }

		if (ch == ts->termio.c_cc[VKILL])
		  {
		    if (ts->inputhead == ts->inputtail)
			return 0;

		    lastindex = ts->inputhead - 1;
		    if (lastindex < 0)
			lastindex = MAX_INPUT - 1;

		    if (ts->inputbuf[lastindex] == ts->termio.c_cc[VEOF] ||
			ts->inputbuf[lastindex] == ts->termio.c_cc[VEOL] ||
			ts->inputbuf[lastindex] == '\n')
		      {
			/*  ECHOK:  echo NL after line kill  */
			if (ts->termio.c_lflag & ECHOK)
			  {
			    minibuf[0] = '\n';
			    res = terminal_write (ts, minibuf, 1);
			    /*  Don't care about res...  */
			  }
		        return 0;
		      }

		    to_remove ++;
		  }
	      }

	    return 0;
	  }


	/*
         *  Wakeup any process (can there be several??? TODO) that
	 *  are waiting for input in canonical mode.
	 */

	if (ch == ts->termio.c_cc[VEOF] ||
		ch == ts->termio.c_cc[VEOL] || ch == '\n')
	  wakeup (&ts->linemode_sleep);
      }
    else
      {
	/*  In non-canonical mode, we always wake up the process:  */
	/*  TODO: first of all, this should be done with interrupts
		disabled etc and we should check the contents of the
		sleep variable to see if we actually need to call wakeup()
		since it takes a lot of CPU cycles... second of all,
		there should be one linemode_sleep and one other XX_sleep  */
	wakeup (&ts->linemode_sleep);
      }


    /*
     *	XON/XOFF flow control. (I'm not sure that this is correct.
     *	If IXANY is used, and the user presses XOFF (VSTOP) twice,
     *	should the output continue then or is it only all-keys-but-xoff
     *	that should re-enable output? TODO)
     */

    if (iflag & IXANY)
	ts->stopped = 0;

    if (iflag & IXON)
      {
	if (ch == ts->termio.c_cc[VSTOP])
	  ts->stopped = 1;

	if (ch == ts->termio.c_cc[VSTART])
	  ts->stopped = 0;

	return 0;
      }


    /*
     *	Local echo? Then write the char using terminal_write():
     */

    if ((ts->termio.c_lflag & ECHO) ||
	((ts->termio.c_lflag & ECHONL) && ch=='\n') )
      {
	minibuf[0] = ch;
	res = terminal_write (ts, minibuf, 1);
	/*  Don't care about resulting value...  */
      }


    /*
     *	Add the char to the inputbuf. ts->inputhead is the index where
     *	we shall place the incoming character.
     */

    next = ts->inputhead + 1;
    if (next >= MAX_INPUT)
	next = 0;

    /*  Cannot add chars when the buffer is full:  */
    if (next == ts->inputtail)
	return -1;

    ts->inputbuf [ts->inputhead] = ch;
    ts->inputhead = next;

    return 0;
  }



size_t terminal_read (struct termstate *ts, char *buf, size_t maxlen)
  {
    /*
     *	terminal_read ()
     *	----------------
     *
     *	Try to read characters from ts->inputbuf. If we are in
     *	canonical input mode, we wait for EOF, EOL, or newline and
     *	then return (at most) one line of data.
     *
     *	Return 0 on error, or of nothing was read.
     *
     *	In non-canonical mode (ie character mode) we simply return
     *	as much data as possible (only limited by maxlen).
     */

    size_t chars_read = 0;
    size_t tmplen;
    int index;
    char ch;
    int oldints;
    int linefound;


    if (!ts || !buf)
	return 0;

    if (ts->termio.c_lflag & ICANON)
      {
	oldints = interrupts (DISABLE);

	while (chars_read == 0)
	  {
	    /*  Do we have a whole line already?  */
	    linefound = 0;
	    index = ts->inputtail;

	    while (index != ts->inputhead)
	      {
		ch = ts->inputbuf [index++];
		if (index >= MAX_INPUT)
		  index = 0;

		if (ch == ts->termio.c_cc[VEOF] ||
			ch == ts->termio.c_cc[VEOL] || ch == '\n')
		  linefound = 1;
	      }

	    if (linefound)
	      {
		/*  Return the line we found:  */
		tmplen = 0;
		index = ts->inputtail;
		while (tmplen < maxlen)
		  {
		    ch = ts->inputbuf [index++];
		    if (index >= MAX_INPUT)
			index = 0;
		    buf[tmplen++] = ch;
		    if (ch == ts->termio.c_cc[VEOF] ||
			ch == ts->termio.c_cc[VEOL] || ch == '\n')
		      tmplen = maxlen;
		    chars_read ++;
		  }
		ts->inputtail = index;
	      }
	    else
	      {
		/*  Let's sleep until a line is read:  */
		sleep (&ts->linemode_sleep, "terminal_read");
	      }
	  }

	interrupts (oldints);
	return chars_read;
      }

    /*  Non-canonical mode, return data from inputbuf:  */
    /*  TODO:  rename linemode_sleep to something else...  */
    if (ts->inputtail == ts->inputhead)
	sleep (&ts->linemode_sleep, "terminal_read");

    while (maxlen > 0)
      {
	if (ts->inputtail == ts->inputhead)
	  return chars_read;

	buf[0] = ts->inputbuf[ts->inputtail++];
	if (ts->inputtail >= MAX_INPUT)
	  ts->inputtail = 0;

	buf ++;
	maxlen --;
	chars_read ++;
      }

    return chars_read;
  }

