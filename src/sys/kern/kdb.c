/*
 *  Copyright (C) 2000,2001 by Anders Gavare.  All rights reserved.
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
 *  kern/kdb.c  --  built-in kernel debugger
 *
 *	The built-in kernel debugger is called when the system panics or from
 *	a console driver when a special combination of keys is pressed.
 *	Interrupts are disabled all the time.  In practice, this is just an
 *	infinite loop consisting of the following two steps:
 *
 *		1)  read a command from the console
 *		2)  execute the command
 *
 *	(TODO: what about synching unwritten data? Then interrupts need to
 *	be reenabled... hm...)
 *
 *	The MD part of the kdb system needs to have at least the following:
 *
 *	    struct kdb_command kdb_machdep_cmds []
 *		This array contains command descriptions and pointers to
 *		the machine dependant command handlers. This is of the same
 *		format as the MI kdb_cmds[].
 *
 *	    void kdb_initconsole ()
 *		Performs any neccessary initialization of the console before
 *		reading/writing. For example, if the cursor is not placed at
 *		column zero, then we might want to print a "\n" first.
 *
 *	    void kdb_print (char *string)
 *		Output string (raw text) to the console. "\n" should be
 *		interpreted as "\r\n".
 *
 *	    int kdb_getch ()
 *		Reads a character (blocking) from the console. At least the
 *		following characters must be supported:  letters a-z,
 *		digits 0-9, space, newline/enter/return (something which
 *		return '\n'), and backspace/delete ('\b').
 *
 *  History:
 *	8 Dec 2000	first version, entering of partial command names
 *			supported. (help, reboot, version, continue, mdump)
 */


#include "../config.h"
#include <sys/std.h>
#include <sys/os.h>
#include <sys/module.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <string.h>
#include <stdio.h>
#include <sys/interrupts.h>
#include <sys/lock.h>
#include <sys/md/machdep.h>


extern char *compile_info, *compile_generation;
extern struct module *root_module;
extern struct proc *curproc;
extern struct proc *procqueue [];

#define	KDB_TMP_BUF_LEN		150


struct kdb_command kdb_cmds[] =
      {
	{  "continue",	"Exit the debugger",		NULL /* special */  },
	{  "help",	"Print a help message",		kdb_help  },
	{  "mdump",	"Raw memory dump",		kdb_mdump  },
	{  "modules",	"Print list of modules",	kdb_modules  },
	{  "reboot",	"Force reboot",			kdb_reboot  },
	{  "status",	"Print system status",		kdb_status  },
	{  "version",	"Print OS version",		kdb_version  },
	{  NULL,	NULL,				NULL  }
      };

extern struct kdb_command kdb_machdep_cmds [];



int kdb_findcmdname (char *cmd, int *which_nr, int *which_list)
  {
    /*
     *	Find 'cmd' in kdb_cmds[] or kdb_machdep_cmds[].
     *	Return 1 if an exact match was found, 2 if two or more partials
     *	were found, or 0 if not found at all.
     *
     *	*which_nr is set to the index into the list, *which_list is
     *	set to 0 for the main list, 1 for the machdep list.
     */

    int list, i;
    struct kdb_command *kp;

    *which_nr = -1;
    *which_list = -1;

    for (list=0; list<2; list++)
      {
	kp = (list==0)? kdb_cmds : kdb_machdep_cmds;

	i = 0;
	while (kp[i].name)
	  {
	    if (!strcmp(cmd, kp[i].name))
	      {
		*which_nr = i;
		*which_list = list;
		return 1;
	      }

	    if (!strncmp(cmd, kp[i].name, strlen(cmd)))
	      {
		if (*which_nr != -1)
		  return 2;

		*which_nr = i;
		*which_list = list;
	      }
	    i++;
	  }
      }

    return (*which_nr != -1)? 1 : 0;
  }



void kdb_dumpmem (size_t addr, size_t len, byte *buf)
  {
    /*
     *	Prints a string that looks something like this:
     *
     *	0x12345678 12 34 .. ef Hello World..
     *
     *	addr is the address to display,
     *	len is the number of bytes to display,
     *	and buf is where the data is.
     *
     *	TODO:  this is hardcoded for 32- and 64-bit addresses
     */

    int i;
    char out[150];
    int slen;		/*  cached strlen(out) to speed things up  */

    snprintf (out, sizeof(out), sizeof(size_t)==4? "0x%x " : "0x%X ", addr);

    slen = strlen(out);

    for (i=0; i<len; i++)
      snprintf (out+slen, sizeof(out)-slen, " %y", buf[i]), slen += 3;

    snprintf (out+slen, sizeof(out)-slen, "  "), slen += 2;

    for (i=0; i<len; i++)
      out[slen++] = (buf[i]>=32 && buf[i]<127)? buf[i] : '.';

    snprintf (out+slen, sizeof(out)-slen, "\n");
    kdb_print (out);
  }



/********    MI kdb_* commands    ********/



void kdb_mdump (char *s)
  {
    static size_t cur_ofs = 0;
    int i;

    for (i=0; i<256; i+=16)
      {
	kdb_dumpmem (cur_ofs, 16, (byte *) cur_ofs);
	cur_ofs += 16;
      }
  }



void kdb_modules__Internal (struct module *m, int spaces)
  {
    /*
     *	Print info for module m, and then recurse for all children.
     *	(TODO:  address for m is printed as 32-bit %x)
     */

    struct module *tmpptr;
    char *buf;
    int len, i;

    if (!m)
      return;

    len = spaces+strlen(m->name)+strlen(m->shortname)+20;
    buf = (char *) malloc (len);
    for (i=0; i<spaces; i++)
	buf[i] = ' ';

    snprintf (buf+spaces, len-spaces-1, "%s (%s) 0x%x\n",
	m->shortname, m->name, m);
    kdb_print (buf);

    tmpptr = m->children;
    while (tmpptr)
      {
	kdb_modules__Internal (tmpptr, spaces+4);
	tmpptr = tmpptr->next;
      }

    free (buf);
  }



void kdb_modules (char *s)
  {
    kdb_modules__Internal (root_module, 0);
  }



void kdb_help (char *s)
  {
    int i, t, len;
    char tmpbuf [KDB_TMP_BUF_LEN];

    /*  Max 16 chars command name, 1 space, 62 description = 79 total  */

    i = 0;
    while (kdb_cmds[i].name)
      {
	for (t=0; t<80; t++)
	  tmpbuf[t] = ' ';
	tmpbuf[80] = '\0';

	len = strlen (kdb_cmds[i].name);
	memcpy (tmpbuf, kdb_cmds[i].name, len<16? len : 16);
	snprintf (tmpbuf+17, 62, "%s", kdb_cmds[i].description);
	snprintf (tmpbuf+strlen(tmpbuf), 2, "\n");
	kdb_print (tmpbuf);
	i++;
      }

    i = 0;
    while (kdb_machdep_cmds[i].name)
      {
	for (t=0; t<80; t++)
	  tmpbuf[t] = ' ';
	tmpbuf[80] = '\0';

	len = strlen (kdb_machdep_cmds[i].name);
	memcpy (tmpbuf, kdb_machdep_cmds[i].name, len<16? len : 16);
	snprintf (tmpbuf+17, 63, "%s", kdb_machdep_cmds[i].description);
	snprintf (tmpbuf+strlen(tmpbuf), 2, "\n");
	kdb_print (tmpbuf);
	i++;
      }
  }



void kdb_reboot (char *s)
  {
    machdep_reboot ();
    kdb_print ("kdb: machdep_reboot() failed\n");
  }



void kdb_status (char *s)
  {
    char buf[200];
    int i, firstonqueue;
    struct proc *pp;

    if (curproc)
      {
	snprintf (buf, sizeof(buf), "curproc: 0x%x pid=%i ppid=%i pgid=%i status=%i\n",
		curproc, curproc->pid, curproc->ppid, curproc->pgid, curproc->status);
	kdb_print (buf);

	snprintf (buf, sizeof(buf), "  lock='%s', sleep addr=0x%x, msg='%s'\n",
		curproc->lock.writelock_value, curproc->wchan, curproc->wmesg);
	kdb_print (buf);

	snprintf (buf, sizeof(buf), "  uid=%i gid=%i siglist=0x%x\n",
		curproc->cred.uid, curproc->cred.gid, curproc->siglist);
	kdb_print (buf);
      }

    for (i=0; i<PROC_MAXQUEUES; i++)
      {
	firstonqueue = 1;
	if ((pp = procqueue[i]))
          {
	    while (pp)
	      {
		if (firstonqueue)
		  {
		    snprintf (buf, sizeof(buf), "procqueue[%i]:\n", i);
		    kdb_print (buf);
		    firstonqueue = 0;
		  }

		snprintf (buf, sizeof(buf), "  pid %i (0x%x): ppid=%i status=%i lock='%s' wmesg='%s'\n",
			pp->pid, pp, pp->ppid, pp->status,
			pp->lock.writelock_value, pp->wmesg);
		kdb_print (buf);

		if (pp->next == procqueue[i])
		  pp = NULL;
		else
		  pp = pp->next;
	      }
	  }
      }

  }



void kdb_version (char *s)
  {
    char tmpbuf [KDB_TMP_BUF_LEN];

    snprintf (tmpbuf, KDB_TMP_BUF_LEN,
	OS_NAME"/"OS_ARCH" "OS_VERSION" %s %s\n",
	compile_generation, compile_info);
    kdb_print (tmpbuf);
  }



/********    End of MI kdb_* commands    ********/



void kdb ()
  {
    /*
     *	Built-in kernel debugger
     *	------------------------
     *
     *	Read description at the top of this file.
     */

    char cmdline [KDB_MAXCMDLINE];
    char command [KDB_MAXCMDLEN+1];
    int cmdline_len;
    char c;
    int oldints;
    int i, which_nr, which_list;
    void (*func)();

    oldints = interrupts (DISABLE);
    kdb_initconsole ();

    while (1)
      {
	/*
	 *  Read a command from the console:
	 */

	kdb_print ("kdb> ");

	cmdline_len = 0;
	cmdline[0] = '\0';
	while ( (c=kdb_getch()) != '\n' )
	  {
	    /*  Backspace = remove last char, otherwise add a char...  */
	    if (c=='\b')
	      {
		if (cmdline_len>0)
		  {
		    cmdline[--cmdline_len] = '\0';
		    kdb_print ("\b \b");
		  }
	      }
	    else
	    if (cmdline_len<KDB_MAXCMDLINE-1)
	      {
		cmdline[cmdline_len] = c;
		cmdline[cmdline_len+1] = '\0';
		kdb_print (cmdline+cmdline_len);
		cmdline_len ++;
	      }
	  }

	kdb_print ("\n");


	/*
	 *  Renice cmdline (remove spaces):
	 */

	i = 0;
	while (i<cmdline_len)
	  {
	    if (cmdline[i]==' ' &&
		(i==0 || i==cmdline_len-1 || cmdline[i-1]==' '))
	      memcpy (cmdline+i, cmdline+i+1, (cmdline_len--)-i);
	    else
	      i++;
	  }


	/*
	 *  Execute the command:
	 *
	 *  Find the command in kdb_cmds[] or kdb_machdep_cmds[].
	 *  If an exact match was found, then run that command.
	 *  If _one_ partial match was found, then run that command.
	 */

	i = 0;
	while (i<cmdline_len && i<KDB_MAXCMDLEN && cmdline[i]!=' ')
	  command[i] = cmdline[i++];
	command[i] = '\0';

	if (command[0])
	  {
	    if ((i = kdb_findcmdname (command, &which_nr, &which_list)))
	      {
		if (i==2)
		  kdb_print ("Ambiguous command.\n");
		else
		  {
		    if (which_list==0)
			func = kdb_cmds[which_nr].func;
		    else
			func = kdb_machdep_cmds[which_nr].func;

		    if (!func)
		      {
			kdb_print ("Exiting kdb...\n");
			interrupts (oldints);
			return;
		      }
		    else
		      func (cmdline);
		  }
	      }
	    else
	      kdb_print ("Unknown command. Type 'help' for help.\n");
	  }

      }

  }



