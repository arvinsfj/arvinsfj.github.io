# 如何在OSX平台使用汇编编程
-----------------------------------------

汇编在逆向工程领域有很重要的作用，看懂汇编就能理解软件逻辑。macosx使用AT&T汇编语法格式，并且是64位汇编（32位也可以，链接库不一样）。

#### 一、基本思路

AT&T汇编是按照节（section）来划分代码的，多个section可以组成二进制里面的segment。程序基本由汇编器指令和机器指令构成。其中对字节对齐、全局函数、标签、cfi汇编器指令、机器指令需要有基本概念。

#### 二、基本步骤

1. hello，world！

```
	.section __TEXT,__text,regular,pure_instructions
	.macosx_version_min 10,13

	.globl _main
	.p2align 4,0x90
_main:
	.cfi_startproc
pushq %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
movq %rsp, %rbp
	.cfi_def_cfa_register %rbp
subq $16, %rsp
movl $0, -4(%rbp)

leaq L_.str(%rip), %rdi
movb $0, %al
callq _printf

xorl %eax, %eax
addq $16, %rsp
popq %rbp
retq
	.cfi_endproc

	.section __TEXT,__cstring,cstring_literals
L_.str:
	.asciz "hello, world!\n"

	.subsections_via_symbols

```
上面的代码，只包含一个main函数，功能打印"hello，world!"字符串。
可以使用：gcc test.s -o test 生成可执行文件

2. 简单函数调用

```
	.section __TEXT,__text,regular,pure_instructions
	.macosx_version_min 10,13

	.globl _hello
	.p2align 4, 0x90
_hello:
	.cfi_startproc
pushq %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
movq %rsp, %rbp
	.cfi_def_cfa_register %rbp
subq $16, %rsp
leaq L_.str(%rip), %rdi
movb $0, %al
callq _printf
movl %eax, -4(%rbp)
addq $16, %rsp
popq %rbp
retq
	.cfi_endproc

	.globl _main
	.p2align 4,0x90
_main:
	.cfi_startproc
pushq %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
movq %rsp, %rbp
	.cfi_def_cfa_register %rbp
subq $16, %rsp
movl $0, -4(%rbp)

leaq L_.str(%rip), %rdi
movb $0, %al
callq _printf

callq _hello

xorl %eax, %eax
addq $16, %rsp
popq %rbp
retq
	.cfi_endproc

	.section __TEXT,__cstring,cstring_literals
L_.str:
	.asciz "hello, world!\n"

	.subsections_via_symbols
```

上面代码添加了无参数的hello函数，并且在main中调用hello函数。

3. 带参数的函数调用

```
	.section __TEXT,__text,regular,pure_instructions
	.macosx_version_min 10,13

	.globl _hello1
	.p2align 4, 0x90
_hello1:
	.cfi_startproc
pushq %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
movq %rsp, %rbp
	.cfi_def_cfa_register %rbp
movl %esi, %edx
movl %edi, %esi
leaq L_.str1(%rip), %rdi
movb $0, %al
callq _printf
popq %rbp
retq
	.cfi_endproc

	.globl _hello
	.p2align 4, 0x90
_hello:
	.cfi_startproc
pushq %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
movq %rsp, %rbp
	.cfi_def_cfa_register %rbp
subq $16, %rsp
leaq L_.str(%rip), %rdi
movb $0, %al
callq _printf
movl %eax, -4(%rbp)
addq $16, %rsp
popq %rbp
retq
	.cfi_endproc

	.globl _main
	.p2align 4,0x90
_main:
	.cfi_startproc
pushq %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
movq %rsp, %rbp
	.cfi_def_cfa_register %rbp
subq $16, %rsp
movl $0, -4(%rbp)

leaq L_.str(%rip), %rdi
movb $0, %al
callq _printf

callq _hello

movl    $12, %edi
movl    $34, %esi
callq    _hello1

xorl %eax, %eax
addq $16, %rsp
popq %rbp
retq
	.cfi_endproc

	.section __TEXT,__cstring,cstring_literals
L_.str:
	.asciz "hello, world!\n"
L_.str1:
	.asciz "a=%d,b=%d\n"

	.subsections_via_symbols
```

下面是异常的函数调用（hacking）：

```
.section __TEXT,__text,regular,pure_instructions
.macosx_version_min 10,13

.globl _hello1
.p2align 4, 0x90
_hello1:
.cfi_startproc
pushq %rbp
.cfi_def_cfa_offset 16
.cfi_offset %rbp, -16
movq %rsp, %rbp
.cfi_def_cfa_register %rbp
subq $16, %rsp
leaq L_.str1(%rip), %rax
movl %edi, -4(%rbp)
movl %esi, -8(%rbp)
movl -4(%rbp), %esi
movl -8(%rbp), %edx
movq %rax, %rdi
movb $0, %al
callq _printf
leaq _hello(%rip), %rax
movl %eax, 8(%rbp)
movl %eax, -12(%rbp)
addq $16, %rsp
popq %rbp
retq
.cfi_endproc

.globl _hello
.p2align 4, 0x90
_hello:
.cfi_startproc
leaq _goto(%rip), %rax
pushq %rax
pushq %rbp
.cfi_def_cfa_offset 16
.cfi_offset %rbp, -16
movq %rsp, %rbp
.cfi_def_cfa_register %rbp
subq $16, %rsp
leaq L_.str(%rip), %rdi
movb $0, %al
callq _printf
movl %eax, -4(%rbp)
addq $16, %rsp
popq %rbp
retq
.cfi_endproc

.globl _main
.p2align 4,0x90
_main:
.cfi_startproc
pushq %rbp
.cfi_def_cfa_offset 16
.cfi_offset %rbp, -16
movq %rsp, %rbp
.cfi_def_cfa_register %rbp
subq $16, %rsp
movl $0, -4(%rbp)

leaq L_.str(%rip), %rdi
movb $0, %al
callq _printf

movl    $12, %edi
movl    $34, %esi
callq    _hello1

_goto:
xorl %eax, %eax
addq $16, %rsp
popq %rbp
retq
.cfi_endproc

.section __TEXT,__cstring,cstring_literals
L_.str:
.asciz "hello, world!\n"
L_.str1:
.asciz "a=%d,b=%d\n"

.subsections_via_symbols

```
上面的代码请自行阅读和理解。ps：将retq指令当成callq指令进行使用（类似缓存区溢出）。

```
#include <stdio.h>

void hello_arr1(void)
{
    int a = 0;
    char arr[1] = {1};
    gets(arr);
	printf("a=%d\n", a);
}

void hello_arr21(void)
{
    printf("hello, world! arr21\n");
}

void hello_arr2(void)
{
    int arr[1];
    arr[3] = hello_arr21;
}

int main(void)
{
    hello_arr1();
    hello_arr2();
}
```
上面的代码输出是什么？

```
	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 13
	.globl	_hello_arr1
	.p2align	4, 0x90
_hello_arr1:                            ## @hello_arr1
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
	subq	$32, %rsp
	leaq	-5(%rbp), %rdi
	movl	$0, -4(%rbp)
	movb	l_hello_arr1.arr(%rip), %al
	movb	%al, -5(%rbp)
	callq	_gets
	leaq	L_.str(%rip), %rdi
	movl	-4(%rbp), %esi
	movq	%rax, -16(%rbp)         ## 8-byte Spill
	movb	$0, %al
	callq	_printf
	movl	%eax, -20(%rbp)         ## 4-byte Spill
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_hello_arr21
	.p2align	4, 0x90
_hello_arr21:                           ## @hello_arr21
	.cfi_startproc
## BB#0:
    leaq    _hello(%rip), %rax
    pushq   %rax
	pushq	%rbp
Lcfi3:
	.cfi_def_cfa_offset 16
Lcfi4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Lcfi5:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	leaq	L_.str.1(%rip), %rdi
	movb	$0, %al
	callq	_printf
	movl	%eax, -4(%rbp)          ## 4-byte Spill
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_hello_arr2
	.p2align	4, 0x90
_hello_arr2:                            ## @hello_arr2
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
	leaq	_hello_arr21(%rip), %rax
	movl	%eax, %ecx
	movl	%ecx, 8(%rbp)
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## BB#0:
	pushq	%rbp
Lcfi9:
	.cfi_def_cfa_offset 16
Lcfi10:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Lcfi11:
	.cfi_def_cfa_register %rbp
	callq	_hello_arr1
	callq	_hello_arr2
_hello:
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__const
l_hello_arr1.arr:                       ## @hello_arr1.arr
	.byte	1

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"a=%d\n"

L_.str.1:                               ## @.str.1
	.asciz	"hello, world! arr21\n"


.subsections_via_symbols
```

上面的代码呢？


---------------------------------------------

END

