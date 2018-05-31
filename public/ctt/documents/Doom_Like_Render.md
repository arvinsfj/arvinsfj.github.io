
# Doom风格的软渲染器

> *作者：Arvin 日期：2018年02月27日*

---------------------------------

>BEGIN

春节经历不少事情，爷爷的过世（2月13日），感冒，撕心裂肺的急性扁桃体发炎。。。到现在一切总算过去了，静下心来写春节过后的第一篇文章。一直想写关于软渲染方面的文章，自己也写过简单的软渲染器，基于三角形的软渲染，比较通用但是由于性能问题不怎么实用，类似Demo吧。去年偶尔在Youtube上看到大神写了个Doom风格的渲染器，研究了一下代码，感觉有一定的实用性，并且非常简单，易于理解。所以这篇主要还是基于大神的那个渲染器进行分析和讲解。（基于三角形的软渲染，有机会再说吧）。

### 一、思路
---------------------------------

渲染的本质是将计算好的颜色值写入对应的像素点。如何计算颜色值是渲染的关键。像素点的颜色值计算涉及到光照、纹理、材质等。我们这里为了简化，假定物体本身会发出固定的光线（物体本身有颜色值）。我们只关注如何计算和填充像素点。视频是由一帧一帧的图片连续播放形成的，我们要讲的是实时渲染器，也就是通过渲染器我们能产生视频。（视频游戏都是实时渲染的）

### 二、几个宏的定义
---------------------------------

```
#define min(a,b) (((a) < (b)) ? (a) : (b))
```
返回两个数值的较小者。注意每个数值都使用小括号括起来，具有更好的通用性，以免在使用该宏的时候产生歧义。还有一个特点是a和b必须是数值（可以使用关系运算），但对数值的类型没什么要求，可以是整数，也可以是实数。

```
#define max(a,b) (((a) > (b)) ? (a) : (b))
```
返回两个数值的较大者。

```
#define clamp(a, mi,ma) min(max(a,mi),ma)
```
返回在mi和ma之间的a数值。如果a小于mi，则返回mi；如果a大于ma，则返回ma；如果a在mi和ma之间，则返回a本身。

```
#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
```
判断范围[a0,a1]跟范围[b0,b1]是否存在重叠部分。关键判断是后面的关系表达式。

```
#define IntersectBox(x0,y0, x1,y1, x2,y2, x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))
```
判断两个2D盒子（矩形）是否存在重叠部分。注意(x0,y0)和(x1,y1)是第一个矩形的对角线两点的坐标。

```
#define vxs(x0,y0, x1,y1) ((x0)*(y1) - (x1)*(y0))
```
返回向量(x0,y0)与向量(x1,y1)的外积在Z轴的数值。根据向量的运算公式得到。

```
#define PointSide(px,py, x0,y0, x1,y1) vxs((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0))
```
判断点(px,py)在线段(x0,y0)--(x1,y1)的上面或者下面。返回值：<0 点在直线下面, =0 点在直线上, >0 点在直线上面。向量外积Z轴分量数值的正负表示向量的旋转方向（右手法则）。仔细理解一下，有点难。

```
#define Intersect(x1,y1, x2,y2, x3,y3, x4,y4) ((struct xy) { \
vxs(vxs(x1,y1, x2,y2), (x1)-(x2), vxs(x3,y3, x4,y4), (x3)-(x4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)), \
vxs(vxs(x1,y1, x2,y2), (y1)-(y2), vxs(x3,y3, x4,y4), (y3)-(y4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)) })
```
返回两条直线的交点坐标。分别计算交点的x和y即可。公式自己展开分析。比较复杂。

### 三、两个实体的定义
---------------------------------

两个实体分别是房间和玩家。房间我们用多边形区域来表示，玩家需要记录空间坐标和所在多边形区域，以及head的旋转角度等。

```
static struct sector
{
    float floor, ceil;
    struct xy { float x,y; } *vertex;
    signed char *neighbors;
    unsigned npoints;
} *sectors = NULL;
static unsigned NumSectors = 0;
```
floor和ceil分别代表房间的地板和天花板；vertex结构体xy指针代表房间多边形的顶点列表；neighbors代表跟其他房间的公共边（墙体）列表；npoints代表多边形顶点列表顶点数量；sectors是sector结构体指针代表多边形（房间）列表；NumSectors代表多边形列表多边形（房间）数量。

```
static struct player
{
    struct xyz { float x,y,z; } where, velocity;
    float angle, anglesin, anglecos, yaw;
    unsigned sector;
} player;
```
where代表玩家的空间坐标点；velocity代表玩家的空间方向上的速度；angle, anglesin, anglecos, yaw代表玩家头部的旋转角度；sector代表玩家当前所在的多边形区域（房间）；结构体player变量代表玩家实体。

从上面可以看出，可以有多个房间，但是玩家某一时刻只在特定单个房间内（具有性能优化价值）。单个房间的构成是由多边形在地板和天花板上的正交投影构成。公共边（公共墙体）区分了玩家所在房间的切换。玩家具有空间坐标和空间速度，以及玩家头部的视线参数。

有了2个实体的内存表示，接下来是实体数据的初始化，也就是实体数据的加载。

不要忘记屏幕像素点二维数组：

```
unsigned int screen_pixels[H][W] = {0};
```

### 四、房间和玩家数据的本地加载
---------------------------------

本地数据文件格式如下：

```
vertex	0	0 6 28
vertex	2	1 17.5
vertex	5	4 6 18 21
vertex	6.5	9 11 13 13.5 17.5
vertex	7	5 7 8 9 11 13 13.5 15 17 19 21
vertex	7.5	4 6
vertex	10.5	4 6
vertex	11	5 7 8 9 11 13 13.5 15 17 19 21
vertex	11.5	9 11 13 13.5 17.5
vertex	13	4 6 18 21
vertex	16	1 17.5
vertex	18	0 6 28

sector	0 20	 3 14 29 49             -1 1 11 22 
sector	0 20	 17 15 14 3 9           -1 12 11 0 21 
sector	0 20	 41 42 43 44 50 49 40   -1 20 -1 3 -1 -1 22 
sector	0 14	 12 13 44 43 35 20      -1 21 -1 2 -1 4 
sector	0 12	 16 20 35 31            -1 -1 3 -1 
sector	16 28	 24 8 2 53 48 39        18 -1 7 -1 6 -1 
sector	16 28	 53 52 46 47 48         5 -1 8 10 -1 
sector	16 28	 1 2 8 7 6              23 -1 5 -1 10 
sector	16 36	 46 52 51 45            -1 6 -1 24 
sector	16 36	 25 26 28 27            24 -1 10 -1 
sector	16 26	 6 7 47 46 28 26        -1 7 -1 6 -1 9 
sector	2 20	 14 15 30 29            0 1 12 22 
sector	4 20	 15 17 32 30            11 1 13 22 
sector	6 20	 17 18 33 32            12 -1 14 -1 
sector	8 20	 18 19 34 33            13 19 15 20 
sector	10 24	 19 21 36 34            14 -1 16 -1 
sector	12 24	 21 22 37 36            15 -1 17 -1 
sector	14 28	 22 23 38 37            16 -1 18 -1 
sector	16 28	 23 24 39 38            17 -1 5 -1 
sector	8 14	 10 11 19 18            -1 21 -1 14 
sector	8 14	 33 34 42 41            -1 14 -1 2 
sector	0 20	 4 13 12 11 10 9 3      -1 -1 3 -1 19 -1 1 
sector	0 20	 29 30 32 40 49         0 11 12 -1 2 
sector	16 36	 1 6 5 0                -1 7 -1 24 
sector	16 36	 0 5 25 27 45 51        -1 23 -1 9 -1 8 

player	2 6	0	0
```
上面除了最后一行是玩家数据，其他的都是房间数据定义。房间数据分为顶点数据定义和多边形数据定义。顶点采用共用单个y值，多个x值的方式定义，具有一定的压缩作用。
多边形前2条数据分别是地板和天花板的值；接下来的数据代表边的顶点数组下标和公共边下标（-1代表非公共边），前一半是顶点下标，后一半是公共边下标。注意实际的多边形是首尾相接的，在处理上多边形的顶点数组需要多增加一个顶点并且保证数组的第一个元素和最后一个元素的数值相等。

玩家数据分别定义了玩家的空间x，y值，angle角度和所在房间（多边形）。

```
static void loadData()
{
    FILE* fp = fopen("map-clear.txt", "rt");
    if (!fp) {
        perror("map-clear.txt");
        exit(0);
    }
    char buf[256], word[256], *ptr;
    struct xy* vert = NULL, v;
    int n, m, NumVertices = 0;
    //
    while (fgets(buf, sizeof buf, fp)) {
        switch(sscanf(ptr = buf, "%32s%n", word, &n) == 1 ? word[0] : '\0'){
            case 'v':{
                for (sscanf(ptr+=n, "%f%n", &v.y, &n); sscanf(ptr+=n, "%f%n", &v.x, &n) == 1; ) {
                    vert = realloc(vert, ++NumVertices*sizeof(*vert));
                    vert[NumVertices-1] = v;
                }
                break;
            }
            case 's':{
                sectors = realloc(sectors, ++NumSectors*sizeof(*sectors));
                struct sector* sect = &sectors[NumSectors-1];
                int* num = NULL;
                sscanf(ptr += n, "%f%f%n", &sect->floor,&sect->ceil, &n);
                for(m=0; sscanf(ptr += n, "%32s%n", word, &n) == 1 && word[0] != '#'; ){
                    num = realloc(num, ++m * sizeof(*num));
                    num[m-1] = word[0]=='x' ? -1 : atoi(word);
                }
                sect->npoints   = m /= 2;
                sect->neighbors = malloc( (m  ) * sizeof(*sect->neighbors) );
                sect->vertex    = malloc( (m+1) * sizeof(*sect->vertex)    );
                for(n=0; n<m; ++n) sect->neighbors[n] = num[m + n];
                for(n=0; n<m; ++n) sect->vertex[n+1]  = vert[num[n]];
                sect->vertex[0] = sect->vertex[m];
                free(num);
                break;
            }
            case 'p':{
                float angle;
                sscanf(ptr += n, "%f %f %f %d", &v.x, &v.y, &angle, &n);
                player = (struct player) { {v.x, v.y, 0}, {0,0,0}, angle,0,0,0, n };
                player.where.z = sectors[player.sector].floor + EyeHeight;
            }
        }
    }
    fclose(fp);
    free(vert);
}
```
上面是实体的本地数据加载的C程序，比较精简，可以膜拜一下。下面是内存数据释放函数：

```
static void unloadData()
{
    for(unsigned a=0; a<NumSectors; ++a) free(sectors[a].vertex);
    for(unsigned a=0; a<NumSectors; ++a) free(sectors[a].neighbors);
    free(sectors);
    sectors    = NULL;
    NumSectors = 0;
}
```
主要释放房间额顶点数组数据和公共边数组数据，以及房间数组sectors自身。

### 五、绘制一条垂直线
---------------------------------

由于房间的绘制采用垂直方向的正交投影，所以有必要单独抽出一个垂直线的绘制函数，作为绘制的最小方法。

```
static void vline(int x, int y1, int y2, int top, int middle, int bottom)
{
    int *pix = (int*)screen_pixels;
    y1 = clamp(y1, 0, H-1);
    y2 = clamp(y2, 0, H-1);
    if (y2 == y1) {
        pix[y1*W+x] = middle;
    } else if(y2 > y1) {
        pix[y1*W+x] = top;
        for (int y=y1+1; y<y2; y++) {
            pix[y*W+x] = middle;
        }
        pix[y2*W+x] = bottom;
    }
}
```
垂直线段的x是相同的，y是不相同的。并且线段的起点、中间点、终点颜色是不一样的。绘制过程，就是将颜色值写入像素点数组对应的位置。


### 六、绘制3D空间（渲染核心）
---------------------------------

```
static void draw_screen()
{
    enum { MaxQueue = 32 };  // maximum number of pending portal renders
    struct item { int sectorno,sx1,sx2; } queue[MaxQueue], *head=queue, *tail=queue;
    int ytop[W]={0}, ybottom[W], renderedsectors[NumSectors];
    for(unsigned x=0; x<W; ++x) ybottom[x] = H-1;
    for(unsigned n=0; n<NumSectors; ++n) renderedsectors[n] = 0;
    
    /* Begin whole-screen rendering from where the player is. */
    *head = (struct item) { player.sector, 0, W-1 };
    if(++head == queue+MaxQueue) head = queue;
    
    do {
        /* Pick a sector & slice from the queue to draw */
        const struct item now = *tail;
        if(++tail == queue+MaxQueue) tail = queue;
        
        if(renderedsectors[now.sectorno] & 0x21) continue; // Odd = still rendering, 0x20 = give up
        ++renderedsectors[now.sectorno];
        const struct sector* const sect = &sectors[now.sectorno];
        /* Render each wall of this sector that is facing towards player. */
        for(unsigned s = 0; s < sect->npoints; ++s)
        {
            /* Acquire the x,y coordinates of the two endpoints (vertices) of this edge of the sector */
            float vx1 = sect->vertex[s+0].x - player.where.x, vy1 = sect->vertex[s+0].y - player.where.y;
            float vx2 = sect->vertex[s+1].x - player.where.x, vy2 = sect->vertex[s+1].y - player.where.y;
            /* Rotate them around the player's view */
            float pcos = player.anglecos, psin = player.anglesin;
            float tx1 = vx1 * psin - vy1 * pcos,  tz1 = vx1 * pcos + vy1 * psin;
            float tx2 = vx2 * psin - vy2 * pcos,  tz2 = vx2 * pcos + vy2 * psin;
            /* Is the wall at least partially in front of the player? */
            if(tz1 <= 0 && tz2 <= 0) continue;
            /* If it's partially behind the player, clip it against player's view frustrum */
            if(tz1 <= 0 || tz2 <= 0)
            {
                float nearz = 1e-4f, farz = 5, nearside = 1e-5f, farside = 20.f;
                // Find an intersection between the wall and the approximate edges of player's view
                struct xy i1 = Intersect(tx1,tz1,tx2,tz2, -nearside,nearz, -farside,farz);
                struct xy i2 = Intersect(tx1,tz1,tx2,tz2,  nearside,nearz,  farside,farz);
                if(tz1 < nearz) { if(i1.y > 0) { tx1 = i1.x; tz1 = i1.y; } else { tx1 = i2.x; tz1 = i2.y; } }
                if(tz2 < nearz) { if(i1.y > 0) { tx2 = i1.x; tz2 = i1.y; } else { tx2 = i2.x; tz2 = i2.y; } }
            }
            /* Do perspective transformation */
            float xscale1 = hfov / tz1, yscale1 = vfov / tz1;    int x1 = W/2 - (int)(tx1 * xscale1);
            float xscale2 = hfov / tz2, yscale2 = vfov / tz2;    int x2 = W/2 - (int)(tx2 * xscale2);
            if(x1 >= x2 || x2 < now.sx1 || x1 > now.sx2) continue; // Only render if it's visible
            /* Acquire the floor and ceiling heights, relative to where the player's view is */
            float yceil  = sect->ceil  - player.where.z;
            float yfloor = sect->floor - player.where.z;
            /* Check the edge type. neighbor=-1 means wall, other=boundary between two sectors. */
            int neighbor = sect->neighbors[s];
            float nyceil=0, nyfloor=0;
            if(neighbor >= 0) // Is another sector showing through this portal?
            {
                nyceil  = sectors[neighbor].ceil  - player.where.z;
                nyfloor = sectors[neighbor].floor - player.where.z;
            }
            /* Project our ceiling & floor heights into screen coordinates (Y coordinate) */
#define Yaw(y,z) (y + z*player.yaw)
            int y1a  = H/2 - (int)(Yaw(yceil, tz1) * yscale1),  y1b = H/2 - (int)(Yaw(yfloor, tz1) * yscale1);
            int y2a  = H/2 - (int)(Yaw(yceil, tz2) * yscale2),  y2b = H/2 - (int)(Yaw(yfloor, tz2) * yscale2);
            /* The same for the neighboring sector */
            int ny1a = H/2 - (int)(Yaw(nyceil, tz1) * yscale1), ny1b = H/2 - (int)(Yaw(nyfloor, tz1) * yscale1);
            int ny2a = H/2 - (int)(Yaw(nyceil, tz2) * yscale2), ny2b = H/2 - (int)(Yaw(nyfloor, tz2) * yscale2);
            
            /* Render the wall. */
            int beginx = max(x1, now.sx1), endx = min(x2, now.sx2);
            for(int x = beginx; x <= endx; ++x)
            {
                /* Calculate the Z coordinate for this point. (Only used for lighting.) */
                int z = ((x - x1) * (tz2-tz1) / (x2-x1) + tz1) * 8;
                /* Acquire the Y coordinates for our ceiling & floor for this X coordinate. Clamp them. */
                int ya = (x - x1) * (y2a-y1a) / (x2-x1) + y1a, cya = clamp(ya, ytop[x],ybottom[x]); // top
                int yb = (x - x1) * (y2b-y1b) / (x2-x1) + y1b, cyb = clamp(yb, ytop[x],ybottom[x]); // bottom
                
                /* Render ceiling: everything above this sector's ceiling height. */
                vline(x, ytop[x], cya-1, 0x111111 ,0x222222,0x111111);
                /* Render floor: everything below this sector's floor height. */
                vline(x, cyb+1, ybottom[x], 0x0000FF,0x0000AA,0x0000FF);
                
                /* Is there another sector behind this edge? */
                if(neighbor >= 0)
                {
                    /* Same for _their_ floor and ceiling */
                    int nya = (x - x1) * (ny2a-ny1a) / (x2-x1) + ny1a, cnya = clamp(nya, ytop[x],ybottom[x]);
                    int nyb = (x - x1) * (ny2b-ny1b) / (x2-x1) + ny1b, cnyb = clamp(nyb, ytop[x],ybottom[x]);
                    /* If our ceiling is higher than their ceiling, render upper wall */
                    unsigned r1 = 0x010101 * (255-z), r2 = 0x040007 * (31-z/8);
                    vline(x, cya, cnya-1, 0, x==x1||x==x2 ? 0 : r1, 0); // Between our and their ceiling
                    ytop[x] = clamp(max(cya, cnya), ytop[x], H-1);   // Shrink the remaining window below these ceilings
                    /* If our floor is lower than their floor, render bottom wall */
                    vline(x, cnyb+1, cyb, 0, x==x1||x==x2 ? 0 : r2, 0); // Between their and our floor
                    ybottom[x] = clamp(min(cyb, cnyb), 0, ybottom[x]); // Shrink the remaining window above these floors
                }
                else
                {
                    /* There's no neighbor. Render wall from top (cya = ceiling level) to bottom (cyb = floor level). */
                    unsigned r = 0x010101 * (255-z);
                    vline(x, cya, cyb, 0, x==x1||x==x2 ? 0 : r, 0);
                }
            }
            /* Schedule the neighboring sector for rendering within the window formed by this wall. */
            if(neighbor >= 0 && endx >= beginx && (head+MaxQueue+1-tail)%MaxQueue)
            {
                *head = (struct item) { neighbor, beginx, endx };
                if(++head == queue+MaxQueue) head = queue;
            }
        } // for s in sector's edges
        ++renderedsectors[now.sectorno];
    } while(head != tail); // render any other queued sectors
}
```

代码比较复杂，但也就100行左右，自己多花点时间慢慢分析。理解了它，基本就能理解整个绘制渲染过程。


### 七、玩家移动
---------------------------------

```
static void move_player(float dx, float dy)
{
    float px = player.where.x, py = player.where.y;
    const struct sector* const sect = &sectors[player.sector];
    const struct xy* const vert = sect->vertex;
    for(unsigned s = 0; s < sect->npoints; s++){
        if(sect->neighbors[s] >= 0
           && IntersectBox(px,py, px+dx,py+dy, vert[s+0].x, vert[s+0].y, vert[s+1].x, vert[s+1].y)
           && PointSide(px+dx, py+dy, vert[s+0].x, vert[s+0].y, vert[s+1].x, vert[s+1].y) < 0)
        {
            player.sector = sect->neighbors[s];
            break;
        }
    }
    
    player.where.x += dx;
    player.where.y += dy;
    player.anglesin = sinf(player.angle);
    player.anglecos = cosf(player.angle);
}
```

玩家的移动，就是根据游戏逻辑循环逐渐修改玩家的空间位置、视线角度和所在房间。

### 八、平台相关逻辑
---------------------------------

该节之前的描述，都是平台无关的逻辑，也是渲染实现的核心。平台相关逻辑，主要集中在像素二维数组到屏幕窗口显示的对接逻辑，渲染更新调用逻辑。

本人喜欢使用特定平台屏幕的刷新回调函数，作为渲染更新调用入口。像素数组到屏幕显示一般各个平台都有特定api来实现的。

在iOS平台最终效果如下：

![效果图](http://arvinsfj.github.io/public/ctt/documents/xgt_r.png)


### 九、随便说点
---------------------------------

1. Doom风格的渲染比三角形渲染性能要高，主要是大量简化了墙体渲染的计算量（使用垂直线渲染），但是通用性就没有使用三角形渲染的好；
2. 编写程序的时候，需要区分平台相关和平台无关代码，使程序具有更好的通用性；


>END

[代码下载](documents/Render_Doom.zip)

