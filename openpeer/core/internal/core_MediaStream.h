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

#include <common_types.h>

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
      #pragma mark IMediaTransport
      #pragma mark
  
      interaction IMediaTransport
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

        virtual int getTransportStatistics(RtpRtcpStatistics &stat) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IReceiveMediaTransportForCallTransport
      #pragma mark
      
      interaction IReceiveMediaTransportForCallTransport
      {
        IReceiveMediaTransportForCallTransport &forCallTransport() {return *this;}
        const IReceiveMediaTransportForCallTransport &forCallTransport() const {return *this;}
        
        static ReceiveMediaTransportPtr create();

        virtual int receivedRTPPacket(const void *data, unsigned int length) = 0;
        virtual int receivedRTCPPacket(const void *data, unsigned int length) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISendMediaTransportForCallTransport
      #pragma mark
      
      interaction ISendMediaTransportForCallTransport
      {
        typedef webrtc::Transport Transport;
        
        ISendMediaTransportForCallTransport &forCallTransport() {return *this;}
        const ISendMediaTransportForCallTransport &forCallTransport() const {return *this;}
        
        static SendMediaTransportPtr create();

        virtual int registerExternalTransport(Transport &transport) = 0;
        virtual int deregisterExternalTransport() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaTransport
      #pragma mark
      
      class MediaTransport
      {
      public:
        friend interaction IMediaTransport;
        
      protected:
        MediaTransport();
        
      public:
        virtual ~MediaTransport();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaTransport => IMediaTransport
        #pragma mark
        
        virtual int getTransportStatistics(IMediaTransport::RtpRtcpStatistics &stat);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaTransport => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaTransport => (data)
        #pragma mark
        
      protected:
        mutable RecursiveLock mLock;
        MediaTransportWeakPtr mThisWeak;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ReceiveMediaTransport
      #pragma mark
      
      class ReceiveMediaTransport : public MediaTransport,
                                    public IMediaTransport,
                                    public IReceiveMediaTransportForCallTransport
      {
        
      public:
        friend interaction IReceiveMediaTransportForCallTransport;
        
      protected:
        ReceiveMediaTransport();
        
      public:
        virtual ~ReceiveMediaTransport();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ReceiveMediaTransport => IMediaTransport
        #pragma mark
        
        virtual int getTransportStatistics(IMediaTransport::RtpRtcpStatistics &stat);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ReceiveMediaTransport => IReceiveMediaTransportForCallTransport
        #pragma mark
        
        virtual int receivedRTPPacket(const void *data, unsigned int length);
        virtual int receivedRTCPPacket(const void *data, unsigned int length);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ReceiveMediaTransport => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ReceiveMediaTransport => (data)
        #pragma mark
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SendMediaTransport
      #pragma mark
      
      class SendMediaTransport : public MediaTransport,
                                 public IMediaTransport,
                                 public ISendMediaTransportForCallTransport
      {
        
      public:
        friend interaction ISendMediaTransportForCallTransport;
        
      protected:
        SendMediaTransport();
        
      public:
        virtual ~SendMediaTransport();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark SendMediaTransport => IMediaTransport
        #pragma mark
        
        virtual int getTransportStatistics(IMediaTransport::RtpRtcpStatistics &stat);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark SendMediaTransport => ISendMediaTransportForCallTransport
        #pragma mark
        
        virtual int registerExternalTransport(Transport &transport);
        virtual int deregisterExternalTransport();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark SendMediaTransport => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark SendMediaTransport => (data)
        #pragma mark
        
      protected:

      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaStream
      #pragma mark
      
      interaction IMediaStream
      {
        struct MediaConstraint;
        typedef std::list<MediaConstraint> MediaConstraintList;
        
        struct MediaConstraint
        {
          String key;
          String value;
        };
        
        virtual ULONG getSSRC() = 0;
        virtual IMediaStreamPtr clone() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocalSendAudioStreamForCall
      #pragma mark

      interaction ILocalSendAudioStreamForCall
      {
      public:
        ILocalSendAudioStreamForCall &forCall() {return *this;}
        const ILocalSendAudioStreamForCall &forCall() const {return *this;}
        
        virtual void setMediaConstraints(IMediaStream::MediaConstraintList constraintList) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocalSendAudioStreamForCallTransport
      #pragma mark
      
      interaction ILocalSendAudioStreamForCallTransport
      {
      public:
        ILocalSendAudioStreamForCallTransport &forCallTransport() {return *this;}
        const ILocalSendAudioStreamForCallTransport &forCallTransport() const {return *this;}
        
        static LocalSendAudioStreamPtr create(
                                              IMessageQueuePtr queue,
                                              IMediaStreamDelegatePtr delegate
                                              );
        
        virtual void start() = 0;
        virtual void stop() = 0;
        
        virtual SendMediaTransportPtr getTransport() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteReceiveAudioStreamForCall
      #pragma mark

      interaction IRemoteReceiveAudioStreamForCall
      {
      public:
        IRemoteReceiveAudioStreamForCall &forCall() {return *this;}
        const IRemoteReceiveAudioStreamForCall &forCall() const {return *this;}

        virtual void setEcEnabled(bool enabled) = 0;
        virtual void setAgcEnabled(bool enabled) = 0;
        virtual void setNsEnabled(bool enabled) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteReceiveAudioStreamForCallTransport
      #pragma mark
      
      interaction IRemoteReceiveAudioStreamForCallTransport
      {
      public:
        IRemoteReceiveAudioStreamForCallTransport &forCallTransport() {return *this;}
        const IRemoteReceiveAudioStreamForCallTransport &forCallTransport() const {return *this;}
        
        static RemoteReceiveAudioStreamPtr create(
                                                  IMessageQueuePtr queue,
                                                  IMediaStreamDelegatePtr delegate
                                                  );
        
        virtual ReceiveMediaTransportPtr getTransport() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteSendAudioStreamForCall
      #pragma mark

      interaction IRemoteSendAudioStreamForCall
      {
      public:
        IRemoteSendAudioStreamForCall &forCall() {return *this;}
        const IRemoteSendAudioStreamForCall &forCall() const {return *this;}
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteSendAudioStreamForCallTransport
      #pragma mark
      
      interaction IRemoteSendAudioStreamForCallTransport
      {
      public:
        IRemoteSendAudioStreamForCallTransport &forCallTransport() {return *this;}
        const IRemoteSendAudioStreamForCallTransport &forCallTransport() const {return *this;}
        
        static RemoteSendAudioStreamPtr create(
                                               IMessageQueuePtr queue,
                                               IMediaStreamDelegatePtr delegate
                                               );
        
        virtual SendMediaTransportPtr getTransport() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocalSendVideoStreamForCall
      #pragma mark

      interaction ILocalSendVideoStreamForCall
      {
      public:
        enum CameraTypes
        {
          CameraType_None,
          CameraType_Front,
          CameraType_Back
        };
        
        static const char *toString(CameraTypes type);

        ILocalSendVideoStreamForCall &forCall() {return *this;}
        const ILocalSendVideoStreamForCall &forCall() const {return *this;}

        virtual void setContinuousVideoCapture(bool continuousVideoCapture) = 0;
        virtual bool getContinuousVideoCapture() = 0;
        
        virtual void setFaceDetection(bool faceDetection) = 0;
        virtual bool getFaceDetection() = 0;
        
        virtual CameraTypes getCameraType() const = 0;
        virtual void setCameraType(CameraTypes type) = 0;
        
        virtual void setRenderView(void *renderView) = 0;
        
        virtual void startRecord(String fileName, bool saveToLibrary = false) = 0;
        virtual void stopRecord() = 0;
        
        virtual void setMediaConstraints(IMediaStream::MediaConstraintList constraintList) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocalSendVideoStreamForCallTransport
      #pragma mark
      
      interaction ILocalSendVideoStreamForCallTransport
      {
      public:
        ILocalSendVideoStreamForCallTransport &forCallTransport() {return *this;}
        const ILocalSendVideoStreamForCallTransport &forCallTransport() const {return *this;}
        
        static LocalSendVideoStreamPtr create(
                                              IMessageQueuePtr queue,
                                              IMediaStreamDelegatePtr delegate
                                              );
        
        virtual void start() = 0;
        virtual void stop() = 0;
        
        virtual SendMediaTransportPtr getTransport() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteReceiveVideoStreamForCall
      #pragma mark

      interaction IRemoteReceiveVideoStreamForCall
      {
      public:
        IRemoteReceiveVideoStreamForCall &forCall() {return *this;}
        const IRemoteReceiveVideoStreamForCall &forCall() const {return *this;}

        virtual void setRenderView(void *renderView) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteReceiveVideoStreamForCallTransport
      #pragma mark
      
      interaction IRemoteReceiveVideoStreamForCallTransport
      {
      public:
        IRemoteReceiveVideoStreamForCallTransport &forCallTransport() {return *this;}
        const IRemoteReceiveVideoStreamForCallTransport &forCallTransport() const {return *this;}
        
        static RemoteReceiveVideoStreamPtr create(
                                                  IMessageQueuePtr queue,
                                                  IMediaStreamDelegatePtr delegate
                                                  );
        
        virtual ReceiveMediaTransportPtr getTransport() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteSendVideoStreamForCall
      #pragma mark

      interaction IRemoteSendVideoStreamForCall
      {
      public:
        IRemoteSendVideoStreamForCall &forCall() {return *this;}
        const IRemoteSendVideoStreamForCall &forCall() const {return *this;}

        virtual void setRenderView(void *renderView) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteSendVideoStreamForCallTransport
      #pragma mark
      
      interaction IRemoteSendVideoStreamForCallTransport
      {
      public:
        IRemoteSendVideoStreamForCallTransport &forCallTransport() {return *this;}
        const IRemoteSendVideoStreamForCallTransport &forCallTransport() const {return *this;}
        
        static RemoteSendVideoStreamPtr create(
                                               IMessageQueuePtr queue,
                                               IMediaStreamDelegatePtr delegate
                                               );
        
        virtual SendMediaTransportPtr getTransport() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaStreamDelegate
      #pragma mark
      
      interaction IMediaStreamDelegate
      {
        virtual void onMediaStreamFaceDetected() = 0;
        virtual void onMediaStreamVideoCaptureRecordStopped() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaStream
      #pragma mark
      
      class MediaStream : public MessageQueueAssociator
      {
      public:
        friend interaction IMediaStream;

      protected:
        MediaStream(
                    IMessageQueuePtr queue,
                    IMediaStreamDelegatePtr delegate
                    );
        
      public:
        virtual ~MediaStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaStream => IMediaStream
        #pragma mark
        
        virtual ULONG getSSRC();
        virtual IMediaStreamPtr clone();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaStream => IMediaStreamForCallTransport
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaStream => (internal)
        #pragma mark
        
      private:
        String log(const char *message) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaStream => (data)
        #pragma mark
        
      protected:
        PUID mID;
        mutable RecursiveLock mLock;
        MediaStreamWeakPtr mThisWeak;
        
        int mError;

        int mChannel;
        IMediaTransportPtr mTransport;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AudioStream
      #pragma mark
      
      class AudioStream : public MediaStream
      {
      public:
        friend interaction IMediaStream;
        
      protected:
        AudioStream(
                    IMessageQueuePtr queue,
                    IMediaStreamDelegatePtr delegate
                    );
        
      public:
        virtual ~AudioStream();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AudioStream => (internal)
        #pragma mark
                
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AudioStream => (data)
        #pragma mark
        
      protected:
        
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark VideoStream
      #pragma mark
      
      class VideoStream : public MediaStream
      {
      public:
        friend interaction IMediaStream;
        friend interaction ILocalSendVideoStreamForCall;
        friend interaction IRemoteReceiveVideoStreamForCall;
        friend interaction IRemoteSendVideoStreamForCall;

      protected:
        VideoStream(
                    IMessageQueuePtr queue,
                    IMediaStreamDelegatePtr delegate
                    );
        
      public:
        virtual ~VideoStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark VideoStream => ILocalSendVideoStreamForCall, IRemoteReceiveVideoStreamForCall, IRemoteSendVideoStreamForCall
        #pragma mark
        
        virtual void setRenderView(void *renderView);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark VideoStream => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark VideoStream => (data)
        #pragma mark
        
      protected:
        
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalSendAudioStream
      #pragma mark
      
      class LocalSendAudioStream : public AudioStream,
                                   public IMediaStream,
                                   public ILocalSendAudioStreamForCall,
                                   public ILocalSendAudioStreamForCallTransport
      {
      public:
        friend interaction ILocalSendAudioStreamForCall;
        friend interaction ILocalSendAudioStreamForCallTransport;
        
      protected:
        LocalSendAudioStream(
                             IMessageQueuePtr queue,
                             IMediaStreamDelegatePtr delegate
                             );
        
      public:
        virtual ~LocalSendAudioStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendAudioStream => IMediaStream
        #pragma mark
        
        virtual ULONG getSSRC();
        virtual IMediaStreamPtr clone();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendAudioStream => ILocalSendAudioStreamForCall
        #pragma mark
        
        virtual void setMediaConstraints(IMediaStream::MediaConstraintList constraintList);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendAudioStream => ILocalSendAudioStreamForCallTransport
        #pragma mark
        
        virtual void start();
        virtual void stop();
        virtual SendMediaTransportPtr getTransport();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendAudioStream => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendAudioStream => (data)
        #pragma mark
        
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteReceiveAudioStream
      #pragma mark
      
      class RemoteReceiveAudioStream : public AudioStream,
                                       public IMediaStream,
                                       public IRemoteReceiveAudioStreamForCall,
                                       public IRemoteReceiveAudioStreamForCallTransport
      {
      public:
        friend interaction IRemoteReceiveAudioStreamForCall;
        friend interaction IRemoteReceiveAudioStreamForCallTransport;
        
      protected:
        RemoteReceiveAudioStream(
                                 IMessageQueuePtr queue,
                                 IMediaStreamDelegatePtr delegate
                                 );
        
      public:
        virtual ~RemoteReceiveAudioStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveAudioStream => IMediaStream
        #pragma mark
        
        virtual ULONG getSSRC();
        virtual IMediaStreamPtr clone();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveAudioStream => IRemoteReceiveAudioStreamForCall
        #pragma mark
        
        virtual void setEcEnabled(bool enabled);
        virtual void setAgcEnabled(bool enabled);
        virtual void setNsEnabled(bool enabled);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveAudioStream => IRemoteReceiveAudioStreamForCallTransport
        #pragma mark
        
        virtual ReceiveMediaTransportPtr getTransport();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveAudioStream => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveAudioStream => (data)
        #pragma mark
        
      protected:
        
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteSendAudioStream
      #pragma mark
      
      class RemoteSendAudioStream : public AudioStream,
                                    public IMediaStream,
                                    public IRemoteSendAudioStreamForCall,
                                    public IRemoteSendAudioStreamForCallTransport
      {
      public:
        friend interaction IRemoteSendAudioStreamForCall;
        friend interaction IRemoteSendAudioStreamForCallTransport;
        
      protected:
        RemoteSendAudioStream(
                              IMessageQueuePtr queue,
                              IMediaStreamDelegatePtr delegate
                              );
        
      public:
        virtual ~RemoteSendAudioStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendAudioStream => IMediaStream
        #pragma mark
        
        virtual ULONG getSSRC();
        virtual IMediaStreamPtr clone();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendAudioStream => IRemoteSendAudioStreamForCall
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendAudioStream => IRemoteSendAudioStreamForCallTransport
        #pragma mark
        
        virtual SendMediaTransportPtr getTransport();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendAudioStream => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendAudioStream => (data)
        #pragma mark
        
        
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalSendVideoStream
      #pragma mark
      
      class LocalSendVideoStream : public VideoStream,
                                   public IMediaStream,
                                   public ILocalSendVideoStreamForCall,
                                   public ILocalSendVideoStreamForCallTransport
      {
      public:
        friend interaction ILocalSendVideoStreamForCall;
        friend interaction ILocalSendVideoStreamForCallTransport;

      protected:
        LocalSendVideoStream(
                             IMessageQueuePtr queue,
                             IMediaStreamDelegatePtr delegate
                             );
        
      public:
        virtual ~LocalSendVideoStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendVideoStream => IMediaStream
        #pragma mark
        
        virtual ULONG getSSRC();
        virtual IMediaStreamPtr clone();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendVideoStream => ILocalSendVideoStreamForCall
        #pragma mark
        
        virtual void setContinuousVideoCapture(bool continuousVideoCapture);
        virtual bool getContinuousVideoCapture();
        
        virtual void setFaceDetection(bool faceDetection);
        virtual bool getFaceDetection();
        
        virtual CameraTypes getCameraType() const;
        virtual void setCameraType(CameraTypes type);
        
        virtual void setRenderView(void *renderView);
        
        virtual void startRecord(String fileName, bool saveToLibrary = false);
        virtual void stopRecord();
        
        virtual void setMediaConstraints(IMediaStream::MediaConstraintList constraintList);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendVideoStream => ILocalSendVideoStreamForCallTransport
        #pragma mark
        
        virtual void start();
        virtual void stop();
        
        virtual SendMediaTransportPtr getTransport();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendVideoStream => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendVideoStream => (data)
        #pragma mark
        
      protected:

      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteReceiveVideoStream
      #pragma mark
      
      class RemoteReceiveVideoStream : public VideoStream,
                                       public IMediaStream,
                                       public IRemoteReceiveVideoStreamForCall,
                                       public IRemoteReceiveVideoStreamForCallTransport
      {
      public:
        friend interaction IRemoteReceiveVideoStreamForCall;
        friend interaction IRemoteReceiveVideoStreamForCallTransport;
        
      protected:
        RemoteReceiveVideoStream(
                                 IMessageQueuePtr queue,
                                 IMediaStreamDelegatePtr delegate
                                 );
        
      public:
        virtual ~RemoteReceiveVideoStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveVideoStream => IMediaStream
        #pragma mark
        
        virtual ULONG getSSRC();
        virtual IMediaStreamPtr clone();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveVideoStream => IRemoteReceiveVideoStreamForCall
        #pragma mark
        
        virtual void setRenderView(void *renderView);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveVideoStream => IRemoteReceiveVideoStreamForCallTransport
        #pragma mark
        
        virtual ReceiveMediaTransportPtr getTransport();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveVideoStream => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveVideoStream => (data)
        #pragma mark
        
        
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteSendVideoStream
      #pragma mark
      
      class RemoteSendVideoStream : public VideoStream,
                                    public IMediaStream,
                                    public IRemoteSendVideoStreamForCall,
                                    public IRemoteSendVideoStreamForCallTransport
      {
      public:
        friend interaction IRemoteSendVideoStreamForCall;
        friend interaction IRemoteSendVideoStreamForCallTransport;
        
      protected:
        RemoteSendVideoStream(
                              IMessageQueuePtr queue,
                              IMediaStreamDelegatePtr delegate
                              );
        
      public:
        virtual ~RemoteSendVideoStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendVideoStream => IMediaStream
        #pragma mark
        
        virtual ULONG getSSRC();
        virtual IMediaStreamPtr clone();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendVideoStream => IRemoteSendVideoStreamForCall
        #pragma mark
        
        virtual void setRenderView(void *renderView);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendVideoStream => IRemoteSendVideoStreamForCallTransport
        #pragma mark
        
        virtual SendMediaTransportPtr getTransport();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendVideoStream => (internal)
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendVideoStream => (data)
        #pragma mark
        
        
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::internal::IMediaStreamDelegate)
ZS_DECLARE_PROXY_METHOD_0(onMediaStreamFaceDetected)
ZS_DECLARE_PROXY_METHOD_0(onMediaStreamVideoCaptureRecordStopped)
ZS_DECLARE_PROXY_END()
