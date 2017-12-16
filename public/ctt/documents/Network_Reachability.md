
# 网络是否可达服务使用（iOS网络状态监测）

> *作者：Arvin 日期：2017年12月15日*

---------------------------------

>BEGIN

网络可达检测在移动端具有很大意义。iOS中有很多第三方的库可以判断网络是否可用。本篇也是阅读AFN库对iOS的SystemConfiguration框架中Reachability功能的理解。重要的不是功能的实现，而是对Apple底层服务提供方式的理解和使用，可以扩展对Apple其他底层服务提供方式的理解。

### 一、思路
---------------------------------

SCNetworkReachabilityRef是一个句柄（代表一个对象，该对象可以是一个类对象、一块内存、一种服务等，而句柄本身则是一个指针、对象编号等），其代表网络是否可达的服务实体，本身是结构体指针。SCNetworkReachabilityContext是一个结构体，代表SCNetworkReachabilityRef的服务环境（上下文），其中包含了SCNetworkReachabilityRef实体的版本，用户自定义数据、retain函数、release函数和描述。SCNetworkReachabilityFlags代表返回的网络状态标识位，不同的标志代表不同的网络状态。SCNetworkReachabilityCallBack是一个函数指针，提供使用者定义网络状态改变的回调函数模板，它回调时返回网络状态实体句柄、网络状态标志和用户自定义数据（在定义实体环境的时候用户定义的）。到这大致的思路就有了：首先创建网络状态实体句柄，然后定义实体的上下文（最重要的是自定义数据的定义），其次设置实体的环境（上下文）和网络状态改变时的回调函数，最后将实体加入到RunLoop中进行实时监测（也就是订阅网络状态的系统消息），如果有需要还可以定义回调的队列。

### 二、SCNetworkReachability.h文件的解析
---------------------------------

网络状态实体句柄的定义
```
typedef const struct CF_BRIDGED_TYPE(id) __SCNetworkReachability * SCNetworkReachabilityRef;
```

实体上下文的结构体定义
```
typedef struct {
	CFIndex		version;
	void *		__nullable info;
	const void	* __nonnull (* __nullable retain)(const void *info);
	void		(* __nullable release)(const void *info);
	CFStringRef	__nonnull (* __nullable copyDescription)(const void *info);
} SCNetworkReachabilityContext;
```

网络状态标志的定义
```
typedef CF_OPTIONS(uint32_t, SCNetworkReachabilityFlags) {
	kSCNetworkReachabilityFlagsTransientConnection	= 1<<0,
	kSCNetworkReachabilityFlagsReachable		= 1<<1,
	kSCNetworkReachabilityFlagsConnectionRequired	= 1<<2,
	kSCNetworkReachabilityFlagsConnectionOnTraffic	= 1<<3,
	kSCNetworkReachabilityFlagsInterventionRequired	= 1<<4,
	kSCNetworkReachabilityFlagsConnectionOnDemand	= 1<<5,	// __OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_3_0)
	kSCNetworkReachabilityFlagsIsLocalAddress	= 1<<16,
	kSCNetworkReachabilityFlagsIsDirect		= 1<<17,
#if	TARGET_OS_IPHONE
	kSCNetworkReachabilityFlagsIsWWAN		= 1<<18,
#endif	// TARGET_OS_IPHONE

	kSCNetworkReachabilityFlagsConnectionAutomatic	= kSCNetworkReachabilityFlagsConnectionOnTraffic
};
```

网络状态改变时的回调函数模板定义
```
typedef void (*SCNetworkReachabilityCallBack)	(
						SCNetworkReachabilityRef			target,
						SCNetworkReachabilityFlags			flags,
						void			     *	__nullable	info
						);
```

数据定义结束。下面是各种实体（句柄）创建和操作函数的定义。
通过ip地址创建实体（句柄）函数
```
SCNetworkReachabilityRef __nullable
SCNetworkReachabilityCreateWithAddress		(
						CFAllocatorRef			__nullable	allocator,
						const struct sockaddr				*address
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
						
SCNetworkReachabilityRef __nullable
SCNetworkReachabilityCreateWithAddressPair	(
						CFAllocatorRef			__nullable	allocator,
						const struct sockaddr		* __nullable	localAddress,
						const struct sockaddr		* __nullable	remoteAddress
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

通过域名创建实体函数定义
```
SCNetworkReachabilityRef __nullable
SCNetworkReachabilityCreateWithName		(
						CFAllocatorRef			__nullable	allocator,
						const char					*nodename
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

通过实体（句柄）获取当前网络状态标志的函数
```
Boolean
SCNetworkReachabilityGetFlags			(
						SCNetworkReachabilityRef	target,
						SCNetworkReachabilityFlags	*flags
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

设置实体的回调函数和上下文函数（核心函数）
```
Boolean
SCNetworkReachabilitySetCallback		(
						SCNetworkReachabilityRef			target,
						SCNetworkReachabilityCallBack	__nullable	callout,
						SCNetworkReachabilityContext	* __nullable	context
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

订阅系统事件（消息）的函数
```
Boolean
SCNetworkReachabilityScheduleWithRunLoop	(
						SCNetworkReachabilityRef	target,
						CFRunLoopRef			runLoop,
						CFStringRef			runLoopMode
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

取消订阅的函数
```
Boolean
SCNetworkReachabilityUnscheduleFromRunLoop	(
						SCNetworkReachabilityRef	target,
						CFRunLoopRef			runLoop,
						CFStringRef			runLoopMode
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

设置回调函数队列的函数
```
Boolean
SCNetworkReachabilitySetDispatchQueue		(
						SCNetworkReachabilityRef			target,
						dispatch_queue_t		__nullable	queue
						)				__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0);
```

获取类型函数（暂时不知道是什么和用途）
```
CFTypeID
SCNetworkReachabilityGetTypeID			(void)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```


### 三、具体的理解
---------------------------------

首先要明确，网络是否可达这种服务是iOS（Mac osx）系统向上层提供的服务，已经存在于iOS系统中。SCNetworkReachability.h文件只是定义了使用这个服务的必要的接口数据结构和操作函数。要是用这个服务，首先要告诉iOS需要创建这个服务实体（句柄），然后定义服务的回调函数以便通知应用层当前的网络状态信息，最后向iOS系统注册（订阅）这个服务，开始监测网络状态。句柄代表一个服务实体，这个实体是操作系统创建的不会开放给应用层，而应用层要设置和操作这个实体，则必须通过系统提供的带句柄参数的函数进行操作（调用）。一般会在在实体环境中设置自定义数据，以便应用层辨别不同的实体。

这个服务最核心的函数是：
```
Boolean
SCNetworkReachabilitySetCallback		(
						SCNetworkReachabilityRef			target,
						SCNetworkReachabilityCallBack	__nullable	callout,
						SCNetworkReachabilityContext	* __nullable	context
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

给实体设置回调函数和实体环境。接下来只要将该实体注册到系统的RunLoop中即可实时监测网络状态。
```
Boolean
SCNetworkReachabilityScheduleWithRunLoop	(
						SCNetworkReachabilityRef	target,
						CFRunLoopRef			runLoop,
						CFStringRef			runLoopMode
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
```

当然还有实体创建函数和实体环境结构体构建等步骤。
```
SCNetworkReachabilityRef __nullable
SCNetworkReachabilityCreateWithAddress		(
						CFAllocatorRef			__nullable	allocator,
						const struct sockaddr				*address
						)				__OSX_AVAILABLE_STARTING(__MAC_10_3,__IPHONE_2_0);
						
构建实体环境结构体
SCNetworkReachabilityContext ctx = {0, (__bridge void *)(callback), NKReachabilityRetainCallback, NKReachabilityReleaseCallback, NULL};
```

基本使用的代码：
```
SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr *)address);
SCNetworkReachabilityContext ctx = {0, (__bridge void *)(callback), NKReachabilityRetainCallback, NKReachabilityReleaseCallback, NULL};
SCNetworkReachabilitySetCallback(self.networkReachability, NKReachabilityCallback, &ctx);
SCNetworkReachabilityScheduleWithRunLoop(self.networkReachability, CFRunLoopGetMain(), kCFRunLoopCommonModes);

取消服务
SCNetworkReachabilityUnscheduleFromRunLoop(self.networkReachability, CFRunLoopGetMain(), kCFRunLoopCommonModes);

当然还可以通过调用服务接口手动获取网络状态标志
SCNetworkReachabilityGetFlags(self.networkReachability, &flags)
```

### 四、随便说点
---------------------------------

1. 这里提供了一种系统功能（服务）应用层使用的接口定义方法。句柄和API是其中的关键。
2. 同时这里也说明了Apple这种服务提供接口的使用方法。
3. 有些实体有实体的上下文（环境），其中提供了用户自定义数据定义方法。这种设计可以学习和参考，让程序更加灵活。
4. block（函数指针或者匿名函数或闭包）提供了一种在面向对象和面向过程语言通信的方法。我们只要关注函数。本质来讲，类最后都是函数（机器指令）。block恰巧是这种本质的体现。
5. 阅读和使用API，按照使用逻辑来理解，会事半功倍。
6. 上面的网络状态检测创建，在ip地址（0.0.0.0）创建时会返回一次状态，在域名创建是会返回2次（请求连接一次，完成后断开链接一次）。



>END

[代码下载](documents/TestNetworkReachability.zip)

