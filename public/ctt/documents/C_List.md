
# C通用双链表

> *作者：Arvin 日期：2018年8月14日*

---------------------------------

>BEGIN

最简单的东西可能是最有用的东西，比如链表，不管是os内核还是arp协议的实现，都有用到链表（数据容器）。

### 一、通用双链表
---------------------------------

先看数据定义：

```
struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

#define LIST_HEAD(name) \
    struct list_head name = { &(name), &(name) }

```

首先是list_head结构体，定义了next和prev两个指针，指向下一个和上一个链表节点。LIST_HEAD宏，定义并初始化了一个list_head结构体变量name（链表节点），其中该节点的next和prev都指向自己。（很优雅的写法!）

表头节点是不存储数据的，并且prev是指向表尾节点的，next是指向第一个数据节点的。

这里与一个问题：结构体list_head类型，并没有定义链表的数据字段。那么链表节点数据存储在什么地方？

进一步思考：节点数据字段为什么不定义在节点结构体里面？(这个地方涉及到抽象)！

--------------------------------

 链表节点初始化方法：

```
static inline void list_init(struct list_head *head)
{
    head->prev = head->next = head;
}

```

节点head的next和prev都指向自己。

判断链表是否为空方法：

```
static inline int list_empty(struct list_head *head)
{
    return head->next == head;
}

```

表头节点head的next是否指向自己。指向自己说明链表为空（除了表头没有其他节点）。

从头部添加节点：

```
static inline void list_add(struct list_head *new, struct list_head *head)
{
    head->next->prev = new;
    new->next = head->next;
    new->prev = head;
    head->next = new;
}

```

注意我们这里是操作双链表，需要同时操作next和prev。head是头节点。

从尾部添加节点：

```
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    head->prev->next = new;
    new->prev = head->prev;
    new->next = head;
    head->prev = new;
}

```

这里的head是仍然是头节点（很奇怪是不是！这里其实是一个循环双链表）。如下图所示：

![循环双链表](http://arvinsfj.github.io/public/ctt/documents/osxv6/c_list.jpg)

注意：当链表没有数据，只有一个头节点的时候，头部插入和尾部插入节点方法，它们的实际操作情况。这个时候，初始化next和prev指向自己就起作用啦，我称作“节点自旋”。“节点自旋”，在分析的时候可以消除节点读取[指针]链长度，在操作的初始阶段具有很好的通用性，可以减少if判断。相反“节点自旋”也可以增长节点读取[指针]链长度，以获取更好的操作通用性。(注意分析，在只有一个头节点和两个节点的时候，操作应该是不同的，代码的写法应该也不同；通过这个“节点自旋”，可以将这2种情况归为1种情况，使代码的写法变成相同的)

删除节点：

```
static inline void list_del(struct list_head *elem)
{
    struct list_head *prev = elem->prev;
    struct list_head *next = elem->next;

    prev->next = next;
    next->prev = prev;
}

```

不管是添加节点或者删除节点，主要操作还是处理好链表中节点的连接关系即可。注意，我们这里的链表节点都不是自己分配内存的，是嵌入到其他结构体中形成完整链表节点的。自己不管理内存的分配和释放。

链表的遍历宏：

```
#define list_entry(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member)))

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, p, head)    \
    for (pos = (head)->next, p = pos->next; \
         pos != (head);                     \
         pos = p, p = pos->next)

```

遍历是用宏写的代码片段，插入到需要遍历链表的真实代码中。

offsetof是一个linux定义的宏，主要是计算当前member在结构体type中的偏移量（这是在编译器编译的时候计算好的，“元编程”）。

offsetof宏实现： ```(size_t)(&((type*)0)->member)``` 。这种C实现方法，解释起来有点难。请参考《C语言额外的点》那篇文章。我称之为C语言的“元编程”。

ptr为list_head结构体指针（指向list_head节点）。

那么，list_entry宏，就是根据链表节点list_head的指针ptr，获取链表整个节点的指针。注意，list_head指针是插入到其他结构体之中的，这里的其他结构体就是真实的链表节点（包含数据字段和链表的list_head字段等）。

其他三个宏，按照这个逻辑分析即可。

--------------------------------

这里回答开始的时候，提出的2个问题。

链表节点数据存储在什么地方？数据在list_head结构体指针所插入的结构体中。list_head只表示节点连接关系。这样做的好处回答了第二个问题。

节点数据字段为什么不定义在节点结构体里面？通用性。如果将数据字段放在list_head结构体中，则随着数据字段的不同，list_head结构体需要不断修改。导致list_head结构体没有通用性。（这里非常巧妙）

链表是一个通用数据（结构）容器，本质是一种数据连接关系，跟存放的数据无关。数据只跟业务相关。

我们将这里只抽象链表（连接关系），将它（这种连接关系）插入到具体的数据（结构体）节点中，就能将数据（结构体）节点连接起来，形成具体的链表。这其中涉及一个技术：从连接关系字段指针获取数据节点指针。（offsetof宏做的事情）

“高内聚，低耦合”，在C中也是适用的。


### 二、怎么使用它呢？
---------------------------------

随便举个例子：

```
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

//首先定义一个数据节点，并插入链表关系节点list_head
struct dataNode
{
    int index;
    struct list_head list;
    char* hello; 
};

static LIST_HEAD(hello_list);

int main(int argc, char** argv)
{
    //创建链表
    for (int i = 0; i < 10; ++i){
        struct dataNode *entry = malloc(sizeof(struct dataNode));
        list_init(&entry->list);
        entry->index = i;
        entry->hello = "hello, tom!";
        list_add_tail(&entry->list, &hello_list);
    }
    //遍历链表
    struct list_head *item;
    struct dataNode *data;
    list_for_each(item, &hello_list) {
        data = list_entry(item, struct dataNode, list);
        printf("%d: %s\n", data->index, data->hello);
    }
    //释放链表
    struct list_head *item_free, *tmp;
    struct dataNode *data_free;
    list_for_each_safe(item_free, tmp, &hello_list) {
        data_free = list_entry(item_free, struct dataNode, list);
        list_del(item_free);
        free(data_free);
    }
    //
    return 0;
}

```

hello_list是链表头节点。

### 三、“节点自旋”补充
---------------------------------

先看下面的图：

![循环双链表](http://arvinsfj.github.io/public/ctt/documents/osxv6/c_list.jpg)

1个节点（最下面的）和2个节点看似完全不同。2个节点跟3个节点（或者更多的节点）是相似的。

我们拿从尾部添加节点操作来说明。

---------------------------------

第一种情况，只有1个头节点的情况，尾部添加新节点的代码应该如下：

```
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    head->next = new;  //1
    new->prev = head;  //2
    new->next = head;
    head->prev = new;
}

```

第二种情况，有2个节点的情况，尾部添加新节点的代码应该如下：

```
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    head->prev->next = new;  //1
    new->prev = head->prev;  //2
    new->next = head;
    head->prev = new;
}

```

上面的代码只有第1句和第2句是不一样的。第二种情况具有通用性（超过2个节点的操作也跟2个节点的操作是一样的），第一种情况是特殊的。为什么第一种情况是特殊的？**头节点也是尾节点**。用伪代码来表达即 ```head <=> head->prev``` 。这个地方就是“尾节点自旋”，即 ```head->prev = head``` 。

同样的道理，在头部添加新节点的操作里我们可以推导出“头节点自旋”，即 ```head->next = head``` 。

最终，为了统一1个节点和大于或等于2个节点的情况，我们只需要这初始化头节点的时候让next和prev同时指向自己即可，即：

```
#define LIST_HEAD(name) struct list_head name = { &(name), &(name) }

``` 

ps：这种写法本身就很有美感。

这样就不需要对1个节点的情况进行单独处理（if判断）。记得在大学的时候，自己或者老师写的链表头节点初始化的时候都是使用NULL赋值next和prev，这样就必须使用if判断了，破坏了通用性。ps：在编程的时候，一个不合理的决定可能会需要很多的判断来弥补。

### 四、随便说点
---------------------------------

1. 双链表用途还是很大的，以后可以这样实现和使用链表；
2. 设计通用组件的时候，需要考虑“高内聚，低耦合”原则；
3. 注意C中面向类型的编程方法：“元编程”;
4. “节点自旋”可以使代码更加具有通用性；
5. 数据结构设计的好坏，很大程度影响程序的通用性和质量；

>END

[代码下载](documents/list.zip)

