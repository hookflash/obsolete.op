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

#include <openpeer/core/internal/types.h>
#include <openpeer/core/IMediaEngine.h>
#include <openpeer/core/internal/core_MediaStream.h>

#include <zsLib/MessageQueueAssociator.h>

#include <voe_base.h>
#include <voe_codec.h>
#include <voe_network.h>
#include <voe_rtp_rtcp.h>
#include <voe_audio_processing.h>
#include <voe_volume_control.h>
#include <voe_hardware.h>
#include <voe_file.h>

#include <vie_base.h>
#include <vie_network.h>
#include <vie_render.h>
#include <vie_capture.h>
#include <vie_codec.h>
#include <vie_rtp_rtcp.h>
#include <vie_file.h>

namespace openpeer
{
  namespace core
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaEngineForStack
      #pragma mark
      
      interaction IMediaEngineForStack
      {
        IMediaEngineForStack &forStack() {return *this;}
        const IMediaEngineForStack &forStack() const {return *this;}
        
        static void setup(IMediaEngineDelegatePtr delegate);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaEngineForCall
      #pragma mark
      
      interaction IMediaEngineForCall
      {
        IMediaEngineForCall &forCall() {return *this;}
        const IMediaEngineForCall &forCall() const {return *this;}
        
        static MediaEnginePtr singleton();
        
        virtual MediaSessionList getReceiveMediaSessions() = 0;
        virtual MediaSessionList getSendMediaSessions() = 0;
        virtual void addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams = true) = 0;
        virtual void removeMediaSession(IMediaSessionPtr session) = 0;
        
        virtual IMediaStreamForCallTransport::MediaConstraintList getVideoConstraints(ILocalSendVideoStreamForCall::CameraTypes cameraType) = 0;
        virtual IMediaStreamForCallTransport::MediaConstraintList getAudioConstraints() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaEngineForCallTransport
      #pragma mark
      
      interaction IMediaEngineForCallTransport
      {
        typedef webrtc::Transport Transport;

        IMediaEngineForCallTransport &forCallTransport() {return *this;}
        const IMediaEngineForCallTransport &forCallTransport() const {return *this;}
        
        static MediaEnginePtr singleton();
      };
      
      interaction IMediaEngineAsync
      {
        virtual void onMediaEngineSessionAdded(IMediaSessionPtr session) = 0;
        virtual void onMediaEngineSessionRemoved(IMediaSessionPtr session) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine
      #pragma mark
      
      class MediaEngine : public Noop,
                          public MessageQueueAssociator,
                          public IMediaEngine,
                          public IMediaEngineForStack,
                          public IMediaEngineForCallTransport,
                          public webrtc::TraceCallback,
                          public webrtc::VoiceEngineObserver,
                          public webrtc::ViECaptureObserver
      {
      public:
        friend interaction IMediaEngineFactory;
        friend interaction IMediaEngine;
        friend interaction IMediaEngineForStack;
        friend interaction IMediaEngineForCallTransport;
        
        typedef webrtc::Transport Transport;
        typedef webrtc::TraceLevel TraceLevel;
        typedef webrtc::VoiceEngine VoiceEngine;
        typedef webrtc::VoEBase VoiceBase;
        typedef webrtc::VoECodec VoiceCodec;
        typedef webrtc::VoENetwork VoiceNetwork;
        typedef webrtc::VoERTP_RTCP VoiceRtpRtcp;
        typedef webrtc::VoEAudioProcessing VoiceAudioProcessing;
        typedef webrtc::VoEVolumeControl VoiceVolumeControl;
        typedef webrtc::VoEHardware VoiceHardware;
        typedef webrtc::VoEFile VoiceFile;
        typedef webrtc::OutputAudioRoute OutputAudioRoute;
        typedef webrtc::EcModes EcModes;
        typedef webrtc::VideoCaptureModule VideoCaptureModule;
        typedef webrtc::VideoEngine VideoEngine;
        typedef webrtc::ViEBase VideoBase;
        typedef webrtc::ViENetwork VideoNetwork;
        typedef webrtc::ViERender VideoRender;
        typedef webrtc::ViECapture VideoCapture;
        typedef webrtc::ViERTP_RTCP VideoRtpRtcp;
        typedef webrtc::ViECodec VideoCodec;
        typedef webrtc::ViEFile VideoFile;
        
      protected:
        
        MediaEngine(
                    IMessageQueuePtr queue,
                    IMediaEngineDelegatePtr delegate
                    );
        
        MediaEngine(Noop);
        
        void init();
        
        static MediaEnginePtr create(IMediaEngineDelegatePtr delegate);
        
        void destroyMediaEngine();
        virtual void setLogLevel();
        
      public:
        ~MediaEngine();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => IMediaEngine
        #pragma mark
        
        static MediaEnginePtr singleton(IMediaEngineDelegatePtr delegate = IMediaEngineDelegatePtr());
        
        virtual void setDefaultVideoOrientation(VideoOrientations orientation);
        virtual VideoOrientations getDefaultVideoOrientation();
        virtual void setRecordVideoOrientation(VideoOrientations orientation);
        virtual VideoOrientations getRecordVideoOrientation();
        virtual void setVideoOrientation();
        
        virtual void setMuteEnabled(bool enabled);
        virtual bool getMuteEnabled();
        virtual void setLoudspeakerEnabled(bool enabled);
        virtual bool getLoudspeakerEnabled();
        virtual OutputAudioRoutes getOutputAudioRoute();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => IMediaEngineForStack
        #pragma mark
        
        static void setup(IMediaEngineDelegateObsoletePtr delegate);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => IMediaEngineForCall
        #pragma mark
        
        virtual MediaSessionList getReceiveMediaSessions();
        virtual MediaSessionList getSendMediaSessions();
        virtual void addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams = true);
        virtual void removeMediaSession(IMediaSessionPtr session);
        
        virtual IMediaStreamForCallTransport::MediaConstraintList getVideoConstraints(ILocalSendVideoStreamForCall::CameraTypes cameraType);
        virtual IMediaStreamForCallTransport::MediaConstraintList getAudioConstraints();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => IMediaEngineForCallTransport
        #pragma mark

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => TraceCallback
        #pragma mark
        
        virtual void Print(const TraceLevel level, const char *traceString, const int length);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => VoiceEngineObserver
        #pragma mark
        
        void CallbackOnError(const int errCode, const int channel);
        void CallbackOnOutputAudioRouteChange(const OutputAudioRoute route);
        
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => ViECaptureObserver
        #pragma mark
        
        void BrightnessAlarm(const int capture_id, const webrtc::Brightness brightness);
        void CapturedFrameRate(const int capture_id, const unsigned char frame_rate);
        void NoPictureAlarm(const int capture_id, const webrtc::CaptureAlarm alarm);
        void FaceDetected(const int capture_id);
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::internal::IMediaEngineAsync)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::core::internal::IMediaSessionPtr, IMediaSessionPtr)
ZS_DECLARE_PROXY_METHOD_1(onMediaEngineSessionAdded, IMediaSessionPtr)
ZS_DECLARE_PROXY_METHOD_1(onMediaEngineSessionRemoved, IMediaSessionPtr)
ZS_DECLARE_PROXY_END()
