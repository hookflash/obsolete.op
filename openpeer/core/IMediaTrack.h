/*
 
 Copyright (c) 2013, SMB Phone Inc.
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

#include <openpeer/core/types.h>

namespace openpeer
{
  namespace core
  {
    interaction IMediaTrack
    {
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
    };
    
    interaction IAudioTrack : public virtual IMediaTrack
    {
    };
    
    interaction IVideoTrack : public virtual IMediaTrack
    {
      virtual void setRenderView(void *renderView) = 0;
    };
    
    interaction ILocalAudioTrack : public virtual IAudioTrack
    {
      virtual void start() = 0;
      virtual void stop() = 0;
    };
    
    interaction IRemoteAudioTrack : public virtual IAudioTrack
    {
      virtual void setEcEnabled(bool enabled) = 0;
      virtual void setAgcEnabled(bool enabled) = 0;
      virtual void setNsEnabled(bool enabled) = 0;
      virtual void setVoiceRecordFile(String fileName) = 0;
      virtual String getVoiceRecordFile() const = 0;
      
      virtual int getTransportStatistics(RtpRtcpStatistics &stat) = 0;
    };

    interaction ILocalVideoTrack : public virtual IVideoTrack
    {
      enum CameraTypes
      {
        CameraType_None,
        CameraType_Front,
        CameraType_Back
      };
      
      static const char *toString(CameraTypes type);
      
      virtual void setContinuousVideoCapture(bool continuousVideoCapture) = 0;
      virtual bool getContinuousVideoCapture() = 0;
      
      virtual void setFaceDetection(bool faceDetection) = 0;
      virtual bool getFaceDetection() = 0;
      
      virtual CameraTypes getCameraType() const = 0;
      virtual void setCameraType(CameraTypes type) = 0;
      
      virtual void start() = 0;
      virtual void stop() = 0;
      
      virtual void startRecord(String fileName, bool saveToLibrary = false) = 0;
      virtual void stopRecord() = 0;
      
    };
    
    interaction IRemoteVideoTrack : public virtual IVideoTrack
    {
      virtual int getTransportStatistics(RtpRtcpStatistics &stat) = 0;
    };

    interaction IMediaTrackDelegate
    {
      virtual void onMediaTrackFaceDetected() = 0;
      virtual void onMediaTrackVideoCaptureRecordStopped() = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::IMediaTrackDelegate)
ZS_DECLARE_PROXY_METHOD_0(onMediaTrackFaceDetected)
ZS_DECLARE_PROXY_METHOD_0(onMediaTrackVideoCaptureRecordStopped)
ZS_DECLARE_PROXY_END()
