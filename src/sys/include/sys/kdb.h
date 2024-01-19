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
 *  sys/kdb.h  --  header file for the built-in kernel debugger
 *
 *  This file should be included from sys/std.h if KDB is defined.
 */

#ifndef	__SYS__KDB_H
#define	__SYS__KDB_H


#include <sys/defs.h>
#include <sys/md/kdb.h>


#define	KDB_MAXCMDLINE	200
#define	KDB_MAXCMDLEN	16

struct kdb_command
      {
	char	*name;			/*  Command name (short)  */
	char	*description;		/*  One-line description  */
	void	(*func) (char *);
      };

void kdb_dumpmem (size_t addr, size_t len, byte *buf);
void kdb_mdump (char *);
void kdb_modules (char *);
void kdb_help (char *);
void kdb_reboot (char *);
void kdb_status (char *);
void kdb_version (char *);
void kdb ();


#endif	/*  __SYS__KDB_H  */

