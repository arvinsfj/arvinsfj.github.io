
# xv6操作系统8259A可编程中断控制器（picirq.c）

> *作者：Arvin 日期：2018年7月24日*

---------------------------------

>BEGIN

前篇《xv6操作系统收集多核信息》是为LAPIC和IOAPIC中断作准备，但是研究下来APIC实在有点难度，不好理解。APIC放在后面再做分析，之前先讲一下IBM PC/AT架构中使用的8259A可编程中断控制器，xv6也有用到，8259A比较简单。

### 一、前言
----------------------------------

8259A可编程中断控制器是一种独立芯片，在PC/AT串联使用了两片这种芯片，下面简称这种芯片为：PIC。每个PIC提供8个中断入口，两个串联共提供15个中断入口。（为什么不是16个呢？从片slave出口连在主片master的入口2，占用了master的一个入口）。slave的irq9跟master的irq2（master的irq2在PC/AT中被用作slave的出口，为了兼容PC架构）功能相同。

![8259A](http://arvinsfj.github.io/public/ctt/documents/osxv6/8259pic.png)

提供一篇关于PIC编程的文章[HRER](https://wiki.osdev.org/8259_PIC)。


### 二、8259 PIC 做什么？
----------------------------------

8259 PIC 控制CPU的中断机制，通过接受多个中断请求，并按照顺序转发给处理器。常见的中断，比如：键盘、鼠标、DMA等外设的中断。如果没有PIC，你需要轮询所有外设，查找需要CPU做事情的设备。如果有了PIC，你的系统可以一直运行直到某时一个设备发送了某个事件。你的系统不需要浪费时间在轮询和等待上，因为设备准备好了会主动通知CPU。

在IBM PC和XT 上，只有一块8259PIC，提供8个IRQs。IBM PC/AT上，提供了两块8259PIC，提供15个IRQs。这种两块PIC的架构，一直沿用到现代系统上，并没有去掉。

每个pic有8个输入，当任何一个输入端有数值的时候（raised），pic会在内部设置1位来告诉这个输入端需要服务。然后，它检查这个输入通道是否被屏蔽（marked），并且检查是否存在一个中断已经在等待决定的。如果输入端通道没有被屏蔽（unmarked）并且没有中断在等待，pic将拉起中断线（告诉cpu需要进行中断处理了）。在slave上，这个情况会反馈到master上，master是连接到处理器中断入口线的。

当处理器接受这个中断，master决定那个pic来响应处理器，然后提供中断编号给到处理器或者让slave提供中断编号给处理器。pic使用内部存储的向量偏移量（vector offset）加上输入线编号来形成中断请求编号。之后，处理器将查找中断地址和调用中断处理程序。

### 三、8259 PIC 编程
----------------------------------

可编程中断处理器，当然是可以编程的啊。每块pic拥有一个命令端口和一个数据端口。当没有命令的时候，数据端口允许我们访问中断掩码（interrupt mask）。

```
master：

command 0x20
data    0x21

slave:

command 0xa0
data    0xa1 

```

在保护模式下，irqs 0-7 跟 Intel保留的 cpu 异常中断编号 产生冲突（会混淆IRQ和软件错误）。建议修改pic的offset，来避免该问题。xv6中偏移量是32，即0x20。slave偏移是0x28。

pic最常见的命令是EOI（the end of interrupt）。中断处理程序结束的时候，会向pic发送该命令。如果irq来自master，只要向master发送一个EOI，如果是来自slave，需要向master和slave都发送EOI命令。

当你进入保护模式，第一个需要向pic发送的命令是初始化命令（code：0x11）。这条命令会让pic在数据端口等待3个额外的初始化字。

```
1. ICW2: 向量偏移量
2. ICW3: 它是如何连接master或者slave的
3. ICW4: 提供关于环境的附加信息

```

如果你将要使用APIC，你必须先关闭pic：

```
mov al, 0xff
out 0xa1, al
out 0x21, al

```

向pic的数据端口写入0xFF。

pic有一个内部寄存器，IMR（Interrupt Mask Register）。它是8位宽。它是pic请求线的一个bitmap（位图，1个字节的每一个位代表一根线）。当某一个位被设置成1，则pic忽略这根线的请求，继续常规操作。各位之间互不影响。master中irq2如果被设置成1，则slave停止响应中断请求。

pic有2个中断状态寄存器（ISR和IRR）：the In-Service Register (ISR) and the Interrupt Request Register (IRR)。ISR告诉我们那个中断正在被服务，意味着IRQs发送到CPU。IRR告诉我们那个中断已经被拉起。基于IMR中的中断掩码，pic将从IRR发送中断到CPU，这个时候它们在ISR中被标记。

ISR和IRR可以通过使用OCW3命令字被读取。这个命令发送到pic的命令端口（设置bit3位）。写入合适的命令到命令端口，然后读取该命令端口（是的，不是数据端口，比较特别）。读取IRR，写入0x0a；读取ISR，写入0x0b。

伪造IRQs，当pic告诉cpu有一个中断请求，但是没有发送中断向量给cpu，这个请求就消失了，就会造成CPU一直等待pic发送中断向量。为了解决这个问题，pic会告诉一个伪造IRQ（一般是优先级最低的中断编号，master中是irq7，slave中是irq15）。

处理伪造IRQs，如果是irq7或者irq15，则需要检查pic的ISR（如果是伪造IRQ，ISR中的flag不会被设置），来判断是不是伪造IRQ，如果是则忽略它并不向pic发送EOI命令，如果不是则正常处理。如果是irq15的伪造IRQ，则不向slave发送EOI，但是需要向master发送EOI。（很奇怪！）

### 四、xv6 的 8259 PIC 编程
----------------------------------

先看代码：

```
// I/O Addresses of the two programmable interrupt controllers
#define IO_PIC1         0x20    // Master (IRQs 0-7)
#define IO_PIC2         0xA0    // Slave (IRQs 8-15)

#define IRQ_SLAVE       2       // IRQ at which slave connects to master

// Current IRQ mask.
// Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
static ushort irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);

static void
picsetmask(ushort mask)
{
  irqmask = mask;
  outb(IO_PIC1+1, mask);
  outb(IO_PIC2+1, mask >> 8);
}

void
picenable(int irq)
{
  picsetmask(irqmask & ~(1<<irq));
}

```

最上面的2个宏，定义了两块pic的命令端口（数据端口各自加上1即可）。第三个宏定义了，irq2串联slave从片。

```
static ushort irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);
```

irqmask 定义成ushort，2字节。我们通过上面的理论知识，知道IMR是8位宽的。为什么这里是2字节（16位宽）的呢？很简单，irqmask将主和从pic的掩码合并成一个ushort类型，占用2字节。高地址字节是从片（slave）的掩码，低地址字节是主片（master）掩码。初始化的值是低字节的bit2（第3位）为0，其他位都设置成1。也就是，只有master的irq2被设置成有效中断线，即串联slave的中断入口线。

picsetmask函数很简单，设置master和slave的掩码。master的掩码的在mask的低字节，slave的在高字节。设置掩码只需要向数据端口写入数据即可。

picenable函数，是设置掩码的irq位为0，即开启IRQ(irq)，开启中入口线irq。记住，掩码对应位置为0，则该入口线（中断）有效。设置成1则被屏蔽，中断请求无效。

上面是如何开启和关闭中断请求线的方法。下面讲如何初始化pic：

```
// Initialize the 8259A interrupt controllers.
void
picinit(void)
{
  // mask all interrupts
  outb(IO_PIC1+1, 0xFF);
  outb(IO_PIC2+1, 0xFF);

  // Set up master (8259A-1)

  // ICW1:  0001g0hi
  //    g:  0 = edge triggering, 1 = level triggering
  //    h:  0 = cascaded PICs, 1 = master only
  //    i:  0 = no ICW4, 1 = ICW4 required
  outb(IO_PIC1, 0x11);

  // ICW2:  Vector offset
  outb(IO_PIC1+1, T_IRQ0);

  // ICW3:  (master PIC) bit mask of IR lines connected to slaves
  //        (slave PIC) 3-bit # of slave's connection to master
  outb(IO_PIC1+1, 1<<IRQ_SLAVE);

  // ICW4:  000nbmap
  //    n:  1 = special fully nested mode
  //    b:  1 = buffered mode
  //    m:  0 = slave PIC, 1 = master PIC
  //      (ignored when b is 0, as the master/slave role
  //      can be hardwired).
  //    a:  1 = Automatic EOI mode
  //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
  outb(IO_PIC1+1, 0x3);

  // Set up slave (8259A-2)
  outb(IO_PIC2, 0x11);                  // ICW1
  outb(IO_PIC2+1, T_IRQ0 + 8);      // ICW2
  outb(IO_PIC2+1, IRQ_SLAVE);           // ICW3
  // NB Automatic EOI mode doesn't tend to work on the slave.
  // Linux source code says it's "to be investigated".
  outb(IO_PIC2+1, 0x3);                 // ICW4

  // OCW3:  0ef01prs
  //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
  //    p:  0 = no polling, 1 = polling mode
  //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
  outb(IO_PIC1, 0x68);             // clear specific mask
  outb(IO_PIC1, 0x0a);             // read IRR by default

  outb(IO_PIC2, 0x68);             // OCW3
  outb(IO_PIC2, 0x0a);             // OCW3

  if(irqmask != 0xFFFF)
    picsetmask(irqmask);
}

```

根据第三节的理论知识，我们后面要使用APIC，故首先将pic的中断全部关闭。master和slave的中断掩码全部设置成0xFF。

然后，pic的命令端口写入初始化命令（0x11），进行pic的初始化任务。写入初始化命令之后，需要在数据端口写入三个初始化字：设置向量偏移量、设置主从连接关系、设置关于环境的附加信息。（ICW2、ICW3、ICW4）。各个初始化的字的含义，代码中有解释，自己去思考。

最后写入OCW3命令，设置成clear specific mask和read IRR by default。上面两个操作对主从pic都是一样的。

设置完成后，调用```picsetmask(irqmask);```函数，设置master的irq2为有效，使主从pic连通。完成pic初始化工作。

后面要开启pic的键盘中断，只要调用```picenable(IRQ_KBD);```开启irq1即可，很简单也很方便。其实，关于8259 PIC，也就这些知识。

这里虽然是在讲8259PIC，其实也是在讲中断初始化和中断处理的前端部分。后面的APIC中断芯片的初始化，也是类似的处理逻辑。这部分还是要仔细研究一下，确保自己理解了中断芯片编程的各个细节，对理解后面的东西很重要。（毕竟操作系统的运行全部是由中断驱动的）

----------------------------------

> END

