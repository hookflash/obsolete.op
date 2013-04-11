/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "engine_configurations.h"

#include "video_render_uiview.h"
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"
#include "libyuv.h"

namespace webrtc {

VideoChannelUIView::VideoChannelUIView(UIView *nsglContext, int iId, VideoRenderUIView* owner) :
_nsglContext( nsglContext),
_id( iId),
_owner( owner),
_width( 0),
_height( 0),
_startWidth( 0.0f),
_startHeight( 0.0f),
_stopWidth( 0.0f),
_stopHeight( 0.0f),
_stretchedWidth( 0),
_stretchedHeight( 0),
//_oldStretchedHeight( 0),
//_oldStretchedWidth( 0),
//_xOldWidth( 0),
//_yOldHeight( 0),
_buffer( 0),
_bufferSize( 0),
_incommingBufferSize( 0),
_bufferIsUpdated( false),
_numberOfStreams( 0),
//_pixelFormat(0),
//_pixelDataType(0),
//_bVideoSizeStartedChanging(false),
_framesDelivered(0)
//_lastFramerateReportTime(TickTime::Now()),
//_lastFramerateReportFramesDelivered(0)
{

}

VideoChannelUIView::~VideoChannelUIView()
{
    if (_buffer)
    {
        delete [] _buffer;
        _buffer = NULL;
    }
}

int VideoChannelUIView::ChangeContext(UIView *nsglContext)
{
    _owner->LockAGLCntx();

    _nsglContext = nsglContext;

    _owner->UnlockAGLCntx();
    return 0;

}

WebRtc_Word32 VideoChannelUIView::GetChannelProperties(float& left,
        float& top,
        float& right,
        float& bottom)
{

    _owner->LockAGLCntx();

    left = _startWidth;
    top = _startHeight;
    right = _stopWidth;
    bottom = _stopHeight;

    _owner->UnlockAGLCntx();
    return 0;
}

WebRtc_Word32 VideoChannelUIView::RenderFrame(const WebRtc_UWord32 /*streamId*/, VideoFrame& videoFrame)
{

    _owner->LockAGLCntx();

    if(_width != (int)videoFrame.Width() ||
            _height != (int)videoFrame.Height())
    {
        if(FrameSizeChange(videoFrame.Width(), videoFrame.Height(), 1) == -1)
        {
            _owner->UnlockAGLCntx();
            return -1;
        }
    }

    int ret = DeliverFrame(videoFrame.Buffer(), videoFrame.Length(), videoFrame.TimeStamp());    
    
    _owner->UnlockAGLCntx();
    return ret;
}

int VideoChannelUIView::UpdateSize(int width, int height)
{
    _owner->LockAGLCntx();
    _width = width;
    _height = height;
    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelUIView::UpdateStretchSize(int stretchHeight, int stretchWidth)
{

    _owner->LockAGLCntx();
    _stretchedHeight = stretchHeight;
    _stretchedWidth = stretchWidth;
    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelUIView::FrameSizeChange(int width, int height, int numberOfStreams)
{
    //  We got a new frame size from VideoAPI, prepare the buffer

    _owner->LockAGLCntx();

    if (width == _width && _height == height)
    {
        // We already have a correct buffer size
        _numberOfStreams = numberOfStreams;
        _owner->UnlockAGLCntx();
        return 0;
    }

    _width = width;
    _height = height;

    // Delete the old buffer, create a new one with correct size.
    if (_buffer)
    {
        delete [] _buffer;
        _bufferSize = 0;
    }

    _incommingBufferSize = CalcBufferSize(kI420, _width, _height);
    _bufferSize = CalcBufferSize(kARGB, _width, _height);//_width * _height * bytesPerPixel;
    _buffer = new unsigned char [_bufferSize];
    memset(_buffer, 0, _bufferSize * sizeof(unsigned char));

    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelUIView::DeliverFrame(unsigned char* buffer, int bufferSize, unsigned int /*timeStamp90kHz*/)
{

    _owner->LockAGLCntx();

    if (bufferSize != _incommingBufferSize)
    {
        _owner->UnlockAGLCntx();
        return -1;
    }
    
    // Allocate ARGB buffer
    VideoFrame captureFrame;
    captureFrame.VerifyAndAllocate(CalcBufferSize(kARGB, _width, _height));
    if (!captureFrame.Buffer())
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to allocate capture frame buffer.");
        _owner->UnlockAGLCntx();
        return -1;
    }
  
    _framesDelivered++;

    ConvertI420ToABGR(buffer, captureFrame.Buffer(), _width, _height, 0);
  
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  
    CGContextRef context = 
        CGBitmapContextCreate(captureFrame.Buffer(), _width, _height, 8,
                              _width * 4, colorSpace,
                              kCGBitmapByteOrder32Little |
                              kCGImageAlphaPremultipliedFirst);
    
    CGImageRef cgImage = CGBitmapContextCreateImage(context); 
    
    CGContextRelease(context); 
    CGColorSpaceRelease(colorSpace);
    
    UIImage* image = [UIImage imageWithCGImage:cgImage];
    
    CGImageRelease(cgImage);
  
    UIImageView* windowRef = _owner->getWindowReference();
//    [windowRef setFrame:CGRectMake(windowRef.frame.origin.x, windowRef.frame.origin.y, image.size.width, image.size.height)];
    [windowRef performSelectorOnMainThread:@selector(setImage:)
                              withObject:image waitUntilDone:NO];
  
    [pool release];
  
//    if ((TickTime::Now() - _lastFramerateReportTime).Milliseconds() >= 1000)
//    {
//        printf("Render frame rate: %d\n", _framesDelivered - _lastFramerateReportFramesDelivered);
//        _lastFramerateReportTime = TickTime::Now();
//        _lastFramerateReportFramesDelivered = _framesDelivered;
//    }
//
//    _bufferIsUpdated = true;

    _owner->UnlockAGLCntx();

    return 0;
}

int VideoChannelUIView::RenderOffScreenBuffer()
{

    _owner->LockAGLCntx();

    _bufferIsUpdated = false;

    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelUIView::IsUpdated(bool& isUpdated)
{
    _owner->LockAGLCntx();

    isUpdated = _bufferIsUpdated;

    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelUIView::SetStreamSettings(int /*streamId*/, float startWidth, float startHeight, float stopWidth, float stopHeight)
{
    _owner->LockAGLCntx();

    _startWidth = startWidth;
    _stopWidth = stopWidth;
    _startHeight = startHeight;
    _stopHeight = stopHeight;

    int oldWidth = _width;
    int oldHeight = _height;
    int oldNumberOfStreams = _numberOfStreams;

    _width = 0;
    _height = 0;

    int retVal = FrameSizeChange(oldWidth, oldHeight, oldNumberOfStreams);

    _owner->UnlockAGLCntx();
    return retVal;
}

int VideoChannelUIView::SetStreamCropSettings(int /*streamId*/, float /*startWidth*/, float /*startHeight*/, float /*stopWidth*/, float /*stopHeight*/)
{
    return -1;
}

/*
 *
 *    VideoRenderUIView
 *
 */

VideoRenderUIView::VideoRenderUIView(UIImageView *windowRef, bool fullScreen, int iId) :
_windowRef( (UIImageView*)windowRef),
_id( iId),
_nsglContextCritSec( *CriticalSectionWrapper::CreateCriticalSection()),
_screenUpdateThread( 0),
_screenUpdateEvent( 0),
_nsglContext( 0),
_nsglFullScreenContext( 0),
_windowRect( ),
_windowWidth( 0),
_windowHeight( 0),
_nsglChannels( ),
_zOrderToChannel( ),
_threadID (0),
_renderingIsPaused (FALSE)
{
    _screenUpdateThread = ThreadWrapper::CreateThread(ScreenUpdateThreadProc, this, kRealtimePriority);
    _screenUpdateEvent = EventWrapper::Create();
}

int VideoRenderUIView::ChangeWindow(UIImageView* newWindowRef)
{

    LockAGLCntx();

    _windowRef = newWindowRef;

    if(CreateMixingContext() == -1)
    {
        UnlockAGLCntx();
        return -1;
    }

    int error = 0;
    std::map<int, VideoChannelUIView*>::iterator it = _nsglChannels.begin();
    while (it!= _nsglChannels.end())
    {
        error |= (it->second)->ChangeContext(_nsglContext);
        it++;
    }
    if(error != 0)
    {
        UnlockAGLCntx();
        return -1;
    }

    UnlockAGLCntx();
    return 0;
}

/* Check if the thread and event already exist. 
 * If so then they will simply be restarted
 * If not then create them and continue
 */
WebRtc_Word32 VideoRenderUIView::StartRender()
{

    LockAGLCntx();

    const unsigned int MONITOR_FREQ = 60;
    if(TRUE == _renderingIsPaused)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceVideoRenderer, _id, "Restarting screenUpdateThread");

        // we already have the thread. Most likely StopRender() was called and they were paused
        if(FALSE == _screenUpdateThread->Start(_threadID) ||
                FALSE == _screenUpdateEvent->StartTimer(true, 1000/MONITOR_FREQ))
        {
            WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id, "Failed to restart screenUpdateThread or screenUpdateEvent");
            UnlockAGLCntx();
            return -1;
        }

        UnlockAGLCntx();
        return 0;
    }

    WEBRTC_TRACE(kTraceDebug, kTraceVideoRenderer, _id, "Starting screenUpdateThread and screenUpdateEvent");
    _screenUpdateThread = ThreadWrapper::CreateThread(ScreenUpdateThreadProc, this, kRealtimePriority);
    _screenUpdateEvent = EventWrapper::Create();

    if (!_screenUpdateThread)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceVideoRenderer, _id, "failed start screenUpdateThread");
        UnlockAGLCntx();
        return -1;
    }

    _screenUpdateThread->Start(_threadID);

    _screenUpdateEvent->StartTimer(true, 1000/MONITOR_FREQ);

    UnlockAGLCntx();
    return 0;
}
WebRtc_Word32 VideoRenderUIView::StopRender()
{

    LockAGLCntx();

    /* The code below is functional
     * but it pauses for several seconds
     */

    // pause the update thread and the event timer
    if(!_screenUpdateThread || !_screenUpdateEvent)
    {
        _renderingIsPaused = TRUE;

        UnlockAGLCntx();
        return 0;
    }

    if(FALSE == _screenUpdateThread->Stop() || FALSE == _screenUpdateEvent->StopTimer())
    {
        _renderingIsPaused = FALSE;

        UnlockAGLCntx();
        return -1;
    }

    _renderingIsPaused = TRUE;

    UnlockAGLCntx();
    return 0;
}

int VideoRenderUIView::configureNSOpenGLView()
{
    return 0;

}

int VideoRenderUIView::configureNSOpenGLEngine()
{

    LockAGLCntx();

    if (_windowWidth != (_windowRect.right - _windowRect.left)
            || _windowHeight != (_windowRect.bottom - _windowRect.top))
    {
        _windowWidth = _windowRect.right - _windowRect.left;
        _windowHeight = _windowRect.bottom - _windowRect.top;
    }

    UnlockAGLCntx();
    return 0;
}

int VideoRenderUIView::setRenderTargetWindow()
{
    LockAGLCntx();

    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoRenderer, _id, "%s:%d", __FUNCTION__, __LINE__);

    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoRenderer, _id, "setRenderTargetWindow glClearColor 0.5");

    DisplayBuffers();

    UnlockAGLCntx();
    return 0;
}

int VideoRenderUIView::setRenderTargetFullScreen()
{
    LockAGLCntx();

    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoRenderer, _id, "%s:%d", __FUNCTION__, __LINE__);

    DisplayBuffers();

    UnlockAGLCntx();
    return 0;
}

VideoRenderUIView::~VideoRenderUIView()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceVideoRenderer, 0, "%s:%d", __FUNCTION__, __LINE__);

    // Signal event to exit thread, then delete it
    ThreadWrapper* tmpPtr = _screenUpdateThread;
    _screenUpdateThread = NULL;

    if (tmpPtr)
    {
        tmpPtr->SetNotAlive();
        _screenUpdateEvent->Set();
        _screenUpdateEvent->StopTimer();

        if (tmpPtr->Stop())
        {
            delete tmpPtr;
        }
        delete _screenUpdateEvent;
        _screenUpdateEvent = NULL;
    }

    if (_nsglContext != 0)
    {
        _nsglContext = nil;
    }

    // Delete all channels
    std::map<int, VideoChannelUIView*>::iterator it = _nsglChannels.begin();
    while (it!= _nsglChannels.end())
    {
        delete it->second;
        _nsglChannels.erase(it);
        it = _nsglChannels.begin();
    }
    _nsglChannels.clear();

    // Clean the zOrder map
    std::multimap<int, int>::iterator zIt = _zOrderToChannel.begin();
    while(zIt != _zOrderToChannel.end())
    {
        _zOrderToChannel.erase(zIt);
        zIt = _zOrderToChannel.begin();
    }
    _zOrderToChannel.clear();

}

/* static */
int VideoRenderUIView::GetOpenGLVersion(int& /*nsglMajor*/, int& /*nsglMinor*/)
{
    return -1;
}

int VideoRenderUIView::Init()
{

    LockAGLCntx();
    if (!_screenUpdateThread)
    {
        UnlockAGLCntx();
        return -1;
    }

    _screenUpdateThread->Start(_threadID);

    // Start the event triggering the render process
    unsigned int monitorFreq = 60;
    _screenUpdateEvent->StartTimer(true, 1000/monitorFreq);

    if (CreateMixingContext() == -1)
    {
        UnlockAGLCntx();
        return -1;
    }

    UnlockAGLCntx();
    return 0;
}

VideoChannelUIView* VideoRenderUIView::CreateNSGLChannel(int channel, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight)
{
    CriticalSectionScoped cs(_nsglContextCritSec);

    if (HasChannel(channel))
    {
        return NULL;
    }

    if (_zOrderToChannel.find(zOrder) != _zOrderToChannel.end())
    {

    }

    VideoChannelUIView* newAGLChannel = new VideoChannelUIView(_nsglContext, _id, this);
    if (newAGLChannel->SetStreamSettings(0, startWidth, startHeight, stopWidth, stopHeight) == -1)
    {
        if (newAGLChannel)
        {
            delete newAGLChannel;
            newAGLChannel = NULL;
        }

        return NULL;
    }

    _nsglChannels[channel] = newAGLChannel;
    _zOrderToChannel.insert(std::pair<int, int>(zOrder, channel));

    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s successfully created NSGL channel number %d", __FUNCTION__, channel);

    return newAGLChannel;
}

int VideoRenderUIView::DeleteAllNSGLChannels()
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    std::map<int, VideoChannelUIView*>::iterator it;
    it = _nsglChannels.begin();

    while (it != _nsglChannels.end())
    {
        VideoChannelUIView* channel = it->second;
        WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s Deleting channel %d", __FUNCTION__, channel);
        delete channel;
        it++;
    }
    _nsglChannels.clear();
    return 0;
}

WebRtc_Word32 VideoRenderUIView::DeleteNSGLChannel(const WebRtc_UWord32 channel)
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    std::map<int, VideoChannelUIView*>::iterator it;
    it = _nsglChannels.find(channel);
    if (it != _nsglChannels.end())
    {
        delete it->second;
        _nsglChannels.erase(it);
    }
    else
    {
        return -1;
    }

    std::multimap<int, int>::iterator zIt = _zOrderToChannel.begin();
    while( zIt != _zOrderToChannel.end())
    {
        if (zIt->second == (int)channel)
        {
            _zOrderToChannel.erase(zIt);
            break;
        }
        zIt++;
    }

    return 0;
}

WebRtc_Word32 VideoRenderUIView::GetChannelProperties(const WebRtc_UWord16 streamId,
        WebRtc_UWord32& zOrder,
        float& left,
        float& top,
        float& right,
        float& bottom)
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    bool channelFound = false;

    // Loop through all channels until we find a match.
    // From that, get zorder.
    // From that, get T, L, R, B
    for (std::multimap<int, int>::reverse_iterator rIt = _zOrderToChannel.rbegin();
            rIt != _zOrderToChannel.rend();
            rIt++)
    {
        if(streamId == rIt->second)
        {
            channelFound = true;

            zOrder = rIt->second;

            std::map<int, VideoChannelUIView*>::iterator rIt = _nsglChannels.find(streamId);
            VideoChannelUIView* tempChannel = rIt->second;

            if(-1 == tempChannel->GetChannelProperties(left, top, right, bottom) )
            {
                return -1;
            }
            break;
        }
    }

    if(false == channelFound)
    {

        return -1;
    }

    return 0;
}

int VideoRenderUIView::StopThread()
{

    ThreadWrapper* tmpPtr = _screenUpdateThread;
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s Stopping thread ", __FUNCTION__, _screenUpdateThread);
    _screenUpdateThread = NULL;

    if (tmpPtr)
    {
        tmpPtr->SetNotAlive();
        _screenUpdateEvent->Set();
        if (tmpPtr->Stop())
        {
            delete tmpPtr;
        }
    }

    delete _screenUpdateEvent;
    _screenUpdateEvent = NULL;

    return 0;
}

bool VideoRenderUIView::HasChannels()
{
    CriticalSectionScoped cs(_nsglContextCritSec);

    if (_nsglChannels.begin() != _nsglChannels.end())
    {
        return true;
    }
    return false;
}

bool VideoRenderUIView::HasChannel(int channel)
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    std::map<int, VideoChannelUIView*>::iterator it = _nsglChannels.find(channel);

    if (it != _nsglChannels.end())
    {
        return true;
    }
    return false;
}

int VideoRenderUIView::GetChannels(std::list<int>& channelList)
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    std::map<int, VideoChannelUIView*>::iterator it = _nsglChannels.begin();

    while (it != _nsglChannels.end())
    {
        channelList.push_back(it->first);
        it++;
    }

    return 0;
}

VideoChannelUIView* VideoRenderUIView::ConfigureNSGLChannel(int channel, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight)
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    std::map<int, VideoChannelUIView*>::iterator it = _nsglChannels.find(channel);

    if (it != _nsglChannels.end())
    {
        VideoChannelUIView* aglChannel = it->second;
        if (aglChannel->SetStreamSettings(0, startWidth, startHeight, stopWidth, stopHeight) == -1)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id, "%s failed to set stream settings: channel %d. channel=%d zOrder=%d startWidth=%d startHeight=%d stopWidth=%d stopHeight=%d",
                    __FUNCTION__, channel, zOrder, startWidth, startHeight, stopWidth, stopHeight);
            return NULL;
        }
        WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s Configuring channel %d. channel=%d zOrder=%d startWidth=%d startHeight=%d stopWidth=%d stopHeight=%d",
                __FUNCTION__, channel, zOrder, startWidth, startHeight, stopWidth, stopHeight);

        std::multimap<int, int>::iterator it = _zOrderToChannel.begin();
        while(it != _zOrderToChannel.end())
        {
            if (it->second == channel)
            {
                if (it->first != zOrder)
                {
                    _zOrderToChannel.erase(it);
                    _zOrderToChannel.insert(std::pair<int, int>(zOrder, channel));
                }
                break;
            }
            it++;
        }
        return aglChannel;
    }

    return NULL;
}

/*
 *
 *    Rendering process
 *
 */

bool VideoRenderUIView::ScreenUpdateThreadProc(void* obj)
{
    return static_cast<VideoRenderUIView*>(obj)->ScreenUpdateProcess();
}

bool VideoRenderUIView::ScreenUpdateProcess()
{

    _screenUpdateEvent->Wait(10);
    LockAGLCntx();

    if (!_screenUpdateThread)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, _id, "%s no screen update thread", __FUNCTION__);
        UnlockAGLCntx();
        return false;
    }

    if (GetWindowRect(_windowRect) == -1)
    {
        UnlockAGLCntx();
        return true;
    }

    if (_windowWidth != (_windowRect.right - _windowRect.left)
            || _windowHeight != (_windowRect.bottom - _windowRect.top))
    {
        _windowWidth = _windowRect.right - _windowRect.left;
        _windowHeight = _windowRect.bottom - _windowRect.top;
    }

    // Check if there are any updated buffers
    bool updated = false;
    std::map<int, VideoChannelUIView*>::iterator it = _nsglChannels.begin();
    while (it != _nsglChannels.end())
    {

        VideoChannelUIView* aglChannel = it->second;
        aglChannel->UpdateStretchSize(_windowHeight, _windowWidth);
        aglChannel->IsUpdated(updated);
        if (updated)
        {
            break;
        }
        it++;
    }

    if (updated)
    {

        // At least on buffers is updated, we need to repaint the texture
        if (RenderOffScreenBuffers() != -1)
        {
            UnlockAGLCntx();
            return true;
        }
    }
    //    }
    UnlockAGLCntx();
    return true;
}

/*
 *
 *    Functions for creating mixing buffers and screen settings
 *
 */

int VideoRenderUIView::CreateMixingContext()
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    if(-1 == setRenderTargetWindow())
    {
        return -1;
    }

    configureNSOpenGLEngine();

    DisplayBuffers();

    return 0;
}

/*
 *
 *    Rendering functions
 *
 */

int VideoRenderUIView::RenderOffScreenBuffers()
{
    LockAGLCntx();

    // Get the current window size, it might have changed since last render.
    if (GetWindowRect(_windowRect) == -1)
    {
        UnlockAGLCntx();
        return -1;
    }

    // Loop through all channels starting highest zOrder ending with lowest.
    for (std::multimap<int, int>::reverse_iterator rIt = _zOrderToChannel.rbegin();
            rIt != _zOrderToChannel.rend();
            rIt++)
    {
        int channelId = rIt->second;
        std::map<int, VideoChannelUIView*>::iterator it = _nsglChannels.find(channelId);

        VideoChannelUIView* aglChannel = it->second;

        aglChannel->RenderOffScreenBuffer();
    }

    DisplayBuffers();

    UnlockAGLCntx();
    return 0;
}

/*
 *
 * Help functions
 *
 * All help functions assumes external protections
 *
 */

int VideoRenderUIView::DisplayBuffers()
{

    LockAGLCntx();

    WEBRTC_TRACE(kTraceDebug, kTraceVideoRenderer, _id, "%s glFinish and [_nsglContext flushBuffer]", __FUNCTION__);

    UnlockAGLCntx();
    return 0;
}

int VideoRenderUIView::GetWindowRect(Rect& rect)
{

    CriticalSectionScoped cs(_nsglContextCritSec);

    if (_windowRef)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

WebRtc_Word32 VideoRenderUIView::ChangeUniqueID(WebRtc_Word32 id)
{

    CriticalSectionScoped cs(_nsglContextCritSec);
    _id = id;
    return 0;
}

WebRtc_Word32 VideoRenderUIView::SetText(const WebRtc_UWord8 /*textId*/,
        const WebRtc_UWord8* /*text*/,
        const WebRtc_Word32 /*textLength*/,
        const WebRtc_UWord32 /*textColorRef*/,
        const WebRtc_UWord32 /*backgroundColorRef*/,
        const float /*left*/,
        const float /*top*/,
        const float /*right*/,
        const float /*bottom*/)
{

    return 0;

}

void VideoRenderUIView::LockAGLCntx()
{
    _nsglContextCritSec.Enter();
}
void VideoRenderUIView::UnlockAGLCntx()
{
    _nsglContextCritSec.Leave();
}

/*

 bool VideoRenderNSOpenGL::SetFullScreen(bool fullscreen)
 {
 NSRect mainDisplayRect, viewRect;

 // Create a screen-sized window on the display you want to take over
 // Note, mainDisplayRect has a non-zero origin if the key window is on a secondary display
 mainDisplayRect = [[NSScreen mainScreen] frame];
 fullScreenWindow = [[NSWindow alloc] initWithContentRect:mainDisplayRect styleMask:NSBorderlessWindowMask
 backing:NSBackingStoreBuffered defer:YES];

 // Set the window level to be above the menu bar
 [fullScreenWindow setLevel:NSMainMenuWindowLevel+1];

 // Perform any other window configuration you desire
 [fullScreenWindow setOpaque:YES];
 [fullScreenWindow setHidesOnDeactivate:YES];

 // Create a view with a double-buffered OpenGL context and attach it to the window
 // By specifying the non-fullscreen context as the shareContext, we automatically inherit the OpenGL objects (textures, etc) it has defined
 viewRect = NSMakeRect(0.0, 0.0, mainDisplayRect.size.width, mainDisplayRect.size.height);
 fullScreenView = [[MyOpenGLView alloc] initWithFrame:viewRect shareContext:[openGLView openGLContext]];
 [fullScreenWindow setContentView:fullScreenView];

 // Show the window
 [fullScreenWindow makeKeyAndOrderFront:self];

 // Set the scene with the full-screen viewport and viewing transformation
 [scene setViewportRect:viewRect];

 // Assign the view's MainController to self
 [fullScreenView setMainController:self];

 if (!isAnimating) {
 // Mark the view as needing drawing to initalize its contents
 [fullScreenView setNeedsDisplay:YES];
 }
 else {
 // Start playing the animation
 [fullScreenView startAnimation];
 }

 }



 */


} //namespace webrtc

