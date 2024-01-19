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
 *  login(1)  --  tiny Yoctix login program
 *
 *  usage: login username
 *
 *  History:
 *      15 Jun 2000	first version
 *	7 Sep 2000	uses getpwnam(), but still doesn't care about password
 */


#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <pwd.h>


/*  #define FORTUNE  */
/*  #define SHOW_MOTD  */


#define	LOGINMAX	256
#define	BUFLEN		1024


void show_uname ()
  {
    struct utsname un;
    int res;

    res = uname (&un);
    if (res)
	perror ("uname()");
    else
	printf ("%s %s %s %s %s\n",
		un.sysname, un.nodename, un.release,
		un.version, un.machine);
  }



#if SHOW_MOTD
void cat_etcmotd ()
  {
    FILE *f;
    char buf[BUFLEN];
    int len;

    f = fopen ("/etc/motd", "r");
    if (!f)
	return;

    while (!feof(f))
      {
	len = fread (buf, 1, 5, f);
	if (len > 0)
	  {
	    buf[len]=0;
	    printf ("%s", buf);
	  }
      }

    fflush (stdout);
    fclose (f);
  }
#endif



#ifdef FORTUNE
void print_random_fortune (char *fname)
  {
    FILE *f;
    char buf[BUFLEN];
    int len;
    int ndelims = 0;
    int i, r;

    f = fopen (fname, "r");
    if (!f)
	return;

    /*  1. Count nr of % signs...  */
    while (!feof(f))
      {
	buf[0]=0;
	fgets (buf, BUFLEN-1, f);
	if (buf[0] > 0)
	  if (buf[0]=='%' && buf[1]=='\n')
		ndelims ++;
      }

    rewind (f);

    r = random() % (ndelims + 1);

    i = 0;
    while (!feof(f))
      {
	buf[0]=0;
	fgets (buf, BUFLEN-1, f);
	if (buf[0] > 0)
	  {
	    if (buf[0]=='%' && buf[1]=='\n')
		i ++;
	    else
	      {
		if (i==r)
		  printf ("%s", buf);
	      }  
	  }
      }

    fflush (stdout);
    fclose (f);
  }
#endif	/*  FORTUNE  */



int main (int argc, char *argv[], char *envp[])
  {
    struct termios backup, new;
    char buf[BUFLEN];
    char password[BUFLEN];
    int len;
    struct timeval tp;
    struct passwd *pw;
    char username [BUFLEN];
    char env_home [BUFLEN];
    char env_user [BUFLEN];
    char env_logname [BUFLEN];
    char env_shell [BUFLEN];
    char env_pwd [BUFLEN];


    /*  Login as who?  */
    if (argc < 2)
      {
	printf ("usage: %s username\n", argv[0]);
	exit (1);
      }

    strlcpy (username, argv[1], BUFLEN);

    /*  Randomize...  */
    gettimeofday (&tp, NULL);
    srandom (tp.tv_usec ^ tp.tv_sec);

    sprintf (buf, "Password:");
    write (1, buf, strlen(buf));

    tcgetattr (0, &backup);
    new = backup;
    new.c_lflag &= ~ECHO;
    new.c_lflag |= ECHONL;
    tcsetattr (0, TCSANOW, &new);

    len = read (0, password, BUFLEN-1);
    if (len<0)
      {
	sprintf (buf, "could not read password from stdin\n");
	write (1, buf, strlen(buf));
	exit (1);
      }

    password[len] = 0;
    if (password[strlen(password)-1]=='\n')
	password[strlen(password)-1]=0;

    tcsetattr (0, TCSANOW, &backup);

    pw = getpwnam (username);
    if (!pw)
      {
	printf ("Login incorrect\n");
	exit (1);
      }
    else
      {
	if (!strcmp(pw->pw_passwd, "") && !strcmp(password, ""))
	  {
	    /*  Do nothing...  */
	  }
	else
	  {
	    /*  TODO:  actually check the password  */
	    printf ("%s: password ignored (TODO)\n", argv[0]);
	  }

	setgid (pw->pw_gid);
	setuid (pw->pw_uid);

	if (getuid()!=pw->pw_uid || getgid()!=pw->pw_gid)
	  {
	    printf ("Could not set uid/gid\n");
	    exit (1);
	  }

	if (pw->pw_dir && strlen(pw->pw_dir) > 0)
	  {
	    snprintf (env_home, BUFLEN, "HOME=%s", pw->pw_dir);
	    snprintf (env_user, BUFLEN, "USER=%s", pw->pw_name);
	    snprintf (env_logname, BUFLEN, "LOGNAME=%s", pw->pw_name);
	    snprintf (env_shell, BUFLEN, "SHELL=%s", pw->pw_shell);
	    putenv (env_home);
	    putenv (env_user);
	    putenv (env_logname);
	    putenv (env_shell);

	    if (chdir (pw->pw_dir))
	      printf ("Warning: could not chdir to home directory '%s'\n",
			pw->pw_dir);
	    else
	      {
		snprintf (env_pwd, BUFLEN, "PWD=%s", pw->pw_dir);
		putenv (env_pwd);
	      }
	  }
      }

    endpwent ();


    /*
     *	We're here if the password was correct and we've changed
     *	to the new UID/GID.
     */

    show_uname ();

#ifdef SHOW_MOTD
    cat_etcmotd ();
#endif

#ifdef FORTUNE
    printf ("\n");
    print_random_fortune ("/usr/share/games/fortune/fortunes");
#endif

    printf ("\n");
    fflush (stdout);


    /*  Run the user's shell:  */
/*  TODO: right now we just start /bin/sh  */
{
char *shell_args[2] = { "-sh", NULL };
    execv ("/bin/sh", shell_args);
}

    printf ("login: error executing sh\n");

    return 0;
  }

