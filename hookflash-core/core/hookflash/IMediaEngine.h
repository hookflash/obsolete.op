/*
 
 Copyright (c) 2012, SMB Phone Inc.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

#pragma once

#include <hookflash/hookflashTypes.h>

namespace hookflash
{
  interaction IMediaEngine
  {
  public:
    
    typedef zsLib::String String;

    enum CameraTypes
    {
      CameraType_None,
      CameraType_Front,
      CameraType_Back
    };

    static const char *toString(CameraTypes type);

    enum VideoOrientations
    {
      VideoOrientation_LandscapeLeft,
      VideoOrientation_PortraitUpsideDown,
      VideoOrientation_LandscapeRight,
      VideoOrientation_Portrait
    };

    static const char *toString(VideoOrientations orientation);

    enum OutputAudioRoutes
    {
      OutputAudioRoute_Headphone,
      OutputAudioRoute_BuiltInReceiver,
      OutputAudioRoute_BuiltInSpeaker
    };
    
    static const char *toString(OutputAudioRoutes route);

    struct RtpRtcpStatistics
    {
      unsigned short fractionLost;
      unsigned int cumulativeLost;
      unsigned int extendedMax;
      unsigned int jitter;
      int rttMs;
      int bytesSent;
      int packetsSent;
      int bytesReceived;
      int packetsReceived;
    };

    static IMediaEnginePtr singleton();

    virtual void setDefaultVideoOrientation(VideoOrientations orientation) = 0;
    virtual VideoOrientations getDefaultVideoOrientation() = 0;
    virtual void setRecordVideoOrientation(VideoOrientations orientation) = 0;
    virtual VideoOrientations getRecordVideoOrientation() = 0;
    virtual void setVideoOrientation() = 0;
    
    virtual void setCaptureRenderView(void *renderView) = 0;
    virtual void setChannelRenderView(void *renderView) = 0;
    
    virtual void setEcEnabled(bool enabled) = 0;
    virtual void setAgcEnabled(bool enabled) = 0;
    virtual void setNsEnabled(bool enabled) = 0;
    virtual void setVoiceRecordFile(String fileName) = 0;
    virtual String getVoiceRecordFile() const = 0;
    
    virtual void setMuteEnabled(bool enabled) = 0;
    virtual bool getMuteEnabled() = 0;
    virtual void setLoudspeakerEnabled(bool enabled) = 0;
    virtual bool getLoudspeakerEnabled() = 0;
    virtual OutputAudioRoutes getOutputAudioRoute() = 0;
    
    virtual void setContinuousVideoCapture(bool continuousVideoCapture) = 0;
    virtual bool getContinuousVideoCapture() = 0;
    
    virtual void setFaceDetection(bool faceDetection) = 0;
    virtual bool getFaceDetection() = 0;

    virtual CameraTypes getCameraType() const = 0;
    virtual void setCameraType(CameraTypes type) = 0;
      
    virtual void startVideoCapture() = 0;
    virtual void stopVideoCapture() = 0;
    
    virtual void startRecordVideoCapture(String fileName) = 0;
    virtual void stopRecordVideoCapture() = 0;

    virtual int getVideoTransportStatistics(RtpRtcpStatistics &stat) = 0;
    virtual int getVoiceTransportStatistics(RtpRtcpStatistics &stat) = 0;
  };

  interaction IMediaEngineDelegate
  {
    typedef IMediaEngine::OutputAudioRoutes OutputAudioRoutes;

    virtual void onMediaEngineAudioRouteChanged(OutputAudioRoutes audioRoute) = 0;
    virtual void onMediaEngineFaceDetected() = 0;
  };
}

ZS_DECLARE_PROXY_BEGIN(hookflash::IMediaEngineDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::IMediaEngine::OutputAudioRoutes, OutputAudioRoutes)
ZS_DECLARE_PROXY_METHOD_1(onMediaEngineAudioRouteChanged, OutputAudioRoutes)
ZS_DECLARE_PROXY_METHOD_0(onMediaEngineFaceDetected)
ZS_DECLARE_PROXY_END()
