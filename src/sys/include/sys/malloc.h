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
 *  sys/malloc.h  --  the kernel memory allocator
 */

#ifndef	__SYS__MALLOC_H
#define	__SYS__MALLOC_H

#include <sys/defs.h>


/*
 *  MCB:  Memory Control Block
 *
 *	Directly after the kernel has figured out how much memory it occupies
 *	it will have a long continuous piece of memory (the rest of the memory
 *	in the system). This continuous block is divided into pages. On i386,
 *	these pages are 4KB each.
 *
 *	Each of these pages are represented by an entry in an array, called
 *	the MCB array. The system has one MCB array. This array is used for
 *	two things:
 *
 *		1)  malloc()
 *		    By using the bitmap field of the mcb, the kernel memory
 *		    allocator can allocate blocks as small as PAGESIZE/32 (this
 *		    needs to be fixed when porting to a 64-bit architecture)
 *
 *		2)  vm_object page chains
 *			(should be described better somewhere)
 *
 *  TODO:  this should be 64-bit on 64-bit archs, so
 *	   maybe just using "int" or "long" would be better (?)
 */

struct mcb
    {
	/*
	 *  The size of allocation blocks in this page, or
	 *  a negative value for reserved or special pages.
	 */
	ssize_t		size;

	/*
	 *  For malloc data:     Bitmap of allocated blocks in this page
	 *  For vm_object data:  Page status
	 */
	u_int32_t	bitmap;

	/*
	 *  page_nr is used by vm_object page chains. This number is the
	 *  virtual page number of a page representing the contents of the
	 *  vm_object. It is _NOT_ an mcb index of any kind.
	 */
	size_t		page_nr;

	/*
	 *  The next number is used by vm_object page chains.
	 *  It is an index into the MCB array, pointing to the next page in
	 *  the vm_object page chain.  A value of 0 means the end of a chain,
	 *  since an MCB array index of 0 always points to a reserved page.
	 */
	size_t		next;
    };


#define	MCB_FREE		0
#define	MCB_CONTINUED		-1
#define	MCB_RESERVED		-2
#define	MCB_NOTAVAILABLE	-3
#define	MCB_VMOBJECT_PAGE	-4


/*  We use 32 bits in the MCB bitmap, which gives a
    minimum block size of 128 bytes on i386 with 4KB PAGESIZE:  */

#define	MCB_MINBLOCKSIZE	(PAGESIZE/32)


/*  To speed up searches for MCBs for memory areas of different
    sizes, there is one 'current position' for the search for every
    blocksize, 2^n, where 1 <= n <= NR_OF_MCBSEARCH.  If this is
    set to 16, memory sizes from 1 byte up to 64 KB will be
    found quicker by the search algorithm.  */

#define	NR_OF_MCBSEARCH		16


void malloc_init ();
void malloc_showstats ();
void *malloc (size_t len);
void free (void *p);
size_t malloc_getsize (void *p);


#endif	/*  __SYS__MALLOC_H  */

