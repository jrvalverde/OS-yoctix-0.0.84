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
 *  reg/dma.c  --  machine independant dma registry
 *
 *	Modules which need to use DMA should register with
 *	dma_register().
 *
 *
 *  History:
 *	6 Jan 2000	first version
 */


#include "../config.h"
#include <stdio.h>
#include <sys/std.h>
#include <sys/dma.h>
#include <sys/interrupts.h>



char *dma_owner [MAX_DMA_CHANNELS];



void dma_init ()
  {
    int i;

    for (i=0; i<MAX_DMA_CHANNELS; i++)
	dma_owner [i] = NULL;
  }



int dma_register (int dmanr, char *name)
  {
    /*
     *	Try to register a DMA channel.
     *	Returns 1 on success, 0 on failure.
     */

    int oldints;

    oldints = interrupts (DISABLE);

    if (dma_owner[dmanr])
      {
	interrupts (oldints);
	return 0;
      }

    dma_owner [dmanr] = name;

    interrupts (oldints);
    return 1;
  }



int dma_unregister (int dmanr)
  {
    /*
     *	Unregister a DMA channel.
     *	Returns 1 on success, 0 on failure.
     */

    int oldints;

    oldints = interrupts (DISABLE);

    if (!dma_owner[dmanr])
      {
	interrupts (oldints);
	return 0;
      }

    dma_owner [dmanr] = NULL;

    interrupts (oldints);
    return 1;
  }

