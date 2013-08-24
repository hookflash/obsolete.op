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
      #pragma mark MediaStream
      #pragma mark
      
      //-------------------------------------------------------------------------
      MediaStream::MediaStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        MessageQueueAssociator(queue),
        mChannel(OPENPEER_MEDIA_ENGINE_INVALID_CHANNEL)
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
      #pragma mark RemoteReceiveAudioStream
      #pragma mark
      
      RemoteReceiveAudioStream::RemoteReceiveAudioStream(IMessageQueuePtr queue, IMediaStreamDelegatePtr delegate) :
        AudioStream(queue, delegate),
        mEcEnabled(false),
        mAgcEnabled(false),
        mNsEnabled(false)
      {
        
      }
    }

  }
}