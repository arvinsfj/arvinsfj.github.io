
# xv6操作系统文件系统的事务支持（log）

> *作者：Arvin 日期：2018年8月2日*

---------------------------------

>BEGIN

事务，将一组操作当成一个操作来对待。可以用“一组操作 -> 缓存操作结果 -> 提交最终结果到实际区域”的模式来实现。

### 一、前言
----------------------------------

上一篇讲了bio缓存和ide驱动，所有的磁盘操作都基于bio层进行（bread、bwrite、brelse）。本篇分析文件系统写操作事务的支持。事务就是一组操作，作为一个整体操作来看待，要么全部成功，要么全部失败。不存在一组操作中，一些操作成功，另外一些操作失败的情况。


### 二、事务数据定义
----------------------------------

```

// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing sector #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged sector #s before commit.

struct logheader {
  int n;   
  int sector[LOGSIZE];
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int busy; // a transaction is active
  int dev;
  struct logheader lh;
};
struct log log;

```

整个log磁盘区域，包含一个header块和连续的block数据块。内存中记录当前header和log数据块区域的位置和其他一些必要信息。

header中包含一个n，记录log数据区域有效块的个数。sector字段是一个数组，记录log数据区域数据块在磁盘实际位置的扇区编号。后面会根据这个扇区编号，将log的数据块移动到对应的扇区。

log结构体变量，在内存中除了header之外还记录了，磁盘中log的磁盘扇区开始编号（start），整个log区域的数据块个数（size），busy为1代表一个事务正在进行（其他事务需要等待），dev是磁盘设备编号（xv6中值为ROOTDEV）。

log是一个磁盘区域包含size个扇区（数据块），从磁盘扇区start开始。它包含头部（header）和记录的实际数据块。


### 三、函数分析
----------------------------------

初始化函数：

```
void
initlog(void)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  initlock(&log.lock, "log");
  readsb(ROOTDEV, &sb);
  log.start = sb.size - sb.nlog;
  log.size = sb.nlog;
  log.dev = ROOTDEV;
  recover_from_log();
}

```

BSIZE宏被定义成512，代表512字节。这句话确保logheader所占用的大小不超过512个字节，因为header也是会写入磁盘扇区的，一个扇区大小是512字节。换句话，header大小不超过1个扇区，或者说header被分配到一个扇区里，占用log的一个扇区。

```readsb(ROOTDEV, &sb);``` 调用readsb函数从磁盘读取一个叫做“superblock”的扇区。readsb函数调用了bread函数。

```
// Read the super block.
void
readsb(int dev, struct superblock *sb)
{
  struct buf *bp;
  
  bp = bread(dev, 1);
  memmove(sb, bp->data, sizeof(*sb));
  brelse(bp);
}

```

很简单的实现。

之后设置start、size和dev。我们注意到，log是存放在最后面的磁盘扇区里面，而且大小都是在superblock中定义的。超级块（super block）是什么后面再讲。

最后调用recover_from_log函数。

```
static void
recover_from_log(void)
{
  read_head();      
  install_trans(); // if committed, copy from log to disk
  log.lh.n = 0;
  write_head(); // clear the log
}

```

从log区域读取header进log变量的lh字段，然后根据header的信息（n和sector）将log的数据块区域的数据块移动到sector数组记录的实际扇区。最后将lh的n设置为0，表示log区域没有数据块了（物理上实际还是存在的），最后将内存中的lh（header）写入磁盘log区域的header。recover_from_log函数其实就是数据恢复函数（事务回滚）。

数据块移动函数：

```
// Copy committed blocks from log to their home location
static void 
install_trans(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
    struct buf *dbuf = bread(log.dev, log.lh.sector[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    brelse(lbuf); 
    brelse(dbuf);
  }
}

```

如注释所示，移动log中的数据块到实际的扇区。


```log.start+tail+1``` 为什么要加1呢？log的header块占用一个最开始的扇区。也就是跳过header，直接读取实际的数据块。

这里面大量使用bread和bwrite，不懂的回头看看上一篇《xv6操作系统文件系统bio》。基本思路是：从log区域读取一个块，然后从目标地扇区读取一个块，将log块覆盖目标块的缓存区data，最后将目标块（同步）写入到磁盘扇区。

读取磁盘header函数：

```
// Read the log header from disk into the in-memory log header
static void
read_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.sector[i] = lh->sector[i];
  }
  brelse(buf);
}

```

```log.start``` 是log区header所在扇区（header在log区的第一个扇区）。lh就是磁盘中log区header的内存镜像指针。然后，将lh的n和section拷贝给内存中的```log.lh```。


写入磁盘header函数：

```
// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    hb->sector[i] = log.lh.sector[i];
  }
  bwrite(buf);
  brelse(buf);
}

```

过程跟read函数相反，没什么可讲的。下面是事务最重要的3个函数：begin_trans、commit_trans和log_write。

------------------------------------

```
void
begin_trans(void)
{
  acquire(&log.lock);
  while (log.busy) {
    sleep(&log, &log.lock);
  }
  log.busy = 1;
  release(&log.lock);
}

void
commit_trans(void)
{
  if (log.lh.n > 0) {
    write_head();    // Write header to disk -- the real commit
    install_trans(); // Now install writes to home locations
    log.lh.n = 0; 
    write_head();    // Erase the transaction from the log
  }
  
  acquire(&log.lock);
  log.busy = 0;
  wakeup(&log);
  release(&log.lock);
}

```

begin_trans提供了事务入口锁机制，如果系统中有一个进程正在进行事务处理，则其他进程需要等待。注意这里的log是系统唯一的，也就是说，xv6系统在某一个时刻只能有一个事务处于“正在执行”状态，其他事务处理必须等待。


commit_trans是实际的事务处理，将log中的数据块移动到实际的物理扇区，然后清空log区域，最后设置busy为0，让其他事务可以进行事务操作。

那么，log中的数据块是谁写入的呢？这就是log_write函数的职责。

```
// Caller has modified b->data and is done with the buffer.
// Append the block to the log and record the block number, 
// but don't write the log header (which would commit the write).
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;

  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  if (!log.busy)
    panic("write outside of trans");

  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.sector[i] == b->sector)   // log absorbtion?
      break;
  }
  log.lh.sector[i] = b->sector;
  struct buf *lbuf = bread(b->dev, log.start+i+1);
  memmove(lbuf->data, b->data, BSIZE);
  bwrite(lbuf);
  brelse(lbuf);
  if (i == log.lh.n)
    log.lh.n++;
  b->flags |= B_DIRTY; // XXX prevent eviction
}

```

首先在内存log中寻找合适的sector，将传入的buf对象的sector记录在```log.lh.sector```数组中，之后从log区域读取一个buf对象（一个数据块或者扇区），把传入的buf对象的data数据移动到log区域的数据块镜像buf对象中，最后将log数据块的buf对象同步写入到log区域对应的磁盘扇区。如果是新增的buf对象，则内存中的```log.lh.n```加1，表示log区域新增了一个数据块。

```b->flags |= B_DIRTY; // XXX prevent eviction``` 表示buf对象数据data，需要更新（写操作）；换句话来讲，它没有更新写入磁盘。（磁盘扇区的数据跟内存对象buf的数据不一致）

只有等到commit_trans函数执行之后，才会真正的（同步）更新扇区数据。

事务的编程方式如下：

```
begin_trans();

文件系统写操作1
文件系统写操作2
文件系统写操作3
...

commit_trans();

```

其中```文件系统写操作```的实现函数中调用了log_write函数，每次把buf中的数据写到log数据区（位于磁盘中），等到调用commit_trans函数的时候，一次性将所有写入到log数据区的数据块移动到相应的磁盘扇区。确保了，多次写入的事务性：要么全部写入成功，要么全部失败。如果某个写操作失败，并不能影响实际扇区的数据（数据先是写入log数据区的，并不是直接写入实际扇区的）；如果某次install_trans失败（比如断点），还可以通过调用recover_from_log函数恢复数据（在os启动时会调用该函数）。

文件系统后面的写操作都是用log_write代替bwrite函数进行磁盘写操作的，当然log_write函数是被begin_trans和commit_trans包围的。

磁盘读和写的同步是谁提供的呢？答案是idelock。


### 四、随便说点
----------------------------------

好像与磁盘文件操作相关的系统，都需要支持事务。比如：数据库、文件系统等。实现方式：缓存一组操作的结果并一次性提交最终结果到实际区域。

-----------------------------------

> END

