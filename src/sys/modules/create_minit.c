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
 *  modules/create_minit.c  --  statically create modules_init.c
 *
 *  History:
 *	5 Jan 2000	first version
 */



#include "../config.h"
#include <stdio.h>


#define	MAXBUF	2000

FILE *f;
char buf[MAXBUF+1];
int i;


int main (int argc, char *argv[])
  {
    f = fopen ("CONFIG", "r");

    printf ("/*\n"
	    " *  This file was automagically created by create_minit.\n"
	    " *  DO NOT EDIT!!!\n"
	    " */\n\n");

    printf ("#include <sys/module.h>\n"
	    "#include \"../config.h\"\n\n");

    while (!feof(f))
      {
	buf[0]=0;
	fgets (buf, MAXBUF, f);

	if (buf[0]!='\n' && buf[0]!='#' && buf[0]!='\0')
	  {
	    /*  Get the first word from the line:  */
	    for (i=0; i<strlen(buf); i++)
		if (buf[i]<=32)
			buf[i]=0;

	    /*  Print a XXX_init(); line to stdout:  */
	    printf ("void %s_init ();\n", buf);
	  }
      }

    printf ("\n\nvoid staticmodules_init ()\n"
	"  {\n");

    fseek (f,0,0);

    while (!feof(f))
      {
	buf[0]=0;
	fgets (buf, MAXBUF, f);

	if (buf[0]!='\n' && buf[0]!='#' && buf[0]!='\0')
	  {
	    /*  Get the first word from the line:  */
	    for (i=0; i<strlen(buf); i++)
		if (buf[i]<=32)
			buf[i]=0;

	    /*  Print a XXX_init(); line to stdout:  */
	    printf ("    %s_init (MODULE_INIT);\n", buf);
	  }
      }

    printf ("  }\n\n");
    return 0;
  }


