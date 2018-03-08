
# hook原理和缓冲区溢出（杂谈）

> *作者：Arvin 日期：2018年03月08日*

---------------------------------

>BEGIN

hook一般是针对动态库函数的加载，动态或静态修改桩向量。最简单的hook（演示）我们直接可以在桩向量表手动修改表项使指向钩子函数。缓冲区溢出主要是数组等的增长方向跟调用栈的增长方向相反，并且保证栈上具有代码执行权限，即可实现缓冲区漏洞攻击。

### 一、思路
---------------------------------

hook直接修改桩向量的指向即可。hook的动态实现请自行参考FB的fishhook库。缓冲区溢出一方面可以修改局部变量的数值，另一方面还可以修改函数返回的地址达到返回到其他函数的目的，如果其他函数的二进制在栈上，并且栈具有执行权限，那么很简单就可以运行时执行任意代码。

### 二、简单的hook
---------------------------------

简单的hook可以理解成动态的函数替换。

C代码如下：

```
#include <stdio.h>

int new_strlen(const char *_s) {
    printf("%s\n", _s);
    return 9;
}

int main(int argc, const char * argv[])
{
    char *str = "hellolazy";
    printf("%d\n", strlen(str));
    return 0;
}
```

使用：gcc main.c -o main 生成可执行文件main。直接运行./main 打印 9 。

使用：otool -vt main 生成反汇编。

```
main:
(__TEXT,__text) section
_new_strlen:
0000000100000ee0	pushq	%rbp
0000000100000ee1	movq	%rsp, %rbp
0000000100000ee4	subq	$0x10, %rsp
0000000100000ee8	leaq	0xb1(%rip), %rax
0000000100000eef	movq	%rdi, -0x8(%rbp)
0000000100000ef3	movq	-0x8(%rbp), %rsi
0000000100000ef7	movq	%rax, %rdi
0000000100000efa	movb	$0x0, %al
0000000100000efc	callq	0x100000f70
0000000100000f01	movl	$0x9, %ecx
0000000100000f06	movl	%eax, -0xc(%rbp)
0000000100000f09	movl	%ecx, %eax
0000000100000f0b	addq	$0x10, %rsp
0000000100000f0f	popq	%rbp
0000000100000f10	retq
0000000100000f11	nopw	%cs:(%rax,%rax)
_main:
0000000100000f20	pushq	%rbp
0000000100000f21	movq	%rsp, %rbp
0000000100000f24	subq	$0x30, %rsp
0000000100000f28	leaq	0x7f(%rip), %rax
0000000100000f2f	leaq	0x6e(%rip), %rcx
0000000100000f36	movl	$0x0, -0x4(%rbp)
0000000100000f3d	movl	%edi, -0x8(%rbp)
0000000100000f40	movq	%rsi, -0x10(%rbp)
0000000100000f44	movq	%rcx, -0x18(%rbp)
0000000100000f48	movq	-0x18(%rbp), %rdi
0000000100000f4c	movq	%rax, -0x20(%rbp)
0000000100000f50	callq	0x100000f76
0000000100000f55	movq	-0x20(%rbp), %rdi
0000000100000f59	movq	%rax, %rsi
0000000100000f5c	movb	$0x0, %al
0000000100000f5e	callq	0x100000f70
0000000100000f63	xorl	%edx, %edx
0000000100000f65	movl	%eax, -0x24(%rbp)
0000000100000f68	movl	%edx, %eax
0000000100000f6a	addq	$0x30, %rsp
0000000100000f6e	popq	%rbp
0000000100000f6f	retq
```

上面可以看出strlen库函数的调用换成了callq 0x100000f76指令。这里的0x100000f76就是桩的入口。这个地方一般放一条jmp指令。jmp指令的地址地址参数是从__la_symbol_ptr表获取的。
换句话，我们只要修改__la_symbol_ptr表的向量就可以实现函数调用的替换（hook）。下面需要找到该向量的位置和向量的值。

使用：otool -l main 打印LoadCommand。

```
main:
Mach header
      magic cputype cpusubtype  caps    filetype ncmds sizeofcmds      flags
 0xfeedfacf 16777223          3  0x80           2    15       1200 0x00200085
Load command 0
      cmd LC_SEGMENT_64
  cmdsize 72
  segname __PAGEZERO
   vmaddr 0x0000000000000000
   vmsize 0x0000000100000000
  fileoff 0
 filesize 0
  maxprot 0x00000000
 initprot 0x00000000
   nsects 0
    flags 0x0
Load command 1
      cmd LC_SEGMENT_64
  cmdsize 472
  segname __TEXT
   vmaddr 0x0000000100000000
   vmsize 0x0000000000001000
  fileoff 0
 filesize 4096
  maxprot 0x00000007
 initprot 0x00000005
   nsects 5
    flags 0x0
Section
  sectname __text
   segname __TEXT
      addr 0x0000000100000ee0
      size 0x0000000000000090
    offset 3808
     align 2^4 (16)
    reloff 0
    nreloc 0
     flags 0x80000400
 reserved1 0
 reserved2 0
Section
  sectname __stubs
   segname __TEXT
      addr 0x0000000100000f70
      size 0x000000000000000c
    offset 3952
     align 2^1 (2)
    reloff 0
    nreloc 0
     flags 0x80000408
 reserved1 0 (index into indirect symbol table)
 reserved2 6 (size of stubs)
Section
  sectname __stub_helper
   segname __TEXT
      addr 0x0000000100000f7c
      size 0x0000000000000024
    offset 3964
     align 2^2 (4)
    reloff 0
    nreloc 0
     flags 0x80000400
 reserved1 0
 reserved2 0
Section
  sectname __cstring
   segname __TEXT
      addr 0x0000000100000fa0
      size 0x0000000000000012
    offset 4000
     align 2^0 (1)
    reloff 0
    nreloc 0
     flags 0x00000002
 reserved1 0
 reserved2 0
Section
  sectname __unwind_info
   segname __TEXT
      addr 0x0000000100000fb4
      size 0x0000000000000048
    offset 4020
     align 2^2 (4)
    reloff 0
    nreloc 0
     flags 0x00000000
 reserved1 0
 reserved2 0
Load command 2
      cmd LC_SEGMENT_64
  cmdsize 232
  segname __DATA
   vmaddr 0x0000000100001000
   vmsize 0x0000000000001000
  fileoff 4096
 filesize 4096
  maxprot 0x00000007
 initprot 0x00000003
   nsects 2
    flags 0x0
Section
  sectname __nl_symbol_ptr
   segname __DATA
      addr 0x0000000100001000
      size 0x0000000000000010
    offset 4096
     align 2^3 (8)
    reloff 0
    nreloc 0
     flags 0x00000006
 reserved1 2 (index into indirect symbol table)
 reserved2 0
Section
  sectname __la_symbol_ptr
   segname __DATA
      addr 0x0000000100001010
      size 0x0000000000000010
    offset 4112
     align 2^3 (8)
    reloff 0
    nreloc 0
     flags 0x00000007
 reserved1 4 (index into indirect symbol table)
 reserved2 0
Load command 3
      cmd LC_SEGMENT_64
  cmdsize 72
  segname __LINKEDIT
   vmaddr 0x0000000100002000
   vmsize 0x0000000000001000
  fileoff 8192
 filesize 336
  maxprot 0x00000007
 initprot 0x00000001
   nsects 0
    flags 0x0
Load command 4
            cmd LC_DYLD_INFO_ONLY
        cmdsize 48
     rebase_off 8192
    rebase_size 8
       bind_off 8200
      bind_size 24
  weak_bind_off 0
 weak_bind_size 0
  lazy_bind_off 8224
 lazy_bind_size 32
     export_off 8256
    export_size 64
Load command 5
     cmd LC_SYMTAB
 cmdsize 24
  symoff 8328
   nsyms 6
  stroff 8448
 strsize 80
Load command 6
            cmd LC_DYSYMTAB
        cmdsize 80
      ilocalsym 0
      nlocalsym 0
     iextdefsym 0
     nextdefsym 3
      iundefsym 3
      nundefsym 3
         tocoff 0
           ntoc 0
      modtaboff 0
        nmodtab 0
   extrefsymoff 0
    nextrefsyms 0
 indirectsymoff 8424
  nindirectsyms 6
      extreloff 0
        nextrel 0
      locreloff 0
        nlocrel 0
Load command 7
          cmd LC_LOAD_DYLINKER
      cmdsize 32
         name /usr/lib/dyld (offset 12)
Load command 8
     cmd LC_UUID
 cmdsize 24
    uuid ABE851B6-0E6F-33DB-96DB-B4C93A1CB26E
Load command 9
      cmd LC_VERSION_MIN_MACOSX
  cmdsize 16
  version 10.13
      sdk 10.13
Load command 10
      cmd LC_SOURCE_VERSION
  cmdsize 16
  version 0.0
Load command 11
       cmd LC_MAIN
   cmdsize 24
  entryoff 3872
 stacksize 0
Load command 12
          cmd LC_LOAD_DYLIB
      cmdsize 56
         name /usr/lib/libSystem.B.dylib (offset 24)
   time stamp 2 Thu Jan  1 08:00:02 1970
      current version 1252.0.0
compatibility version 1.0.0
Load command 13
      cmd LC_FUNCTION_STARTS
  cmdsize 16
  dataoff 8320
 datasize 8
Load command 14
      cmd LC_DATA_IN_CODE
  cmdsize 16
  dataoff 8328
 datasize 0
```
从上面的加载命令可以查到0x100001010这个地址是表__la_symbol_ptr的基地址。我们总共用到了两个库函数（printf和strlen），所以该表中存在两条向量。

使用vi打开main文件，可以找到该位置确实存在两个向量：0x10000000f8c和0x10000000f96。结合0x100000f76是strlen函数桩的入口，我们查一下0x100000f76地址处的jmp指令参数地址（f7c+9c=1018），确定strlen函数的表向量是0x10000000f96。

我们只要把0x10000000f96修改成我们的new_strlen函数的入口地址即可。new_strlen的入口地址是：0x100000ee0。使用vi修改一下即可。

![hook效果](http://arvinsfj.github.io/public/ctt/documents/testasm/hook_xg1.png)

使用./main再次运行效果如下：

![hook效果](http://arvinsfj.github.io/public/ctt/documents/testasm/hook_xg2.png)



### 三、简单的缓冲区溢出（修改局部变量）
---------------------------------

C代码如下：

```
#include <stdio.h>

void of_data()
{
    unsigned char a = 2;
    unsigned char b = 0;
    *((unsigned short*)&b) = 256;
    printf("a = %d\n", a);
}


int main(int argc, const char * argv[])
{
    of_data();
    return 0;
}
```

gcc编译运行，会打印a=1。原因很简单，*((unsigned short*)&b) = 256; 赋值超出了char的范围，最终会用00000001覆盖a的字节，使a的数值变成1。

有上面的测试可以知道，局部变量是放在栈上的，并且先定义的变量在先入栈，比如a。一般栈低在高地址，栈顶在低地址。而数值的排列一般按照小端序排列，增长方向是低地址向高地址增长，跟栈的增长方向相反。
这就导致了上面的问题。这种溢出只会影响局部变量，如果数组是局部变量的话，就可以影响调用帧了，可以覆盖函数的返回地址。


### 四、简单的缓冲区溢出（修改返回地址）
---------------------------------

C代码如下：

```
#include <stdio.h>

static long tmp;

void of_test()
{
    char a = 8;
    *((long*)(&a+9)) = tmp;
}

void of_data()
{
    char a = 2;
    tmp = *((long*)(&a+9));
    *((long*)(&a+9)) = of_test;
}

int main(int argc, const char * argv[])
{
    of_data();
    return 0;
}
```

使用gcc -g of_main.c -o of_main 生成可执行文件。使用lldb of_main调试程序，看看程序的跳转逻辑。

跳转顺序是：main->of_data->of_test->main。这就是覆盖函数返回地址造成的。

对于mach-o可执行结构，设置mach_header_64的flags字段成MH_ALLOW_STACK_EXECUTION（0x20000）就可以让可执行文件在栈上具有执行权限。

可以自己尝试一下。


### 五、随便说点
---------------------------------

1. 简单的做了一下hook和overflow。
2. 这当中还有许多可玩的东西，可以自己尝试。比如：动态hook、真实的缓冲区溢出攻击等。


>END


