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

#include <hookflash/IMediaEngine.h>
#include <hookflash/internal/hookflashTypes.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

#include <voe_base.h>
#include <vie_base.h>
#include <voe_codec.h>
#include <voe_network.h>
#include <vie_network.h>
#include <vie_render.h>
#include <voe_rtp_rtcp.h>
#include <voe_audio_processing.h>
#include <voe_volume_control.h>
#include <voe_hardware.h>
#include <voe_file.h>

#include <vie_capture.h>
#include <vie_codec.h>
#include <vie_rtp_rtcp.h>
#include <video_capture.h>
#include <video_capture_factory.h>

namespace hookflash
{
  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMediaEngineForStack
    #pragma mark

    interaction IMediaEngineForStack
    {
      static IMediaEngineForStackPtr convert(IMediaEnginePtr engine);

      static void setup(IMediaEngineDelegatePtr delegate);
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMediaEngineForCallTransport
    #pragma mark

    interaction IMediaEngineForCallTransport
    {
      typedef webrtc::Transport Transport;

      static IMediaEngineForCallTransportPtr convert(IMediaEnginePtr engine);

      static IMediaEngineForCallTransportPtr singleton();

      virtual void startVoice() = 0;
      virtual void stopVoice() = 0;
      
      virtual int registerVoiceExternalTransport(Transport &transport) = 0;
      virtual int deregisterVoiceExternalTransport() = 0;
      virtual int receivedVoiceRTPPacket(const void *data, unsigned int length) = 0;
      virtual int receivedVoiceRTCPPacket(const void *data, unsigned int length) = 0;

      virtual void startVideo() = 0;
      virtual void stopVideo() = 0;

      virtual int registerVideoExternalTransport(Transport &transport) = 0;
      virtual int deregisterVideoExternalTransport() = 0;
      virtual int receivedVideoRTPPacket(const void *data, const int length) = 0;
      virtual int receivedVideoRTCPPacket(const void *data, const int length) = 0;
    };
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMediaEngineForTestApplication
    #pragma mark
    
    interaction IMediaEngineForTestApplication
    {
      typedef zsLib::String String;
      
      static IMediaEngineForTestApplicationPtr convert(IMediaEnginePtr engine);
      
      static IMediaEngineForTestApplicationPtr singleton();
      
      static void setup(IMediaEngineDelegatePtr delegate);
      
      virtual void setVideoOrientation() = 0;
      
      virtual void setCaptureRenderView(void *renderView) = 0;
      virtual void setChannelRenderView(void *renderView) = 0;
      
      virtual void setEcEnabled(bool enabled) = 0;
      virtual void setAgcEnabled(bool enabled) = 0;
      virtual void setNsEnabled(bool enabled) = 0;
      virtual void setRecordFile(String fileName) = 0;
      virtual String getRecordFile() const = 0;
      
      virtual void setMuteEnabled(bool enabled) = 0;
      virtual bool getMuteEnabled() = 0;
      virtual void setLoudspeakerEnabled(bool enabled) = 0;
      virtual bool getLoudspeakerEnabled() = 0;

      virtual void setReceiverAddress(String receiverAddress) = 0;
      virtual String getReceiverAddress() const = 0;

      virtual void startVoice() = 0;
      virtual void stopVoice() = 0;
      
      virtual void startVideo() = 0;
      virtual void stopVideo() = 0;
    };
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark MediaEngine
    #pragma mark

    class MediaEngine : public zsLib::MessageQueueAssociator,
                        public IMediaEngine,
                        public IMediaEngineForStack,
                        public IMediaEngineForCallTransport,
                        public IMediaEngineForTestApplication,
                        public zsLib::ITimerDelegate,
                        public webrtc::TraceCallback,
                        public webrtc::VoiceEngineObserver
    {
    public:
      
      typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
      typedef zsLib::String String;
      typedef zsLib::PUID PUID;
      typedef zsLib::TimerPtr TimerPtr;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::IMessageQueuePtr IMessageQueuePtr;

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
      
    protected:
      
      MediaEngine(
                  IMessageQueuePtr queue,
                  IMediaEngineDelegatePtr delegate
                  );

      void init();
      
      static MediaEnginePtr create(IMediaEngineDelegatePtr delegate);

    public:
      ~MediaEngine();
      
    public:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => IMediaEngine
      #pragma mark

      static MediaEnginePtr singleton(IMediaEngineDelegatePtr delegate = IMediaEngineDelegatePtr());

      virtual void setVideoOrientation();

      virtual void setCaptureRenderView(void *renderView);
      virtual void setChannelRenderView(void *renderView);

      virtual void setEcEnabled(bool enabled);
      virtual void setAgcEnabled(bool enabled);
      virtual void setNsEnabled(bool enabled);
      virtual void setRecordFile(String fileName);
      virtual String getRecordFile() const;
      
      virtual void setMuteEnabled(bool enabled);
      virtual bool getMuteEnabled();
      virtual void setLoudspeakerEnabled(bool enabled);
      virtual bool getLoudspeakerEnabled();
      virtual OutputAudioRoutes getOutputAudioRoute();
      
      virtual CameraTypes getCameraType() const;
      virtual void setCameraType(CameraTypes type);

      virtual int getVideoTransportStatistics(RtpRtcpStatistics &stat);
      virtual int getVoiceTransportStatistics(RtpRtcpStatistics &stat);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => IMediaEngineForStack
      #pragma mark

      static void setup(IMediaEngineDelegatePtr delegate);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => IMediaEngineForCallTransport
      #pragma mark

      virtual void startVoice();
      virtual void stopVoice();

      virtual void startVideo();
      virtual void stopVideo();

      virtual int registerVoiceExternalTransport(Transport &transport);
      virtual int deregisterVoiceExternalTransport();
      virtual int receivedVoiceRTPPacket(const void *data, unsigned int length);
      virtual int receivedVoiceRTCPPacket(const void *data, unsigned int length);

      virtual int registerVideoExternalTransport(Transport &transport);
      virtual int deregisterVideoExternalTransport();
      virtual int receivedVideoRTPPacket(const void *data, const int length);
      virtual int receivedVideoRTCPPacket(const void *data, const int length);
      
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => IMediaEngineForTestApplication
      #pragma mark
      
      virtual void setReceiverAddress(String receiverAddress);
      virtual String getReceiverAddress() const;


      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => ITimerDelegate
      #pragma mark
      
      void onTimer(TimerPtr timer);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => TraceCallback
      #pragma mark
      
      void Print(const TraceLevel level, const char *traceString, const int length);
      
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => VoiceEngineObserver
      #pragma mark

      void CallbackOnError(const int errCode, const int channel);
      void CallbackOnOutputAudioRouteChange(const OutputAudioRoute route);
      
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => (internal)
      #pragma mark

    public:
      void operator()();

    protected:
      virtual void internalStartVoice();
      virtual void internalStopVoice();

      virtual void internalStartVideo(CameraTypes cameraType);
      virtual void internalStopVideo();

    protected:
      int setVideoCaptureRotationAndCodecParameters();
      EcModes getEcMode();

    private:
      String log(const char *message) const;

    protected:
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine::RedirectTransport
      #pragma mark

      class RedirectTransport : public Transport
      {
      public:
        RedirectTransport(const char *transportType);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine::RedirectTransport => webrtc::Transport
        #pragma mark

        virtual int SendPacket(int channel, const void *data, int len);
        virtual int SendRTCPPacket(int channel, const void *data, int len);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine::RedirectTransport => friend MediaEngine
        #pragma mark

        void redirect(Transport *transport);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine::RedirectTransport => (internal)
        #pragma mark
        
        String log(const char *message);

      private:
        PUID mID;
        mutable RecursiveLock mLock;

        const char *mTransportType;

        Transport *mTransport;
      };

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => (data)
      #pragma mark

      PUID mID;
      mutable RecursiveLock mLock;
      MediaEngineWeakPtr mThisWeak;
      IMediaEngineDelegatePtr mDelegate;

      int mError;
      unsigned int mMtu;
      String mMachineName;

      bool mEcEnabled;
      bool mAgcEnabled;
      bool mNsEnabled;
      String mRecordFile;
      String mReceiverAddress;

      int mVoiceChannel;
      Transport *mVoiceTransport;
      VoiceEngine *mVoiceEngine;
      VoiceBase *mVoiceBase;
      VoiceCodec *mVoiceCodec;
      VoiceNetwork *mVoiceNetwork;
      VoiceRtpRtcp *mVoiceRtpRtcp;
      VoiceAudioProcessing *mVoiceAudioProcessing;
      VoiceVolumeControl *mVoiceVolumeControl;
      VoiceHardware *mVoiceHardware;
      VoiceFile *mVoiceFile;
      bool mVoiceEngineReady;

      int mVideoChannel;
      Transport *mVideoTransport;
      int mCaptureId;
      CameraTypes mCameraType;
      VideoCaptureModule *mVcpm;
      VideoEngine *mVideoEngine;
      VideoBase *mVideoBase;
      VideoNetwork *mVideoNetwork;
      VideoRender *mVideoRender;
      VideoCapture *mVideoCapture;
      VideoRtpRtcp *mVideoRtpRtcp;
      VideoCodec *mVideoCodec;
      void *mIPhoneCaptureRenderView;
      void *mIPhoneChannelRenderView;
      bool mVideoEngineReady;

      RedirectTransport mRedirectVoiceTransport;
      RedirectTransport mRedirectVideoTransport;

      // lifetime start / stop state
      mutable RecursiveLock mLifetimeLock;

      bool mLifetimeWantAudio;
      bool mLifetimeWantVideo;

      bool mLifetimeHasAudio;
      bool mLifetimeHasVideo;

      bool mLifetimeInProgress;
      CameraTypes mLifetimeWantCameraType;

      mutable RecursiveLock mMediaEngineReadyLock;

    private:
      TimerPtr mVoiceStatisticsTimer;
    };
  }
}
