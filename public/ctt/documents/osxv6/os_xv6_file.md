
# xv6操作系统文件系统文件描述符和PIPE（file和pipe）

> *作者：Arvin 日期：2018年8月3日*

---------------------------------

>BEGIN

经过fs的洗礼，文件系统整体结构可谓很清晰。相对于fs本篇很简单，只是对fs的上层封装。当然，pipe（管道）也是作为file来处理的。

### 一、前言
----------------------------------

本篇分析文件描述符（fd）层，fd是一个文件系统分配的整数，该整数对应于一个file对象。file对象有可能是代表一个inode或者一个pipe。在该层，file封装了inode和pipe。类unix系统“一切皆文件”的思想，在这个地方有所体现（另外一个地方是inode封装了设备文件，即外设也作为文件inode来处理）。

### 二、file对象数据定义（file.h）
----------------------------------

先看inode和devsw数据定义如下：

```
// in-memory copy of an inode
struct inode {
  uint dev;           // 设备编号
  uint inum;          // inode索引号
  int ref;            // 该inode（文件）被文件夹引用（包含）的次数
  int flags;          // inode（文件）的状态：I_BUSY, I_VALID

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};
#define I_BUSY 0x1
#define I_VALID 0x2

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

struct devsw devsw[NDEV];

#define CONSOLE 1 // 终端（console）的主设备编号，在外设列表devsw数组中的下标。

```

inode代表一个内存中的文件，它拷贝了磁盘中的dinode（结构体的下半部分，请参考上一篇中dinode的说明），并且自带一些额外的字段（前半部分），比如：ref、flags等。

flags这个字段到这已经见了很多（大多是类似的概念），主要标记该inode的状态：I_BUSY（inode正在被使用中，需要等待）、I_VALID（inode的数据跟对应的dinode已经同步）。

devsw结构体，定义了read和write函数指针，代表一个外设（比如：console）的读写函数（入口）。该结构体对象存放在devsw数组中。系统通过外设编号（比如：CONSOLE）就可以获取到该外设的读写（函数）对象了。这里其实将外设抽象成一个能够读数据和写数据的对象（devsw对象）。对于文件系统（或者操作系统）来讲，并不关心外设的读写实现细节和外设的其他属性（外设驱动程序只需要实现上面定义的两个[read和write]函数即可）。ps：这才是抽象和封装。NDEV定义成10，也就是最多能支持10个外设（console占用了一个）。

devsw数组，文件系统可见的外设列表。

----------------------------------

file数据定义：

```
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};

struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

```

type, 枚举类型：FD_NONE（file对象空闲，可被重新分配）、FD_PIPE（file对象是管道pipe）、FD_INODE（file对象是磁盘文件inode）。

ref，file对象被进程使用（引用）的次数。注意区别inode中的ref（它是表示inode被文件夹包含的次数）。

readable和writable，标记该file对象读写是否可用属性。

pipe，表示file对象代表的管道对象。（此字段有效，type必须为FD_PIPE）

ip，表示file对象代表的磁盘文件inode对象。（此字段有效，type必须为FD_INODE）

off，file对象的读写偏移量。（是用户层面所讲的文件“游标”，表示文件读写的当前位置）。ps：读和写好像是用的同一个off。

ftable，是文件系统全局变量，保存了系统中可使用的的所有file对象（使用或者空闲的）。NFILE被定义成100，也就是最多100个file对象。当然ftable需要同步使用。

### 三、file对象操作分析（file.c）
----------------------------------

初始化：

```
void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

```

初始化自旋锁即可。

file分配函数：

```
// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

```

遍历ftable中的file数组，找到一个空闲（这里使用ref为0的条件）的file对象，设置ref为1，然后返回该file即可。

dup函数：

```
// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

```

对应于系统调用dup，ref自加1即可。

file对象close函数：

```
// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);
  
  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    begin_trans();
    iput(ff.ip);
    commit_trans();
  }
}

```

主要是ref减去1，如果减去1之后ref为0，则将该file对象置为空闲file对象（ref=0，type=FD_NONE），并且将file对象的实体（pipe或者ip）关闭。

pipe关闭调用pipe字节的关闭函数pipeclose函数。后面会见到该函数。

ip关闭，调用inode层的iput函数，并且注意因为iput用到写操作，必须使用事务。iput函数见前篇《xv6操作系统文件系统磁盘文件》。

这里体现了file对磁盘文件inode和管道pipe的封装（抽象）。

file对象获取文件属性（文件元数据）：

```
// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE){
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

```

pipe（管道）是没有元数据的。这里使用了inode的ilock和iunlock锁函数，实现同步操作。stati函数是拷贝inode的属性数据到st对象中，最后返回st对象。

file对象读取函数：

```
// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
  int r;

  if(f->readable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);
  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}

```

首先判断file是否可读，如果可读，则判断type类型，分别从pipe或者inode中读取实际数据到指定内存地址addr，最后返回读取的字节数量。

file对象写入函数：

```
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return pipewrite(f->pipe, addr, n);
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((LOGSIZE-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_trans();
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      commit_trans();

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    return i == n ? n : -1;
  }
  panic("filewrite");
}

```

跟fileread函数实现差不多。读函数修改成写函数。只是写函数使用了循环写入。最后返回实际写入的字节数量。

注意写入是使用事务写入的，而且循环中每次写入的数据大小不能超过log数据区域的大小（事务写入需要先在log数据区域缓存一下）。

到这，file对象及其操作方法全部写完。下面分析一下pipe对象（跟file关系比较紧密）。

### 四、pipe对象分析（pipe.c）
----------------------------------

数据定义：

```
#define PIPESIZE 512

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE]; // 数据缓冲区，512字节
  uint nread;     // 读取的字节数量
  uint nwrite;    // 写入的字节数量
  int readopen;   // 读取fd是否还被打开
  int writeopen;  // 写入fd是否还被打开
};

```

管道（pipe）实际是系统中一段缓存区域（data数组，512字节）。其中还包含了读写fd的开启状态和读写的字节数量。管道需要同步使用。

你可以想象管道是一个数据水池（缓存区data），一头有一个进水口（写入口），另外一头有一个出水口（读取口）。

---------------------------------

pipe分配（创建）函数：

```
int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *p;

  p = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((p = (struct pipe*)kalloc()) == 0)
    goto bad;
  p->readopen = 1;
  p->writeopen = 1;
  p->nwrite = 0;
  p->nread = 0;
  initlock(&p->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = p;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = p;
  return 0;

//PAGEBREAK: 20
 bad:
  if(p)
    kfree((char*)p);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

```

代码虽然有点长，但逻辑简单。首先调用filealloc函数分配2个file对象（一个用于读，另外一个用于写），然后调用kalloc函数分配一个内存页（1个内存页面是4096字节，够用）作为pipe对象（这种创建[分配]结构体对象，之前见过）。后面就是设置pipe对象的各个字段值和2个file对象字段的值。pipe对象读和写都是打开的，file对象f0是读取对象，f1是写入对象，它们都是FD_PIPE类型并且实体都是pipe对象。

pipe关闭函数：

```
void
pipeclose(struct pipe *p, int writable)
{
  acquire(&p->lock);
  if(writable){
    p->writeopen = 0;
    wakeup(&p->nread);
  } else {
    p->readopen = 0;
    wakeup(&p->nwrite);
  }
  if(p->readopen == 0 && p->writeopen == 0){
    release(&p->lock);
    kfree((char*)p);
  } else
    release(&p->lock);
}

```

在file对象close操作中调用。因为管道存在2个file对象，所以该函数会调用2次。一次是关闭pipe的读取状态，一次是关闭pipe的写入状态。谁先调用谁后调用，其实无所谓的。如果pipe的读取和写入都被关闭了，则调用kfree函数释放pipe对象。

pipe写入函数：

```
int
pipewrite(struct pipe *p, char *addr, int n)
{
  int i;

  acquire(&p->lock);
  for(i = 0; i < n; i++){
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
      if(p->readopen == 0 || proc->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }
    p->data[p->nwrite++ % PIPESIZE] = addr[i];
  }
  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
  release(&p->lock);
  return n;
}

```

通过for循环，依次将addr位置的字节数据写入pipe对象的data数组中。

写入之前需要简单判断一下pipe缓存区是否已经满了；如果满了，则先唤醒读取进程然后写入进程需要等待直到缓存区有空闲位置；如果没有满，则直接写入数据到缓存区data，写完之后唤醒读取进程（让它去读取缓存区data中的数据）并返回实际写入的字节数量。

pipe是被2个进程操作的（一个读一个写，file对象也是分配在2个进程中的）。pipe是进程间传递数据（通信）的一种常见方式。

pipe读取函数：

```
int
piperead(struct pipe *p, char *addr, int n)
{
  int i;

  acquire(&p->lock);
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
    if(proc->killed){
      release(&p->lock);
      return -1;
    }
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
  }
  for(i = 0; i < n; i++){  //DOC: piperead-copy
    if(p->nread == p->nwrite)
      break;
    addr[i] = p->data[p->nread++ % PIPESIZE];
  }
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return i;
}

```

类似写入函数pipewrite。只是读取函数判断的是缓存区data是否为空，如果为空则读取进程需要等待，直到缓存区有数据。

如果缓存区有数据，则使用for循环依次从缓存区data中读取n字节到addr位置。最后唤醒写入进程并返回实际读取的字节数量。

这就是pipe（管道）的实现。很简单对吗 :0 。

### 五、随便说点
----------------------------------

到这，xv6的文件系统实现全部讲完。磁盘文件、文件夹和外设被抽象成inode（文件）对象，管道提供了一段缓存区并可以使用file对象读写，inode和pipe被抽象成了file对象。对于使用文件系统的系统调用函数，直接使用file对象及其操作（大部分系统调用）就可以完成对磁盘文件、文件夹、外设（比如：console）、管道（pipe）的使用。是不是很强大 :] !。

比如下面的系统调用：

```
int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;
  
  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

```

通过argfd和argptr函数获取到f和st参数，然后调用filestat函数即可。

其他使用文件系统的系统调用全部在 ```sysfile.c``` 文件中，自己花点时间分析一下。

-----------------------------------

> END

