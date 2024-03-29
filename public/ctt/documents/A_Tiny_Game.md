
# 一个简单的游戏原型

> *作者：Arvin 日期：2017年7月27日*

---------------------------------

>BEGIN

游戏有很多类型，我们这里讲的游戏特指2D视频游戏。2D说明只有四个方向可控（上下左右），视频游戏说明有动态图像并且可以通过键盘活着鼠标进行操作。我们这是一个简单的2D射击游戏，简单来讲就是《魂斗乐》类似，而且不实现具体的游戏，只为了演示游戏的一些技术。

### 一、思路
---------------------------------

我们只实现跳跃（上）和左右切换，以及射击（空格键）操作。有一个背景代表游戏角色所处的环境，有一个主角（玩家控制的角色）和一个敌人。弹丸射中敌人，敌人就会消失。第一步我们需要在画布中添加一个背景和一个角色；第二步考虑控制这个角色；第三步考虑射击；第四步考虑敌人。

    
### 二、画布中添加背景和角色
---------------------------------

画布我们采用SDL2实现，背景可以认为是一个大的Texture，角色也是Texture。逻辑上，我们初始化好SDL2后加载两个Texture，并绘制出来。

```
#include <stdio.h>
#include <SDL2/SDL.h>

SDL_Texture* bgTex, *manTex;

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    SDL_Renderer* render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(render, 320, 240);

    SDL_Surface *man_sfa = SDL_LoadBMP("man.bmp");
    if(!man_sfa){
        printf("Cannot find man bmp\n");
        return 1;
    }
    manTex = SDL_CreateTextureFromSurface(render, man_sfa);
    SDL_FreeSurface(man_sfa);

    SDL_Surface *bg_sfa = SDL_LoadBMP("bg.bmp");
    if(!bg_sfa){
        printf("Cannot find bg bmp\n");
        return 1;
    }
    bgTex = SDL_CreateTextureFromSurface(render, bg_sfa);
    SDL_FreeSurface(bg_sfa);

    while(1){
        SDL_SetRenderDrawColor(render, 0, 0, 255, 255);
        SDL_RenderClear(render);
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);

        SDL_RenderCopy(render, bgTex, NULL, NULL);
        SDL_RenderCopy(render, manTex, NULL, NULL);

        SDL_RenderPresent(render);
        SDL_Delay(10);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(render);
    SDL_DestroyTexture(manTex);
    SDL_DestroyTexture(bgTex);

    SDL_Quit();

    return 0;
}

```

上面的代码编译"gcc test1.c -o test1 -framework SDL2"运行"./test1"（mac osx下需要设置SDL2库）。编译和运行都是可以的，但是有很多逻辑问题。
第一，窗口没有关闭按钮；第二，角色图片占满窗口屏幕；第三，角色背景是黑色的需要透明。我们先对第一和第二两个问题进行修正。第一个问题徐岙添加窗口事件处理来修正；第二个问题需要指定角色图片显示的位置和大小。


```
#include <stdio.h>
#include <SDL2/SDL.h>

SDL_Texture* bgTex, *manTex;

int handleEvents(SDL_Window* window)
{
    SDL_Event event;
    int done = 0;

    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_WINDOWEVENT_CLOSE:{
                if(window){
                    SDL_DestroyWindow(window);
                    window = NULL;
                    done = 1;
                }
            }
            break;
            case SDL_KEYDOWN:{
                switch(event.key.keysym.sym){
                    case SDLK_ESCAPE:
                    done = 1;
                    break;
                }
            }
            break;
            case SDL_QUIT:
                done = 1;
            break;
        }
    }

    return done;
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    SDL_Renderer* render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(render, 320, 240);

    SDL_Surface *man_sfa = SDL_LoadBMP("man.bmp");
    if(!man_sfa){
        printf("Cannot find man bmp\n");
        return 1;
    }
    manTex = SDL_CreateTextureFromSurface(render, man_sfa);
    SDL_FreeSurface(man_sfa);

    SDL_Surface *bg_sfa = SDL_LoadBMP("bg.bmp");
    if(!bg_sfa){
        printf("Cannot find bg bmp\n");
        return 1;
    }
    bgTex = SDL_CreateTextureFromSurface(render, bg_sfa);
    SDL_FreeSurface(bg_sfa);

    int done = 0;
    while(!done){
        done = handleEvents(window);//处理窗口事件，ESC按键和关闭按钮都可以关闭程序

        SDL_SetRenderDrawColor(render, 0, 0, 255, 255);
        SDL_RenderClear(render);
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);

        SDL_RenderCopy(render, bgTex, NULL, NULL);
        SDL_Rect srcRect = { 0, 0, 40, 50 };
        SDL_Rect rect = { 50, 60, 40, 50 };
        SDL_RenderCopyEx(render, manTex, &srcRect, &rect, 0, NULL, 0);//指定位置和大小

        SDL_RenderPresent(render);
        SDL_Delay(10);//循环间隔
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(render);
    SDL_DestroyTexture(manTex);
    SDL_DestroyTexture(bgTex);

    SDL_Quit();

    return 0;
}

```

将生成Texture和渲染的代码抽离成函数，以便逻辑清晰和后面修改。

```
#include <stdio.h>
#include <SDL2/SDL.h>

SDL_Texture* bgTex, *manTex;

SDL_Texture* createTex(SDL_Renderer* render, char* filename)
{
    SDL_Texture* tempTex;
    SDL_Surface *sfa = SDL_LoadBMP(filename);
    if(!sfa){
        return NULL;
    }
    tempTex = SDL_CreateTextureFromSurface(render, sfa);
    SDL_FreeSurface(sfa);

    return tempTex;
}

int handleEvents(SDL_Window* window)
{
    SDL_Event event;
    int done = 0;

    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_WINDOWEVENT_CLOSE:{
                if(window){
                    SDL_DestroyWindow(window);
                    window = NULL;
                    done = 1;
                }
            }
            break;
            case SDL_KEYDOWN:{
                switch(event.key.keysym.sym){
                    case SDLK_ESCAPE:
                        done = 1;
                        break;
                }
            }
            break;
            case SDL_QUIT:
                done = 1;
            break;
        }
    }
    return done;
}

void handleRender(SDL_Renderer* render)
{
    SDL_SetRenderDrawColor(render, 0, 0, 255, 255);
    SDL_RenderClear(render);
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);

    SDL_RenderCopy(render, bgTex, NULL, NULL);
    SDL_Rect srcRect = { 0, 0, 40, 50 };
    SDL_Rect rect = { 50, 60, 40, 50 };
    SDL_RenderCopyEx(render, manTex, &srcRect, &rect, 0, NULL, 0);

    SDL_RenderPresent(render);
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    SDL_Renderer* render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(render, 320, 240);

    manTex = createTex(render, "man.bmp");
    if (!manTex) {
        printf("Cannot find man bmp\n");
        return 1;
    }
    bgTex = createTex(render, "bg.bmp");
    if (!bgTex) {
        printf("Cannot find bg bmp\n");
        return 1;
    }

    int done = 0;
    while(!done){
        done = handleEvents(window);

        handleRender(render);

        SDL_Delay(10);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(render);
    SDL_DestroyTexture(manTex);
    SDL_DestroyTexture(bgTex);

    SDL_Quit();

    return 0;
}

```
上面的代码基本满足加载一个角色和一张背景的需求。对于角色黑色背景的问题（需要透明），是由于我们的man.bmp就是这样子的，如果需要可以使用SDL2的扩展SDL2_image库加载png图片。第一步到此为止。接下来考虑让角色动起来（左右移动和跳跃）。

角色在游戏中叫做"精灵"，一般附带动画，比如角色移动的时候要有跑动的动画，射击的时候要有射击的动画等。角色的这种动画，一般使用帧动画实现。还有就是角色和背景需要一定的约束，比如角色的脚不能在背景地面的下面，否则严重影响客观物理规律，给人感觉很假。角色动画使用帧动画，所以我们加载的角色图片应该是一张包含所有关键帧序列的图片；第二种情况涉及简单的物理碰撞实现。


### 三、让角色动起来
---------------------------------

角色动起来有两方面的事情，一个是角色需要移动（位置改变），二个是移动的时候需要切换角色的显示图片（动画）。为了能够实现这两个“动起来”，我们需要将角色抽象成一个结构体，包含位置信息（x，y）和动画信息（关键帧编号aindex和角色序列图片纹理manTex）。最后还需要关注一下纹理问题，由于纹理只有一个朝向的动作图片（只有朝向右边的），朝左的动作怎么处理？这个交给SDL2就行了，有一个纹理左右（上下）切换的方法参数。

首先定义下面的结构体：
```
typedef struct{
    float x, y;
    int aindex;
    SDL_Texture* manTex;
    int faceLeft;//1:朝左 0:朝右
} Man;
```

其次，修改渲染函数handleRender的实现，支持动态获取角色关键帧图片和显示的位置。如下：
```
void handleRender(SDL_Renderer* render, Man* man)
{
    SDL_SetRenderDrawColor(render, 0, 0, 255, 255);
    SDL_RenderClear(render);
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);

    SDL_RenderCopy(render, bgTex, NULL, NULL);
    SDL_Rect srcRect = { 40*man->aindex, 0, 40, 50 };
    SDL_Rect rect = { man->x, man->y, 40, 50 };
    SDL_RenderCopyEx(render, man->manTex, &srcRect, &rect, 0, NULL, man->faceLeft);

    SDL_RenderPresent(render);
}

```

上面的两处修改给角色的动画和移动提供了支持，但是角色不会自己移动和切换动画的，需要我们实现“游戏”逻辑。这个逻辑需要在渲染之前完成，动态的修改角色的位置朝向（x，y，faceLeft）和切换角色的关键帧（aindex）。这种修改需要事件驱动，也就是玩家按下方向键，换句话说，我们在程序中要捕获键盘事件，根据不同的事件修改上面的数值。（这就是所谓的人机交互）

修改事件处理函数handleEvents如下：

```
int handleEvents(SDL_Window* window, Man* man)
{
    SDL_Event event;
    int done = 0;

    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_WINDOWEVENT_CLOSE:{
                if(window){
                    SDL_DestroyWindow(window);
                    window = NULL;
                    done = 1;
                }
            }
            break;
            case SDL_KEYDOWN:{
                switch(event.key.keysym.sym){
                    case SDLK_ESCAPE:
                        done = 1;
                        break;
                }
            }
            break;
            case SDL_QUIT:
                done = 1;
            break;
        }
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if(state[SDL_SCANCODE_LEFT]){
        man->x -= 3;
        man->faceLeft = 1;

        man->aindex++;
        man->aindex %= 4;

    } else if(state[SDL_SCANCODE_RIGHT]) {
        man->x += 3;
        man->faceLeft = 0;

        man->aindex++;
        man->aindex %= 4;

    } else if(state[SDL_SCANCODE_UP]) {
        //man->dy = -8;
    } else if(state[SDL_SCANCODE_DOWN]) {
        //man->y += 10;
    } else {
        man->aindex = 4;
    }

    return done;
}

```
初步实现了左右移动和精灵动画。但是动画切换太快了。其次就是向上跳跃的实现。动画切换太快了，可以适当限制切换的时间间隔（不要每帧都去切换）。跳跃相比左右移动要难一点，主要是不能无限向上移动和需要自由落体下降。跳跃是修改角色的y值，但是会自动下降到地面（我们这里简化成匀速下降，而非自由落体），我们可以在按下上键的时候给一个足够大的向上向量，然后每次游戏循环的时候给一个较小的向下向量，直到落到地面为止。该向量使用dy表示，放在Man结构体里面。向上的向量赋值可以放在事件处理函数里面，向下的向量放在那里呢？这里我们需要抽象一个处理游戏逻辑的函数handleLogic。将处理窗口按键事件函数中的更新角色状态的职能分离出来形成updateManState函数。如下：

```
void updateManState(Man* man)
{
    static int globalTime = 0;
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if(state[SDL_SCANCODE_LEFT]){
        man->x -= 3;
        man->faceLeft = 1;

        if(globalTime % 6 == 0){
            man->aindex++;
            man->aindex %= 4;
        }

    } else if(state[SDL_SCANCODE_RIGHT]) {
        man->x += 3;
        man->faceLeft = 0;

        if(globalTime % 6 == 0){
            man->aindex++;
            man->aindex %= 4;
        }

    } else if(state[SDL_SCANCODE_UP] && man->y == 60) {
        man->dy = -8;//跳跃
    } else if(state[SDL_SCANCODE_DOWN]) {
        //
    } else {
        man->aindex = 4;
    }

    globalTime++;
}

```

handleLogic函数如下：

```
void handleLogic(Man* man)
{
    //实现跳跃
    man->y += man->dy;
    man->dy += 0.5;
    if (man->y > 60) {
        man->y = 60;//落到地面
        man->dy = 0;
    }
}

```

上面的改动可以实现限制精灵动画切换速度和角色跳跃功能。由上面的分析可以知道，游戏的一部分角色状态需要在人机交互事件处理中进行更新，另外一部分需要单独在游戏逻辑处理中处理。

### 四、让角色更加暴力：射击
---------------------------------

射击可以模拟成子弹从枪口位置按照枪口方向（我们这里只考虑水平方向）匀速移动。子弹需要一个纹理（bulletTex），一个位置（x, y），一个方向（dx），最后需要一个数组存储子弹（点射和扫射）。子弹可以作为一个结构体来表示。由于子弹的纹理都一样，没有什么帧动画，可以考虑使用一个纹理。当然射击动作有按下空格键来触发。位置由角色位置作固定偏移来确定，方向由faceLeft决定，速度取固定值，射击的时候角色要抖动。还需要考虑角色移动的时候不能射击这种问题。

```
SDL_Texture* bulletTex;
typedef struct{
    float x, y, dx;
    SDL_Texture* btTex;
} Bullet;
Bullet *bullets[1000] = { NULL };

```

添加和删除子弹函数：

```
void addBullet(float x, float y, float dx) {
    for(int i = 0; i < 1000; i++) {
        if(bullets[i] == NULL) {
            bullets[i] = malloc(sizeof(Bullet));
            bullets[i]->x = x;
            bullets[i]->y = y;
            bullets[i]->dx = dx;
            bullets[i]->btTex = bulletTex;
            break;
        }
    }
}

void subBullet(int i) {
    if(bullets[i]) {
        free(bullets[i]);
        bullets[i] = NULL;
    }
}

```

角色状态更新函数updateManState需要添加按下空格键射击的操作：

```
if(state[SDL_SCANCODE_SPACE] && man->isWalk == 0) {
    //射击
    if(globalTime % 6 == 0) {
        if (man->aindex == 4) {
            man->aindex = 5;
        }else{
            man->aindex = 4;
        }
        if (man->faceLeft == 0) {
            addBullet(man->x+35, man->y+20, 3);
        }else{
            addBullet(man->x+5, man->y+20, -3);
        }
    }
}

```

上面只是添加了子弹，并没有渲染显示子弹，而且没有定义移除子弹的操作。

渲染函数handleRender修改如下：

```
void handleRender(SDL_Renderer* render, Man* man)
{
    SDL_SetRenderDrawColor(render, 0, 0, 255, 255);
    SDL_RenderClear(render);
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);

    SDL_RenderCopy(render, bgTex, NULL, NULL);
    SDL_Rect srcRect = { 40*man->aindex, 0, 40, 50 };
    SDL_Rect rect = { man->x, man->y, 40, 50 };
    SDL_RenderCopyEx(render, man->manTex, &srcRect, &rect, 0, NULL, man->faceLeft);

    //渲染子弹
    for(int i = 0; i < 1000; i++) if(bullets[i]) {
        SDL_Rect rect = { bullets[i]->x, bullets[i]->y, 8, 8 };
        SDL_RenderCopy(render, bullets[i]->btTex, NULL, &rect);
    }

    SDL_RenderPresent(render);
}

```

还需要在每帧的时候修改子弹的水平位置x：

```
void handleLogic(Man* man)
{
    //实现跳跃
    man->y += man->dy;
    man->dy += 0.5;
    if (man->y > 60) {
        man->y = 60;//落到地面
        man->dy = 0;
    }

    //修改子弹的位置
    for(int i = 0; i < 1000; i++) if(bullets[i]) {
        bullets[i]->x += bullets[i]->dx;
        if(bullets[i]->x < -1000 || bullets[i]->x > 1000) {
            subBullet(i);//超出屏幕外就要移除掉子弹
        }
    }
}

```

上面的修改就可以实现角色的射击行为了。是不是有点小激动，接下来需要敌人作为射击靶子。


### 五、需要敌人
---------------------------------

敌人可以理解成由电脑控制的角色（NPC）。作为角色也就是精灵（Sprite）了，跟我们的主角唯一的区别就是它是由电脑控制的，可能还有其他的区别我们这里不作讨论。我们这里的敌人只会等待主角的射击，然后自己消失。子弹碰到敌人，敌人就会死亡消失。这里涉及到简单的碰撞检测和死亡动画。碰撞检测，就是检测两个对象的边缘是否存在交集（重叠）。这里就是子弹的边界跟敌人的边界是否重叠，若重叠则敌人的alive状态设置为0（表示死亡），然后敌人进入死亡动画，动画结束后设置visible状态为0，让敌人消失。死亡动画，就是普通的精灵动画了，上面我们已经接触过了主角的移动动画和射击动画了。

修改Man结构体：

```
typedef struct{
    float x, y, dy;
    int aindex;
    SDL_Texture* manTex;
    int faceLeft;//1:朝左 0:朝右
    int isWalk;//1:在移动 0:没有移动
    int alive;//1:活着 0:死亡
    int visible;//1:显示 0:不显示
} Man;

Man* enemy;//敌人

```

上面的enemy是敌人对象，需要在main函数里面进行初始化。然后还需要在渲染函数handleRender里面进行渲染。

```
//渲染敌人
if (enemy->visible) {
    SDL_Rect eSrcRect = { 40*enemy->aindex, 0, 40, 50 };
    SDL_Rect eRect = { enemy->x, enemy->y, 40, 50 };
    SDL_RenderCopyEx(render, enemy->manTex, &eSrcRect, &eRect, 0, NULL, enemy->faceLeft);
}

```

修改handleLogic函数实现死亡动画：

```
//实现敌人的死亡动画
if(enemy->alive == 0 && globalTime % 6 == 0) {
    if(enemy->aindex < 6) {
        enemy->aindex = 6;
    } else if(enemy->aindex >= 6) {
        enemy->aindex++;
        if(enemy->aindex > 7) {
            enemy->visible = 0;
            enemy->aindex = 7;
        }
    }
}

```

实现简单的碰撞检测：

```
//修改子弹的位置
for(int i = 0; i < 1000; i++) if(bullets[i]) {
    bullets[i]->x += bullets[i]->dx;

    //简单子弹跟敌人的碰撞检测
    if (enemy->alive == 1) {
        if(bullets[i]->x > enemy->x && bullets[i]->x < enemy->x+40 &&
            bullets[i]->y > enemy->y && bullets[i]->y < enemy->y+50){
            enemy->alive = 0;//击中死亡
            bullets[i]->visible = 0;
        }
    }

    if(bullets[i]->x < -1000 || bullets[i]->x > 1000 || bullets[i]->visible == 0) {
        subBullet(i);//超出屏幕外就要移除掉子弹
    }
}

```

注意碰撞检测代码的位置，是在更新子弹位置之后，但是在移除不必要的子弹代码之前。其次需要注意的是只有敌人存在的时候才需要进行碰撞检测。子弹结构体需要添加可见属性visible。

```
typedef struct{
    float x, y, dx;
    SDL_Texture* btTex;
    int visible;//1:显示 0:不显示
} Bullet;

```

上面完成了敌人逻辑，主要是敌人显示、敌人死亡动画和子弹敌人的碰撞检测。OK，整个游戏原型结束。


### 六、随便说点
---------------------------------

做游戏还是比较有趣，挺有意思的。我们从一个静态的场景到角色操作（动态场景），再到射击敌人，整个过程充满了不同问题，解决问题的同时也获得很多乐趣（比较有成就感）。这个原型，各位也可以进一步的添加新元素，比如：敌人的射击策略、多个敌人、不同的子弹、多个场景、网络人人对战等等。

>END

[代码下载](documents/TinyGame.zip)

