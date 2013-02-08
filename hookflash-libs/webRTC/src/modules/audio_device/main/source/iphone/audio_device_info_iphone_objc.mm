#import "audio_device_info_iphone_objc.h"
#import <UIKit/UIKit.h>

@implementation AudioDeviceInfoIPhoneObjC

- (NSString*)getOSVersion
{
    NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
    return systemVersion;
}

@end
