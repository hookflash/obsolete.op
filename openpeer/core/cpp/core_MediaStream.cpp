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

#include <openpeer/core/internal/core_MediaStream.h>

#define OPENPEER_MEDIA_ENGINE_INVALID_CHANNEL (-1)

namespace openpeer { namespace core { ZS_DECLARE_SUBSYSTEM(openpeer_webrtc) } }

namespace openpeer
{
  namespace core
  {
    using zsLib::Stringize;
    
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaTransport
      #pragma mark
      
      //-------------------------------------------------------------------------
      MediaTransport::MediaTransport()
      {
        
      }
      
      //-------------------------------------------------------------------------
      MediaTransport::~MediaTransport()
      {
        
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ReceiveMediaTransport
      #pragma mark
      
      //-------------------------------------------------------------------------
      ReceiveMediaTransport::ReceiveMediaTransport()
      {
        
      }
      
      //-------------------------------------------------------------------------
      ReceiveMediaTransport::~ReceiveMediaTransport()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ReceiveMediaTransport => IReceiveMediaTransportForCallTransport
      #pragma mark
      
      //-------------------------------------------------------------------------
      int ReceiveMediaTransport::receivedRTPPacket(const void *data, unsigned int length)
      {
        
      }
      
      //-------------------------------------------------------------------------
      int ReceiveMediaTransport::receivedRTCPPacket(const void *data, unsigned int length)
      {
        
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SendMediaTransport
      #pragma mark
      
      //-------------------------------------------------------------------------
      SendMediaTransport::SendMediaTransport()
      {
        
      }
      
      //-------------------------------------------------------------------------
      SendMediaTransport::~SendMediaTransport()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SendMediaTransport => ISendMediaTransportForCallTransport
      #pragma mark
      
      //-------------------------------------------------------------------------
      int SendMediaTransport::registerExternalTransport(Transport &transport)
      {
        
      }
      
      int SendMediaTransport::deregisterExternalTransport()
      {
        
      }
      
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocalSendVideoStreamForCall
      #pragma mark
      
      //-------------------------------------------------------------------------
      const char *ILocalSendVideoStreamForCall::toString(CameraTypes type)
      {
        switch (type) {
          case CameraType_None:   return "None";
          case CameraType_Front:  return "Front";
          case CameraType_Back:   return "Back";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      MediaStream::MediaStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        MessageQueueAssociator(queue),
        mChannel(OPENPEER_MEDIA_ENGINE_INVALID_CHANNEL)
      {
        
      }
      
      //-------------------------------------------------------------------------
      MediaStream::~MediaStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaStream => IMediaStream
      #pragma mark
    
      //-------------------------------------------------------------------------
      ULONG MediaStream::getSSRC()
      {
        
      }
      
      //-------------------------------------------------------------------------
      IMediaStreamPtr MediaStream::clone()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaStream => (internal)
      #pragma mark
      
      //-------------------------------------------------------------------------
      String MediaStream::log(const char *message) const
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AudioStream
      #pragma mark

      //-------------------------------------------------------------------------
      AudioStream::AudioStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        MediaStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      AudioStream::~AudioStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark VideoStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      VideoStream::VideoStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        MediaStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      VideoStream::~VideoStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark VideoStream => ILocalSendVideoStreamForCall, IRemoteReceiveVideoStreamForCall, IRemoteSendVideoStreamForCall
      #pragma mark
      
      //-------------------------------------------------------------------------
      void VideoStream::setRenderView(void *renderView)
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalSendAudioStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      LocalSendAudioStream::LocalSendAudioStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        AudioStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      LocalSendAudioStream::~LocalSendAudioStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalSendAudioStream => ILocalSendAudioStreamForCall
      #pragma mark
      
      //-------------------------------------------------------------------------
      void LocalSendAudioStream::start()
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendAudioStream::stop()
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendAudioStream::setMediaConstraints(IMediaStream::MediaConstraintList constraintList)
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteReceiveAudioStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      RemoteReceiveAudioStream::RemoteReceiveAudioStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        AudioStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      RemoteReceiveAudioStream::~RemoteReceiveAudioStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteReceiveAudioStream => IRemoteReceiveAudioStreamForCall
      #pragma mark
      
      //-------------------------------------------------------------------------
      void RemoteReceiveAudioStream::setEcEnabled(bool enabled)
      {
        
      }

      //-------------------------------------------------------------------------
      void RemoteReceiveAudioStream::setAgcEnabled(bool enabled)
      {
        
      }
      
      //-------------------------------------------------------------------------
      void RemoteReceiveAudioStream::setNsEnabled(bool enabled)
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteSendAudioStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      RemoteSendAudioStream::RemoteSendAudioStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        AudioStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      RemoteSendAudioStream::~RemoteSendAudioStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalSendVideoStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      LocalSendVideoStream::LocalSendVideoStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        VideoStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      LocalSendVideoStream::~LocalSendVideoStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalSendVideoStream => ILocalSendVideoStreamForCall
      #pragma mark

      //-------------------------------------------------------------------------
      void LocalSendVideoStream::setContinuousVideoCapture(bool continuousVideoCapture)
      {
        
      }

      //-------------------------------------------------------------------------
      bool LocalSendVideoStream::getContinuousVideoCapture()
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::setFaceDetection(bool faceDetection)
      {
        
      }

      //-------------------------------------------------------------------------
      bool LocalSendVideoStream::getFaceDetection()
      {
        
      }
      
      //-------------------------------------------------------------------------
      ILocalSendVideoStreamForCall::CameraTypes LocalSendVideoStream::getCameraType() const
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::setCameraType(CameraTypes type)
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::setRenderView(void *renderView)
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::start()
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::stop()
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::startRecord(String fileName, bool saveToLibrary)
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::stopRecord()
      {
        
      }
      
      //-------------------------------------------------------------------------
      void LocalSendVideoStream::setMediaConstraints(IMediaStream::MediaConstraintList constraintList)
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteReceiveVideoStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      RemoteReceiveVideoStream::RemoteReceiveVideoStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        VideoStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      RemoteReceiveVideoStream::~RemoteReceiveVideoStream()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteSendVideoStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      RemoteSendVideoStream::RemoteSendVideoStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        VideoStream(queue, delegate)
      {
        
      }
      
      //-------------------------------------------------------------------------
      RemoteSendVideoStream::~RemoteSendVideoStream()
      {
        
      }
    }

  }
}