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
 *  std/random.c  --  random number generator
 *
 *	NOTE:	This must be rewritten, but right now I don't have time
 *		do dive into the science of pseudo-random numbers...
 *
 *		Why have this here if it is not fully functional? Well,
 *		actually, it works but the randomness is very very low...
 *
 *  History:
 *	15 Apr 2000	test
 */


#include "../config.h"
#include <sys/defs.h>
#include <sys/std.h>
#include <sys/interrupts.h>



u_int64_t random_seed;



void srandom (u_int64_t seed)
  {
    /*
     *	Set the random seed.  (Note that interrupts need to be disabled,
     *	since otherwise we cannot be sure that random_seed is updated
     *	correctly on all machines.)
     */

    int oldints;

    oldints = interrupts (DISABLE);
    random_seed = seed;
    interrupts (oldints);
  }



int random ()
  {
    /*  This is not random...  TODO  */

    int oldints;
    int tmp;

    oldints = interrupts (DISABLE);
    tmp = (random_seed & 0xffffffff) ^ ((random_seed >> 32) & 0xffffffff);

    tmp %= 4711;
    tmp ^= 0xc0debabe;
    tmp -= random_seed;
    tmp *= 171;
    tmp += random_seed;

    random_seed ^= tmp;
    random_seed <<= 1;

    interrupts (oldints);
    return tmp;
  }

