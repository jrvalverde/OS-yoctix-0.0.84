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
 *  getty(8)  --  tiny Yoctix getty
 *
 *  usage: getty [tty]
 *
 *  History:
 *      2 Jun 2000	first version
 */


#include <sys/utsname.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>


extern int errno;


#define	LOGINMAX	256
#define	BUFLEN		4096



int main (int argc, char *argv[], char *envp[])
  {
    int res;
    int d_in, d_out;
    struct utsname un;
    char ttyname [30];
    char loginname [LOGINMAX+1];
    char buf [BUFLEN];
    char *loginargs[10];
    int i;

    if (argc<2)
      {
	printf ("usage: %s ttyname\n", argv[0]);
	exit (1);
      }

    /*  If a tty name was given without /, assume "/dev/":  */
    if (argc>1)
      {
	if (argv[1][0]!='/')
	  snprintf (ttyname, sizeof(ttyname), "/dev/%s", argv[1]);
	else
	  snprintf (ttyname, sizeof(ttyname), "%s", argv[1]);
      }
    else
      ttyname[0]=0;

    if (ttyname[0])
      {
	d_in = open (ttyname, O_RDONLY);
	if (d_in<0)
	  {
	    sprintf (buf, "%s: could not open %s\n", argv[0], ttyname);
	    write (STDOUT_FILENO, buf, strlen(buf));
	    exit (1);
	  }
	dup2 (d_in, STDIN_FILENO);
	close (d_in);

	d_out = open (ttyname, O_WRONLY);
	if (d_out<0)
	  {
	    sprintf (buf, "%s: could not open %s\n", argv[0], ttyname);
	    write (STDOUT_FILENO, buf, strlen(buf));
	    exit (1);
	  }
	dup2 (d_out, STDOUT_FILENO);
	dup2 (d_out, STDERR_FILENO);
	close (d_out);
      }
    else
      {
	printf ("%s: bad tty name\n", argv[0]);
	exit (1);
      }

    while (1)
      {
	/*
	 *  Print system login banner, something like:
	 *
	 *	OS/arch (hostname) (tty)
	 *
	 *  or just
	 *
	 *	hostname (tty)
	 */

	memset (&un, 0, sizeof(un));
	res = uname (&un);

	/*  sprintf (buf, "\n%s/%s (%s)", un.sysname, un.machine, un.nodename);  */
	sprintf (buf, "\n%s", un.nodename);

	if (ttyname[0])
	  sprintf (buf+strlen(buf), " (%s)", argv[1]);

	sprintf (buf+strlen(buf), "\n\nlogin: ");
	write (STDOUT_FILENO, buf, strlen(buf));

	loginname[0] = 0;
	res = read (STDIN_FILENO, loginname, LOGINMAX);

	if (loginname[0])
	  {
	    loginname[res] = '\0';

	    if (loginname[strlen(loginname)-1]=='\n')
		loginname[strlen(loginname)-1]='\0';

	    if (isalnum(loginname[0]))
	      {
		/*  Execute /bin/login:  */

		loginargs[0] = "/bin/login";
		loginargs[1] = loginname;
		loginargs[2] = NULL;

		/*  TODO:  this should depend on entries in /etc/ttys  */
		putenv ("TERM=vt100");

		res = execv ("/bin/login", loginargs);

		sprintf (buf, "%s: execve /bin/login failed: %s\n",
			argv[0], sys_errlist[errno]);
		write (STDOUT_FILENO, buf, strlen(buf));
		exit (0);
	      }
	    else
	    if (loginname[0])
	      {
		/*  Incorrect first char:  */
		sprintf (buf, "Login incorrect\n");
		write (STDOUT_FILENO, buf, strlen(buf));
	      }

	  }

      }

    return 0;
  }

