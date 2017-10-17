
# 如何从视频中获取图片(iOS)

> *作者：Arvin 日期：2017年10月17日*

---------------------------------

>BEGIN

视频可以看作是图片在时间上的连续序列（当然我们不考虑音频部分）。原始视频非常大，需要压缩（帧间压缩和帧内压缩）后进行存储和传输（否则就是浪费资源和时间）。压缩编码后的视频需要解码还原成原始视频数据才能进行播放。播放当然是图片帧了，可以认为是一张一张的图片。换句话讲，我们只需要解码视频（编码压缩后的数据）数据就能得到视频图片。

### 一、思路
---------------------------------

通过iOS的AVURLAsset类加载视频资源，然后使用asset对象初始化AVAssetImageGenerator对象，并调用copyCGImageAtTime:actualTime:error:方法获取CGImageRef对象（也就是图片了！）。
AVURLAsset作为多媒体资源对象抽象，可以获取资源的各种信息，比如视频时长、视频大小等。
AVAssetImageGenerator是获取缩略图和预览图的工具。参数time（CMTime），是图片帧在视频中时间上所在的预期请求位置。actualTime是实际时间位置。

当然你可以在播放视频的时候进行屏幕截图，呵呵！原理差不多，上面的方法不过是所有工作全部在内存中进行。

### 二、代码
---------------------------------

```
NSURL *url = [NSURL URLWithString:@"videofilepath"];
AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
//float videoLength = (float)asset.duration.value/asset.duration.timescale;//获取视频时长
AVAssetImageGenerator *generator = [AVAssetImageGenerator assetImageGeneratorWithAsset:asset];
CGImageRef imageRef = [generator copyCGImageAtTime:CMTimeMakeWithSeconds(50, 600) actualTime:nil error:nil];
UIImage *image=[UIImage imageWithCGImage:imageRef];

```

### 三、作用
---------------------------------

1. 生成缩略图和预览图（单张图片）
2. 将多张图片合成GIF、WEBP或APNG等格式的文件（多张图片）
3. 将多张图片通过slam算法生成物体的3D模型