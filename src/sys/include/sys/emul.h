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
 *  sys/emul.h  --  binary emulation support
 */

#ifndef	__SYS__EMUL_H
#define	__SYS__EMUL_H



struct emul
      {
	/*  Pointer to next emulation:  */
	struct emul	*next;

	/*  Emulation "identification":  */
	char		*name;

	/*  Set to 1 when all fields are setup correctly:  */
	int		active;

	/*  check_magic() returns 1 if an exec_header is recognized, 0 otherwise  */
	int		(*check_magic)();		/*  exec_header_buffer, length  */

	/*  loadexec() maps the executable file to a process' vmregion chain
	    (if demand loading is not wanted, loadexec() needs to actually
	    allocate and load the data into physical memory). Returns errno.  */
	int		(*loadexec)();			/*  process, vmobject  */

	/*  Setup stack and hardware registers of a process:  */
	int		(*setupstackandhwregisters)();	/*  process  */

	/*  Setup argv and envp of a process:  */
	int		(*setupargvenvp)();		/*  process, argv, envp  */

	/*  Low-level function to start the process after execve:  */
	int		(*startproc)();			/*  process  */

	/*  syscalls  */
	int		highest_syscall_nr;
	void		**syscall;
      };



struct emul *emul_register (const char *name);
int emul_unregister (struct emul *e);


#endif	/*  __SYS__EMUL_H  */

