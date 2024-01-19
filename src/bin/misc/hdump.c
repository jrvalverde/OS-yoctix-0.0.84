/*
 *  Copyright (C) 1997,2000,2001 by Anders Gavare.  All rights reserved.
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

/*  hdump  --  file hex/asc dump utility  */

#include <stdio.h>
#include <unistd.h>

#define MAXBUFSIZE 64
#define bufsize 16
#define unprintable '.'


char hexaddr[9];

void to_hex (int n)
  {
  int p=7;
  do
    {
      hexaddr[p] = (n & 15) + 48;
      n = (n >> 4);
      if (hexaddr[p]>=58)  hexaddr[p]+=39;
      p--;
    }
  while (p>=0);
  hexaddr[8]='\0';
  }


void dump_file (char *filename)
  {
    FILE *f;
    char buf[MAXBUFSIZE];
    int siz, addr, i;

    if ((f=fopen(filename, "rb"))==NULL)
      {
	perror (filename);
	return;
      }

    addr = 0;
    do
      {
	siz = fread (buf, 1, bufsize, f);
	if (siz<1)  {  fclose (f); return;  }
	to_hex (addr);
	printf ("%s  ", hexaddr);
	for (i=0; i<siz; i++)
	  {
	    to_hex (buf[i]);
	    printf ("%s ", hexaddr+6);
	  }
	while (i<bufsize)
	  {  printf ("   ");  i++;  }
	putchar (' ');
	for (i=0; i<siz; i++)
	  {
	    if ( (buf[i]>31) && (buf[i]!=0x7f) )
		putchar (buf[i]);
	    else putchar (unprintable);
	  }
	putchar ('\n');
	addr += bufsize;
      }
    while (!feof(f));
    fclose (f);
  }


int main_hdump (int argc, char *argv[])
  {
    int argpos;

    if (argc==1)
      {
	printf ("usage: %s filename [..]\n", argv[0]);
	exit (1);
      }

    argpos=1;
    while (argpos<argc)
      {
	if (argc>2)
	  printf ("%s:\n", argv[argpos]);

	dump_file (argv[argpos]);
	argpos ++;
      }

    return 0;
  }

