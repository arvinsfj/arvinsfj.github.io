
# xv6操作系统中断和陷阱（Trap and Interrupt）

> *作者：Arvin 日期：2018年7月30日*

---------------------------------

>BEGIN

在前面我们完成了中断的芯片级编程。可以说，os完全是由中断驱动执行的。中断、陷阱、异常以及系统调用，之间的关系和区别，这里不讲，有兴趣的自己去找资料。这里讲，中断的os层实现。

### 一、前言
----------------------------------

中断的大致原理如下：内核中有一张全局中断向量表（idt），cpu有一个寄存器idtr专门记录该表的基地址和长度，idt表的每个表项是一个门描述符实体（gatedesc），每个门描述符代表一个中断向量（门描述符是通用的，它既可以是中断门，还可以是陷阱门，也可以是任务门），每个向量最重要的是包含了寻找中断服务程序的入口地址的信息（代码段描述符和地址偏移量）。不管是硬件中断还是软件中断，都会产生一个中断向量编号（也就是idt表的索引编号），根据该编号就可以找到对应的中断服务程序并执行它，中断返回使用iret指令即可。门描述符中除了中断服务程序的入口地址信息，还包含了该表项（中断向量）的访问权限DPL，控制用户程序是否能访问该表项，在xv6中，只有64号向量用户程序是可以访问的，该向量就是系统调用（system call）的软中断向量。关于x86中断体系也可以参考[这里](http://www.mouseos.com/arch/interrupt.html)。

### 二、中断向量表初始化
----------------------------------

门描述符结构体定义如下：

```
// Gate descriptors for interrupts and traps
struct gatedesc {
  uint off_15_0 : 16;   // low 16 bits of offset in segment
  uint cs : 16;         // code segment selector
  uint args : 5;        // # args, 0 for interrupt/trap gates
  uint rsv1 : 3;        // reserved(should be zero I guess)
  uint type : 4;        // type(STS_{TG,IG32,TG32})
  uint s : 1;           // must be 0 (system)
  uint dpl : 2;         // descriptor(meaning new) privilege level
  uint p : 1;           // Present
  uint off_31_16 : 16;  // high bits of offset in segment
};

```

在32位机器上，共占用8字节。其中我们需要关心的字段：地址偏移量（off_15_0、off_31_16）、代码段描述符（cs）、门描述符类型（type）和门描述符的优先级（dpl）。

其中，cs是内核的代码段描述符，根据先前的定义的段描述符表（seginit函数中定义的），可以知道cs的值是```SEG_KCODE<<3```，映射到内存区域[0, 4G)，可读可执行，段描述符优先级为0（特权级别，内核级别）。基地址是0。

偏移量共32位，寻址空间达到4GB。加上前面从cs中拿到的基地址，就可以得到完整的线性地址（xv6开启了分页机制），通过分页机制就可以知道中断服务程序入口确定的物理地址了。

type定义了该门描述符的类型，是中断门还是陷阱门。（x86中还允许16的门描述符，不知道为什么）

dpl定义该门描述符的访问权限，0: 特权级；1: 用户级

![定位中断服务程序入口地址](http://arvinsfj.github.io/public/ctt/documents/osxv6/locate_interrupt_handler.png)

中断向量表初始化：

```
// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  
  initlock(&tickslock, "time");
}

```

首先要注意的是，在xv6中中断向量表是被所有CPU共享的。中断表包含256个中断向量。实际用不到这么多的中断向量，不用的表项设置一个哑中断服务程序（不做任何处理的程序）即可。

vectors.S文件中，使用汇编定义了256个中断服务程序入口地址，其实就是设置好中断编号，然后跳转到alltraps函数（这个函数是汇编写的）。注意：vectors.S是由vectors.pl脚本文件自动生成的。

SETGATE是一个设置门描述符变量的一个初始化宏。简化初始化工作。

值得注意的是idt是一个数组，定义成全局变量。idt数组中除了T_SYSCALL（64号中断），全部属性设置是一样的（当然中断服务程序入口地址不一样），T_SYSCALL是系统调用中断编号（软中断），该中断向量被设置成类型位陷阱门类型（STS_TG32），优先级设置成了用户级（DPL_USER，1）。也就是该中断向量描述符表项是可以被用户程序访问的。ps：其他中断向量是不能被用户程序访问的。

因为涉及，时钟中断，需要记录cpu周期（ticks），总运行的时钟周期资源需要同步使用，故使用了自旋锁（tickslock）。

接下来是设置idtr寄存器：

```
void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

static inline void
lidt(struct gatedesc *p, int size)
{
  volatile ushort pd[3];

  pd[0] = size-1;
  pd[1] = (uint)p;
  pd[2] = (uint)p >> 16;

  asm volatile("lidt (%0)" : : "r" (pd));
}

```

使用指令```lidt```加载idt数组的基地址和数组的大小到idtr寄存器。



### 三、中断分发器
----------------------------------

alltraps如下：

```
 # vectors.S sends all traps here.
.globl alltraps
alltraps:
  # Build trap frame.
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushal
  
  # Set up data and per-cpu segments.
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es
  movw $(SEG_KCPU<<3), %ax
  movw %ax, %fs
  movw %ax, %gs

  # Call trap(tf), where tf=%esp
  pushl %esp
  call trap
  addl $4, %esp

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

上面的汇编很有技巧。首先保存所有寄存器的值到栈中。注意```pushal```指令，它是将所有通用寄存器的值保存到栈（而不是只有ax寄存器的）。接着设置（切换）内核数据段寄存器（ds、es）和CPU数据段寄存器（fs、gs）。最后将当前esp栈顶指针值作为trap函数的参数保存到栈，调用trap中断分发函数。"当前esp栈顶指针值"其实就是trapframe结构实体的基地址，你可以理解成一个在栈上的一个trapframe结构体变量地址。```addl $4, %esp```其实就是为了将call之前的esp出栈（不是真正的出栈，没有使用pop指令，仅移动了esp）。之后进入```trapret```标签执行。恢复所有寄存器的值（寄存器出栈）。跳过 trapno 和 errcode 字段，直接将esp指向eip（中断返回到“用户程序”的地址）。最后使用 iret 指令，从中断返回。其中有很多东西没有讲，比如用户程序的寄存器是如何恢复的？为什么要定义trapret标签？（其实是为了跳过alltraps直接执行trapret标签下的指令）

trapframe结构体定义如下：

```
// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe {
  // registers as pushed by pusha
  uint edi;
  uint esi;
  uint ebp;
  uint oesp;      // useless & ignored
  uint ebx;
  uint edx;
  uint ecx;
  uint eax;

  // rest of trap frame
  ushort gs;
  ushort padding1;
  ushort fs;
  ushort padding2;
  ushort es;
  ushort padding3;
  ushort ds;
  ushort padding4;
  uint trapno;

  // below here defined by x86 hardware
  uint err;
  uint eip;
  ushort cs;
  ushort padding5;
  uint eflags;

  // below here only when crossing rings, such as from user to kernel
  uint esp;
  ushort ss;
  ushort padding6;
};

```

有点长，不过就是一些寄存器字段。padding是为了补足字段长度。最好，将操作跟该结构结合起来进行分析。


trap分发器函数：

```
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(proc->killed)
      exit();
    proc->tf = tf;
    syscall();
    if(proc->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpu->id == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpu->id, tf->cs, tf->eip);
    lapiceoi();
    break;
   
  //PAGEBREAK: 13
  default:
    if(proc == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpu->id, tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip, 
            rcr2());
    proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running 
  // until it gets to the regular system call return.)
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();
}

```

代码不难，优先处理系统调用中断。如果是系统调用则分发给syscall函数处理，系统调用编号（不同于中断向量编号）保存在tf栈帧的eax寄存器字段中。当然，系统调用是软中断，由用户进程产生。调用syscall函数前后要处理进程状态killed标记。

之后是trapno的正常分发，比如时钟中断，cpu周期ticks变量自加1，调用wakeup尝试唤醒一些等待在ticks上的进程，最后通知ioapic芯片，时钟中断已经完成，可以处理其他中断了。

在中断分发和处理完成之后，还要处理时钟中断，比如调用yield函数，当前进程主动放弃CPU，让调度程序决定接下来执行的进程。进程调度后面再讲。

自己可以试验一下键盘中断。


### 四、系统调用
----------------------------------

syscall函数：

```
static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
};

void
syscall(void)
{
  int num;

  num = proc->tf->eax;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    proc->tf->eax = syscalls[num]();
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            proc->pid, proc->name, num);
    proc->tf->eax = -1;
  }
}

```

从tf的eax寄存器字段，获取系统调用编号，然后根据该编号从syscalls数组中，获取系统调用服务函数入口地址并调用该函数。函数的返回值保存在tf的eax寄存器字段（eax字段最终会通过popal指令真实的赋值给eax寄存器，用户程序可以在eax寄存器中获取系统调用的返回值）。各个系统调用服务函数的实现，自己可以分析，你还可以添加一些额外的系统调用。（youtube上有人已经这样做了）

这里还有一个问题：虽然系统调用服务函数参数都是void，但实际的C库函数系统调用是有参数的，这些参数在内核中是如何获取到的？

我们拿close系统调用服务函数举例：

```
int
sys_close(void)
{
  int fd;
  struct file *f;
  
  if(argfd(0, &fd, &f) < 0)
    return -1;
  proc->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

```

在C库函数中close是关闭一个文件描述符FD，这个fd是作为参数传入内核的。内核如何获取该fd呢？

从上面的实现代码可知argfd函数，就是用来获取用户程序传入的fd参数的。

```
// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint(proc->tf->esp + 4 + 4*n, ip);
}

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  if(addr >= proc->sz || addr+4 > proc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

```

argfd函数会调用argint函数```argint(n, &fd)```，此时n=0，&fd是int类型的变量指针。获取到的用户程序传入的fd数值通过&fd返回。

argint函数调用fetchint函数```fetchint(proc->tf->esp + 4 + 4*n, ip);```获取fd数值。此时，```proc->tf->esp + 4 + 4*n```等于```proc->tf->esp + 4 + 0```，ip是返回值（指针）。

```proc->tf->esp + 4 + 0```是什么？```proc->tf->esp```是用户进程的用户栈顶（地址）指针。加上4，是向高地址方向移动4个位置（每个位置1字节），也就是库函数close调用第一个参数的地址。为什么是4？因为```proc->tf->esp```指向的栈顶是call指令调用向栈上push的返回地址，每个地址（指针）占用4字节。

最后，fetchint用addr所指向的变量值给ip所指向的变量赋值。此时，addr所指向的变量就是close函数的实参变量，即fd变量。

注意一下，上面的所有操作是在内核中执行的，操作的却是用户程序的栈内存。tf实体是在内核栈上的，而tf各字段的值却是用户程序在用户空间上寄存器的值。tf就是内核空间访问用户程序空间的桥梁。

![call stack](http://arvinsfj.github.io/public/ctt/documents/osxv6/call_stack.png)

注意：系统调用并不是正常的函数调用。它只会在函数调用的地方用call指令代替（其实是编译系统处理的），call先将返回地址放入栈，然后跳转到call的操作数（地址）执行。而这个地址并不是真正的C函数，而是汇编的标签。

```
#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(close)

```

可以看到，这段汇编并没有向栈中放入更多的参数或者变量。（不同于C函数，C函数的汇编实现会一开始push ebp，然后分配局部变量的栈空间）。上面的汇编，将系统调用编号放入eax寄存器，然后直接使用int指令进行64号软中断（系统调用中断），陷入内核。最后使用ret指令弹出（pop）call指令放入栈的返回地址。程序从系统调用语句的下一条指令继续执行。注意到，系统调用用户栈上除了函数参数就只有一个返回地址，没有其他东西，而此时的esp是指向返回地址（ip）的。

系统调用中，在内核空间获取用户空间的参数，方法大致如上所述。参数除了整数，还有指针和字符串，获取的具体方法虽然不同，但是原理是一样的，自己去分析。ps：再次证明了函数调用的时候并不需要立即决定传入的参数信息，只要在函数内部执行的时候能获取到参数信息即可。上一次是不定参数的函数实现，这次是系统调用。

系统调用是用户程序使用内核符服务的唯一途径。也是os功能的体现。

### 五、随便说点
----------------------------------

os层的中断处理，相对简单。设置idt表，编写中断服务程序，分发中断，编写系统调用用户空间库（C标准函数库，是可选的）。注意trapframe结构体在中断调用过程中的桥梁作用。

-----------------------------------

> END

