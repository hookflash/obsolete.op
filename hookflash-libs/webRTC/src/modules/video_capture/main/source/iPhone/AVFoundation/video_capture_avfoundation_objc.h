 /*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//
//  video_capture_avfoundation_objc.h
//
//

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_IPHONE_AVFOUNDATION_VIDEO_CAPTURE_AVFOUNDATION_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_IPHONE_AVFOUNDATION_VIDEO_CAPTURE_AVFOUNDATION_OBJC_H_

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <UiKit/UiKit.h>
#import <CoreData/CoreData.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreVideo/CoreVideo.h>



#import "video_capture_recursive_lock.h"


#include "video_capture_avfoundation.h"

#ifndef BACK_CAMERA_INDEX
#define BACK_CAMERA_INDEX               0
#endif
#ifndef FRONT_CAMERA_INDEX
#define FRONT_CAMERA_INDEX              1
#endif

@interface VideoCaptureIPhoneAVFoundationObjC : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>{
    // class properties
    bool                                   _capturing;
    int                                    _counter;
    int                                    _frameRate;
    int                                    _frameWidth;
    int                                    _frameHeight;
    int                                    _captureWidth;
    int                                    _captureHeight;
    bool                                   _faceDetection;
    int                                    _framesDelivered;
    int                                    _framesRendered;
    bool                                   _OSSupported;
    bool                                   _captureInitialized;
    webrtc::TickTime                       _lastFramerateReportTime;
    int                                    _lastFramerateReportFramesDelivered;
    
    // WebRTC Custom classes
    webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundation*                _owner;
    VideoCaptureRecursiveLock*            _rLock;
    
    // AVFoundation variables
    AVCaptureSession*                    _captureSession;
    AVCaptureDeviceInput*                _captureVideoDeviceInput;
    AVCaptureVideoDataOutput*    _captureDecompressedVideoOutput;
    NSArray*                            _captureDevices;
    int                                    _captureDeviceCount;
    int                                    _captureDeviceIndex;
    NSString*                            _captureDeviceName;
    char                                _captureDeviceNameUTF8[1024];
    char                                _captureDeviceNameUniqueID[1024];
    char                                _captureDeviceNameProductID[1024];
    NSString*                            _key;
    NSNumber*                            _val;
    NSDictionary*                        _videoSettings;
    NSString*                            _captureQuality;
    NSString*                            _machineName;
    
    // other
    NSAutoreleasePool*                    _pool;
    NSInteger                             _index;
}
/**************************************************************************
 *
 *   The following functions are considered to be private.
 *
 ***************************************************************************/

- (NSNumber*)getCaptureDevices;
- (NSNumber*)initializeVideoCapture;
- (NSNumber*)initializeVariables;
- (void)checkOSSupported;



/**************************************************************************
 *
 *   The following functions are considered public and to be called by the VideoCaptureIPhoneAVFoundation class.
 *
 ***************************************************************************/


- (NSNumber*)getCaptureDeviceWithIndex:(int)index ToString:(char*)name WithLength:(int)length;
- (NSNumber*)registerOwner:(webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundation*)owner;
- (NSInteger)getCaptureDeviceIndex;
- (NSNumber*)setCaptureDeviceByIndex:(int)index;
- (NSNumber*)setCaptureDeviceByName:(char*)name;
- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width AndFrameRate:(int)frameRate AndFaceDetection:(bool)faceDetection;
- (NSNumber*)startCapture;
- (NSNumber*)stopCapture;

@end

#endif  // WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_IPHONE_AVFOUNDATION_VIDEO_CAPTURE_AVFOUNDATION_OBJC_H_
