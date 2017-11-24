# 变量自加++（自减--）如何区分左加和右加
-----------------------------------------

很多同学在学习C的时候对于++运算符在变量左边和右边，如何参与运算的，如何影响运算结果，都非常模糊。先给出结论：++在变量右边不影响表达式的结果，等价于去掉++进行表达式运算，只会影响变量本身自加1；++在变量左边是需要先自加并且自加1的结果参与表达式的后续运算。

#### 一、基本思路

1. 编写相关的C函数（包含++的表达式）

2. 使用命令：gcc -o testc testc.c 编译成可执行文件

3. 使用命令：otool -tv testc > testc.txt 导出反编译的汇编码

4. 比较++在变量左边和右边的汇编码区别来理解不同写法的具体含义

#### 二、结果

```
int test3()
{
    int a = 0, k = 0;
    k = a++;
    k = ++a;
    return k;
}

```

对应的汇编码如下：

```
_test3:
0000000100000e60	pushq	%rbp
0000000100000e61	movq	%rsp, %rbp

0000000100000e64	movl	$0x0, -0x4(%rbp)
0000000100000e6b	movl	$0x0, -0x8(%rbp)

//k = a++; 共5行，引入了额外的临时变量%ecx
0000000100000e72	movl	-0x4(%rbp), %eax
0000000100000e75	movl	%eax, %ecx
0000000100000e77	addl	$0x1, %ecx
0000000100000e7a	movl	%ecx, -0x4(%rbp) //赋值给变量a
0000000100000e7d	movl	%eax, -0x8(%rbp) //赋值给变量k，其中%eax是a的原始值

//k = ++a; 共4行
0000000100000e80	movl	-0x4(%rbp), %eax
0000000100000e83	addl	$0x1, %eax
0000000100000e86	movl	%eax, -0x4(%rbp) //赋值给变量a
0000000100000e89	movl	%eax, -0x8(%rbp) //赋值给变量k，其中%eax是a的值自加1后的数值


0000000100000e8c	movl	-0x8(%rbp), %eax
0000000100000e8f	popq	%rbp
0000000100000e90	retq
0000000100000e91	nopw	%cs:(%rax,%rax)

```

#### 三、结论

++在变量右边不影响表达式的结果，等价于去掉++进行表达式运算，只会影响变量本身自加1；++在变量左边是需要先自加并且自加1的结果参与表达式的后续运算。

++在变量右边：表达式使用变量原始值进行运算。估算表达式的结果可以先去掉++后估计，同时变量的数值自加1。

++在变量左边：表达式使用变量自加1后的值进行运算。估算表达式的结果不能去掉++，变量先加1后参与表达式运算。

技巧：拆分表达式，++在前面则使用*a = a + 1;*替换++，并放在目标表达式的前面；++在后面则反之。

ps：使用自加运算跟使用正常的加法运算，实现的指令数其实是差不多的。性能可能也差不多。（跟课堂上讲的有差别）