/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once

#include "video_render.h"
#include "testDefines.h"

#ifdef _WIN32

#include <windows.h>
#elif defined(MAC_IPHONE)
#import <UIKit/UIKit.h>
//#import "CocoaWindow.h"
#import "uiview_renderer.h"
#elif defined (WEBRTC_ANDROID)
#include <JNI.h>
#elif defined(WEBRTC_LINUX)
typedef void* HWND;
#endif

#include "thread_wrapper.h"
#include "iphone_render_view.h"

namespace webrtc
{

class Renderer
{
public:
    Renderer(IPhoneRenderView* renderView, bool preview = false);
    ~Renderer(void);

    void RenderFrame(VideoFrame& videoFrame);
    void PaintGreen();
    void PaintBlue();
    void* GetWindow();

#if defined (WEBRTC_ANDROID)
    static void SetRenderWindow(jobject renderWindow);
#endif

private:

    static bool RenderThread(ThreadObj);
    bool RenderThreadProcess();

    VideoRender* _renderModule;
    VideoRenderCallback* _renderProvider;
    VideoFrame _videoFrame;
    bool _quiting;
    ThreadWrapper* _messageThread;
    static int _screen;
    static const WebRtc_UWord32 _frameWidth = 640;
    static const WebRtc_UWord32 _frameHeight = 480;

#if defined(_WIN32)
    HWND _renderWindow;
#elif defined(MAC_IPHONE)
    NSAutoreleasePool* _pool;
    IPhoneRenderView* _renderView;
#elif defined (WEBRTC_ANDROID)
    jobject _renderWindow; //this is a glsurface.
public:
    static jobject g_renderWindow;
#elif defined(WEBRTC_LINUX)
    typedef void* HWND;
    HWND _renderWindow;
#endif
};
} // namespace webrtc
