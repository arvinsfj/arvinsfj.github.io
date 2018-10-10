
# VGB模拟器中断和时钟

> *作者：Arvin 日期：2018年10月10日*

---------------------------------

>BEGIN

gameboy掌机硬件参考[这里](http://bgb.bircd.org/pandocs.htm)。

最近事情比较多，没有时间更新这个VGB模拟器系列。今天忙里抽空，来分析一下GB的中断和时钟。

### 一、前言
---------------------------------

貌似类似计算机硬件的机器都有中断机制，gameboy游戏机不严格的来讲也属于一种游戏专用计算机吧（你可以想象在GB上装操作系统，然后基于该操作系统开发和运行一些软件:）。它也有中断机制的。时钟中断是计算机最基本的中断信号，在GB中也有时钟中断。（你可以是尝试去掉时钟信号，看看Tetris会发生情况）

### 二、GB中断
---------------------------------

先讲一些理论，下面是GB中一些中断：

```
INT 40 - V-Blank Interrupt

The V-Blank interrupt occurs ca. 59.7 times a second on a regular GB and ca. 61.1 times a second on a Super GB (SGB). This interrupt occurs at the beginning of the V-Blank period (LY=144).
During this period video hardware is not using video ram so it may be freely accessed. This period lasts approximately 1.1 milliseconds.

INT 48 - LCDC Status Interrupt

There are various reasons for this interrupt to occur as described by the STAT register ($FF40). One very popular reason is to indicate to the user when the video hardware is about to redraw a given LCD line. This can be useful for dynamically controlling the SCX/SCY registers ($FF43/$FF42) to perform special video effects.

INT 50 - Timer Interrupt

Each time when the timer overflows (ie. when TIMA gets bigger than FFh), then an interrupt is requested by setting Bit 2 in the IF Register (FF0F). When that interrupt is enabled, then the CPU will execute it by calling the timer interrupt vector at 0050h.

INT 58 - Serial Interrupt

When the transfer has completed (ie. after sending/receiving 8 bits, if any) then an interrupt is requested by setting Bit 3 of the IF Register (FF0F). When that interrupt is enabled, then the Serial Interrupt vector at 0058 is called.

INT 60 - Joypad Interrupt

Joypad interrupt is requested when any of the above Input lines changes from High to Low. Generally this should happen when a key becomes pressed (provided that the button/direction key is enabled by above Bit4/5), however, because of switch bounce, one or more High to Low transitions are usually produced both when pressing or releasing a key.

It's more or less useless for programmers, even when selecting both buttons and direction keys simultaneously it still cannot recognize all keystrokes, because in that case a bit might be already held low by a button key, and pressing the corresponding direction key would thus cause no difference. The only meaningful purpose of the keystroke interrupt would be to terminate STOP (low power) standby state.
Also, the joypad interrupt does not appear to work with CGB and GBA hardware (the STOP function can be still terminated by joypad keystrokes though).

```

在GB中关于中断有3个标志字节（寄存器）：IME、IE、IF。

```
IME - Interrupt Master Enable Flag (Write Only)
  0 - Disable all Interrupts
  1 - Enable all Interrupts that are enabled in IE Register (FFFF)
The IME flag is used to disable all interrupts, overriding any enabled bits in the IE Register. It isn't possible to access the IME flag by using a I/O address, instead IME is accessed directly from the CPU, by the following opcodes/operations:
  EI     ;Enable Interrupts  (ie. IME=1)
  DI     ;Disable Interrupts (ie. IME=0)
  RETI   ;Enable Ints & Return (same as the opcode combination EI, RET)
  <INT>  ;Disable Ints & Call to Interrupt Vector
Whereas <INT> means the operation which is automatically executed by the CPU when it executes an interrupt.

中断主标志寄存器，只写，具有最大权限，可以关闭和开启中断机制的作用。不能通过I/O端口访问，只能通过CPU的特定指令进行访问（EI、DI、RETI等）。

FFFF - IE - Interrupt Enable (R/W)
  Bit 0: V-Blank  Interrupt Enable  (INT 40h)  (1=Enable)
  Bit 1: LCD STAT Interrupt Enable  (INT 48h)  (1=Enable)
  Bit 2: Timer    Interrupt Enable  (INT 50h)  (1=Enable)
  Bit 3: Serial   Interrupt Enable  (INT 58h)  (1=Enable)
  Bit 4: Joypad   Interrupt Enable  (INT 60h)  (1=Enable)

中断使能标志寄存器，可读写，开启和关闭特定5个中断。

FF0F - IF - Interrupt Flag (R/W)
  Bit 0: V-Blank  Interrupt Request (INT 40h)  (1=Request)
  Bit 1: LCD STAT Interrupt Request (INT 48h)  (1=Request)
  Bit 2: Timer    Interrupt Request (INT 50h)  (1=Request)
  Bit 3: Serial   Interrupt Request (INT 58h)  (1=Request)
  Bit 4: Joypad   Interrupt Request (INT 60h)  (1=Request)
When an interrupt signal changes from low to high, then the corresponding bit in the IF register becomes set. For example, Bit 0 becomes set when the LCD controller enters into the V-Blank period.

特定（5个）中断的标志寄存器，可读写。当某个中断信号从低变高，IF中对应的标志位会被设置成1。

```

```

Interrupt Requests
Any set bits in the IF register are only <requesting> an interrupt to be executed. The actual <execution> happens only if both the IME flag, and the corresponding bit in the IE register are set, otherwise the interrupt 'waits' until both IME and IE allow its execution.

任何IF中的位被设置成1则表示请求特定中断（并不一定会执行中断操作）。只有当IME和IE中对应位被设置成1，则中断会被执行。

Interrupt Execution
When an interrupt gets executed, the corresponding bit in the IF register becomes automatically reset by the CPU, and the IME flag becomes cleared (disabeling any further interrupts until the program re-enables the interrupts, typically by using the RETI instruction), and the corresponding Interrupt Vector (that are the addresses in range 0040h-0060h, as shown in IE and IF register decriptions above) becomes called.

当一个中断被执行，IF会被CPU自动重置，并且IME被清空（阻止任何其他的中断），同时对应的中断向量（地址为：0040h-0060h）会被调用。

Manually Requesting/Discarding Interrupts
As the CPU automatically sets and cleares the bits in the IF register it is usually not required to write to the IF register. However, the user may still do that in order to manually request (or discard) interrupts. As for real interrupts, a manually requested interrupt isn't executed unless/until IME and IE allow its execution.

IF是被CPU自动设置的，通常是不需要手动设置的。但是IF是可以被手动的设置的（生效或者丢弃的）。

Interrupt Priorities
In the following three situations it might happen that more than 1 bit in the IF register are set, requesting more than one interrupt at once:
  1) More than one interrupt signal changed from Low
     to High at the same time.
  2) Several interrupts have been requested during a
     time in which IME/IE didn't allow these interrupts
     to be executed directly.
  3) The user has written a value with several "1" bits
     (for example 1Fh) to the IF register.
Provided that IME and IE allow the execution of more than one of the requested interrupts, then the interrupt with the highest priority becomes executed first. The priorities are ordered as the bits in the IE and IF registers, Bit 0 (V-Blank) having the highest priority, and Bit 4 (Joypad) having the lowest priority.

中断优先级，Bit 0 (V-Blank)具有最高优先级别。Bit 4 (Joypad)最低。同时有多个中断请求的时候，按优先级进行响应。

Nested Interrupts
The CPU automatically disables all other interrupts by setting IME=0 when it executes an interrupt. Usually IME remains zero until the interrupt procedure returns (and sets IME=1 by the RETI instruction). However, if you want any other interrupts of lower or higher (or same) priority to be allowed to be executed from inside of the interrupt procedure, then you can place an EI instruction into the interrupt procedure.

中断嵌套，CPU在执行中断操作的时候会阻止任何其他的中断请求，但是如果你想在中断处理程序中执行其他的中断，你可以在中断处理程序中使用EI指令（打开IME）。

```

---------------------------------------

VGB模拟器中断部分实现代码如下：

数据结构：

```
#ifndef interrupt_h
#define interrupt_h

#include <stdio.h>

#define VBLANK    (1 << 0)
#define LCDSTAT   (1 << 1)
#define TIMER     (1 << 2)
#define SERIAL    (1 << 3)
#define JOYPAD    (1 << 4)

struct interrupt
{
    unsigned char pending;// 某些指令（0xD9、0xFB）设置IME会在下一个CPU周期中生效，并不会立即生效
    unsigned char master;
    unsigned char enable;
    unsigned char flags;
};

extern struct interrupt interrupt;

void interruptCycle(void);

#endif /* interrupt_h */

```

首先是5种中断标志位宏定义，其次是三个中断标志字节master、enable、flags。其中master不提供i/o端口访问。

enable的I/O地址是0xFFFF，flags的I/O地址是0xFF0F。

中断实现代码如下：

```

void interruptCycle()
{
    if (interrupt.pending == 1) {
        interrupt.pending -= 1;//修改IME的指令，IME的值在下一个CPU周期生效。
        return;
    }
    
    // if everything is enabled and there is a flag set
    if (interrupt.master && interrupt.enable && interrupt.flags) {
        
        // get which interrupt is currently being executed
        static unsigned char inter = 0;
        inter = interrupt.enable & interrupt.flags;
        
        if (inter & VBLANK) {
            interrupt.flags &= ~VBLANK; // turn off the flag
            cpuInterrupt(0x40);
        }
        
        if (inter & LCDSTAT) {
            interrupt.flags &= ~LCDSTAT;
            cpuInterrupt(0x48);
        }
        
        if (inter & TIMER) {
            interrupt.flags &= ~TIMER;
            cpuInterrupt(0x50);
        }
        
        if (inter & SERIAL) {
            interrupt.flags &= ~SERIAL;
            cpuInterrupt(0x58);
        }
        
        if (inter & JOYPAD) {
            interrupt.flags &= ~JOYPAD;
            cpuInterrupt(0x60);
        }
    }
}

void cpuInterrupt(unsigned short address)
{
    interrupt.master = 0;
    registers.SP -= 2;
    write16(registers.SP, registers.PC);
    registers.PC = address;
}

```

在3个标志寄存器都有效的时候才进行中断处理。中断处理需要按照优先级别（低位到高位）依次进行操作（if判断的位置不能换）。flags（对应位）和master的值在处理的时候需要清空。返回地址PC寄存器的值需要入栈，PC使用新地址（中断向量地址）然后跳转到对应的中断处理程序。

enable的I/O地址是0xFFFF，flags的I/O地址是0xFF0F。操作如下：

```

读操作：

    else if (address == 0xFF0F)
        return interrupt.flags;
    else if (address == 0xFFFF)
        return interrupt.enable;

写操作：

    else if (address == 0xFF0F)
        interrupt.flags = value;
    else if (address == 0xFFFF)
        interrupt.enable = value;

```

很正常的操作。


### 三、GB的时钟
---------------------------------

先看几个寄存器：

```
FF04 - DIV - Divider Register (R/W)
This register is incremented at rate of 16384Hz (~16779Hz on SGB). In CGB Double Speed Mode it is incremented twice as fast, ie. at 32768Hz. Writing any value to this register resets it to 00h.

分配器（分隔器）寄存器，按照16384Hz的频率进行自增长。向0xFF04该内存字节进行写操作会将寄存器的数值重置成0。

FF05 - TIMA - Timer counter (R/W)
This timer is incremented by a clock frequency specified by the TAC register ($FF07). When the value overflows (gets bigger than FFh) then it will be reset to the value specified in TMA (FF06), and an interrupt will be requested, as described below.

时钟计数器，会按照TAC寄存器的（数值）说明进行自增长。当数值溢出的时候（大于0xFF）会重置成TMA寄存器的值，并且触发时钟中断（INT 50 - Timer Interrupt）。

FF06 - TMA - Timer Modulo (R/W)
When the TIMA overflows, this data will be loaded.

TIMA寄存器的初始化值寄存器。

FF07 - TAC - Timer Control (R/W)
  Bit 2    - Timer Stop  (0=Stop, 1=Start)
  Bits 1-0 - Input Clock Select
             00:   4096 Hz    (~4194 Hz SGB)
             01: 262144 Hz  (~268400 Hz SGB)
             10:  65536 Hz   (~67110 Hz SGB)
             11:  16384 Hz   (~16780 Hz SGB)

定义TIMA寄存器的自增长频率。

```

数据结构：

```
#ifndef timer_h
#define timer_h

#include <stdio.h>

struct timer
{
    unsigned int div;   // divider
    unsigned int tima;  // timer counter
    unsigned int tma;   // timer module
    unsigned char tac;  // timer controller
    
    unsigned int speed;
    unsigned int started;
    
    unsigned int tick;
};

extern struct timer timer;

void timerCycle(void);

#endif /* timer_h */

```

这里面计算频率比较复杂，首先考虑的是CPU的执行频率（4.194304MHz），然后换算成上面定义的频率。

代码如下：

```
#include "timer.h"

#include "interrupt.h"
#include "cpu.h"

struct timer timer;

void tick(void)
{
    timer.tick += 1;
    
    /* Divider updates at 16384Hz */
    if (timer.tick == 16) {
        timer.div += 1;
        timer.tick = 0;
    }
    
    if (!timer.started) return;
    
    if (timer.tick == timer.speed) {
        timer.tima += 1;
        timer.tick = 0;
    }
    
    // >0xFF value overflows
    if (timer.tima == 0x100) {
        interrupt.flags |= TIMER;
        timer.tima = timer.tma;
    }
}

void timerCycle(void)
{
    //使用局部静态变量
    static unsigned int change = 0;
    static unsigned int time = 0;
    static unsigned int delta = 0;
    
    delta = getCpuCycles() - time;//cpu前进的cycles
    time = getCpuCycles();
    
    change += delta;
    
    if (change >= 16) {
        tick();
        change -= 16;
    }
}

```

上面的换算不一定完全正确。

时钟寄存器读写I/O操作：

```

读操作：

    else if (address == 0xFF04)
        return timer.div;
    else if (address == 0xFF05)
        return timer.tima;
    else if (address == 0xFF06)
        return timer.tma;
    else if (address == 0xFF07)
        return timer.tac;

写操作：

    else if (address == 0xFF04)
        timer.div = 0; // setting div to anything makes it 0
    else if (address == 0xFF05)
        timer.tima = value;
    else if (address == 0xFF06)
        timer.tma = value;
    else if (address == 0xFF07)
    {
        static int speeds[] = {256, 4, 16, 64};
        timer.tac = value;
        timer.started = value & 0x04;//bit2
        timer.speed = speeds[value & 0x03];//bit01
    }

```

这些不多讲，自己根据寄存器的定义慢慢理清思路即可。

一句话，时钟根本的作用就是不断触发时钟中断的，GB中的时钟是可以（通过TIMA、TMA、TAC）自定义自增长的速度和起始值的。分隔器就更简单了，按照固定频率自增长，溢出或者写入都会重置成0。

### 四、随便说点
---------------------------------

1. 这里都是通过软件来模拟中断和时钟硬件的行为，不一定需要完全准确。
2. 注意硬件模块的软件文件划分，不要把所有代码放在一起。一个思路是按照硬件模块进行划分。
3. 注意局部静态变量的使用，用好可以减少全局变量的使用，从而减少运行时错误。另类的信息封装。
4. 因为涉及CPU模拟代码，中断和时钟代码不能正常执行，后面写CPU代码的时候会给出执行截图（自己也可以模拟的）

后面先讲CPU，后讲LCD显示，敬请期待。

>END


