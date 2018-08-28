
# VGB内存和ROM

> *作者：Arvin 日期：2018年8月28日*

---------------------------------

>BEGIN

gameboy掌机硬件资料参考[这里](http://bgb.bircd.org/pandocs.htm)。

### 一、前言
---------------------------------

要模拟GB内存（16位地址，64KB内存）映射且能够使用，需要先将ROM文件中的内容拷贝到内存的特定位置（0x0000-0x7FFF）。我们这里暂时只考虑无MBC的情况，只要简单的将ROM内容完全拷贝到该内存区域即可。在文章的最后，我们会讲一点I/O端口相关的知识（joypad）和DMA。

### 二、ROM文件头
---------------------------------

```
The memory at 0100-014F contains the cartridge header. 

This area contains information about the program, its entry point, checksums, information about the used MBC chip, the ROM and RAM sizes, etc. 

Most of the bytes in this area are required to be specified correctly.

```

文件头在ROM文件的0100-014F区域。

下面是文件头的英文描述（重要的信息会有中文解释）：

 The memory at 0100-014F contains the cartridge header.
 
 An internal information area is located at 0100-014F in each cartridge.
 It contains the following values:
 
 0100-0103 - Entry Point
 
 After displaying the Nintendo Logo, the built-in boot procedure jumps to this address (100h), which should then jump to the actual main program in the cartridge. Usually this 4 byte area contains a NOP instruction, followed by a JP 0150h instruction. But not always.

 这个位置的4字节是ROM中游戏程序的入口点。里面的4字节大多数情况包含一条NOP指令和一条 ```JP 0x0150``` 指令。第二条指令会直接跳转到0x150位置执行游戏程序。前面知道ROM文件头到014F结束，而下一字节就是游戏程序了（文件头后面紧跟游戏程序）。该入口程序会在显示完任天堂LOGO（显示和检验无误）之后执行。
 
 0104-0133 - Nintendo Logo
 
 These bytes define the bitmap of the Nintendo logo that is displayed when the gameboy gets turned on.
 The gameboys boot procedure verifies the content of this bitmap (after it has displayed it), and LOCKS ITSELF UP if these bytes are incorrect. A CGB verifies only the first 18h bytes of the bitmap, but others (for example a pocket gameboy) verify all 30h bytes.

 这个地方的0x30个字节是任天堂LOGO的位图数据。这些数据（在cartridge中）会跟GB硬件ROM中的LOGO数据比较，只有完全正确GB才能启动。CGB只会比较前0x18个字节，其他的会比较全部的0x30字节。这个位图每个字节代表8个像素，也就是1bit代表1个像素。bit被设置成1则表示黑色（暗颜色），0表示白色（亮颜色）。GB只有4个灰阶，也就是黑白的。48乘上8等于384个像素，并且LOGO是48*8的分辨率。而且这些像素是按照4*4的块顺序存储的，像素不是按照按行存储的，也就是每2个字节代表一个块，共12*2=24个块。后面会给出解析和显示该logo的C代码。
 
 0134-0143 - Title

 Title of the game in UPPER CASE ASCII. If it is less than 16 characters then the remaining bytes are filled with 00's. When inventing the CGB, Nintendo has reduced the length of this area to 15 characters, and some months later they had the fantastic idea to reduce it to 11 characters only. The new meaning of the ex-title bytes is described below.

 大些的ASCII码游戏标题。最初的ROM占用16字节。CGB占用15字节（多出的1字节用作其他用途）。再后面变成了11字节。我们用Tetris做测试，占用16字节。

 013F-0142 - Manufacturer Code
 
 In older cartridges this area has been part of the Title (see above), in newer cartridges this area contains an 4 character uppercase manufacturer code. Purpose and Deeper Meaning unknown.

 制造商编号。对写模拟器没什么用。
 
 0143 - CGB Flag
 
 In older cartridges this byte has been part of the Title (see above). In CGB cartridges the upper bit is used to enable CGB functions. This is required, otherwise the CGB switches itself into Non-CGB-Mode. Typical values are:
 80h - Game supports CGB functions, but works on old gameboys also.
 C0h - Game works on CGB only (physically the same as 80h).

 CGB标志，高位用于开启CGB功能。典型的值如上。
 
 0144-0145 - New Licensee Code
 
 Specifies a two character ASCII licensee code, indicating the company or publisher of the game. These two bytes are used in newer games only (games that have been released after the SGB has been invented). Older games are using the header entry at 014B instead.

 说明游戏的发布者或者公司（SGB之后的游戏）。老游戏使用0x014B位置的字节代替。
 
 0146 - SGB Flag
 
 Specifies whether the game supports SGB functions, common values are:
 00h = No SGB functions (Normal Gameboy or CGB only game)
 03h = Game supports SGB functions
 The SGB disables its SGB functions if this byte is set to another value than 03h.

 SGB标志，典型的值如上。当该值是非0x03，SGB会自动关闭SGB功能。（也就是要开启SGB功能，该值必须位0x03）。
 
 0147 - Cartridge Type
 
 Specifies which Memory Bank Controller (if any) is used in the cartridge, and if further external hardware exists in the cartridge.
 
```
 00h  ROM ONLY                 13h  MBC3+RAM+BATTERY
 01h  MBC1                     15h  MBC4
 02h  MBC1+RAM                 16h  MBC4+RAM
 03h  MBC1+RAM+BATTERY         17h  MBC4+RAM+BATTERY
 05h  MBC2                     19h  MBC5
 06h  MBC2+BATTERY             1Ah  MBC5+RAM
 08h  ROM+RAM                  1Bh  MBC5+RAM+BATTERY
 09h  ROM+RAM+BATTERY          1Ch  MBC5+RUMBLE
 0Bh  MMM01                    1Dh  MBC5+RUMBLE+RAM
 0Ch  MMM01+RAM                1Eh  MBC5+RUMBLE+RAM+BATTERY
 0Dh  MMM01+RAM+BATTERY        FCh  POCKET CAMERA
 0Fh  MBC3+TIMER+BATTERY       FDh  BANDAI TAMA5
 10h  MBC3+TIMER+RAM+BATTERY   FEh  HuC3
 11h  MBC3                     FFh  HuC1+RAM+BATTERY
 12h  MBC3+RAM

```

 说明使用的MBC类型和其他外部硬件。典型的值如上。非常重要。Tetris该值是0x00，也就是ROM ONLY。后面可以进行扩展，以支持其他类型的ROM。
 
 0148 - ROM Size

 Specifies the ROM Size of the cartridge. Typically calculated as "32KB shl N".

```
 00h -  32KByte (no ROM banking)
 01h -  64KByte (4 banks)
 02h - 128KByte (8 banks)
 03h - 256KByte (16 banks)
 04h - 512KByte (32 banks)
 05h -   1MByte (64 banks)  - only 63 banks used by MBC1
 06h -   2MByte (128 banks) - only 125 banks used by MBC1
 07h -   4MByte (256 banks)
 52h - 1.1MByte (72 banks)
 53h - 1.2MByte (80 banks)
 54h - 1.5MByte (96 banks)

```

 cartridge的ROM大小。计算方式是“32KB左移N”，N是该值。Tetris该值是0x00，也就是32KB。
 
 0149 - RAM Size
 
 Specifies the size of the external RAM in the cartridge (if any).
 
```
 00h - None
 01h - 2 KBytes
 02h - 8 Kbytes
 03h - 32 KBytes (4 banks of 8KBytes each)

```
 
 When using a MBC2 chip 00h must be specified in this entry, even though the MBC2 includes a built-in RAM of 512 x 4 bits.

 外部RAM的大小。Tetris该值是0x00，也就是无外部RAM。MBC2的时候，该值必须位0x00，即使MBC2包含内建的512*4位大小的RAM。
 
 014A - Destination Code

 Specifies if this version of the game is supposed to be sold in japan, or anywhere else. Only two values are defined.
 00h - Japanese
 01h - Non-Japanese

 发售地区标志。没什么用。ps：仅作日本跟非日本的区分，有“大日本”的嫌疑。
 
 014B - Old Licensee Code
 
 Specifies the games company/publisher code in range 00-FFh. A value of 33h signalizes that the New License Code in header bytes 0144-0145 is used instead.
 (Super GameBoy functions won't work if <> $33.)

 老游戏（SGB之前的游戏）用于说明游戏的发布者或者公司。如果该值为0x33，则用0144-0145区域的字节代替。SGB该字段必须为0x33.
 
 014C - Mask ROM Version number
 
 Specifies the version number of the game. That is usually 00h.

 游戏版本编号。通常是0x00。
 
 014D - Header Checksum
 
 Contains an 8 bit checksum across the cartridge header bytes 0134-014C. The checksum is calculated as follows:
 
 x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT
 
 The lower 8 bits of the result must be the same than the value in this entry. The GAME WON'T WORK if this checksum is incorrect.

 文件头检验值。计算公式如上。在GB中，如果检验没通过则不能启动。检验的是x的最后一个字节的值是否跟该位置的值相等。
 
 014E-014F - Global Checksum
 
 Contains a 16 bit checksum (upper byte first) across the whole cartridge ROM. Produced by adding all bytes of the cartridge (except for the two checksum bytes). The Gameboy doesn't verify this checksum.

 全局校验值。计算方式是对整个ROM字节进行求和（除去该位置的2个字节），得到的值的最后两个字节跟该位置的2个字节进行比较。
 GB不会验证该校验值。

--------------------------------

由于我们使用的Tetris是无MBC的，ROM的内容是直接拷贝到0000-7FFF区域的。

ROM数据结构定义如下：

```
struct rom {
    unsigned char *romBytes; // ROM原始数据
    char gameTitle[17]; // 游戏标题
    int romType; // MBC类型
    int romSize; // ROM大小
    int ramSize; // 外部RAM大小
};

#define ROM_TITLE_OFFSET 0x134
#define ROM_TYPE_OFFSET 0x147
#define ROM_SIZE_OFFSET 0x148
#define ROM_RAM_OFFSET 0x149
#define HEADER_SIZE 0x14F

```

这个地方其实只是为了记录一下ROM的元数据，方便以后扩展。该次使用的是无MBC的Tetris，是可以不创建该数据结构的。5个宏定义，是定义了rom结构体各个字段在ROM中的读取地址。

读取函数如下：

```
void romInit(const char* filename)
{
    long len = 0, i = 0;
    FILE *file;
    unsigned char header[HEADER_SIZE];
    
    file = fopen(filename,"rb");
    fseek(file, 0, SEEK_END);
    len = ftell(file); 
    
    //读取整个Cartridge到romBytes中
    rom.romBytes = malloc(len);
    rewind(file);
    fread(rom.romBytes, len, 1, file);
    
    //读取Cartridge Header
    rewind(file);
    fread(header, 1, HEADER_SIZE, file);//Cartridge Header size: 0x14F
    
    rom.romType = header[ROM_TYPE_OFFSET];//Cartridge Type
    for (i = 0; i<16; i++) {
        rom.gameTitle[i] = header[ROM_TITLE_OFFSET+i];//Game Title
    }
    rom.romSize = header[ROM_SIZE_OFFSET];//ROM size
    rom.romSize = pow(2,rom.romSize+1) * 16;
    
    rom.ramSize = header[ROM_RAM_OFFSET];//External RAM size
    rom.ramSize = pow(4, rom.ramSize)/2;
    
    printf("Title: %s\n", rom.gameTitle);
    printf("MBC: %d\n", rom.romType);
    printf("ROM SIZE: %d KB\n", rom.romSize);
    printf("ERAM SIZE: %d KB\n\n", rom.ramSize);
    
    //检验rom
    checkRom(rom.romBytes, len);
    
    //由于MMC为00的情况，可以直接拷贝ROM内容到0000-7FFF区域
    memcpy(&cart[0x0000], &rom.romBytes[0x0000], 0x8000);
    
    fclose(file);
}

```

正常的文件读取方法。只不过最后讲读取的ROM数据拷贝到了内存cart中。cart代表内存的0000-7FFF区域。

checkRom方法是根据上面的ROM头部说明进行的校验和显示，有兴趣的可以自己看看。ps：Tetris的全局校验值不知道为什么不正确。

上面提到的解析和显示任天堂LOGO的函数：

```
void showLogo(unsigned char* rom, unsigned int* buf)
{
    int length = 0x30*8*sizeof(char);
    unsigned char* logo = malloc(length);
    memset(logo, 0, length);
    //0104-0133 - Nintendo Logo
    for (int i = 0; i < 0x30; ++i) {
        char byte = rom[0x0104+i];
        for (int j = 0; j < 8; ++j) {
            logo[i*8+(7-j)] = (byte & (1 << j))? 0x50 : 0xFF;//注意像素位置跟位的位置是相反的，高位在低位置像素上。
        }
    }
    
    //设置SDL像素
    int x = 0, y = 0;
    for (int i = 0; i < 24; ++i) {
        x = (i * 4) % 48;
        y = ((i * 4) / 48) * 4;
        for (int j = 0; j < 16; ++j) {
            unsigned char byte = logo[i*16+j];
            buf[x+(j%4)+(y+(j/4))*160] = ((0x01<<24) | (byte<<16) | (byte<<8) | byte);//ARGB
        }
    }
    free(logo);
}

```

在lcdCycle函数中调用一下该函数即可显示LOGO。注意0104-0133区域位图的解析成SDL2像素方法。注意位图数据是由4x4像素块顺序组成的。

附一张执行效果图：

![LOGO执行图](http://arvinsfj.github.io/public/ctt/documents/gameboy/vgb_logo.png)


### 三、GB内存映射
---------------------------------

```
0000-3FFF   16KB ROM Bank 00     (in cartridge, fixed at bank 00)
4000-7FFF   16KB ROM Bank 01..NN (in cartridge, switchable bank number)
8000-9FFF   8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
A000-BFFF   8KB External RAM     (in cartridge, switchable bank, if any)

C000-CFFF   4KB Work RAM Bank 0 (WRAM)
D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)

FE00-FE9F   Sprite Attribute Table (OAM)
FEA0-FEFF   Not Usable
FF00-FF7F   I/O Ports
FF80-FFFE   High RAM (HRAM)
FFFF        Interrupt Enable Register

```

如上所示。0000-7FFF是ROM区域在cartridge中，只读不能写。8000-9FFF是VRAM区域，在GB中。A000-BFFF是外部RAM，在cartridge中（如果有的话）。C000-DFFF是WRAM区域，在GB中。E000-FDFF区域是内存C000-DDFF区域的镜像。FE00-FE9F区域是精力属性表，也叫做OAM。FEA0-FEFF没有使用。FF00-FF7F是I/O端口映射，是CPU跟其他硬件通信的接口，比如CPU写0xFF00字节则会选择joypad的按钮键或者方向键，读取0xFF00则会读取joypad的按键值（按钮键或者方向键的那个键被按下或者弹起了），I/O端口就是这种作用。FF80-FFFE高端RAM，暂时不知道用途。最后一个位置0xFFFF是中断开启的寄存器。

数据结构定义如下：

```
unsigned char cart[0x8000];   // ROM (Cart 1 & 2)
unsigned char vram[0x2000];  // video RAM
unsigned char sram[0x2000];  // switchable RAM
unsigned char wram[0x2000];  // working RAM
unsigned char oam[0x100];     // Sprite Attribute Memory
unsigned char io[0x100];     // Input/Output - Not sure if I need 0x100, 0x40 may suffice
unsigned char hram[0x80];    // High RAM

```

当然你也可以开辟一个64KB大小的字节数组来表示内存（这样不够直观）。注意上面数组的大小不一定跟内存各段的大小一致，但是不小于各段大小。下面是读写内存的映射方法：

读取函数：

```
unsigned char read8(unsigned short address)
{
    unsigned char mask = 0;
    
    if (0x0000 <= address && address <= 0x7FFF)
        return cart[address];
    else if (0x8000 <= address && address <= 0x9FFF)
        return vram[address - 0x8000];
    else if (0xA000 <= address && address <= 0xBFFF)
        return sram[address - 0xA000];
    else if (0xC000 <= address && address <= 0xDFFF)
        return wram[address - 0xC000];
    else if (0xE000 <= address && address <= 0xFDFF) //echo of wram
        return wram[address - 0xE000];
    else if (0xFE00 <= address && address <= 0xFEFF)
        return oam[address - 0xFE00];
    else if (address == 0xFF00) {
        if (!jb) mask = getButton();
        if (!jd) mask = getDirection();
        return (0xC0 | (0xF ^ mask) | ((jb) | (jd)));
    }
    else if (address == 0xFF04)
        ;//return getDiv();
    else if (address == 0xFF05)
        ;//return getTima();
    else if (address == 0xFF06)
        ;//return getTma();
    else if (address == 0xFF04)
        ;//return getTac();
    else if (address == 0xFF0F)
        ;//return interrupt.flags;
    else if (address == 0xFF40)
        ;//return getLCDC();
    else if (address == 0xFF41)
        ;//return getLCDS();
    else if (address == 0xFF42)
        ;//return getScrollY();
    else if (address == 0xFF43)
        ;//return getScrollX();
    else if (address == 0xFF44)
        ;//return getLine();
    else if(0xFF00 <= address && address <= 0xFF7F) // maybe only up to 0xFF4F
        return io[address - 0xFF00];
    else if (0xFF80 <= address && address <= 0xFFFE)
        return hram[address - 0xFF80];
    else if (address == 0xFFFF)
        return interrupt.enable;
    
    return 0;
}

unsigned short read16(unsigned short address)
{
    return (read8(address) | (read8(address+1) << 8));
}

```

首先，分为8位读取和16位读取，因为GB中指令是8位的，地址是16位的。现阶段我们只看前面部分：

```
if (0x0000 <= address && address <= 0x7FFF)
        return cart[address];
    else if (0x8000 <= address && address <= 0x9FFF)
        return vram[address - 0x8000];
    else if (0xA000 <= address && address <= 0xBFFF)
        return sram[address - 0xA000];
    else if (0xC000 <= address && address <= 0xDFFF)
        return wram[address - 0xC000];
    else if (0xE000 <= address && address <= 0xFDFF) //echo of wram
        return wram[address - 0xE000];
    else if (0xFE00 <= address && address <= 0xFEFF)
        return oam[address - 0xFE00];

```

就是简单的将真实的地址映射到各个对应数组下标。注意cart是只读的。


内存写函数：

```
void write8(unsigned short address, unsigned char value)
{
    // can't write to ROM
    if (0x8000 <= address && address <= 0x9FFF)
        vram[address - 0x8000] = value;
    else if (0xA000 <= address && address <= 0xBFFF)
        sram[address - 0xA000] = value;
    else if (0xC000 <= address && address <= 0xDFFF)
        wram[address - 0xC000] = value;
    else if (0xE000 <= address && address <= 0xFDFF)
        wram[address - 0xE000] = value;
    else if (0xFE00 <= address && address <= 0xFEFF)
        oam[address - 0xFE00] = value;
    else if (address == 0xFF04)
        ;//setDiv(value);
    else if (address == 0xFF05)
        ;//setTima(value);
    else if (address == 0xFF06)
        ;//setTma(value);
    else if (address == 0xFF04)
        ;//setTac(value);
    else if (address == 0xFF40)
        ;//setLCDC(value);
    else if (address == 0xFF41)
        ;//setLCDS(value);
    else if (address == 0xFF42)
        ;//setScrollY(value);
    else if (address == 0xFF43)
        ;//setScrollX(value);
    else if (address == 0xFF45)
        ;//setLyCompare(value);
    else if(address == 0xff46){
        for(int i = 0; i < 160; i++) write8(0xfe00 + i, read8((value << 8) + i));
    }
    else if (address == 0xFF47)
        ;//setBGPalette(value);
    else if (address == 0xFF48)
        ;//setSpritePalette1(value);
    else if (address == 0xFF49)
        ;//setSpritePalette2(value);
    else if (address == 0xFF4A)
        ;//setWindowY(value);
    else if (address == 0xFF4B)
        ;//setWindowX(value);
    else if (address == 0xFF00) {
        jb = value & 0x20; jd = value & 0x10; 
    }
    else if(0xFF00 <= address && address <= 0xFF7F)
        io[address - 0xFF00] = value;
    else if (0xFF80 <= address && address <= 0xFFFE)
        hram[address - 0xFF80] = value;
    else if (address == 0xFF0F)
        interrupt.flags = value;
    else if (address == 0xFFFF)
        interrupt.enable = value;
}

void write16(unsigned short address, unsigned short value)
{
    write8(address,(value & 0x00FF));
    write8(address+1,(value & 0xFF00) >> 8);
}

```

一样的道理，分为8位和16位写函数。cart是不能写的。地址映射到数组下标然后数组元素赋值。

初始化方法：

```
void memInit(void)
{
    write8(0xFF10, 0x80);
    write8(0xFF11, 0xBF);
    write8(0xFF12, 0xF3);
    write8(0xFF14, 0xBF);
    write8(0xFF16, 0x3F);
    write8(0xFF19, 0xBF);
    write8(0xFF1A, 0x7F);
    write8(0xFF1B, 0xFF);
    write8(0xFF1C, 0x9F);
    write8(0xFF1E, 0xBF);
    write8(0xFF20, 0xFF);
    write8(0xFF23, 0xBF);
    write8(0xFF24, 0x77);
    write8(0xFF25, 0xF3);
    write8(0xFF26, 0xF1);
    write8(0xFF40, 0x91);
    write8(0xFF47, 0xFC);
    write8(0xFF48, 0xFF);
    write8(0xFF49, 0xFF);
}

```

这些地址和值是哪来的？[pandocs](http://bgb.bircd.org/pandocs.htm)的 **Power Up Sequence** 节有说明。这里是硬编码，应该能满足大部分情况。全部是其他硬件的寄存器（I/O端口）初始化。

### 四、补充说明：joypad和DMA
---------------------------------

joypad是GB的用户操作接口。I/O端口地址是0xFF00。该端口可以读写。

The eight gameboy buttons/direction keys are arranged in form of a 2x4 matrix. Select either button or direction keys by writing to this register, then read-out bit 0-3.

```
  Bit 7 - Not used
  Bit 6 - Not used
  Bit 5 - P15 Select Button Keys      (0=Select)
  Bit 4 - P14 Select Direction Keys   (0=Select)
  Bit 3 - P13 Input Down  or Start    (0=Pressed) (Read Only)
  Bit 2 - P12 Input Up    or Select   (0=Pressed) (Read Only)
  Bit 1 - P11 Input Left  or Button B (0=Pressed) (Read Only)
  Bit 0 - P10 Input Right or Button A (0=Pressed) (Read Only)

```

Note: Most programs are repeatedly reading from this port several times (the first reads used as short delay, allowing the inputs to stabilize, and only the value from the last read actually used).

如果向0xFF00地址写数据，则根据写入的数值来记录选择ButtonKeys或者DirectionKeys组别。之后从0xFF00读取的数据就是该组别的键值（那个键被按下了）。读取的时候，返回1字节数据，该字节各个位bit的意义如上所示。注意，选择或者按下使用0表示。而且组别选择是单选的。bit6和bit7不使用补1即可。

读写取函数：

```
读取

else if (address == 0xFF00) {
        if (!jb) mask = getButton();
        if (!jd) mask = getDirection();
        return (0xC0 | (0xF ^ mask) | ((jb) | (jd)));
    }

写入

else if (address == 0xFF00) {
        jb = value & 0x20; jd = value & 0x10; 
    }

```

写入的时候，根据写入字节的5、6位来确定选择的组别。如果value的bit4（第5位）为0则选中方向键组别，此后读取的数据是方向键（上下左右）。如果value的bit5（第6位）位0则选中按钮组别，此后读取的数据是按钮键（A、B、start、select）。

最后提一下，joypad中断号是0x60。一个按钮按下或者释放的时候会触发该中断。

---------------------------------

在GB中存在DMA这种数据快速传输方式，一般只用于LCD OAM数据传输。CGB还有LCD VRAM数据传输，这里我们不考虑。

DMA端口是0xFF46。只能写不能读取。写入的数据value是ROM或者RAM中的传输源起始地址。那么传输的大小是多少呢？160字节。目标地址是0xFE00。

Writing to this register launches a DMA transfer from ROM or RAM to OAM memory (sprite attribute table). The written value specifies the transfer source address divided by 100h, ie. source & destination are:

```
  Source:      XX00-XX9F   ;XX in range from 00-F1h
  Destination: FE00-FE9F

```
It takes 160 microseconds until the transfer has completed (80 microseconds in CGB Double Speed Mode), during this time the CPU can access only HRAM (memory at FF80-FFFE). For this reason, the programmer must copy a short procedure into HRAM, and use this procedure to start the transfer from inside HRAM, and wait until the transfer has finished:

```
   ld  (0FF46h),a ;start DMA transfer, a=start address/100h
   ld  a,28h      ;delay...
  wait:           ;total 5x40 cycles, approx 200ms
   dec a          ;1 cycle
   jr  nz,wait    ;4 cycles

```
Most programs are executing this procedure from inside of their VBlank procedure, but it is possible to execute it during display redraw also, allowing to display more than 40 sprites on the screen (ie. for example 40 sprites in upper half, and other 40 sprites in lower half of the screen).

这里提到了HRAM的一个用途，就是DMA传输时，CPU只能访问HRAM内存区域。开启和等待DMA传输只能是处在HRAM中的程序。而且DMA传输一般发生在VBlank期间（当然也可以是LCD重绘期间）。

写函数：

```
else if(address == 0xff46){
        for(int i = 0; i < 160; i++) write8(0xfe00 + i, read8((value << 8) + i));
    }

```

传输160字节到0xfe00开始的区域。注意源地址的构建方式。


### 五、随便说点
---------------------------------

1. 英语是IT行业的基本技能之一，好的资料或者优秀开发者基本都来自英语国家
2. 文件结构分析（或者协议分析）是编程能力的一部分，根据文件结构构建对应的数据结构非常重要
3. 内存模拟相对简单，定义好内存数据存储区域，然后提供相应的读写方法即可，其中要注意的是I/O映射部分，它不是常规的内存读写
4. 注意GB中joypad和dma实现的方法，主要还是I/O端口操作，这种编程模式在其他平台也会出现的（将端口地址映射到内存地址）
5. 接下来实现中断和timer组件 D:)

>END

[DOWNLOAD](documents/gameboy/ROM_MEM.zip)


