
# 一个非矩形的Button（iOS）

> *作者：Arvin 日期：2017年12月14日*

---------------------------------

>BEGIN

在iOS平台如何实现一个任意形状的UIButton？外形可以使用图片代替，但是点击响应区域不能用图片代替。这里提供一个实现思路，通过图片的透明度来决定该Button是否可以点击（响应点击事件）。

### 一、思路
---------------------------------

主要思路是通过图片（图片或者背景图片）的透明度来决定该Button某个区域是否可以点击（响应点击事件）。这里需要解决两个问题：1、图片上某点的透明度如何获取；2、如何将UIButton组件上的坐标点转换成图片上的坐标点（由于UIImageView具有contentMode属性，UIButton上的点坐标不一定就是图片上的点坐标）。第一个问题可以通过CG框架，将图片转换成位图并且获取指定点的RGBA数值即可。第二个问题需要根据contentMode属性变换UIButton的点击坐标（通过构建变换矩阵和应用矩阵来实现）。

### 二、获取图片指定点的颜色（Alpha值）
---------------------------------

注意为了运行效率，并不是获取所有像素点，只获取我们关心的数据点。

```
- (UIColor *)colorAtPixel:(CGPoint)point
{
    // Cancel if point is outside image coordinates
    if (!CGRectContainsPoint(CGRectMake(0.0f, 0.0f, self.size.width, self.size.height), point)) {
        return nil;
    }
    
    // Create a 1x1 pixel byte array and bitmap context to draw the pixel into.
    // Reference: http://stackoverflow.com/questions/1042830/retrieving-a-pixel-alpha-value-for-a-uiimage
    NSInteger pointX = trunc(point.x);
    NSInteger pointY = trunc(point.y);
    //NSLog(@"%ld, %ld", pointX, pointY);//左上角为坐标原点
    CGImageRef cgImage = self.CGImage;
    NSUInteger width = self.size.width;
    NSUInteger height = self.size.height;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    int bytesPerPixel = 4;
    int bytesPerRow = bytesPerPixel * 1;
    NSUInteger bitsPerComponent = 8;
    unsigned char pixelData[4] = { 0, 0, 0, 0 };
    CGContextRef context = CGBitmapContextCreate(pixelData, 
                                                 1,
                                                 1,
                                                 bitsPerComponent, 
                                                 bytesPerRow, 
                                                 colorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);
    CGContextSetBlendMode(context, kCGBlendModeCopy);

    // Draw the pixel we are interested in onto the bitmap context
    CGContextTranslateCTM(context, -pointX, pointY-(CGFloat)height);
    CGContextDrawImage(context, CGRectMake(0.0f, 0.0f, (CGFloat)width, (CGFloat)height), cgImage);
    CGContextRelease(context);
    
    // Convert color values [0..255] to floats [0.0..1.0]
    CGFloat red   = (CGFloat)pixelData[0] / 255.0f;
    CGFloat green = (CGFloat)pixelData[1] / 255.0f;
    CGFloat blue  = (CGFloat)pixelData[2] / 255.0f;
    CGFloat alpha = (CGFloat)pixelData[3] / 255.0f;
    return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}
```

### 三、点击事件坐标到图片坐标的变换
---------------------------------

contentMode影响坐标变换矩阵。

```
-(CGAffineTransform) viewToImageTransform {
    
    UIViewContentMode contentMode = self.contentMode;
    
    // failure conditions. If any of these are met – return the identity transform
    if (!self.image || self.frame.size.width == 0 || self.frame.size.height == 0 ||
        (contentMode != UIViewContentModeScaleToFill && contentMode != UIViewContentModeScaleAspectFill && contentMode != UIViewContentModeScaleAspectFit)) {
        return CGAffineTransformIdentity;
    }
    
    // the width and height ratios
    CGFloat rWidth = self.image.size.width/self.frame.size.width;
    CGFloat rHeight = self.image.size.height/self.frame.size.height;
    
    // whether the image will be scaled according to width
    BOOL imageWiderThanView = rWidth > rHeight;
    
    if (contentMode == UIViewContentModeScaleAspectFit || contentMode == UIViewContentModeScaleAspectFill) {
        
        // The ratio to scale both the x and y axis by
        CGFloat ratio = ((imageWiderThanView && contentMode == UIViewContentModeScaleAspectFit) || (!imageWiderThanView && contentMode == UIViewContentModeScaleAspectFill)) ? rWidth:rHeight;
        
        // The x-offset of the inner rect as it gets centered
        CGFloat xOffset = (self.image.size.width-(self.frame.size.width*ratio))*0.5;
        
        // The y-offset of the inner rect as it gets centered
        CGFloat yOffset = (self.image.size.height-(self.frame.size.height*ratio))*0.5;
        
        return CGAffineTransformConcat(CGAffineTransformMakeScale(ratio, ratio), CGAffineTransformMakeTranslation(xOffset, yOffset));
    } else {
        return CGAffineTransformMakeScale(rWidth, rHeight);
    }
}

-(CGAffineTransform) imageToViewTransform {
    return CGAffineTransformInvert(self.viewToImageTransform);
}
```
矩阵倒置，就是在改变映射的方向。


### 四、UIButton点击事件处理
---------------------------------

主要是用UIKit框架中的pointInside:withEvent:方法实现。

```
- (BOOL)isAlphaVisibleAtPoint:(CGPoint)point forImage:(UIImage *)image
{
    // Correction for image scaling including contentmode
    CGPoint pt = CGPointApplyAffineTransform(point, self.imageView.viewToImageTransform);
    point = pt;
    

    UIColor *pixelColor = [image colorAtPixel:point];
    CGFloat alpha = 0.0;
    
    if ([pixelColor respondsToSelector:@selector(getRed:green:blue:alpha:)])
    {
        // available from iOS 5.0
        [pixelColor getRed:NULL green:NULL blue:NULL alpha:&alpha];
    }
    else
    {
        // for iOS < 5.0
        // In iOS 6.1 this code is not working in release mode, it works only in debug
        // CGColorGetAlpha always return 0.
        CGColorRef cgPixelColor = [pixelColor CGColor];
        alpha = CGColorGetAlpha(cgPixelColor);
    }
    return alpha >= kAlphaVisibleThreshold;
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event 
{
    // Return NO if even super returns NO (i.e., if point lies outside our bounds)
    BOOL superResult = [super pointInside:point withEvent:event];
    if (!superResult) {
        return superResult;
    }

    // Don't check again if we just queried the same point
    // (because pointInside:withEvent: gets often called multiple times)
    if (CGPointEqualToPoint(point, self.previousTouchPoint)) {
        return self.previousTouchHitTestResponse;
    } else {
        self.previousTouchPoint = point;
    }

    BOOL response = NO;
    
    if (self.buttonImage == nil && self.buttonBackground == nil) {
        response = YES;
    }
    else if (self.buttonImage != nil && self.buttonBackground == nil) {
        response = [self isAlphaVisibleAtPoint:point forImage:self.buttonImage];
    }
    else if (self.buttonImage == nil && self.buttonBackground != nil) {
        response = [self isAlphaVisibleAtPoint:point forImage:self.buttonBackground];
    }
    else {
        if ([self isAlphaVisibleAtPoint:point forImage:self.buttonImage]) {
            response = YES;
        } else {
            response = [self isAlphaVisibleAtPoint:point forImage:self.buttonBackground];
        }
    }
    
    self.previousTouchHitTestResponse = response;
    return response;
}
```

### 五、随便说点
---------------------------------

1. 优化的方向是“最小内存，最少指令周期”；
2. 通过增加新维度的限制，来增加新的功能；


>END

[代码下载](documents/OBShapedButton.zip)

