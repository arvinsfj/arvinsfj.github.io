# 如何制作AppStore的视频
-----------------------------------------

AppStore支持视频的上传（以前只能上传图片），大公司有自己的视频制作部门，小公司这个工作就落到开发人员头上了，作为开发人员是不会被这些琐事难道的。

#### 一、基本思路

1. 通过iOS9提供的ReplayKit框架进行iPhone6P的屏幕录制（appstore要求视频分辨率1080*1920）

2. 通过AirDrop将视频导出到mac中

3. 通过ffmpeg降低视频的帧率成30fps（appstore要求最大帧率不超过30fps，录屏导出的视频的帧率为60fps左右）

#### 二、基本步骤

1. 在app中设置开始录制点和结束点并且运行app，录屏代码如下：

```
- (void)replayStart
{
    if ([RPScreenRecorder sharedRecorder].available) {
        [[RPScreenRecorder sharedRecorder] startRecordingWithMicrophoneEnabled:YES handler:^(NSError * _Nullable error) {
            NSLog(@"%@", error);
        }];
    } else {
        NSLog(@"录制回放功能不可用");
    }
}

- (void)replayStop:(UIViewController*)vc
{
    if (vc) {
        [[RPScreenRecorder sharedRecorder] stopRecordingWithHandler:^(RPPreviewViewController * _Nullable previewViewController, NSError * _Nullable error) {
            if (error) {
                NSLog(@"%@", error);
            }
            if (previewViewController) {
                previewViewController.previewControllerDelegate = self;
                [vc presentViewController:previewViewController animated:YES completion:nil];
            }
        }];
    }
}

//delegate 
- (void)previewControllerDidFinish:(RPPreviewViewController *)previewController
{
    [previewController dismissViewControllerAnimated:YES completion:nil];
}

```

2. 录屏结束后，通过AirDrop将视频导出到mac中

3. 安装ffmpeg库，brew install ffmpeg

4. 使用命令降低帧率，ffmpeg -i ./xxx.mp4  -r 30 ./xxx1.mp4

5. ok，上传视频到appstore就行了

---------------------------------------------

[DOWNLOAD](./ScreenReplay.zip)

