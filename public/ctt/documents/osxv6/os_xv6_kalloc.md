
# xv6操作系统物理页面管理（kalloc）

> *作者：Arvin 日期：2018年7月21日*

---------------------------------

>BEGIN

由于xv6系统采用了分页机制，为了方便管理物理内存，我们要将物理内存以页面的形式（物理内存按照页面划分，分配和释放都按整页来处理，即页面是内存管理的基本单位，每个页面占用4096字节）进行组织。内核中包含全局的内存管理对象（kmem对象，包含空闲页面的单链表的基址）即可。

### 一、前言
----------------------------------

xv6是以简单的单链表的形式来管理空闲页面的。并且内核中包含一个内存管理对象kmem，它记录了空闲页面链表（freelist）的基址。还包含一个自旋锁对象和锁使用标志。因为物理内存在多核环境下是一种需要同步使用的资源。注意单链表节点自身就是物理内存页面，每个被管理的页面的开头有一个指向下一个节点（空闲页面）的指针。

### 二、单链表定义
----------------------------------

页面结构体定义如下：

```
struct run {
  struct run *next;
};

```

只定义了一个指向下一个物理页面的指针（地址）。从而可以将整个物理内存构建成一个链表。这里要注意一下，next指针只占用4字节，页面剩下的字节都是可用的内存，但是却不能根据run结构体来访问，因为我们是在管理页面而非使用页面，不需要访问页面剩下的内存。这里跟应用编程不一样的地方是，我们分配给结构体的内存大小是大于结构体自身的大小。

页面管理对象定义如下：

```
struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

```

注意这里的kmem是一个结构体对象（变量），而非结构体名称。其中，freelist是页面单链表头指针；lock是单链表资源的自旋锁，防止在多核下单链表资源被不同步操作；use_lock是表示单链表是否需要同步操作，因为在启动后只有一个cpu在操作不存在资源同步问题，此时use_lock等于0，当多核启动之后，内存的申请和释放就需要同步进行操作了，此时use_lock等于1。

初始化、申请和释放页面的方法如下：

```
// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}

//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || v2p(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

```

其中，kfree和kalloc是页面的释放和分配方法，在正常情况下（系统完全启动之后）是需要对单链表进行同步操作的。但是在一个cpu的情况下是不需要的，操作自然是同步的。这两个函数是正常的单链表头部插入和删除操作。freerange是初始化的核心方法，```p = (char*)PGROUNDUP((uint)vstart);```将传入的vstart地址，进行页面上的向高地址对齐（有点像向上取整操作，只不过单位是页面）。然后是一个步长是4096（1页）的for循环，释放（kfree）所有循环内的页面。释放操作（kfree）采用```memset(v, 1, PGSIZE);```将页面初始化为1。申请操作（kalloc）将头指针（freelist）移动到下一个节点（页面）并返回当前页面的指针即可。

kinit1和kinit2，是内核启动两个阶段的物理页面初始化方法。kinit1是初始化内核结尾（end）到4M内存的。kinit2是初始化4M到剩下的内存之间的内存的。为什么要分两个阶段呢？原因是未完全启动多核之前，当前已经启动了的cpu使用entrypgdir映射的[0, 4M)作为执行空间的。当前程序能访问的物理内存也就4M，大于4M的内存是不能被当前cpu识别的。

以上，就是xv6内核中物理内存页面的管理方式。很简单，本质就是单链表的操作，只不过单链表节点是物理内存的页面（4096字节）。

对于忘记了自旋锁（spinlock）的同学，可以翻翻之前的文章。

### 三、补充说明
----------------------------------

两个宏解释如下：

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1)) // 向高地址按4K对齐
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1)) // 确保数值a的低12位全部为0，其它高位保持原值不变。即，向低地址按4K对齐。

```PGSIZE```宏定义成4096，即 2^12 二进制表示：0001, 0000, 0000, 0000

```PGSIZE-1``` 二进制表示：0000, 1111, 1111, 1111

```~(PGSIZE-1)``` 二进制表示：1111, 0000, 0000, 0000

也就是说，任何数值跟```~(PGSIZE-1)```作按位与(&)操作，会确保该数值的低12位全部为0，其它高位保持原值不变。

```(((a)) & ~(PGSIZE-1))``` 宏就是上面的按位与(&)操作。简单来讲，这种操作就是将数值a的低12位全部置0。也就是，向低地址按4K（2^12）对齐。

```(((sz)+PGSIZE-1) & ~(PGSIZE-1))``` 相比较于前面的操作多了一个映射，将数值sz映射成数值sz+PGSIZE-1，你也可以想象成一个[sz, sz+PGSIZE-1]区间在另外一个PGSIZE页面区间[x, x+PGSIZE]移动。结果就是向高地址按4K（2^12）对齐。

-------------------------------------

关于memset函数：

```
void*
memset(void *dst, int c, uint n)
{
  if ((int)dst%4 == 0 && n%4 == 0){
    c &= 0xFF;
    stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
  } else
    stosb(dst, c, n);
  return dst;
}

static inline void
stosb(void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosb" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}

static inline void
stosl(void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosl" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}

```

这里的memset函数是属于内核函数（不是C标准库中的，内核开发是不允许使用任何C标准库函数的）。

上面的memset函数核心是使用工具函数stosb或者stosl将[dst, dst+n)的内存区域使用数值c填充。函数stosb和stosl唯一的区别是使用的汇编指令不同，stosb使用stosb指令，stosl使用stosl指令。一个是单字节填充一个4字节填充。这里memset函数为什么要使用两个函数，大概是因为stosl指令效率更高吧:)，具体原因不明。gcc内嵌汇编，不懂的自己去学习一下，很简单的。

-------------------------------

关于panic函数，你可以认为是内核崩溃函数，进入了这个函数会打印一些崩溃信息然后进入无限死循环。

```
void
panic(char *s)
{
  int i;
  uint pcs[10];
  
  cli();
  cons.locking = 0;
  cprintf("cpu%d: panic: ", cpu->id);
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

```

--------------------------------

关于如何获取到内核在内存中结束的位置。```kinit1(end, P2V(4*1024*1024)); // phys page allocator```在main函数中有上面一句话，其中的end在那个地方定义的呢？你在代码中找不到的，end是在链接脚本kernel.ld中定义的。链接脚本你可以理解成程序在物理内存中的布局。可以定义起始位置，也可以获取结束位置，还可以布局各个段。end就是获取到的内核结束物理地址。能不能读写该地址呢？当然是可以的。读到这，是不是对什么是变量有一些了解了呢。end定义在kernel.ld中但是可以作为指针（没有类型，也可以认为是```void*```类型）来使用，所指向的内存可以认为是一个变量（但是没有类型，也可以认为是void类型），是不是很神奇。

----------------------------------

> END

