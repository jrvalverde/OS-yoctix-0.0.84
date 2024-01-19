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
 *  fcntl.h
 */


#ifndef	__FCNTL_H
#define	__FCNTL_H

#include <sys/defs.h>


/*  Flags for open():  */

#define	O_RDONLY		0x0000		/*  read only   */
#define	O_WRONLY		0x0001		/*  write only  */
#define	O_RDWR			0x0002		/*  read/write  */
#define	O_ACCMODE		0x0003		/*  (mask)      */

#define	O_NONBLOCK		0x0004		/*  nonblocking  */


/*  fcntl() commands:  */
#define	F_DUPFD			0		/*  duplicate a file descriptor  */
#define	F_GETFD			1		/*  get descriptor flag  */
#define	F_SETFD			2		/*  set descriptor flag  */
#define	F_GETFL			3		/*  get file status flag  */
#define	F_SETFL			4		/*  set file status flag  */

/*  fcntl F_GETFD and F_SETFD flags:  */
#define	FD_CLOEXEC		1		/*  close-on-exec  */


#endif	/*  __FCNTL_H  */
