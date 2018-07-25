
# xv6操作系统APIC1（ioapic）

> *作者：Arvin 日期：2018年7月25日*

---------------------------------

>BEGIN

关于APIC的硬件知识，在本篇都不会详细讲解。它的详细信息在Intel的文档[多核架构说明](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_1_4.pdf)（MPS）中有。ps：我不觉得我能讲清楚。本篇只针对xv6的ioapic部分进行分析。

### 一、前言
----------------------------------

APIC（高级可编程中断控制器），是Intel针对MP（多核）架构提出的对称中断体系解决方案，替代早期的8259A PIC架构。APIC分为2部分：local apic 和 i/o apic。本地apic一般是集成在CPU的处理器核心中（每个处理核心都有一块），主要功能是接受ioapic的中断请求信号和处理器之间的中断请求。ioapic一般存在于主板上，根据实际情况可能有一块或者几块。这两种apic构成了Intel的多核中断处理体系方案。ps：apic跟8259PIC是可以协同工作的，在对称I/O模式下，pic的信号会发送给ioapic。附一张结构图：

![APIC](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_apic.png)
 
### 二、xv6的IOAPIC
----------------------------------

首先看数据定义：

```
#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.  
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

volatile struct ioapic *ioapic;

// IO APIC MMIO structure: write reg, then read or write data.
struct ioapic {
  uint reg;
  uint pad[3];
  uint data;
};

```

"The APIC default base memory addresses defined by this specification are
0FEC0_0000h and 0FEE0_0000h."（0FEC0_0000h是ioapic的基地址，0FEE0_0000h是lapic的基地址，每个区域占用0x100000字节）。

![APIC MM](http://arvinsfj.github.io/public/ctt/documents/osxv6/apic_mm.png)

注意ioapic这块内存区域是共享的。

"The address space reserved for the local APIC is used by each processor to access its own local APIC.

The address space reserved for the I/O APIC must be shareable by all processors to permit dynamic
reconfiguration."

什么意思呢？这块保留的内存区域是被所有的处理器核心共同使用的，允许处理器核心动态配置该区域。（lapic是不共享的，每个处理器核心有自己的访问向量，来访问自己的lapic）

"Unlike the local APICs, the I/O APICs are mapped to give shared access from all processors,
providing full symmetric I/O access. The default base address for the first I/O APIC is
0FEC0_0000h. Subsequent I/O APIC addresses are assigned in 4K increments. For example, the
second I/O APIC is at 0FEC0_1000h."

IOAPIC宏，是IOAPIC的默认物理地址。这个位置的内存结构（MMIO）使用结构体ioapic定义。操作方式是：写入寄存器的索引，然后读写数据。

```
#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

```

上面的三个宏定义了三个寄存器（可以这样理解）的索引。在操作ioaipc的时候，写入该寄存器索引，就会让ioapic控制器知道你操作的是那个寄存器，然后就可以正常的读写该寄存器了。（ps：这个地方没有找到资料，通过xv6代码大概是这个意思）。


```
#define REG_TABLE  0x10  // Redirection table base
```

比较特殊，它是重定向表的基地址，什么是重定向表？其实就是ioapic（共16个IRQ线）的中断请求线编号到中断向量编号的映射表。后面会初始化这张表的，建立映射关系。当然这张表的表项除了中断向量编号之外，还包含其他一些属性（比如：中断无效、水平触发、激活低字节和目标是CPU ID等）。

```
// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.  
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

```

上面的四个宏就是定义这些属性的。


### 三、IO APIC 编程
----------------------------------

初始化代码如下：

```
void
ioapicinit(void)
{
  int i, id, maxintr;

  if(!ismp)
    return;

  ioapic = (volatile struct ioapic*)IOAPIC;
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
  id = ioapicread(REG_ID) >> 24;
  if(id != ioapicid)
    cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");

  // Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  for(i = 0; i <= maxintr; i++){
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}

static uint
ioapicread(int reg)
{
  ioapic->reg = reg;
  return ioapic->data;
}

static void
ioapicwrite(int reg, uint data)
{
  ioapic->reg = reg;
  ioapic->data = data;
}

```

下面的2个函数是ioapic的读写工具函数，符合ioapic的mmio内存块的操作规范：写入寄存器的索引，然后读写数据。

ioapicinit函数首先将ioapic的mmio基地址赋值给结构体指针变量ioapic，后面使用这个遍历进行操作（也就是后面的操作全部是操作ioapic基地址的那块内存区域）。

```
maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
```

通过读取REG_VER（Version）寄存器，获取ioapic支持的最大中断向量编号。

```
id = ioapicread(REG_ID) >> 24;
```

通过读取REG_ID（ID）寄存器，获取当前ioapic的唯一id。这个id需要跟apic配置表中定义的id相同。（ioapicid是在收集mp信息的时候赋值的）

```
// Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  for(i = 0; i <= maxintr; i++){
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }

```

最后，通过一个for循环，设置重定向表，并设置一些属性。每个表项占用8字节，前4个字节定义属性和中断向量编号，后4个字节定义指向的（路由）CPU ID。初始化的时候，设置全部中断向量为无效。


开启特定中断的函数代码如下：

```
void
ioapicenable(int irq, int cpunum)
{
  if(!ismp)
    return;

  // Mark interrupt edge-triggered, active high,
  // enabled, and routed to the given cpunum,
  // which happens to be that cpu's APIC ID.
  ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
}

```

通过设置重定向表项```REG_TABLE+2*irq```，成(T_IRQ0 + irq, cpunum << 24)，来开启irq线的中断。其中```cpunum << 24```代表该向量指向的CPU ID。(如果在该向量上发生了中断，则中断信号会发送到该CPU的LAPIC芯片)。

cpunum的获取在lapic.c文件中：

```
int
cpunum(void)
{
  // Cannot call cpu when interrupts are enabled:
  // result not guaranteed to last long enough to be used!
  // Would prefer to panic but even printing is chancy here:
  // almost everything, including cprintf and panic, calls cpu,
  // often indirectly through acquire and release.
  if(readeflags()&FL_IF){
    static int n;
    if(n++ == 0)
      cprintf("cpu called from %x with interrupts enabled\n",
        __builtin_return_address(0));
  }

  if(lapic)
    return lapic[ID]>>24;
  return 0;
}

```
首先判断CPU的eflags寄存器IF标志位，如果中断无效则返回该CPU ID（lapic id）的高8位（作为cpunum使用）。

### 四、随便说点
----------------------------------

总结下来，IOAPIC的编程，最终只会操作两个地址（一个“寄存器地址”和一个数据读写地址），它们是对ioapic芯片内部寄存器和重定向表的简化抽象。这两个地址到底在物理内存中存不存在，我们不得而知（如果你懂NES模拟器的编写就知道其中的原因），有可能只是外设I/O内存映射的地址（并不关心内存中存不存在）。后面如果需要开启某个IRQ线，只要调用ioapicenable函数即可。比如：```ioapicenable(IRQ_KBD, 0);```就在IOAPIC芯片中开启了键盘中断请求。注意区分os中的IDT和这里的重定向表，这里的表更加底层，它保存在IOAPIC芯片中的（类似8259中的IVT）。

后面的操作系统（OS），只跟IOAPIC打交道，不会跟LAPIC交互的（LAPIC只要初始化好就可以了）。

----------------------------------

> END

