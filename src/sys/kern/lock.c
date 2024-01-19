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
 *  kern/lock.c  --  resource locking
 *
 *
 *  History:
 *	14 Apr 2000	test
 *	7 Jun 2000	if DEBUGLEVEL>5 then we printk lock values...
 *	13 Dec 2000	combined readonly/readwrite locks (lockstruct)
 */


#include "../config.h"
#include <sys/std.h>
#include <string.h>
#include <sys/interrupts.h>
#include <sys/proc.h>
#include <sys/lock.h>



int lock (struct lockstruct *lockaddr, char *lockvalue, int flags)
  {
    /*
     *	lock ()
     *	-------
     *
     *	Lock a kernel resource.
     *
     *	Return values are:
     *		0    lock ok
     *		-1   could not lock, and blocking==0
     *
     *	TODO: Is it better tojust set need_to_pswitch than to do a pswitch()
     *	call manually?
     */

    int oldints;
    int writeflag, blocking;

    oldints = interrupts (DISABLE);

#if DEBUGLEVEL>=5
    printk ("lock   [%s] old='%s'", lockvalue, lockaddr->writelock_value);
#endif

    if (oldints == DISABLE && !(flags & LOCK_NOINTERRUPTS))
      panic ("lock(): called while interrupts disabled, lockvalue='%s'",
		lockvalue);

    blocking = (flags & LOCK_BLOCKING)? 1 : 0;
    writeflag = (flags & LOCK_RW)? 1 : 0;


    /*  Resource not locked? Then lock it and return successfully:  */
    if (!(lockaddr->writelock_value))
      {
	if (!writeflag || lockaddr->readlock_refcount==0)
	  goto lock_and_return;
      }


    /*
     *	We are here if the resource was locked by someone else...
     *
     *	If the caller didn't use the LOCK_BLOCKING flag, then return -1
     *	immediately. Otherwise, let's loop forever until we get the lock:
     */

    if (!blocking)
      {
	interrupts (oldints);
	return -1;
      }

    while (1)
      {
int i=0;

	interrupts (ENABLE);
//	sleep ((void *)lockaddr, "lock");

i++;
if (i==2000)
  {
    printk ("waiting for 0x%x (%s)", lockaddr, lockvalue);
  }

	interrupts (DISABLE);

	if (!(lockaddr->writelock_value))
	  {
	    if (!writeflag || lockaddr->readlock_refcount==0)
	      goto lock_and_return;
	  }
      }

    panic ("lock: not reached");


lock_and_return:

    if (writeflag)
      lockaddr->writelock_value = lockvalue;
    else
      lockaddr->readlock_refcount ++;

    interrupts (oldints);
    return 0;
  }



int unlock (struct lockstruct *lockaddr)
  {
    /*
     *	unlock ()
     *	---------
     *
     *	Returns 0 on success, or -1 if the resource was NOT locked.
     */

    int oldints;

    if (!lockaddr)
      return -1;

    oldints = interrupts (DISABLE);

    /*  Write lock?  */
    if (lockaddr->writelock_value)
      {
#if DEBUGLEVEL>=5
	printk ("unlock [%s]", lockaddr->writelock_value);
#endif
	/*  A resource locked for write cannot have read locks too:  */
	if (lockaddr->readlock_refcount > 0)
	  panic ("unlock: lock struct is inconsistent (%s)",
		lockaddr->writelock_value);

	lockaddr->writelock_value = NULL;
	interrupts (oldints);
	return 0;
      }

    /*  Read lock?  */
    if (lockaddr->readlock_refcount == 0)
      {
#if DEBUGLEVEL>=2
	printk ("unlock: trying to unlock non-locked resource "
		"(refcount problems)");
#endif
	interrupts (oldints);
	return -1;
      }

    /*  Yes, it was a read lock:  */
    lockaddr->readlock_refcount --;

    interrupts (oldints);

    /*  TODO: This wakeup should really be a "wakeup_one" or something:  */
    wakeup ((void *)lockaddr);
    return 0;
  }

