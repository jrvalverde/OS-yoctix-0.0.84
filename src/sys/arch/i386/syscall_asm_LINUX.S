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
 *  arch/i386/syscall_asm.S  --  Linux/i386 compatible syscall entry
 */

.text

.globl	_syscall

.globl _arch_syscallasm
	.type   _arch_syscallasm , @function
	.align 5,0x90

_arch_syscallasm:

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	push	%ds
	push	%es

	pushl	%eax
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	popl	%eax

	/*  Reserve a dword on the stack for errno  */
	subl	$4, %esp
	mov	%esp, %ebp

	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	pushl	%ebp
	call	_syscall
	addl	$28, %esp

	popl	%ebx		/*  pop errno  */

	/*  Clear Carry-bit in the return flags:  */
	movl	%esp, %ebp
	add	$40, %ebp

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

	pop	%es
	pop	%ds
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%edx
	popl	%ecx
	popl	%ebx

	iret

