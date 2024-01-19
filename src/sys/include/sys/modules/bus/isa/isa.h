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
 *  sys/modules/bus/isa/isa.h  --  i386 ISA bus
 */

#ifndef	__SYS__MODULES__BUS__ISA__ISA_H
#define	__SYS__MODULES__BUS__ISA__ISA_H


#include <sys/defs.h>



/*
 *  Write a string such as
 *	"module0 at parent0 port 0x1234-0x5678 irq 6 dma 2,5"
 *  to a string at buf, max length buflen.
 */

int isa_module_nametobuf (struct module *m, char *buf, int buflen);



/*
 *  DMA transfer functions:
 *  (Remember to register the DMA channel with dma_register() first.
 *   See sys/dma.h for more information.)
 */


/*  Modes used in isadma_startdma():  */

#define	ISADMA_WRITE		44h
#define	ISADMA_READ		48h

void isadma_stopdma (int channelnr);
int isadma_startdma (int channelnr, void *addr, size_t len, int mode);



#endif	/*  __SYS__MODULES__BUS__ISA__ISA_H  */

