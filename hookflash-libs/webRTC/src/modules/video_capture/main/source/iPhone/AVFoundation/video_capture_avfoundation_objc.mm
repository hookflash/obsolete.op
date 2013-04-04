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
#define YUV_CAPTURE
//#define RGBA_CAPTURE

#import <UIKit/UIKit.h>
#import "video_capture_avfoundation_objc.h"
#include "video_capture_avfoundation_utility.h"
#include "trace.h"
#include "libyuv.h"
#include <sys/sysctl.h>

@interface VideoCaptureIPhoneAVFoundationObjC()

@property (nonatomic, retain) CIDetector *faceDetector;

- (NSNumber *) exifOrientation: (UIDeviceOrientation) orientation;
- (BOOL) doFaceDetectionFromSampleBuffer:(CMSampleBufferRef) sampleBuffer;

@end

@implementation VideoCaptureIPhoneAVFoundationObjC
@synthesize faceDetector = _faceDetector;
#pragma mark **** over-written OS methods

/// ***** Objective-C. Similar to C++ constructor, although must be invoked
///       manually.
/// ***** Potentially returns an instance of self
-(id)init{
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
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
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);
    if(_captureSession)
    {
        [_captureSession stopRunning];
        [_captureSession release];
    }
    
    if (_faceDetector)
        [_faceDetector release];
    
    [super dealloc];
}

#pragma mark **** public methods



/// ***** Registers the class's owner, which is where the delivered frames are
///       sent
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)registerOwner:(webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundation*)owner{
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0, "%s:%d", __FUNCTION__, __LINE__);
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
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d index=%s name=%s", __FUNCTION__, __LINE__, name);

    return [NSNumber numberWithInt:0];
}

/// ***** Get selected capture device index.
- (NSInteger)getCaptureDeviceIndex {
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d name", __FUNCTION__, __LINE__);
    return _index;
}

/// ***** Supposed to set capture device by index.
/// ***** Currently not used
- (NSNumber*)setCaptureDeviceByIndex:(int)index {
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d name", __FUNCTION__, __LINE__);
    return [NSNumber numberWithInt:0];
}

/// ***** Sets the AVCaptureSession's input device from a char*
/// ***** Sets several member variables. Can signal the error system if one has
///       occurred
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)setCaptureDeviceByName:(char*)name{
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d name=%s", __FUNCTION__, __LINE__, name);
    if(NO == _OSSupported)
    {
        webrtc::Trace::Add(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, 0,
                     "%s:%d OS version does not support necessary APIs",
                     __FUNCTION__, __LINE__);
        return [NSNumber numberWithInt:0];
    }

    if(!name || (0 == strcmp("", name)))
    {
        webrtc::Trace::Add(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, 0,
                     "%s:%d  \"\" was passed in for capture device name",
                     __FUNCTION__, __LINE__);
        memset(_captureDeviceNameUTF8, 0, 1024);
        return [NSNumber numberWithInt:0];
    }

    if(0 == strcmp(name, _captureDeviceNameUTF8))
    {
        // camera already set
        webrtc::Trace::Add(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, 0,
                     "%s:%d Capture device is already set to %s", __FUNCTION__,
                     __LINE__, _captureDeviceNameUTF8);
        return [NSNumber numberWithInt:0];
    }

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
            webrtc::Trace::Add(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, 0,
                         "%s:%d Found capture device %s as index %d",
                         __FUNCTION__, __LINE__, tempCaptureDeviceName, index);
            success = YES;
            _index = index;
            break;
        }

    }

    if(NO == success)
    {
        // camera not found
        // nothing has been changed yet, so capture device will stay in it's
        // state
        webrtc::Trace::Add(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, 0,
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

    webrtc::Trace::Add(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, 0,
                 "%s:%d successfully added capture device: %s", __FUNCTION__,
                 __LINE__, _captureDeviceNameUTF8);
    return [NSNumber numberWithInt:0];
}


/// ***** Updates the capture devices size and frequency
/// ***** Sets member variables _frame* and _captureDecompressedVideoOutput
/// ***** Returns 0 on success, -1 otherwise.
- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width
                 AndFrameRate:(int)frameRate AndFaceDetection:(bool)faceDetection {
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d height=%d width=%d frameRate=%d", __FUNCTION__,
                 __LINE__, height, width, frameRate);
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }

    _frameWidth = width;
    _frameHeight = height;
    _frameRate = frameRate;
    _faceDetection = faceDetection;

    NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
    if ([systemVersion compare:@"5.0" options:NSNumericSearch] != NSOrderedAscending)
    {
        AVCaptureConnection* connection = [_captureDecompressedVideoOutput connectionWithMediaType:AVMediaTypeVideo];
        [connection setVideoMinFrameDuration:CMTimeMake(1, _frameRate)];
        //[connection setVideoMaxFrameDuration:CMTimeMake(1, _frameRate)];
        [connection setVideoMaxFrameDuration:CMTimeMake(1, 1)];
    }
    else 
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [_captureDecompressedVideoOutput setMinFrameDuration:CMTimeMake(1, _frameRate)];
#pragma clang diagnostic pop
    }
  
    if (_index == FRONT_CAMERA_INDEX)
    {
        if ([_machineName hasPrefix:@"iPod"])
        {
            _captureWidth = 640;
            _captureHeight = 480;
        }
        else if ([_machineName hasPrefix:@"iPhone5"])
        {
            _captureWidth = 1280;
            _captureHeight = 720;
        }
        else if ([_machineName hasPrefix:@"iPhone"])
        {
            _captureWidth = 640;
            _captureHeight = 480;
        }
        else if ([_machineName hasPrefix:@"iPad"])
        {
            _captureWidth = 640;
            _captureHeight = 480;
        }
    }
    else if (_index == BACK_CAMERA_INDEX)
    {
        if ([_machineName hasPrefix:@"iPod"])
        {
            _captureWidth = 1280;
            _captureHeight = 720;
        }
        else if ([_machineName hasPrefix:@"iPhone5"])
        {
            _captureWidth = 1920;
            _captureHeight = 1080;
        }
        else if ([_machineName hasPrefix:@"iPhone"])
        {
            _captureWidth = 1280;
            _captureHeight = 720;
        }
        else if ([_machineName hasPrefix:@"iPad3"])
        {
            _captureWidth = 1920;
            _captureHeight = 1080;
        }
        else if ([_machineName hasPrefix:@"iPad"])
        {
            _captureWidth = 1280;
            _captureHeight = 720;
        }
    }

    NSDictionary* captureDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
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
    
    NSDictionary *detectorOptions = [[NSDictionary alloc] initWithObjectsAndKeys:CIDetectorAccuracyLow, CIDetectorAccuracy, nil];
    self.faceDetector = [CIDetector detectorOfType:CIDetectorTypeFace context:nil options:detectorOptions];
    
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
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
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
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
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
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
    _lastFramerateReportTime = webrtc::TickTime::Now();
    _lastFramerateReportFramesDelivered = 0;
    _capturing = NO;
    _captureInitialized = NO;
    _frameRate = DEFAULT_FRAME_RATE;
    _frameWidth = DEFAULT_FRAME_WIDTH;
    _frameHeight = DEFAULT_FRAME_HEIGHT;
    _captureWidth = DEFAULT_FRAME_WIDTH;
    _captureHeight = DEFAULT_FRAME_HEIGHT;
    _faceDetection = false;
    _captureDeviceName = [[NSString alloc] initWithFormat:@""];
    _rLock = [[VideoCaptureRecursiveLock alloc] init];
    _captureSession = [[AVCaptureSession alloc] init];
    _captureDecompressedVideoOutput = [[AVCaptureVideoDataOutput alloc]
                                        init];
    _index = -1;
    
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
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
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
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
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
    webrtc::Trace::Add(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);

    if(YES == _captureInitialized)
    {
        return [NSNumber numberWithInt:-1];
    }
  
    int name[] = {CTL_HW, HW_MACHINE};
    size_t size;
    if (sysctl(name, 2, NULL, &size, NULL, 0) == 0)
    {
        char* machine = (char*)malloc(size);
        if (sysctl(name, 2, machine, &size, NULL, 0) == 0)
        {
            _machineName = [[NSString alloc] initWithCString:machine encoding: NSUTF8StringEncoding];
      
            webrtc::Trace::Add(webrtc::kTraceStateInfo, webrtc::kTraceUtility, -1,
                         "Machine name: %s", machine);
        }
        else 
        {
            _machineName = NULL;
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                         "Failed to get machine name size");
        }
        free(machine);
    }
    else 
    {
        _machineName = NULL;
        webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                     "Failed to get machine name size");
    }

    [_captureDecompressedVideoOutput setVideoSettings:
        [NSDictionary dictionaryWithObjectsAndKeys:
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

- (NSNumber *) exifOrientation: (UIDeviceOrientation) orientation
{
	int exifOrientation;
    /* kCGImagePropertyOrientation values
     The intended display orientation of the image. If present, this key is a CFNumber value with the same value as defined
     by the TIFF and EXIF specifications -- see enumeration of integer constants.
     The value specified where the origin (0,0) of the image is located. If not present, a value of 1 is assumed.
     
     used when calling featuresInImage: options: The value for this key is an integer NSNumber from 1..8 as found in kCGImagePropertyOrientation.
     If present, the detection will be done based on that orientation but the coordinates in the returned features will still be based on those of the image. */
    
	enum {
		PHOTOS_EXIF_0ROW_TOP_0COL_LEFT			= 1, //   1  =  0th row is at the top, and 0th column is on the left (THE DEFAULT).
		PHOTOS_EXIF_0ROW_TOP_0COL_RIGHT			= 2, //   2  =  0th row is at the top, and 0th column is on the right.
		PHOTOS_EXIF_0ROW_BOTTOM_0COL_RIGHT      = 3, //   3  =  0th row is at the bottom, and 0th column is on the right.
		PHOTOS_EXIF_0ROW_BOTTOM_0COL_LEFT       = 4, //   4  =  0th row is at the bottom, and 0th column is on the left.
		PHOTOS_EXIF_0ROW_LEFT_0COL_TOP          = 5, //   5  =  0th row is on the left, and 0th column is the top.
		PHOTOS_EXIF_0ROW_RIGHT_0COL_TOP         = 6, //   6  =  0th row is on the right, and 0th column is the top.
		PHOTOS_EXIF_0ROW_RIGHT_0COL_BOTTOM      = 7, //   7  =  0th row is on the right, and 0th column is the bottom.
		PHOTOS_EXIF_0ROW_LEFT_0COL_BOTTOM       = 8  //   8  =  0th row is on the left, and 0th column is the bottom.
	};
	
	switch (orientation) {
		case UIDeviceOrientationPortraitUpsideDown:  // Device oriented vertically, home button on the top
			exifOrientation = PHOTOS_EXIF_0ROW_LEFT_0COL_BOTTOM;
			break;
		case UIDeviceOrientationLandscapeLeft:       // Device oriented horizontally, home button on the right
            if (_index == FRONT_CAMERA_INDEX)
                exifOrientation = PHOTOS_EXIF_0ROW_BOTTOM_0COL_RIGHT;
			else
                exifOrientation = PHOTOS_EXIF_0ROW_TOP_0COL_LEFT;
			break;
		case UIDeviceOrientationLandscapeRight:      // Device oriented horizontally, home button on the left
            if (_index == FRONT_CAMERA_INDEX)
                exifOrientation = PHOTOS_EXIF_0ROW_TOP_0COL_LEFT;
			else
                exifOrientation = PHOTOS_EXIF_0ROW_BOTTOM_0COL_RIGHT;
			break;
		case UIDeviceOrientationPortrait:            // Device oriented vertically, home button on the bottom
		default:
			exifOrientation = PHOTOS_EXIF_0ROW_RIGHT_0COL_TOP;
			break;
	}
    return [NSNumber numberWithInt:exifOrientation];
}

- (BOOL) doFaceDetectionFromSampleBuffer:(CMSampleBufferRef) sampleBuffer
{
    BOOL ret = NO;
    CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	CFDictionaryRef attachments = CMCopyDictionaryOfAttachments(kCFAllocatorDefault, sampleBuffer, kCMAttachmentMode_ShouldPropagate);
	CIImage *ciImage = [[CIImage alloc] initWithCVPixelBuffer:pixelBuffer options:(NSDictionary*)attachments];
	if (attachments) {
		CFRelease(attachments);
    }
    
    // make sure your device orientation is not locked.
	UIDeviceOrientation curDeviceOrientation = [[UIDevice currentDevice] orientation];
    
	NSDictionary *imageOptions = nil;
    
	imageOptions = [NSDictionary dictionaryWithObject:[self exifOrientation:curDeviceOrientation]
                                               forKey:CIDetectorImageOrientation];
    
	NSArray *features = [self.faceDetector featuresInImage:ciImage
                                                   options:imageOptions];
	
    ret = [features count] > 0;
    
//    if (ret)
//        NSLog(@"Face Detected");
//    else
//        NSLog(@"Face NOT Detected");
    
    [ciImage release];
    return ret;
}
// This is the callback that is called when the OS has a frame to deliver to us.
// Starts being called when [_captureSession startRunning] is called. Stopped
// similarly.
// Parameter videoFrame contains the image. The format, size, and frequency
// were setup earlier.
// Returns 0 on success, -1 otherwise.
- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    bool faceDetected = false;
    if (_faceDetection)
      faceDetected = [self doFaceDetectionFromSampleBuffer:sampleBuffer];
    /*CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	CFDictionaryRef attachments = CMCopyDictionaryOfAttachments(kCFAllocatorDefault, sampleBuffer, kCMAttachmentMode_ShouldPropagate);
	CIImage *ciImage = [[CIImage alloc] initWithCVPixelBuffer:pixelBuffer options:(NSDictionary*)attachments];
	if (attachments) {
		CFRelease(attachments);
    }
    
    // make sure your device orientation is not locked.
	UIDeviceOrientation curDeviceOrientation = [[UIDevice currentDevice] orientation];
    
	NSDictionary *imageOptions = nil;
    
	imageOptions = [NSDictionary dictionaryWithObject:[self exifOrientation:curDeviceOrientation]
                                               forKey:CIDetectorImageOrientation];
    
	NSArray *features = [self.faceDetector featuresInImage:ciImage
                                                   options:imageOptions];
	
    if ([features count] > 0 )
        NSLog(@"Detected");
    else
        NSLog(@"NOT Detected");*/
    
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
    void* baseAddressY = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
    void* baseAddressUV = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 1);
#elif defined (RGBA_CAPTURE)
    int bufferWidth = CVPixelBufferGetWidth(imageBuffer);
    int bufferHeight = CVPixelBufferGetHeight(imageBuffer);
    int bufferBytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    void* baseAddressY = CVPixelBufferGetBaseAddress(imageBuffer);
#endif

    if(_owner)
    {
        CVBufferRetain(imageBuffer);
        webrtc::VideoCaptureCapability tempCaptureCapability;
#ifdef YUV_CAPTURE
        if (_captureWidth >> 1 == _frameWidth)
        {
            tempCaptureCapability.width = _captureWidth >> 1;
            tempCaptureCapability.height = _captureHeight >> 1;
        }
        else if (_captureWidth >> 2 == _frameWidth)
        {
            tempCaptureCapability.width = _captureWidth >> 2;
            tempCaptureCapability.height = _captureHeight >> 2;
        }
        else if (_captureWidth >> 3 == _frameWidth)
        {
          tempCaptureCapability.width = _captureWidth >> 3;
          tempCaptureCapability.height = _captureHeight >> 3;
        }
        tempCaptureCapability.maxFPS = _frameRate;
        tempCaptureCapability.rawType = webrtc::kVideoI420;
        tempCaptureCapability.codecType = webrtc::kVideoCodecUnknown;
#elif defined RGBA_CAPTURE
        tempCaptureCapability.width = _captureWidth >> 1;
        tempCaptureCapability.height = _captureHeight >> 1;
        tempCaptureCapability.maxFPS = _frameRate;
        tempCaptureCapability.rawType = webrtc::kVideoARGB;
        tempCaptureCapability.codecType = webrtc::kVideoCodecUnknown;
#endif
      
#ifdef YUV_CAPTURE
      int frameSize = 0;
      if (_captureWidth >> 1 == _frameWidth)
      {
          frameSize = bytesPerRowY*planeHeightY+bytesPerRowUV*planeHeightUV >> 2;
          webrtc::ConvertNV12ToI420AndScaleFrameDouble(planeWidthY, planeHeightY, (WebRtc_UWord8*)baseAddressY, (WebRtc_UWord8*)baseAddressUV);
      }
      else if (_captureWidth >> 2 == _frameWidth)
      {
          frameSize = bytesPerRowY*planeHeightY+bytesPerRowUV*planeHeightUV >> 4;
          webrtc::ConvertNV12ToI420AndScaleFrameQuad(planeWidthY, planeHeightY, (WebRtc_UWord8*)baseAddressY, (WebRtc_UWord8*)baseAddressUV);
      }
      else if (_captureWidth >> 3 == _frameWidth)
      {
        frameSize = bytesPerRowY*planeHeightY+bytesPerRowUV*planeHeightUV >> 6;
        webrtc::ConvertNV12ToI420AndScaleFrameQuad(planeWidthY, planeHeightY, (WebRtc_UWord8*)baseAddressY, (WebRtc_UWord8*)baseAddressUV);
        webrtc::ScaleI420FrameDouble(planeWidthY >> 2, planeHeightY >> 2, (WebRtc_UWord8*)baseAddressY);
      }
#elif defined RGBA_CAPTURE
        int frameSize = bufferBytesPerRow*bufferHeight >> 2;
        webrtc::ScaleRGBAFrameDouble(bufferWidth, bufferHeight, (WebRtc_UWord8*)baseAddressY);
#endif
      
        _owner->IncomingFrame((unsigned char*)baseAddressY,
                              frameSize,
                              tempCaptureCapability,
                              0,
                              faceDetected);
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
