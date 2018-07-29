
# xv6操作系统APIC2（lapic）

> *作者：Arvin 日期：2018年7月25日*

---------------------------------

>BEGIN

关于APIC的硬件知识，在本篇都不会详细讲解。它的详细信息在Intel的文档[多核架构说明](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_1_4.pdf)（MPS）中有。本篇只针对xv6的lapic部分进行分析。

### 一、前言
----------------------------------

APIC（高级可编程中断控制器），是Intel针对MP（多核）架构提出的对称中断体系解决方案，替代早期的8259A PIC架构。APIC分为2部分：local apic 和 i/o apic。本地apic一般是集成在CPU的处理器核心中（每个处理核心都有一块），主要功能是接受ioapic的中断请求信号和处理器之间的中断请求。ioapic一般存在于主板上，根据实际情况可能有一块或者几块。这两种apic构成了Intel的多核中断处理体系方案。ps：apic跟8259PIC是可以协同工作的，在对称I/O模式下，pic的信号会发送给ioapic。附一张结构图：

![APIC](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_apic.png)
 
### 二、xv6的LAPIC
----------------------------------

首先看数据定义：

```
// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define EOI     (0x00B0/4)   // EOI
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
  #define ENABLE     0x00000100   // Unit Enable
#define ESR     (0x0280/4)   // Error Status
#define ICRLO   (0x0300/4)   // Interrupt Command
  #define INIT       0x00000500   // INIT/RESET
  #define STARTUP    0x00000600   // Startup IPI
  #define DELIVS     0x00001000   // Delivery status
  #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
  #define DEASSERT   0x00000000
  #define LEVEL      0x00008000   // Level triggered
  #define BCAST      0x00080000   // Send to all APICs, including self.
  #define BUSY       0x00001000
  #define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
  #define X1         0x0000000B   // divide counts by 1
  #define PERIODIC   0x00020000   // Periodic
#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
  #define MASKED     0x00010000   // Interrupt masked
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

volatile uint *lapic;  // Initialized in mp.c

```

上面定义了一些lapic的寄存器（索引）和寄存器的属性值。类似ioapic中的REG_VER，只不过ioapic需要处理的寄存器只有2个。每个寄存器的作用，暂时位查找到好的资料（什么时候找到了再分享出来：）。每个寄存器占用4字节。

最后定义了，一个uint指针变量lapic。它是在mp.c中初始化的，并且是lapic的操作基地址。后面的操作都是基于这块内存空间的。（这块内存可以想象成一张寄存器数组，数组的每一项占用4个字节，代表一个寄存器）

--------------------------------

lapic写函数：

```
static void
lapicw(int index, int value)
{
  lapic[index] = value;
  lapic[ID];  // wait for write to finish, by reading
}

```

很简单，选择好寄存器lapic[index]，然后写入value。然后通过读取ID寄存器的值（lapic[ID];）等待寄存器完成写入操作。

上面是写入操作，怎么读取寄存器呢？更简单了，比如：读取ID寄存器lapic[ID];。直接从数组中读取值就可以了。

----------------------------------

lapic初始化函数：

```
void
lapicinit(void)
{
  if(!lapic) 
    return;

  // Enable local APIC; set spurious interrupt vector.
  lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

  // The timer repeatedly counts down at bus frequency
  // from lapic[TICR] and then issues an interrupt.  
  // If xv6 cared more about precise timekeeping,
  // TICR would be calibrated using an external time source.
  lapicw(TDCR, X1);
  lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
  lapicw(TICR, 10000000); 

  // Disable logical interrupt lines.
  lapicw(LINT0, MASKED);
  lapicw(LINT1, MASKED);

  // Disable performance counter overflow interrupts
  // on machines that provide that interrupt entry.
  if(((lapic[VER]>>16) & 0xFF) >= 4)
    lapicw(PCINT, MASKED);

  // Map error interrupt to IRQ_ERROR.
  lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

  // Clear error status register (requires back-to-back writes).
  lapicw(ESR, 0);
  lapicw(ESR, 0);

  // Ack any outstanding interrupts.
  lapicw(EOI, 0);

  // Send an Init Level De-Assert to synchronise arbitration ID's.
  lapicw(ICRHI, 0);
  lapicw(ICRLO, BCAST | INIT | LEVEL);
  while(lapic[ICRLO] & DELIVS);

  // Enable interrupts on the APIC (but not on the processor).
  lapicw(TPR, 0);
}

```

这个函数，为什么要这样写请参考[MPS](
关于APIC的硬件知识，在本篇都不会详细讲解。它的详细信息在Intel的文档[多核架构说明](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_1_4.pdf)。上面的注释也还好。

通过```lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));```设置伪造IRQ并使当前CPU的LAPIC有效。

之后设置时钟中断，屏蔽逻辑中断线0和1，屏蔽PCO中断，映射error中断到中断向量T_IRQ0 + IRQ_ERROR，清理error状态寄存器，应答任何未完成的中断，校准LAPIC的ID（确保ID的唯一性），最后开启APIC的中断。这里只映射了2个IRQ（时钟中断和Error中断）。其他基本是在设置lapic的初始化状态。

-------------------------------------

EOI应答函数：

```
// Acknowledge interrupt.
void
lapiceoi(void)
{
  if(lapic)
    lapicw(EOI, 0);
}

```

是CPU在中断程序执行完成之前，向LAPIC发送的中断完成消息（概念同8259PIC的EOI相同）。直接向EOI寄存器写入0即可。lapic接受到这个消息之后，才会重新打开“请求线”的掩码（在CPU处理中断的过程中，是屏蔽所有中断请求的）。

-----------------------------------

BSP启动AP的函数：

```
#define IO_RTC  0x70

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.
void
lapicstartap(uchar apicid, uint addr)
{
  int i;
  ushort *wrv;
  
  // "The BSP must initialize CMOS shutdown code to 0AH
  // and the warm reset vector (DWORD based at 40:67) to point at
  // the AP startup code prior to the [universal startup algorithm]."
  outb(IO_RTC, 0xF);  // offset 0xF is shutdown code
  outb(IO_RTC+1, 0x0A);
  wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
  wrv[0] = 0;
  wrv[1] = addr >> 4;

  // "Universal startup algorithm."
  // Send INIT (level-triggered) interrupt to reset other CPU.
  lapicw(ICRHI, apicid<<24);
  lapicw(ICRLO, INIT | LEVEL | ASSERT);
  microdelay(200);
  lapicw(ICRLO, INIT | LEVEL);
  microdelay(100);    // should be 10ms, but too slow in Bochs!
  
  // Send startup IPI (twice!) to enter code.
  // Regular hardware is supposed to only accept a STARTUP
  // when it is in the halted state due to an INIT.  So the second
  // should be ignored, but it is part of the official Intel algorithm.
  // Bochs complains about the second one.  Too bad for Bochs.
  for(i = 0; i < 2; i++){
    lapicw(ICRHI, apicid<<24);
    lapicw(ICRLO, STARTUP | (addr>>12));
    microdelay(200);
  }
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(int us)
{
}

```

这个函数在BSP启动完成之后，会去调用来启动除了BSP（boot CPU）之外的CPU（AP）。[MPS](
关于APIC的硬件知识，在本篇都不会详细讲解。它的详细信息在Intel的文档[多核架构说明](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_1_4.pdf)的附录B，有比较详细的描述。

```
An AP may be started either by the BSP or by another active AP. The operating system causes
application processors to start executing their initial tasks in the operating system code by using the
following universal algorithm. 

The algorithm detailed below consists of a sequence of
interprocessor interrupts and short programmatic delays to allow the APs to respond to the wakeup
commands. 

The algorithm shown here in pseudo-code assumes that the BSP is starting an AP for
documentation convenience. 

The BSP must initialize BIOS shutdown code to 0AH and the warm
reset vector (DWORD based at 40:67) to point to the AP startup code prior to executing the
following sequence:

BSP sends AP an INIT IPI
BSP DELAYs (10mSec)
If (APIC_VERSION is not an 82489DX) {
 BSP sends AP a STARTUP IPI
 BSP DELAYs (200µSEC)
 BSP sends AP a STARTUP IPI
 BSP DELAYs (200µSEC)
}
BSP verifies synchronization with executing AP

```

```
Shutdown code. One of the first actions of the BIOS POST procedure is to read the shutdown
code from location 0Fh of the CMOS RAM. This code can have any of several values that
indicate the reason that an INIT was performed. A value of 0Ah indicates a warm reset.

Warm-reset vector. When POST finds a shutdown code of 0Ah, it executes an indirect jump
via the warm-reset vector, which is a doubleword pointer in system RAM location 40:67h.

```

首先宏IO_RTC定义的是CMOS的基地址。起手的2句outb调用是设置shutdown code成0Ah（暗示 a warm reset）。之后设置warm-reset向量成addr>>4。（xv6中addr是0x7000,向右平移4位等于0x700）。为什么要右移4位？因为这个时候带启动的AP还是处于实模式。实模式下，cs寄存器的段基址需要左移4位（或者说需要乘上16）得到实际的代码段基地址。设置好warm-reset向量之后，BSP就会向AP发送INIT IPI（独立和集成的LAPIC都有该中断命令）。怎么发送？命令寄存器的高位写入AP的LAPIC ID，低位写入INIT命令属性即可。调用microdelay（函数调用也是需要花费时间的）是为了等待AP启动。在独立LAPIC情况下，到这启动AP代码就结束了。不过如果是集成的（现在都是集成的）LAPIC，还需要发送另外一个称作STARTUP IPI的命令中断。高位写入AP的LAPIC ID，低位写入STARTUP属性和addr地址。为什么要addr>>12？

“The STARTUP IPI causes the target processor to start executing in Real Mode from address
000VV000h, where VV is an 8-bit vector that is part of the IPI message. ”

addr目前是0x7000。右移12位等于0x07。STARTUP IPI使AP在实模式下开始执行的起始地址是0x00007000（设置CS:IP=0700:0000）。

0x7000位置放的是什么东西？是entryother.S文件汇编代码编译之后的二进制代码。作用是设置AP的临时gdt，开启保护模式，设置临时页目录entrypgdir，开启PSE模式的分页，最后跳转到main.c文件中的mpenter函数执行。这个过程中，注意BSP使用的是虚拟地址，而AP中使用的是逻辑地址。entryother.S汇编功能相比较于bootasm.S汇编，少了A20的开启。（说明A20线的开始，是一个通用功能，跟初始化CPU无关）

### 三、随便说点
----------------------------------

到这，APIC中断分析完成。IOAPIC的初始化只需要BSP执行一次（可以认为它是硬件外设中断到CPU的分发器，起到桥梁作用，被所有CPU核心共用），其他AP中不需要再次执行IOAPIC的初始化（因为它是独立于CPU的）。在APIC对称I/O模式下，8259A中断会发送给IOAPIC，经由它转发给LAPIC，最后给到CPU。任何一个处理器核心访问IOAPIC的寄存器得到的数值是一致的。LAPIC就不一样了，每个处理器核心都有一个LAPIC，启动每个处理器核心的时候都要初始化自己的LAPIC芯片。并且，每个处理器核心从自己的LAPIC寄存器中读取到的数值是不一样的，比如：lapic[ID];读取的是该CPU的LAPIC ID。同样的，写入到寄存器的数值保存的位置不同的核心也是不一样的（虽然lapic的值可能是一样的，这个地方很疑惑🤔）。LAPIC设置完成就不会改变了，它不需要跟外界（OS）打交道（lapiceoi和cpunum函数除外），所有硬件外设的中断由IOAPIC处理（重定向表在IOAPIC中设置），开启某个外设的中断重新设置一下IOAPIC重定向表中的表项属性即可。

至此，xv6的关于硬件中断的底层（3种芯片）的初始化部分分析完成。关于中断的后半部分（os中CPU接受中断和处理中断）比较简单（相对于这部分的芯片设置内容），后面再慢慢分析和讲解吧。在操作系统中，使用到这部分的函数接口如下：

```
picenable函数

ioapicenable函数

lapiceoi函数

cpunum函数

lapicstartap函数

```

好吧，终于写完了中断初始化。里面的一些硬件编程知识，可以慢慢消化，不懂的地方继续翻文档吧。ps：我也想去多多翻翻文档。

----------------------------------

> END

