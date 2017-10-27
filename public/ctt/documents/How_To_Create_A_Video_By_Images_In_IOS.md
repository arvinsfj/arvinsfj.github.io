
# 如何利用连续的图片生成视频(iOS)

> *作者：Arvin 日期：2017年10月27日*

---------------------------------

>BEGIN

确定视频的FPS，然后在时序上排列图片，当然其中需要编码(H264)压缩。

### 一、思路
---------------------------------

利用AVAssetWriter对象，合成视频。其输出为一个文件路径，输入是AVAssetWriterInput对象（可以有多个），AVAssetWriterInput对象的输入是CMSampleBuffer，我们需要使用AVAssetWriterInputPixelBufferAdaptor适配一下输入，将UIImage转成CVPixelBuffer即可。时序使用CMTime表示。

### 二、代码
---------------------------------

```
NSURL *url = [NSURL fileURLWithPath:@"videofilepath"];
NSError *error = nil;
AVAssetWriter *videoWriter = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeMPEG4 error:&error];
```

```
NSDictionary *videoSettings = @{AVVideoCodecKey: AVVideoCodecH264,
                                    AVVideoWidthKey: [NSNumber numberWithInt:size.width],
                                    AVVideoHeightKey: [NSNumber numberWithInt:size.height]};
    
AVAssetWriterInput* writerInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];

AVAssetWriterInputPixelBufferAdaptor *adaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:writerInput sourcePixelBufferAttributes:nil];

```

```
[videoWriter addInput:writerInput];  
[videoWriter startWriting];
[videoWriter startSessionAtSourceTime:kCMTimeZero];
```

```
CVPixelBufferRef buffer;
CVPixelBufferPoolCreatePixelBuffer(NULL, adaptor.pixelBufferPool, &buffer);
CMTime presentTime = CMTimeMake(0, 10);
```

```
int i = 0;
while (1)
{
if(writerInput.readyForMoreMediaData){
presentTime = CMTimeMake(i, fps);
if (i >= [array count]) {
buffer = NULL;
} else {
buffer = [HJImagesToVideo pixelBufferFromCGImage:[array[i] CGImage] size:CGSizeMake(480, 320)];
}
if (buffer) {
BOOL appendSuccess = [HJImagesToVideo appendToAdapter:adaptor pixelBuffer:buffer atTime:presentTime withInput:writerInput];
i++;
} else {
[writerInput markAsFinished];
[videoWriter finishWritingWithCompletionHandler:^{}];
CVPixelBufferPoolRelease(adaptor.pixelBufferPool);
break;
}
}}
```

```
+ (CVPixelBufferRef)pixelBufferFromCGImage:(CGImageRef)image
                                      size:(CGSize)imageSize
{
NSDictionary *options = @{(id)kCVPixelBufferCGImageCompatibilityKey: @YES,
                              (id)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES};
CVPixelBufferRef pxbuffer = NULL;
CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, imageSize.width,
                                          imageSize.height, kCVPixelFormatType_32ARGB, (__bridge CFDictionaryRef) options,
                                          &pxbuffer); 
CVPixelBufferLockBaseAddress(pxbuffer, 0);
void *pxdata = CVPixelBufferGetBaseAddress(pxbuffer);
CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
CGContextRef context = CGBitmapContextCreate(pxdata, imageSize.width,
                                                 imageSize.height, 8, 4*imageSize.width, rgbColorSpace,
                                                 kCGImageAlphaNoneSkipFirst); 
CGContextDrawImage(context, CGRectMake(0 + (imageSize.width-CGImageGetWidth(image))/2,
                                           (imageSize.height-CGImageGetHeight(image))/2,
                                           CGImageGetWidth(image),
                                           CGImageGetHeight(image)), image);
CGColorSpaceRelease(rgbColorSpace);
CGContextRelease(context);
CVPixelBufferUnlockBaseAddress(pxbuffer, 0);
return pxbuffer;
}
```

```
+ (BOOL)appendToAdapter:(AVAssetWriterInputPixelBufferAdaptor*)adaptor
            pixelBuffer:(CVPixelBufferRef)buffer
                 atTime:(CMTime)presentTime
              withInput:(AVAssetWriterInput*)writerInput
{
while (!writerInput.readyForMoreMediaData) {
usleep(1);
}
return [adaptor appendPixelBuffer:buffer withPresentationTime:presentTime];
}
```

### 三、在意的事情
---------------------------------

1. AVAssetWriter提供了多媒体数据的复杂写入操作（合并）
2. AVAssetWriter可以有多个输入
3. 视频可以看成是图片在时序上的有序排列
4. 多媒体的读入也有对应的类AVAssetReader，其可以有多个输出，并且支持Track操作

>END
---------------------------------
[DOWNLOAD](./HJImagesToVideo.zip)