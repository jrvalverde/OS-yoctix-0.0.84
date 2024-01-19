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
 *  reg/ports.c  --  registration of ports
 *
 *	NOTE:	Not all architectures may use I/O ports.
 *		Then this stuff is simply not used.
 *
 *	ports_dump()
 *		Debug dump of ports ranges in use
 *
 *	ports_register()
 *		Register a ports range
 *
 *	ports_unregister()
 *		Unregister a ports range
 *
 *  History:
 *	5 Jan 2000	first version
 */


#include "../config.h"
#include <stdio.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/ports.h>
#include <sys/interrupts.h>



/*
 *  Note: there's no lock for portsranges, so interrupts should be
 *  temporarily disabled when using portsranges directly...
 */

struct portsrange *portsranges = NULL;



void ports_init ()
  {
    /*  Nothing.  */
  }



void ports_dump ()
  {
    struct portsrange *p;
    int oldints;

    oldints = interrupts (DISABLE);

    p = portsranges;

    while (p)
      {
	printk ("%x-%x %s", p->baseport, p->baseport+p->nrofports-1, p->owner);
	p = p->next;
      }

    interrupts (oldints);
  }



int ports_register (u_int32_t baseport, u_int32_t nrofports, char *owner)
  {
    /*
     *	ports_register ()
     *	-----------------
     *
     *	Try to register a range of ports.
     *	Returns 1 on success, 0 on failure.
     */

    struct portsrange *p, *newp;
    int conflict;
    u_int32_t pstart, pend, rstart, rend;
    int oldints;


    /*  Go through all portsranges and see if we have a conflict:  */

    oldints = interrupts (DISABLE);
    p = portsranges;
    conflict = 0;

    while (p)
      {
	/*
	 *  Does the requested range interfere with p? If either the starting
	 *  port or the ending port of p is within the requested range, or if
	 *  the starting port of p is before and the ending port of p is after
	 *  the requested range, then we have a conflict.
	 */

	pstart = p->baseport;
	pend = p->baseport + p->nrofports - 1;

	rstart = baseport;
	rend = baseport + nrofports - 1;

	if (pstart >= rstart && pstart <= rend)	conflict = 1;
	if (pend >= rstart && pend <= rend)	conflict = 1;
	if (pstart < rstart && pend > rend)	conflict = 1;

	if (conflict)
	  {
	    interrupts (oldints);
#if DEBUGLEVEL>=2
	    printk ("%s: ports %Y-%Y cannot be registered: conflict with %s (%Y-%Y)",
		owner, rstart, rend, p->owner, pstart, pend);
#endif
	    return 0;
	  }

	p = p->next;
      }


    /*  Ok, now we can register the ports range:  */
    newp = (struct portsrange *) malloc (sizeof(struct portsrange));
    if (!newp)
      {
	printk ("ports_register(): out of memory");
	interrupts (oldints);
	return 0;
      }

    newp->next = NULL;
    newp->owner = owner;
    newp->nrofports = nrofports;
    newp->baseport = baseport;

    if (!portsranges)
	portsranges = newp;
    else
      {
	p = portsranges;
	while (p->next)
		p = p->next;
	p->next = newp;
      }

    interrupts (oldints);
    return 1;
  }



int ports_unregister (u_int32_t baseport)
  {
    /*
     *	ports_unregister ()
     *	-------------------
     *
     *	Unregister a range of ports (only the baseport needs to be
     *	specified).  Returns 1 on success, 0 on failure.
     */

    struct portsrange *p, *last = NULL;
    int oldints;

    oldints = interrupts (DISABLE);

    /*  Find the portsranges entry:  */
    p = portsranges;

    while (p)
      {
	if (p->baseport == baseport)
	  {
	    /*  Free it:  */
	    if (!last)
	      {
		portsranges = p->next;
		interrupts (oldints);
		free (p);
		return 1;
	      }
	    else
	      {
		last->next = p->next;
		interrupts (oldints);
		free (p);
		return 1;
	      }
	  }

	last = p;
	p = p->next;
      }

    interrupts (oldints);
    return 0;
  }


