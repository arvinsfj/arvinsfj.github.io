
# xv6操作系统COM1串口（Intel 8250 uart）

> *作者：Arvin 日期：2018年7月30日*

---------------------------------

>BEGIN

串口是一种常用的接口，嵌入式开发环境中，开发板通常都会提供串口与pc相连，方便调试。在早期网络环境中，pc通常通过串口连modem再接通电话线来连通对方电脑。串口经常用来作为远程终端使用。

### 一、前言
----------------------------------

Intel 8250 串口芯片，提供8个寄存器。COM1串口基地址是0x3f8。这8个寄存器分成2种模式进行操作，由DLAB位控制（线控制寄存器器LCR"base+3"的bit7）。数据的读写是在DLAB=0情况下，地址0x3f8（base）上进行的。中断在base+1寄存器控制，FIFO在base+2寄存器控制，波特率在DLAB=01情况下的base+0寄存器设置的，base+5可以获取串口的状态信息。具体串口芯片的编程参考[HERE](https://www.lammertbies.nl/comm/info/serial-uart.html)。

### 二、数据定义
----------------------------------

数据定义如下：

```
#define COM1    0x3f8

static int uart;    // is there a uart?

```

COM1定义串口的寄存器入口地址（或者叫做"base"）。

静态变量uart，定义了一个标记，记录串口是否存在。

### 三、函数分析
----------------------------------

初始化函数如下：

```
void
uartinit(void)
{
  char *p;

  // Turn off the FIFO
  outb(COM1+2, 0);
  
  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  outb(COM1+3, 0x80);    // Unlock divisor 除数锁存访问位DLAB开启
  outb(COM1+0, 115200/9600);
  outb(COM1+1, 0);
  outb(COM1+3, 0x03);    // Lock divisor, 8 data bits. 除数锁存访问位DLAB关闭
  outb(COM1+4, 0);
  outb(COM1+1, 0x01);    // Enable receive interrupts.

  // If status is 0xFF, no serial port.
  if(inb(COM1+5) == 0xFF)
    return;
  uart = 1;

  // Acknowledge pre-existing interrupt conditions;
  // enable interrupts.
  inb(COM1+2);
  inb(COM1+0);
  picenable(IRQ_COM1);
  ioapicenable(IRQ_COM1, 0);
  
  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p);
}

```

没什么好讲的，参考8250串口硬件编程文档即可。DLAB开启是为了设置波特率，随后就关闭了DLAB位。在检查了串口存在之后，就设置uart变量等于1，标记存在串口。最后在pic和ioapic中断芯片中设置COM1中断有效完成UART的初始化操作。

串口字符写函数：

```
void
uartputc(int c)
{
  int i;

  if(!uart)
    return;
  for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
    microdelay(10);
  outb(COM1+0, c);
}

```

首先判断串口是否存在，如果存在则循环读取COM1串口的当前状态，如果串口线的状态空闲则跳出循环等待，直接向base+0地址写入字符c（串口发送字符c）。

串口字符读取函数：

```
static int
uartgetc(void)
{
  if(!uart)
    return -1;
  if(!(inb(COM1+5) & 0x01))
    return -1;
  return inb(COM1+0);
}

```

首先判断串口是否存在，如果存在则读取COM1串口的当前状态一次，如果状态有效则从base+0地址读取一个字符，并返回给调用函数。


串口中断处理函数：

```
void
uartintr(void)
{
  consoleintr(uartgetc);
}

```

中断处理直接转发给终端中断处理函数，只是传入了一个uartgetc函数指针（该函数功能是从串口读取一个字符）。consoleintr函数的具体处理请看上一篇文章《xv6操作系统终端（console）》。

xv6的串口驱动程序分析完成。初始化稍微难一点，其他都很简单。注意console跟uart的关系。

### 四、补充说明：键盘驱动
----------------------------------

键盘是输入设备，只有读取操作和中断处理操作，没有写入操作。因为内容很少，并且都属于终端的外设，而且跟uart很像。在这里简单的介绍一下：

```
int
kbdgetc(void)
{
  static uint shift;
  static uchar *charcode[4] = {
    normalmap, shiftmap, ctlmap, ctlmap
  };
  uint st, data, c;

  st = inb(KBSTATP);
  if((st & KBS_DIB) == 0)
    return -1;
  data = inb(KBDATAP);

  if(data == 0xE0){
    shift |= E0ESC;
    return 0;
  } else if(data & 0x80){
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    return 0;
  } else if(shift & E0ESC){
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }

  shift |= shiftcode[data];
  shift ^= togglecode[data];
  c = charcode[shift & (CTL | SHIFT)][data];
  if(shift & CAPSLOCK){
    if('a' <= c && c <= 'z')
      c += 'A' - 'a';
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }
  return c;
}

// 中断处理函数，跟uart一样，直接转发给终端中断函数处理，只不过传入的kbdgetc函数指针
void
kbdintr(void)
{
  consoleintr(kbdgetc);
}

```

上面的核心函数是键盘读取字符函数：kbdgetc函数。

```
#define KBSTATP         0x64    // kbd controller status port(I)
#define KBS_DIB         0x01    // kbd data in buffer
#define KBDATAP         0x60    // kbd data port(I)

```

注意一下，上面的三个宏分别定义了键盘的（控制）状态端口、数据在缓存中状态位掩码和数据读取端口。

```
st = inb(KBSTATP);
if((st & KBS_DIB) == 0)
  return -1;
data = inb(KBDATAP);

```

上面的三句话，就是读取键盘状态，如果状态是"数据在缓存区中"时，则从数据端口读取一个字节的数据到data变量中。

在读取到一个字节数据data之后的处理，无非是处理键盘原始数据data，从而得到一个字符c，并返回给调用函数。

这中间的键盘原始数据处理（原始键盘码到ascii字符的映射关系），可以自己查找资料和分析。

### 五、随便说点
----------------------------------

到这，讲完了抽象设备终端封装的三种外设（VGA显示器、UART串口和键盘）的初始化、读写和中断处理。

芯片等硬件编程，无非是向port地址（芯片寄存器在内存中的映射地址）写入或者读取数据。编程复杂程度，根据芯片的寄存器多少决定（还有读写的操作方法）。这样一想，CPU（也是芯片硬件）编程的方法也是这样的，只是寄存器不是mmio的（映射地址），而是用寄存器符号代替，这些寄存器除了读写操作还有很多其他操作，比如：ADD、MOV等CPU操作指令。CPU可能是最复杂的硬件编程吧。ps：NES的PPU编程也很复杂。

接下来，会去补充中断的OS层处理（之前讲的中断是底层芯片级别的初始化和开启特定IRQ线等处理）和非常重要的文件系统fs。毕竟在类Unix系统里"一切皆文件"嘛。24：30 ") 今晚到这吧。

-----------------------------------

> END

