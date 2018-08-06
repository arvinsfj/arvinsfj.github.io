
# xv6操作系统进程（proc）

> *作者：Arvin 日期：2018年8月6日*

---------------------------------

>BEGIN

现代操作系统都有进程。

### 一、前言
----------------------------------

xv6基于unix v6，当然也有进程。xv6进程有几种状态：UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE。进程的调度程序就是一个状态机。这几个状态的切换，大多数教材已经讲得太多，这里不讲，我们只讲进程实现和进程调度。

### 二、数据定义（proc.h）
----------------------------------

多核CPU结构体定义如下：

```
// Segments in proc->gdt.
#define NSEGS     7

// Per-CPU state
struct cpu {
  uchar id;                    // Local APIC ID; index into cpus[] below
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  
  // Cpu-local storage variables; see below
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Per-CPU variables, holding pointers to the
// current cpu and to the current process.
// The asm suffix tells gcc to use "%gs:0" to refer to cpu
// and "%gs:4" to refer to proc.  seginit sets up the
// %gs segment register so that %gs refers to the memory
// holding those two variables in the local cpu's struct cpu.
// This is similar to how thread-local variables are implemented
// in thread libraries such as Linux pthreads.
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc

```

结构体cpu，定义了CPU的的一些信息：lapic id， 上下文（context，是一个拥有寄存器变量的结构体指针），ts（x86找中断状态的栈，暂时不清楚作用。），gdt（全局描述符表，7条表项），started（CPU是否已经启动），ncli（pushcli的嵌套深度，暂时不知道用途），intena（在pushcli之前中断是否开启，暂时不知道用途），cpu和proc（这两个字段是当前CPU存储的局部变量，保存在每个CPU的SEG_KCPU段，有点难解释：你可以理解成CPU自己在SEG_KCPU段存储的关于CPU自己的一些变量）。

cpus数组，是多核CPU数组，数组的每一项代表一个CPU（信息）。

ncpu，cpus数组的长度。

```
// Per-CPU variables, holding pointers to the
// current cpu and to the current process.

每个CPU变量，拥有指向当前cpu结构体变量的指针和当前进程的指针。
```

SEG_KCPU段，使用的段寄存器是gs，gs:0指向cpu结构体变量，gs:4指向proc变量。这两个变量其实就是结构体cpu中最后的两个变量（cpu和proc），而倒数第二的cpu变量，是一个结构体指针，指向的就是它自己所在的cpu结构体变量实体。有点绕:]。proc是指当前CPU正在执行的进程指针。

下面是context和proc结构体定义：

```
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  volatile int pid;            // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

```

context结构体只需要保存：edi、esi、ebx、ebp、eip。其他的都不需要或者隐式的被保存了。context是用于进程切换的。

procstate枚举定义了进程的6中状态。

结构体proc，定义了一个进程（所有信息）。进程是程序的运行状态。这些状态信息（或者说元信息）就保存在这个proc结构体变量中。那么程序在哪？pgdir字段定义的是页目录，页目录中定义了页表，页表中定义了页面，这些页面就是进程程序所在的内存位置。sz是进程占用的内存大小（单位：字节）。kstack是内核栈指针。state进程状态（只有处于RUNNABLE状态，进程调度程序才会去执行该进程）。pid进程唯一标识。parent父进程指针（类unix系统所有进程构成一棵进程树）。tf陷阱帧的栈上指针（应该是内核栈）。context进程切换的上下文（寄存器值的切换）。chan中文意思是槽，可以理解成进程同步时用到的等待槽点，如果该值为1，则在进程调度的时候等待在该变量上的进程会继续等待，直到该值为0，结束等待。注意这个chan的类型是void指针，也就是说它可以是任何类型的实体（变量）。换句话，槽点可以是任何内核实体（变量）。killed标记进程是否已经被杀掉。ofile数组是进程打开的文件（file）（类unix系统至少会打开3个file：stdin、stdout、stderr）。cwd当前进程所在的工作目录（当然是文件夹啦）。name字符数组是进程的名称，debug用。

```
// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
```

上面是进程的内存布局。从上向下是低地址到高地址。

进程表：

```
struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;

```

ptable是内核中的进程表，同步使用（lock），proc是进程数组（实际的进程表）。initproc记录了一个特殊进程init的指针。nextpid是进程id的全局变量，每创建一个新进程，该变量自加1。


### 三、进程的函数分析（proc.c）
----------------------------------

初始化函数如下：

```
 void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

```

初始化进程表的自旋锁即可。

进程结构体分配：

```
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

```

首先遍历进程表，找到一个UNUSED的进程结构体proc的变量。注意UNUSED是等于0的，数组在创建的时候所有变量的数值都是置0的。

之后使用goto到found标签。改变状态成EMBRYO，分配pid，释放锁。

最后，分配该进程的内核栈（占用1个页面）。初始化tf、context。注意在tf和context之间插入一个trapret指针（其实是一个汇编的标签：trapasm.S文件）。tf在内核栈的高地址，之后是trapret指针，最后是context（context的eip被设置成forkret函数入口指针）。最后返回进程结构体指针p。（进程结构体变量是在ptable中的，它的内核栈是分配的内存页面）。

KSTACKSIZE定义成4096。```sp = p->kstack + KSTACKSIZE;```让sp指向页面的尾部。

eip设置成forkret是为了内核函数调用返回到forkret函数执行。

```
// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    initlog();
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

```

第一次执行的时候初始化文件系统的log系统。其他情况，直接返回“return”。ps：C语言即使在函数末尾没有return关键字，也会在编译的时候自动加上ret指令（从函数返回）。ret这条指令执行之后会弹出进程内核栈的下一个变量（返回地址），这个时候就是上面设置的trapret标签地址。并执行trapret开始的汇编代码。

```
  # Return falls through to trapret...
.globl trapret
trapret:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret

```

trapret弹出进程内核栈的tf帧，然后调用中断返回指令iret，切换到用户空间。

注意一下：allocproc函数一个通用函数，任何新建的进程都会调用该函数。

-----------------------------------

设置第一个进程：

```
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
}

```

这个函数只会调用一次，不是通用函数。

```_binary_initcode_start```是打包进内核的```initcode.S```代码开始地址。

首先调用allocproc函数分配proc结构体变量和分配进程的内核栈。初始化tf、context、trapret等。

将该进程标记成init进程。然后设置进程内核页目录（页表、页面等），页目录指针赋值给pgdir字段。然后调用inituvm函数，分配保存进程程序text、data等的页面（逻辑地址0，映射成页面的虚拟起始地址，可写并且是用户级别），将该页面映射到pgdir所指向的页目录中，最后将initcode.S汇编代码移动到该页面中。

进程占用PGSIZE大小（字节），一个页面。(注意进程内核栈也是占用一个页面，区别这个地方的页面)。这个地方的页面是为了存储进程的程序text、data、用户栈等数据的。(即，调用inituvm分配的页面)

设置进程的tf结构，tf是在分配内核栈的时候分配的，也就是tf是在内核栈上的（而且是在高地址上的：栈底）。注意tf被设置的数值。tf在从内核切换到用户空间的时候，起到桥梁作用，它的值会赋给对应寄存器的。DPL_USER指代用户级别。FL_IF打开CPU中断。```p->tf->esp = PGSIZE;```指向用户栈栈底。```p->tf->eip = 0;```指向initcode.S的开始，虚拟地址0。最后设置一下进程名称、当前工作目录和进程状态RUNNABLE。

inituvm函数如下：

```
// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;
  
  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, v2p(mem), PTE_W|PTE_U);
  memmove(mem, init, sz);
}

```

只执行一次，非通用函数。

在userinit函数执行完成之后，initcode.S就可以作为一个进程在进程调度程序之下执行了。注意这个进程创建的整个过程：分配proc和内核栈、设置内核页目录等、分配进程用户页面和载入程序并关联到进程内核页目录、设置tf（其实是在设置用户进程的寄存器值）、最后设置一下名称工作目录和进程状态。最后的最后调度器会切换到该进程执行。

注意，第一个用户进程的段表设置是在seginit函数中设置的，可以回头看看。

```
c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);

```

你可以认为xv6的段映射是一个单位映射，不起任何作用（段权限级别除外），起作用的是分页机制。

上面就是xv6的第一个用户级别的进程（initcode.S）创建过程。该进程代码如下：

```
# exec(init, argv)
.globl start
start:
  pushl $argv
  pushl $init
  pushl $0  // where caller pc would be
  movl $SYS_exec, %eax
  int $T_SYSCALL

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

```

其实就start标签起作用，它是一个SYS_exec系统调用。请记住，xv6的第一个进程并不是init.c定义的进程或者shell，而是这段不起眼的系统调用代码。这段代码的作用后面再进行分析，你可以认为它是一个创建shell用户进程的出口。

---------------------------------

用户进程内存增长函数：

```
// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

```

将用户进程所占用的内存（text、data、用户statck等）增加n字节。该函数为了支持系统调用SYS_sbrk。

allocuvm函数：

```
// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;

  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    mappages(pgdir, (char*)a, PGSIZE, v2p(mem), PTE_W|PTE_U);
  }
  return newsz;
}

```

用户进程页面是在KERNBASE之下的。使用for循环分配页面直到分配的页面足够多（满足新增字节的大小）。并且这些新分配的页面会加入到进程的页目录中。最后返回新进程占用的字节大小。

用户进程页面缩减函数：

```
// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a  < oldsz; a += PGSIZE){
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a += (NPTENTRIES - 1) * PGSIZE;
    else if((*pte & PTE_P) != 0){
      pa = PTE_ADDR(*pte);
      if(pa == 0)
        panic("kfree");
      char *v = p2v(pa);
      kfree(v);
      *pte = 0;
    }
  }
  return newsz;
}

```
本质是释放页目录中不必要的页表。

switchuvm函数：

```
// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
  pushcli();
  cpu->gdt[SEG_TSS] = SEG16(STS_T32A, &cpu->ts, sizeof(cpu->ts)-1, 0);
  cpu->gdt[SEG_TSS].s = 0;
  cpu->ts.ss0 = SEG_KDATA << 3;
  cpu->ts.esp0 = (uint)proc->kstack + KSTACKSIZE;
  ltr(SEG_TSS << 3);
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");
  lcr3(v2p(p->pgdir));  // switch to new address space
  popcli();
}

```

切换tss段和页目录（用户空间的）。tss段不怎么懂，但是好像用的地方也不多，暂时就这样了。

----------------------------------

fork函数：

```
// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

```

fork函数应该很有名了，作用自己去查一下。

首先，调用allocproc函数，分配proc变量和内核栈。调用copyuvm函数，拷贝当前进程的页表数据，注意这个地方是拷贝页表，原进程的页目录和页表都没有修改而且新进程的页目录和页表都是新分配的页面，最后将新分配的页目录基地址赋值给新进程的pgdir。换句话，新进程和原有进程通过分页机制映射的物理地址空间是一样的，也就是新进程共用原有进程的内存空间。

```
// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)p2v(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, v2p(mem), PTE_W|PTE_U) < 0)
      goto bad;
  }
  return d;

bad:
  freevm(d);
  return 0;
}

```

注意该函数。

之后，fork函数中设置新进程的sz、tf跟原有进程一样，parent为原有进程（也就是当前进程）。新进程tf中的eax（函数返回值）设置为0。(也就是fork调用之后新进程返回0，老进程返回子进程的pid)。然后拷贝（使用filedup函数）原有进程打开的文件描述符数组和当前工作目录。```pid = np->pid;```设置父进程的fork返回为子进程的pid。最后修改新进程的状态成RUNNABLE和拷贝原有进程debug名称。

fork函数调用1次返回2次。父进程的返回应该很简单，也就是fork函数的系统调用返回。那么子进程的返回呢？

注意这句话：```np->state = RUNNABLE;```。设置完成后，子进程就会在进程调度器的作用下执行。执行的时候，因为tf和内存空间跟原有进程完全一样（除了eax），在切换的时候会执行forkret和trapret，会返回到原有进程调用fork之后的一条指令开始执行，但是返回值是0（eax在子进程的tf中被设置成0）。很巧妙！代码是同一份，但是是两个独立的进程。

---------------------------------

exit函数：

```
// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

```

首先关闭该进程的已经打开的文件描述符。关闭当前工作目录。调用wakeup1函数唤醒父进程。遍历当前进程的子进程设置子进程的父进程为init进程，如果子进程为ZOMBIE状态则唤醒init进程。最后吧当前进程的状态修改成ZOMBIE状态，调用调度器切换进程，等待父进程被唤醒。

注意：exit函数并不返回。

---------------------------------

wait函数：

```
// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

```

等待子进程的exit，并返回子进程的pid。这个函数会检测子进程的状态为ZOMBIE状态则返回，否则会一直等待（for循环）。

如果子进程的状态为ZOMBIE状态，则函数返回子进程的pid。同时释放子进程的内存资源（内核栈、进程页面）并设置子进程为UNUSED状态，回到ptable等待复用。也就是，exit并不释放子进程的资源，它是在父进程的wait函数中释放的。

```
// Wait for children to exit.  (See wakeup1 call in proc_exit.)
sleep(proc, &ptable.lock);  //DOC: wait-sleep

```

这个地方会让当前进程进入睡眠状态（等待标志是当前进程自身）。所以子进程中需要wakeup1父进程（传入的唤醒标志为proc->parent）。wait函数中的for循环并不是一直占用cpu时间的，因为用了sleep函数。这种细节自己体会一下。ps：死循环跟一直占用cpu时间是两回事，中间隔着进程调度器。

----------------------------------

sleep函数：

```
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

```

首先设置睡眠的等待"槽点"chan。然后设置进程为SLEEPING状态。然后调用 ```sched();```函数。启动进程调度器，切换执行进程。如果当前进程从sleep中醒来，则清除chan，释放锁等。

-----------------------------------

wakeup1函数：

```
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

```

唤醒所有在chan上睡眠的进程。也就是修改进程的状态SLEEPING成RUNNABLE。在下一次调度的时候的让进程可以被执行。

wakeup函数：

```
// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

```

直接调用wakeup1函数，只不过做了获取进程表的锁和释放锁的操作。也就是多了多核同步使用操作。

----------------------------------

kill函数：

```
// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

```

遍历进程表找到要被杀死的进程，设置killed标记为1（代表被kill了）。如果该进程在sleep，则唤醒它。注意该函数只是设置了进程的killed标记，并没有释放进程的资源。那么在那个地方释放呢？

还记得trap函数吗？也就是陷阱（中断）分配函数。不管是中断还是陷阱，都会去检查进程的killed字段，如果字段为1，则调用exit函数。exit函数设置进程的状态成ZOMBIE，并唤醒父进程。父进程在被唤醒的时候会检查子进程的状态，如果为ZOMBIE，则释放该子进程的内存资源（文件描述符等资源是在exit函数中释放的）。大致是这样的释放过程。

进程的分配、第一个用户进程创建、进程内存的扩展、进程fork以及进程的状态转移等函数，大致如上所讲。下面着重看看进程的调度器。

### 四、进程调度器
----------------------------------

scheduler函数：

```
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);

  }
}

```

整体上是一个不断执行的for循环，换句话，进程调度器会一直执行。```sti();```打开当前CPU的中断标志。同步使用进程表ptable。

找到一个状态为RUNNABLE状态的进程（RUNNABLE状态可以认为是执行准备就绪状态）。该进程作为当前进程。调用switchuvm函数，当前CPU切换成该进程的tss段和页目录（内存地址空间）。设置进程的状态成RUNNING状态（正在执行状态）。调用switch函数，先保存内核自身的寄存器信息到内核自身的栈上，然后切换栈到用户进程的内核栈（内核自身的栈到用户进程的内核栈），最后弹出用户进程内核栈上的寄存器的值到对应的寄存器，并执行ret指令函数返回。换句话，switch函数切换调度器程序到用户进程。

注意```ret```指令，非常关键。注意此时用户进程内核栈的布局（在创建第一个用户进程的时候讲过）。context的起始地址是esp的值，也就是栈顶指向context结构体变量，该变量的最后一个字段是eip（高地址上）。```ret```会弹出字段eip的数值到CPU的eip寄存器，改变程序执行的路径。这个地址是forkret函数入口地址，forkret函数最后再执行ret指令返回到trapret处代码，弹出tf（恢复用户进程的用户空间寄存器值），最后调用iret指令，从中断返回，进入用户空间执行代码。换句话，switch函数的ret指令会让进程从内核空间切换用户空间执行，并且寄存器值再执行之前全部被切换成用户空间的值。

注意，switch函数这里我们只考虑从调度器上下文切换到用户进程上下文，相反方向也是可以的。自己分析啦！

```
# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save current register context in old
# and then load register context from new.

.globl swtch
swtch:
  movl 4(%esp), %eax  # &(cpu->scheduler) , 即cpu变量scheduler字段的地址
  movl 8(%esp), %edx  # proc->context , 即是用户进程的内核栈的栈顶指针esp

  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl %esp, (%eax)  # 该句会将%esp，内核自己的栈顶地址赋值给当前cpu变量的scheduler字段。也就是scheduler指向内核自身栈顶。
  movl %edx, %esp    # 将进程的proc->context（也就是进程的内核栈的栈顶地址）赋值给当前CPU的esp寄存器

  # Load new callee-save registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret

```

这里，栈的切换全部是指内核中的栈切换（内核自身的栈和用户进程的内核栈）。不涉及用户进程用户空间的栈。


回到scheduler函数。在执行switch函数（不是正常的C函数，只是可以调用形式是C的函数调用形式）之后，调度器是不会继续执行的（此时执行的是当前进程用户空间代码）。当当前进程放弃执行的时候（时间片用完了、主动放弃等系统调用），执行权会回到调度器手上（同样会调用switch函数，切换栈、寄存器等环境，并ret到switch函数下面的语句），会继续执行switch函数下面的语句，调用```switchkvm();```函数，切换页目录（地址空间）到内核自身的页目录kpgdir。然后继续寻找下一个RUNNABLE状态的进程并切换到该进程执行。

这就是进程调度器核心。非常巧妙，程序执行的流程跟用户程序完全不一样。ps：写应用层面的程序是见不到的。

----------------------------------

进程调度还有两个函数：

```
// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

```

sched函数的前面几条if判断，自然是进程切换的前提条件（具体代表什么自己分析）。

后面三句是关键，```swtch(&proc->context, cpu->scheduler);```切换当前进程到内核调度器。注意参数方向。

```cpu->intena = intena;```恢复intena。ps：intena暂时不知道它的作用。

yield函数，是进程主动放弃执行权调用的函数。设置当前进程状态为RUNNABLE状态（之前应该是RUNNING状态），然后调用sched函数切换执行权到内核调度器。（内核调度器会切换到其他进程的）。


这就是全部的xv6进程创建和进程调度部分。看似简单，实则复杂。不过，按照逻辑分析还是能理解的。

### 五、补充说明：shell进程的创建
----------------------------------

前面讲到```initcode.S```文件的代码如下：

```
# exec(init, argv)
.globl start
start:
  pushl $argv
  pushl $init
  pushl $0  // where caller pc would be
  movl $SYS_exec, %eax
  int $T_SYSCALL

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

```

本质是一个```SYS_exec```系统调用，参数是："/init\0" 和 { init, 0 } 。用C来表达就是```exec(init, argv)```。

也就是将当前进程的执行代码，替换成init程序，那init是什么？(在init.c文件中)

```
// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf(1, "init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}

```

它完全是一个正常的用户程序。也就是执行完```SYS_exec```系统调用之后，当前进程的程序变成了这段代码，并从main函数执行。```SYS_exec```系统调用自己分析一下（从内核加载init程序到当前进程用户空间的页面，并执行入口函数）。这段程序，大量使用了系统调用，比如：open终端、dup(0)、fork()、printf()、exit()、exec()等。这些系统调用实现自己去慢慢分析吧，有了之前的基础分析起来并不难。

主要分析for循环里面的代码。

fork()调用，创建一个子进程。子进程调用exec，替换init进程执行实体成sh程序。父进程init调用wait等待子进程sh结束。

这个sh程序就是shell程序，这个子进程就是shell进程。shell是什么呢？你可以认为是一个用户命令解释器。也可以认为是shell脚本的解释器。还可以理解成用户命令行操作界面。看你从什么角度看待shell吧。但是shell进程确实是用户级别的进程，不属于内核。


### 六、随便说点
----------------------------------

上面讲的进程比较糙，细节部分需要自己去研究，特别是进程跟各个os子系统之间的联系。并且进程比较难，执行的路径比较复杂。涉及调度器跟用户进程之间的切换，以及用户空间跟内核空间的切换。注意trapframe的结构，毕竟是内核跟用户空间之间切换的桥梁。

至此，xv6操作系统序列文章全部结束。系统调用的实现（大致分为进程方面的和文件系统方面的）和用户标准库等，没有分析，这部分留给读者自己去研究吧。

xv6操作系统内核，大致分为：boot、内存（分段、分页）、多核、中断和陷阱、终端、uart、文件系统、进程等几个主题。每个主题涉及的细节很多，需要慢慢吸收。

我开这个系列，主要有几个目的：

1. 国内教材或书籍偏理论，给人云里雾里的感觉，想从代码角度入手os的各种理论和实现
2. 对内存管理、文件系统、中断、进程加深理解，并能够实践
3. 对硬件编程的实践
4. 理解各种锁机制和实现
5. "一切皆文件"的实现

这几个目的基本都达成了。现在我对os有一个整体而且具体的理解。额外的好处是多核、终端、管道等的实现细节，并且对C语言的理解进一步加深（之前已经很深了，深入理解C是学习操作系统的基本前提）。写完这个系列差不多用了1个月的时间（阅读源码、理解和分析、实践、写文章），感觉已经很快了。

理解这些内容，并不是让你去写一个操作系统（当然也是可以的），而是让你对高层概念的底层实现（比如：锁、终端、中断、文件管理等）有一个更加具体的理解，并能更好的用于实践（而不出错）。ps：你还可以增强xv6的功能，比如：完善系统调用、修改成64位系统、添加GUI界面。

有机会再进一步细化一些细节上的实现（说明）。后面一段时间给自己放个短假吧。

-----------------------------------

最后，附上xv6的全部源码（请在linux上编译，qemu上运行）。[xv6源码](http://arvinsfj.github.io/public/ctt/documents/osxv6/TestXV6.zip)

还有 [xv6官方文档](http://arvinsfj.github.io/public/ctt/documents/osxv6/book-rev8.pdf)。

-----------------------------------

**All Copyright reserved for avin.**

[Gmail](mailto:arvin.sfj@gmail.com)

-----------------------------------

> END

