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
#include <openpeer/core/IMediaStream.h>

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
        
        IMediaTransport &forCallTransport() {return *this;}
        const IMediaTransport &forCallTransport() const {return *this;}

        virtual int getTransportStatistics(RtpRtcpStatistics &stat) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IReceiveMediaTransportForCallTransport
      #pragma mark
      
      interaction IReceiveMediaTransportForCallTransport : public virtual IMediaTransport
      {
        IReceiveMediaTransportForCallTransport &forCallTransport() {return *this;}
        const IReceiveMediaTransportForCallTransport &forCallTransport() const {return *this;}
        
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
      
      interaction ISendMediaTransportForCallTransport : public virtual IMediaTransport
      {
        typedef webrtc::Transport Transport;
        
        ISendMediaTransportForCallTransport &forCallTransport() {return *this;}
        const ISendMediaTransportForCallTransport &forCallTransport() const {return *this;}
        
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
      
      class MediaTransport : public virtual IMediaTransport
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
        
        virtual int getTransportStatistics(RtpRtcpStatistics &stat);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ReceiveMediaTransport
      #pragma mark
      
      class ReceiveMediaTransport : public MediaTransport,
                                    public virtual IReceiveMediaTransportForCallTransport
      {
        
      public:
        friend interaction IMediaTransport;
        friend interaction IReceiveMediaTransportForCallTransport;
        
      protected:
        ReceiveMediaTransport();
        
      public:
        virtual ~ReceiveMediaTransport();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ReceiveMediaTransport => IReceiveMediaTransportForCallTransport
        #pragma mark
        
        virtual int receivedRTPPacket(const void *data, unsigned int length);
        virtual int receivedRTCPPacket(const void *data, unsigned int length);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SendMediaTransport
      #pragma mark
      
      class SendMediaTransport : public MediaTransport,
                                 public virtual ISendMediaTransportForCallTransport
      {
        
      public:
        friend interaction IMediaTransport;
        friend interaction ISendMediaTransportForCallTransport;
        
      protected:
        SendMediaTransport();
        
      public:
        virtual ~SendMediaTransport();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark SendMediaTransport => ISendMediaTransportForCallTransport
        #pragma mark
        
        virtual int receivedRTPPacket(const void *data, unsigned int length);
        virtual int receivedRTCPPacket(const void *data, unsigned int length);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaStreamForCallTransport
      #pragma mark
      
      interaction IMediaStreamForCallTransport
      {
      public:
        struct MediaConstraint;
        typedef std::list<MediaConstraint> MediaConstraintList;
        
        struct MediaConstraint
        {
          String key;
          String value;
        };
        
        IMediaStreamForCallTransport &forCallTransport() {return *this;}
        const IMediaStreamForCallTransport &forCallTransport() const {return *this;}
        
        virtual ULONG getSSRC() = 0;
        
        virtual IMediaTransportPtr getTransport() = 0;

        virtual void setMediaConstraints(MediaConstraintList constraintList) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaStream
      #pragma mark
      
      class MediaStream : public MessageQueueAssociator,
                          public virtual IMediaStream,
                          public virtual IMediaStreamForCallTransport
      {
        
      public:
        friend interaction IMediaStream;
        friend interaction IMediaStreamForCallTransport;
        
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
        
        virtual IMediaTransportPtr getTransport();
        virtual void setMediaConstraints(MediaConstraintList constraintList);
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
        friend interaction IMediaStreamForCallTransport;
        
      protected:
        AudioStream(
                    IMessageQueuePtr queue,
                    IMediaStreamDelegatePtr delegate
                    );
        
      public:
        virtual ~AudioStream();
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
        friend interaction IMediaStreamForCallTransport;
        
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
        #pragma mark VideoStream => ILocalSendVideoStream, IRemoteReceiveVideoStream, IRemoteSendVideoStream
        #pragma mark
        
        virtual void setRenderView(void *renderView);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocalStreamForCallTransport
      #pragma mark
      
      interaction ILocalStreamForCallTransport : public virtual IMediaStreamForCallTransport
      {
        ILocalStreamForCallTransport &forCallTransport() {return *this;}
        const ILocalStreamForCallTransport &forCallTransport() const {return *this;}
        
        virtual void start() = 0;
        virtual void stop() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteStreamForCallTransport
      #pragma mark
      
      interaction IRemoteStreamForCallTransport : public virtual IMediaStreamForCallTransport
      {
        IRemoteStreamForCallTransport &forCallTransport() {return *this;}
        const IRemoteStreamForCallTransport &forCallTransport() const {return *this;}
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalSendAudioStream
      #pragma mark
      
      class LocalSendAudioStream : public AudioStream,
                                   public virtual ILocalSendAudioStream,
                                   public virtual ILocalStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction ILocalSendAudioStream;
        friend interaction IMediaStreamForCallTransport;
        friend interaction ILocalStreamForCallTransport;
        
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
        #pragma mark LocalSendAudioStream => ILocalSendAudioStream
        #pragma mark
        
        virtual void start();
        virtual void stop();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendAudioStream => ILocalStreamForCallTransport
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
                                       public virtual IRemoteReceiveAudioStream,
                                       public virtual IRemoteStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction IRemoteReceiveAudioStream;
        friend interaction IMediaStreamForCallTransport;
        friend interaction IRemoteStreamForCallTransport;
        
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
        #pragma mark RemoteReceiveAudioStream => IRemoteReceiveAudioStream
        #pragma mark
        
        virtual void setEcEnabled(bool enabled) = 0;
        virtual void setAgcEnabled(bool enabled) = 0;
        virtual void setNsEnabled(bool enabled) = 0;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveAudioStream => IRemoteStreamForCallTransport
        #pragma mark
        
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteSendAudioStream
      #pragma mark
      
      class RemoteSendAudioStream : public AudioStream,
                                    public virtual IRemoteSendAudioStream,
                                    public virtual IRemoteStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction IRemoteSendAudioStream;
        friend interaction IMediaStreamForCallTransport;
        friend interaction IRemoteStreamForCallTransport;
        
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
        #pragma mark RemoteSendAudioStream => IRemoteSendAudioStream
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendAudioStream => IRemoteStreamForCallTransport
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
                                   public virtual ILocalSendVideoStream,
                                   public virtual ILocalStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction ILocalSendVideoStream;
        friend interaction IMediaStreamForCallTransport;
        friend interaction ILocalStreamForCallTransport;
        
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
        #pragma mark LocalSendVideoStream => ILocalSendVideoStream
        #pragma mark
        
        virtual void setContinuousVideoCapture(bool continuousVideoCapture);
        virtual bool getContinuousVideoCapture();
        
        virtual void setFaceDetection(bool faceDetection);
        virtual bool getFaceDetection();
        
        virtual CameraTypes getCameraType() const;
        virtual void setCameraType(CameraTypes type);
        
        virtual void setRenderView(void *renderView);
        
        virtual void start();
        virtual void stop();
        
        virtual void startRecord(String fileName, bool saveToLibrary = false);
        virtual void stopRecord();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalSendVideoStream => ILocalStreamForCallTransport
        #pragma mark
        
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteReceiveVideoStream
      #pragma mark
      
      class RemoteReceiveVideoStream : public VideoStream,
                                       public virtual IRemoteReceiveVideoStream,
                                       public virtual IRemoteStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction IRemoteReceiveVideoStream;
        friend interaction IMediaStreamForCallTransport;
        friend interaction IRemoteStreamForCallTransport;
        
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
        #pragma mark RemoteReceiveVideoStream => IRemoteReceiveVideoStream
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteReceiveVideoStream => IRemoteStreamForCallTransport
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
                                    public virtual IRemoteSendVideoStream,
                                    public virtual IRemoteStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction IRemoteSendVideoStream;
        friend interaction IMediaStreamForCallTransport;
        friend interaction IRemoteStreamForCallTransport;
        
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
        #pragma mark RemoteSendVideoStream => IRemoteSendVideoStream
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteSendVideoStream => IRemoteStreamForCallTransport
        #pragma mark
        
      };
    }
  }
}
