/*
 *  Copyright (C) 1999,2001 by Anders Gavare.  All rights reserved.
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
 *  mkdir(1)  --  part of Yoktix by Anders Gavare
 *
 *  usage: mkdir [-p] [-m mode] dirname [...]
 *
 *  History:
 *	15 Oct 1999	First version. -p and -m options.
 *	11 Feb 2001	included in misc
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>


extern char *optarg;
extern int optind;


void mkdir_help (char *name)
  {
    fprintf (stderr, "usage: %s [-p] [-m mode] dirname [...]\n", name);
  }


int main_mkdir (int argc, char *argv[])
  {
    int switch_m = 0777;
    int switch_p = 0;
    int ch;
    int i, q, res;
    int nroferrors = 0;
    char pathbuf [FILENAME_MAX];

    if (argc<2)
      {
	mkdir_help (argv[0]);
	exit (1);
      }

    while ((ch = getopt(argc, argv, "pm:")) != -1)
      {
	switch (ch)
	  {
	    case 'p':
		switch_p = 1;
		break;
	    case 'm':
		switch_m = atoi(optarg);
		if (switch_m == 0)
		  {
		    fprintf (stderr, "%s: warning! creating dir with mode %i\n",
			argv[0], switch_m);
		  }
		break;
	    default:
		mkdir_help (argv[0]);
		exit (1);
	  }
       }


    i = optind;

    while (i < argc)
      {
	if (switch_p)
	  {
	    if (strlen(argv[i])>=FILENAME_MAX-1)
	      fprintf (stderr, "%s: name too long\n", argv[i]);
	    else
	      {
		/*  Create each directory specified in argv[i]:  */
		q = 0;
		while (q<strlen(argv[i]))
		  {
		    pathbuf[q] = argv[i][q];
		    q++;
		    pathbuf[q] = '\0';
		    if (argv[i][q]=='/')
			mkdir (pathbuf, 0777);
		  }

		mkdir (argv[i], switch_m);
	      }
	  }
	else
	  {
	    res = mkdir (argv[i], switch_m);

	    if (res)
	      {
		perror (argv[i]);
		nroferrors ++;
	      }
	  }

	i ++;
      }


    if (nroferrors>0)
	return 1;

    return 0;
  }


