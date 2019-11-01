
# FreeRTOS（版本10.2.1）的链表

> *作者：Arvin 日期：2019年11月01日*

---------------------------------

>BEGIN

好久没有写文章了，今年有点忙，不过今天还好。最近一个月研究了一下FreeRTOS，一个用于嵌入式的实时操作系统。它的核心代码在list、task和queue文件中，其中queue是可选的，不过绝大部分项目中需要用到它。我们将从list开始分析。

### 一、前言
----------------------------------

list顾名思义，列表或者链表。在FreeRTOS中，它是一个带头节点的双链表，是核心组件之一。主要有事件列表（例如：队列的发送等待列表和接受等待列表）、任务就绪态列表、任务延迟态列表（普通延迟和时间溢出延迟列表）、挂起就绪态列表、任务等待终止态列表、任务挂起态列表。我们在这里主要关注列表的主要方面，不讨论细节（比如：volatile关键字，节点check value等）。

### 二、链表与链表节点
----------------------------------

最新版的FreeRTOS中，有两种链表节点定义：完整的链表节点和链表头节点。区别在于头节点不带“所属”字段（pvOwner和pxContainer）。

```
// 完整的双链表节点
struct xLIST;
struct xLIST_ITEM
{
	listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE			
	configLIST_VOLATILE TickType_t xItemValue;			// 链表节点数值字段，大部分情况是代表任务的优先级或延迟时间（时钟周期数）
	struct xLIST_ITEM * configLIST_VOLATILE pxNext;		// 双链表下一个节点指针
	struct xLIST_ITEM * configLIST_VOLATILE pxPrevious;	 // 前一个节点指针
	void * pvOwner;										// 当前节点所代表的任务句柄（任务控制块指针），可以通过节点快速获取任务控制块（设计上比较巧妙）
	struct xLIST * configLIST_VOLATILE pxContainer;		// 当前节点所在的链表指针，快速获取节点所在的链表（在不同情况下节点会被放到不同的链表中）
	listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE			
};
typedef struct xLIST_ITEM ListItem_t;					

// 链表头节点
struct xMINI_LIST_ITEM
{
	listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE			
	configLIST_VOLATILE TickType_t xItemValue; // 头节点数值字段，一般存放最大的优先级
	struct xLIST_ITEM * configLIST_VOLATILE pxNext; // 下一个节点的指针
	struct xLIST_ITEM * configLIST_VOLATILE pxPrevious; // 前一个节点的指针
};
typedef struct xMINI_LIST_ITEM MiniListItem_t;

```

使用上，两种节点会存在同一个链表中，操作上就需要进行指针类型转换（头节点基本用于判断，而不进行写操作），头节点指针转完整节点指针。

对应到具体的FreeRTOS中，主要使用完整节点。这里有一个隐含假设：每个任务控制块有一个链表节点（实际上有两个），并且不同情况会被插入到不同的链表中或者不在任何链表中。每个链表节点代表一个任务，它在不同的链表中，表示任务处于不同的状态。（设计上够独特的！，常规思路是任务控制块中用一个字段表示任务的状态）


```

// 链表
typedef struct xLIST
{
	listFIRST_LIST_INTEGRITY_CHECK_VALUE				
	volatile UBaseType_t uxNumberOfItems; // 链表节点（不包含头节点）个数
	ListItem_t * configLIST_VOLATILE pxIndex;			// 最后一个节点的指针，当前尾节点
	MiniListItem_t xListEnd;							// 头节点。注意这里是结构体对象，而不是指针。在创建链对象的时候会自动创建链表头节点。
	listSECOND_LIST_INTEGRITY_CHECK_VALUE				
} List_t;

```

链表（列表）对象代表一个双链表，链表的数据节点需要另外创建并在合适的时机插入到链表中。在FreeRTOS中，有两种插入操作：1、按照节点值（xItemValue值）大小顺序依次插入；2、直接插入到尾节点（pxIndex指向的节点）前面。第一种插入方式一般与优先级或延迟时间顺序相关的；第二种属于无序插入（节点被一视同仁，一般属于对链表的整体操作或者对节点的随机操作）。

### 三、链表操作
-------------------------------------------

先看下面几个宏定义：

```

// 节点pvOwner字段的设置和读取
#define listSET_LIST_ITEM_OWNER( pxListItem, pxOwner )		( ( pxListItem )->pvOwner = ( void * ) ( pxOwner ) )
#define listGET_LIST_ITEM_OWNER( pxListItem )	( ( pxListItem )->pvOwner )

// 节点xItemValue字段的设置和读取
#define listSET_LIST_ITEM_VALUE( pxListItem, xValue )	( ( pxListItem )->xItemValue = ( xValue ) )
#define listGET_LIST_ITEM_VALUE( pxListItem )	( ( pxListItem )->xItemValue )

// 读取第一个有效节点的xItemValue字段的值
#define listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxList )	( ( ( pxList )->xListEnd ).pxNext->xItemValue )

// 获取第一个有效节点的指针
#define listGET_HEAD_ENTRY( pxList )	( ( ( pxList )->xListEnd ).pxNext )

// 获取当前节点的下一个节点的指针
#define listGET_NEXT( pxListItem )	( ( pxListItem )->pxNext )

// 获取头节点的指针
#define listGET_END_MARKER( pxList )	( ( ListItem_t const * ) ( &( ( pxList )->xListEnd ) ) )

// 判断链表是否为空，以链表的uxNumberOfItems字段（节点个数）为准
#define listLIST_IS_EMPTY( pxList )	( ( ( pxList )->uxNumberOfItems == ( UBaseType_t ) 0 ) ? pdTRUE : pdFALSE )

// 获取链表的有效节点个数
#define listCURRENT_LIST_LENGTH( pxList )	( ( pxList )->uxNumberOfItems )

// 获取当前pxIndex节点的下一个节点的所有者（节点所代表的任务控制块指针），当前链表是带头节点的循环双链表，需要考虑回环问题
#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )										\
{																							\
List_t * const pxConstList = ( pxList );													\
	/* Increment the index to the next item and return the item, ensuring */				\
	/* we don't return the marker used at the end of the list.  */							\
	( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;							\
	if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) )	\
	{																						\
		( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;						\
	}																						\
	( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;											\
}

// 获取第一个有效节点的所有者（节点所代表的任务控制块指针）
#define listGET_OWNER_OF_HEAD_ENTRY( pxList )  ( (&( ( pxList )->xListEnd ))->pxNext->pvOwner )

// 判断节点是否在某个链表中
#define listIS_CONTAINED_WITHIN( pxList, pxListItem ) ( ( ( pxListItem )->pxContainer == ( pxList ) ) ? ( pdTRUE ) : ( pdFALSE ) )

// 获取节点所在的链表
#define listLIST_ITEM_CONTAINER( pxListItem ) ( ( pxListItem )->pxContainer )

// 判断链表是否已经初始化，初始化过的链表的xItemValue数值为portMAX_DELAY
#define listLIST_IS_INITIALISED( pxList ) ( ( pxList )->xListEnd.xItemValue == portMAX_DELAY )

```

以上就是几个操作宏，主要还是节点字段的设置和获取以及特定判断。

下面是5个链表的函数操作。

初始化链表和初始化链表节点：

```

void vListInitialise( List_t * const pxList )
{

	pxList->pxIndex = ( ListItem_t * ) &( pxList->xListEnd );			// pxIndex指向头节点

	pxList->xListEnd.xItemValue = portMAX_DELAY; // 头节点的xItemValue设置为portMAX_DELAY，最大延迟值

	pxList->xListEnd.pxNext = ( ListItem_t * ) &( pxList->xListEnd );	// 头节点的下一个节点指针指向头节点
	pxList->xListEnd.pxPrevious = ( ListItem_t * ) &( pxList->xListEnd );// 前一个节点指针指向头节点

	pxList->uxNumberOfItems = ( UBaseType_t ) 0U;// 设置列表有效节点个数为0，头节点不计入有效节点

	listSET_LIST_INTEGRITY_CHECK_1_VALUE( pxList );
	listSET_LIST_INTEGRITY_CHECK_2_VALUE( pxList );
}

void vListInitialiseItem( ListItem_t * const pxItem )
{
	pxItem->pxContainer = NULL; // 设置节点不属于任何链表，节点一般在创建任务控制块时创建，表示任务在创建的时候不属于任何状态

	listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
	listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
}

```

链表尾部插入节点：

```

void vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem )
{
ListItem_t * const pxIndex = pxList->pxIndex; //记录当前链表尾部指针

	listTEST_LIST_INTEGRITY( pxList );
	listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );

	pxNewListItem->pxNext = pxIndex; // 被插入的节点下一个节点指针指向当前尾部节点，也就是插入到尾节点的前面
	pxNewListItem->pxPrevious = pxIndex->pxPrevious; // 被插入节点的前一个节点指针指向当前尾节点的前一个节点

	mtCOVERAGE_TEST_DELAY();

	pxIndex->pxPrevious->pxNext = pxNewListItem; // 当前尾节点的前一个节点的下一个节点指向被插入节点
	pxIndex->pxPrevious = pxNewListItem; // 当前尾节点的前一个节点指针指向被插入节点

  // 以上操作，就是将被插入节点插入到当前尾节点的前面

	pxNewListItem->pxContainer = pxList; // 设置被插入节点的所在容器（所处链表）为当前链表

	( pxList->uxNumberOfItems )++; // 当然，插入一个新节点之后当前链表有效节点个数增加一
}

```

按照节点值顺序插入节点到链表：

```

void vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem )
{
ListItem_t *pxIterator; // 记录被插入的位置
const TickType_t xValueOfInsertion = pxNewListItem->xItemValue; // 记录被插入节点的节点值

	listTEST_LIST_INTEGRITY( pxList );
	listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );

	if( xValueOfInsertion == portMAX_DELAY )
	{
		pxIterator = pxList->xListEnd.pxPrevious; // 如果插入节点节点值为portMAX_DELAY，则插入位置为头节点的前一个节点
	}
	else
	{
    // 其他情况则从链表中按照节点值大小找到合适的插入位置，比插入节点节点值大的节点位置（后面）插入。这里面隐含着portMAX_DELAY为最大节点值。
		for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd ); pxIterator->pxNext->xItemValue <= xValueOfInsertion; pxIterator = pxIterator->pxNext ) 
		{
			
		}
	}

	pxNewListItem->pxNext = pxIterator->pxNext; 
	pxNewListItem->pxNext->pxPrevious = pxNewListItem;
	pxNewListItem->pxPrevious = pxIterator;
	pxIterator->pxNext = pxNewListItem;

  // 以上操作是在插入位置后面插入新节点

	pxNewListItem->pxContainer = pxList; // 新节点的所在链表为当前链表

	( pxList->uxNumberOfItems )++; // 链表的有效节点个数增加1
}

```

从链表中删除节点：

```

UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
{
List_t * const pxList = pxItemToRemove->pxContainer; // 获取删除节点所在的链表，有点类似反射

  // 从链表中删除当前节点的链接关系
	pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious; 
	pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;

	mtCOVERAGE_TEST_DELAY();

	if( pxList->pxIndex == pxItemToRemove )
	{
		pxList->pxIndex = pxItemToRemove->pxPrevious; // 如果当前节点为pxIndex所指向的尾节点，则移动pxIndex到当前节点的前一个节点
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	pxItemToRemove->pxContainer = NULL; // 当前节点被删除则不属于任何容器（链表），也就是节点代表的任务不属于任何状态或者等待任何事件
	( pxList->uxNumberOfItems )--; // 链表删除节点，有效节点个数减少1

	return pxList->uxNumberOfItems; // 返回当前链表的有效节点个数
}

```

### 四、随便说说
-------------------------------------------

1. FreeRTOS的list是带头节点（尾节点）的循环双链表，并且节点中包含所在列表和所属任务控制块指针。
2. 这里的头节点也可以被认为是尾节点，pxIndex也可以被视为头节点。个人理解不同，本质一样。
3. volatile关键字主要防止编译器过度优化，和ISR中会使用节点字段导致的字段数值不一致问题。具体请自行翻阅资料。
4. check value等宏，是为了校验节点有没有被非法修改。因为嵌入式系统很多没有MMU硬件，系统和应用共用内存空间。
5. 这里的链表是FreeRTOS的基本核心组件，基本可以认为是任务所处的几种状态或者事件状态。这种设计可以多加思考和学习。
6. FreeRTOS被AWS收了，用于亚马逊云服务的IoT终端的操作系统（物联网操作系统）。

下一篇分析task，任务和任务调度。任务可以理解成通用操作系统的进程。任务和任务调度器是rtos的关键组件，是其存在的意义所在。队列queue是基于task实现的，task中会大量用到list。本篇是task的基础知识之一。如果你对协程或者线程或者进程比较了解，这会很有利于你理解任务和任务调度。（这些东西本质上大同小异）

> END

![FreeRTOS官网](https://www.freertos.org/a00104.html)