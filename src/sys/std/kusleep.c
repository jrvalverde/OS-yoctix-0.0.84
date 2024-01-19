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
 *  kusleep.c  --  kernel microsecond sleep
 *
 *	NOTE:
 *
 *	The system timer must be running for kusleep() to work!
 *	kusleep() is blocking!
 *
 *  History:
 *	5 Jan 2000	first version
 */


#include "../config.h"
#include <sys/std.h>
#include <sys/defs.h>
#include <sys/timer.h>
#include <sys/interrupts.h>


volatile extern ticks_t system_ticks;


void kusleep (int microseconds)
  {
    ticks_t q, goal;


    /*  Don't sleep if we don't have a timer to count on...  */
    if (!machdep_oldinterrupts())
      {
	printk ("warning: interrupts not enabled; aborting kusleep()");
	return;
      }


/*  TODO:  system_ticks should only be read with interrupts disabled!  */


    q = (int)system_ticks;
    goal = microseconds * (int)HZ / 1000000 + 1;


 /*  TODO:  FIX!  this COULD get stuck in a neverending loop...  */


    while ((int)system_ticks - q < goal)  ;
  }


