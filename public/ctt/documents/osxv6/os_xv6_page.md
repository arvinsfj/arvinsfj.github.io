
# xv6操作系统分页

> *作者：Arvin 日期：2018年7月20日*

---------------------------------

>BEGIN

现代操作系统都支持x86的分页机制（构建物理地址的另外一种机制，之前有分段机制）。正常的分页机制大家可能都清楚，32位机器采用二级页表的形式（页目录->页表->物理地址），64位机采用4级页表。如果打开cr4寄存器的page size extention（PSE）并设置页目录项的bit7（第8位）成1呢？开启了PSE的分页跟正常的分页区别很大。ps：另外一个特例是PAE（page address extention）。

### 一、前言
----------------------------------

接上篇《xv6操作系统boot》，boot将kernel加载到物理内存0x100000处，并跳转到内核的第一条指令开始执行。数据段（ds、es、ss等）、代码段（cs:ip）以及栈顶指针（esp）都设置好了，C代码是可以执行的。本篇讲解分页，要将线性地址（xv6分段中，将虚拟地址直接映射成线性地址了，即虚拟地址就是线性地址）映射成实际的物理地址。内核代码都使用线性地址0x80100000开始会映射到物理地址0x100000。其次，我们后面需要知道内核各个段的开始和结束地址。我们需要更加强大的工具，使用[linker script](http://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_mono/ld.html)可以实现这一点。xv6内核的前面10几条指令是使用AT&T的32位汇编写成（entry.S文件中），功能是开启分页（也开启了pse），设置新的esp栈顶指针，然后间接跳转到内核的main函数。开启分页是这段汇编的主要功能。

### 二、xv6分页（开启带PSE的分页）
----------------------------------

代码如下：

```
# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
_start = V2P_WO(entry)

# Entering xv6 on boot processor, with paging off.
.globl entry
entry:
  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4
  # Set page directory
  movl    $(V2P_WO(entrypgdir)), %eax
  movl    %eax, %cr3
  # Turn on paging.
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Set up the stack pointer.
  movl $(stack + KSTACKSIZE), %esp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  mov $main, %eax
  jmp *%eax

.comm stack, KSTACKSIZE

```

连接文件kernel.ld中定义了内核的入口函数（标签）是```_start```。就是上面的_start = V2P_WO(entry)。V2P_WO是一个宏，将虚拟地址转换成物理地址。这个地方为什么要作转换呢？kernel.ld中定义了内核的开始地址是0x80100000（这个是开启分页之后的地址）。而我们的内核是被加载到物理地址0x100000处的。但是执行到_start处的时候，还未开启分页，也就是cpu这个时候还只认识物理地址。entry的数值是0x80100000，start的数值是0x100000，刚好是需要的物理地址。这个地方比较巧妙。

开启分页的第一步（特指xv6中这个阶段的）是打开pse，很简单设置cr4寄存器的bit4（第5位）为1即可（宏CR4_PSE定义成0x00000010）。开启pse之后有可能就是分页机制的PSE“模式”了。在PSE下，只有页目录和一张4M的页表。其实就是线性地址的低22位是偏移量，高10位是页目录的索引值，不需要查找第二级页表了。具体参考[PSE Wiki](https://en.wikipedia.org/wiki/Page_Size_Extension)。图如下：

![PSE下的分页](http://arvinsfj.github.io/public/ctt/documents/osxv6/page_pse.png)

少了二级页表查找的过程。

开启PSE之后，就是正常的开启分页过程。1》cr3寄存器加载页目录基址；2》cr0设置开启分页标识位。注意：cr3寄存器加载页目录基址，这里的基址是物理地址，而非线性地址。entrypgdir在main.c文件中定义，属于内核是线性地址，需要转换成物理地址才能使用。32位寄存器cr0的最高位（bit31）是开启分页位，最低位（bit0）是开启保护模式位。

开启分页之后，就是简单的设置新的栈顶指针esp和跳转到main函数执行了。.comm宏指令是在bss段定义了一段内存区域作为栈区域（4096字节）。由于栈是向低地址增长的，所以esp要指向地址$(stack + KSTACKSIZE)。为什么栈是向低地址增长的呢？想想pop和push指令就知道了。

需要注意的是```mov $main, %eax 和 jmp *%eax```这两句汇编。作用是跳转到main标签开始执行（也就是内核中的main函数）。为什么不直接使用```jmp main```指令呢？原因是直接跳转jmp的跳转地址是相对当前PC（ip寄存器）的值的。而main标签就是线性（绝对）地址，不需要相对于当前pc的值。可以采用间接跳转解决这个问题。

下面是entrypgdir页目录表的定义：

```
__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

```

页表和页目录表必须按照页大小（4K）对齐，分页机制决定的。在开启PSE的情况下，线性地址的高10位是页目录的索引（每个页目录项和页表项，占用4字节，pde_t被定义成uint类型），共1K个项目，占用4K大小（1页）。线性地址的低22位作为偏移量，可以表示4M空间。上面的entrypgdir页目录表，共定义了2项，分别将虚拟地址[0,4M)和[KERNBASE, KERNBASE+4M)都映射到物理地址[0,4M)空间。这也是使用分页机制的好处，可以灵活的将线性地址（虚拟地址）空间映射到同一个物理地址空间。上面页目录项目定义的4M空间的基地址是物理地址0。页目录项（页表项）的高20位是基地址的物理地址，低12位定义所表示的地址空间的访问属性等。

注意：上面的定义的PTE_PS字段，需要配合cr4的PSE字段使用。共同表示使用分页机制的PSE”模式“。

在上面的页目录定义下，线性地址0x800b8000和线性地址0xb8000表示的是同一个物理地址0xb8000。

到这个地方，分页机制的开启就完成了。

内核的main函数定义如下：

```
int
main(int argc, char** argv)
{
    asm volatile("movl %0, %%ebp; movl $0x2f4b2f4f, 0(%%ebp)"::"r"(0xb8000+2));//0x800b8000，显示结果一样
    asm volatile("movl %0, %%ebp; movl $0x2f4f2f4b, 0(%%ebp)"::"r"(0xb8000+16));
    asm volatile("hlt"::);
    return 0;
}

```

用到了前篇中提到的gcc内嵌汇编的知识。主要是在vga显示器中显示OK和KO四个字母。地址0xb8000时vga显存的开始地址。$0x2f4b2f4f表示绿底白字的OK两个字母。关于VGA显示卡的知识请自行学习[VGA Text Mode](https://os.phil-opp.com/vga-text-mode/)和[VGA Hardware](https://wiki.osdev.org/VGA_Hardware)。关于[Multiboot](https://os.phil-opp.com/multiboot-kernel/)也可以了解一下，毕竟很方便。关于x64的[Long Mode](https://os.phil-opp.com/entering-longmode/)了解一下。

该篇到此为止。开启分页过程：CR4开启PSE、CR3加载页目录基址和CR0开启分页。页目录项和页表项结构也要注意一下。还有就是整个过程中的物理地址和线性地址之间的关系和转换。

### 三、后话
----------------------------------

开发环境如下：

```
macos Hign Sierra下：

模拟器qemu：安装使用 brew install qemu | 运行xv6 qemu-system-i386 -hda xv6.img -m 512 -smp 2

代码编译环境 VMware Fusion + CentOS7(minimal) ： 配置网络 + 安装wget + 安装python + 安装gcc + 安装perl


windows10下：

模拟器qemu：官网下载安装程序并安装 | 运行xv6 qemu-system-i386 -hda xv6.img -m 512 -smp 2

代码编译环境 win10自带的Linux子系统(WSL) + Debian ： 安装wget + 安装gcc + 安装perl

```

----------------------------------

补充一下xv6.img镜像制作过程(在Makefile文件中)：

```
xv6.img: bootblock kernel
  dd if=/dev/zero of=xv6.img count=10000
  dd if=bootblock of=xv6.img conv=notrunc
  dd if=kernel of=xv6.img seek=1 conv=notrunc

bootblock: bootasm.S bootmain.c
  $(CC) $(CFLAGS) -fno-pic -O -nostdinc -I. -c bootmain.c
  $(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c bootasm.S
  $(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
  $(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
  ./sign.pl bootblock

kernel: $(OBJS) entry.o kernel.ld
  $(LD) $(LDFLAGS) -T kernel.ld -o kernel entry.o $(OBJS)
```

第一步是将bootasm.S bootmain.c编译成目标文件（.o结尾的），然后链接成一个目标文件bootblock.o，注意起始是0x7C00。接着使用objcopy单独将bootblock.o的代码段（.text）拷贝进bootblock文件，最后使用perl脚本sign.pl在bootblock文件最后2个字节改写成0x55AA（表示是一个启动分区）。注意这一步生成的bootblock必须是一个分区的大小512个字节。（代码段最多510个字节）

第二步是编译entry.S和main.c成一个IA32位ELF可执行文件（正常结构的ELF可执行文件）。注意使用了linker script脚本文件kernel.ld。

第三步是使用dd命令，先分配一个包含1000个块（block）的xv6.img文件，默认每个块应该是512字节，并且全部使用数值0填充（/dev/zero文件的作用）。其次，是将bootloader镜像bootblock写入xv6.img文件，不截短（？不知道是什么意思）。最后将kernel的ELF文件从第二个块（扇区）（seek=1）开始写入xv6.img文件。完成xv6.img内核镜像的制作。

下面是xv6.img磁盘镜像文件的16进制截图：

![xv6.img镜像](http://arvinsfj.github.io/public/ctt/documents/osxv6/img_vi.png)

注意标红的地方，可以清楚的看到bootblock跟kernel的边界。而且由于boot代码的原因（bootmain.c中默认从扇区1加载内核ELF文件到内存中），在启动模拟器qemu的时候，xv6.img需要作为hda（第一个磁盘驱动器）的镜像文件加载，否则qemu可以找到启动扇区但是不能加载内核ELF文件。

---------------------------------------

后面的计划会按照xv6的main函数中的函数调用顺序依次进行分析：

```
int
main(void)
{
  kinit1(end, P2V(4*1024*1024)); // phys page allocator
  kvmalloc();      // kernel page table
  mpinit();        // collect info about this machine
  lapicinit();
  seginit();       // set up segments
  cprintf("\ncpu%d: starting xv6\n\n", cpu->id);
  picinit();       // interrupt controller
  ioapicinit();    // another interrupt controller
  consoleinit();   // I/O devices & their interrupts
  uartinit();      // serial port
  pinit();         // process table
  tvinit();        // trap vectors
  binit();         // buffer cache
  fileinit();      // file table
  iinit();         // inode cache
  ideinit();       // disk
  if(!ismp)
    timerinit();   // uniprocessor timer
  startothers();   // start other processors
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
  userinit();      // first user process
  // Finish setting up this processor in mpmain.
  mpmain();
}
```

大概的思路是：内存管理、中断、控制台、串口、陷阱和系统调用、文件管理、多核处理器启动、第一个用户进程和进程管理。

现在来看，整个操作系统都是由中断进行驱动。（周期性的时钟中断、外设硬件中断和软中断等）


> END

[Download](http://arvinsfj.github.io/public/ctt/documents/osxv6/TestOS.zip)

