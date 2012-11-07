//
//  VideoCaptureIPhoneAVFoundationObjC.cpp
//
//


#define DEFAULT_CAPTURE_DEVICE_INDEX    1
#define DEFAULT_FRAME_RATE              30
#define DEFAULT_FRAME_WIDTH             640
#define DEFAULT_FRAME_HEIGHT            480
#define ROTATE_CAPTURED_FRAME           1
#define LOW_QUALITY                     1

#import <UIKit/UIKit.h>
#import "video_capture_avfoundation_objc.h"
#include "video_capture_avfoundation_utility.h"
#include "trace.h"
#include "vplib.h"

@implementation VideoCaptureIPhoneAVFoundationObjC

#pragma mark **** over-written OS methods

/// ***** Objective-C. Similar to C++ constructor, although must be invoked
///       manually.
/// ***** Potentially returns an instance of self
-(id)init{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);
    self = [super init];
    if(nil != self)
    {
        [self checkOSSupported];
        [self initializeVariables];
    }
    else
    {
        return nil;
    }
    return self;
}

/// ***** Objective-C. Similar to C++ destructor
/// ***** Returns nothing
- (void)dealloc {
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);
    if(_captureSession)
    {
        [_captureSession stopRunning];
        [_captureSession release];
    }
    [super dealloc];
}

#pragma mark **** public methods



/// ***** Registers the class's owner, which is where the delivered frames are
///       sent
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)registerOwner:(VideoCaptureIPhoneAVFoundation*)owner{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0, "%s:%d", __FUNCTION__, __LINE__);
    if(!owner){
        return [NSNumber numberWithInt:-1];
    }
    _owner = owner;
    return [NSNumber numberWithInt:0];
}

/// ***** Supposed to get capture device by index.
/// ***** Currently not used
- (NSNumber*)getCaptureDeviceWithIndex:(int)index ToString:(char*)name
        WithLength:(int)length{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d index=%s name=%s", __FUNCTION__, __LINE__, name);

    index = index;
    name = name;
    length = length;
    return [NSNumber numberWithInt:0];
}

/// ***** Supposed to set capture device by index.
/// ***** Currently not used
- (NSNumber*)setCaptureDeviceByIndex:(int)index {
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d name", __FUNCTION__, __LINE__);
    index = index;
    return [NSNumber numberWithInt:0];
}

/// ***** Sets the AVCaptureSession's input device from a char*
/// ***** Sets several member variables. Can signal the error system if one has
///       occurred
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)setCaptureDeviceByName:(char*)name{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d name=%s", __FUNCTION__, __LINE__, name);
    if(NO == _OSSupported)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d OS version does not support necessary APIs",
                     __FUNCTION__, __LINE__);
        return [NSNumber numberWithInt:0];
    }

    if(!_captureSession)
    {
    }

    if(!name || (0 == strcmp("", name)))
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d  \"\" was passed in for capture device name",
                     __FUNCTION__, __LINE__);
        memset(_captureDeviceNameUTF8, 0, 1024);
        return [NSNumber numberWithInt:0];
    }

    if(0 == strcmp(name, _captureDeviceNameUTF8))
    {
        // camera already set
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d Capture device is already set to %s", __FUNCTION__,
                     __LINE__, _captureDeviceNameUTF8);
        return [NSNumber numberWithInt:0];
    }
    /*
    for(int index = 0; index < _captureDeviceCount; index++)
    {
        AVCaptureDevice* captDevice;
        captDevice = (AVCaptureDevice*)[_captureDevices
                                               objectAtIndex:index];

    }
     */
    bool success = NO;
    AVCaptureDevice* tempCaptureDevice;
    for(int index = 0; index < _captureDeviceCount; index++)
    {
        tempCaptureDevice = (AVCaptureDevice*)[_captureDevices
                                               objectAtIndex:index];
        char tempCaptureDeviceName[1024] = "";
        [[tempCaptureDevice localizedName]
          getCString:tempCaptureDeviceName maxLength:1024
          encoding:NSUTF8StringEncoding];
        if(0 == strcmp(name, tempCaptureDeviceName))
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                         "%s:%d Found capture device %s as index %d",
                         __FUNCTION__, __LINE__, tempCaptureDeviceName, index);
            success = YES;
            break;
        }

    }

    if(NO == success)
    {
        // camera not found
        // nothing has been changed yet, so capture device will stay in it's
        // state
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d Capture device %s was not found in list of "
                     "available devices", __FUNCTION__, __LINE__,
                     _captureDeviceNameUTF8);
        return [NSNumber numberWithInt:0];
    }

    NSError* error;

    if(_captureVideoDeviceInput)
    {
        [_captureVideoDeviceInput release];
    }
    _captureVideoDeviceInput = [[AVCaptureDeviceInput alloc]
                                 initWithDevice:tempCaptureDevice error:&error];

    [_captureSession addInput:_captureVideoDeviceInput];

    WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                 "%s:%d successfully added capture device: %s", __FUNCTION__,
                 __LINE__, _captureDeviceNameUTF8);
    return [NSNumber numberWithInt:0];
}


/// ***** Updates the capture devices size and frequency
/// ***** Sets member variables _frame* and _captureDecompressedVideoOutput
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width
             AndFrameRate:(int)frameRate{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d height=%d width=%d frameRate=%d", __FUNCTION__,
                 __LINE__, height, width, frameRate);
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }

    _frameWidth = width;
    _frameHeight = height;
    _frameRate = frameRate;

    NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
    if ([systemVersion compare:@"5.0" options:NSNumericSearch] != NSOrderedAscending)
    {
        AVCaptureConnection* connection = [_captureDecompressedVideoOutput connectionWithMediaType:AVMediaTypeVideo];
        [connection setVideoMinFrameDuration:CMTimeMake(1, _frameRate)];
        [connection setVideoMaxFrameDuration:CMTimeMake(1, 1)];
    }
    else 
    {
        [_captureDecompressedVideoOutput setMinFrameDuration:CMTimeMake(1, _frameRate)];
    }

    NSDictionary* captureDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                       [NSNumber numberWithDouble:_frameWidth], (id)kCVPixelBufferWidthKey,
                                       [NSNumber numberWithDouble:_frameHeight], (id)kCVPixelBufferHeightKey,
#ifdef YUV_CAPTURE
                                       [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange],
#elif defined RGBA_CAPTURE
                                       [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA],
#endif
                                       (id)kCVPixelBufferPixelFormatTypeKey, nil]; 
    [_captureDecompressedVideoOutput setVideoSettings:captureDictionary];
    // these methods return type void so there isn't much we can do about
    // checking success
    return [NSNumber numberWithInt:0];
}

/// ***** Starts the AVCaptureSession, assuming correct state. Also ensures that
///       an NSRunLoop is running
/// ***** Without and NSRunLoop to process events, the OS doesn't check for a
///       new frame.
/// ***** Sets member variables _capturing
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)startCapture{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }

    if(YES == _capturing)
    {
        return [NSNumber numberWithInt:0];
    }
  
//    NSLog(@"--------------- before ---------------");
//    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate distantFuture]];
//    NSLog(@"--------------- after ---------------");

    if(NO == _captureInitialized)
    {
        // this should never be called..... it is initialized on class init
        [self initializeVideoCapture];
    }    
    [_captureSession startRunning];

    if (![_captureSession isRunning]){
	    return [NSNumber numberWithInt:-1];
    }

    _capturing = YES;

    return [NSNumber numberWithInt:0];
}

/// ***** Stops the AVCaptureSession, assuming correct state
/// ***** Sets member variables _capturing
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)stopCapture{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);

    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }

    if(nil == _captureSession)
    {
        return [NSNumber numberWithInt:0];
    }

    if(NO == _capturing)
    {
        return [NSNumber numberWithInt:0];
    }

    if(YES == _capturing)
    {
        [_captureSession stopRunning];
    }

    _capturing = NO;
    return [NSNumber numberWithInt:0];
}

// ********** "private" functions below here **********
#pragma mark **** "private" methods

/// ***** Class member variables are initialized here
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)initializeVariables{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);

    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }

    _pool = [[NSAutoreleasePool alloc]init];

    memset(_captureDeviceNameUTF8, 0, 1024);
    _counter = 0;
    _framesDelivered = 0;
    _framesRendered = 0;
    _captureDeviceCount = 0;
    _lastFramerateReportTime = TickTime::Now();
    _lastFramerateReportFramesDelivered = 0;
    _capturing = NO;
    _captureInitialized = NO;
    _frameRate = DEFAULT_FRAME_RATE;
    _frameWidth = DEFAULT_FRAME_WIDTH;
    _frameHeight = DEFAULT_FRAME_HEIGHT;
    _captureDeviceName = [[NSString alloc] initWithFormat:@""];
    _rLock = [[VideoCaptureRecursiveLock alloc] init];
    _captureSession = [[AVCaptureSession alloc] init];
    _captureDecompressedVideoOutput = [[AVCaptureVideoDataOutput alloc]
                                        init];
    
    dispatch_queue_t queue;
    queue = dispatch_queue_create("captureQueue", NULL);
    
    [_captureDecompressedVideoOutput setSampleBufferDelegate:self queue:queue];

    [self getCaptureDevices];
    [self initializeVideoCapture];

    return [NSNumber numberWithInt:0];

}

// Checks to see if the AVCaptureSession framework is available in the OS
// If it is not, isOSSupprted = NO.
// Throughout the rest of the class isOSSupprted is checked and functions
// are/aren't called depending
// The user can use weak linking to the QTKit framework and run on older
// versions of the OS. I.E. Backwards compaitibility
// Returns nothing. Sets member variable
- (void)checkOSSupported{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);

    Class osSupportedTest = NSClassFromString(@"AVCaptureSession");
    _OSSupported = NO;
    if(nil == osSupportedTest)
    {
    }
    _OSSupported = YES;
}

/// ***** Retrieves the number of capture devices currently available
/// ***** Stores them in an NSArray instance
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)getCaptureDevices{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);

    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }

    if(_captureDevices)
    {
        [_captureDevices release];
    }
    _captureDevices = [[NSArray alloc] initWithArray:
        [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]];

    _captureDeviceCount = _captureDevices.count;
    if(_captureDeviceCount < 1)
    {
        return [NSNumber numberWithInt:0];
    }
    return [NSNumber numberWithInt:0];
}

// Initializes a AVCaptureSession (member variable) to deliver frames via
// callback
// AVCapture* member variables affected
// The image format and frequency are setup here
// Returns 0 on success, -1 otherwise.
- (NSNumber*)initializeVideoCapture{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);

    if(YES == _captureInitialized)
    {
        return [NSNumber numberWithInt:-1];
    }

    [_captureDecompressedVideoOutput setVideoSettings:
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithDouble:_frameWidth], (id)kCVPixelBufferWidthKey,
            [NSNumber numberWithDouble:_frameHeight], (id)kCVPixelBufferHeightKey,
#ifdef YUV_CAPTURE
            [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange],
#elif defined RGBA_CAPTURE
            [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA],
#endif
            (id)kCVPixelBufferPixelFormatTypeKey, nil]];

    [_captureDecompressedVideoOutput setAlwaysDiscardsLateVideoFrames:YES];

    [_captureSession addOutput:_captureDecompressedVideoOutput];
    
    _captureInitialized = YES;

    return [NSNumber numberWithInt:0];
}

// This is the callback that is called when the OS has a frame to deliver to us.
// Starts being called when [_captureSession startRunning] is called. Stopped
// similarly.
// Parameter videoFrame contains the image. The format, size, and frequency
// were setup earlier.
// Returns 0 on success, -1 otherwise.
- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    if(YES != [_rLock tryLock])
    {
      NSLog(@"************************ unable to obtain lock **************************");
      return;
    }

    if(NO == _OSSupported)
    {
        [_rLock unlock];
        return;
    }
    
    if (!sampleBuffer)
    {
        [_rLock unlock];
        return;
    }
  
    _framesRendered++;
    _framesDelivered++;

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer); 

    const int LOCK_FLAGS = 0; // documentation says to pass 0

    // get size of the frame
    CVPixelBufferLockBaseAddress(imageBuffer, LOCK_FLAGS);
#ifdef YUV_CAPTURE
    int planeWidthY = CVPixelBufferGetWidthOfPlane(imageBuffer, 0);
    int planeHeightY = CVPixelBufferGetHeightOfPlane(imageBuffer, 0);
    int bytesPerRowY = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0);
    int planeHeightUV = CVPixelBufferGetHeightOfPlane(imageBuffer, 1);
    int bytesPerRowUV = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 1);
    void* baseAddress = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
#elif defined (RGBA_CAPTURE)
    int bufferWidth = CVPixelBufferGetWidth(imageBuffer);
    int bufferHeight = CVPixelBufferGetHeight(imageBuffer);
    int bufferBytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    void* baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
#endif

    if(_owner)
    {
        CVBufferRetain(imageBuffer);
        VideoCaptureCapability tempCaptureCapability;
        tempCaptureCapability.width = _frameWidth;
        tempCaptureCapability.height = _frameHeight;
        tempCaptureCapability.maxFPS = _frameRate;
#ifdef YUV_CAPTURE
        tempCaptureCapability.rawType = kVideoI420;
#elif defined RGBA_CAPTURE
        tempCaptureCapability.rawType = kVideoARGB;
#endif
        tempCaptureCapability.codecType = kVideoCodecUnknown;
      
#ifdef YUV_CAPTURE
        int frameSize = bytesPerRowY*planeHeightY+bytesPerRowUV*planeHeightUV >> 2;
        ConvertNV12ToI420AndScaleFrameDouble_LIBYUV(planeWidthY, planeHeightY, (WebRtc_UWord8*)baseAddress);
#elif defined RGBA_CAPTURE
        int frameSize = bufferBytesPerRow*bufferHeight >> 2;
        ScaleRGBAFrameDouble_LIBYUV(bufferWidth, bufferHeight, (WebRtc_UWord8*)baseAddress);
#endif
      
        _owner->IncomingFrame((unsigned char*)baseAddress,
                              frameSize,
                              tempCaptureCapability,
                              0);
    }
    
    CVPixelBufferUnlockBaseAddress(imageBuffer, LOCK_FLAGS);
    CVBufferRelease(imageBuffer);
  
//    if ((TickTime::Now() - _lastFramerateReportTime).Milliseconds() >= 1000)
//    {
//        printf("Capture frame rate: %d\n", _framesDelivered - _lastFramerateReportFramesDelivered);
//        _lastFramerateReportTime = TickTime::Now();
//        _lastFramerateReportFramesDelivered = _framesDelivered;
//    }

    [_rLock unlock];
}

@end
