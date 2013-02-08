/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if defined(_WIN32)
#include <tchar.h>
#include <windows.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#elif defined(WEBRTC_LINUX)
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/time.h>

#elif defined(WEBRTC_MAC_INTEL)
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <QTKit/QTKit.h>
#import "cocoa_renderer.h"
#include <sys/time.h>
#include <iostream>
#endif

#include "iphone_render_view.h"
#include "trace.h"

using namespace std;
using namespace webrtc;

#include <stdio.h>
#include "testExternalCapture.h"
#ifndef WEBRTC_VIDEO_EXTERNAL_CAPTURE_AND_RENDER
#include "testPlatformDependent.h"
//#include "testCameraEncoder.h"
#endif

bool TestThread(ThreadObj obj);
const char* g_fileName;
int g_testID; 
IPhoneRenderView* g_renderView;

#if defined(_WIN32)
int _tmain(int argc, _TCHAR* argv[])
#elif defined(WEBRTC_LINUX)
int main(int argc, char* argv[])
#elif defined(WEBRTC_MAC_INTEL)
int main (int argc, const char * argv[])
#endif
int hookflash_test(const char* fileName, int testID, IPhoneRenderView* renderView)
{
    g_fileName = fileName;
    g_renderView = renderView;
    
    ThreadWrapper* _messageThread = ThreadWrapper::CreateThread(TestThread, NULL,
                                                 kLowPriority, "TestThread");
    unsigned int threadId;
    _messageThread->Start(threadId);
    
    return 0;
}

bool TestThread(ThreadObj obj)
{
    Trace::CreateTrace();
    Trace::SetTraceFile(g_fileName);
    Trace::SetLevelFilter(kTraceAll);
    
#if defined(WEBRTC_MAC_INTEL)
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];
#endif
    int testResult=0;
    /*
     {
     webrtc::testExternalCapture test;
     testResult=test.DoTest();
     printf("\nExternal capture test result %d\n",testResult);
     }
     */
#ifndef WEBRTC_VIDEO_EXTERNAL_CAPTURE_AND_RENDER
    {
        webrtc::testPlatformDependent platformDependent;
        testResult=platformDependent.DoTest(g_renderView);
        printf("\nPlatform dependent test result %d\n",testResult);
    }
    /*
     {
     webrtc::testCameraEncoder cameraEncoder;
     testResult=cameraEncoder.DoTest();
     printf("\nCamera encoder test result %d\n",testResult);
     
     }
     */
#endif
    
    Trace::ReturnTrace();
    
    //getchar();
    
#if defined (WEBRTC_MAC_INTEL)
    [pool release];
#endif
    return 0;
}


