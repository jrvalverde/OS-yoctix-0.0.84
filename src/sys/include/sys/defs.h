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
 *  sys/defs.h  --  various type definitions
 */

#ifndef	__SYS__DEFS_H
#define	__SYS__DEFS_H


#define	NULL	0

#define	MAX_NODENAME		64


/*  Machine dependant definitions:  (size_t etc.)  */
#include <sys/md/defs.h>


typedef	s_int8_t	int8_t;
typedef	s_int16_t	int16_t;
typedef	s_int32_t	int32_t;
typedef	s_int64_t	int64_t;
typedef	u_int8_t	u_char;


/*  Maybe some of these could be 64 bits instead ???  */

typedef	u_int32_t	uid_t;
typedef	u_int32_t	gid_t;
typedef	u_int32_t	pid_t;
typedef	u_int32_t	hash_t;


/*  Priority values, nice values etc.  */
typedef	s_int16_t	pri_t;


/*  Clock ticks (lots of 'em per second...)  */
typedef	s_int64_t	ticks_t;


/*  Time:  */
typedef	s_int64_t	time_t;
typedef	s_int32_t	time32_t;


/*  Signal sets:  */
typedef	u_int32_t	sigset_t;


/*  VFS / file types:  */
typedef	s_int64_t	off_t;
typedef	u_int64_t	daddr_t;
typedef	u_int32_t	daddr32_t;
typedef	u_int64_t	inode_t;
typedef	u_int32_t	inode32_t;
typedef	u_int32_t	dev_t;
typedef	u_int16_t	dev16_t;
typedef	u_int16_t	mode_t;


/*  Syscall return type:  */
typedef	s_int64_t	ret_t;

/*  Object reference coutns:  */
typedef	s_int32_t	ref_t;


#endif	/*  __SYS__DEFS_H  */

