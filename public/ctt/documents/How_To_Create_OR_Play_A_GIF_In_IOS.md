
# 如何从视频中获取生成GIF和播放GIF(iOS)

> *作者：Arvin 日期：2017年10月17日*

---------------------------------

>BEGIN

通过**如何从视频中获取图片(iOS)**我们可以知道如何从视频中提取图片，多张图片可以合并成一个GIF文件。iOS中播放GIF就是提取多张图片使用UIImage动画进行播放。

### 一、思路
---------------------------------

生成GIF，首先准备视频图片提取时间点数组，其次通过上篇的方法提取多张图片，然后多每张图片进行处理，最后添加到CGImageDestinationRef容器中进行GIF合并处理。这里对图片不进行处理了。
播放GIF，通过CGImageSource获取每张图片，并计算总的时长，将每张图片放进UIImage中并设置动画时间。这里涉及图片的获取和时间的获取和累加。

### 二、代码
---------------------------------

```
NSURL *url = [NSURL URLWithString:@"videofilepath"];
AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
float videoLength = (float)asset.duration.value/asset.duration.timescale;
float increment = (float)videoLength/100;
NSMutableArray *timePoints = [NSMutableArray array];
for (int currentFrame = 0; currentFrame&lt;100; ++currentFrame) {
  float seconds = (float)increment * currentFrame;
  CMTime time = CMTimeMakeWithSeconds(seconds, [timeInterval intValue]);
  [timePoints addObject:[NSValue valueWithCMTime:time]];
}
```

```
NSURL *gifurl = [NSURL fileURLWithPath:@"giffilepath"];
CGImageDestinationRef destination = CGImageDestinationCreateWithURL((__bridge CFURLRef)gifurl, kUTTypeGIF , 100, NULL);
AVAssetImageGenerator *generator = [AVAssetImageGenerator assetImageGeneratorWithAsset:asset];
for (NSValue *time in timePoints) {
  CGImageRef imageRef;
  imageRef = [generator copyCGImageAtTime:[time CMTimeValue] actualTime:nil error:nil];
  CGImageDestinationAddImage(destination, imageRef, (CFDictionaryRef)@{(NSString *)kCGImagePropertyGIFDictionary:@{(NSString *)kCGImagePropertyGIFDelayTime: @(0.125)},
             (NSString *)kCGImagePropertyColorModel:(NSString *)kCGImagePropertyColorModelRGB});
  CGImageRelease(imageRef);
}
CGImageDestinationSetProperties(destination, (CFDictionaryRef)@{(NSString *)kCGImagePropertyGIFDictionary:@{(NSString *)kCGImagePropertyGIFLoopCount: @(loopCount)}});
CGImageDestinationFinalize(destination)
CFRelease(destination);

```

```
sd_frameDurationAtIndex:index source:source方法如下：
float frameDuration = 0.1f;
NSData *data = [NSData dataWithContentsOfFile:gifPath];
CGImageSourceRef source = CGImageSourceCreateWithData((__bridge CFDataRef)data, NULL);
CFDictionaryRef cfFrameProperties = CGImageSourceCopyPropertiesAtIndex(source, index, nil);
NSDictionary *frameProperties = (__bridge NSDictionary *)cfFrameProperties;
NSDictionary *gifProperties = frameProperties[(NSString *)kCGImagePropertyGIFDictionary];
NSNumber *delayTimeUnclampedProp = gifProperties[(NSString *)kCGImagePropertyGIFUnclampedDelayTime];
if (delayTimeUnclampedProp) {
  frameDuration = [delayTimeUnclampedProp floatValue];
} else {
  NSNumber *delayTimeProp = gifProperties[(NSString *)kCGImagePropertyGIFDelayTime];
  if (delayTimeProp) {
    frameDuration = [delayTimeProp floatValue];
   }
}
CFRelease(cfFrameProperties);
```

```
size_t count = CGImageSourceGetCount(source);
NSTimeInterval duration = 0.0f;
NSMutableArray *images = [NSMutableArray array];
for (size_t i = 0; i < count; i++) {
  CGImageRef image = CGImageSourceCreateImageAtIndex(source, i, NULL);
  if (!image) {
    continue;
  }
  duration += [self sd_frameDurationAtIndex:i source:source];
  [images addObject:[UIImage imageWithCGImage:image]];
  CGImageRelease(image);
}
UIImage *animatedImage = [UIImage animatedImageWithImages:images duration:duration];
```

### 三、在意的事情
---------------------------------

1. CoreGraphics框架提供了大量对单张图片处理的方法（本质是绘制）
2. ImageIO框架提供了图片集的读取和合并处理方法
3. AVFoundation框架提供了对多媒体数据（音视频数据）的处理方法，其中AVAsset实现了多媒体资源的抽象
4. iOS官方的多媒体框架大多基于生产者消费者模式架构（输入-处理会话-输出）

>END
---------------------------------
[DOWNLOAD](documents/TestGIF.zip)
