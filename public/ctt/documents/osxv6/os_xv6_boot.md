
# xv6操作系统boot

> *作者：Arvin 日期：2018年7月15日*

---------------------------------

>BEGIN

最近在研究MIT的教学用操作系统xv6。受益颇多，准备开一个系列来分享操作系统知识。xv6是一个简单、类Unix的教学操作系统内核。它是MIT操作系统课程（6.828）使用的案例操作系统（另外一个操作系统是JOS，基于外核的），是对Unix V6的重新实现，遵循了v6的结构和风格（宏内核），但是它是基于多核x86采用ANSI C实现的，它是32位的。

### 一、前言
----------------------------------

Github上的xv6库很多都不能直接在linux下编译成功，需要自己仔细找找（后面会放出完整的源代码的）。编译成功后，需要使用Qemu模拟器运行，一些参数的设置自己可以查看Makefile文件。最终的运行效果如下：

![xv6运行效果](http://arvinsfj.github.io/public/ctt/documents/osxv6/xv6_run.png)

也可以采用非图形模式运行，xv6还有一本课程讲义草稿，可以在MIT官网下载（见过的最好的os资料，建议看英文的）。

### 二、boot讲解
----------------------------------

当然可以直接使用grub的multiboot加一个multiheader就可以了，grub直接给你引导到32位保护模式下。（不需要自己开启A20、不需要设置gdt、不需要开启保护模式等）。但是作为一个学习者，我们要掌握每一个细节，从cpu通电起的每一个细节。

xv6的boot代码是在bootasm.S和bootmain.c两个文件中的。前者进行底层的cpu操作（初始化和设置段寄存器、开启A20、设置gdt和开启保护模式，最后跳转到bootmain函数（C函数）执行）。后者是用C写的，从硬盘加载编译好的内核（kernel）可执行文件到内存中。最后，bootloader将程序的执行权交给（跳转到）内核执行（entry.S中）。

```
Start the first CPU: switch to 32-bit protected mode, jump into C.
The BIOS loads this code from the first sector of the hard disk into memory at physical address 0x7c00 and starts executing in real mode with %cs=0 %ip=7c00.
```

x86机器在BIOS自检和加载第一个启动扇区（0扇区，512字节，最后两个字节是0x55AA），就会从物理内存0x7c00处开始执行（原因：%cs=0 %ip=7c00）。第一个扇区被加载到物理内存的0x7c00处。为什么是物理内存，因为这个时候x86（CPU）还处于实模式，地址“寄存器cs：寄存器ip”就是指向物理内存的，实模式是16位的（可以认为是一块8086）。如果你想学习8086汇编，可以在实模式下运行汇编程序。实模式应该是最直接的CPU模式，你可以控制所有东西，只不过它是16位的，限制了内存的使用。我们想用4GB的内存，必须切换CPU模式到x86保护模式。在切换之前，还要开启A20。开启A20跟切换保护模式，之间有一些微妙的关系，自己可以查查（不开启A20，也可以切换到保护模式的，只不过内存有限制）。还有一点就是在编译的时候，生成的代码段（.text）必须从0x7c00地址开始。linux下gcc添加"-Ttext 0x7c00"选项。

```
.code16                       # 16位汇编，因为运行到这个位置当前x86还是实模式下。
.globl start
start:
  cli                         # BIOS 开启了中断; 这个地方要关闭

  # 将寄存器 DS, ES, 和 SS 都设置成0，对应段（data段、额外段、栈段）的基地址为0；
  xorw    %ax,%ax             # 将ax设置为0，gas采用AT&T汇编格式，xorw是异或指令。
  movw    %ax,%ds             # -> 数据段
  movw    %ax,%es             # -> 额外段
  movw    %ax,%ss             # -> 栈段

  #物理地址线A20默认是为0（关闭的），在保护模式下会出现不能访问全部内存的问题，需要开启。在实模式下和A20关闭的情况下，超过1MB的内存会自动回卷到从0开始（减去1MB）。A20的设置，是i286为了兼容8086的实模式，根本是由高端内存区HMA造成的（地址线20根、段寄存器16位）。打开A20，有2中方式：调用BIOS中断就开启A2和使用键盘控制器（8042芯片）来开启A20。下面是第2种方式，因为这种方式大部分pc都支持（虽然是一种hack方式）。
seta20.1:
  inb     $0x64,%al               # 等待直到键盘控制器不忙
  testb   $0x2,%al
  jnz     seta20.1

  movb    $0xd1,%al               # 0xd1 -> port 0x64 ， 将0xd1写入端口0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # 等待直到键盘控制器不忙
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60 ， 将0xdf写入端口0x64
  outb    %al,$0x60

  #从实模式切换到保护模式，使用一个GDT让虚拟地址直接映射成物理地址（GDT的基址设置为0，大小4GB，以及设置读写运行权限），有效内存映射，在切换过程中并没有改变。（实模式下ds、es、ss都为0，保护模式下GDT的段基地址表项也为0）
  #切换方法：只要设置寄存器cr0的最低位为1即可。在下一次修改cs的时候生效。
  #gdtr记录gdt的物理地址是必须的，gdtr包含gdt表的最大界限长（size-1）和gdt的起始地址。gdtr是6字节的。
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE, %eax
  movl    %eax, %cr0

  # 使用ljmp完成到32位保护模式的切换，重新加载cs和ip， $(SEG_KCODE<<3)加载到cs中，$start32加载到ip中。
  # 注意这个时候cs和ip的长度，保护模式下cs等段寄存器的值是作为gdt索引值的（实模式下是物理地址的基地址）
  ljmp    $(SEG_KCODE<<3), $start32     #ljmp    $(1<<3), $start32

  .......

  # Bootstrap GDT
.p2align 2                                # 强制 4 字节 对齐
gdt:
  SEG_NULLASM                             # null 段， 这个段是某些x86模拟器需要的，否则会报错
  SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code 段
  SEG_ASM(STA_W, 0x0, 0xffffffff)         # data 段

gdtdesc:
  .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
  .long   gdt    

```

SEG_NULLASM和SEG_ASM宏定义如下（gdt每个表项占用8字节）：
```
#define SEG_NULLASM                                             \
        .word 0, 0;                                             \
        .byte 0, 0, 0, 0

// The 0xC0 means the limit is in 4096-byte units
// and (for executable segments) 32-bit mode.
#define SEG_ASM(type,base,lim)                                  \
        .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);      \
        .byte (((base) >> 16) & 0xff), (0x90 | (type)),         \
                (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#define STA_X     0x8       // Executable segment
#define STA_E     0x4       // Expand down (non-executable segments)
#define STA_C     0x4       // Conforming code segment (executable only)
#define STA_W     0x2       // Writeable (non-executable segments)
#define STA_R     0x2       // Readable (executable segments)
#define STA_A     0x1       // Accessed

```

以上是16位汇编代码，是实模式下的执行代码。下面是32位汇编代码（保护模式下的）。

```
.code32  # 告诉汇编器现在是32位代码了
start32:
  # 设置保护模式下的数据段寄存器
  movw    $(SEG_KDATA<<3), %ax    # 数据段选择器的索引（索引为16字节）
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  movw    $0, %ax                 # 不用的段设置为0
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS

  # 设置栈顶寄存器esp（32位）为 $start地址（0x7c00）（注意：栈是从高地址向低地址增长的，也就是栈不会覆盖boot代码的）
  movl    $start, %esp
  call    bootmain

  # 如果 bootmain函数 返回了（说明后面的函数出错了）, 触发bochs模拟器回调，并且死循环。
  movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
  movw    %ax, %dx
  outw    %ax, %dx
  movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
  outw    %ax, %dx
spin:
  jmp     spin

```

在设置了代码段（cs:ip）、数据段（ds、es、ss、fs、gs）和栈顶指针（esp）之后，就可以调用C代码函数了。（C跟汇编差不了多少）

保护模式下的地址构造是：段寄存器 -> 查表GDT -> 表项包含段基址 -> 段基址+代码中的虚拟地址（从0开始） -> 实际的物理地址。

上面就是保护模下的分段机制。比实模式多了一个基地址查表（GDT）过程。注意各个寄存器的长度。如果你想学习16位汇编就只需要在实模式下就行了，如果想学习32汇编在保护模式下就好了（先开启A20地址线，并设置GDT表项段基地址为0）。（当然如果想学习64位汇编，开启长模式就可以了，请自行查资料。）

### 三、boot加载内核过程
-------------------------------------------

xv6的内核是一个elf格式的可执行文件，从硬盘1扇区开始存放（0扇区是bootloader）。现在ds、es、ss指向地址0，fs、gs为0，cs:ip指向bootmain函数入口，esp指向0x7c00处。（di、si、bp不考虑）。

如何从磁盘读取内核到内存中，内核放在内存的什么位置？xv6使用insl等指令读取磁盘内核elf文件头（4096字节），并且放到从0x10000开始的内存位置（当然真正的内核执行代码放在什么地方，elf文件头中有描述，物理位置0x100000）。下面是读取内核的核心C代码：

```
void
bootmain(void)
{
  struct elfhdr *elf;
  struct proghdr *ph, *eph;
  void (*entry)(void);
  uchar* pa;

  elf = (struct elfhdr*)0x10000;  // elf文件头放在内存中的位置，看懂这句需要看透C的本质。

  // 读取1扇区开始的4096个字节，1扇区在readseg函数中有体现
  readseg((uchar*)elf, 4096, 0);

  // 检验elf魔数
  if(elf->magic != ELF_MAGIC)
    return;  // 返回的话，就是回到了bootasm.S文件了，整个启动失败

  // 分析elf格式并加载每个程序段，到指定的内存位置（0x100000）
  ph = (struct proghdr*)((uchar*)elf + elf->phoff);
  eph = ph + elf->phnum;
  for(; ph < eph; ph++){
    pa = (uchar*)ph->paddr;
    readseg(pa, ph->filesz, ph->off); // 读取内核程序段（大小filesz、偏移off）到内存的pa位置。
    if(ph->memsz > ph->filesz)
      stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz); //内存多余的空间补上0
  }

  // 获取elf的执行入口，并跳转到入口开始执行
  entry = (void(*)(void))(elf->entry);
  entry(); // entry代表的是那个函数呢？
}
```

其中elf文件格式，可以自行查找资料。这段C代码跟我们平常写的C代码很不一样，自己可以细细品味一下。比如：elf、ph和entry，在程序中并没有创建（结构体和函数）实体，但是为何就可以正常的进行结构体读写和函数调用。上面的代码其实在正常的C的开发中可以进行验证（不一定非要在内核开发中）。你可以在C代码中加载一个elf文件到内存中，并跳转到该elf程序进行执行（前提是这段内存有执行权限）。现在看来，C语言只是对汇编进行了逻辑结构封装和数据类型的封装。

readseg函数调用汇编代码（特权级指令in、out、insl等）直接从磁盘读取内核数据到指定的内存区域。代码自己可以细细研读，没什么特别的地方。

bootasm.S和bootmain.c所有的代码编译之后的大小必须小于或者等于510个字节，因为bios加载的启动扇区只有1个扇区大小512字节，并且最后两个字节必须是0x55AA，以便识别成启动扇区。另外上面的C代码必须编译成32位指令。（现在的计算机都是64位的，gcc默认都是编译成64位指令的，必须加上-m32选项）

现在内核已经加载到指定内存位置了，下一步是跳转到内核执行。

entry代表的是那个函数呢？entry()调用那个函数呢？又会跳转到哪里执行呢？答案在kernel.ld链接脚本中，请自行查找。（entry.S中的_start标签位置，也是entry标签位置）

xv6的下一步是开启x86的分页机制，敬请期待。24点瞌睡了。:]

> END

