/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video_render_bb_impl.h"

#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "thread_wrapper.h"
#include "tick_util.h"

#include "video_render_bb_opengles20.h"
#include <screen/screen.h>

#include "trace.h"

namespace webrtc {

VideoRenderBlackBerry::VideoRenderBlackBerry(
    const WebRtc_Word32 id,
    const VideoRenderType videoRenderType,
    void* window,
    const bool /*fullscreen*/):
    _id(id),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _renderType(videoRenderType),
    _ptrWindow((screen_window_t*)(window)),
    _streamsMap()
{
}

VideoRenderBlackBerry::~VideoRenderBlackBerry() {
  WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id,
               "VideoRenderBlackBerry dtor");

  for (MapItem* item = _streamsMap.First(); item != NULL; item
           = _streamsMap.Next(item)) { // Delete streams
    delete static_cast<BlackberryRenderCallback*> (item->GetItem());
  }
}

WebRtc_Word32 VideoRenderBlackBerry::ChangeUniqueId(const WebRtc_Word32 id) {
  CriticalSectionScoped cs(&_critSect);
  _id = id;

  return 0;
}

WebRtc_Word32 VideoRenderBlackBerry::ChangeWindow(void* /*window*/) {
  WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
  return -1;
}

VideoRenderCallback*
VideoRenderBlackBerry::AddIncomingRenderStream(const WebRtc_UWord32 streamId,
                                            const WebRtc_UWord32 zOrder,
                                            const float left, const float top,
                                            const float right,
                                            const float bottom) {
  CriticalSectionScoped cs(&_critSect);

  BlackberryRenderCallback* renderStream = NULL;
  MapItem* item = _streamsMap.Find(streamId);
  if (item) {
    renderStream = (BlackberryRenderCallback*) (item->GetItem());
    if (NULL != renderStream) {
      WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, -1,
                   "%s: Render stream already exists", __FUNCTION__);
      return renderStream;
    }
  }

  renderStream = CreateRenderChannel(streamId, zOrder, left, top, right, bottom, *this);
  if (renderStream) {
    _streamsMap.Insert(streamId, renderStream);
  }
  else {
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "(%s:%d): renderStream is NULL", __FUNCTION__, __LINE__);
    return NULL;
  }
  return renderStream;
}

WebRtc_Word32 VideoRenderBlackBerry::DeleteIncomingRenderStream(
    const WebRtc_UWord32 streamId) {
  CriticalSectionScoped cs(&_critSect);

  MapItem* item = _streamsMap.Find(streamId);
  if (item) {
    _streamsMap.Erase(streamId);
  }
  else {
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "(%s:%d): renderStream is NULL", __FUNCTION__, __LINE__);
    return -1;
  }
  return 0;
}

WebRtc_Word32 VideoRenderBlackBerry::GetIncomingRenderStreamProperties(
    const WebRtc_UWord32 streamId,
    WebRtc_UWord32& zOrder,
    float& left,
    float& top,
    float& right,
    float& bottom) const {
  return -1;
}

WebRtc_Word32 VideoRenderBlackBerry::StartRender() {
  CriticalSectionScoped cs(&_critSect);

  unsigned int tId = 0;
  return 0;
}

WebRtc_Word32 VideoRenderBlackBerry::StopRender() {
  WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s:", __FUNCTION__);
  {
    CriticalSectionScoped cs(&_critSect);
  }

  return 0;
}

void VideoRenderBlackBerry::ReDraw() {
  CriticalSectionScoped cs(&_critSect);
  // Allow redraw if it was more than 20ms since last.
}

VideoRenderType VideoRenderBlackBerry::RenderType() {
  return _renderType;
}

RawVideoType VideoRenderBlackBerry::PerferedVideoType() {
  return kVideoI420;
}

bool VideoRenderBlackBerry::FullScreen() {
  return false;
}

WebRtc_Word32 VideoRenderBlackBerry::GetGraphicsMemory(
    WebRtc_UWord64& /*totalGraphicsMemory*/,
    WebRtc_UWord64& /*availableGraphicsMemory*/) const {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

WebRtc_Word32 VideoRenderBlackBerry::GetScreenResolution(
    WebRtc_UWord32& /*screenWidth*/,
    WebRtc_UWord32& /*screenHeight*/) const {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

WebRtc_UWord32 VideoRenderBlackBerry::RenderFrameRate(
    const WebRtc_UWord32 /*streamId*/) {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

WebRtc_Word32 VideoRenderBlackBerry::SetStreamCropping(
    const WebRtc_UWord32 /*streamId*/,
    const float /*left*/,
    const float /*top*/,
    const float /*right*/,
    const float /*bottom*/) {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

WebRtc_Word32 VideoRenderBlackBerry::SetTransparentBackground(const bool enable) {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

WebRtc_Word32 VideoRenderBlackBerry::ConfigureRenderer(
    const WebRtc_UWord32 streamId,
    const unsigned int zOrder,
    const float left,
    const float top,
    const float right,
    const float bottom) {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

WebRtc_Word32 VideoRenderBlackBerry::SetText(
    const WebRtc_UWord8 textId,
    const WebRtc_UWord8* text,
    const WebRtc_Word32 textLength,
    const WebRtc_UWord32 textColorRef,
    const WebRtc_UWord32 backgroundColorRef,
    const float left, const float top,
    const float rigth, const float bottom) {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

WebRtc_Word32 VideoRenderBlackBerry::SetBitmap(const void* bitMap,
                                            const WebRtc_UWord8 pictureId,
                                            const void* colorKey,
                                            const float left, const float top,
                                            const float right,
                                            const float bottom) {
  WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
               "%s - not supported on Blackberry", __FUNCTION__);
  return -1;
}

}  // namespace webrtc
