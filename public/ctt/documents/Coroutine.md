
# 协程的实现（coobjc库）

> *作者：Arvin 日期：2019年03月05日*

---------------------------------

>BEGIN

之前接触过进程实现方面的知识，但对协程底层不是很清楚，最近阿里出了一个iOS上的协程库（coobjc），趁最近有时间就研究了一下，今天记录下来。

### 一、思路
---------------------------------

协程跟进程、线程的区别：首先协程是单线程的；其次协程是用户手动切换的，线程和进程都是有专门算法进行自动选择切换的。它们的共同点都是提供异步执行程序指令的途径，都需要切换指令执行路径。

如何切换程序执行的路径呢？切换程序执行路径有那些条件和方法？

切换所有寄存器，包括：SP和IP。换句话，也就是切换执行点的栈和指令地址。当然其他剩下的寄存器值也是需要切换的。对于C程序（或者说所有程序），能让cpu执行的必要条件是一个调用栈，当然寄存器在cpu中已经存在了。

我们可以用movq指令来获取和设置大部分寄存器的数值，当然作为参数的rdi寄存器（保存结构体的指针）需要最后进行切换，作为栈顶指针的sp也需要重新计算。当然也可以将sp指向参数（结构体指针）内存区域，使用popq和pushq进行寄存器数据的获取和设置。当然ip读取的是返回地址（下一条指令的地址），ip设置的也是返回地址。ip的设置采用先修改返回地址栈内存的值然后使用ret指令设置。

这里提到切换执行栈，也就是说每个协程有自己的执行栈而非共用一个执行栈。总结下来，协程可以认为是一个结构体对象，包含了一个执行栈和寄存器上下文。

这里仅提供思考的入口点，后面会做进一步分析。

### 二、函数执行的寄存器上下文切换
---------------------------------

这里指单纯的cpu寄存器值的获取和恢复，我们采用x64进行分析，其他cpu寄存器不一样但是原理是一样的。

先看寄存器值获取代码：

```
.text
.align 2
.global _coroutine_getcontext
_coroutine_getcontext:
    movq  %rax,   (%rdi)
    movq  %rbx,  8(%rdi)
    movq  %rcx, 16(%rdi)
    movq  %rdx, 24(%rdi)
    movq  %rdi, 32(%rdi)
    movq  %rsi, 40(%rdi)
    movq  %rbp, 48(%rdi)        // pre fp store
    movq  %rsp, 56(%rdi)
    addq  $8,   56(%rdi)        // sp
    movq  %r8,  64(%rdi)
    movq  %r9,  72(%rdi)
    movq  %r10, 80(%rdi)
    movq  %r11, 88(%rdi)
    movq  %r12, 96(%rdi)
    movq  %r13,104(%rdi)
    movq  %r14,112(%rdi)
    movq  %r15,120(%rdi)
    movq  (%rsp), %rsi          //  lr
    movq  %rsi, 128(%rdi)
    # skip rflags
    # skip cs
    # skip fs
    # skip gs
    xorl  %eax, %eax
    ret
```

当然是汇编了，C的话编译器会自动加上```pushq rbp```等无关的指令会造成一些麻烦，并且不够直接。当然你也可以尝试用C来写一写。

上面的代码可以看成是汇编写的函数，C的声明如下：

```
extern int coroutine_getcontext (coroutine_ucontext_t *__ucp);
```

调用该函数的时候，会使用callq指令，自动会在执行栈上添加一个返回地址（8字节）并且sp指向栈上的该返回地址。并且__ucp变量的值保存在rdi寄存器中（x64标准）。我们看一下coroutine_ucontext_t结构体的定义：

```
typedef struct coroutine_ucontext {
    uint64_t data[21];
} coroutine_ucontext_t;

struct coroutine_ucontext_re {
    struct GPRs {
        uint64_t __rax;
        uint64_t __rbx;
        uint64_t __rcx;
        uint64_t __rdx;
        uint64_t __rdi;
        uint64_t __rsi;
        uint64_t __rbp;
        uint64_t __rsp;
        uint64_t __r8;
        uint64_t __r9;
        uint64_t __r10;
        uint64_t __r11;
        uint64_t __r12;
        uint64_t __r13;
        uint64_t __r14;
        uint64_t __r15;
        uint64_t __rip;
        uint64_t __rflags;
        uint64_t __cs;
        uint64_t __fs;
        uint64_t __gs;
    } GR;
};
```

coroutine_ucontext和coroutine_ucontext_re其实是同一种数据结构的不同表现形式，对内使用coroutine_ucontext_re更加方便，对外使用coroutine_ucontext更加抽象。如果是我的话，可能使用共用体来定义该结构了。

你可以认为__ucp是一个指向coroutine_ucontext_re结构体的指针，它用作协程保存cpu寄存器值的。

rax到rbp都是正常的赋值操作，但是注意一下rsp的赋值操作

```
movq  %rsp, 56(%rdi)
addq  $8,   56(%rdi)        // sp
```

为什么需要加上8？这里是因为callq指令会自动将返回地址加载到执行栈上并且rsp自动移动指向它。加上8其实是获取到执行callq指令前的rsp值。接下来看看如何获取返回值地址的值（ip）：

```
movq  (%rsp), %rsi          //  lr
movq  %rsi, 128(%rdi)
```

很简单，当前rsp指向的就是返回地址的值。接下来是函数返回：

```
xorl  %eax, %eax
ret
```

函数的返回值是放在rax寄存器的，这里使用xorl异或操作指令设置rax为0。ret指令从栈上popq出返回地址到rip中，并且rsp向高地址移动8字节。（栈低处在高地址，出栈rsp增大）

总结一下，执行coroutine_getcontext函数会将x64的寄存器值（和返回地址）保存在__ucp结构体对象中，并且所有寄存器的值保持调用之前的状态不变。该函数可以认为是记录下一条指令及其执行上下文的函数。

----------------------------------

再看寄存器设置（恢复）代码：

```
.global _coroutine_begin
.global _coroutine_setcontext

_coroutine_begin:
_coroutine_setcontext:
    movq  56(%rdi), %rax # rax holds new stack pointer
    subq  $16, %rax
    movq  %rax, 56(%rdi)
    movq  32(%rdi), %rbx  # store new rdi on new stack
    movq  %rbx, 0(%rax)
    movq  128(%rdi), %rbx # store new rip on new stack
    movq  %rbx, 8(%rax)
    # restore all registers
    movq    0(%rdi), %rax
    movq    8(%rdi), %rbx
    movq   16(%rdi), %rcx
    movq   24(%rdi), %rdx
    # restore rdi later
    movq   40(%rdi), %rsi
    movq   48(%rdi), %rbp
    # restore rsp later
    movq   64(%rdi), %r8
    movq   72(%rdi), %r9
    movq   80(%rdi), %r10
    movq   88(%rdi), %r11
    movq   96(%rdi), %r12
    movq  104(%rdi), %r13
    movq  112(%rdi), %r14
    movq  120(%rdi), %r15
    # skip rflags
    # skip cs
    # skip fs
    # skip gs
    movq  56(%rdi), %rsp  # cut back rsp to new location
    pop    %rdi      # rdi was saved here earlier
    ret            # rip was saved here

```

C函数声明如下：

```
extern int coroutine_setcontext (coroutine_ucontext_t *__ucp);
extern int coroutine_begin (coroutine_ucontext_t *__ucp);
```

注意一下，这里coroutine_setcontext和coroutine_begin函数是同一个函数实现，这里也是汇编标签的写法（很奇妙的写法）。

```56(%rdi)```可以看成coroutine_ucontext_re结构体的```GR.__rsp```，意思是说rdi寄存器的值加上偏移量56后的内存值，也就是　```__ucp->__rsp```。注意rdi寄存器是参数变量的值（x64规范）。

```
movq  56(%rdi), %rax # rax holds new stack pointer
subq  $16, %rax
movq  %rax, 56(%rdi)
```
上面的三句话，是将```56(%rdi)```的的值减去16，也就是将要切换的新栈栈顶rsp向下移动16字节，即新栈空出2个地址的位置。并且此时的rax寄存器保存着该rsp值。


```
movq  32(%rdi), %rbx  # store new rdi on new stack
movq  %rbx, 0(%rax)
```

将```__ucp->__rdi```的值保存在rax寄存器所指向的地址，也就是保存```__ucp->__rdi```到新栈的栈顶，空出来的2个地址的第一个位置。为什么要这样做呢？因为rdi目前需要作为函数参数使用，不能立即恢复新值，需要最后才能恢复到新栈的值。先在栈上暂存起来，最后通过popq进行恢复。

```
movq  128(%rdi), %rbx # store new rip on new stack
movq  %rbx, 8(%rax)
```

将新的返回地址（rip值）保存到```8(%rax)```也就是空出来的2个地址的第二个位置。最后可以通过ret指令设置新的rip寄存器值。后面的几句汇编代码是正常的寄存器设置操作没什么好分析。看最后的三句：

```
movq  56(%rdi), %rsp  # cut back rsp to new location
pop    %rdi      # rdi was saved here earlier
ret            # rip was saved here
```

恢复新栈rsp的值（切换栈），从新栈的第一个位置恢复rdi的值，用ret指令从新栈恢复rip寄存器值。到这，栈和所有寄存器的值都切换到新的执行环境了。

注意，这里的ret指令，是从新栈返回的。callq指令是在旧栈调用的函数coroutine_setcontext，ret指令是从新栈返回的到新的执行路径。新栈的返回地址并不会覆盖旧栈的返回地址，不破环栈帧。

这里的技巧性就很强了，需要足够的C和汇编的知识。

总结一下，coroutine_setcontext函数，会用传入的__ucp结构体对象值覆盖现有cpu寄存器的值，恢复__ucp所代表的执行环境。它同时切换栈和寄存器值，但不破环老的栈数据。配合之前的get函数一起，可以切换（跳转）到任意记录过的执行点执行。这也是实现协程的基础。

这里补充一下make函数：

```
void coroutine_makecontext (coroutine_ucontext_t *ctx, IMP func, void *arg, void *stackTop)
{
    struct coroutine_ucontext_re *uctx = (struct coroutine_ucontext_re *)ctx;
    uintptr_t stackBegin = (uintptr_t)stackTop - sizeof(uintptr_t);
    uctx->GR.__rbp = stackBegin;
    uctx->GR.__rsp = stackBegin - 3 * sizeof(uintptr_t); // start sp must end withs with 0xc, to make the sp align with 16 Byte at the process entry, or will crash at objc_msgSend_uncached
    uctx->GR.__rdi = (uintptr_t)arg;
    uctx->GR.__rip = (uintptr_t)func;
}
```

它是用C写成的，作用是设置新执行环境（上下文）的栈（bp、sp）和函数入口（di、ip）。没什么好分析的。ps：上面的get和set方法是一种程序非正常跳转的通用方法，并不限于协程实现中，打破了C中所有的跳转方式（if、while、for、goto、函数调用等）。

### 三、协程的实现
---------------------------------

先看协程的结构体定义：

```
    struct coroutine_scheduler;
    typedef void (*coroutine_func)(void *);
    struct coroutine {
        coroutine_func entry;                   // Process entry.
        void *userdata;                         // Userdata.
        coroutine_func userdata_dispose;        // Userdata's dispose action.
        void *context;                          // Coroutine's Call stack data.
        void *pre_context;                      // Coroutine's source process's Call stack data.
        int status;                             // Coroutine's running status.
        uint32_t stack_size;                    // Coroutine's stack size
        void *stack_memory;                     // Coroutine's stack memory address.
        void *stack_top;                    // Coroutine's stack top address.
        struct coroutine_scheduler *scheduler;  // The pointer to the scheduler.
        int8_t   is_scheduler;                  // The coroutine is a scheduler.
        
        struct coroutine *prev;
        struct coroutine *next;
        
        void *autoreleasepage;                  // If enable autorelease, the custom autoreleasepage.
        bool is_cancelled;                      // The coroutine is cancelled
    };
    typedef struct coroutine coroutine_t;

    struct coroutine_list {
        coroutine_t *head;
        coroutine_t *tail;
    };
    typedef struct coroutine_list coroutine_list_t;
    
    struct coroutine_scheduler {
        coroutine_t         *main_coroutine;
        coroutine_t         *running_coroutine;
        coroutine_list_t     coroutine_queue;
    };
    typedef struct coroutine_scheduler coroutine_scheduler_t;
```

定义了协程函数入口entry，用户自定义数据userdata，当前环境的寄存器值context，源环境的寄存器值pre_context，协程当前状态status，协程执行栈stack_memory，协程调度器scheduler。
同时定义了前一个和后一个协程指针prev和next，可以构成一个双向链表便于调度器管理。结构体coroutine_list代表一个双向链表，coroutine_scheduler结构体是一个协程调度器，main_coroutine代表调度器协程（调度器本身也是一个协程，它可以管理链表中的其他协程），running_coroutine代表正在执行的用户协程，coroutine_list_t是一个调度器管理的用户协程双向链表。

补充一点链表的节点添加和节点删除：

```
// add routine to the queue
void scheduler_add_coroutine(coroutine_list_t *l, coroutine_t *t) {
    if(l->tail) {
        l->tail->next = t;
        t->prev = l->tail;
    } else {
        l->head = t;
        t->prev = nil;
    }
    l->tail = t;
    t->next = nil;
}

// delete routine from the queue
void scheduler_delete_coroutine(coroutine_list_t *l, coroutine_t *t) {
    if(t->prev) {
        t->prev->next = t->next;
    } else {
        l->head = t->next;
    }
    
    if(t->next) {
        t->next->prev = t->prev;
    } else {
        l->tail = t->prev;
    }
}

```

数据结构的基本知识，很容易但很重要。上面的添加节点（协程）函数scheduler_add_coroutine，是从尾部加入到链表中的，并且但只有一个节点时，head和tail都指向该节点。删除节点函数scheduler_delete_coroutine，是从任意节点删除的。

再补充一点iOS下内存申请和释放的知识：

```
void *coroutine_memory_malloc(size_t s) {
    vm_address_t address;
    
    vm_size_t size = s;
    kern_return_t ret = vm_allocate((vm_map_t)mach_task_self(), &address, size,  VM_MAKE_TAG(VM_MEMORY_STACK) | VM_FLAGS_ANYWHERE);
    if ( ret != ERR_SUCCESS ) {
        return NULL;
    }
    return (void *)address;
}

void  coroutine_memory_free(void *ptr, size_t size) {
    if (ptr) {
        vm_deallocate((vm_map_t)mach_task_self(), (vm_address_t)ptr, size);
    }
}

```

如上所示，利用专有的系统提供的api进行操作即可。这两个函数主要是用来进行协程执行栈的分配和回收的。

--------------------------------

首先看协程调度器的创建和释放：

```
//获取
coroutine_scheduler_t *coroutine_scheduler_self(void) {
    
    if (!coroutine_scheduler_key) {
        pthread_key_create(&coroutine_scheduler_key, coroutine_scheduler_free);
    }
    
    void *schedule = pthread_getspecific(coroutine_scheduler_key);
    return schedule;
}

//创建
coroutine_scheduler_t *coroutine_scheduler_self_create_if_not_exists(void) {
    
    if (!coroutine_scheduler_key) {
        pthread_key_create(&coroutine_scheduler_key, coroutine_scheduler_free);
    }
    
    void *schedule = pthread_getspecific(coroutine_scheduler_key);
    if (!schedule) {
        schedule = coroutine_scheduler_new();
        pthread_setspecific(coroutine_scheduler_key, schedule);
    }
    return schedule;
}

//创建
coroutine_scheduler_t *coroutine_scheduler_new(void) {
    
    coroutine_scheduler_t *scheduler = calloc(1, sizeof(coroutine_scheduler_t));
    coroutine_t *co = coroutine_create((void(*)(void *))coroutine_scheduler_main);
    co->stack_size = 16 * 1024; // scheduler does not need so much stack memory.
    scheduler->main_coroutine = co;
    co->scheduler = scheduler;
    co->is_scheduler = true;
    return scheduler;
}

//释放
void coroutine_scheduler_free(coroutine_scheduler_t *schedule) {
    coroutine_close_ifdead(schedule->main_coroutine);
}

```

coroutine_scheduler_key是一个全局变量，它是从线程中取回协程调度器的键值。coroutine_scheduler_free是调度器的释放函数，在所在线程结束时进行回调。

调度器创建函数是coroutine_scheduler_new函数，它为调度器分配内存，创建调度器协程，设置标志等。ps：协程调度器好像只有一个，因为coroutine_scheduler_key是一个全局变量。

调度器的释放调用的是协程的释放函数coroutine_close_ifdead，后面再说。

调度器协程管理：

```
void coroutine_scheduler_main(coroutine_t *scheduler_co) {
    
    coroutine_t *co;
    coroutine_scheduler_t *scheduler = scheduler_co->scheduler;
    for (;;) {
        
        co = scheduler->coroutine_queue.head;
        if (co == NULL) {
            // jump out. scheduler will enter idle.
            coroutine_yield(scheduler_co);
            continue;
        }
        // delete from the scheduler's queue
        scheduler_delete_coroutine(&scheduler->coroutine_queue, co);
        // set scheduler's current running coroutine.
        scheduler->running_coroutine = co;
        // resume the coroutine
        coroutine_resume_im(co);
        
        // scheduler's current running coroutine.
        scheduler->running_coroutine = nil;
        
        // if coroutine finished, free coroutine.
        if (co->status == COROUTINE_DEAD) {
            coroutine_close_ifdead(co);
        }
    }
}

```

这里整体是一个for循环，可以使用```coroutine_yield(scheduler_co);```跳出循环，如果没有用户协程并且scheduler_co是调度器协程的话。该循环的基本思路是：从协程双向链表头部取出（取出表示从链表中删除）一个协程，然后设置为正在执行的协程，然后立即切换执行环境并执行该协程，该协程执行完成或者挂起后，设置正在执行的协程为nil值（表示没有正在执行的协程），如果协程执行为完成状态则释放该协程的所占用资源。然后进入下一次循环。如果链表中没有用户协程可以执行，则调度器协程调用coroutine_yield函数进入挂起状态，并返回源执行环境，等待被唤醒（调度器协程并不会被释放）。ps：这里关键的函数是coroutine_resume_im函数，它是立即唤醒或执行用户协程的关键函数。

补充协程创建和释放函数：

```
coroutine_t *coroutine_create(coroutine_func func) {
    coroutine_t *co = calloc(1, sizeof(coroutine_t));
    co->entry = func;
    co->stack_size = STACK_SIZE;
    co->status = COROUTINE_READY;
    
    // check debugger is attached, fix queue debugging.
    co_rebind_backtrace();
    return co;
}

void coroutine_close(coroutine_t *co) {
    
    coroutine_setuserdata(co, nil, nil);
    if (co->stack_memory) {
        coroutine_memory_free(co->stack_memory, co->stack_size);
    }
    free(co->context);
    free(co->pre_context);
    free(co);
}

void coroutine_close_ifdead(coroutine_t *co) {
    
    if (co->status == COROUTINE_DEAD) {
        coroutine_close(co);
    }
}
```

都是很普通的函数操作，需要注意的是在调用coroutine_create函数的时候并不会直接分配协程的执行栈内存，而是延后到协程真正执行的时候进行分配。当前只是确定栈的大小STACK_SIZE和协程的状态COROUTINE_READY，以及协程的执行函数入口func。协程的释放函数coroutine_close是会真实释放栈内存，当前上下文，源上下文和协程自身内存。

继续补充自定义协程用户数据：

```
void coroutine_setuserdata(coroutine_t* co, void* userdata, coroutine_func ud_dispose) {
    if (co->userdata && co->userdata_dispose) {
        co->userdata_dispose(co->userdata);
    }
    co->userdata = userdata;
    co->userdata_dispose = ud_dispose;
}

void *coroutine_getuserdata(coroutine_t* co) {
    
    return co->userdata;
}

```

首先为什么协程要携带这种无关的数据？很简单，为了关联上层数据对象。比如，你在上层封装C实现的协程成一个Coroutine对象，你可以通过在该对象中定义协程结构体获取协程的各种数据，但是你如何从协程结构体中获取该对象呢？这个时候用户自定义数据userdata字段就可以用上了，你可以将该协程对象关联到该字段上。该字段更多的是程序设计上的需要。很多C库都用到了该技术。

函数自身就没什么好分析了，正常操作。

----------------------------------

上面补充了协程的创建和释放，以及协程的用户自定义数据。这里重点分析协程的yield和resume函数。

首先看resume恢复函数

```
static void coroutine_main(coroutine_t *co) {
    co->status = COROUTINE_RUNNING;
    co->entry(co);
    co->status = COROUTINE_DEAD;
    coroutine_setcontext(co->pre_context);
}

// use optnone to keep the `skip` not be optimized.
__attribute__ ((optnone))
void coroutine_resume_im(coroutine_t *co) {
    switch (co->status) {
        case COROUTINE_READY:
        {
            co->stack_memory = coroutine_memory_malloc(co->stack_size);
            co->stack_top = co->stack_memory + co->stack_size - 3 * sizeof(void *);
            // get the pre context
            co->pre_context = malloc(sizeof(coroutine_ucontext_t));
            BOOL skip = false;
            coroutine_getcontext(co->pre_context);
            if (skip) {
                // when proccess reenter(resume a coroutine), skip the remain codes, just return to pre func.
                return;
            }
            skip = true;
            
            free(co->context);
            co->context = calloc(1, sizeof(coroutine_ucontext_t));
            coroutine_makecontext(co->context, (IMP)coroutine_main, co, (void *)co->stack_top);
            // setcontext
            coroutine_begin(co->context);
            
            break;
        }
        case COROUTINE_SUSPEND:
        {
            BOOL skip = false;
            coroutine_getcontext(co->pre_context);
            if (skip) {
                // when proccess reenter(resume a coroutine), skip the remain codes, just return to pre func.
                return;
            }
            skip = true;
            // setcontext
            coroutine_setcontext(co->context);
            
            break;
        }
        default:
            assert(false);
            break;
    }
}

void coroutine_resume(coroutine_t *co) {
    if (!co->is_scheduler) {
        coroutine_scheduler_t *scheduler = coroutine_scheduler_self_create_if_not_exists();
        co->scheduler = scheduler;
        
        scheduler_add_coroutine(&scheduler->coroutine_queue, co);
        
        if (scheduler->running_coroutine) {
            // resume a sub coroutine.
            scheduler_add_coroutine (&scheduler->coroutine_queue, scheduler->running_coroutine);
            coroutine_yield(scheduler->running_coroutine);
        } else {
            // scheduler is idle
            coroutine_resume_im(co->scheduler->main_coroutine);
        }
    }
}

```

coroutine_main函数是协程执行的入口点，```co->entry(co);```之前是运行状态COROUTINE_RUNNING，之后是完成状态COROUTINE_DEAD。那么挂起状态呢？在协程执行过程中有可能调用了yield函数进入挂起状态。也就是```co->entry(co);```语句进入协程执行体之后，有可能调用了yield函数挂起了当前协程，而不会执行yield调用之后的语句，当然也不会执行```co->status = COROUTINE_DEAD;```和```coroutine_setcontext(co->pre_context);```两句。只有当该协程再次唤醒（resume恢复）之后，才继续yield调用之后的语句执行，执行完协程执行体之后就会执行状态设置语句（协程成了完成状态COROUTINE_DEAD）和切换到源环境的语句继续源环境的执行。

coroutine_resume_im函数是最重要的函数之一，它根据协程的两种状态（准备状态和挂起状态）选择性的进行执行。当将要执行的协程是准备状态COROUTINE_READY时，会去分配协程执行栈内存，并且构建源执行环境（上下文）也就是当前环境（get函数获取当前执行环境并保存在源上下文结构体中），最后分配当前上下文环境内存并设置栈入口（stack_memory）和函数执行入口（coroutine_main），使用```coroutine_begin(co->context);```语句切换到新的执行环境开始执行。也就是，调用coroutine_resume_im函数的协程（或者线程）跟将要执行的协程co的执行环境（栈和寄存器值）是不一样的。当将要执行的协程是挂起状态COROUTINE_SUSPEND时，只要记录当前环境（源执行环境）并切换到协程原来的执行环境（将要执行的协程自身的当前执行环境）即可。

注意下面的代码：

```
BOOL skip = false;
coroutine_getcontext(co->pre_context);
if (skip) {
    // when proccess reenter(resume a coroutine), skip the remain codes, just return to pre func.
    return;
}
skip = true;
// setcontext
coroutine_setcontext(co->context);
```

看似是多余的if判断，其实包含着玄机。```coroutine_getcontext(co->pre_context);```记录了```coroutine_setcontext(co->context);```执行结束（挂起或者完成）后的返回点。当从set切换环境执行中返回后此时的skip已经变成true了，也就是会执行return会从该段代码所在的函数中返回。这里体现了，get和set函数的神奇之处，代码可以穿越回去。细细理解一下，有点难。ps：在```coroutine_setcontext(co->context);```切换到新环境中执行，如果要返回则需要执行```coroutine_setcontext(co->pre_context);```，返回到源执行环境的记录点。

这里由于记录点处是直接从函数中返回，也就是只要从协程中返回（挂起或者完成）则直接从coroutine_resume_im函数返回到调用处（coroutine_resume、coroutine_add和coroutine_scheduler_main函数中）。用户协程返回永远返回到coroutine_scheduler_main函数中。调度器协程会返回到coroutine_resume或coroutine_add函数中。

coroutine_resume函数其实很简单，设置将要执行的协程的调度器为当前调度器，并将协程加入到调度双向链表（表尾）中，最后判断是否有协程正在执行，如果有则当前正在执行的协程让出cpu进入挂起状态并且调度器协程执行调度选择出执行的协程，如果没有正在执行的协程，则唤醒调度器协程进行协程调度选择将要执行的协程。

补充一下coroutine_add函数：

```
void coroutine_add(coroutine_t *co) {
    if (!co->is_scheduler) {
        coroutine_scheduler_t *scheduler = coroutine_scheduler_self_create_if_not_exists();
        co->scheduler = scheduler;
        if (scheduler->main_coroutine->status == COROUTINE_DEAD) {
            coroutine_close_ifdead(scheduler->main_coroutine);
            coroutine_t *main_co = coroutine_create(coroutine_scheduler_main);
            main_co->is_scheduler = true;
            main_co->scheduler = scheduler;
            scheduler->main_coroutine = main_co;
        }
        scheduler_add_coroutine(&scheduler->coroutine_queue, co);
        
        if (!scheduler->running_coroutine) {
            coroutine_resume_im(co->scheduler->main_coroutine);
        }
    }
}

```

跟coroutine_resume函数类似，只是不会去挂起（抢占）当前正在执行的协程。加进去的协程只会默默的等待调度器将自己唤醒并执行。（调用coroutine_resume_im函数）

--------------------------------

下面看看coroutine_yield函数：

```
__attribute__ ((optnone))
void coroutine_yield(coroutine_t *co)
{
    if (co == NULL) {
        // if null
        co = coroutine_self();
    }
    BOOL skip = false;
    coroutine_getcontext(co->context);
    if (skip) {
        return;
    }
    skip = true;
    co->status = COROUTINE_SUSPEND;
    coroutine_setcontext(co->pre_context);
}

```

```__attribute__ ((optnone))```注释是为了防止编译器自动优化，把```if (skip) {return;}```语句优化没了。

coroutine_yield函数逻辑很简单，记录当前环境执行点，设置当前协程的状态为挂起COROUTINE_SUSPEND，最后切换到源执行环境执行。该函数大部分是在协程执行体中调用的，表示挂起当前协程，主动让出cpu。当唤醒该协程的时候，会继续执行yield调用之后的协程执行体语句。

补充一下coroutine_self函数：

```
coroutine_t *coroutine_self() {
    coroutine_scheduler_t *schedule = coroutine_scheduler_self();
    if (schedule) {
        return schedule->running_coroutine;
    } else {
        return nil;
    }
}

```

返回当前正在执行的协程（用户协程或者调度器协程），或者返回nil。

总结一下，通过简单的get函数和set函数，我们可以实现复杂的resume函数和yield函数，从而实现协程。

### 四、随便说点
---------------------------------

1. 通过简单的原语函数，可以实现复杂的业务功能，怎样实现是一门学问
2. 在C里面或者底层编程中，像链表等数据结构是很常见的，如果对这些结构很熟悉，学习或者阅读C源码会事半功倍快很多
3. 协程间的通信（或者协程间同步等）如何实现呢，csp（Communicating Sequential Processes），本质就是在sender和receiver协程间加一个管道（pipe），通过复制数据来传输数据，通过管道有无数据或者满载来阻塞（yield挂起）接受者协程或者发送者协程。
4. 管道模型的实现，有时间再开新文章或者你们自己也可以去分析源码
5. 上层的封装（面向对象的封装）也可以自己去看看，不过只要底层实现原理清楚了，上层只是封装和简化使用而已

>END

[代码下载](https://github.com/alibaba/coobjc)

