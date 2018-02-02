# 如何在Data段中执行机器指令（osx平台）
-----------------------------------------

该篇接上一篇《如何在C代码中执行机器指令》，主要研究在DATA段中嵌入机器代码进行编译和执行。相对于TEXT段，DATA段没有执行权限，但是我们可以通过修改LoadCommand中DATA段的权限来让DATA段具有执行权限。

#### 一、基本思路

思路跟《如何在C代码中执行机器指令》篇差不多。只是需要手工修改可执行文件中DATA段的权限。

#### 二、基本步骤

1. 编写C源代码

```
#include <stdio.h>

static unsigned char sc[16] = "\x55\x48\x89\xe5\xe8\xf7\xfe\xff\xff\x5d\xc3\x0f\x1f\x44\x00\x00";

void mia()
{
	 printf("%d\n", 'a');
}

void mic()
{
    ((void (*)())sc)();
}

int main(int argc, char** argv)
{
	mia();
    mic();
	return 0;
}

```
上面的代码，包含3个函数，mia打印静态变量字符'a'，mic是调用机器代码定义的函数sc，其在DATA段，main是主函数。

2. 使用GCC编译：gcc test.c -o test

3. 使用反汇编工具：otool -tv test 显示汇编码

```
text_exec:
(__TEXT,__text) section
_mia:
0000000100000f20	pushq	%rbp
0000000100000f21	movq	%rsp, %rbp
0000000100000f24	subq	$0x10, %rsp
0000000100000f28	leaq	0x7b(%rip), %rdi
0000000100000f2f	movl	$0x61, %esi
0000000100000f34	movb	$0x0, %al
0000000100000f36	callq	0x100000f88
0000000100000f3b	movl	%eax, -0x4(%rbp)
0000000100000f3e	addq	$0x10, %rsp
0000000100000f42	popq	%rbp
0000000100000f43	retq
0000000100000f44	nopw	%cs:(%rax,%rax)
_mic:
0000000100000f50	pushq	%rbp
0000000100000f51	movq	%rsp, %rbp
0000000100000f54	movb	$0x0, %al
0000000100000f56	callq	0x100001020
0000000100000f5b	popq	%rbp
0000000100000f5c	retq
0000000100000f5d	nopl	(%rax)
_main:
0000000100000f60	pushq	%rbp
0000000100000f61	movq	%rsp, %rbp
0000000100000f64	subq	$0x10, %rsp
0000000100000f68	movl	$0x0, -0x4(%rbp)
0000000100000f6f	movl	%edi, -0x8(%rbp)
0000000100000f72	movq	%rsi, -0x10(%rbp)
0000000100000f76	callq	0x100000f20
0000000100000f7b	callq	0x100000f50
0000000100000f80	xorl	%eax, %eax
0000000100000f82	addq	$0x10, %rsp
0000000100000f86	popq	%rbp
0000000100000f87	retq

```
完整的汇编码可以使用：gcc -S test.c -o test.s

```
	.section	__DATA,__data
	.p2align	4               ## @sc
_sc:
	.asciz	"UH\211\345\350\367\376\377\377]\303\017\037D\000"
	
```
注意我们的机器代码字符串常量，被定义在__DATDA段。使用：otool -lv test 可以知道DATA段具有可读可写的权限，但是没有执行权限。

```
Load command 2
      cmd LC_SEGMENT_64
  cmdsize 312
  segname __DATA
   vmaddr 0x0000000100001000
   vmsize 0x0000000000001000
  fileoff 4096
 filesize 4096
  maxprot rwx
 initprot rw-
   nsects 3
    flags (none)
```
我们需要修改DATA段的权限。使用：vi test 打开二进制文件，并使用:%!xxd子命令格式化显示二进制（%!xxd -r子命令取消格式化）
![修改位置](http://arvinsfj.github.io/public/ctt/documents/testasm/de.png)

再次使用：otool -lv test 可以看到DATA段具有可读可写可执行的权限了，达到了我们的目的。
```
Load command 2
      cmd LC_SEGMENT_64
  cmdsize 312
  segname __DATA
   vmaddr 0x0000000100001000
   vmsize 0x0000000000001000
  fileoff 4096
 filesize 4096
  maxprot rwx
 initprot rwx
   nsects 3
    flags (none)

```

至此，我们的DATA段具有执行权限了。按照上一篇《如何在C代码中执行机器指令》的方法，我们修改一下e8（callq）指令的偏移地址就可以执行了。


![执行结果](http://arvinsfj.github.io/public/ctt/documents/testasm/mc.png)

注意mia函数被调用了两次，也就是验证了我们的想法。

在mic中调用sc函数，sc函数中调用mia函数。

sc函数是定义在DATA段的，是全局的，可以在任何C函数中调用。不过还有位置问题（因为机器码中的偏移量是硬编码的）。

因为DATA段具有可写的权限，我们接下来尝试通过外部输入机器码，然后执行，即运行时执行外部代码。

C代码如下：

```
#include <stdio.h>
#include <string.h>

static unsigned char sc[16] = {'a'};

//./test `perl -e 'print "\x55\x48\x89\xe5\xe8\xd7\xfe\xff\xff\x5d\xc3\x0f\x1f\x44\x00\x00"'`

void mia()
{
	 printf("%d\n", 'a');
}

void mic()
{
    ((void (*)())sc)();
}

int main(int argc, char** argv)
{
    memcpy(sc, argv[1], 16);
	mia();
    mic();
	return 0;
}
```

使用：gcc test.c -o test 生成可执行文件test，然后按照本文的方法修改DATA段的权限。最后使用下面的方法执行程序：

```
./test `perl -e 'print "\x55\x48\x89\xe5\xe8\xd7\xfe\xff\xff\x5d\xc3\x0f\x1f\x44\x00\x00"'`

```

执行结果如下：

![执行结果](http://arvinsfj.github.io/public/ctt/documents/testasm/derd.png)



#### 三、话语

1. DATA段是可以执行机器代码的，并且可以执行外部传入的机器代码
2. mach-o文件格式确实存在这种问题
3. bss段应该也存在这种问题，其他的动态区比如：堆区、栈区等，有没有相同的问题


---------------------------------------------

END

