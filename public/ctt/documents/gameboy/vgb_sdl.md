
# VGB模拟器窗口SDL

> *作者：Arvin 日期：2018年8月24日*

---------------------------------

>BEGIN

模拟器名称暂定成"VGB"。先实现C版本的模拟器，采用SDL2作为窗口系统（处理显示和按键）。gameboy掌机硬件参考[这里](http://bgb.bircd.org/pandocs.htm)。

### 一、前言
---------------------------------

先做好显示系统，可以方便的调试模拟器，可以及时的看见效果。这个是后面LCD显示的基础。可以理解成初始项目，核心是SDL的一些操作。并且给出了项目的大致框架。

### 二、模拟器基本框架
---------------------------------

代码如下：

```
#include <stdio.h>

#include "lcd.h"

int main(int argc, const char * argv[])
{
    // 组件初始化
    sdlInit();
    
    while (1) {
        unsigned int timeStart = SDL_GetTicks();
        // 组件执行循环
        if (!lcdCycle(timeStart)) break;
    }
    
    // 组件退出清理
    SDL_Quit();
    
    return 0;
}

```

框架大概的逻辑是：初始化组件、组件执行循环和组件退出清理。这里的组件包括：cpu、lcd、timer等。

我们这里简单的实现了一个lcd组件，创建SDL的窗口，退出事件处理等。


### 三、SDL2窗口和事件
---------------------------------

SDL2封装的比较好，简单易用。

数据结构：

```
struct buttons {
    char start;
    char select;
    char a;
    char b;
    
    char up;
    char down;
    char left;
    char right;
};

struct display {
    SDL_Window *window;
    SDL_Surface *surface;
    unsigned int frames;
};

```

上面是gameboy按键的简单封装。下面是SDL显示参数的封装，其中frames是帧率FPS。

--------------------------------

函数分析：

```
// 初始化SDL2
void sdlInit(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    display.window = SDL_CreateWindow("VGB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160, 144, SDL_WINDOW_OPENGL);
    display.surface = SDL_GetWindowSurface(display.window);
    display.frames = 0;
}

```

初始化函数，注意窗口的宽和高分别是：160像素和144像素。这是gameboy显示屏的分辨率。通过surface可以拿到当前SDL窗口的像素而为数组。如下：

```
unsigned int* sdlPixels(void)
{
    return display.surface->pixels;
}

```

注意返回的是int数组指针，int占用4字节，跟ABGR一个像素占用的字节数一样。

拿到界面的像素数组之后，需要根据LCD渲染的结果填充该像素数组。然后，调用SDL2的界面更新函数，刷新界面显示。

```
void sdlUpdateFrame(void)
{
    if (display.frames == 0) {
        gettimeofday(&t1, NULL);
    }
    display.frames++;
    if (display.frames % 1000 == 0) {
        gettimeofday(&t2, NULL);
        printf("FPS: %i\n", display.frames/((int)t2.tv_sec - (int)t1.tv_sec));
    }
    SDL_UpdateWindowSurface(display.window);
}

```

关键是 ```SDL_UpdateWindowSurface(display.window);``` 这句，刷新界面显示。

这里说一句，这两个函数就是VGB模拟器到SDL的显示接口。VGB通过LCD渲染出具体的像素数组，然后传递给SDL，最后调用```sdlUpdateFrame```函数刷新界面。这一切的核心是LCD的渲染像素数组的方法。

```
int lcdCycle(int timeStart)
{
    int end = 0;
    
    unsigned int *buf = sdlPixels();
    for (int i = 0; i < 160*144; ++i) {
        buf[i] = ((0x01<<24) | (0xFF<<16) | (0x00<<8) | 0xFF);//ARGB
    }
    sdlUpdateFrame();
    if(sdlUpdateEvent()) end = 1;
    //
    float deltaT = (float)1000 / (59.7) - (float)(SDL_GetTicks() - timeStart);
    if (deltaT > 0) SDL_Delay(deltaT);
    
    return end?0:1;
}

```

这里是一个简单的LCD渲染方法。先获取SDL的像素数组，然后用(0xFF, 0x00, 0xFF, 1)填充所有 ```160*144``` 个像素。最后调用 ```sdlUpdateFrame();``` 刷新界面。sdlUpdateEvent函数是在处理SDL2的按键事件，映射到gameboy的按键。

剩下的代码是在调整帧率FPS，如果在模拟器执行一次循环之后有多余的时间deltaT，则SDL睡眠一段时间deltaT。当然执行一次循环时间超出了预期（1/60秒）则丢帧（实际情况基本都是丢帧，达不到60fps）。lcdCycle必须在一次循环的最后进行调用。

---------------------------------

按键处理：

```
int sdlUpdateEvent(void)
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        
        if(event.type == SDL_QUIT)
            return 1;
        
        switch (event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_LEFT:
                        buttons.left = 1;
                        break;
                    case SDLK_RIGHT:
                        buttons.right = 1;
                        break;
                    case SDLK_UP:
                        buttons.up = 1;
                        break;
                    case SDLK_DOWN:
                        buttons.down = 1;
                        break;
                    case SDLK_z:
                        buttons.a = 1;
                        break;
                    case SDLK_x:
                        buttons.b = 1;
                        break;
                    case SDLK_a:
                        buttons.start = 1;
                        break;
                    case SDLK_s:
                        buttons.select = 1;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.sym) {
                    case SDLK_LEFT:
                        buttons.left = 0;
                        break;
                    case SDLK_RIGHT:
                        buttons.right = 0;
                        break;
                    case SDLK_UP:
                        buttons.up = 0;
                        break;
                    case SDLK_DOWN:
                        buttons.down = 0;
                        break;
                    case SDLK_z:
                        buttons.a = 0;
                        break;
                    case SDLK_x:
                        buttons.b = 0;
                        break;
                    case SDLK_a:
                        buttons.start = 0;
                        break;
                    case SDLK_s:
                        buttons.select = 0;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

unsigned int getButton(void)
{
    return ((buttons.start * 8) | (buttons.select * 4) | (buttons.b * 2) | buttons.a);
}

unsigned int getDirection(void)
{
    return ((buttons.down * 8) | (buttons.up * 4) | (buttons.left * 2) | buttons.right);
}

```

这里写复杂了。但是代码这样更清晰一点。sdlUpdateEvent定义了映射关系。

附一张执行效果图：

![效果图](http://arvinsfj.github.io/public/ctt/documents/gameboy/vgb_init.png)

### 四、随便说点
---------------------------------

1. 初始项目结构
2. 主要是SDL2的窗口创建和刷新，以及键盘事件处理
3. 这个项目不仅仅是可以作为gameboy模拟器的初始项目，也可以作为其他绘制项目的起点
4. SDL2是可以跨pc平台的

>END

[DOWNLOAD](documents/ameboy/TestVGB.zip)


