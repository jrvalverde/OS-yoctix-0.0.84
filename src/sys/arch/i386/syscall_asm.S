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
 *  arch/i386/syscall_asm.S  --  BSD/i386 compatible syscall entry
 */

.text

.globl	_syscall

.globl _arch_syscallasm
	.type   _arch_syscallasm , @function
	.align 5,0x90

	/*
	 *  The stack looks something like this upon entry:
	 *
	 *  +16	old-ss
	 *  +12	old-esp
	 *  +8	old-flags
	 *  +4	old-cs
	 *  +0	old-eip
	 */

_arch_syscallasm:

	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	push	%ds
	push	%es
	push	%fs
	push	%gs

	/*  Set esi to point to userland esp:  */
	movl	56(%esp), %esi

	/*  Reserve a dword on the stack for errno  */
	subl	$4, %esp
	movl	%esp, %ebp

	/*  Get parameters from userland stack:  */
	/*  (except eax which we already have)   */
	movl	40(%esi), %ebx	/*  arg 10  */
	pushl	%ebx
	movl	36(%esi), %ebx	/*  arg 9  */
	pushl	%ebx
	movl	32(%esi), %ebx	/*  arg 8  */
	pushl	%ebx
	movl	28(%esi), %ebx	/*  arg 7  */
	pushl	%ebx
	movl	24(%esi), %ebx	/*  arg 6  */
	pushl	%ebx
	movl	20(%esi), %ebx	/*  arg 5  */
	pushl	%ebx
	movl	16(%esi), %ebx	/*  arg 4  */
	pushl	%ebx
	movl	12(%esi), %ebx	/*  arg 3  */
	pushl	%ebx
	movl	8(%esi), %ebx	/*  arg 2  */
	pushl	%ebx
	movl	4(%esi), %ebx	/*  arg 1  */
	pushl	%ebx

	pushl	%eax
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	popl	%eax

	pushl	%eax		/*  syscall number  */
	pushl	%ebp		/*  address of errno  */

	call	_syscall

	addl	$48, %esp	/*  10 args + syscall nr + errno address =
				    12 arguments, 4 bytes each  */

	popl	%ebx		/*  pop errno  */

	/*  Clear Carry-bit in the return flags:  */
	movl	%esp, %ebp
	add	$52, %ebp

	movl	$0xfffffffe, %ecx
	andl	%ecx, (%ebp)

	cmpl	$0, %ebx	/*  no error?  then let's return the  */
	jz	1f		/*  result from the syscall  */

	/*  Return errno instead of result code  */
	movl	%ebx, %eax

	/*  Set Carry-bit in the return flags:  */
	movl	$1, %ecx
	orl	%ecx, (%ebp)
1:


	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%edx
	popl	%ecx
	popl	%ebx
	addl	$4, %esp		/*  don't overwrite return eax  */


	/*
	 *  TODO:  OpenBSD a.out binary emulation requires fork() to
	 *  set edx to 1 in the child and 0 in the parent. Forcing
	 *  it to be 0 always will work too... but it's really ugly.
	 *
	 *  Generally, we should allow edx to pass through, as this is
	 *  how compilers (at least gcc??) treat 64-bit values...
	 */

	movl $0, %edx

	iret

