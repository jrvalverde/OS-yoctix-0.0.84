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
 *  termios.h
 */


#ifndef	__TERMIOS_H
#define	__TERMIOS_H

#include <sys/defs.h>


/*
 *  Control characters:
 */

#define	VEOF		0		/*  EOF   character	*/
#define	VEOL		1		/*  EOL   character	*/
#define	VERASE		2		/*  ERASE character	*/
#define	VINTR		3		/*  INTR  character	*/
#define	VKILL		4		/*  KILL  character	*/
#define	VQUIT		5		/*  QUIT  character	*/
#define	VSUSP		6		/*  SUSP  character	*/
#define	VSTART		7		/*  START character	*/
#define	VSTOP		8		/*  STOP  character	*/
#define	VMIN		9		/*  MIN   value		*/
#define	VTIME		10		/*  TIME  value		*/

#define	NCCS		11


/*
 *  The termios struct:
 */

typedef u_int32_t	tcflag_t;
typedef	u_int32_t	speed_t;
typedef	byte		cc_t;

struct termios
      {
	tcflag_t	c_iflag;	/*  Input flags  */
	tcflag_t	c_oflag;	/*  Output flags  */
	tcflag_t	c_cflag;	/*  Control flags  */
	tcflag_t	c_lflag;	/*  Local flags  */
	cc_t		c_cc[NCCS];	/*  Control characters  */
	speed_t		c_ispeed;	/*  Input line speed  */
	speed_t		c_ospeed;	/*  Output line speed  */
      };


/*
 *  Input flags:
 *
 *  Note: The order is as listed by the Single Unix Specification,
 *  but values are choosen to be compatible with OpenBSD.
 */

#define	BRKINT		0x00000002	/*  Signal interrupt on break.  */
#define	ICRNL		0x00000100	/*  Map CR to NL on input.  */
#define	IGNBRK		0x00000001	/*  Ignore break condition.  */
#define	IGNCR		0x00000080	/*  Ignore CR.  */
#define	IGNPAR		0x00000004	/*  Ignore characters with parity errors.  */
#define	INLCR		0x00000040	/*  Map NL to CR on input.  */
#define	INPCK		0x00000010	/*  Enable input parity check.  */
#define	ISTRIP		0x00000020	/*  Strip 8th bit of character.  */
#define	IUCLC		0x00001000	/*  Map upper case to lower case on input.  */
#define	IXANY		0x00000800	/*  Enable any character to restart output.  */
#define	IXOFF		0x00000400	/*  Enable start/stop input control.  */
#define	IXON		0x00000200	/*  Enable start/stop output control.  */
#define	PARMRK		0x00000008	/*  Mark parity errors.  */


/*
 *  Output flags:
 *
 *  Note: The order is as listed by the Single Unix Specification,
 *  but values are choosen to be compatible with OpenBSD.
 */

#define	OPOST		0x00000001	/*  Perform output processing.  */
#define	OLCUC		0x00000020	/*  Map lower case to upper on output.  */
#define	ONLCR		0x00000002	/*  Map NL to CR-NL on output.  */
#define	OCRNL		0x00000010	/*  Map CR to NL on output.  */
#define	ONOCR		0x00000040	/*  No CR output at column 0.  */
#define	ONLRET		0x00000080	/*  NL performs CR function.  */


/*
 *  Control flags:
 *
 *  Note: The order is as listed by the Single Unix Specification,
 *  but values are choosen to be compatible with OpenBSD.
 */

#define	CLOCAL		0x00008000	/*  Ignore modem status lines.  */
#define	CREAD		0x00000800	/*  Enable receiver.  */
#define	CSIZE		0x00000300
#define	  CS5		0x00000000	/*  5 bits  */
#define	  CS6		0x00000100	/*  6 bits  */
#define	  CS7		0x00000200	/*  7 bits  */
#define	  CS8		0x00000300	/*  8 bits  */
#define	CSTOPB		0x00000400	/*  Send 2 stopbits instead of 1.  */
#define	HUPCL		0x00004000	/*  Hang up on last close.  */
#define	PARENB		0x00001000	/*  Parity enable.  */
#define	PARODD		0x00002000	/*  Odd parity, else even.  */


/*
 *  Local flags:
 *
 *  Note: The order is as listed by the Single Unix Specification,
 *  but values are choosen to be compatible with OpenBSD.
 */

#define	ECHO		0x00000008	/*  Enable echo.  */
#define	ECHOE		0x00000002	/*  Echo ERASE as an error correcting backspace.  */
#define	ECHOK		0x00000004	/*  Echo NL after line kill  */
#define	ECHONL		0x00000010	/*  Echo NL even if ECHO is off.  */
#define	ICANON		0x00000100	/*  Canonical input (erase/kill processing).  */
#define	IEXTEN		0x00000400	/*  Enable extended functions.  */
#define	ISIG		0x00000080	/*  Enable signals: INTR, QUIT, SUSP  */
#define	NOFLSH		0x80000000	/*  Disable flush after interrupt, quit, or suspend.  */
#define	TOSTOP		0x00400000	/*  Send SIGTTOU for background output.  */
#define	XCASE		0x01000000	/*  Canonical upper/lower presentation.  */


#endif	/*  __TERMIOS_H  */
