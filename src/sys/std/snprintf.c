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
 *  snprintf.c  --  printf to a string buffer
 *
 *  NOTE!!!  Assumes that we're allowed to write the string to memory,
 *		and to read all the given arguments!
 *
 *  Currently implemented tokens:
 *	%s	string			(char *)
 *	%c	char			(signed char)
 *	%i	int			(signed int)
 *	%u	int			(unsigned int)
 *	%x	32-bit hex number	(u_int32_t)
 *	%X	64-bit hex number
 *	%y	8-bit hex number
 *	%Y	16-bit hex number
 *
 *  History:
 *	18 Oct 1999	first version
 *	21 Oct 1999	refreshing
 */


#include <stdarg.h>
#include <string.h>


/*  This should be able to hold a full integer  */
#define	SNPRINTF__BUFLEN	40


static char *snprintf_hexc = "0123456789abcdef";
static char snprintf__buf[SNPRINTF__BUFLEN];


void snprintf__int2str (int number)
  {
    int i = SNPRINTF__BUFLEN-1;
    int rest, signedbit;

    signedbit = number<0? 1:0;
    if (signedbit)
	number = -number;

    memset (snprintf__buf, ' ', SNPRINTF__BUFLEN);
    snprintf__buf[i] = '\0';  i--;
    snprintf__buf[i] = '0';

    while (number>0 && i>=0)
      {
	rest = number % 10;
	snprintf__buf[i] = snprintf_hexc[rest];
	i--;
	number /= 10;
      }

    if (signedbit)
	snprintf__buf[i] = '-';
  }



void snprintf__uint2str (unsigned int number)
  {
    int i = SNPRINTF__BUFLEN-1;
    unsigned int rest;

    memset (snprintf__buf, ' ', SNPRINTF__BUFLEN);
    snprintf__buf[i] = '\0';  i--;
    snprintf__buf[i] = '0';

    while (number>0 && i>=0)
      {
	rest = number % 10;
	snprintf__buf[i] = snprintf_hexc[rest];
	i--;
	number /= 10;
      }
  }



size_t vsnprintf (char *str, size_t len, const char *fmt, va_list argp)
  {
    size_t outputlen = 0;
    char *p, ch, ch2;
    int fmtlen, fmtpos;
    int i, slen;
    s_int8_t si8;
    s_int16_t si16;
    s_int32_t si32;
/*
    s_int64_t si64;
*/

    if (!str)
	return 0;

    if (!fmt)
      {
	str[0] = '\0';
	return 0;
      }

    fmtlen = strlen ((char *)fmt);
    fmtpos = 0;


    while (fmtpos < fmtlen)
      {
	ch = fmt[fmtpos];
	if (ch=='%')
	  {
	    ch = fmt[++fmtpos];
	    if (ch=='%')
	      {
		str[outputlen++] = '%';
		if (outputlen >= len-1)
		  fmtpos = fmtlen;	/*  abort  */
	      }
	    else
	    if (ch=='i')
	      {
		/*  int  */
		i = va_arg (argp, int);
		snprintf__int2str (i);
		i = 0;
		while (snprintf__buf[i])
		  {
		    if (snprintf__buf[i]!=' ')
		      str[outputlen++] = snprintf__buf [i];
		    if (outputlen >= len-1)
		      {
			fmtpos = fmtlen;	/*  abort  */
			i = SNPRINTF__BUFLEN-2;
		      }
		    i++;
		  }
	      }
	    else
	    if (ch=='u')
	      {
		/*  int  */
		i = va_arg (argp, int);
		snprintf__uint2str (i);
		i = 0;
		while (snprintf__buf[i])
		  {
		    if (snprintf__buf[i]!=' ')
		      str[outputlen++] = snprintf__buf [i];
		    if (outputlen >= len-1)
		      {
			fmtpos = fmtlen;	/*  abort  */
			i = SNPRINTF__BUFLEN-2;
		      }
		    i++;
		  }
	      }
	    else
	    if (ch=='c')
	      {
		/*  Char  */
		ch2 = va_arg (argp, int);
		str[outputlen++] = ch2;
		if (outputlen >= len-1)
		  fmtpos = fmtlen;	/*  abort  */
	      }
	    else
	    if (ch=='s')
	      {
		/*  String (this is slow, should be optimized!):  */
		p = va_arg (argp, char *);
		if (!p)
		  p = "(null)";
		slen = strlen (p);
		for (i=0; i<slen; i++)
		  {
		    str[outputlen++] = p[i];
		    if (outputlen >= len-1)
		      {
			fmtpos = fmtlen;
			i = slen;  /*  abort  */
		      }
		  }
	      }
	    else
	    if (ch=='x')
	      {
		/*  32-bit hex  */
		si32 = va_arg (argp, u_int32_t);
		for (i=0; i<8; i++)
		  {
		    ch2 = (si32 >> ((7-i)*4)) & 15;
		    if (outputlen+i < len-1)
			str[outputlen+i] = snprintf_hexc [(int)ch2];
		  }
		outputlen += 8;
		if (outputlen >= len-1)
		    fmtpos = fmtlen;
	      }
	    else
	    if (ch=='X')
	      {
		/*  64-bit hex  */
/*
		si64 = va_arg (argp, u_int64_t);
		for (i=0; i<16; i++)
		  {
		    ch2 = (si64 >> ((15-i)*4)) & 15;
		    if (outputlen+i < len-1)
			str[outputlen+i] = snprintf_hexc [(int)ch2];
		  }
		outputlen += 16;
		if (outputlen >= len-1)
		    fmtpos = fmtlen;
*/
	      }
	    else
	    if (ch=='y')
	      {
		/*  8-bit hex  */
		si8 = va_arg (argp, u_int8_t);
		for (i=0; i<2; i++)
		  {
		    ch2 = (si8 >> ((1-i)*4)) & 15;
		    if (outputlen+i < len-1)
			str[outputlen+i] = snprintf_hexc [(int)ch2];
		  }
		outputlen += 2;
		if (outputlen >= len-1)
		    fmtpos = fmtlen;
	      }
	    else
	    if (ch=='Y')
	      {
		/*  16-bit hex  */
		si16 = va_arg (argp, u_int16_t);
		for (i=0; i<4; i++)
		  {
		    ch2 = (si16 >> ((3-i)*4)) & 15;
		    if (outputlen+i < len-1)
			str[outputlen+i] = snprintf_hexc [(int)ch2];
		  }
		outputlen += 4;
		if (outputlen >= len-1)
		    fmtpos = fmtlen;
	      }
	  }
	else
	  {
	    str[outputlen++] = ch;
	    if (outputlen >= len-1)
		fmtpos = fmtlen;	/*  abort  */
	  }

	/*  Advance position in fmt:  */
	fmtpos++;
      }


    if (outputlen >= len-1)
	outputlen = len-1;

    /*  Terminate str  */
    str[outputlen] = '\0';

    return outputlen;
  }



size_t snprintf (char *str, size_t len, const char *fmt, ...)
  {
    size_t result;

    va_list argp;
    va_start (argp, fmt);

    result = vsnprintf (str, len, fmt, argp);

    va_end (argp);
    return result;
  }


