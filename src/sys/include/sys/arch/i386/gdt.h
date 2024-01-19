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
 *  sys/arch/i386/gdt.h  --  Global Descriptor Table
 */

#ifndef	__SYS__ARCH__I386__GDT_H
#define	__SYS__ARCH__I386__GDT_H


#include <sys/defs.h>


#define	SEL_NULL	0
#define	SEL_CODE	0x0008
#define	SEL_DATA	0x0010
#define	SEL_USERCODE	0x0018
#define	SEL_USERDATA	0x0020
#define	SEL_DUMMYTSS	0x0028
#define	SEL_FIRSTTSS	0x0030

#define	I386_FIRSTTSS	6


#define GDT_DESCTYPE_SYSTEM		0
#define GDT_DESCTYPE_CODEDATA		1
   
#define GDT_SEGTYPE_EXEC		8
#define GDT_SEGTYPE_EXPDOWNCONFORMING	4
#define GDT_SEGTYPE_READ_CODE		2
#define GDT_SEGTYPE_WRITE_DATA		2
#define GDT_SEGTYPE_ACCESSED		1

#define GDT_SEGTYPE_AVAILABLETSS	9


/*  Setup a segment descriptor  */
void i386setgdtentry (void *addr, size_t limit, size_t baseaddr, int segtype, int desctype,
	int dpl, int presentbit, int avl, int opsize, int dbit, int granularity);

/*  Initialize the system gdt, called from machdep_main_startup()  */
void gdt_init ();



#endif	/*  __SYS__ARCH__I386__GDT_H  */

