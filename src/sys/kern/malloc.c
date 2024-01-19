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
 *  kern/malloc.c  --  the kernel memory allocator
 *
 *	The kernel memory allocator (not to be confused with userland memory
 *	allocators) should handle memory allocation requests of any size at any
 *	time.  Memory is made up of a number of pages (where the size of each
 *	page is machine dependant, for example 4KB on i386).  To know which
 *	pages are in use and which ones are free, we have an array of memory
 *	control blocks (MCBs), where all pages of physical memory (*) are
 *	represented by one MCB each.
 *
 *	(*)  Actually, only pages between malloc_firstaddr and malloc_lastaddr
 *	     are included.  The memory pages between these two addresses
 *	     should be free to use, and should therefore not include any
 *	     hardware reserved pages, or the pages of the kernel image in memory.
 *
 *	malloc_init() must be called before malloc() or free() can be used.
 *	Right now the entire system relies on the fact that the physical
 *	memory is continuous, but it could be modified to allow "holes" too
 *	(by using pages marked as NOTAVAIL in the MCB array).
 *
 *	When the MCB array (always located at the lowest possible address,
 *	malloc_firstaddr) has been initialized, we can now use malloc() and
 *	free().
 *
 *	malloc() allocates kernel memory of any size.  Allocations smaller than
 *	one physical page will cause the bitmap of the MCB to be used.  Larger
 *	allocations (one page or more) will only use full pages, and the bitmap
 *	part of the MCB struct will not be used by malloc().
 *
 *	free() frees kernel memory, by marking the page(s) MCB entry (or
 *	entries) as MCB_FREE.
 *
 *	malloc_showstats() shows some statistics.
 *
 *	malloc_getsize() returns the blocksize of allocated memory at given
 *	address.  This can be used for debugging.
 *
 *
 *  History:
 *	24 Nov 1999	first version
 *	17 Dec 1999	begun on malloc()
 *	18 Dec 1999	continuing on malloc(),
 *			adding malloc_getsize()
 *	3 Jan 2000	more comments
 *	14 Jan 2000	interrupts enable/disable
 *	7 Mar 2000	adding next and prev fields to mcb
 */


#include <stdio.h>
#include <sys/malloc.h>
#include <sys/std.h>
#include <sys/md/machdep.h>	/*  for PAGESIZE  */
#include <sys/interrupts.h>
#include "../config.h"


size_t		malloc_firstaddr;	/*  physical addr of first byte to use  */
size_t		malloc_lastaddr;	/*  physical addr of last byte + 1  */

struct mcb	*first_mcb;		/*  physical addr of first mcb  */
size_t		nr_of_mcbs;		/*  nr of memory control blocks  */

size_t malloc_mcbsearch_pos [NR_OF_MCBSEARCH];


/***   STATISTICAL VARIABLES:   ***/

size_t	malloc_totalfreememory;
size_t	malloc_totalavailablememory;

u_int64_t malloc_totalrequested;	/*  The ratio between these two gives the  */
u_int64_t malloc_totalallocated;	/*  effectiveness (memorywise) of the system  */



void malloc_init ()
  {
    /*
     *	malloc_init ()
     *	--------------
     *
     *	Initialize the MCB array by filling it with sane data. The first
     *	entries in the MCB array represent the pages occupied by the MCB array
     *	itself, because it is always placed first in the available continuous
     *	memory.
     */

    int totpages;
    int mcbsize, mcbpages;
    struct mcb *mcb;
    int i;

    /*  Just check so that we're on page boundaries:  */
    if ((malloc_firstaddr%PAGESIZE) || (malloc_lastaddr%PAGESIZE))
	panic ("malloc_init(): not on page boundary: malloc_firstaddr=0x%x malloc_lastaddr=0x%x",
		malloc_firstaddr, malloc_lastaddr);

    /*  How much total memory do we have? (lastaddr-firstaddr)  (nr of pages)  */
    totpages = (malloc_lastaddr-malloc_firstaddr) / PAGESIZE;

    /*  How much will the MCB chain (memory control blocks) occupy?  (bytes)  */
    mcbsize = totpages * sizeof (struct mcb);

    /*  How many pages will the MCB chain occupy?  */
    mcbpages = mcbsize / PAGESIZE;
    if ((mcbsize % PAGESIZE) > 0)
	mcbpages ++;


    /*
     *	+--  firstaddr			lastaddr ---------+
     *	|						  |
     *	V						  V
     *								where "...."
     *	.... .... .... .... .... .... .... .... .... ....	means one page
     *
     *	The first pages contain the MCB array itself. Those pages are marked as
     *	MCB_RESERVED. Other pages are marked as MCB_FREE. If the MCB array is
     *	large enough to contain information about pages _after_ lastaddr, then
     *	those pages are marked as MCB_NOTAVAILABLE.
     */

    for (i=0; i<(mcbpages*PAGESIZE/sizeof(struct mcb)); i++)
      {
	/*
	 *  Fill the MCB for page i with correct data:
	 */

	/*  Get address of the mcb:  */
	mcb = (struct mcb *) (malloc_firstaddr + i*sizeof (struct mcb));

	/*  Clear the bitmap field:  */
	mcb->bitmap = 0;

	/*  Does this page belong to the mcb itself? Then it is MCB_RESERVED:  */
	if (i < mcbpages)
	    mcb->size = MCB_RESERVED;
	else
	/*  Are we done with all the physically available pages? Then the rest are MCB_NOTAVAILABLE:  */
	if (i >= totpages)
	    mcb->size = MCB_NOTAVAILABLE;
	else
	/*  Otherwise, this is a free (available) page:  */
	    mcb->size = MCB_FREE;
      }


    /*
     *	Setup statistics varibles:
     */

    malloc_totalavailablememory = malloc_lastaddr - malloc_firstaddr - mcbpages*PAGESIZE;
    malloc_totalfreememory = malloc_totalavailablememory;

    malloc_totalrequested = 0;
    malloc_totalallocated = 0;


    /*
     *	Searches for free memory blocks will begin at MCB number 0:
     */

    for (i=0; i<NR_OF_MCBSEARCH; i++)
	malloc_mcbsearch_pos [i] = 0;


    /*
     *	Print nice output on the console:
     */

    printk ("mem: %i KB total, %i KB kernel+reserved, %i KB available",
	malloc_lastaddr/1024, (malloc_lastaddr-malloc_totalavailablememory)/1024,
	malloc_totalavailablememory/1024);


    first_mcb = (struct mcb *) malloc_firstaddr;
    nr_of_mcbs = mcbpages*PAGESIZE/sizeof(struct mcb);
  }



void malloc_showstats ()
  {
    /*  more or less a debugging routine:  */

    struct mcb *mcb;
    int i;
    int nfree = 0, ncont = 0, nres = 0, nnotav = 0;

    mcb = first_mcb;
    for (i=0; i<nr_of_mcbs; i++)
      {
	if (mcb->size == MCB_FREE)		nfree ++;
	if (mcb->size == MCB_CONTINUED)		ncont ++;
	if (mcb->size == MCB_RESERVED)		nres ++;
	if (mcb->size == MCB_NOTAVAILABLE)	nnotav ++;

	mcb++;
      }

    printk ("malloc_showstats():\n\r"
	"  avail mem = %i bytes,  free mem = %i bytes\n\r"
	"  total requested = %i bytes, total allocated = %i bytes\n\r"
	"  nr of MCB_FREE = %i  MCB_CONTINUED = %i\n\r"
	"  MCB_RESERVED = %i  MCB_NOTAVAILABLE = %i"
	"  ==>  nr of MCBs in use = %i",
	(int)malloc_totalavailablememory, (int)malloc_totalfreememory,
	(int)malloc_totalrequested, (int)malloc_totalallocated,
	nfree,ncont,nres,nnotav,
	nr_of_mcbs-nfree-nres-nnotav);

    if (malloc_totalallocated > 0)
	printk ("  requested/allocated = %i%%",
	  (int)(100*(int)malloc_totalrequested/(int)malloc_totalallocated));

/*  TODO:  show nr of allocations using every possible size, eg 128 256 512 ... bytes  */
  }



void *malloc (size_t len)
  {
    /*
     *	malloc ()
     *	---------
     *
     *	The task of malloc() is to find a continuous piece of memory, of size len,
     *	and return a pointer to it. If this fails, NULL is returned.
     *
     *	Finding the piece of memory is done by scanning the MCB array. Allocating
     *	the piece of memory is done by setting the fields of one or more MCB entries.
     */

    size_t bsize, bn;	/*  Size in bytes, and "2-exponential", of block to allocate  */
    size_t tmp;
    struct mcb *mcb;
    int found, foundpos;
    int i;
    int tofind;
    int oldints;
    void *retvalue;

    int mcbbits;	/*  For allocation of small blocks < PAGESIZE  */
    int j;
    int foundbit;


//printk ("malloc (len=%i (0x%x))", len, len);

    if (len==0)
      {
#if DEBUGLEVEL>=2
	printk ("warning: malloc() called with len == 0!");
#endif
	return NULL;
      }


    oldints = interrupts (DISABLE);


    /*  Update statistics:  */
    malloc_totalrequested += len;


    /*  Requested length >= 2 pages?  Then round up to nearest page...  */
    if (len >= 2*PAGESIZE)
      {
	bsize = len/PAGESIZE;
	if (len > bsize*PAGESIZE)
	  bsize++;
	bsize *= PAGESIZE;
      }
    /*  or if the request was for < 2 pages, then round up to the nearest 2^n:  */
    else
      {
	/*  TODO:  FIX THIS! This is slow, and should be done some better way!!!
		A lookup table can be used, but that could also be a stupid waste of memory...  */
	found = 0;
	bsize = PAGESIZE*2;
	while (!found)
	  {
	    if (len > bsize)
		{  bsize *= 2;  found = 1;  }
	    else
	      {
		bsize /= 2;
		if (bsize < MCB_MINBLOCKSIZE)
		  {
		    bsize = MCB_MINBLOCKSIZE;
		    found = 1;
		  }
	      }
	  }
      }


    /*  Convert bsize (in bytes) to 2^n form:  */
    /*  TODO: This is also slow...  */
    found = 0;
    tmp = (1 << (NR_OF_MCBSEARCH-1));
    bn = NR_OF_MCBSEARCH-1;
    while (!found)
      {
	if (bsize >= tmp)
	  found = 1;
	else
	  {
	    tmp /= 2;
	    bn --;
	  }
      }


    /*  for example, if len=2049, then bsize=4096 and bn=12  */
#if DEBUGLEVEL>=4
    printk ("malloc(): requested len = %i, bsize = %i, bn = %i", len,bsize,bn);
#endif


    /*  Now, bn is the index into malloc_mcbsearch_pos[] we should use
	to find the next available free MCB for our bsize!  */

    mcb = first_mcb;
    i = malloc_mcbsearch_pos [bn];


    /*
     *	SCAN FOR A FREE BLOCK OF MEMORY AND ALLOCATE IT
     *
     *	We do blocks of bsize>=PAGESIZE differently from the
     *	small bsize<PAGESIZE blocks.
     */

    if (bsize >= PAGESIZE)
      {
	/*
	 *  We have to find (bsize/PAGESIZE) nr of free
	 *  pages in a row.
	 *
	 *  Begin search at MCB nr i
	 */

	tofind = bsize/PAGESIZE;
	found = 0;
	foundpos = i;

	while (found < tofind)
	  {
	    if (mcb[i].size == MCB_FREE)
	      {
		found++;
		i++;
	      }
	    else
	      {
		/*  a non-free page. we have to start a
			new search...  */
		found = 0;
		i++;
		foundpos = i;
	      }

	    if (i >= nr_of_mcbs)
	      {
		i = 0;
		found = 0;	/*  A block cannot "wrap over", but must be continuous,  */
		foundpos = i;	/*  so we restart at MCB 0 if we reach the end...  */
	      }
	    if (i == malloc_mcbsearch_pos [bn])
	      {
#ifdef MALLOC_PANIC
		  panic ("in malloc(): not enough room for a %i bytes (multi-page) block", bsize);
#endif
		printk ("in malloc(): not enough room for a %i bytes (multi-page) block", bsize);
		interrupts (oldints);
		return NULL;
	      }
	  }

	/*  Update the current search position for blocksize 2^bn:  */
	malloc_mcbsearch_pos [bn] = i;


	/*  Set the status of the found pages to "non-free" by setting the block size:  */
	mcb[foundpos].size = bsize;

	/*  If bsize > PAGESIZE, then we should set the status of the following pages
		to MCB_CONTINUED:  */
	if (bsize > PAGESIZE)
	  {
	    for (i = foundpos+1; i < foundpos + tofind; i++)
		mcb[i].size = MCB_CONTINUED;
	  }


	/*  Return the address of the allocated block:  */
	retvalue = (void *) (malloc_firstaddr + PAGESIZE * foundpos);
      }

    else
    /*  if bsize < PAGESIZE:  */

      {
	/*
	 *  ALLOCATE A BLOCK OF bsize<PAGESIZE
	 *
	 *  What it does:
	 *    o  Search for either a
	 *		1)  free page
	 *	    or  2)  page with the same size as bsize,
	 *		    where the bitmap says there is room
	 *
	 *  Begin the search at MCB nr i
	 */

	/*  How many bits should we check in the MCB bitmap?  */
	mcbbits = PAGESIZE/bsize;

	found = 0;
	foundpos = i;	/*  should not matter  */
	foundbit = 0;

	while (!found)
	  {
	    if (mcb[i].size == MCB_FREE)
		{  found = 1;  foundpos = i;  }
	    else
	    if (mcb[i].size == bsize)
	      {
		/*  Check the MCB bitmap for free space on this page...  */
		for (j=0; j<mcbbits; j++)
		  if ( (mcb[i].bitmap & (1 << j)) == 0 )
		    {
		      foundpos = i;
		      foundbit = j;
		      found = 1;
		      j = mcbbits;  /*  abort the search, since we have now found a free bit  */
		    }
	      }

	    i++;
	    if (i >= nr_of_mcbs)
		i = 0;
	    if (i == malloc_mcbsearch_pos [bn])
	      {
#ifdef MALLOC_PANIC
		  panic ("in malloc(): not enough room for a %i bytes (sub-page) block", bsize);
#endif
		printk ("in malloc(): not enough room for a %i bytes (sub-page) block", bsize);
		interrupts (oldints);
		return NULL;
	      }
	  }

	/*  Update the current search position for blocksize 2^bn:  */
	malloc_mcbsearch_pos [bn] = i;
	/*  ...but if we found a partially filled page, then there's a chance that
	    it will not be filled even by _this_ allocation, and therefore it will be
	    smart to begin the next search for this blocksize from _the same_ position!  */
	malloc_mcbsearch_pos [bn] = foundpos;


	/*  If the page was totally free, then we simply set the size and one
		bit in the bitmap:  */
	if (mcb[foundpos].size == MCB_FREE)
	  {
	    mcb[foundpos].size = bsize;
	    mcb[foundpos].bitmap = 1;	/*  the lowest bit  */
	    retvalue = (void *) (malloc_firstaddr + PAGESIZE * foundpos);
	  }
	else
	/*  ... otherwise we have to mark the foundbit bit:  */
	  {
	    mcb[foundpos].bitmap |= (1 << foundbit);
	    retvalue = (void *) (malloc_firstaddr + PAGESIZE * foundpos + bsize * foundbit);
	  }
/*printk ("  small-block: mcb[%i].size=%i bitmap=0x%x", foundpos, mcb[foundpos].size, mcb[foundpos].bitmap);*/
      }


    /*  Update statistics:  */
    malloc_totalallocated += bsize;
    malloc_totalfreememory -= bsize;

    interrupts (oldints);

    return retvalue;
  }



void free (void *p)
  {
    /*
     *	free ()
     *	-------
     *
     *	Free one or more pages of memory starting at the address "p".
     */

    int i, j, bitnum;
    size_t bsize, boffset;
    int oldints;


    if ((size_t)p < malloc_firstaddr)
	panic ("malloc_getsize() p=0x%x < malloc_firstaddr", (size_t)p);

    oldints = interrupts (DISABLE);

    i = ((size_t)p - malloc_firstaddr) / PAGESIZE;

    bsize = first_mcb[i].size;

    if (bsize == MCB_NOTAVAILABLE  ||  i >= nr_of_mcbs)
	panic ("free(0x%x): trying to free non-existant memory", (size_t)p);

    if (bsize == MCB_FREE)
	panic ("free(0x%x): not allocated", (size_t)p);

    if (bsize == MCB_CONTINUED)
	panic ("free(0x%x): incorrect pointer", (size_t)p);

    if (bsize == MCB_RESERVED)
	panic ("free(0x%x): trying to free reserved memory", (size_t)p);

    if (bsize == MCB_VMOBJECT_PAGE)
	bsize = PAGESIZE;

    /*
     *	If bsize >= PAGESIZE, then we free one or more full pages.
     *	If bsize < PAGESIZE, then we clear one bit in the mcb bitmap,
     *	and ONLY if ALL bits are clear, then we free the full page.
     */

    if (bsize >= PAGESIZE)
      {
	first_mcb[i].size = MCB_FREE;
	first_mcb[i].bitmap = 0;	/*  actually not needed  */

	j = bsize/PAGESIZE;	/*  Nr of pages to free  */
	j = j+i-1;		/*  j = last mcb to free  */
	while (j>i)
	  {
	    if (first_mcb[j].size != MCB_CONTINUED)
		panic ("free(): corrupt MCB chain found");
	    first_mcb[j].size = MCB_FREE;
	    j--;
	  }
      }

    else

      {
	boffset = (size_t)p - (malloc_firstaddr + i*PAGESIZE);
	bitnum = boffset/bsize;
	if (first_mcb[i].bitmap & (1 << bitnum))
	  {
	    /*  Clear the bit:  */
	    first_mcb[i].bitmap = first_mcb[i].bitmap & ~(1<<bitnum);

	    /*  If the entire page is free, then set size to MCB_FREE:  */
	    if (first_mcb[i].bitmap == 0)
		first_mcb[i].size = MCB_FREE;
	  }
	else
	  panic ("free(): trying to free already freed sub-page block");

/*printk ("after free: new size=%i bitmap=%x", first_mcb[i].size, first_mcb[i].bitmap);*/

      }

    malloc_totalfreememory += bsize;

    interrupts (oldints);
  }



size_t malloc_getsize (void *p)
  {
    int i;

    /*  TODO:  should we turn off interrupts?  */

    if ((size_t)p < malloc_firstaddr)
	panic ("malloc_getsize(0x%x): p < malloc_firstaddr", (size_t)p);

    i = ((size_t)p - malloc_firstaddr) / PAGESIZE;
    return first_mcb[i].size;
  }

