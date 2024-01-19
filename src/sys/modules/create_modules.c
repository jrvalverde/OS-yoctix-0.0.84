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
 *  modules/create_modules.c  --  compile (or clean) module subdirectories
 *
 *  History:
 *	5 Jan 2000	first version
 */



#include "../config.h"
#include <stdio.h>


#define	MAXBUF	2000

#define	MODE_COMPILE	1
#define	MODE_CLEAN	2

int mode = 0;

FILE *f;
char buf[MAXBUF+1];
char modulename[MAXBUF+1];
char moduledir[MAXBUF+1];
int i;


int main (int argc, char *argv[])
  {
    if (argc==1 && strcmp(argv[1], "clean") && strcmp(argv[1], "compile"))
      {
	printf ("usage:\n"
		"  %s clean       Clean module subdirectories\n"
		"  %s compile     Compile everything\n", argv[0], argv[0]);
	exit (1);
      }

    if (!strcmp(argv[1], "compile"))	mode = MODE_COMPILE;
    if (!strcmp(argv[1], "clean"))	mode = MODE_CLEAN;


    f = fopen ("CONFIG", "r");

    while (!feof(f))
      {
	buf[0]=0;
	modulename[0]=0;
	moduledir[0]=0;
	fgets (buf, MAXBUF, f);

	if (buf[0]!='\n' && buf[0]!='#' && buf[0]!='\0')
	  {
	    /*  Get the first word from the line:  */
	    for (i=0; i<strlen(buf); i++)
		if (buf[i]<=32)
		  {
		    strcpy (modulename, buf);
		    modulename[i]=0;
		    i++;
		    /*  Search for first char of moduledir:  */
		    while (i<strlen(buf))
		      {
			if (buf[i]>32)
			  {
			    strcpy (moduledir, buf+i);
			    /*  abort search:  */
			    i = MAXBUF;
			  }
			i++;
		      }
		  }

	    /*  moduledir has a \n at the end:  */
	    if (moduledir[0]==0)
		{
		  printf ("%s has no directory!\n", modulename);
		  exit (1);
		}
	    if (moduledir[strlen(moduledir)-1]=='\n')
		moduledir[strlen(moduledir)-1]='\0';

	    printf ("# %s module '%s' in '%s'\n",
		mode==MODE_CLEAN?"Cleaning":"Compiling",
		modulename, moduledir);

	    if (mode==MODE_CLEAN)
	      {
		sprintf (buf, "cd %s; make clean; cd -", moduledir);
		printf ("# %s\n", buf);
		system (buf);
	      }

	    if (mode==MODE_COMPILE)
	      {
		sprintf (buf, "cd %s; make; cd -", moduledir);
		printf ("# %s\n", buf);
		system (buf);

		sprintf (buf, "ar r libmodules.a %s/*.o", moduledir);
		printf ("# %s\n", buf);
		system (buf);
	      }

	  }
      }

    return 0;
  }


