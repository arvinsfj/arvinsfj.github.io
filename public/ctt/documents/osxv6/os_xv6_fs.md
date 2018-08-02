
# xv6操作系统文件系统磁盘文件（fs）

> *作者：Arvin 日期：2018年8月2日*

---------------------------------

>BEGIN

文件系统大部分工作是对磁盘文件的操作，本篇聚焦于存在于磁盘中的文件。

### 一、前言
----------------------------------

之前讲的xv6文件系统，都是处理block块和buf对象，并没有文件这个概念。本篇是文件系统的核心，主要讲文件和文件夹读写操作。应该算文件系统中最复杂的部分。


### 二、数据定义
----------------------------------

```
// Block 0 is unused.
// Block 1 is super block.
// Blocks 2 through sb.ninodes/IPB hold inodes.
// Then free bitmap blocks holding sb.size bits.
// Then sb.nblocks data blocks.
// Then sb.nlog log blocks.

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
};

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

```


首先看下面的一张文件系统磁盘布局：

![磁盘文件系统布局](http://arvinsfj.github.io/public/ctt/documents/osxv6/dk_l.png)

然后应该什么都清楚了。开玩笑 :) 

上图跟注释说明是一致的

```
// Block 0 is unused.
// Block 1 is super block.
// Blocks 2 through sb.ninodes/IPB hold inodes.
// Then free bitmap blocks holding sb.size bits.
// Then sb.nblocks data blocks.
// Then sb.nlog log blocks.

翻译过来：

block可以理解成磁盘扇区（sector）。

扇区0，被bootloader占据了。

扇区1，是超级块（superblock结构体所示），包含四个字段：size（文件系统中block数量）、nblocks（数据block的数量）、ninodes（inode的数量，每个inode代表一个文件，也就是文件的数量）、nlog（log区占用的block数量，log区是在磁盘末尾的）。

扇区2到sb.ninodes/IPB，存放inodes数据（也就是文件元数据），占用的block数量可变。

在sb.ninodes/IPB后面，是bitmap扇区，bitmap扇区中的每一个字节的每一位表示某个扇区是否被使用，被标记的扇区范围是[0, 1, 2, ...., sb.size]，也就是整个磁盘扇区。如果bitmap中某位为1，表示对应扇区被使用了；反之，扇区没有被使用（处于空闲）。按逻辑，该块区域占用的block数量应该是固定的，毕竟磁盘总共有多少个扇区是固定的。

之后是数据block，被用作文件内容存放或者文件夹内容存放（注意，文件夹内容存放的是文件inode的索引和文件名称）。

最后是log区域，占用的block数量在sb中定义。

```

上面是文件系统，整个磁盘的布局描述。下面讲具体的数据结构定义。

-----------------------------------

```
#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // 文件系统中，block（扇区）的数量
  uint nblocks;      // 数据block的数量
  uint ninodes;      // inode（文件或者文件夹）的数量
  uint nlog;         // log区域的block数量
};

```

BSIZE定义block大小是512字节（一个扇区的大小）

其他如注释所示，这里不再BB。

```
#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // 文件类型：正常文件、文件夹、设备文件等
  short major;          // 主设备号 (T_DEV only)
  short minor;          // 次设备号 (T_DEV only)
  short nlink;          // 连接到该inode（文件）的（文件夹）数量。
  uint size;            // 文件的大小（单位：字节）
  uint addrs[NDIRECT+1];   // 文件内容在磁盘扇区的编号数组，前NDIRECT个数组元素是文件数据直接的磁盘扇区编号，最后一个也是扇区编号，但存放的不是文件内容而是指向文件内磁盘容扇区编号。也就是讲最后一个扇区存放的还是扇区编号，是文件剩下的内容扇区的编号。文件扇区分为direct和indirect。
};

```

结构体dinode是，inode的磁盘存储结构（暗示着内存中的inode还包含其他字段，比如：ref）。大小是固定的。

注释所示。。。。


```
// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

```

IPB，一个扇区包含的inode数量。

IBLOCK，返回inode所在的扇区编号，i是inode的唯一编号，从0开始到sb.ninodes-1。加2是因为，inode区域是从扇区2开始的。

BPB，一个扇区能提供的bitmap位数，一个扇区512字节，一个字节8位，共提供```512*8```位，每一位表示一个扇区是否被使用

BBLOCK，返回包含扇区b的bitmap位的扇区，该扇区肯定是在bitmap磁盘区域内的。自己慢慢分析。

DIRSIZ，定义文件名称的最大长度。

dirent结构体，定义文件夹内一个文件（inode）的索引和名称。


### 三、磁盘扇区（block）的分配和释放
----------------------------------

文件系统一切数据的开始是读取超级块（superblock）。

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

superblock位于磁盘的扇区1。记录了文件系统的基本信息。

没什么好讲的，只要你知道bio中的扇区读取函数bread。

扇区清0函数：

```
// Zero a block.
static void
bzero(int dev, int bno)
{
  struct buf *bp;
  
  bp = bread(dev, bno);
  memset(bp->data, 0, BSIZE);
  log_write(bp);
  brelse(bp);
}

```

从磁盘读扇区bno进内存，在内存中清0，然后写入磁盘。


磁盘扇区分配函数：

```
// Allocate a zeroed disk block.
static uint
balloc(uint dev)
{
  int b, bi, m;
  struct buf *bp;
  struct superblock sb;

  bp = 0;
  readsb(dev, &sb);
  for(b = 0; b < sb.size; b += BPB){
    bp = bread(dev, BBLOCK(b, sb.ninodes));
    for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
      m = 1 << (bi % 8);
      if((bp->data[bi/8] & m) == 0){  // Is block free?
        bp->data[bi/8] |= m;  // Mark block in use.
        log_write(bp);
        brelse(bp);
        bzero(dev, b + bi);
        return b + bi;
      }
    }
    brelse(bp);
  }
  panic("balloc: out of blocks");
}

```

整体思路是：遍历整个磁盘扇区，在bitmap中找到该扇区的对应位，如果该位位0，表示该扇区空闲（没有被使用），之后设置bitmap的对应位为1并写回磁盘bitmap扇区，表示该扇区被使用了，最后将该空闲扇区清0并返回该空闲扇区的扇区编号。

为了高效，外层循环是按照BPB步长前进的，BPB占用bitmap区域的一个扇区。

磁盘扇区释放函数：

```
// Free a disk block.
static void
bfree(int dev, uint b)
{
  struct buf *bp;
  struct superblock sb;
  int bi, m;

  readsb(dev, &sb);
  bp = bread(dev, BBLOCK(b, sb.ninodes));
  bi = b % BPB;
  m = 1 << (bi % 8);
  if((bp->data[bi/8] & m) == 0)
    panic("freeing free block");
  bp->data[bi/8] &= ~m;
  log_write(bp);
  brelse(bp);
}

```

首先读取超级块获得 ```sb.ninodes``` 的值，然后获取扇区b对应bitmap位所在的扇区，后面就是设置扇区b对应的bitmap位为0，表示该扇区空闲（也就是该扇区被释放了），最后将bitmap位所在扇区写回磁盘。

### 四、文件（inode）操作
----------------------------------

数据定义如下：

```
struct {
  struct spinlock lock;
  struct inode inode[NINODE];
} icache;

```

定义了一个icache全局变量，里面包含了缓存在内存中inode（文件）。ps：该变量主要是为了同步，而不是缓存。

```
void
iinit(void)
{
  initlock(&icache.lock, "icache");
}

```

初始化很简单，初始化一个自旋锁即可。icache是需要同步使用的。

分配inode函数：

```
// Allocate a new inode with the given type on device dev.
// A free inode has a type of zero.
struct inode*
ialloc(uint dev, short type)
{
  int inum;
  struct buf *bp;
  struct dinode *dip;
  struct superblock sb;

  readsb(dev, &sb);

  for(inum = 1; inum < sb.ninodes; inum++){
    bp = bread(dev, IBLOCK(inum));
    dip = (struct dinode*)bp->data + inum%IPB;
    if(dip->type == 0){  // a free inode
      memset(dip, 0, sizeof(*dip));
      dip->type = type;
      log_write(bp);   // mark it allocated on the disk
      brelse(bp);
      return iget(dev, inum);
    }
    brelse(bp);
  }
  panic("ialloc: no inodes");
}

```

遍历所有磁盘中的dinode数据，找到type等于0（即，空闲的dinode），之后设置该空闲的dinode类型（type）并写回磁盘，最后通过调用iget函数返回一个inode对象。iget函数后面会讲到。

同步内存的inode数据到磁盘dinode中：

```
// Copy a modified in-memory inode to disk.
void
iupdate(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;

  bp = bread(ip->dev, IBLOCK(ip->inum));
  dip = (struct dinode*)bp->data + ip->inum%IPB;
  dip->type = ip->type;
  dip->major = ip->major;
  dip->minor = ip->minor;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
  log_write(bp);
  brelse(bp);
}

```

正常操作，读取磁盘dinode到内存中，然后用传入的inode数据填充dinode，最后将dinode写回磁盘。

iget函数：

```
// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode*
iget(uint dev, uint inum)
{
  struct inode *ip, *empty;

  acquire(&icache.lock);

  // Is the inode already cached?
  empty = 0;
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
      ip->ref++;
      release(&icache.lock);
      return ip;
    }
    if(empty == 0 && ip->ref == 0)    // Remember empty slot.
      empty = ip;
  }

  // Recycle an inode cache entry.
  if(empty == 0)
    panic("iget: no inodes");

  ip = empty;
  ip->dev = dev;
  ip->inum = inum;
  ip->ref = 1;
  ip->flags = 0;
  release(&icache.lock);

  return ip;
}

```

首先从icache中寻找符合条件的inode，如果找到ref加1返回，如果没有找到则从icache中找一个空闲的inode并设置一些字段，最后返回该inode。换句话来讲，该函数始终会返回一个符合条件的inode（ref会加1）给函数调用者，并且这个inode是在icache中的。

```
// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode*
idup(struct inode *ip)
{
  acquire(&icache.lock);
  ip->ref++;
  release(&icache.lock);
  return ip;
}

```

传入的inode的ref会加1。表示引用加1，该inode代表的文件（或者文件夹）添加到了一个新的文件夹中了。

```
// Lock the given inode.
// Reads the inode from disk if necessary.
void
ilock(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;

  if(ip == 0 || ip->ref < 1)
    panic("ilock");

  acquire(&icache.lock);
  while(ip->flags & I_BUSY)
    sleep(ip, &icache.lock);
  ip->flags |= I_BUSY;
  release(&icache.lock);

  if(!(ip->flags & I_VALID)){
    bp = bread(ip->dev, IBLOCK(ip->inum));
    dip = (struct dinode*)bp->data + ip->inum%IPB;
    ip->type = dip->type;
    ip->major = dip->major;
    ip->minor = dip->minor;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
    brelse(bp);
    ip->flags |= I_VALID;
    if(ip->type == 0)
      panic("ilock: no type");
  }
}

// Unlock the given inode.
void
iunlock(struct inode *ip)
{
  if(ip == 0 || !(ip->flags & I_BUSY) || ip->ref < 1)
    panic("iunlock");

  acquire(&icache.lock);
  ip->flags &= ~I_BUSY;
  wakeup(ip);
  release(&icache.lock);
}

```

锁住某个inode和解锁该inode。在加锁的时候，如果该inode没有同步磁盘中对应的dinode数据，则进行数据同步。

该处的锁，跟自旋锁的区别仅仅是等待方式的不同，其他原理一样。这里的等待方式是"循环+进程sleep"。这种锁在有了进程之后，到处都是的。

iput函数：

```
// Drop a reference to an in-memory inode.
// If that was the last reference, the inode cache entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
void
iput(struct inode *ip)
{
  acquire(&icache.lock);
  if(ip->ref == 1 && (ip->flags & I_VALID) && ip->nlink == 0){
    // inode has no links: truncate and free inode.
    if(ip->flags & I_BUSY)
      panic("iput busy");
    ip->flags |= I_BUSY;
    release(&icache.lock);
    itrunc(ip);
    ip->type = 0;
    iupdate(ip);
    acquire(&icache.lock);
    ip->flags = 0;
    wakeup(ip);
  }
  ip->ref--;
  release(&icache.lock);
}

// Common idiom: unlock, then put.
void
iunlockput(struct inode *ip)
{
  iunlock(ip);
  iput(ip);
}

```

iput最主要的作用是ref减去1。如果某个inode的ref等于0，则释放该inode占用的所有磁盘扇区。type置0，代表该inode空闲。

iunlockput解锁并调用iput。

----------------------------------

```
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are 
// listed in block ip->addrs[NDIRECT].

```

注意文件数据内容的扇区分布，分为direct扇区和indirect扇区。ip->addrs数组的前NDIRECT个是direct扇区的扇区编号，剩下的扇区编号（indirect扇区编号）存放在```ip->addrs[NDIRECT]```标记的扇区里面。如下图：

![文件数据扇区分布](http://arvinsfj.github.io/public/ctt/documents/osxv6/fs_file.png)

下面讲inode的文件内容操作。

首先是bmap函数：

```
// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;

  if(bn < NINDIRECT){
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }

  panic("bmap: out of range");
}

```

该函数返回ip所代表的文件（内容）第bn个扇区的实际磁盘扇区编号。如果没有该扇区，则先分配扇区后返回该扇区编号。作用就是，返回文件ip第bn个扇区的磁盘扇区编号。

```
// Truncate inode (discard contents).
// Only called when the inode has no links
// to it (no directory entries referring to it)
// and has no in-memory reference to it (is
// not an open file or current directory).
static void
itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  uint *a;

  for(i = 0; i < NDIRECT; i++){
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }
  
  if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    for(j = 0; j < NINDIRECT; j++){
      if(a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}

```

该函数释放ip所占用的扇区。ip自己并不释放，因为icache中的inode是复用的。

```
// Copy stat information from inode.
void
stati(struct inode *ip, struct stat *st)
{
  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}

```

拷贝inode的元数据到st中，给用户进程获取文件属性用的。（系统调用：stat）

读取inode文件数据：

```
// Read data from inode.
int
readi(struct inode *ip, char *dst, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
      return -1;
    return devsw[ip->major].read(ip, dst, n);
  }

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > ip->size)
    n = ip->size - off;

  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    memmove(dst, bp->data + off%BSIZE, m);
    brelse(bp);
  }
  return n;
}

```

inode代表一个文件（也代表其他文件）。该函数是从该文件中读取数据。这里考虑到了如果inode是一个设备文件（比如：console等）则调用设备文件的读取函数。到这，这个函数并不难，不多讲。注意bmap函数的使用。

文件数据写入函数：

```
// Write data to inode.
int
writei(struct inode *ip, char *src, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
      return -1;
    return devsw[ip->major].write(ip, src, n);
  }

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > MAXFILE*BSIZE)
    return -1;

  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    memmove(bp->data + off%BSIZE, src, m);
    log_write(bp);
    brelse(bp);
  }

  if(n > 0 && off > ip->size){
    ip->size = off;
    iupdate(ip);
  }
  return n;
}

```

跟readi函数差不多。只是读函数改写成写函数即可，最后更新一下ip自身的dinode数据即可。

readi和writei是对接文件系统上层的主要接口。


### 五、文件夹（inode）操作
----------------------------------

文件夹在xv6的文件系统中是作为特殊文件处理的，区别是：type类型不同和文件内容扇区中存放的是文件夹中文件（inode）的索引和名称对象（dirent对象）。

```
int
namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZ);
}

```

字符串比较函数。只不过限制了比较长度是文件名称长度DIRSIZ。

```
// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode*
dirlookup(struct inode *dp, char *name, uint *poff)
{
  uint off, inum;
  struct dirent de;

  if(dp->type != T_DIR)
    panic("dirlookup not DIR");

  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlink read");
    if(de.inum == 0)
      continue;
    if(namecmp(name, de.name) == 0){
      // entry matches path element
      if(poff)
        *poff = off;
      inum = de.inum;
      return iget(dp->dev, inum);
    }
  }

  return 0;
}

```

从文件夹中寻找一个名称为name的文件（或者文件夹）。如果找到了则返回该文件的inode（调用iget函数获取inode）。注意，这里除了返回inode之外，还会通过参数poff返回该文件（dirent实体）在文件夹扇区中的偏移量，方便用户程序修改该文件元数据（dirent对象）。

```
// Write a new directory entry (name, inum) into the directory dp.
int
dirlink(struct inode *dp, char *name, uint inum)
{
  int off;
  struct dirent de;
  struct inode *ip;

  // Check that name is not present.
  if((ip = dirlookup(dp, name, 0)) != 0){
    iput(ip);
    return -1;
  }

  // Look for an empty dirent.
  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlink read");
    if(de.inum == 0)
      break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inum = inum;
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("dirlink");
  
  return 0;
}

```

该函数写入一个文件（或者文件夹）到dp表示的文件中。原理是向dp的磁盘扇区写入一个文件索引实体（dirent实体）。

文件夹操作到此结束。注意文件夹的抽象方法。（文件夹作为特殊文件）

### 六、文件路径抽象
----------------------------------

对用户来讲，文件系统应该提供文件操作的路径抽象。用户操作文件或者文件夹都是通过文件路径实现。

```
static char*
skipelem(char *path, char *name)
{
  char *s;
  int len;

  while(*path == '/')
    path++;
  if(*path == 0)
    return 0;
  s = path;
  while(*path != '/' && *path != 0)
    path++;
  len = path - s;
  if(len >= DIRSIZ)
    memmove(name, s, DIRSIZ);
  else {
    memmove(name, s, len);
    name[len] = 0;
  }
  while(*path == '/')
    path++;
  return path;
}

```

该函数返回路径上的一个节点名称和剩下的路径。

比如：

```
skipelem("a/bb/c", name) = "bb/c", setting name = "a"

```

通过路径返回inode的函数：

```
// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
static struct inode*
namex(char *path, int nameiparent, char *name)
{
  struct inode *ip, *next;

  if(*path == '/')
    ip = iget(ROOTDEV, ROOTINO);
  else
    ip = idup(proc->cwd);

  while((path = skipelem(path, name)) != 0){
    ilock(ip);
    if(ip->type != T_DIR){
      iunlockput(ip);
      return 0;
    }
    if(nameiparent && *path == '\0'){
      // Stop one level early.
      iunlock(ip);
      return ip;
    }
    if((next = dirlookup(ip, name, 0)) == 0){
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  if(nameiparent){
    iput(ip);
    return 0;
  }
  return ip;
}

```

使用skipelem函数获取路径上的节点（inode），路径上的节点肯定是文件夹。使用dirlookup函数在文件夹ip中寻找名称为name的文件或者文件夹（inode）。直到路径的终点（如果设置了nameiparent参数，则返回终点文件所在的文件夹inode）。整个寻找过程是在循环中不断解析路径并寻找inode进行的。

```
struct inode*
namei(char *path)
{
  char name[DIRSIZ];
  return namex(path, 0, name);
}

struct inode*
nameiparent(char *path, char *name)
{
  return namex(path, 1, name);
}

```

没什么好讲的，namei调用namex函数返回路径所代表的文件inode，nameiparent函数则返回文件所在文件夹inode。

到这整个文件系统关于磁盘文件（数据文件、文件夹、设备文件）的操作（读写、取元数据等），分析完成。理解该部分最重要的除了bio层和log层，就是文件在磁盘中的布局。

![磁盘文件系统布局](http://arvinsfj.github.io/public/ctt/documents/osxv6/dk_l.png)

最后附一张整个文件系统的分层结构图：

![文件系统分层结构](http://arvinsfj.github.io/public/ctt/documents/osxv6/fs_h.png)


### 七、随便说点
----------------------------------

该篇应该是文件系统最重要的文章。后面还剩下文件描述符层面的实现和pipe实现，再就是关于文件系统的系统调用函数实现。

虽然该篇比较长，讲的东西很多，但是它是文件系统的核心实现，需要花时间去理解（ps：本篇看不懂，文件系统你就不懂）。

还有就是，注意文件系统各层的抽象（功能划分），以及每层的接口函数（抽象）。

-----------------------------------

> END

