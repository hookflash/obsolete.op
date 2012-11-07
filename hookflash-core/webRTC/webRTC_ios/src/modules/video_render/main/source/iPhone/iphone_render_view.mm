// 
//  CocoaRenderView.mm
//

#import <UIKit/UIKit.h>
#import "iphone_render_view.h"
#include "trace.h"

using namespace webrtc;

@implementation IPhoneRenderView
@synthesize delegate;// = _delegate;
@synthesize viewId;
@synthesize frameWidth;
@synthesize frameHeight;
@synthesize frameBytesPerRow;

- (id)init
{
    self = [super init];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

- (void)dealloc
{
    delegate = nil;
    [super dealloc];
}

- (void) SetVideoFrameProperties:(size_t) bytesPerRow width:(size_t) width height:(size_t) height
{
    frameWidth = width;
    frameHeight = height;
    frameBytesPerRow = bytesPerRow;
}
- (void)SendLastFrame:(UIImage *)image
{
    if (viewId == 0)
        [self.delegate SendLastFrameToMainView:image :self];
    else
        [self.delegate SendLastFrameToSubView:image :self];
}

- (void) SendLastFrameRawDataToAPI:(Byte*) rawData
{
    //API
    //Call API to receive data
    
    //Uncomment to see current frame on iPad
    CGColorSpaceRef frameColorSpace = CGColorSpaceCreateDeviceRGB(); 
    CGContextRef context = CGBitmapContextCreate(rawData, frameWidth, frameHeight, 8, frameBytesPerRow, frameColorSpace, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst); 
    CGImageRef newImage = CGBitmapContextCreateImage(context);
    
    UIImage *image = [UIImage imageWithCGImage:newImage scale:1.0 orientation:UIImageOrientationRight];
    
    if (viewId == 0)
        [self.delegate SendLastFrameToMainView:image :self];
    else
        [self.delegate SendLastFrameToSubView:image :self];
    
    CGContextRelease(context); 
    CGColorSpaceRelease(frameColorSpace);
    CGImageRelease(newImage);
}
@end

