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
 *  arch/mac68k/pmap.c
 */


#include "../config.h"
#include <sys/proc.h>
#include <string.h>
#include <sys/malloc.h>
#include <sys/std.h>
#include <sys/errno.h>
#include <sys/vm.h>
#include <sys/interrupts.h>


extern size_t userland_startaddr;


int pmap_mappage (struct proc *p, size_t virtualaddr, byte *a_page,
		u_int32_t region_type)
  {
    return 0;
  }



int pmap_markpages (struct proc *p, size_t startaddr, size_t endaddr, int mask)
  {
    /*
     *	Mark all pages from address startaddr upto (and including)
     *	endaddr of process p according to mask. startaddr and endaddr are
     *	userland addresses.
     *
     *	Return 1 on success, 0 on failure.
     */

    return 1;
  }



int pmap_markpages_cow (struct proc *p, size_t startaddr, size_t endaddr)
  {
    /*
     *	Mark all pages from address startaddr upto (and including)
     *	endaddr of process p as non-writable. startaddr and endaddr are
     *	userland addresses.
     *
     *	Return 1 on success, 0 on failure.
     */

    /*  Remove "write" bit from all pages:  */
/*    return pmap_markpages (p, startaddr, endaddr, 2); */
return 0;
  }



int pmap_unmappages (struct proc *p, size_t startaddr, size_t endaddr)
  {
    /*
     *	Unmap all pages from address startaddr upto (and including)
     *	endaddr by marking them non-readable, non-writable, non-user.
     *
     *	Return 1 on success, 0 on failure.
     */

    /*  Remove all access bits from the pages:  */
/*    return pmap_markpages (p, startaddr, endaddr, 7); */
return 0;
  }



int pmap_mapped (struct proc *p, size_t virtualaddr)
  {
    /*
     *	Return 1 if the virtual address was mapped in process p,
     *	0 if it was not mapped.
     */

    return 0;
  }

