
# VGB模拟器的LCD实现

> *作者：Arvin 日期：2018年10月16日*

---------------------------------

>BEGIN

gameboy掌机硬件参考[这里](http://bgb.bircd.org/pandocs.htm)。

LCD显示游戏视频，终于可以看到Tetris游戏的全貌了。

### 一、前言
---------------------------------

GB为了节省电量使用的是四色黑白屏幕（NES显示用的是电视机）。垂直同步59.73 Hz。分辨率是160x144。整个屏幕刷新需要70224个CPU周期，每条扫描线需要465个周期，VBlank需要4560个周期。

### 二、数据定义
---------------------------------

```

FF40 - LCDC - LCD Control (R/W)
  Bit 7 - LCD Display Enable             (0=Off, 1=On)
  Bit 6 - Window Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
  Bit 5 - Window Display Enable          (0=Off, 1=On)
  Bit 4 - BG & Window Tile Data Select   (0=8800-97FF, 1=8000-8FFF)
  Bit 3 - BG Tile Map Display Select     (0=9800-9BFF, 1=9C00-9FFF)
  Bit 2 - OBJ (Sprite) Size              (0=8x8, 1=8x16)
  Bit 1 - OBJ (Sprite) Display Enable    (0=Off, 1=On)
  Bit 0 - BG Display (for CGB see below) (0=Off, 1=On)

LCD控制寄存器

FF41 - STAT - LCDC Status (R/W)
  Bit 6 - LYC=LY Coincidence Interrupt (1=Enable) (Read/Write)
  Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
  Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
  Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
  Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
  Bit 1-0 - Mode Flag       (Mode 0-3, see below) (Read Only)
            0: During H-Blank
            1: During V-Blank
            2: During Searching OAM-RAM
            3: During Transfering Data to LCD Driver

LCD控制状态寄存器


FF42 - SCY - Scroll Y (R/W)
FF43 - SCX - Scroll X (R/W)
Specifies the position in the 256x256 pixels BG map (32x32 tiles) which is to be displayed at the upper/left LCD display position.
Values in range from 0-255 may be used for X/Y each, the video controller automatically wraps back to the upper (left) position in BG map when drawing exceeds the lower (right) border of the BG map area.

屏幕滑动位置寄存器

FF44 - LY - LCDC Y-Coordinate (R)
The LY indicates the vertical line to which the present data is transferred to the LCD Driver. The LY can take on any value between 0 through 153. The values between 144 and 153 indicate the V-Blank period. Writing will reset the counter.

扫描线寄存器（共154条扫描线，144-153是V-Blank，0-143是可视区域扫描线）

FF45 - LYC - LY Compare (R/W)
The gameboy permanently compares the value of the LYC and LY registers. When both values are identical, the coincident bit in the STAT register becomes set, and (if enabled) a STAT interrupt is requested.

比较寄存器。。。

FF4A - WY - Window Y Position (R/W)
FF4B - WX - Window X Position minus 7 (R/W)
Specifies the upper/left positions of the Window area. (The window is an alternate background area which can be displayed above of the normal background. OBJs (sprites) may be still displayed above or behinf the window, just as for normal BG.)
The window becomes visible (if enabled) when positions are set in range WX=0..166, WY=0..143. A postion of WX=7, WY=0 locates the window at upper left, it is then completly covering normal background.

窗口坐标寄存器


FF47 - BGP - BG Palette Data (R/W) - Non CGB Mode Only
This register assigns gray shades to the color numbers of the BG and Window tiles.
  Bit 7-6 - Shade for Color Number 3
  Bit 5-4 - Shade for Color Number 2
  Bit 3-2 - Shade for Color Number 1
  Bit 1-0 - Shade for Color Number 0
The four possible gray shades are:
  0  White
  1  Light gray
  2  Dark gray
  3  Black

背景调色板寄存器

FF48 - OBP0 - Object Palette 0 Data (R/W) - Non CGB Mode Only
This register assigns gray shades for sprite palette 0. It works exactly as BGP (FF47), except that the lower two bits aren't used because sprite data 00 is transparent.

sprite调色板1寄存器

FF49 - OBP1 - Object Palette 1 Data (R/W) - Non CGB Mode Only
This register assigns gray shades for sprite palette 1. It works exactly as BGP (FF47), except that the lower two bits aren't used because sprite data 00 is transparent.

sprite调色板2寄存器

----------------------------

下面是每个OAM属性描述（每个OAM占用4字节）

Sprite attributes reside in the Sprite Attribute Table (OAM - Object Attribute Memory) at $FE00-FE9F. Each of the 40 entries consists of four bytes with the following meanings:

Byte0 - Y Position
Specifies the sprites vertical position on the screen (minus 16).
An offscreen value (for example, Y=0 or Y>=160) hides the sprite.

Byte1 - X Position
Specifies the sprites horizontal position on the screen (minus 8).
An offscreen value (X=0 or X>=168) hides the sprite, but the sprite
still affects the priority ordering - a better way to hide a sprite is to set its Y-coordinate offscreen.

Byte2 - Tile/Pattern Number
Specifies the sprites Tile Number (00-FF). This (unsigned) value selects a tile from memory at 8000h-8FFFh. In CGB Mode this could be either in VRAM Bank 0 or 1, depending on Bit 3 of the following byte.
In 8x16 mode, the lower bit of the tile number is ignored. Ie. the upper 8x8 tile is "NN AND FEh", and the lower 8x8 tile is "NN OR 01h".

Byte3 - Attributes/Flags:
  Bit7   OBJ-to-BG Priority (0=OBJ Above BG, 1=OBJ Behind BG color 1-3)
         (Used for both BG and Window. BG color 0 is always behind OBJ)
  Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
  Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
  Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
  Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
  Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)

```

数据结构：

```
// LCD其他寄存器
struct LCD
{
    int windowX;
    int windowY;
    int scrollX;
    int scrollY;
    int line;
    int lyCompare;
    
    int frame; // 当前绘制的总帧数
};

// LCD Control
struct LCDC
{
    char lcdDisplay; // 7
    char windowTileMap; // 6
    char windowDisplay; // 5
    char tileDataSelect; // 4
    char tileMapSelect; // 3
    char spriteSize; // 2
    char spriteDisplay; // 1
    char bgWindowDisplay; // 0
};

// LCD STAT
struct LCDS
{
    char lyInterrupt;
    char oamInterrupt;
    char vblankInterrupt;
    char hblankInterrupt;
    char lyFlag;
    char modeFlag;
};

// 精灵OAM属性结构
struct sprite
{
    int y;
    int x;
    int patternNum;
    int flags;
};

// LCD内部数据，主要是寄存器结构题变量定义和3个调色板定义

struct LCD LCD;
struct LCDC LCDC;
struct LCDS LCDS;

int bgPalette[] = {3,2,1,0};
int spritePalette1[] = {0, 1, 2, 3};
int spritePalette2[] = {0, 1, 2, 3};
unsigned int colours[4] = {0xFFFFFF, 0xC0C0C0, 0x808080, 0x000000};

```

然后是各个寄存器的操作函数定义：

```

void setLCDC(unsigned char value)
{
    LCDC.lcdDisplay = (!!(value & 0x80));
    LCDC.windowTileMap = (!!(value & 0x40));
    LCDC.windowDisplay = (!!(value & 0x20));
    LCDC.tileDataSelect = (!!(value & 0x10));
    LCDC.tileMapSelect = (!!(value & 0x08));
    LCDC.spriteSize = (!!(value & 0x04));
    LCDC.spriteDisplay = (!!(value & 0x02));
    LCDC.bgWindowDisplay = (!!(value & 0x01));
}

unsigned char getLCDC(void)
{
    return ((LCDC.lcdDisplay << 7) | (LCDC.windowTileMap << 6) | (LCDC.windowDisplay << 5) | (LCDC.tileDataSelect << 4) | (LCDC.tileMapSelect << 3) | (LCDC.spriteSize << 2) | (LCDC.spriteDisplay << 1) | (LCDC.bgWindowDisplay));
}

void setLCDS(unsigned char value)
{
    LCDS.lyInterrupt = (!!(value & 0x40));
    LCDS.oamInterrupt = ((value & 0x20) >> 5);
    LCDS.vblankInterrupt = ((value & 0x10) >> 4);
    LCDS.hblankInterrupt = ((value & 0x08) >> 3);
    LCDS.lyFlag = ((value & 0x04) >> 2);
    LCDS.modeFlag = ((value & 0x03));
}

unsigned char getLCDS(void)
{
    return ((LCDS.lyInterrupt << 6) | (LCDS.oamInterrupt << 5) | (LCDS.vblankInterrupt << 4) | (LCDS.hblankInterrupt << 3) | (LCDS.lyFlag << 2) | (LCDS.modeFlag));
}

void setBGPalette(unsigned char value)
{
    bgPalette[3] = ((value >> 6) & 0x03);
    bgPalette[2] = ((value >> 4) & 0x03);
    bgPalette[1] = ((value >> 2) & 0x03);
    bgPalette[0] = ((value) & 0x03);
}

void setSpritePalette1(unsigned char value)
{
    spritePalette1[3] = ((value >> 6) & 0x03);
    spritePalette1[2] = ((value >> 4) & 0x03);
    spritePalette1[1] = ((value >> 2) & 0x03);
    spritePalette1[0] = 0;
}

void setSpritePalette2(unsigned char value)
{
    spritePalette2[3] = ((value >> 6) & 0x03);
    spritePalette2[2] = ((value >> 4) & 0x03);
    spritePalette2[1] = ((value >> 2) & 0x03);
    spritePalette2[0] = 0;
}

void setScrollX(unsigned char value)
{
    LCD.scrollX = value;
}

unsigned char getScrollX(void)
{
    return LCD.scrollX;
}

void setScrollY(unsigned char value)
{
    LCD.scrollY = value;
}

unsigned char getScrollY(void)
{
    return LCD.scrollY;
}

void setWindowX(unsigned char value)
{
    LCD.windowX = value;
}

void setWindowY(unsigned char value)
{
    LCD.windowY = value;
}

int getLine(void)
{
    return LCD.line;
}

void setLyCompare(unsigned char value)
{
    LCD.lyCompare = (LCD.line == value);
}

```

都是正常定义。


### 三、LCD绘制实现
---------------------------------

GB每条扫描线上最多10个精灵。并且在绘制精灵的时候，数组中的精灵对象需要从左到右排序。

```

void sortSprites(struct sprite* sprite, int c)
{
    // blessed insertion sort
    struct sprite s;
    int i, j;
    for (i = 0; i < c; i++) {
        for (j = 0; j < c-1; j++) {
            if (sprite[j].x < sprite[j+1].x) {
                s = sprite[j+1];
                sprite[j+1] = sprite[j];
                sprite[j] = s;
            }
        }
    }
}

```

sprite是OAM对象数组，c是数组大小（不超过10个）。冒泡排序。

绘制背景函数：

```

void drawBgWindow(unsigned int *buf, int line)
{
    for(int x = 0; x < 160; x++) // for the x size of the window (160x144)
    {
        unsigned int mapSelect, tileMapOffset, tileNum, tileAddr, currX, currY;
        unsigned char buf1, buf2, mask, colour;
        
        if(line >= LCD.windowY && LCDC.windowDisplay && line - LCD.windowY < 144) { // wind
            
            currX = x;
            currY = line - LCD.windowY;
            mapSelect = LCDC.windowTileMap;
            
        } else {
            
            if (!LCDC.bgWindowDisplay) { // background
                buf[line*160 + x] = 0; // if not window or background, make it white
                return;
            }
            currX = (x + LCD.scrollX) % 256; // mod 256 since if it goes off the screen, it wraps around
            currY = (line + LCD.scrollY) % 256;
            mapSelect = LCDC.tileMapSelect;
            
        }
        
        // map window to 32 rows of 32 bytes
        tileMapOffset = (currY/8)*32 + currX/8;
        
        tileNum = read8(0x9800 + mapSelect*0x400 + tileMapOffset);
        if(LCDC.tileDataSelect) {
            tileAddr = 0x8000 + (tileNum*16);
        } else {
            tileAddr = 0x9000 + (((signed int)tileNum)*16); // pattern 0 lies at 0x9000
        }
        
        buf1 = read8(tileAddr + (currY%8)*2); // 2 bytes represent the line
        buf2 = read8(tileAddr + (currY%8)*2 + 1);
        mask = 128>>(currX%8);
        colour = (!!(buf2&mask)<<1) | !!(buf1&mask);
        buf[line*160 + x] = colours[bgPalette[colour]];
    }
}

```

其实是绘制一条背景扫描线。line是将要绘制的扫描线，buf是二维像素点数组。x从0到159遍历每个像素点。函数前面的if判断是确定绘制的绝对curX和curY，以及选择的tilemap（GB内存ram中有2块tilemap）。这里面有个点是，虽然GB显示的分辨率是160x144，但是真实的绘制区域是256x256，也就是讲160x144之外的图像区域是不显示的但会去计算。

映射坐标（curX，curY）到tile的RAM地址。

```
    // map window to 32 rows of 32 bytes
    tileMapOffset = (currY/8)*32 + currX/8;

    tileNum = read8(0x9800 + mapSelect*0x400 + tileMapOffset);
    if(LCDC.tileDataSelect) {
        tileAddr = 0x8000 + (tileNum*16);
    } else {
        tileAddr = 0x9000 + (((signed int)tileNum)*16); // pattern 0 lies at 0x9000
    }

```

首先将屏幕坐标点（curX，curY）映射成8x8的tilemap索引下标tileMapOffset。然后读取该索引下的tile编号tileNum，mapSelect取0或者1，对应的起始地址是0x9800和0x9c00。然后通过tileNum从0x8000或者0x9000（0x8800）开始地址获取tile的真实起始地址tileAddr，每个tile（8x8）占用16字节，每个黑白（4个灰度）像素占用字节的2位，并且高低位是分在连续的2个字节里的（很另类的设计）。

```

    buf1 = read8(tileAddr + (currY%8)*2); // 2 bytes represent the line
    buf2 = read8(tileAddr + (currY%8)*2 + 1);
    mask = 128>>(currX%8);
    colour = (!!(buf2&mask)<<1) | !!(buf1&mask);
    buf[line*160 + x] = colours[bgPalette[colour]];

```

上面的代码是读取屏幕坐标点（curX，curY）的颜色。首先读取2个字节。128  <=> 2^7，也就是1个字节的最高位为1，其他位置为0， ```mask = 128>>(currX%8);```  也就是构建curX位置的掩码mask。注意操作 ```!!``` 将正常数值转成布尔值0和1，特别是非0数值转成1。colour只有最低的2位是有效的，也就是只能取：0、1、2、3，四个值（4个灰度）。最后，将colour映射成具体的RGB颜色值，并赋给像素数组buf中具体的像素。

----------------------------------

绘制精灵函数：

```
void drawSprites(unsigned int *buf, int line, int blocks, struct sprite *sprite)
{
    unsigned int buf1, buf2, tileAddr, spriteRow, x;
    unsigned char mask, colour; int *pal;
    
    for(int i = 0; i < blocks; i++)
    {
        // off screen
        if(sprite[i].x < -7) continue;
        
        spriteRow = sprite[i].flags & 0x40 ? (LCDC.spriteSize ? 15 : 7)-(line - sprite[i].y) : line -sprite[i].y;
        
        // similar to background
        tileAddr = 0x8000 + (sprite[i].patternNum*16) + spriteRow*2;
        
        buf1 = read8(tileAddr);
        buf2 = read8(tileAddr+1);
        
        // draw each pixel
        for(x = 0; x < 8; x++) {
            // out of bounds check
            if((sprite[i].x + x) >= 160) continue;
            
            if((sprite[i].x + x) >= 160) continue;
            
            mask = sprite[i].flags & 0x20 ? 128>>(7-x) : 128>>x;
            colour = ((!!(buf2&mask))<<1) | !!(buf1&mask);
            
            if(colour == 0) continue;
            
            pal = (sprite[i].flags & 0x10) ? spritePalette2 : spritePalette1;
            
            // only render over colour 0
            if(sprite[i].flags & 0x80) {
                unsigned int temp = buf[line*160+(x + sprite[i].x)];
                if(temp != colours[bgPalette[0]]) continue;
            }
            buf[line*160+(x + sprite[i].x)] = colours[pal[colour]]; // for testing
        }
    }
}

```

整体逻辑跟绘制背景差不多。spriteRow是扫描线line在某个精灵（8x8）下的第几行。tileAddr是获取精灵tile的起始地址。后面是读取2个字节（精灵的1行）。

后面的for循环是绘制精灵的1行，8个像素。跟绘制背景差不多的逻辑，通过colour从精灵调色板pal中获取colours中的颜色值，并赋值给对应的buf像素数组。

精灵绘制是从数组sprite中绘制blocks个精灵的第line行扫描线。


--------------------------------

绘制扫描线函数：

```

void renderLine(int line)
{
    int c = 0; // block counter
    struct sprite sprite[10]; // max 10 sprites per line
    unsigned int *buf = sdlPixels();
    int y = 0;
    
    // OAM is divided into 40 4-byte blocks each - corresponding to a sprite
    for (int i = 0; i < 40; i++)
    {
        y = read8(0xFE00 + (i*4)) - 16;
        if (line < y || line >= y + 8 + (8*LCDC.spriteSize)) continue;
        
        sprite[c].y = y;
        sprite[c].x = read8(0xFE00 + (i*4) + 1) - 8;
        sprite[c].patternNum = read8(0xFE00 + (i*4) + 2);
        sprite[c].flags = read8(0xFE00 + (i*4) + 3);
        
        if (++c == 10) break; // max 10 sprites per line
    }
    
    if (c) sortSprites(sprite, c);
    
    drawBgWindow(buf, line);
    drawSprites(buf, line, c, sprite);
}

```

主要是获取扫描线line下的最多10个精灵对象OAM，通过sortSprites函数排序，然后调用drawBgWindow函数绘制背景和调用drawSprites函数绘制精灵。0xFE00是OAM存储区域的起始地址。

--------------------------------

最后看一下LCD循环函数：

```
// lcd循环
int lcdCycle(int timeStart)
{
    int cycles = getCycles();
    static int prevLine;
    
    int this_frame;
    int end = 0;
    
    this_frame = cycles % (70224/4); // 70224 clks per screen
    LCD.line = this_frame / (456/4); // 465 clks per line
    
    if (this_frame < 204/4)
    LCDS.modeFlag = 2;  // OAM
    else if (this_frame < 284/4)
    LCDS.modeFlag = 3;  // VRA
    else if (this_frame < 456/4)
    LCDS.modeFlag = 0;  // HBlank
    
    // Done all lines
    if (LCD.line >= 144)
    LCDS.modeFlag = 1;  // VBlank
    
    if (LCD.line != prevLine && LCD.line < 144) {
        renderLine(LCD.line);
    }
    
    if (LCDS.lyInterrupt && LCD.line == LCD.lyCompare) {
        interrupt.flags |= LCDSTAT;
    }
    
    if (prevLine == 143 && LCD.line == 144) {
        // draw the entire frame
        interrupt.flags |= VBLANK;
        sdlUpdateFrame();
        if(sdlUpdateEvent())
        end = 1;
        float deltaT = (float)1000 / (59.7) - (float)(SDL_GetTicks() - timeStart);
        if (deltaT > 0) SDL_Delay(deltaT);
    }
    
    if (end) return 0;
    
    prevLine = LCD.line;
    
    return 1;
}

```

主要是区分不同的扫描线做不同的事情。首先获取cpu的周期数，第1条扫描线切换模式0，2，3等。大于等于144的扫描线进行V-Blank操作1。

0-143扫描线进行144条扫描线的绘制操作。

后面是进行LCDSTAT和VBLANK中断操作。sdlUpdateFrame函数SDL更新画面，sdlUpdateEvent函数更新SDL事件。SDL_Delay函数将多余的时间还给SDL，保持稳定的60fps。

LCD显示大概就是这样吧。ps：```周期数/4```没看懂。。。。


最后附上一张效果图：


![VGB运行图像](http://arvinsfj.github.io/public/ctt/documents/gameboy/vgb_tetris.png)

--------------------------------

iOS版今天（2018/11/3）移植完成了，如下图：

![VGB运行图像](http://arvinsfj.github.io/public/ctt/documents/gameboy/vgb_ios.png)

[Fork In Github](https://github.com/arvinsfj/gb_ios)


### 四、随便说点
---------------------------------

1. LCD绘制主要是2个绘制函数和扫描线职责分配函数的理解。
2. 注意LCD是怎么跟CPU进行同步的和通信的。
3. 自己可以整理一下LCD是如何从内存中的数据显示出SDL中的像素的。

写完了，如果没有看懂自己去查资料，不要自以为是（低调:）。我之前是玩过NES模拟器的，现在玩GameBoy模拟器感觉坡度还好，不那么迷茫。（GB感觉比NES更简单一点点，黑白的嘛）

>END

[DOWNLOAD](documents/gameboy/TestVG_FS.zip)


