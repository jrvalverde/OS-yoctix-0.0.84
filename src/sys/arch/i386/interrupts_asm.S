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
 *  arch/i386/interrupt_asm.S
 */


.text
.globl _irq0_asm
	.type _irq0_asm, @function
	.align 5,0x90

_irq0_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20

	push	$0x0
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq1_asm
	.type _irq1_asm, @function
	.align 5,0x90

_irq1_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	push	$0x1
	call	_irq_dispatcher
	addl	$4, %esp

	movb	$0x20, %al
	outb	%al, $0x20

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq2_asm
	.type _irq2_asm, @function
	.align 5,0x90

_irq2_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20

	push	$0x2
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq3_asm
	.type _irq3_asm, @function
	.align 5,0x90

_irq3_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20

	push	$0x3
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq4_asm
	.type _irq4_asm, @function
	.align 5,0x90

_irq4_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20

	push	$0x4
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq5_asm
	.type _irq5_asm, @function
	.align 5,0x90

_irq5_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20

	push	$0x5
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq6_asm
	.type _irq6_asm, @function
	.align 5,0x90

_irq6_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20

	push	$0x6
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq7_asm
	.type _irq7_asm, @function
	.align 5,0x90

_irq7_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20

	push	$0x7
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq8_asm
	.type _irq8_asm, @function
	.align 5,0x90

_irq8_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0x8
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irq9_asm
	.type _irq9_asm, @function
	.align 5,0x90

_irq9_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0x9
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irqa_asm
	.type _irqa_asm, @function
	.align 5,0x90

_irqa_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0xa
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irqb_asm
	.type _irqb_asm, @function
	.align 5,0x90

_irqb_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0xb
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irqc_asm
	.type _irqc_asm, @function
	.align 5,0x90

_irqc_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0xc
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irqd_asm
	.type _irqd_asm, @function
	.align 5,0x90

_irqd_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0xd
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irqe_asm
	.type _irqe_asm, @function
	.align 5,0x90

_irqe_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0xe
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


.globl _irqf_asm
	.type _irqf_asm, @function
	.align 5,0x90

_irqf_asm:
	pushal
	push	%ds
	push	%es
	push	%fs
	push	%gs

	movl	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movb	$0x20, %al
	outb	%al, $0x20
	outb	%al, $0xa0

	push	$0xf
	call	_irq_dispatcher
	addl	$4, %esp

	pop	%gs
	pop	%fs
	pop	%es
	pop	%ds
	popal
	iret


