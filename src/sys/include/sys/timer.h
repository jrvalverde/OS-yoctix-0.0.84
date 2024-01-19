/*
 *  Copyright (C) 1999 by Anders Gavare.  All rights reserved.
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
 *  sys/timer.h  --  system timer functions
 */

#ifndef	__SYS__TIMER_H
#define	__SYS__TIMER_H


#include <sys/md/timer.h>

struct proc;
struct timespec;


struct timer_wakeup_chain
      {
	volatile struct timer_wakeup_chain *next;
	void		*wakeup_addr;		/*  address to wakeup()  */
	ticks_t		ticks;			/*  nr of remaining ticks before wakeup  */
	int		flags;			/*  user or kernel mode timer  */
      };

#define	TWC_USER	0
#define	TWC_KERNEL	1


void timer_init ();
void timer ();
time_t time_rawtounix (int year, int month, int day, int hour, int min, int sec);
int timer_sleep (struct proc *p, struct timespec *ts, char *sleepmsg);
int timer_ksleep (struct timespec *ts, void *k_func, int resetflag);

#endif	/*  __SYS__TIMER_H  */

