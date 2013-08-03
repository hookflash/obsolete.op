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
#include <openpeer/core/IMediaTrack.h>

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
      #pragma mark IMediaTrackForCallTransport
      #pragma mark
      
      interaction IMediaTrackForCallTransport
      {
      public:
        struct MediaConstraint;
        typedef std::list<MediaConstraint> MediaConstraintList;
        
        struct MediaConstraint
        {
          String key;
          String value;
        };
        
        IMediaTrackForCallTransport &forCallTransport() {return *this;}
        const IMediaTrackForCallTransport &forCallTransport() const {return *this;}
        
        virtual void setMediaConstraints(MediaConstraintList constraintList) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaTrack
      #pragma mark
      
      class MediaTrack : public MessageQueueAssociator,
                         public virtual IMediaTrack,
                         public virtual IMediaTrackForCallTransport
      {
        
      public:
        friend interaction IMediaTrack;
        friend interaction IMediaTrackForCallTransport;

      protected:
        MediaTrack(
                   IMessageQueuePtr queue,
                   IMediaTrackDelegatePtr delegate
                   );
        
      public:
        virtual ~MediaTrack();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaTrack => IMediaTrackForCallTransport
        #pragma mark
        
        virtual void setMediaConstraints(MediaConstraintList constraintList);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AudioTrack
      #pragma mark
      
      class AudioTrack : public MediaTrack,
                         public virtual IAudioTrack
      {
      public:
        friend interaction IMediaTrack;
        friend interaction IAudioTrack;

      protected:
        AudioTrack(
                   IMessageQueuePtr queue,
                   IMediaTrackDelegatePtr delegate
                   );
        
      public:
        virtual ~AudioTrack();
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark VideoTrack
      #pragma mark
      
      class VideoTrack : public MediaTrack,
                         public virtual IVideoTrack
      {
      public:
        friend interaction IMediaTrack;
        friend interaction IVideoTrack;
        
      protected:
        VideoTrack(
                   IMessageQueuePtr queue,
                   IMediaTrackDelegatePtr delegate
                   );
        
      public:
        virtual ~VideoTrack();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark VideoTrack => IVideoTrack
        #pragma mark
        
        virtual void setRenderView(void *renderView);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalAudioTrack
      #pragma mark
      
      class LocalAudioTrack : public AudioTrack,
                              public virtual ILocalAudioTrack
      {
      public:
        friend interaction IMediaTrack;
        friend interaction IAudioTrack;
        friend interaction ILocalAudioTrack;
        
      protected:
        LocalAudioTrack(
                        IMessageQueuePtr queue,
                        IMediaTrackDelegatePtr delegate
                        );
        
      public:
        virtual ~LocalAudioTrack();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalVideoTrack => ILocalVideoTrack
        #pragma mark
        
        virtual void start();
        virtual void stop();
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteAudioTrackForCallTransport
      #pragma mark
      
      interaction IRemoteAudioTrackForCallTransport : public virtual IMediaTrackForCallTransport
      {
        typedef webrtc::Transport Transport;
        
        IRemoteAudioTrackForCallTransport &forCallTransport() {return *this;}
        const IRemoteAudioTrackForCallTransport &forCallTransport() const {return *this;}
        
        virtual void start() = 0;
        virtual void stop() = 0;
        
        virtual int registerExternalTransport(Transport &transport) = 0;
        virtual int deregisterExternalTransport() = 0;
        virtual int receivedRTPPacket(const void *data, unsigned int length) = 0;
        virtual int receivedRTCPPacket(const void *data, unsigned int length) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteAudioTrack
      #pragma mark
      
      class RemoteAudioTrack : public AudioTrack,
                               public virtual IRemoteAudioTrack,
                               public virtual IRemoteAudioTrackForCallTransport
      {
      public:
        friend interaction IMediaTrack;
        friend interaction IAudioTrack;
        friend interaction IRemoteAudioTrack;
        friend interaction IMediaTrackForCallTransport;
        friend interaction IRemoteAudioTrackForCallTransport;

      protected:
        RemoteAudioTrack(
                         IMessageQueuePtr queue,
                         IMediaTrackDelegatePtr delegate
                         );
        
      public:
        virtual ~RemoteAudioTrack();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteAudioTrack => IRemoteAudioTrack
        #pragma mark
        
        virtual void setEcEnabled(bool enabled) = 0;
        virtual void setAgcEnabled(bool enabled) = 0;
        virtual void setNsEnabled(bool enabled) = 0;
        virtual void setVoiceRecordFile(String fileName) = 0;
        virtual String getVoiceRecordFile() const = 0;
        
        virtual int getTransportStatistics(RtpRtcpStatistics &stat) = 0;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteAudioTrack => IRemoteAudioTrackForCallTransport
        #pragma mark
        
        virtual void start();
        virtual void stop();
        
        virtual int registerExternalTransport(Transport &transport);
        virtual int deregisterExternalTransport();
        virtual int receivedRTPPacket(const void *data, unsigned int length);
        virtual int receivedRTCPPacket(const void *data, unsigned int length);
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalVideoTrack
      #pragma mark
      
      class LocalVideoTrack : public VideoTrack,
                              public virtual ILocalVideoTrack
      {
      public:
        friend interaction IMediaTrack;
        friend interaction IVideoTrack;
        friend interaction ILocalVideoTrack;
        
      protected:
        LocalVideoTrack(
                        IMessageQueuePtr queue,
                        IMediaTrackDelegatePtr delegate
                        );
        
      public:
        virtual ~LocalVideoTrack();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalVideoTrack => ILocalVideoTrack
        #pragma mark
        
        virtual void setContinuousVideoCapture(bool continuousVideoCapture);
        virtual bool getContinuousVideoCapture();
        
        virtual void setFaceDetection(bool faceDetection);
        virtual bool getFaceDetection();
        
        virtual CameraTypes getCameraType() const;
        virtual void setCameraType(CameraTypes type);
        
        virtual void start();
        virtual void stop();
        
        virtual void startRecord(String fileName, bool saveToLibrary = false);
        virtual void stopRecord();
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteAudioTrackForCallTransport
      #pragma mark
      
      interaction IRemoteVideoTrackForCallTransport : public virtual IMediaTrackForCallTransport
      {
        typedef webrtc::Transport Transport;
        
        IRemoteVideoTrackForCallTransport &forCallTransport() {return *this;}
        const IRemoteVideoTrackForCallTransport &forCallTransport() const {return *this;}
        
        virtual void start() = 0;
        virtual void stop() = 0;
        
        virtual int registerExternalTransport(Transport &transport) = 0;
        virtual int deregisterExternalTransport() = 0;
        virtual int receivedRTPPacket(const void *data, unsigned int length) = 0;
        virtual int receivedRTCPPacket(const void *data, unsigned int length) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteVideoTrack
      #pragma mark
      
      class RemoteVideoTrack : public VideoTrack,
                               public virtual IRemoteVideoTrack,
                               public virtual IRemoteVideoTrackForCallTransport
      {
      public:
        friend interaction IMediaTrack;
        friend interaction IVideoTrack;
        friend interaction IRemoteVideoTrack;
        friend interaction IMediaTrackForCallTransport;
        friend interaction IRemoteVideoTrackForCallTransport;

      protected:
        RemoteVideoTrack(
                         IMessageQueuePtr queue,
                         IMediaTrackDelegatePtr delegate
                         );
        
      public:
        virtual ~RemoteVideoTrack();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteVideoTrack => IRemoteVideoTrack
        #pragma mark
        
        virtual int getTransportStatistics(RtpRtcpStatistics &stat) = 0;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteVideoTrack => IRemoteVideoTrackForCallTransport
        #pragma mark
        
        virtual void start();
        virtual void stop();
        
        virtual int registerExternalTransport(Transport &transport);
        virtual int deregisterExternalTransport();
        virtual int receivedRTPPacket(const void *data, unsigned int length);
        virtual int receivedRTCPPacket(const void *data, unsigned int length);
      };
    }
  }
}
