#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_INFO_IPHONE_OBJC_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_INFO_IPHONE_OBJC_H

#import <Foundation/Foundation.h>

#include "audio_device_utility.h"

@interface AudioDeviceInfoIPhoneObjC : NSObject {
  
}

- (NSString*)getOSVersion;

@end

#endif  // WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_INFO_IPHONE_OBJC_H
