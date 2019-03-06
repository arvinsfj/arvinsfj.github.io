
# CSP(Communicating Sequential Processes)分析

> *作者：Arvin 日期：2019年03月06日*

---------------------------------

>BEGIN

CSP是一种协程间通信（同步）的模型。典型的例子是Go语言，它在语言级支持协程并且使用Channel进行通信。该篇是接《协程实现分析》，主要分析剩下的协程间通信和同步所使用的技术。

### 一、数据定义
---------------------------------

先看数据定义：

```
/**
 Define the channel's op code.  send/receive.
 */
typedef enum {
    CHANNEL_SEND = 1,
    CHANNEL_RECEIVE,
} channel_op;


typedef struct chan_alt chan_alt;
typedef struct chan_queue chan_queue;
typedef struct co_channel co_channel;

/**
 Define the chan alt, record a send/receive context.
 */
struct chan_alt
{
    co_channel          *channel;
    void                *value;
    coroutine_t         *task;
    channel_op          op;
    int                 can_block;
};

/**
 Define the queue used by channel.
 */
struct chan_queue
{
    void            *arr;
    unsigned int    elemsize;
    unsigned int    size;
    unsigned int    head;
    unsigned int    tail;
    unsigned int    count;
    unsigned int    expandsize;
};

/**
 Define the channel struct
 */
struct co_channel {
    chan_queue    buffer;
    chan_queue    asend;
    chan_queue    arecv;
    pthread_mutex_t  lock;
    void (*custom_resume)(coroutine_t *co);
};

```

一个channel（管道）包括：发送者、数据缓冲区队列和接受者。当缓冲队列满载，发送者需要等待（阻塞）；当缓冲队列无数据，接受者需要等待（阻塞）。如果缓冲区有数据并且没有满载，则channel正常运行，将发送者的数据压入队列，将队列的弹出数据保存到接受者，不阻塞发送者和接受者。这里的接受者和发送者可以看成一个协程任务，这里的数据可以看成一个任务。这就是管道CSP模型：

senders ========= receivers

管道实现的基础是队列，任务缓冲区是队列，发送者和接受者的任务也可以放在队列中（多对多关系，共用同一个缓冲区）。接着我们可以考虑阻塞的实现，它其实就是协程中的yield调用，相对的唤醒发送者或者接受者操作就是resume调用。最后我们把channel传递的数据封装成一个任务，它包含了数据和操作数据的协程等。

chan_queue结构体是队列，chan_alt是任务，co_channel是管道。

ps：这里的队列是一个可扩展的通用队列实现。

### 二、队列的实现
---------------------------------

数据移动工具：

```
static void amove(void *dst, void *src, uint n) {
    if(dst){
        if(src == nil) {
            memset(dst, 0, n);
        } else {
            memmove(dst, src, n);
        }
    }
}

```

如果没有src则设置dst区域的n字节为，否则将src区域n字节的数据移动到dst区域。

队列的数据定义：

```
struct chan_queue
{
    void            *arr;
    unsigned int    elemsize;
    unsigned int    size;
    unsigned int    head;
    unsigned int    tail;
    unsigned int    count;
    unsigned int    expandsize;
};

```

队列初始化：

```
static void queueinit(chan_queue *q, int elemsize, int bufsize, int expandsize, void *buf) {
    q->elemsize = elemsize;
    q->size = bufsize;
    q->expandsize = expandsize;
    
    if (expandsize) {
        if (bufsize > 0) {
            q->arr = malloc(bufsize * elemsize);
        }
    } else {
        if (buf) {
            q->arr = buf;
        }
    }
}

```

elemsize代表队列中数据元素的内存大小（单位字节），size代表队列的大小（队列能放多少个数据元素，每个元素的大小是elemsize），expandsize是队列扩展大小（单位是元素的个数），arr是实际的缓冲区指针，有可能是初始化时分配的也有可能是外部传入的。

加入队列函数：

```
static int queuepush(chan_queue *q, void *element)
{
    if (q->count == q->size) {
        
        if (q->expandsize) {
            // expand buffer, example:
            //   ⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽
            //  |█ █ █ █ _ _ _ _ █ █ █ █ |    size=12, count=8, head=4, tail=8;
            //   ⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺
            //            ↓
            //   ⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽
            //  |█ █ █ █ _ _ _ _ _ _ _ _ █ █ █ █ |    size=16, count=8, head=4, tail=12;
            //   ⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺
            size_t oldsize = q->size;
            q->size += q->expandsize;
            q->arr = realloc(q->arr, q->size * q->elemsize);
            
            if (q->head <= q->tail) {
                void *copyaddr = q->arr + q->tail * q->elemsize;
                void *destaddr = copyaddr + q->expandsize * q->elemsize;
                size_t copysize = (oldsize - q->tail) * q->elemsize;
                memmove(destaddr, copyaddr, copysize);
                q->tail += q->expandsize;
            }
            
        } else {
            return 0;
        }
    }
    
    amove(q->arr + q->head * q->elemsize, element, q->elemsize);
    q->head = (q->head + 1) % q->size;
    q->count ++;
    return 1;
}
```

前半部分是队列缓冲区如果不够就进行扩展操作，最后3句是真实的入队列操作。元素加入队列是从头部加入的，出队列是从尾部出来的。使用amove移动数据，head向后移动，队列元素实际个数count加1。

队列缓冲区扩展自己分析。

出队列函数：

```
static int queuepop(chan_queue *q, void *val) {
    
    if (q->count > 0) {
        
        amove(val, q->arr + q->tail * q->elemsize, q->elemsize);
        q->tail = (q->tail + 1) % q->size;
        q->count--;
        return 1;
    } else {
        return 0;
    }
}
```

没什么好分析的。这里head跟tail都是自加1，并且在size内循环。右边（地址大的地方）是队列头，左边是队列尾。

### 三、Channel的实现
---------------------------------

管道数据定义如下：

```
/**
 Define the channel struct
 */
struct co_channel {
    chan_queue    buffer;
    chan_queue    asend;
    chan_queue    arecv;
    pthread_mutex_t  lock;
    void (*custom_resume)(coroutine_t *co);
};
```
定义了缓冲区队列，发送者任务队列，接受者任务队列，一个线程锁和自定义resume函数指针。

创建函数如下：

```
co_channel *chancreate(int elemsize, int bufsize, void (*custom_resume)(coroutine_t *co)) {
    
    co_channel *c;
    if (bufsize < 0) {
        c = calloc(1, sizeof(co_channel));
    } else {
        c = calloc(1, (sizeof(co_channel) + bufsize*elemsize));
    }
    
    // init buffer
    if (bufsize < 0) {
        queueinit(&c->buffer, elemsize, 16, 16, NULL);
    } else {
        queueinit(&c->buffer, elemsize, bufsize, 0, (void *)(c+1));
    }
    
    // init queue
    queueinit(&c->asend, sizeof(chan_alt), 16, 16, NULL);
    queueinit(&c->arecv, sizeof(chan_alt), 16, 16, NULL);
    
    // init lock
    c->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    
    c->custom_resume = custom_resume;

    return c;
}

```

这个函数的通用性并不强，根据传入的bufsize大小来创建不同的管道。主要是缓冲区队列的创建不同。一种是自定义缓冲区大小的，另外一种是默认缓冲区大小的。

该函数主要还是初始化三个队列，留意一下队列的初始化即可。

管道释放函数：

```
void chanfree(co_channel *c) {
    
    if(c == nil) {
        return;
    }
    if (c->buffer.expandsize) {
        free(c->buffer.arr);
    }
    pthread_mutex_destroy(&c->lock);
    free(c->arecv.arr);
    free(c->asend.arr);
    free(c);
}

```

没什么好说的，正常操作。注意跟创建函数一起看，expandsize为0的时候是用户自定义缓冲区的，不需要在该函数中释放。

补充一下加锁和释放锁的函数：

```
static void chanlock(co_channel *c) {
    pthread_mutex_lock(&c->lock);
}

static void chanunlock(co_channel *c) {
    pthread_mutex_unlock(&c->lock);
}

```

pthread的锁操作。

取反宏：

```
#define otherop(op)    (CHANNEL_SEND+CHANNEL_RECEIVE-(op))
```

获取的是op的相反类别，比如op为CHANNEL_SEND，则返回的是CHANNEL_RECEIVE。

根据操作码返回对应队列：

```
static chan_queue *chanarray(co_channel *c, uint op) {
    switch(op){
        default:
            return nil;
        case CHANNEL_SEND:
            return &c->asend;
        case CHANNEL_RECEIVE:
            return &c->arecv;
    }
}
```

ps：default可以写在第一位置的。

任务定义如下：

```
/**
 Define the chan alt, record a send/receive context.
 */
struct chan_alt
{
    co_channel          *channel;
    void                *value;
    coroutine_t         *task;
    channel_op          op;
    int                 can_block;
};
```

所在管道channel，任务数据value，任务所在协程task（操作数据的函数），任务操作类型op，任务是否能阻塞can_block。

任务加入正确队列函数：

```
static void altqueue(chan_alt *a) {
    chan_queue *altqueue = chanarray(a->channel, a->op);
    queuepush(altqueue, a);
}
```

先根据op从所在channel中返回对应任务队列，然后将任务加入该队列中。

任务是否能执行函数：

```
static int altcanexec(chan_alt *a) {
    chan_queue *altqueue;
    co_channel *c;
    
    c = a->channel;
    if(c->buffer.size == 0){
        altqueue = chanarray(c, otherop(a->op));
        return altqueue && altqueue->count;
    } else if (c->buffer.expandsize) {
        // expandable buffer
        switch(a->op){
            default:
                return 0;
            case CHANNEL_SEND:
                // send always success.
                return 1;
            case CHANNEL_RECEIVE:
                return c->buffer.count > 0;
        }
    } else{
        switch(a->op){
            default:
                return 0;
            case CHANNEL_SEND:
                return c->buffer.count < c->buffer.size;
            case CHANNEL_RECEIVE:
                return c->buffer.count > 0;
        }
    }
}
```

任务能否执行表达的是当前任务是否需要阻塞，任务协程是否需要调用yield函数。

size等于0是特殊的管道，只用于阻塞任务协程，当前任务能执行的条件是管道的另外一端有等待（阻塞）的任务。

size大于0且expandsize大于0，则是默认缓冲区大小的可无限扩展管道（size=16并且expandsize=16），因为缓冲区是可以扩展的所以CHANNEL_SEND操作任何时候是可以执行的，CHANNEL_RECEIVE操作只要队列里面存在数据元素则可以执行，不需要阻塞。

size大于0且expandsize等于0，则是自定义缓冲区大小的不可扩展的管道（size=16且expandsize=0），这个时候CHANNEL_SEND操作任务当缓冲区有空位置的时候可以执行，CHANNEL_RECEIVE操作任务缓冲区有数据元素时能执行。属于标准操作。

任务数据传输函数：

```
static void altcopy(chan_alt *s, chan_alt *r) {
    chan_alt *t;
    co_channel *c;
    
    /*
     * Work out who is sender and who is receiver
     */
    if(s == nil && r == nil) {
        return;
    }
    assert(s != nil);
    c = s->channel;
    if(s->op == CHANNEL_RECEIVE){
        t = s;
        s = r;
        r = t;
    }
    assert(s==nil || s->op == CHANNEL_SEND);
    assert(r==nil || r->op == CHANNEL_RECEIVE);
    
    /*
     * Channel is empty (or unbuffered) - copy directly.
     */
    if(s && r && c->buffer.count == 0){
        amove(r->value, s->value, c->buffer.elemsize);
        return;
    }
    
    /*
     * Otherwise it's always okay to receive and then send.
     */
    if(r){
        queuepop(&c->buffer, r->value);
    }
    if(s){
        queuepush(&c->buffer, s->value);
    }
}

```

需要注意的是这里面会自动将s设置成发送者任务，r设置成接受者任务。当缓冲区为空是，直接从s移动数据到r。如果缓冲区不空，则r从缓冲区获取数据，s向缓冲区加数据。

任务执行函数：

```
static void altexec(chan_alt *a) {

    chan_queue *altqueue;
    chan_alt other_alt;
    chan_alt *other = &other_alt;
    co_channel *c;
    
    c = a->channel;
    altqueue = chanarray(c, otherop(a->op));
    if(altqueue && altqueue->count){

        queuepop(altqueue, other);
        altcopy(a, other);
        coroutine_t *co = other->task;
        void (*custom_resume)(coroutine_t *co) = c->custom_resume;
        chanunlock(c);
        
        if (custom_resume) {
            custom_resume(co);
        } else {
            coroutine_add(co);
        }
        
    } else {
        altcopy(a, nil);
        chanunlock(c);
    }
}

```

当前任务执行（altcanexec函数判断可以执行），主要是移动数据到另外一端任务中并且通知管道另外一端等待（阻塞）的任务协程去执行（唤醒另外一端等待阻塞的任务协程）。这是协程使用管道的核心逻辑（协程同步机制原理）。

获取管道阻塞任务的数量：

```
int changetblocking(co_channel *c, int *sendBlockingCount, int *receiveBlockingCount) {
    int send = 0, recv = 0;
    chanlock(c);
    
    chan_queue *ar = chanarray(c, CHANNEL_SEND);
    if (ar && ar->count) {
        
        send = ar->count;
        if (sendBlockingCount) {
            *sendBlockingCount = send;
        }
    }
    
    chan_queue *receiveAr = chanarray(c, CHANNEL_RECEIVE);
    if (receiveAr && receiveAr->count) {
        recv = receiveAr->count;
        if (receiveBlockingCount) {
            *receiveBlockingCount = recv;
        }
    }

    chanunlock(c);
    return send > 0 || recv > 0;
}

```

没什么好说的。

管道消息传递函数：

```
int chanalt(chan_alt *a) {
    
    int canblock = a->can_block;
    co_channel *c;
    coroutine_t *t = coroutine_self();
        
    a->task = t;
    c = a->channel;
    
    chanlock(c);
    
    if(altcanexec(a)) {
        altexec(a);
        return 0;
    }
    
    if(!canblock) {
        chanunlock(c);
        return -1;
    }
    
    // add to queue
    altqueue(a);
    
    chanunlock(c);
    
    // blocking.
    coroutine_yield(t);
    
    return 0;
}

```

如果任务能执行，则执行后返回。如果不能执行且不阻塞为真则直接返回。其他情况（当前不能执行并且阻塞为真）则把任务加入阻塞队列并调用任务协程的yield函数挂起任务协程，最后返回。

任务组装函数：

```
static int _chanop(co_channel *c, int op, void *p, int canblock) {
    chan_alt a;
    
    a.channel = c;
    a.op = op;
    a.value = p;
    a.op = op;
    a.can_block = canblock;
    
    if(chanalt(&a) < 0) {
        return -1;
    }
    return 1;
}

```

通过参数构建一个任务，并调用消息传递函数。

以上就是Channel的实现。后面都是接口封装，这里就不讲了。


### 四、随便说点
---------------------------------

1. CSP模型理解一下，有什么用处
2. 相对的Actor模型也可以去了解一下
3. 基于Channel和Coroutine就可以做很多事情了，比如：await、生成器
4. 阻塞是怎么实现的，什么时候被唤醒，协程同步是怎么实现的
5. C原语接口的任何设计，名称、参数、返回值如何设计，需要遵守什么原则

--------------------------------

coobjc到这核心就分析完了，上层的封装不太关注，也不打算分析了，自己看懂就好。这里面的队列和Channel是通用的，用到的地方不限于coobjc协程，自己留心一下。

>END

[代码下载](https://github.com/alibaba/coobjc)

