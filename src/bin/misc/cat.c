/*
 *  Copyright (C) 1999,2000,2001 by Anders Gavare.  All rights reserved.
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
 *  cat(1)  --  tiny Yoctix cat
 *
 *  usage: cat [filename [..]]
 *
 *  History:
 *      29 Sep 1999	test
 *	24 Jul 2000	refreshed
 *	7 Feb 2001	included in misc
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


#define	MAX_BUF_SIZE	32768



void cat_argerror (char *progname, char *filename)
  {
    char *buf;

    buf = (char *) malloc (strlen(progname) + strlen(filename) + 10);
    sprintf (buf, "%s: %s", progname, filename);
    perror (buf);
    free (buf);
    fflush (stderr);
  }



int cat (int f)
  {
    char buf [MAX_BUF_SIZE];
    int len = 1;

    while (len > 0)
      {
	len = read (f, buf, MAX_BUF_SIZE);
	if (len > 0)
	  write (1, buf, len);
      }

    return 0;
  }



int main_cat (int argc, char *argv[])
  {
    int f, i, res=0;

    if (argc==1)
      return cat (0);

    i = 1;
    while (i < argc)
      {
	f = open (argv[i], O_RDONLY);

	if (f >= 0)
	  {
	    if (cat (f))
		res = 1;
	    close (f);
	  }
	else
	  {
	    cat_argerror (argv[0], argv[i]);
	    res = 1;
	  }

	i++;
      }

    return res;
  }

