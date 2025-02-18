/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_BASE_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_BASE_IMPL_H_

#include "vie_base.h"
#include "vie_defines.h"
#include "vie_ref_count.h"
#include "vie_shared_data.h"

namespace webrtc {

class Module;
class VoiceEngine;

class ViEBaseImpl
    : public ViEBase,
      public ViERefCount {
 public:
  virtual int Release();

  // Implements ViEBase.
  virtual int Init();
  virtual int SetVoiceEngine(VoiceEngine* voice_engine);
  virtual int CreateChannel(int& video_channel);
  virtual int CreateChannel(int& video_channel, int original_channel);
  virtual int CreateReceiveChannel(int& video_channel, int original_channel);
  virtual int DeleteChannel(const int video_channel);
  virtual int ConnectAudioChannel(const int video_channel,
                                  const int audio_channel);
  virtual int DisconnectAudioChannel(const int video_channel);
  virtual int StartSend(const int video_channel);
  virtual int StopSend(const int video_channel);
  virtual int StartReceive(const int video_channel);
  virtual int StopReceive(const int video_channel);
  virtual int RegisterObserver(ViEBaseObserver& observer);
  virtual int DeregisterObserver();
  virtual int GetVersion(char version[1024]);
  virtual int LastError();
  virtual int GetAverageSystemCPU(int& systemCPU);

 protected:
  ViEBaseImpl();
  virtual ~ViEBaseImpl();

  ViESharedData* shared_data() { return &shared_data_; }

 private:
  // Version functions.
  WebRtc_Word32 AddViEVersion(char* str) const;
  WebRtc_Word32 AddBuildInfo(char* str) const;
  WebRtc_Word32 AddExternalTransportBuild(char* str) const;

  int CreateChannel(int& video_channel, int original_channel, bool sender);

  // ViEBaseImpl owns ViESharedData used by all interface implementations.
  ViESharedData shared_data_;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_BASE_IMPL_H_
