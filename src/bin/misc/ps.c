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


void ps_showentry (char *procname, struct stat *ss)
  {
    static int ps_first = 1;

    if (ps_first)
      {
	/*       12345678 12345  */
	printf ("USER       PID\n");
	ps_first = 0;
      }

    printf ("%-8i", ss->st_uid);
    printf (" %5s", procname);
    printf ("\n");
  }



int main_ps (int argc, char *argv[])
  {
    struct dirent *dp;
    DIR *dirp;
    struct stat ss;
    char tmpname [50];
    int err;

printf ("YO2!\n");
    dirp = opendir("/proc");
    if (dirp)
      {
printf ("YO!\n");
	while ((dp = readdir(dirp)) != NULL)
	  {
printf ("'%s'\n", dp->d_name);
	    if (dp->d_name[0]!='.')
	      {
		snprintf (tmpname, 50, "/proc/%s", dp->d_name);
		err = stat (tmpname, &ss);
		if (err)
		    printf ("%s: stat proc %s: error %i\n",
			argv[0], dp->d_name, err);
		else
		  ps_showentry (dp->d_name, &ss);
	      }
	  }
	closedir(dirp);
      }
    else
      printf ("%s: could not open directory /proc\n", argv[0]);

    return 0;
  }

