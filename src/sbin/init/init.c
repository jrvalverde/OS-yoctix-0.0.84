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
 *  init(8)  --  tiny Yoctix /sbin/init
 *
 *	If run as 'halt' ==> reboot (RB_HALT);
 *	If run as 'reboot' ==> reboot (RB_AUTOBOOT);
 *
 *  History:
 *      7 Jun 2000	first version (ugly)
 *	27 Jun 2000	restarts gettys when they die
 */


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/reboot.h>


#define	MAXARGLEN	30
#define	GETTY_NAME	"/sbin/getty"

struct childproc
      {
	char	*progname;
	char	arg1[MAXARGLEN];
	pid_t	pid;
	struct childproc *next;
      };


#define	TTYS_FILE	"/etc/ttys"
#define	BUFLEN		8192


char **envir;
int ttys_d;

#define	MAXCHILD	50
struct childproc child [MAXCHILD];
int childnr = 0;

/*
struct childproc *firstchild = NULL;
*/


void start_child (struct childproc *c)
  {
    char tmpbuf [BUFLEN];
    int pid;
    char *args[3];

    args[0] = c->progname;
    args[1] = c->arg1;
    args[2] = NULL;

    pid = fork ();
/*
    sprintf (tmpbuf, "init: pid=%i\n", pid);
    write (1, tmpbuf, strlen(tmpbuf));
*/
    if (pid == 0)
      {
	close (ttys_d);

	execve (c->progname, args, envir);

	sprintf (tmpbuf, "init: could not execute %s\n", c->progname);
	write (1, tmpbuf, strlen(tmpbuf));
	c->pid = 0;
      }
    else
	c->pid = pid;
  }



void start_getty (char *ttyname)
  {
    struct childproc *c;
/*
    char **args;

    args = (char **) malloc (sizeof(char *) * 3);
    args[0] = GETTY_NAME;
    args[1] = ttyname;
    args[2] = NULL;
*/

/*
    c = (struct childproc *) malloc (sizeof(struct childproc));
*/
c = &child[childnr++];

    c->progname = GETTY_NAME;
    strcpy (c->arg1, ttyname);

/*
    c->args = args;
    c->next = firstchild;
*/
    c->pid = 0;
/*
    firstchild = c;
*/

    start_child (c);
  }



int start_rc ()
  {
    char *path = "/bin/sh";
    char *args[] = { "/etc/rc", NULL };
    pid_t p;
    int status;

    p = fork ();
printf ("before: p=%i\n", p);
    if (p==0)
      {
	execve (path, args, envir);
	perror ("execve");
	exit (1);
      }
    p = wait (&status);
printf ("return: p=%i\n", p);
    return 0;
  }



int main (int argc, char *argv[], char *envp[])
  {
    char buf [BUFLEN];
    char tmpbuf [BUFLEN];
    int i, i2, len;
    pid_t p;
    int status;
    struct childproc *c;
    char *pname, *pname2;


    /*  Check argv[0] for 'init', 'halt', 'reboot'...  */
    pname = argv[0];
    if (argv[0])
      {
	while ((pname2 = strstr(pname, "/")))
		pname = pname2;
	if (pname[0]=='/')
		pname++;

	if (!strcmp(pname, "halt"))
	  {
	    reboot (RB_HALT);
	    perror ("reboot (RB_HALT)");
	    exit (1);
	  }

	if (!strcmp(pname, "reboot"))
	  {
	    reboot (RB_AUTOBOOT);
	    perror ("reboot (RB_AUTOBOOT)");
	    exit (1);
	  }
      }


    if (getpid() != 1)
      {
	sprintf (tmpbuf, "%s: must be started with pid=1 by the kernel\n", argv[0]);
	write (1, tmpbuf, strlen(tmpbuf));
	exit (1);
      }


    envir = envp;

printf ("A\n");
    start_rc ();
printf ("B\n");

for (i=0; i<MAXCHILD; i++)
	child[i].pid = 0;

    ttys_d = open (TTYS_FILE, O_RDONLY);
    if (ttys_d<0)
      {
	sprintf (tmpbuf, "%s: warning! could not open "TTYS_FILE"\n", argv[0]);
	write (1, tmpbuf, strlen(tmpbuf));

	/*  Assume /dev/ttyC0:  */
	start_getty ("/dev/ttyC0");
      }
    else
      {
	/*  Read names of ttys from ttys_d:  */
	/*  TODO:  this should be line-buffered...  */
	len = read (ttys_d, buf, BUFLEN-1);
	if (len < 1)
	  {
	    sprintf (tmpbuf, "%s: error reading "TTYS_FILE"\n", argv[0]);
	    write (1, tmpbuf, strlen(tmpbuf));
	    close (ttys_d);

	    /*  Assume /dev/ttyC0:  */
	    start_getty ("/dev/ttyC0");
	    for (;;);
	  }

	buf[len]=0;
	/*  Empty lines and lines beginning with a '#' sign are ignored:  */
	i=0;
	while (i<len)
	  {
	    if (buf[i]=='\n')
		i++;
	    else
	    if (buf[i]=='#')
	      {
		while (buf[i]!='\n' && i<len)
		  i++;
	      }
	    else
	      {
		/*  This is a tty name. Find end of line:  */
		i2 = i;
		while (buf[i2]!='\n' && i2<len)
			i2++;
		buf[i2]=0;
		start_getty (buf+i);
		i = i2+1;
	      }
	  }
      }

    close (ttys_d);


    for (;;)
      {
	p = wait (&status);

	/*  Restart pid p:  */
	i = 0;
	while (i<MAXCHILD)
	  {
	    c = &child[i];
	    if (c->pid == p)
		start_child (c);
	    i++;
	  }

/*
	c = firstchild;
	while (c)
	  {
	    if (c->pid == p)
		start_child (c);
	    c = c->next;
	  }
*/

      }

    return 0;
  }

