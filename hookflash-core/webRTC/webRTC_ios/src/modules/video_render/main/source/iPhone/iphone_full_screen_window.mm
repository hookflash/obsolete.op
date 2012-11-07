//
//  CocoaFullScreenWindow.m
//

#import "iphone_full_screen_window.h"
#include "trace.h"

using namespace webrtc;

@implementation IPhoneFullScreenWindow



-(id)init{	
	WEBRTC_TRACE(kTraceModuleCall, kTraceVideoRenderer, 0, "%s:%d", __FUNCTION__, __LINE__); 
	
	self = [super init];
	if(!self){
		WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, 0, "%s:%d COULD NOT CREATE INSTANCE", __FUNCTION__, __LINE__); 
		return nil;
	}
	
	
	WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "%s:%d Created instance", __FUNCTION__, __LINE__); 
	return self;
}

-(UIView*)window{
    return _window;
}


-(void)grabFullScreen{
}
 
-(void)releaseFullScreen
{
}

- (void) dealloc
{
	WEBRTC_TRACE(kTraceModuleCall, kTraceVideoRenderer, 0, "%s:%d", __FUNCTION__, __LINE__); 
	[self releaseFullScreen];
	[super dealloc];
}	


	
@end
