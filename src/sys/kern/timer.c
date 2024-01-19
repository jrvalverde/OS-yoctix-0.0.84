/*
 *  Copyright (C) 1999,2000 by Anders Gavare.  All rights reserved.
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
 *  kern/timer.c  --  system timers
 *
 *	timer_init()
 *		Initialize the system timers.
 *
 *	timer()
 *		This function is called by machine dependant code on each
 *		hardware clock interrupt. Read below for detailed description.
 *
 *	time_rawtounix()
 *		Can be used by machdep initialization routines to convert
 *		hardware time given in hours, minutes, seconds etc. to Unix
 *		seconds.
 *
 *	timer_sleep()
 *		Add a process to the timer sleep "queue".
 *
 *  History:
 *	28 Dec 1999	first version
 *	15 Jun 2000	adding timer sleep queues
 */


#include "../config.h"
#include <sys/std.h>
#include <sys/defs.h>
#include <sys/interrupts.h>
#include <sys/md/timer.h>
#include <sys/timer.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/errno.h>


/*  This variable (and curproc->[usi]ticks) should be updated HZ times per second  */
volatile ticks_t	system_ticks;

/*  Nr of ticks left until it's time to switch processes:  */
volatile int		ticks_until_pswitch;

/*  Linked list with processes to wake up...  */
volatile struct timer_wakeup_chain *first_twc;

/*  system_time should contain the number of seconds (and nanoseconds) since 1970-01-01:  */
volatile struct timespec system_time;

/*  Nr of ticks left until it's time to update system_time:  */
volatile int		ticks_until_systimeinc;
long			nanosec_tick_length;


/*  *****  TODO: ta bort  */
extern struct mcb      *first_mcb;
extern size_t          nr_of_mcbs;


extern volatile struct proc *curproc;
extern int switchratio;



void timer_init ()
  {
    /*
     *	timer_init ()
     *	-------------
     *
     *	Initialize the system timers.
     */

    system_ticks = 0;
    ticks_until_pswitch = 0;
    ticks_until_systimeinc = 0;
    first_twc = NULL;				/*  timer wakeup chain  */
    nanosec_tick_length = 1000000000 / HZ;

    /*  Machine dependant system timer initialization:  */
    machdep_timer_init ();
  }



void timer ()
  {
    /*
     *	timer ()
     *	--------
     *
     *	This routine is called by the machine dependant timer_asm routine HZ
     *	times per second. What we should do here includes:
     *
     *		(o)  Increase system_ticks
     *
     *		(o)  Increase the correct ticks field
     *			of the current process
     *
     *		(o)  If there are any processes sleeping on the timer,
     *		     then decrease the time remaining on the first
     *		     sleeping process and wake up one or more from the
     *		     timer sleep chain.
     *
     *		(o)  Increase system_time
     *
     *		(o)  Call pswitch() if enough time has passed since
     *		     the last time we switched processes.
     *
     *	Interrupts should be DISABLED by timer_asm before calling this
     *	routine.
     */

    volatile struct timer_wakeup_chain *twentry;
    void (*k_func)();


    /*
     *	Update system_ticks and curproc->ticks:
     */

    system_ticks ++;


/*  TODO: temporary i386 hack to see if we are running at all...  */
{byte *p;p=(byte *)0xb8000;p[158] = (byte)(system_ticks&63)+32;p[159] =15;}


    if (curproc && curproc->ticks)
	(*(curproc->ticks)) ++;


    /*
     *	Check the timer sleep chain, and wake up one or more processes
     *	if they have slept enough:
     */

    if (first_twc)
      {
	twentry = first_twc;

	/*  There may be more than one process to wake up, so we
	    loop for a little while here:  :-)  */
	while (twentry)
	  {
	    if (twentry->ticks <= 0)
	      {
		/*  Wake up the process, or run the kernel function:  */
		if (twentry->flags == TWC_USER)
		  wakeup (twentry->wakeup_addr);
		else
		  {
		    k_func = twentry->wakeup_addr;
		    k_func ();
		  }

		/*  Remove this entry from the wakeup chain:  */
		first_twc = twentry->next;
		free ((struct timer_wakeup_chain *)twentry);

		/*  Continue the loop to try to wake up others...  */
		twentry = first_twc;
	      }
	    else
	      {
		/*  Don't awaken anyone...  */
		twentry->ticks --;
		twentry = NULL;		/*  to abort the while()  */
	      }
	  }
      }


    /*
     *	Advance the system_time (nr of seconds since 1970):
     */

    system_time.tv_nsec += nanosec_tick_length;
    if (--ticks_until_systimeinc <= 0)
      {
	ticks_until_systimeinc = HZ;
	system_time.tv_sec ++;
	system_time.tv_nsec = 0;
      }


    /*
     *	Call pswitch() at regular intervals. switchratio is set by machine
     *	dependant code according to SWITCH_HZ value in include/sys/md/timer.h
     */

    if (--ticks_until_pswitch <= 0)
	pswitch();
  }



time_t time_rawtounix (int year, int month, int day, int hour, int min, int sec)
  {
    time_t t;
    int monthlength[] =
	{  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31  };
    int m, y, days;

    t = sec + 60*min + 60*60*hour + 60*60*24*(day-1);
    days = 0;

    for (y=1970; y<year; y++)
      {
	days += 365;

	/*  Leap year?  */
	if ((y & 3)==0)
	  days++;
      }

    for (m=1; m<month; m++)
      {
	days += monthlength[m-1];

	/*  Leap year, and February?  */
	if ((year & 3)==0 && m==2)
	  days++;
      }

    return t + days*24*60*60;
  }



int timer_sleep__internal (struct proc *p, struct timespec *ts, char *sleepmsg, void *k_func)
  {
    /*
     *	NOTE:  This function assumes that process 'p' is the one we
     *	are currently running. (TODO:  this is not SMP ready)
     *	We add an entry somewhere in the timer wakeup chain, and then
     *	sleep(). It's up to the timer() function to wake us up later.
     *
     *	ts is assumed to contain a valid tv_nsec value.
     *	If p is NULL, then we'll add a kernel mode sleep (with address k_func).
     */

    volatile struct timer_wakeup_chain *newtwentry, *curlink, *lastlink;
    int oldints;
    int added;


    newtwentry = (struct timer_wakeup_chain *) malloc (sizeof(struct timer_wakeup_chain));
    if (!newtwentry)
	return ENOMEM;

    /*  TODO:  this drops accuracy to microseconds instead of nanoseconds:  */
    newtwentry->ticks = ts->tv_sec * HZ +
	(((s_int32_t)ts->tv_nsec/1000) * HZ / 1000000);
    newtwentry->wakeup_addr = (p==NULL)? k_func : &p->ticks;
    newtwentry->next = NULL;
    newtwentry->flags = (p==NULL)? TWC_KERNEL : TWC_USER;

    oldints = interrupts (DISABLE);

    /*  Find a place in the first_twc where we should insert newtwentry:  */
    if (first_twc)
      {
	curlink = first_twc;
	lastlink = NULL;
	added = 0;
	while (curlink)
	  {
	    if (newtwentry->ticks < curlink->ticks)
	      {
		/*  Insert newtwentry before this curlink...  */
		newtwentry->next = curlink;
		curlink->ticks -= newtwentry->ticks;
		if (lastlink)
		    lastlink->next = newtwentry;
		else
		    first_twc = newtwentry;
		curlink = NULL; 	/*  to abort the while loop  */
		added = 1;
	      }
	    else
	      {
		newtwentry->ticks -= curlink->ticks;
		lastlink = curlink;
		curlink = curlink->next;
	      }
	  }
	if (!added)
	  {
	    /*  Add this entry last in the chain:  */
	    if (lastlink)
	      {
		lastlink->next = newtwentry;
	      }
	    else
		panic ("timer_sleep: lastlink==NULL");
	  }
      }
    else
      {
	first_twc = newtwentry;
      }

    if (p)
	sleep (newtwentry->wakeup_addr, sleepmsg);

    interrupts (oldints);
    return 0;
  }



int timer_sleep (struct proc *p, struct timespec *ts, char *sleepmsg)
  {
    return timer_sleep__internal (p, ts, sleepmsg, NULL);
  }



int timer_ksleep (struct timespec *ts, void *k_func, int resetflag)
  {
    /*
     *	Add a kernel timer with address k_func. k_func() will be
     *	started when "ts" time has passed.
     *
     *	If resetflag is non-zero, we first remove any occurances of
     *	k_func() from the timer wakeup chain.
     */

    volatile struct timer_wakeup_chain *curlink, *lastlink, *nextlink;
    int oldints;
    int res;

/*
printk ("ts=%x k_func=%x resetflag=%x", (int)ts, (int)k_func, (int)resetflag);
*/

    oldints = interrupts (DISABLE);

    if (resetflag)
      {
	/*  Remove k_func from the timer wakeup chain:  */
	lastlink = NULL;
	curlink = first_twc;
	while (curlink)
	  {
	    nextlink = curlink -> next;

	    if (curlink->wakeup_addr == k_func)
	      {
		/*  Add ticks from this link to the next link:  */
		if (nextlink)
		  nextlink->ticks += curlink->ticks;

		/*  Link from previous to next:  */
		if (lastlink)
		  lastlink->next = nextlink;
		else
		  first_twc = nextlink;

		free ((struct timer_wakeup_chain *)curlink);
		curlink = nextlink;
	      }
	    else
	      {
		lastlink = curlink;
		curlink = nextlink;
	      }
	  }
      }

    res = timer_sleep__internal (NULL, ts, NULL, k_func);

    interrupts (oldints);
    return res;
  }

