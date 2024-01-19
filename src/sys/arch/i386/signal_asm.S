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
 *  arch/i386/signal_asm.S
 */

.text

.globl _arch_sigtramp_asmcode

	.type   _arch_sigtramp_asmcode , @function
	.align 5,0x90

_arch_sigtramp_asmcode:

	/*
	 *  The registers should be setup as following on entry:
	 *
	 *	eax = signal number
	 *	ebx = signal handler address
	 *	esp = points to A
	 *
	 *  This is how things should be placed in memory:
	 *
	 *	    [  Old user stack, before signal occurs  ]
	 *	B-->[  struct sigcontext to return to after signal handling  ]
	 *	A-->[  arch_sigtramp_asmcode  ]
	 *	    [  stack used by the sigtramp_asmcode and the handler itself  ]
	 *
	 *  Where A is the lowest address of the sigtramp_asmcode!
	 *  So, to reach B, we simply add the size of the sigtramp_asmcode to A...
	 *
	 *  TODO:  I assume that 32 bytes is enough to hold this code,
	 *	this should be changed to something with size=.-archsigblahblah...
	 */


	/*  signal_handler (signal_number);  */

	push	%eax
	call	%ebx
	add	$4, %esp


	/*  sigreturn ((void *)A + sizeof(sigtramp_asmcode));  */

	mov	%esp, %eax
	add	$32, %eax
	push	%eax
	movl	$103, %eax		/*  TODO:  this is openbsd_aout specific!!!  */
	push	%eax
	int	$0x80


	/*  And that's it.  */

