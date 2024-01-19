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
 *  sys/mman.h  --  mmap() and other syscalls
 */

#ifndef	__SYS__MMAN_H
#define	__SYS__MMAN_H


/*
 *  Region protection:  (ORed together)
 */

#define	PROT_NONE	0
#define	PROT_READ	1
#define	PROT_WRITE	2
#define	PROT_EXEC	4


/*
 *  Mapping type:  (choose one)
 *  MAP_COPY causes the region to actually be copied by mmap (TODO)
 */

#define	MAP_SHARED	1
#define	MAP_PRIVATE	2
#define	MAP_COPY	4


/*
 *  Other mapping flags:
 *  (Constants choosen to be same as OpenBSD values...)
 */

#define	MAP_FIXED	0x10	/*  TODO  */
#define	MAP_INHERIT	0x80	/*  TODO  */
#define	MAP_NOEXTEND	0x100	/*  TODO  */

#define	MAP_FILE	0
#define	MAP_ANON	0x1000


#define	MAP_FAILED	((void *)-1)


#endif	/*  __SYS__MMAN_H  */

