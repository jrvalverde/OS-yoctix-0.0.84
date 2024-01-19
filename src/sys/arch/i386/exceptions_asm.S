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
 *  arch/i386/exceptions_asm.S  --  i386 low-level exception entry points
 */


.text

.globl _exception0_asm
	.type _exception0_asm, @function
	.align 5,0x90

_exception0_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x0
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception1_asm
	.type _exception1_asm, @function
	.align 5,0x90

_exception1_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x1
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception2_asm
	.type _exception2_asm, @function
	.align 5,0x90

_exception2_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x2
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception3_asm
	.type _exception3_asm, @function
	.align 5,0x90

_exception3_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x3
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception4_asm
	.type _exception4_asm, @function
	.align 5,0x90

_exception4_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x4
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception5_asm
	.type _exception5_asm, @function
	.align 5,0x90

_exception5_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x5
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception6_asm
	.type _exception6_asm, @function
	.align 5,0x90

_exception6_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x6
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception7_asm
	.type _exception7_asm, @function
	.align 5,0x90

_exception7_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x7
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception8_asm
	.type _exception8_asm, @function
	.align 5,0x90

_exception8_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x8
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception9_asm
	.type _exception9_asm, @function
	.align 5,0x90

_exception9_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$9
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception10_asm
	.type _exception10_asm, @function
	.align 5,0x90

_exception10_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$10
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception11_asm
	.type _exception11_asm, @function
	.align 5,0x90

_exception11_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$11
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception12_asm
	.type _exception12_asm, @function
	.align 5,0x90

_exception12_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$12
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception13_asm
	.type _exception13_asm, @function
	.align 5,0x90

_exception13_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$13
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception14_asm
	.type _exception14_asm, @function
	.align 5,0x90

_exception14_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movl	%esp, %ebx
	addl	$48, %ebx
	movl	(%ebx), %eax
	pushl	%eax
	movl	%cr2, %eax
	pushl	%eax
	call	_machdep_pagefaulthandler
	addl	$8, %esp
	movl	%eax, %cr3
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	addl	$4, %esp
	iret


.globl _exception15_asm
	.type _exception15_asm, @function
	.align 5,0x90

_exception15_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$15
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception16_asm
	.type _exception16_asm, @function
	.align 5,0x90

_exception16_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$16
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exception17_asm
	.type _exception17_asm, @function
	.align 5,0x90

_exception17_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$17
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exceptionreserved_asm
	.type _exceptionreserved_asm, @function
	.align 4,0x90

_exceptionreserved_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x100
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _exceptioninvalid_asm
	.type _exceptioninvalid_asm, @function
	.align 4,0x90

_exceptioninvalid_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs
	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	$0x101
	call	_machdep_exceptionhandler
	addl	$4, %esp
	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


