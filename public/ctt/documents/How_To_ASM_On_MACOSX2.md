# 如何在OSX平台使用汇编编程2
-----------------------------------------

该篇讲讲macosx平台64位汇编的一些基本结构。

#### 一、基本思路

AT&T汇编是按照节（section）来划分代码的，多个section可以组成二进制里面的segment。程序基本由汇编器指令和机器指令构成。其中对字节对齐、全局函数、标签、cfi汇编器指令、机器指令需要有基本概念。实践的话，使用C编码基本结构，通过gcc编译成汇编文件，两种文件互相对照进行学习。

#### 二、基本步骤

1. hello，asm！

```
	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 13
	.globl	_main
	.p2align	4, 0x90
_main:                                  ## @main
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
	
	leaq	L_.str(%rip), %rax
	movl	$0, -4(%rbp)
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rax, %rdi
	movb	$0, %al
	callq	_printf
	
	xorl	%ecx, %ecx
	movl	%eax, -20(%rbp)         ## 4-byte Spill
	movl	%ecx, %eax
	addq	$32, %rsp
	
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"hello, asm!\n"


	.subsections_via_symbols

```
上面的代码，只包含一个main函数，功能打印"hello，asm!"字符串。请留意汇编的基本结构，64位C调用规范的基本方法（前序、后序），函数参数使用rdi、rsi、rdx...(6个基本入参寄存器，超过6个参数还需要借助栈)，函数返回值使用寄存器rax返回，AT&T汇编的寻址方式，函数调用，函数参数的栈空间分配，参数入栈顺序、资源字符串、寄存器分配（b\w\l\q）、返回值栈存储等。
可以使用：gcc testasm.s -o testasm 生成可执行文件，C源码如下：
```
#include <stdio.h>

int main(int argc, char** argv)
{
	printf("hello, asm!\n");
    return 0;
}
```
使用：gcc -S testasm.c -o testasm.s 可以生成汇编代码。注意printf函数的调用代码块，这个会经常用到（最基本的输出）。
下面使用精简的汇编代码来演示，平台相关的代码参考上面的代码就行了。(otool -tv testasm)

2. main函数参数显示

```
_main:
0000000100000f40	pushq	%rbp
0000000100000f41	movq	%rsp, %rbp

0000000100000f44	subq	$0x20, %rsp

0000000100000f48	leaq	0x53(%rip), %rax
0000000100000f4f	movl	$0x0, -0x4(%rbp)
0000000100000f56	movl	%edi, -0x8(%rbp)
0000000100000f59	movq	%rsi, -0x10(%rbp)

0000000100000f5d	movl	-0x8(%rbp), %esi
0000000100000f60	movq	-0x10(%rbp), %rcx
0000000100000f64	movq	0x8(%rcx), %rdx
0000000100000f68	movq	%rax, %rdi
0000000100000f6b	movb	$0x0, %al
0000000100000f6d	callq	0x100000f80

0000000100000f72	xorl	%esi, %esi
0000000100000f74	movl	%eax, -0x14(%rbp)
0000000100000f77	movl	%esi, %eax
0000000100000f79	addq	$0x20, %rsp

0000000100000f7d	popq	%rbp
0000000100000f7e	retq
```

第1、2句是函数调用规范的前序部分，第3句是分配函数的局部变量（参数、变量等）栈空间，6、7句主要是保存入参到栈，8~13主要是调用printf函数（参数进入寄存器rdi\rsi\rdx），14~17句主要是作函数返回的栈清理工作，18、19句是函数调用规范的后序部分。不管是函数调用的前序、后序、入参、返回值，全部严格遵循64位C的函数调用规范。注意参数的顺序和数组的偏移。

C代码如下：
```
#include <stdio.h>

int main(int argc, char** argv)
{
	printf("argc=%d, argv[1]=%s\n", argc, argv[1]);
    return 0;
}
```

3. 带参数的函数调用

```
_hello:
0000000100000f10	pushq	%rbp
0000000100000f11	movq	%rsp, %rbp

0000000100000f14	subq	$0x20, %rsp

0000000100000f18	leaq	0x87(%rip), %rax
0000000100000f1f	movl	%edi, -0x4(%rbp)
0000000100000f22	movq	%rsi, -0x10(%rbp)

0000000100000f26	movl	-0x4(%rbp), %esi
0000000100000f29	movq	-0x10(%rbp), %rdx
0000000100000f2d	movq	%rax, %rdi
0000000100000f30	movb	$0x0, %al
0000000100000f32	callq	0x100000f84

0000000100000f37	movl	$0x1, %esi
0000000100000f3c	movl	%eax, -0x14(%rbp)
0000000100000f3f	movl	%esi, %eax
0000000100000f41	addq	$0x20, %rsp

0000000100000f45	popq	%rbp
0000000100000f46	retq
0000000100000f47	nopw	(%rax,%rax)
_main:
0000000100000f50	pushq	%rbp
0000000100000f51	movq	%rsp, %rbp

0000000100000f54	subq	$0x20, %rsp

0000000100000f58	movl	$0x0, -0x4(%rbp)
0000000100000f5f	movl	%edi, -0x8(%rbp)
0000000100000f62	movq	%rsi, -0x10(%rbp)

0000000100000f66	movl	-0x8(%rbp), %edi
0000000100000f69	movq	-0x10(%rbp), %rsi
0000000100000f6d	movq	0x8(%rsi), %rsi
0000000100000f71	callq	0x100000f10

0000000100000f76	xorl	%edi, %edi
0000000100000f78	movl	%eax, -0x14(%rbp)
0000000100000f7b	movl	%edi, %eax
0000000100000f7d	addq	$0x20, %rsp

0000000100000f81	popq	%rbp
0000000100000f82	retq
```
上面包含两个函数：main、hello函数，main中调用hello函数。没什么好讲的，遵循调用规范就可以了。注意返回值的栈存储"movl	%eax, -0x14(%rbp)"。栈分配局部变量（函数参数等）是按照16字节对齐的。函数也是按照16字节对齐的，hello函数不足的部分使用0x90填充(nopw指令)。可变参数函数调用暂不考虑，有兴趣的自己去实验。

C代码如下：

```
#include <stdio.h>

int hello(int a, char* b)
{
    printf("a=%d, b=%s\n", a, b);
    return 1;
}

int main(int argc, char** argv)
{
    int flag = hello(argc, argv[1]);
    return 0;
}
```

4. 数据类型和强制转换

```
_hello2:
0000000100000e10	pushq	%rbp
0000000100000e11	movq	%rsp, %rbp

0000000100000e14	subq	$0x50, %rsp

0000000100000e18	leaq	-0x19(%rbp), %rax
#字符数组初始化
0000000100000e1c	movq	0x1dd(%rip), %rcx
0000000100000e23	movq	(%rcx), %rcx
0000000100000e26	movq	%rcx, -0x8(%rbp)
#数组初始化
0000000100000e2a	movl	0x15e(%rip), %edx
0000000100000e30	movl	%edx, -0x19(%rbp)
0000000100000e33	movb	0x158(%rip), %sil
0000000100000e3a	movb	%sil, -0x15(%rbp)
0000000100000e3e	movq	0x14f(%rip), %rcx
0000000100000e45	movq	%rcx, -0x14(%rbp)
0000000100000e49	movl	0x14d(%rip), %edx
0000000100000e4f	movl	%edx, -0xc(%rbp)
#数组赋值
0000000100000e52	movl	$0x4, -0x14(%rbp)
#指针赋值
0000000100000e59	movq	%rax, -0x28(%rbp)
#结构体初始化
0000000100000e5d	movq	0x13c(%rip), %rax
0000000100000e64	movq	%rax, -0x38(%rbp)
0000000100000e68	movl	0x13a(%rip), %edx
0000000100000e6e	movl	%edx, -0x30(%rbp)
#共用体赋值
0000000100000e71	movw	$0x3, -0x40(%rbp)
#枚举赋值
0000000100000e77	movl	$0x4, -0x44(%rbp)

0000000100000e7e	movq	0x17b(%rip), %rax
0000000100000e85	movq	(%rax), %rax
0000000100000e88	movq	-0x8(%rbp), %rcx
0000000100000e8c	cmpq	%rcx, %rax
0000000100000e8f	jne	0x100000e9b

0000000100000e95	addq	$0x50, %rsp

0000000100000e99	popq	%rbp
0000000100000e9a	retq

0000000100000e9b	callq	0x100000f6e


_hello1:
0000000100000ea0	pushq	%rbp
0000000100000ea1	movq	%rsp, %rbp

0000000100000ea4	movb	$0x4, -0x1(%rbp)
0000000100000ea8	movw	$0x3, -0x4(%rbp)
0000000100000eae	movl	$0x5, -0x8(%rbp)
0000000100000eb5	movq	$0x6, -0x10(%rbp)

0000000100000ebd	movb	-0x10(%rbp), %al
0000000100000ec0	movb	%al, -0x1(%rbp)

0000000100000ec3	movsbq	-0x1(%rbp), %rcx
0000000100000ec8	movq	%rcx, -0x10(%rbp)

0000000100000ecc	movl	$0x40900000, -0x14(%rbp)
0000000100000ed3	movabsq	$0x4002666666666666, %rcx
0000000100000edd	movq	%rcx, -0x20(%rbp)

0000000100000ee1	movsd	-0x20(%rbp), %xmm0
0000000100000ee6	cvtsd2ss	%xmm0, %xmm0
0000000100000eea	movss	%xmm0, -0x14(%rbp)

0000000100000eef	movss	-0x14(%rbp), %xmm0
0000000100000ef4	cvtss2sd	%xmm0, %xmm0
0000000100000ef8	movsd	%xmm0, -0x20(%rbp)

0000000100000efd	movswl	-0x4(%rbp), %edx
0000000100000f01	cvtsi2ssl	%edx, %xmm0
0000000100000f05	movss	%xmm0, -0x14(%rbp)

0000000100000f0a	cvttss2si	-0x14(%rbp), %edx
0000000100000f0f	movw	%dx, %si
0000000100000f12	movw	%si, -0x4(%rbp)

0000000100000f16	movswl	-0x4(%rbp), %edx
0000000100000f1a	cvtsi2sdl	%edx, %xmm0
0000000100000f1e	movsd	%xmm0, -0x20(%rbp)

0000000100000f23	movsd	-0x20(%rbp), %xmm0
0000000100000f28	cvttsd2si	%xmm0, %edx
0000000100000f2c	movw	%dx, %si
0000000100000f2f	movw	%si, -0x4(%rbp)

0000000100000f33	movq	-0x10(%rbp), %rcx
0000000100000f37	movb	%cl, %al
0000000100000f39	movsbl	%al, %eax

0000000100000f3c	popq	%rbp
0000000100000f3d	retq
0000000100000f3e	nop
_main:
0000000100000f40	pushq	%rbp
0000000100000f41	movq	%rsp, %rbp

0000000100000f44	subq	$0x20, %rsp

0000000100000f48	movl	$0x0, -0x4(%rbp)
0000000100000f4f	movl	%edi, -0x8(%rbp)
0000000100000f52	movq	%rsi, -0x10(%rbp)

0000000100000f56	callq	0x100000ea0
0000000100000f5b	movsbl	%al, %edi
0000000100000f5e	movl	%edi, -0x14(%rbp)

0000000100000f61	callq	0x100000e10

0000000100000f66	xorl	%eax, %eax
0000000100000f68	addq	$0x20, %rsp

0000000100000f6c	popq	%rbp
0000000100000f6d	retq

#下面是数据资源
	.section	__TEXT,__cstring,cstring_literals
L_hello2.a:                             ## @hello2.a
	.asciz	"asdc"

	.section	__TEXT,__const
	.p2align	2               ## @hello2.b
l_hello2.b:
	.long	1                       ## 0x1
	.long	2                       ## 0x2
	.long	3                       ## 0x3

	.p2align	2               ## @hello2.name1
l_hello2.name1:
	.long	1                       ## 0x1
	.byte	2                       ## 0x2
	.space	3
	.long	1077936128              ## float 3
```
注意数据资源的定义，float是使用.long定义的，并且考虑字节对齐，中间需要空出3字节".space	3"。注意main函数中调用hello1函数返回值的处理（涉及类型转换）。指令"movsbl"是指有符号的从byte传送给long类型。hello1中变量是保存在栈中的，并且按字节对齐分配空间，并且按先后顺序入栈。数据类型的转换使用不同操作数的传送指令即可。浮点类型数据存储和传送都跟整型数据不太一样。float类型存储按照符号位（1位）、阶码位（8位）、位数位（23位）的方式存储，其中阶码使用移位存储的方式存储（原数+127）。movabsq是新增的指令，用来将一个64位的值直接存到一个64位寄存器中。浮点型数据转换需要额外的寄存器和转换指令，在开发中尽量少用。基本数据类型重要体现在字节空间分配和传送指令选择以及寄存器上，字符串采用字符数组形式存储（首地址表示字符串）。hello2函数主要是操作组合类型数据，操作方法跟基本类型差不多，主要也是栈分配存储空间。不同的是组合类型需要先定义模版，指导编译器(链接器)分配栈空间。编译完成后组合类型的定义（模版）都会消失，不存在于可执行二进制文件中。还有就是组合类型变量栈空间分配需要考虑字节对齐。枚举类型变量占用4字节栈空间（int类型）。共用体类型变量占用空间是最大的字段类型所占空间，还需要考虑栈的16字节对齐。结构体类型变量占用空间是每个字段对齐后所占空间的总和。可以将数组理解成字段类型一致的结构体。数组地址是递增的（低地址到高地址存储），函数中的数组变量是存储在栈上的，栈的高地址含有函数的返回地址，所以通过栈缓冲区的溢出可以修改函数的返回地址等栈帧重要信息（缓冲区溢出攻击）。局部变量都是存储在栈上的，可以通过数组的溢出修改其他变量的数值。可以将组合类型定义理解成变量在栈上存储空间的分配模版和访问模板。具有超强的类型拓展性。数据类型如果定义成全局的则分配的空间位在data段或者bss段中，不在栈空间中，具有永久的访问权限（程序运行的整个时期），但是变量类型的作用是一样的，都是指导存储空间的分配。

```
#include <stdio.h>

struct sct1 {
    int a;
    char b;
    float c;
};

union uni1 {
    char a;
    char b;
    short c;
};

enum enum1 {
    wa = 2,
    wb,
    wc,
};

void hello2()
{
    char a[5] = "asdc";
    int b[3] = {1,2,3};
    b[0] = 4;
    void* p = a;
    //
    struct sct1 name1 = {1,2,3};
    union uni1 name2; name2.c = 3;
    enum enum1 name3; name3 = wc;
}

char hello1()
{
    char a = 4; short b = 3; int c = 5; long d = 6;
    a = d; d = a;
    float e = 4.5; double f = 2.3;
    e = f; f = e; e = b; b = e; f = b; b = f;
    
    return d;
}

int main(int argc, char** argv)
{
    int flag = hello1();
    hello2();
    return 0;
}
```

5. 条件判断

```
_hello1:
0000000100000ec0	pushq	%rbp
0000000100000ec1	movq	%rsp, %rbp

0000000100000ec4	movl	$0x2, -0x4(%rbp)
0000000100000ecb	movl	-0x4(%rbp), %eax
0000000100000ece	movl	%eax, %ecx


0000000100000ed0	subl	$0x1, %ecx
0000000100000ed3	movl	%eax, -0x8(%rbp)
0000000100000ed6	movl	%ecx, -0xc(%rbp)
0000000100000ed9	je	0x100000f0c
0000000100000edf	jmp	0x100000ee4

0000000100000ee4	movl	-0x8(%rbp), %eax
0000000100000ee7	subl	$0x2, %eax
0000000100000eea	movl	%eax, -0x10(%rbp)
0000000100000eed	je	0x100000f18
0000000100000ef3	jmp	0x100000ef8

0000000100000ef8	movl	-0x8(%rbp), %eax
0000000100000efb	subl	$0x3, %eax
0000000100000efe	movl	%eax, -0x14(%rbp)
0000000100000f01	je	0x100000f24
0000000100000f07	jmp	0x100000f30

0000000100000f0c	movl	$0x1, -0x4(%rbp)
0000000100000f13	jmp	0x100000f37
0000000100000f18	movl	$0x2, -0x4(%rbp)
0000000100000f1f	jmp	0x100000f37
0000000100000f24	movl	$0x3, -0x4(%rbp)
0000000100000f2b	jmp	0x100000f37

0000000100000f30	movl	$0x4, -0x4(%rbp)


0000000100000f37	popq	%rbp
0000000100000f38	retq
0000000100000f39	nopl	(%rax)
_max:
0000000100000f40	pushq	%rbp
0000000100000f41	movq	%rsp, %rbp
0000000100000f44	movl	%edi, -0x8(%rbp)
0000000100000f47	movl	%esi, -0xc(%rbp)

0000000100000f4a	movl	-0x8(%rbp), %esi
0000000100000f4d	cmpl	-0xc(%rbp), %esi
0000000100000f50	jl	0x100000f61
0000000100000f56	movl	-0x8(%rbp), %eax
0000000100000f59	movl	%eax, -0x4(%rbp)
0000000100000f5c	jmp	0x100000f67
0000000100000f61	movl	-0xc(%rbp), %eax
0000000100000f64	movl	%eax, -0x4(%rbp)

0000000100000f67	movl	-0x4(%rbp), %eax
0000000100000f6a	popq	%rbp
0000000100000f6b	retq
0000000100000f6c	nopl	(%rax)
_main:
0000000100000f70	pushq	%rbp
0000000100000f71	movq	%rsp, %rbp
0000000100000f74	subq	$0x20, %rsp
0000000100000f78	movl	$0xc, %eax
0000000100000f7d	movl	$0x22, %ecx
0000000100000f82	movl	$0x0, -0x4(%rbp)
0000000100000f89	movl	%edi, -0x8(%rbp)
0000000100000f8c	movq	%rsi, -0x10(%rbp)
0000000100000f90	movl	%eax, %edi
0000000100000f92	movl	%ecx, %esi
0000000100000f94	callq	0x100000f40
0000000100000f99	movl	%eax, -0x14(%rbp)
0000000100000f9c	callq	0x100000ec0
0000000100000fa1	xorl	%eax, %eax
0000000100000fa3	addq	$0x20, %rsp
0000000100000fa7	popq	%rbp
0000000100000fa8	retq
```
上面包含main、max和hello函数，条件判断主要在max中5~13句实现。先cmp比较，会使标志寄存器fs中的某些位改变，紧接着使用jl（条件跳转）指令判断fs中的状态，根据状态进行指令跳转，如果为no则继续向下执行正向逻辑，正向逻辑的最后一句必须是jmp指令跳转到公共代码进行执行。jmp指令的下一句是是yes的处理代码也是jl（条件跳转）指令所跳转目标位置。上面就是条件判断的基本汇编结构。cmp（比较）指令、jl（条件跳转）指令、jmp指令构成条件判断的框架。条件嵌套也是这种基本结构的嵌套。C的switch语句实现也是一样。

```
#include <stdio.h>

void hello1()
{
    int a = 2;
    switch (a) {
        case 1:
            //
            a = 1;
            break;
        case 2:
            //
            a = 2;
            break;
        case 3:
            //
            a = 3;
            break;
        default:
            a = 4;
            break;
    }
}

int max(int a, int b)
{
    if (a >= b) {
        return a;
    } else {
        return b;
    }
}

int main(int argc, char** argv)
{
    int flag = max(12, 34);
    hello1();
    return 0;
}
```

6. 条件循环

```
_hello3:
0000000100000eb0	pushq	%rbp
0000000100000eb1	movq	%rsp, %rbp
0000000100000eb4	movl	$0x0, -0x4(%rbp)
0000000100000ebb	movl	$0x3, -0x8(%rbp)

0000000100000ec2	cmpl	$0xa, -0x8(%rbp)
0000000100000ec6	jge	0x100000f06
0000000100000ecc	cmpl	$0x5, -0x8(%rbp)
0000000100000ed0	jne	0x100000edb
0000000100000ed6	jmp	0x100000ef8
0000000100000edb	movl	-0x4(%rbp), %eax
0000000100000ede	addl	$0x1, %eax
0000000100000ee1	movl	%eax, -0x4(%rbp)
0000000100000ee4	cmpl	$0x6, -0x8(%rbp)
0000000100000ee8	jne	0x100000ef3
0000000100000eee	jmp	0x100000f06
0000000100000ef3	jmp	0x100000ef8
0000000100000ef8	movl	-0x8(%rbp), %eax
0000000100000efb	addl	$0x1, %eax
0000000100000efe	movl	%eax, -0x8(%rbp)
0000000100000f01	jmp	0x100000ec2

0000000100000f06	popq	%rbp
0000000100000f07	retq
0000000100000f08	nopl	(%rax,%rax)
_hello2:
0000000100000f10	pushq	%rbp
0000000100000f11	movq	%rsp, %rbp
0000000100000f14	movl	$0x0, -0x4(%rbp)
0000000100000f1b	movl	$0x3, -0x8(%rbp)

0000000100000f22	movl	-0x8(%rbp), %eax
0000000100000f25	addl	$0x1, %eax
0000000100000f28	movl	%eax, -0x8(%rbp)
0000000100000f2b	movl	-0x4(%rbp), %eax
0000000100000f2e	addl	$0x1, %eax
0000000100000f31	movl	%eax, -0x4(%rbp)
0000000100000f34	cmpl	$0xa, -0x8(%rbp)
0000000100000f38	jl	0x100000f22

0000000100000f3e	popq	%rbp
0000000100000f3f	retq
_hello1:
0000000100000f40	pushq	%rbp
0000000100000f41	movq	%rsp, %rbp
0000000100000f44	movl	$0x0, -0x4(%rbp)
0000000100000f4b	movl	$0x3, -0x8(%rbp)

0000000100000f52	cmpl	$0xa, -0x8(%rbp)
0000000100000f56	jge	0x100000f73
0000000100000f5c	movl	-0x8(%rbp), %eax
0000000100000f5f	addl	$0x1, %eax
0000000100000f62	movl	%eax, -0x8(%rbp)
0000000100000f65	movl	-0x4(%rbp), %eax
0000000100000f68	addl	$0x1, %eax
0000000100000f6b	movl	%eax, -0x4(%rbp)
0000000100000f6e	jmp	0x100000f52

0000000100000f73	popq	%rbp
0000000100000f74	retq
0000000100000f75	nopw	%cs:(%rax,%rax)
_main:
0000000100000f80	pushq	%rbp
0000000100000f81	movq	%rsp, %rbp
0000000100000f84	subq	$0x10, %rsp
0000000100000f88	movl	$0x0, -0x4(%rbp)
0000000100000f8f	movl	%edi, -0x8(%rbp)
0000000100000f92	movq	%rsi, -0x10(%rbp)
0000000100000f96	callq	0x100000f40
0000000100000f9b	callq	0x100000f10
0000000100000fa0	callq	0x100000eb0
0000000100000fa5	xorl	%eax, %eax
0000000100000fa7	addq	$0x10, %rsp
0000000100000fab	popq	%rbp
0000000100000fac	retq
```

包含4个函数：main、hello1、hello2、hello3函数。分别是主函数、while循环、do-while循环、for循环。while循环由cmp（比较）指令、jge（条件跳转）指令、jmp指令构成。jge跟jmp之间是循环体执行指令。do-while循环由循环体、cmp（比较）指令、jl（条件跳转）指令构成。循环体在前面。只需要2条特殊指令就可以构成do-while循环。for循环跟while循环是一样的实现。注意for循环里面的continue和break的实现。

```
#include <stdio.h>

void hello3()
{
    int k = 0;
    for (int i = 3; i < 10; i++) {
        if(i == 5) continue;
        k++;
        if(i == 6) break;
    }
}

void hello2()
{
    int k = 0;
    int a = 3;
    do {
        a++;
        k++;
    } while(a < 10);
}

void hello1()
{
    int k = 0;
    int a = 3;
    while (a < 10) {
        a++;
        k++;
    }
}

int main(int argc, char** argv)
{
    hello1();
    hello2();
    hello3();
    return 0;
}
```


7. 位域和typedef

```
_hello1:
0000000100000f60	pushq	%rbp
0000000100000f61	movq	%rsp, %rbp

0000000100000f64	movl	0x4a(%rip), %eax
0000000100000f6a	movl	%eax, -0x8(%rbp)

0000000100000f6d	movb	-0x8(%rbp), %cl
0000000100000f70	andb	$-0x2, %cl
0000000100000f73	orb	$0x1, %cl
0000000100000f76	movb	%cl, -0x8(%rbp)

0000000100000f79	movb	-0x8(%rbp), %cl
0000000100000f7c	andb	$-0x3, %cl
0000000100000f7f	movb	%cl, -0x8(%rbp)

0000000100000f82	popq	%rbp
0000000100000f83	retq
0000000100000f84	nopw	%cs:(%rax,%rax)
_main:
0000000100000f90	pushq	%rbp
0000000100000f91	movq	%rsp, %rbp
0000000100000f94	subq	$0x10, %rsp
0000000100000f98	movl	$0x0, -0x4(%rbp)
0000000100000f9f	movl	%edi, -0x8(%rbp)
0000000100000fa2	movq	%rsi, -0x10(%rbp)
0000000100000fa6	callq	0x100000f60
0000000100000fab	xorl	%eax, %eax
0000000100000fad	addq	$0x10, %rsp
0000000100000fb1	popq	%rbp
0000000100000fb2	retq

#数据资源
	.section	__TEXT,__literal4,4byte_literals
	.p2align	2               ## @hello1.name
L_hello1.name:
	.byte	1                       ## 0x1
	.space	3
```
首先要注意的是数据资源定义，可以看到stc1结构体的初始化只用到了1字节。其余3字节空闲（4字节对齐）。这就是位域对结构体的影响。typedef对实现不产生影响，定义的简写。

```
#include <stdio.h>

typedef unsigned int u_int;

typedef struct {
    u_int width:1;
    u_int height:1;
} stc1;

void hello1()
{
    stc1 name = {1, 8};
    name.width = 3;
    name.height = 6;
}

int main(int argc, char** argv)
{
    hello1();
    return 0;
}
```

8. scanf输入

```
_main:
0000000100000f20	pushq	%rbp
0000000100000f21	movq	%rsp, %rbp

0000000100000f24	subq	$0x20, %rsp

0000000100000f28	leaq	0x79(%rip), %rax
0000000100000f2f	leaq	-0x14(%rbp), %rcx

0000000100000f33	movl	$0x0, -0x4(%rbp)

0000000100000f3a	movl	%edi, -0x8(%rbp)
0000000100000f3d	movq	%rsi, -0x10(%rbp)
0000000100000f41	movl	$0x0, -0x14(%rbp)

0000000100000f48	movq	%rax, %rdi
0000000100000f4b	movq	%rcx, %rsi
0000000100000f4e	movb	$0x0, %al
0000000100000f50	callq	0x100000f7c

0000000100000f55	leaq	0x4c(%rip), %rdi
0000000100000f5c	movl	-0x14(%rbp), %esi
0000000100000f5f	movl	%eax, -0x18(%rbp)
0000000100000f62	movb	$0x0, %al
0000000100000f64	callq	0x100000f76

0000000100000f69	xorl	%esi, %esi
0000000100000f6b	movl	%eax, -0x1c(%rbp)
0000000100000f6e	movl	%esi, %eax
0000000100000f70	addq	$0x20, %rsp

0000000100000f74	popq	%rbp
0000000100000f75	retq
```

注意scanf的调用方法即可。没什么好讲的。

```
#include <stdio.h>

int main(int argc, char** argv)
{
    int f = 0;
    scanf("%d", &f);
    printf("%d", f);
    return 0;
}
```

static 关键字，不管是修饰函数还是变量，代表的是非.globl表示，即对链接器来讲该函数或者变量不可见，只能在本文件内使用，其他文件（模块）不可见，但是存储方式是在data段或者bss段，程序整个运行时期有效。register关键字修饰变量使变量使用寄存器存储和操作（非内存）。extern关键字定义函数或者变量告诉链接器它们在其他文件（模块），不产生实质影响。auto局部变量的默认选项，控制作用域（时间和空间内有效）。

其他方面，比如：标准库函数、错误处理、宏定义、递归、可变参数函数定义\、运算符对应指令、内联汇编等，可以自行实验研究。ps：大多不属于语言本身的知识。汇编中还有些指令是C语言不具备的需要使用内联汇编使用，请自行研究。


OK！大概就这样吧！

---------------------------------------------

END

