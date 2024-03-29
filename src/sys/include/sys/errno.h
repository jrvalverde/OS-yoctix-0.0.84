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
 *  sys/errno.h  --  error numbers
 */

#ifndef	__SYS__ERRNO_H
#define	__SYS__ERRNO_H


#define	EPERM		1	/*  Permission denied (for the syscall)  */
#define	ENOENT		2	/*  No such file or directory  */
#define	ESRCH		3	/*  No such process  */
#define	EINTR		4	/*  Interrupted system call  */
#define	EIO		5	/*  Input/output error  */
#define	ENXIO		6	/*  Device not configured  */
#define	E2BIG		7	/*  Argument list too long  */
#define	ENOEXEC		8	/*  Exec format error  */
#define	EBADF		9	/*  Bad file descriptor  */
#define	ECHILD		10	/*  No child process  */
#define	EDEADLK		11	/*  Resource deadlock avoided  */
#define	ENOMEM		12	/*  Cannot allocate memory  */
#define	EACCES		13	/*  Permission denied  */
#define	EFAULT		14	/*  Bad address  */
#define	ENOTBLK		15	/*  Block device required  */
#define	EBUSY		16	/*  Device busy  */
#define	EEXIST		17	/*  File exists  */
#define	EXDEV		18	/*  Cross-device link  */
#define	ENODEV		19	/*  Operation not supported by device  */
#define	ENOTDIR		20	/*  Not a directory  */
#define	EISDIR		21	/*  Is a directory  */
#define	EINVAL		22	/*  Invalid argument  */
#define	ENFILE		23	/*  Too many open files in system  */
#define	EMFILE		24	/*  Too many open files  */
#define	ENOTTY		25	/*  Inappropriate ioctl for device  */
#define	ETXTBSY		26	/*  Text file busy  */
#define	EFBIG		27	/*  File too large  */
#define	ENOSPC		28	/*  No space left on device  */
#define	ESPIPE		29	/*  Illegal seek  */
#define	EROFS		30	/*  Read-only file system  */
#define	EMLINK		31	/*  Too many links  */
#define	EPIPE		32	/*  Broken pipe  */

#define	EAGAIN		35	/*  Resource temporarily unavailable  */

#define	EOPNOTSUPP	45	/*  Operation not supported  */

#define	ENOSYS		78	/*  Function not implemented  */

#endif	/*  __SYS__ERRNO_H  */

