
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


> END

