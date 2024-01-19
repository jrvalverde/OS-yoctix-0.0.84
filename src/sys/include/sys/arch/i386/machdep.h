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
 *  sys/arch/i386/machdep.h  --  machine dependant definitions
 */

#ifndef	__SYS__ARCH__I386__MACHDEP_H
#define	__SYS__ARCH__I386__MACHDEP_H


#include <sys/defs.h>


#define	PAGESIZE		4096

#define	KSTACK_SIZE		PAGESIZE*2
#define	KSTACK_MARGIN		32


/*  Physical address of the kernel's page directory:  */
#define	KERNPDIRADDR		0x10000


#define	MAX_IRQS		16
#define	MAX_DMA_CHANNELS	8


/*
 *  Machine dependant functions:
 */


/*  First thing started by init_main():  */
int machdep_main_startup ();

/*  Allocate conventional ("low") memory: (not free:able)  */
void *i386_lowalloc (size_t len);

/*  Return the amount of free conventional memory:  */
size_t i386_lowfree ();

/*  initialize i386 exception handlers  */
void i386exceptions_init ();

/*  Machine dependant resource initialization:  */
int machdep_res_init ();

void i386_ltr (int nr);

/*  cli and sti stuff:  */
void machdep_interrupts (int);
int machdep_oldinterrupts ();

void bios_init ();
int cmos_read (int pos);
void cmos_write (int pos, int value);
int bcd_to_int (int);
void machdep_reboot ();
void machdep_halt ();


#endif	/*  __SYS__ARCH__I386__MACHDEP_H  */

