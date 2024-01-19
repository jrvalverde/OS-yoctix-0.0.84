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
 *  reg/emul.c  --  binary emulation registry
 *
 *  History:
 *	18 Feb 2000	first version
 */


#include "../config.h"
#include <stdio.h>
#include <string.h>
#include <sys/std.h>
#include <sys/malloc.h>
#include <sys/emul.h>
#include <sys/interrupts.h>



struct emul *first_emul = NULL;



struct emul *emul_register (const char *name)
  {
    /*
     *	emul_register ()
     *	----------------
     *
     *	Adds an emulation called 'name' to the chain of registered
     *	emulations starting at first_emul.
     *
     *	TODO:  check to see if name is already registered!
     *
     *	This functions returns NULL on failure.
     */

    struct emul *e;
    int oldints;

    e = (struct emul *) malloc (sizeof(struct emul));
    if (!e)
	return NULL;

    memset (e, 0, sizeof (struct emul));
    e->name = (char *) name;


    /*
     *	Add 'name' first in chain. (Interrupts should be temporarily disabled
     *	while first_emul is updated, as there is no lock variable for it.)
     */

    oldints = interrupts (DISABLE);
    e->next = first_emul;
    first_emul = e;
    interrupts (oldints);
    return e;
  }



int emul_unregister (struct emul *e)
  {
    /*
     *	emul_unregister ()
     *	------------------
     *
     *	Unregisters the emulation pointed to by 'e'.
     *	Returns 1 if ok, 0 on error.
     */

    struct emul *tmp, *prev;
    int oldints;

    if (!e)
	return 0;

    /*
     *	There's no lock for first_emul, so we have to disable
     *	interrupts temporarily.
     */

    oldints = interrupts (DISABLE);

    tmp = first_emul;
    if (e == tmp)
      {
	first_emul = e->next;
	interrupts (oldints);
	free (e);
	return 1;
      }

    prev = tmp;
    tmp = tmp->next;

    while (tmp)
      {
	if (tmp==e)
	  {
	    prev->next = e->next;
	    interrupts (oldints);
	    free (e);
	    return 1;
	  }
	prev = tmp;
	tmp = tmp->next;
      }

    interrupts (oldints);
    return 0;
  }

