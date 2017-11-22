# 如何修改可执行文件（osx平台）
-----------------------------------------

二进制文件很难操作（对人类不友好），可执行的二进制文件更难修改，因为它程序编译后的机器指令和数据资源，具有很强的逻辑性。修改可执行文件需要保证不破坏机器指令和操作数地址。

#### 一、基本思路

使用C源码编译成可执行文件，使用反汇编工具生成汇编码，分析汇编码和二进制文件的对应关系，并修改操作数地址或者数据资源，以改变运行时的跳转逻辑或界面显示。这里我们简单演示函数调用的修改和打印字符的修改。

#### 二、基本步骤

1. 编写C源代码

```
#include <stdio.h>

int max(int a, int b)
{
    return a>=b?a:b;
}

void hello()
{
    printf("hello, world!\n");
}

int main(int argc, char** argv)
{
    printf("%d, %d\n", 1, 2);
    hello();
    max(12, 34);
    return 0;
}

```
上面的代码，包含三个函数，max返回两个数的较大者，hello打印字符串“hello, world!”，main是主函数（系统调用程序的入口）。

2. 使用GCC编译：gcc test.c -o test

3. 使用反汇编工具：otool -tv test 显示汇编码

4. 如果觉得上面的汇编码不完整，也可以使用：gcc -S test.c -o test.s 生成完整的汇编码

```
[1]
test:
(__TEXT,__text) section
_max:
0000000100000ed0	pushq	%rbp
0000000100000ed1	movq	%rsp, %rbp
0000000100000ed4	movl	%edi, -0x4(%rbp)
0000000100000ed7	movl	%esi, -0x8(%rbp)
0000000100000eda	movl	-0x4(%rbp), %esi
0000000100000edd	cmpl	-0x8(%rbp), %esi
0000000100000ee0	jl	0x100000ef1
0000000100000ee6	movl	-0x4(%rbp), %eax
0000000100000ee9	movl	%eax, -0xc(%rbp)
0000000100000eec	jmp	0x100000ef7
0000000100000ef1	movl	-0x8(%rbp), %eax
0000000100000ef4	movl	%eax, -0xc(%rbp)
0000000100000ef7	movl	-0xc(%rbp), %eax
0000000100000efa	popq	%rbp
0000000100000efb	retq
0000000100000efc	nopl	(%rax)
_hello:
0000000100000f00	pushq	%rbp
0000000100000f01	movq	%rsp, %rbp
0000000100000f04	subq	$0x10, %rsp
0000000100000f08	leaq	0x8b(%rip), %rdi
0000000100000f0f	movb	$0x0, %al
0000000100000f11	callq	0x100000f78
0000000100000f16	movl	%eax, -0x4(%rbp)
0000000100000f19	addq	$0x10, %rsp
0000000100000f1d	popq	%rbp
0000000100000f1e	retq
0000000100000f1f	nop
_main:
0000000100000f20	pushq	%rbp
0000000100000f21	movq	%rsp, %rbp
0000000100000f24	subq	$0x20, %rsp
0000000100000f28	leaq	0x7a(%rip), %rax
0000000100000f2f	movl	$0x1, %ecx
0000000100000f34	movl	$0x2, %edx
0000000100000f39	movl	$0x0, -0x4(%rbp)
0000000100000f40	movl	%edi, -0x8(%rbp)
0000000100000f43	movq	%rsi, -0x10(%rbp)
0000000100000f47	movq	%rax, %rdi
0000000100000f4a	movl	%ecx, %esi
0000000100000f4c	movb	$0x0, %al
0000000100000f4e	callq	0x100000f78
0000000100000f53	movl	%eax, -0x14(%rbp)
0000000100000f56	callq	0x100000f00
0000000100000f5b	movl	$0xc, %edi
0000000100000f60	movl	$0x22, %esi
0000000100000f65	callq	0x100000ed0
0000000100000f6a	xorl	%ecx, %ecx
0000000100000f6c	movl	%eax, -0x18(%rbp)
0000000100000f6f	movl	%ecx, %eax
0000000100000f71	addq	$0x20, %rsp
0000000100000f75	popq	%rbp
0000000100000f76	retq


```

完整的汇编码如下：

```
[2]
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


```

5. 使用：vi test 打开二进制文件，并使用:%!xxd子命令格式化显示二进制（%!xxd -r子命令取消格式化）

vi中打开的二进制地址跟[1]中的地址是一致的。

6. 在vi中修改二进制文件

[2]汇编码，阅读时请忽略.开始的汇编器命令，关注核心功能。比如：x86-64的函数调用规范、函数参数传递和返回方法、逻辑地址的计算、字符串数据存储等等。

x86-64的函数调用规范：使用寄存器代替栈进行参数传递（除非寄存器不够用，最多6个寄存器，edi、esi等）；返回值保存在eax寄存器；由函数调用者维护帧栈的申请和清理；函数调用之前需要对之前函数返回值eax进行保存（保存在当前帧栈）；

-4(%rbp)你可以看作一个变量，本质是帧栈底部向上偏移4个字节的操作数。

主函数相较于普通函数多了“movl	$0, -4(%rbp)”这句指令，具体作用还不清楚。

修改数据资源其实很简单，找到对应的位置（特别是字符串）直接修改就行。但是需要满足特殊要求，比如格式化字符串，你不能丢失%d等。修改跳转地址相对麻烦，涉及偏移地址的计算。

callq指令机器码是e8，后面跟4个字节的偏移地址。跳转地址计算是：rip+偏移地址。注意rip中存放的是下一条指令的地址，也就是当前callq指令的下一条指令的地址。如果我们知道了目标函数的地址，就可以根据公式：偏移地址=目标地址-rip的值，得到callq的偏移地址。rip的值和目标函数地址都可以通过otool反汇编得到。

虽然可以修改callq的偏移地址，但是最终的目标地址所表示的函数应该满足当前的参数要求，否则目标函数的运行有可能出现错误。


![机器码](documents/jqm.png)
机器码截图

我们可以找到"hello, world!"字符串，直接修改就行了。

函数的开始是以"5548"开始的，以"5dc3"结束。可以通过机器码看到上图中包含3个函数，正是我们的max、hello、main函数。

第二个函数是hello函数，机器码函数摆放顺序是按照C中函数顺序摆放的。我们可以从hello函数机器码中找到"e862000000"二进制数据。这段翻译过来就是"callq _printf"。其中"62000000"是偏移地址，当前rip地址是"0000000100000f16"，我们就可以得到跳转的目标地址是"0x100000f16+0x62000000=0x100000f78"，这个地址跟otool工具反编译出来的地址是一样的。

同样的道理，我们可以修改main函数中的max调用，使其变成调用hello调用。我们可以找到max调用发生在"0x100000f65"地址，此时rip的值是"0x100000f6a"，我们的目标地址是"0x100000f00"，通过计算可以得到偏移地址是"ffffff96"。由于主机序，我们要写成"96ffffff"。也就是说我们讲"0x100000f65"处的callq(e8)指令的操作数修改成"96ffffff"就可以让程序本来调用max函数转换成调用hello函数。



![修改后的机器码](documents/jqmxg.png)
修改后的机器码截图

![修改后的运行效果](documents/zxxg.png)
修改后的运行效果



#### 三、话语

1. C到汇编是一个层次，汇编到机器码是另外一个层次
2. 当你汇编足够熟悉的时候，看C源码就会联想到汇编代码，看到C隐藏的东西，并且对性能有一定的思考
3. 机器码是最小的，其次是C源码，汇编代码很多，直接修改C或者机器码会容易一些，前提是足够熟悉
4. 程序堆栈和帧栈的栈顶sp是相同的，不同的栈低bp，帧栈是函数调用临时堆栈
5. x86的64位汇编跟32位汇编还是有很多不同之处
6. 想对机器码了解的更深，需要对指令机器码有更多的了解，因为指令的长度是可变的并且不同指令不同的位有不同的含义，x86是复杂指令系统
7. 6502，如果希望更深的了解指令机器码，可以深入研究（6502指令少，并且文档多）

---------------------------------------------

[DOWNLOAD](documents/TestAudioReplay.zip)

