
# xv6操作系统文件系统数据缓存层和IDE硬盘驱动（bio和ide）

> *作者：Arvin 日期：2018年8月1日*

---------------------------------

>BEGIN

文件系统是类unix系统比较重要的部分，“一切皆文件”。从最底层的硬盘上的数据块，到最上层的文件抽象对象，一步一步的象成不同的层次（文件系统是分层设计的，层次感比较强）。对软件系统的设计具有很好的指导作用，而且文件系统相对独立关系比较简单，适合学习。


### 一、前言
----------------------------------

xv6的文件系统分为5层：块、日志、文件、文件夹、路径。

```
// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
```

本篇分析ide硬盘的驱动和基于该驱动层的内存i/o缓存层，是属于比较底层的部分，文件系统的其他部分，都是基于该层的。我们先分析ide驱动，再讲bio缓存层。

### 二、IDE硬盘驱动（ide.c）
----------------------------------

ide硬盘的操作，我们之前在boot加载内核的时候是遇到过的，那个时候是直接使用汇编操作硬盘的。这部分当然需要使用汇编操作硬盘，但是大部分还是使用C编程的，调用的一些C函数是封装内嵌汇编代码的。相对简单一些，本质原理基本一样。

数据定义如下：

```
#define IDE_BSY       0x80 //磁盘忙 busy
#define IDE_DRDY      0x40 //磁盘准本好了 ready
#define IDE_DF        0x20 //
#define IDE_ERR       0x01 //磁盘出错

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct buf *idequeue;

```

首先定义磁盘操作的4种状态（可以认为磁盘操作是一个状态机，os中很多都是状态机的设计）。然后是磁盘的读写命令（都是写入磁盘端口：0x1f7）。最后定义了一个任务队列（idequeue），并且该队列是需要同步操作的，故还有一个自旋锁（idelock）。

注意这个任务队列的设计和实现。可以看一下，buf结构体的定义，里面有一个qnext字段，指向下一个需要读或者写的buf。（形成一个了磁盘任务单链表）。

----------------------------------

以下是驱动函数分析。

磁盘等待函数：

```
// Wait for IDE disk to become ready.
static int
idewait(int checkerr)
{
  int r;

  while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY) 
    ;
  if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}

```

磁盘状态只有处于ready状态，并且没有错误或者IDE_DF（不知道是什么状态），才能进行读写。从上面可以看出，状态读取端口是0x1f7。inb函数之前已经遇到过，里面封装了内嵌汇编指令（端口读写指令：inb和outb，在C中是没有对应的对象，也就是在C中是无法实现这2条指令的功能）。上面用while循环实现磁盘状态等待。函数返回0，代表一切准备完成，磁盘可以进行读写了。ps：这比汇编的实现逻辑更加清晰，更加易于阅读和编写。checkerr参数可以认为是一个是否检查磁盘错误的开关变量（0: 不检查错误， 1: 检查错误），这种写法可以注意一下。

---------------------------------

磁盘初始化函数：

```
void
ideinit(void)
{
  int i;

  initlock(&idelock, "ide");
  picenable(IRQ_IDE);
  ioapicenable(IRQ_IDE, ncpu - 1);
  idewait(0);
  
  // Check if disk 1 is present
  outb(0x1f6, 0xe0 | (1<<4));
  for(i=0; i<1000; i++){
    if(inb(0x1f7) != 0){
      havedisk1 = 1;
      break;
    }
  }
  
  // Switch back to disk 0.
  outb(0x1f6, 0xe0 | (0<<4));//设置当前磁盘
}

```

首先初始化自旋锁变量idelock，然后开启IDE磁盘中断（pic和ioapic），之后等待磁盘ready状态。磁盘准备好之后，检查是否存在另一块磁盘1，如果存在记录“存在磁盘1”的状态在havedisk1变量中。最后强制切换当前磁盘到磁盘0。

设置当前磁盘的端口是0x1f6。读取当前磁盘状态的端口是0x1f7。


判断是否存在磁盘1的原理是：设置当前磁盘为磁盘1，然后最多读取1000次该磁盘的状态，如果不等于0，则表示存在该磁盘，否则不存在。

-----------------------------------

磁盘的读写都是附属于buf对象的。什么意思呢？文件系统的上层都是基于buf进行磁盘的读写，而不是直接操作磁盘，buf提供了一层磁盘读写抽象。

buf结构体定义：

```
struct buf {
  int flags;
  uint dev;
  uint sector;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[512];
};
#define B_BUSY  0x1  // buffer is locked by some process
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk

```

从这个结构体定义可以知道，该结构体可以形成一个双链表和一个单链表（某个bug对象既可以是双链表中的一个节点，同时也可以是单链表中的一个节点）。data是一个512字节的数组，大概知道是一个扇区（block）的缓存。dev是设备号（读写头？），sector是扇区号。flags是该buf的状态标志。有3种状态：B_BUSY（该buf被进程使用中,如果是其他进程使用中则当前进程需要等待）、B_VALID（该buf已经读取了磁盘，data中存在扇区的缓存数据）、B_DIRTY（该buf的数据跟磁盘扇区的数据不一致，需要更新磁盘扇区数据，也就是需要写磁盘）。

---------------------------------

读或者写磁盘函数：

```
// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
  if(b == 0)
    panic("idestart");

  idewait(0);
  outb(0x3f6, 0);  // generate interrupt
  outb(0x1f2, 1);  // number of sectors
  outb(0x1f3, b->sector & 0xff);
  outb(0x1f4, (b->sector >> 8) & 0xff);
  outb(0x1f5, (b->sector >> 16) & 0xff);
  outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((b->sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
    outb(0x1f7, IDE_CMD_WRITE);
    outsl(0x1f0, b->data, 512/4);
  } else {
    outb(0x1f7, IDE_CMD_READ);
  }
}

```

就像函数名称所表示的“开始b的请求”，该函数直接根据b中的信息，进行磁盘的操作（读或者写）。```outb(0x3f6, 0);```设置在操作完成后，触发一个中断。其他都是设置操作的扇区和磁头。如果b的flags是B_DIRTY则进行写磁盘（扇区），如果是其他状态则读磁盘（扇区）。

----------------------------------

磁盘中断处理函数：

```
// Interrupt handler.
void
ideintr(void)
{
  struct buf *b;

  // First queued buffer is the active request.
  acquire(&idelock);
  if((b = idequeue) == 0){
    release(&idelock);
    // cprintf("spurious IDE interrupt\n");
    return;
  }
  idequeue = b->qnext;

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
    insl(0x1f0, b->data, 512/4);
  
  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
  wakeup(b);
  
  // Start disk on next buf in queue.
  if(idequeue != 0)
    idestart(idequeue);

  release(&idelock);
}

```

磁盘请求函数（idestart），在操作完成后，磁盘会触发一个磁盘中断，会执行ideintr函数。首先拿到触发中断的buf（中断需要处理的buf），然后idequeue指向下一个任务buf，之后处理本次中断操作。如果flags不是写并且磁盘处于ready状态，则从端口0x1f0读取磁盘（数据缓冲区）数据到b的缓存区域data中（共1个扇区，512字节）。之后，设置b的flags成缓存区有效（B_VALID）和非写，唤醒等待b的进程。如果idequeue（这个时候指向任务列表的下一个节点buf）非0存在，则继续请求下一个buf的磁盘操作（直到任务列表所有节点的磁盘任务完成）。最后释放任务列表的自旋锁。

这里，idestart会产生中断，调用ideintr函数；而ideintr函数中也会主动调用idestart函数。这里就是一个隐含的循环，注意结束条件（idequeue == 0）。

----------------------------------

缓存buf同步函数：

```
// Sync buf with disk. 
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
  struct buf **pp;

  if(!(b->flags & B_BUSY))
    panic("iderw: buf not busy");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if(b->dev != 0 && !havedisk1)
    panic("iderw: ide disk 1 not present");

  acquire(&idelock);  //DOC:acquire-lock

  // Append b to idequeue.
  b->qnext = 0;
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
    ;
  *pp = b;
  
  // Start disk if necessary.
  if(idequeue == b)
    idestart(b);
  
  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
    sleep(b, &idelock);
  }

  release(&idelock);
}

```

函数的作用上面的注释讲的很清楚了。作用就是同步buf跟对应磁盘扇区的数据。如果，buf的数据比较新则更新扇区的数据（写磁盘），如果buf缓存区data没有数据，则读取磁盘扇区的数据到data中。

```
// Append b to idequeue.

b->qnext = 0; // 必须设置，否则可能引起死循环。ideintr函数的结束条件是 idequeue == 0 。
for(pp=&idequeue; *pp; pp=&(*pp)->qnext); // 在头为idequeue的单链表中寻找最后一个任务节点pp
*pp = b; // 将b追加到该单链表，此时b->qnext等于0，标记：没有下一个节点

```

如果 idequeue == b 则开始b的磁盘任务（数据同步）操作。注意，idequeue有可能跟b不相等，则并不是立即进行同步操作的。

最后进程循环等待数据同步操作完成，之后当前进程释放自旋锁。

磁盘数据同步驱动，就这么简单。数据同步是在内存buf对象跟磁盘扇区进行的。这里的buf对象，可以看成对应磁盘扇区的内存（镜像）缓存。磁盘有很多扇区，对应的buf应该也有很多，xv6使用双链表组织这些buf。


### 三、磁盘扇区内存缓存双链表（bio.c和buf.h）
----------------------------------

还记得结构体buf的字段定义吗？其中包含：

```
struct buf *prev; // LRU cache list
struct buf *next;

```

这两个字段定义了一个双链表节点结构。ps：LRU好像叫作“最近最少使用算法”，对应的有MRU（最近最常使用算法）。

"所谓的LRU(Least recently used)算法的基本概念是:当内存的剩余的可用空间不够时,缓冲区尽可能的先保留使用者最常使用的数据,换句话说就是优先清除”较不常使用的数据”,并释放其空间.之所以”较不常使用的数据”要用引号是因为这里判断所谓的较不常使用的标准是人为的、不严格的.所谓的MRU(Most recently used)算法的意义正好和LRU算法相反。"

这里是完整的双链表定义：

```
struct {
  struct spinlock lock; // 需要同步使用该链表
  struct buf buf[NBUF]; // 磁盘扇区内存缓存区域；物理上是连续的，但是逻辑上是不一定是连续的（很好玩的设计，跟常规思维相反）

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head; // 双链表的头节点，head的pre指向链表的结束节点，next指向链表的起始节点。
} bcache;

#define NBUF         10  // size of disk block cache

```

准确的说，应该是 "循环双链表"，这里不纠结了。这里bcache是全局结构体变量，而不是类型。

下面是我绘制的该链表示意图：

![xv6运行效果](http://arvinsfj.github.io/public/ctt/documents/osxv6/bio_list.png)

----------------------------------

链表初始化函数：

```
void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    b->dev = -1;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

```

该函数会构造上图所示的循环双链表。具体构造的过程，逻辑上有点复杂（输入代码很简单），脑壳疼。自己在本子上按照代码逻辑画一画自然会明白的。

--------------------------------------

获取一个磁盘扇区buf对象：

```
// Look through buffer cache for sector on device dev.
// If not found, allocate fresh block.
// In either case, return B_BUSY buffer.
static struct buf*
bget(uint dev, uint sector)
{
  struct buf *b;

  acquire(&bcache.lock);

 loop:
  // Is the sector already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->sector == sector){
      if(!(b->flags & B_BUSY)){
        b->flags |= B_BUSY;
        release(&bcache.lock);
        return b;
      }
      sleep(b, &bcache.lock);
      goto loop;
    }
  }

  // Not cached; recycle some non-busy and clean buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if((b->flags & B_BUSY) == 0 && (b->flags & B_DIRTY) == 0){
      b->dev = dev;
      b->sector = sector;
      b->flags = B_BUSY;
      release(&bcache.lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

```

注意goto语句（感觉很脏）。首先从bcache（链表）中，寻找符合条件的扇区缓存buf对象，如果找到了直接返回该buf。

没有找到则进行分配操作。首先找一个不"忙"并且不需要更新的（写）操作的buf，然后设置对应的参数，之后返回该buf对象。（毕竟bcache是缓存，里面的buf对象都是临时的和重用的）。

这个函数的作用是：一定会返回一个合适的buf对象给上层。

---------------------------------

buf读取函数：

```
// Return a B_BUSY buf with the contents of the indicated disk sector.
struct buf*
bread(uint dev, uint sector)
{
  struct buf *b;

  b = bget(dev, sector);
  if(!(b->flags & B_VALID))
    iderw(b);
  return b;
}

```

调用bget函数获取一个符合要求的buf对象，如果缓存区data无效（没有扇区数据），则进行buf的磁盘同步操作，最后返回该buf对象。

该函数一定会返回一个带磁盘扇区数据的buf对象。

----------------------------------

buf写函数：

```
// Write b's contents to disk.  Must be B_BUSY.
void
bwrite(struct buf *b)
{
  if((b->flags & B_BUSY) == 0)
    panic("bwrite");
  b->flags |= B_DIRTY;
  iderw(b);
}

```

buf写操作，只需要设置一下flags的状态成B_DIRTY，之后调用```iderw(b);```即可。

该函数会将buf缓存区data的数据更新到磁盘对应的扇区（注意：有可能不是立即写入磁盘，但是会立即加入到磁盘任务单链表中）。

---------------------------------

buf释放函数：

```
// Release a B_BUSY buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if((b->flags & B_BUSY) == 0)
    panic("brelse");

  acquire(&bcache.lock);

  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = bcache.head.next;
  b->prev = &bcache.head;
  bcache.head.next->prev = b;
  bcache.head.next = b;

  b->flags &= ~B_BUSY;
  wakeup(b);

  release(&bcache.lock);
}

```

最关键的2句是```b->flags &= ~B_BUSY; wakeup(b);```。设置该buf不"忙"，也就是buf没有被进程使用，然后唤醒等待在b上的其他进程。其他语句基本都是“循环双链表”操作。

这个buf释放函数，里面的链表操作细节很好玩的，可以仔细分析一下（不改变链表大小的头插入）。该函数在逻辑上会打乱缓存区的节点顺序。物理上是连续的，逻辑上不连续。


### 四、随便说点
----------------------------------

磁盘驱动程序只提供一个数据同步函数（iderw函数）；扇区数据内存缓存程序提供10个buf的缓存区域，以循环双链表形式组织，向上提供buf的读取、写入和释放函数。对上层来讲，并不去直接操作磁盘，操作的是buf对象（这就是封装，这种思想可以仔细品味一下）。这个链表，以10个buf节点的代价就可以读写整个磁盘了（是不是很强大），在易用性和性能上都比直接操作磁盘更加优秀。

在设计软件的时候，注意功能职责划分和功能层次划分，当然还有数据结构的设计和算法的选择。这种思想贯穿整个文件系统的设计。

-----------------------------------

> END

