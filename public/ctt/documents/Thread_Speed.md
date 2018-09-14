# 记录一下多核并发实现
-----------------------------------------

多线程一般用在高延迟的I/O异步实现上，但它还有一个用途：实现多核并发，提高计算性能。

#### 一、基本思路

这两种情况使用多线程，都是为了缓解某种操作高延迟的问题。这里主要记录用多线程实现多核并发（Mac平台的GCD）。比如：计算 ```1+2+3+...+1000000```。使用1个CPU核心就是顺序的累加操作，共调用了999999次加法指令；使用2个核心的思路就是前500000次加法指令用1个核心执行，剩下的499999次加法指令用另外一个核心去执行，这样这2个核心可以进行真正意义上的并行计算，这样理论上耗时应该是单核心计算的一半。

#### 二、Mac平台下的GCD实现代码

```
- (long long)getTime
{
    NSTimeInterval interval = [[NSDate date] timeIntervalSince1970];
    long long totalMilliseconds = interval*1000*1000;
    return totalMilliseconds;
}

- (NSInteger)sumStart:(NSInteger)start andEnd:(NSInteger)end
{
    NSInteger sum = 0;
    for (NSInteger i = start; i <= end; ++i) {
        sum += i;
    }
    return sum;
}

- (void)oneSum
{
    long long time0 = [self getTime];
    NSLog(@"%ld", (long)[self sumStart:1 andEnd:100000000]);
    long long time1 = [self getTime];
    NSLog(@"one: %lld", time1 - time0);
}

- (void)twoSum
{
    long long time0 = [self getTime];
    dispatch_group_t group = dispatch_group_create();
    __block NSInteger sum = 0;
    dispatch_group_async(group, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        //
        sum += [self sumStart:1 andEnd:50000000];
    });
    sum += [self sumStart:50000001 andEnd:100000000];
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    NSLog(@"%ld", (long)sum);
    long long time1 = [self getTime];
    NSLog(@"two: %lld", time1 - time0);
}

```

主要看oneSum和twoSum方法。oneSum是单核执行求和计算，twoSum是两个线程分别在2个核心进行求和计算。twoSum方法相比oneSum方法多出了任务拆分和结果同步的过程。

在6s上执行的结果如下图：

![多核并发](http://arvinsfj.github.io/public/ctt/documents/thread_speed.png)


#### 三、结论

从结果上看，2核心并行计算性能差不多是单核心计算的2倍，确实是可以提高计算性能，减少响应时间的。如果计算量不是很大的情况（非计算密集型应用），单核耗时是要小于多核的（这个时候任务拆分和结果同步操作占主要因素）。注意在实际开发中灵活使用。

当然，我是在mac平台用GCD实现的多核并发计算，跨平台通用的多核并发实现可以使用openMP等第三方框架。

上面讲的都是基于CPU的多核并发计算，注意跟基于GPU的并行计算进行区分。GPU并行计算一般是针对矩阵的SIMD计算，像图片处理、3D渲染、机器学习模型训练等。

