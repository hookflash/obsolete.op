// 
//  CocoaRenderer.mm
//  testCocoaCommandLine
//#import <Cocoa/Cocoa.h>
//#import <AppKit/AppKit.h>
#import "uiview_renderer.h"

@implementation UIViewRenderer
@synthesize screen = _screen;
- (void)initUIViewRenderer{
    UIWindow *window = [[UIApplication sharedApplication] keyWindow];
	
	self = [[UIImageView alloc] initWithFrame:[window bounds]];
	[window addSubview:self];

}
@end

