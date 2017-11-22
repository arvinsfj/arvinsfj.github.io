	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 13
	.globl	_max
	.p2align	4, 0x90
_max:                                   ## @max
	.cfi_startproc
## BB#0:
	pushq	%rbp
Lcfi0:
	.cfi_def_cfa_offset 16
Lcfi1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Lcfi2:
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %esi
	cmpl	-8(%rbp), %esi
	jl	LBB0_2
## BB#1:
	movl	-4(%rbp), %eax
	movl	%eax, -12(%rbp)         ## 4-byte Spill
	jmp	LBB0_3
LBB0_2:
	movl	-8(%rbp), %eax
	movl	%eax, -12(%rbp)         ## 4-byte Spill
LBB0_3:
	movl	-12(%rbp), %eax         ## 4-byte Reload
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_hello
	.p2align	4, 0x90
_hello:                                 ## @hello
	.cfi_startproc
## BB#0:
	pushq	%rbp
Lcfi3:
	.cfi_def_cfa_offset 16
Lcfi4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Lcfi5:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	leaq	L_.str(%rip), %rdi
	movb	$0, %al
	callq	_printf
	movl	%eax, -4(%rbp)          ## 4-byte Spill
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## BB#0:
	pushq	%rbp
Lcfi6:
	.cfi_def_cfa_offset 16
Lcfi7:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Lcfi8:
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	leaq	L_.str.1(%rip), %rax
	movl	$1, %ecx
	movl	$2, %edx
	movl	$0, -4(%rbp)
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rax, %rdi
	movl	%ecx, %esi
	movb	$0, %al
	callq	_printf
	movl	%eax, -20(%rbp)         ## 4-byte Spill
	callq	_hello
	movl	$12, %edi
	movl	$34, %esi
	callq	_max
	xorl	%ecx, %ecx
	movl	%eax, -24(%rbp)         ## 4-byte Spill
	movl	%ecx, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"hello, world!\n"

L_.str.1:                               ## @.str.1
	.asciz	"%d, %d\n"


.subsections_via_symbols
