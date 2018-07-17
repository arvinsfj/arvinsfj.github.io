
# xv6操作系统spinlock(多处理器的自旋锁)

> *作者：Arvin 日期：2018年7月17日*

---------------------------------

>BEGIN

自旋锁（spinlock）多用在多处理器上，实现计算机资源多处理器同步操作。xv6支持多处理器，自然需要自旋锁。

### 一、前言
----------------------------------

锁机制的实现都需要某种支持原子操作的指令。xv6中使用xchg指令，实现自旋锁。

### 二、自旋锁讲解
----------------------------------

自旋锁结构体定义如下：

```
struct spinlock {
  uint locked;       // Is the lock held?
  
  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  uint pcs[10];      // The call stack (an array of program counters)
                     // that locked the lock.
};

```

真正重要的是locked和cpu字段。其他字段基本都是为了debug用的。

初始化方法如下：

```
void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

```

没什么好讲的，使用起来一般是某个结构体下包含一个struct spinlock lock，以表示该结构体代表的资源是需要同步操作的。比如：xv6中kmem、ptable、bcache、ftable、icache等等。

锁的获取和锁的释放：

```
void
acquire(struct spinlock *lk)
{
  pushcli(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire");

  // The xchg is atomic.
  while(xchg(&lk->locked, 1) != 0);//其他cpu调用acquire方法时，会在这个地方会阻塞，直到当前cpu调用了release方法。

  // Record info about lock acquisition for debugging.
  lk->cpu = cpu;
  getcallerpcs(&lk, lk->pcs);
}

void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->pcs[0] = 0;
  lk->cpu = 0;

  xchg(&lk->locked, 0);//释放锁的核心语句

  popcli();
}

int
holding(struct spinlock *lock)
{
  return lock->locked && lock->cpu == cpu;
}
```

上面的三个方法是实现的核心。

xchg函数的底层实现如下：

```
static inline uint
xchg(volatile uint *addr, uint newval)
{
  uint result;
  
  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}
```

使用了xchgl指令，该指令是原子性的。作用是将原值设置成新值，并返回原值。

最关键的2条语句如下：

```
while(xchg(&lk->locked, 1) != 0);

和

xchg(&lk->locked, 0);
```

注意上面的2条语句是在多核环境下执行的。假设当前cpu执行了获取锁操作之后，则lk->locked的值为1。其他cpu再获取锁的时候，xchg(&lk->locked, 1)始终会返回1，导致不能跳出while循环（“自旋”的来历）。当当前cpu调用了释放锁的操作，lk->locked被设置成0。这个瞬间，其他阻塞在循环里的cpu竞争锁资源，其中一个cpu的xchg(&lk->locked, 1)会返回0，并且跳出while循环，得到锁。其他的cpu的xchg(&lk->locked, 1)还是会返回1继续等待阻塞，直到得到锁的cpu调用了释放锁操作。这就是自旋锁的原理。自旋锁跟其他锁的不同之处，是阻塞的方式采用了死循环，而非其他方法。

思考锁的同步问题，需要在多核、多进程或者多线程环境下思考。因为涉及锁的竞争，所以需要使用原子操作的指令支持。


> END

