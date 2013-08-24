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
#include <openpeer/core/IMediaEngineObsolete.h>

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
      #pragma mark IMediaEngineForStackObsolete
      #pragma mark

      interaction IMediaEngineForStackObsolete
      {
        IMediaEngineForStackObsolete &forStack() {return *this;}
        const IMediaEngineForStackObsolete &forStack() const {return *this;}

        static void setup(IMediaEngineDelegateObsoletePtr delegate);
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaEngineForCallTransportObsolete
      #pragma mark

      interaction IMediaEngineForCallTransportObsolete
      {
        typedef webrtc::Transport Transport;

        IMediaEngineForCallTransportObsolete &forCallTransport() {return *this;}
        const IMediaEngineForCallTransportObsolete &forCallTransport() const {return *this;}

        static MediaEngineObsoletePtr singleton();

        virtual void startVoice() = 0;
        virtual void stopVoice() = 0;

        virtual int registerVoiceExternalTransport(Transport &transport) = 0;
        virtual int deregisterVoiceExternalTransport() = 0;
        virtual int receivedVoiceRTPPacket(const void *data, unsigned int length) = 0;
        virtual int receivedVoiceRTCPPacket(const void *data, unsigned int length) = 0;
        
        virtual void startVideoChannel() = 0;
        virtual void stopVideoChannel() = 0;

        virtual int registerVideoExternalTransport(Transport &transport) = 0;
        virtual int deregisterVideoExternalTransport() = 0;
        virtual int receivedVideoRTPPacket(const void *data, const int length) = 0;
        virtual int receivedVideoRTCPPacket(const void *data, const int length) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngineObsolete
      #pragma mark

      class MediaEngineObsolete : public Noop,
                          public MessageQueueAssociator,
                          public IMediaEngineObsolete,
                          public IMediaEngineForStackObsolete,
                          public IMediaEngineForCallTransportObsolete,
                          public webrtc::TraceCallback,
                          public webrtc::VoiceEngineObserver,
                          public webrtc::ViECaptureObserver
      {
      public:
        friend interaction IMediaEngineFactoryObsolete;
        friend interaction IMediaEngineObsolete;
        friend interaction IMediaEngineForStackObsolete;
        friend interaction IMediaEngineForCallTransportObsolete;

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

        MediaEngineObsolete(
                    IMessageQueuePtr queue,
                    IMediaEngineDelegateObsoletePtr delegate
                    );
        
        MediaEngineObsolete(Noop);

        void init();

        static MediaEngineObsoletePtr create(IMediaEngineDelegateObsoletePtr delegate);
        
        void destroyMediaEngine();
        virtual void setLogLevel();

      public:
        ~MediaEngineObsolete();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => IMediaEngine
        #pragma mark

        static MediaEngineObsoletePtr singleton(IMediaEngineDelegateObsoletePtr delegate = IMediaEngineDelegateObsoletePtr());

        virtual void setDefaultVideoOrientation(VideoOrientations orientation);
        virtual VideoOrientations getDefaultVideoOrientation();
        virtual void setRecordVideoOrientation(VideoOrientations orientation);
        virtual VideoOrientations getRecordVideoOrientation();
        virtual void setVideoOrientation();

        virtual void setCaptureRenderView(void *renderView);
        virtual void setChannelRenderView(void *renderView);

        virtual void setEcEnabled(bool enabled);
        virtual void setAgcEnabled(bool enabled);
        virtual void setNsEnabled(bool enabled);
        virtual void setVoiceRecordFile(String fileName);
        virtual String getVoiceRecordFile() const;

        virtual void setMuteEnabled(bool enabled);
        virtual bool getMuteEnabled();
        virtual void setLoudspeakerEnabled(bool enabled);
        virtual bool getLoudspeakerEnabled();
        virtual OutputAudioRoutes getOutputAudioRoute();
        
        virtual void setContinuousVideoCapture(bool continuousVideoCapture);
        virtual bool getContinuousVideoCapture();
        
        virtual void setFaceDetection(bool faceDetection);
        virtual bool getFaceDetection();

        virtual CameraTypes getCameraType() const;
        virtual void setCameraType(CameraTypes type);
        
        virtual void startVideoCapture();
        virtual void stopVideoCapture();
        
        virtual void startRecordVideoCapture(String fileName, bool saveToLibrary = false);
        virtual void stopRecordVideoCapture();

        virtual int getVideoTransportStatistics(RtpRtcpStatistics &stat);
        virtual int getVoiceTransportStatistics(RtpRtcpStatistics &stat);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => IMediaEngineForStack
        #pragma mark

        static void setup(IMediaEngineDelegateObsoletePtr delegate);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => IMediaEngineForCallTransport
        #pragma mark

        virtual void startVoice();
        virtual void stopVoice();
        
        virtual void startVideoChannel();
        virtual void stopVideoChannel();

        virtual int registerVoiceExternalTransport(Transport &transport);
        virtual int deregisterVoiceExternalTransport();
        virtual int receivedVoiceRTPPacket(const void *data, unsigned int length);
        virtual int receivedVoiceRTCPPacket(const void *data, unsigned int length);

        virtual int registerVideoExternalTransport(Transport &transport);
        virtual int deregisterVideoExternalTransport();
        virtual int receivedVideoRTPPacket(const void *data, const int length);
        virtual int receivedVideoRTCPPacket(const void *data, const int length);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => TraceCallback
        #pragma mark

        virtual void Print(const TraceLevel level, const char *traceString, const int length);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => VoiceEngineObserver
        #pragma mark

        void CallbackOnError(const int errCode, const int channel);
        void CallbackOnOutputAudioRouteChange(const OutputAudioRoute route);
        
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => ViECaptureObserver
        #pragma mark
        
        void BrightnessAlarm(const int capture_id, const webrtc::Brightness brightness);
        void CapturedFrameRate(const int capture_id, const unsigned char frame_rate);
        void NoPictureAlarm(const int capture_id, const webrtc::CaptureAlarm alarm);
        void FaceDetected(const int capture_id);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => (internal)
        #pragma mark

      public:
        void operator()();

      protected:
        virtual void internalStartVoice();
        virtual void internalStopVoice();
        
        virtual int registerVoiceTransport();
        virtual int setVoiceTransportParameters();

        virtual void internalStartVideoCapture();
        virtual void internalStopVideoCapture();
        virtual void internalStartVideoChannel();
        virtual void internalStopVideoChannel();
        virtual void internalStartRecordVideoCapture(String videoRecordFile, bool saveVideoToLibrary);
        virtual void internalStopRecordVideoCapture();

        virtual int registerVideoTransport();
        virtual int deregisterVideoTransport();
        virtual int setVideoTransportParameters();

      protected:
        int getVideoCaptureParameters(webrtc::RotateCapturedFrame orientation, int& width, int& height,
                                      int& maxFramerate, int& maxBitrate);
        int setVideoCodecParameters();
        int setVideoCaptureRotation();
        EcModes getEcMode();

      private:
        String log(const char *message) const;

      protected:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete::RedirectTransport
        #pragma mark

        class RedirectTransport : public Transport
        {
        public:
          RedirectTransport(const char *transportType);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MediaEngineObsolete::RedirectTransport => webrtc::Transport
          #pragma mark

          virtual int SendPacket(int channel, const void *data, int len);
          virtual int SendRTCPPacket(int channel, const void *data, int len);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MediaEngineObsolete::RedirectTransport => friend MediaEngineObsolete
          #pragma mark

          void redirect(Transport *transport);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MediaEngineObsolete::RedirectTransport => (internal)
          #pragma mark

          String log(const char *message);

        private:
          PUID mID;
          mutable RecursiveLock mLock;

          const char *mTransportType;

          Transport *mTransport;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngineObsolete => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        MediaEngineObsoleteWeakPtr mThisWeak;
        IMediaEngineDelegateObsoletePtr mDelegate;

        int mError;
        unsigned int mMtu;
        String mMachineName;

        bool mEcEnabled;
        bool mAgcEnabled;
        bool mNsEnabled;
        String mVoiceRecordFile;
        VideoOrientations mDefaultVideoOrientation;
        VideoOrientations mRecordVideoOrientation;

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
        bool mFaceDetection;

        int mVideoChannel;
        Transport *mVideoTransport;
        int mCaptureId;
        char mDeviceUniqueId[512];
        CameraTypes mCameraType;
        VideoCaptureModule *mVcpm;
        VideoEngine *mVideoEngine;
        VideoBase *mVideoBase;
        VideoNetwork *mVideoNetwork;
        VideoRender *mVideoRender;
        VideoCapture *mVideoCapture;
        VideoRtpRtcp *mVideoRtpRtcp;
        VideoCodec *mVideoCodec;
        VideoFile *mVideoFile;
        void *mCaptureRenderView;
        void *mChannelRenderView;
        bool mVideoEngineReady;

        RedirectTransport mRedirectVoiceTransport;
        RedirectTransport mRedirectVideoTransport;

        // lifetime start / stop state
        mutable RecursiveLock mLifetimeLock;

        bool mLifetimeWantAudio;
        bool mLifetimeWantVideoCapture;
        bool mLifetimeWantVideoChannel;
        bool mLifetimeWantRecordVideoCapture;

        bool mLifetimeHasAudio;
        bool mLifetimeHasVideoCapture;
        bool mLifetimeHasVideoChannel;
        bool mLifetimeHasRecordVideoCapture;

        bool mLifetimeInProgress;
        CameraTypes mLifetimeWantCameraType;
        bool mLifetimeContinuousVideoCapture;
        
        String mLifetimeVideoRecordFile;
        bool mLifetimeSaveVideoToLibrary;

        mutable RecursiveLock mMediaEngineReadyLock;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaEngineFactory
      #pragma mark
      
      interaction IMediaEngineFactoryObsolete
      {
        static IMediaEngineFactoryObsolete &singleton();
        
        virtual MediaEngineObsoletePtr createMediaEngine(IMediaEngineDelegateObsoletePtr delegate);
      };
    }
  }
}
