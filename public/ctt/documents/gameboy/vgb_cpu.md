
# VGB模拟器的CPU实现

> *作者：Arvin 日期：2018年10月11日*

---------------------------------

>BEGIN

gameboy掌机硬件参考[这里](http://bgb.bircd.org/pandocs.htm)。

cpu是任何电子产品的核心，图灵机的物理实现吧。ps：需要加上内存。

### 一、前言
---------------------------------

GB中的CPU是Z80处理器（类似Z80，而Z80是兼容intel的8080的），是标准Z80、8080的混合体吧。8位处理器。ps：z80、6502、8080等是同时代的8位处理器。

### 二、GameBoy的CPU数据定义
---------------------------------

```

Registers
  16bit Hi   Lo   Name/Function
  AF    A    -    Accumulator & Flags
  BC    B    C    BC
  DE    D    E    DE
  HL    H    L    HL
  SP    -    -    Stack Pointer
  PC    -    -    Program Counter/Pointer
As shown above, most registers can be accessed either as one 16bit register, or as two separate 8bit registers.

类似8080的寄存器结构，可以单独8bit使用，也可以组合成16bit使用。F寄存器是标志寄存器具有4个标志。SP是栈指针寄存器。PC是程序指针寄存器。

The Flag Register (lower 8bit of AF register)
  Bit  Name  Set Clr  Expl.
  7    zf    Z   NZ   Zero Flag
  6    n     -   -    Add/Sub-Flag (BCD)
  5    h     -   -    Half Carry Flag (BCD)
  4    cy    C   NC   Carry Flag
  3-0  -     -   -    Not used (always zero)
Conatins the result from the recent instruction which has affected flags.

F标志寄存器的标志位定义。

The Zero Flag (Z)
This bit becomes set (1) if the result of an operation has been zero (0). Used for conditional jumps.

The Carry Flag (C, or Cy)
Becomes set when the result of an addition became bigger than FFh (8bit) or FFFFh (16bit). Or when the result of a subtraction or comparision became less than zero (much as for Z80 and 80x86 CPUs, but unlike as for 65XX and ARM CPUs). Also the flag becomes set when a rotate/shift operation has shifted-out a "1"-bit.
Used for conditional jumps, and for instructions such like ADC, SBC, RL, RLA, etc.

The BCD Flags (N, H)
These flags are (rarely) used for the DAA instruction only, N Indicates whether the previous instruction has been an addition or subtraction, and H indicates carry for lower 4bits of the result, also for DAA, the C flag must indicate carry for upper 8bits.
After adding/subtracting two BCD numbers, DAA is intended to convert the result into BCD format; BCD numbers are ranged from 00h to 99h rather than 00h to FFh.
Because C and H flags must contain carry-outs for each digit, DAA cannot be used for 16bit operations (which have 4 digits), or for INC/DEC operations (which do not affect C-flag).

```

数据结构定义：

```
struct registers
{
    unsigned char A;
    unsigned char F; // flags: Z N H C
    
    unsigned char B;
    unsigned char C;
    
    unsigned char D;
    unsigned char E;
    
    unsigned char H;
    unsigned char L;
    
    unsigned short SP;
    unsigned short PC;
    
    unsigned int cycles;//cpu总周期数
};

```

当然也可以使用union共用体来定义AF、BC、DE、HL的组合结构。这里为了简单直观直接采用8个单独的char类型来定义寄存器，组合关系使用单独的宏定义操作来实现。cycles是额外的字段，表示当前CPU执行的总CPU周期量。宏定义如下：

```

#define SET_AF(x) do {registers.A = ((x & 0xFF00) >> 8); registers.F = (x&0x00FF);} while(0) // multi-line macro
#define SET_BC(x) do {registers.B = ((x & 0xFF00) >> 8); registers.C = (x&0x00FF);} while(0)
#define SET_DE(x) do {registers.D = ((x & 0xFF00) >> 8); registers.E = (x&0x00FF);} while(0)
#define SET_HL(x) do {registers.H = ((x & 0xFF00) >> 8); registers.L = (x&0x00FF);} while(0)

#define GET_AF() ((registers.A << 8) | registers.F)
#define GET_BC() ((registers.B << 8) | registers.C)
#define GET_DE() ((registers.D << 8) | registers.E)
#define GET_HL() ((registers.H << 8) | registers.L)

#define SET_Z(x) registers.F = ((registers.F & 0x7F) | (x << 7))
#define SET_N(x) registers.F = ((registers.F & 0xBF) | (x << 6))
#define SET_H(x) registers.F = ((registers.F & 0xDF) | (x << 5))
#define SET_C(x) registers.F = ((registers.F & 0xEF) | (x << 4))

#define FLAG_Z ((registers.F >> 7) & 0x1)
#define FLAG_N ((registers.F >> 6) & 0x1)
#define FLAG_H ((registers.F >> 5) & 0x1)
#define FLAG_C ((registers.F >> 4) & 0x1)

```

F标志寄存器需要单独处理，因为它只用到了4个bit位。


### 三、GameBoy的CPU指令集实现
---------------------------------

先看两个函数：

```

void cpuInterrupt(unsigned short address)
{
    interrupt.master = 0;
    registers.SP -= 2;
    write16(registers.SP, registers.PC);
    registers.PC = address;
}

void cpuInit(void)
{
    interrupt.master = 1;
    interrupt.enable = 0;
    interrupt.flags = 0;
    
    SET_AF(0x01B0);
    SET_BC(0x0013);
    SET_DE(0x00D8);
    SET_HL(0x014D);
    registers.SP = 0xFFFE;
    registers.PC = 0x0100;
    registers.cycles = 0;
    
    memInit();
}

unsigned int getCpuCycles(void)
{
    return registers.cycles;
}

```

cpuInterrupt在讲中断的时候遇到过，关闭中断的master标志，SP栈顶指针向低地址移动2个字节（空出空间存放返回地址PC），然后使用write16函数向SP栈中写入返回地址PC，最后PC寄存器的数值修改成传入的地址address，跳转到中断服务程序。

cpuInit函数是初始化中断标志、CPU寄存器和内存RAM的函数。调整整个系统到初始状态，准备开始执行cpu指令。

注意SP和PC的初始化值。

getCpuCycles函数返回当前CPU的总执行周期数。

---------------------------------

CPU执行循环函数：

```

void cpuCycle(void)
{
    static int halted = 0;
    if (halted) {
        registers.cycles += 1;
        return;
    }
    
    int i;
    unsigned char s;
    unsigned short t;
    unsigned int u;
    unsigned char instruction = read8(registers.PC);
    
    switch (instruction) {
        case 0x00:    // NOP
            registers.PC += 1;
            registers.cycles += 1;
            break;
        case 0x01:    // LD BC,nn
            SET_BC(read16(registers.PC+1));
            registers.PC += 3;
            registers.cycles += 3;
            break;
        case 0x02:    // LD (BC),A
            write8(GET_BC(), registers.A);
            registers.PC += 1;
            registers.cycles += 2;
            break;
        case 0x03:    // INC BC
            SET_BC((GET_BC() + 1));
            registers.PC += 1;
            registers.cycles += 2;
            break;
        ...
        ...
        default:
            printf("Instruction: %02X\n", (int)instruction);
            printf("Undefined instruction.\n");
            break;
    }
}

```

cpuCycle函数是在一个"死循环"中被调用的，会不断的被执行。上面的代码不全，只是示意一下CPU指令集的模拟实现。

```unsigned char instruction = read8(registers.PC);```是从PC指向的内存位置读取一个8bit的指令。然后通过switch条件判断，匹配instruction指令码的实际操作，相当于一个指令解码过程。

举例来讲，指令```0x03```是BC值自增1操作，然后PC向后移动1个字节（0x03指令长度为1字节），最后cycles加2（0x03指令CPU执行周期为2）。

PC和cycles分别表示指令的空间长度和时间长度。在写模拟器的时候，一定要精确计算指令操作所耗费的CPU周期数，以便精确模拟CPU（同步CPU和外设PPU、APU等）。

至于halted变量，它是为了实现指令```0x76和0x10```，分别表示HALT和STOP。

上面的函数表示了```取指令-解码指令-执行指令```的循环过程。

任何模拟器，都会有类似的CPU指令执行循环函数，在实际的分析中注意识别。

Z80是8bit的处理器，即指令码长度是8bit，最多256条指令（0x00-0xFF）。但是注意一下，GB中有一条指令是有扩展指令的，它是指令```0xCB```，称作“Prefix指令”。这条指令（1字节）码后面紧跟的是扩展指令码，在执行```0xCB```指令的时候，实际上执行的是扩展指令操作。

```

      case 0xCB:    // Prefix
            cbPrefix(read8(registers.PC + 1));
            registers.PC += 2;
            registers.cycles += 2;
            break;

```

cbPrefix函数是扩展指令解码和执行的函数。当然扩展指令也是占用1字节，最多也只有256条（0x00-0xFF）。该函数跟cpuCycle函数结构是一样的。

```

void cbPrefix(unsigned char inst)
{    
    unsigned char s;
    unsigned char instruction = inst;
    
    switch (instruction) {
        case 0x00:    // RLC B
            s = (registers.B >> 7);
            registers.B = (registers.B << 1) | s;
            SET_Z(!registers.B);
            SET_N(0);
            SET_H(0);
            SET_C(s);
            break;
        case 0x01:    // RLC C
            s = (registers.C >> 7);
            registers.C = (registers.C << 1) | s;
            SET_Z(!registers.C);
            SET_N(0);
            SET_H(0);
            SET_C(s);
            break;
        ...
        ...
        default:
            printf("Instruction: %02X\n", (int)instruction);
            printf("Undefined instruction.\n");
            break;
        }
}

```

这种设计是很精妙的，通过这种方式可以使8位处理器的指令数量超过256条。

关于GB处理器指令集，可以自行查阅资料（主要关注指令操作、长度和耗费的CPU周期）。

上面基本是完整的CPU模拟示意。剩下的就是自己查阅指令集资料和填充剩下的指令码实现了。ps：指令操作的大部分是寄存器数据，少量是内存数据。操作本身有算数运算、逻辑运算、移位运算、数据传输、跳转、硬件控制等种类。


### 四、做点坏事
---------------------------------

我在GB游戏rom文件定义中，没有找到tile数据。后面找到了一种方法来导出Tetris游戏中的tile图像数据。Tetris游戏执行过程中，当PC等于0x282a的时候，正好是tile被拷贝到vram结束的时候。也就是在CPU模拟中，判断PC等于0x282a的时候，讲整个vram数据导出成文件tile0.bin即可，后面解析tile0.bin文件中的tile数据即可显示tile图像。

在cpuCycle函数中添加下面的代码：

```


    //tile拷贝到vram结束
    if(registers.PC == 0x282a)
    {
        FILE *f = fopen("tile0.bin", "wb");
        fwrite(vram, 16*20*18, 1, f);
        fclose(f);
    }

```

断点进入2次，生成完整tile数据。之后手动停止程序的执行，注释掉该段代码。然后打开下面的lcdCycle和showTiles代码，就可以看到效果。

之后将lcdCycle函数改写成下面的代码：

```

void showTiles(const char* filename, unsigned int* buf);

int lcdCycle(int timeStart)
{
    int end = 0;
    
    unsigned int *buf = sdlPixels();
    //设置SDL背景
    for (int i = 0; i < 160*144; ++i) {
        buf[i] = ((0x01<<24) | (0xFF<<16) | (0x00<<8) | 0xFF);//ARGB
    }
    //显示所有tile
    showTiles("tile0.bin", buf);
    
    sdlUpdateFrame();
    if(sdlUpdateEvent()) end = 1;
    //
    float deltaT = (float)1000 / (59.7) - (float)(SDL_GetTicks() - timeStart);
    if (deltaT > 0) SDL_Delay(deltaT);
    
    return end?0:1;
}

void showTiles(const char* filename, unsigned int* buf)
{
    unsigned char* tiles = malloc(16*20*18*sizeof(unsigned char));
    FILE *f = fopen("tile0.bin", "rb");
    fread(tiles, 16*20*18, sizeof(unsigned char), f);
    fclose(f);
    
    //设置SDL像素
    for (int row = 0; row < 18; ++row) {
        for (int col = 0; col < 20; ++col) {
            unsigned char* start = (tiles+(row*20+col)*16);
            for (int i = 0; i < 8;  ++i) {//行
                unsigned char byte1 = start[2*i];
                unsigned char byte2 = start[2*i+1];
                for (int j = 0; j < 8; ++j) {//列
                    unsigned char mask = 128>>(j%8);
                    unsigned char colour = (!!(byte2&mask)<<1) | !!(byte1&mask);
                    buf[(row*8+i)*160 + (col*8+j)] = colours[colour];
                }
            }
        }
    }
    free(tiles);
}

```

代码自行去看，跟之前显示“Nintendo”的logo方法类似。效果图如下：


![tile图像](http://arvinsfj.github.io/public/ctt/documents/gameboy/vgb_tile.png)


### 五、随便说点
---------------------------------

1. CPU一般需要准确模拟的，因为它是核心硬件。并且涉及到与外设的同步（ppu、apu、中断、时钟等）。
2. CPU的模拟本身结构并不难，难点在于指令数量很多，完全调试正确并不简单。
3. CPU模拟不仅要考虑指令长度，还要考虑指令的CPU周期（时间长度）。
4. 中断会打乱CPU执行的顺序，因为它会修改PC寄存器。并且中断完成能够恢复中断之前的指令执行状态。
5. CPU是什么呢？如何跟图灵机对应起来呢？内存就是纸带，CPU就是读写头（包含一些状态和操作，也就是寄存器和硬件实现的指令操作，加减乘除等）。只不过CPU不会真的在内存中移动，“移动”是通过写入地址进行读写实现的。
6. “存储程序”思想，本质是将数据和操作都看作一样，都是01表示的数据，有区别的是数据是通过PC指针获取的还是其他方法获取的。想想计算机并没有人们想象的那样智能和神奇，并不是魔法。
7. 如果你理解了图灵机，那么计算机相关的东西很多都是图灵机。比如：任何软件，它只不过是一种专用的图灵机。编程语言，也可以看作一种完备的图灵机。虚拟机（语言解释器）也是一样。编程语言和虚拟机本质上是等效的。
8. 任何正确的事物背后都有另外一套正确的逻辑，正反两面无所谓对错，只是人们自己规定了对错。hacking就是正确逻辑背后的逻辑。计算机里只要能运行就OK，没有对错。
9. 这里有很多能延伸的东西，只要你的想象力够丰富。。。。

剩下LCD显示了（NES中称作PPU）。

>END

[DOWNLOAD](documents/gameboy/TestVG_FS.zip)

[tile0.bin](documents/gameboy/tile0.bin)


