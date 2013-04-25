/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_BB_VIDEO_RENDER_BB_IMPL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_BB_VIDEO_RENDER_BB_IMPL_H_

#include "i_video_render.h"
#include "common_types.h"
#include "map_wrapper.h"

struct _screen_context;
struct _screen_window;
struct _screen_display;
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLSurface;

namespace webrtc {

class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class BlackberryWindowWrapper;
class VideoRenderBlackBerry;
class VideoRenderOpenGles20;

// The object a module user uses to send new frames to the Blackberry OpenGL ES window

class BlackberryRenderCallback : public VideoRenderCallback {
 public:
  BlackberryRenderCallback(VideoRenderBlackBerry* parentRenderer,
                           WebRtc_UWord32 streamId);

  virtual WebRtc_Word32 RenderFrame(const WebRtc_UWord32 streamId,
                                    VideoFrame& videoFrame);

  virtual ~BlackberryRenderCallback() {};

  void RenderToGL();

  VideoRenderOpenGles20* GetRenderer() { return _ptrOpenGLRenderer; }

 private:
  VideoRenderBlackBerry* _ptrParentRenderer;
  VideoFrame _videoFrame;
  bool _hasFrame;
  bool _frameIsRendered;
  bool _isSetup;
  VideoRenderOpenGles20* _ptrOpenGLRenderer;
};

class VideoRenderBlackBerry : IVideoRender
{
 public:
  static WebRtc_Word32 SetAndroidEnvVariables(void* javaVM);

  VideoRenderBlackBerry(const WebRtc_Word32 id,
                     const VideoRenderType videoRenderType,
                     void* window,
                     const bool fullscreen);

  virtual ~VideoRenderBlackBerry();

  virtual WebRtc_Word32 Init() { return 1; }

  virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

  virtual WebRtc_Word32 ChangeWindow(void* window);

  virtual VideoRenderCallback* AddIncomingRenderStream(
      const WebRtc_UWord32 streamId,
      const WebRtc_UWord32 zOrder,
      const float left, const float top,
      const float right, const float bottom);

  virtual WebRtc_Word32 DeleteIncomingRenderStream(
      const WebRtc_UWord32 streamId);

  virtual WebRtc_Word32 GetIncomingRenderStreamProperties(
      const WebRtc_UWord32 streamId,
      WebRtc_UWord32& zOrder,
      float& left, float& top,
      float& right, float& bottom) const;

  virtual WebRtc_Word32 StartRender();

  virtual WebRtc_Word32 StopRender();

  virtual void ReDraw();

  virtual void OnBBRenderEvent();

  // Properties

  virtual VideoRenderType RenderType();

  virtual RawVideoType PerferedVideoType();

  virtual bool FullScreen();

  virtual WebRtc_Word32 GetGraphicsMemory(
      WebRtc_UWord64& totalGraphicsMemory,
      WebRtc_UWord64& availableGraphicsMemory) const;

  virtual WebRtc_Word32 GetScreenResolution(
      WebRtc_UWord32& screenWidth,
      WebRtc_UWord32& screenHeight) const;

  virtual WebRtc_UWord32 RenderFrameRate(const WebRtc_UWord32 streamId);

  virtual WebRtc_Word32 SetStreamCropping(const WebRtc_UWord32 streamId,
                                          const float left, const float top,
                                          const float right,
                                          const float bottom);

  virtual WebRtc_Word32 SetTransparentBackground(const bool enable);

  virtual WebRtc_Word32 ConfigureRenderer(const WebRtc_UWord32 streamId,
                                          const unsigned int zOrder,
                                          const float left, const float top,
                                          const float right,
                                          const float bottom);

  virtual WebRtc_Word32 SetText(const WebRtc_UWord8 textId,
                                const WebRtc_UWord8* text,
                                const WebRtc_Word32 textLength,
                                const WebRtc_UWord32 textColorRef,
                                const WebRtc_UWord32 backgroundColorRef,
                                const float left, const float top,
                                const float rigth, const float bottom);

  virtual WebRtc_Word32 SetBitmap(const void* bitMap,
                                  const WebRtc_UWord8 pictureId,
                                  const void* colorKey, const float left,
                                  const float top, const float right,
                                  const float bottom);

 protected:

  bool CreateGLThread();
  static void* GLThread(void* pThis);
  void GLThreadRun();

  bool CreateGLWindow();
  bool CleanUpGLWindow();

  virtual BlackberryRenderCallback* CreateRenderChannel(
      WebRtc_Word32 streamId,
      WebRtc_Word32 zOrder,
      const float left,
      const float top,
      const float right,
      const float bottom,
      VideoRenderBlackBerry& renderer);

//  screen_window_t

  WebRtc_Word32 _id;
  CriticalSectionWrapper& _critSect;
  VideoRenderType _renderType;
  BlackberryWindowWrapper* _ptrWindowWrapper;
  _screen_window* _ptrGLWindow;
  _screen_display* _ptrDisplay;
  EGLDisplay _eglDisplay;
  EGLConfig _eglConfig;
  EGLContext _eglContext;
  EGLSurface _eglSurface;

  int _windowWidth;
  int _windowHeight;
  bool _glInitialized;
  bool _stopped;


 private:

  // Map with streams to render.
  MapWrapper _streamsMap;
};

} //namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_BB_VIDEO_RENDER_BB_IMPL_H_
