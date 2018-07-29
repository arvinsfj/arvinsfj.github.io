
# xv6操作系统终端（console）

> *作者：Arvin 日期：2018年7月29日*

---------------------------------

>BEGIN

终端最开始操作计算机的物理设备，后面慢慢演化成一个程序。在xv6中，把console抽象成了内核设备。

### 一、前言
----------------------------------

xv6中终端通过中断可以接收来自键盘和COM0串口的数据并显示到VGA显示器上，其他程序还可以从终端缓存中（input）读取字符，当然其他程序还可以向终端写入字符。本篇讲console，下一篇讲串口（UART）。

```
// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.
```

### 二、数据定义
----------------------------------

数据结构定义如下：

```
static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

#define INPUT_BUF 128
struct {
  struct spinlock lock;
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

```

panicked全局静态变量，记录了某个cpu是否奔溃了（或者说标记内核是否奔溃了）。

cons结构体变量，代表一个终端。其中包含了一个终端自旋锁（lock）和一个是否需要使用该自旋锁的标记变量（locking）。console被抽象成设备，也是计算机的一种资源（显示器等），需要同步使用。

BACKSPACE宏定义了VGA显示器的回退功能的命令。

CRTPORT定义了VGA显示器的操作端口地址（读写光标位置）。

crt静态变量定义了VGA显存的入口地址（0xb8000），需要使用虚拟地址，所以用P2V宏转换了一下地址。

INPUT_BUF定义了，终端输入缓存大小。键盘输入的字符或者串口输入的字符都会被缓存终端缓存中，以备后面使用（程序从终端读取数据，就是从该缓存中读取的）。

input结构体变量，定义了终端字符缓存（上面所讲的）。该缓存资源需要同步使用，带了一个lock锁（比如，在写的时候，不能读）。r、w、e分别定义了缓存当前读取、写入和编辑的位置索引。

```#define C(x)  ((x)-'@')``` 如注释所讲，获取 ```Control-x```的数值。根据该数值来判断是否是组合键。

### 三、函数分析
----------------------------------

初始化函数如下：

```
void
consoleinit(void)
{
  initlock(&cons.lock, "console");
  initlock(&input.lock, "input");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  picenable(IRQ_KBD);
  ioapicenable(IRQ_KBD, 0);
}

```

分别初始化两个自旋锁lock（一个用于终端写，一个用于终端（缓存）读）。

devsw是一个定义成主设备编号到设备读写功能的映射数组。CONSOLE（定义成1）是终端的主设备编号。这两句，相当于初始化这个映射关系。

```cons.locking = 1;```开启终端自旋锁（lock），使锁有效。

最后两句```picenable(IRQ_KBD); ioapicenable(IRQ_KBD, 0);```是打开键盘的中断掩码（pic和ioapic同时设置键盘中断掩码位0，使中断有效）。这里体现了，pic可以跟ioapic的协同工作，pic在前ioapic在后（pic的信号会转发给ioapic），不明白的地方回头看看关于中断的三篇文章。

终端写函数：

```
int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

```

ip参数我没暂不考虑，作用是：获取到终端锁之后，将buf数组的字符写入到VGA显示器（consputc函数），最后是否锁和返回写入（显示）的字符个数。关键是consputc函数。

终端读取函数：

```
int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&input.lock);
  while(n > 0){
    while(input.r == input.w){
      if(proc->killed){
        release(&input.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &input.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&input.lock);
  ilock(ip);

  return target - n;
}

```

ip暂不考虑。target记录需要（想要的）读取的字符个数n。获取input缓存的自旋锁，当input的读取和写入的位置相等（即没有字符能读取）时当前进程进入等待（有数据可读）。注意这个时候还没有真正的进程（这种情况会panic的，或者说终端读取函数是需要存在进程之后调用的）。

``` c = input.buf[input.r++ % INPUT_BUF];```从input缓存读取当前字符（input.r位置），然后r索引自加1。注意使用求余%操作是为了回滚。（毕竟是缓存可以重复利用缓存区域，而且可以防止数组越界）。

如果读取到的字符是C('D')也就是```Control-D```组合键字符，代表EOF。退出字符读取循环，返回已经读取的字符数量。否则，将读取的字符c写入目标地址dst内存区域，如果读取的字符是'\n'退出读取循环。如果n等于0，代表已经读取了n个字符，同样页要退出读取循环。

最后，释放input的自旋锁，返回读取的字符数量（这个实际读取到的字符数量是小于或者等于需要读取的字符数量）。

跟终端写函数不一样（操作VGA显示器），终端读函数是操作input缓存的。这里使用读取进程的sleep函数，来实现数据读取的等待。这样的C标准库函数很多：getc、scanf等等。

终端中断处理函数：

```
void
consoleintr(int (*getc)(void))
{
  int c;

  acquire(&input.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      procdump();
      break;
    case C('U'):  // Kill line.
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if(input.e != input.w){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    default:
      if(c != 0 && input.e-input.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c);
        if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&input.lock);
}

```

注意参数是一个函数指针getc。这里很有程序设计技巧，抽象了实际的字符读取方法成getc，因为实际的读取字符操作，可能是键盘读取字符，也可能是串口（UART）读取字符。不同的中断，实现不同的getc函数，在终端的中断处理中，只负责调用传入的getc函数得到相应的字符，不管具体的实现。

循环读取字符c，直到c小于0。因为要将字符写入input缓存，故循环之前先要获取到input的自旋锁。首先处理三个控制字符```Control-P/U/H```，分别是进程列表显示、删除当前行和回退。进程列表显示主要为了debug用，调用procdump函数实现。删除当前行和回退，主要是操作编辑索引e递减和VGA显示光标回退。注意结束条件。

如果是正常字符并且缓冲区有空闲区域，则将字符c写入缓存区，索引e自加1，VGA显示该字符。如果字符c代表写入（完成）结束，则索引e赋值给索引w。换句话来讲，e代表某（缓存区编辑）操作进行中的临时索引，w是完成操作后的缓存区状态索引，r也是缓存区的读取状态索引。最后唤醒（wakeup）等待在缓存区读取操作的进程（中断处理是向缓存区进行写入操作的，其他进程是从缓存区进行读取操作的，比如调用了getc库函数的进程）。

最后，释放input的自旋锁完成终端中断处理。

终端字符（VGA）显示函数：

```
void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else
    uartputc(c);
  cgaputc(c);
}

```

如果内核已经崩溃，则关闭CPU中断并且进如死循环。

uartputc是向串口（COM0）写入（发送）字符的函数。这里暂不考虑串口的问题（下一篇会分析），这里只要知道终端字符显示函数会调用串口的发送字符函数的。也就是说，终端VGA显示的字符，也同样会通过串口发送到连接串口（COM0）的设备的。

cgaputc函数，是VGA显示器字符写入（显示）函数。将字符c写入VGA的显存，也就是在VGA屏幕上显示字符c。（注意：这里的函数名称是cga，CGA也是一种IBM显示卡标准，不知道为什么是cga而不是vgaputc）

VGA显示函数：

```
static void
cgaputc(int c)
{
  int pos;
  
  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } else
    crt[pos++] = (c&0xff) | 0x0700;  // black on white
  
  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }
  
  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = ' ' | 0x0700;
}

```

这个函数相当于VGA的驱动函数。控制端口是CRTPORT，数据端口是CRTPORT+1。最上面的4句，是读取显示器光标的位置（2字节，16位），分2次进行高低位读取。

处理控制字符'\n'和回退操作，进行光标位置的重新计算（后面会把新的光标位置写入VGA的数据端口）。

如果是正常的字符，则写入VGA的显存的pos位置。显存的起始地址是crt，也就是物理地址0xb8000。注意这里将整个显存当作一个数组来处理（需要看清本质）。VGA显存中每个字符占用2字节，低地址字节代表字符ascii码，高地址字节负责字符显示的控制，比如：0x0700代表白底黑字（black on white）。

最后处理一下，显示的向上滚动（屏幕最多显示24*80个字符）。超出24行，则需要进行向上滚动操作。将1-24行（0行是起始行，程序员习惯）的显存整体向上移动1行。pos光标位置减去80（1行的字符数量），然后将最后一行（第23行），从光标位置开始向后全部用字符0填充。

```
outb(CRTPORT, 14);
outb(CRTPORT+1, pos>>8);
outb(CRTPORT, 15);
outb(CRTPORT+1, pos);

```

将处理之后的当前光标位置写入VGA的数据端口，让VGA显示卡记录当前光标的位置，当然也让光标位置显示正确。

```
crt[pos] = ' ' | 0x0700;
```

光标显示空格字符' '，白底黑字。

VGA字符显示程序，向上滚动操作稍微复杂一点，其他的都很简单，主要需要有硬件编程的基本概念。（VGA默认是字符显示模式，还有图形显示模式的，有兴趣的自己去研究，GUI就是基于图形显示模式的）

到这，终端（console）处理方法都讲完了。后面补充其他的函数。

--------------------------------------

整数数值显示函数：

```
static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}

```

基本方法：将数值的各个位上的数值用对应的字符替换，然后显示字符即可。

注意，负数负号的显示和16进制的数字显示。上面的函数，最多显示16位的数值，因为数组buf只定义了最多16个字符。

完整的cprintf格式化显示函数：

```
// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

```

从这里我们大致知道格式化输出和不定参数（个数）函数的实现原理：函数第一个确定的参数包含了函数整个参数的个数信息（不管是明确说明或者隐含说明）。根据参数个数信息和获取到栈上参数的地址，就可以读取其他参数的数值。其实还是有点巧妙的。本质是函数的参数传入本身就是没有限制的，只是函数内参数读取的时候需要考虑参数个数和类型，否则会错误的操作参数。换句话来讲，只要函数内部知道参数的个数和类型，就可以正常操作参数了（本质上并不需要在函数定义的时候就决定参数的个数和类型）。

```argp = (uint*)(void*)(&fmt + 1);```是根据第一个参数fmt的地址获取其他参数的地址的。C语言的函数参数（或者局部变量）是存放在栈上的（寄存器SP标记的内存区域）。而且C语言定义了函数参数入栈的顺序（函数参数列表从右向左以次入栈，也就是第一个参数是最后入栈的），并且栈是向低地址方向增长的（后入栈的数据存放在低地址上）。因此，```&fmt + 1```得到的是参数fmt后面的参数的栈上地址。

后面没什么好讲的，读取一个字符解释一个字符，调用显示函数进行对应的显示操作即可。最终都会转到consputc函数显示字符的。数值会进行字符转换。:] 这个过程有点像编程语言的解释器。

------------------------------------

内核崩溃函数：

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

关闭CPU中断，显示崩溃的CPU和奔溃信息。显示pcs信息（内核崩溃前内核函数调用的栈上ip执行路径信息，函数调用栈帧ip信息）。getcallerpcs函数就是拿到栈上的ip执行路径信息的（最多获取10层ip信息）。最后标记内核已经奔溃了```panicked = 1;```,让其他CPU知道，然后进入死循环。

getcallerpcs函数：

```
// Record the current call stack in pcs[] by following the %ebp chain.
void
getcallerpcs(void *v, uint pcs[])
{
  uint *ebp;
  int i;
  
  ebp = (uint*)v - 2;
  for(i = 0; i < 10; i++){
    if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
      break;
    pcs[i] = ebp[1];     // saved %eip
    ebp = (uint*)ebp[0]; // saved %ebp
  }
  for(; i < 10; i++)
    pcs[i] = 0;
}

```

有些知识，自己从来没见过是想不出来的（ps：有时候的顿悟，会想到的），比如上面的函数做的事情。

C语言栈上的函数调用帧，当前帧是会记录上一帧的BP寄存器的值的（BP是函数调用帧的开始位置的栈上地址）。仔细一想，这不就构成了一个栈帧单链表吗:)。链表的头部是当前栈帧，它会指向调用当前函数的函数栈帧（上一个函数的栈帧）。我们遍历该单链表，就可以得到函数的调用顺序（backtrace）。当然，每个栈帧是包含函数调用结束的返回地址的（ip）。

上面的函数就是这个原理，最多得到10层的各个函数调用的返回地址，记录在pcs中返回给调用函数使用。

```ebp = (uint*)v - 2;```是什么？

拿panic函数来讲，panic定义成```panic(char *s)```，它调用getcallerpcs函数```getcallerpcs(&s, pcs);```，第一个参数是传入的panic函数参数s指针变量的栈上地址（指针），为了避免陷入"指针的指针"这种混乱的思维方式，我们想象s就是一个正常变量（正常思维就是这样的，某书不知道为了什么非要让读者去思考"指针的指针"这种混乱的概念），这里传给getcallerpcs函数的是s所在内存的地址，s作为函数参数是放在栈（内存）上的。那么上一句的v就是变量s在panic函数栈帧中的内存地址（栈帧不仅仅只包含s变量的，还有其他局部变量的）。注意：在32位机器上，地址占用4字节，等价于一个uint类型。

```(uint*)v - 2```，就是v所指向的内存位置向低地址方向，移动2个位置，每个位置4字节（一个内存地址）。如果你知道C函数调用栈帧内存布局就知道，这个位置就是panic函数栈帧中上一个函数栈帧bp所在的位置（地址）。v代表变量s的地址，bp+1是panic函数的返回地址所在位置（地址），ebp[1]（也就是```*(bp+1)```）是获取当前函数（panic）调用栈帧（panic函数）中的返回地址，ebp[0]获取当前函数（panic）调用栈帧中包含的上一个函数调用栈帧的起始位置（上一个函数调用栈帧基地址：bp地址）。

函数调用栈内存布局：

![call stack](http://arvinsfj.github.io/public/ctt/documents/osxv6/call_stack.png)

### 四、随便说点
----------------------------------

本篇讲的知识不少：终端被抽象成内核设备，可以进行读写操作，还处理了中断；知道了VGA显示器显示数据（字符和数值）的原理；从终端读取字符数据的原理（实际是从input缓存区域读取数据）；知道了"抽象函数"的实现和使用场景（getc函数指针作为参数，抽象并不是OOP的专利）；知道了C不定参数函数的实现细节和本质；最后对奔溃回溯（backtrace）的实现原理进行了详细分析，知道了函数调用栈帧的内存布局和看到了所谓的"栈帧单链表"。

这些知识，如果你已经知道或者是自己领悟到的说明你在编程领域是above-average的，未来是可以期许的。国内书籍或者大学课堂是不会讲这些的。如果你是在编程领域待了5年以上并且已经很努力查阅资料和理解这些文字了，但还是看不懂这些，或许你可以考虑其他方向了，这样可能对你更好。一己之见吧!)。

-----------------------------------

> END

