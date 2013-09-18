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

#include <openpeer/core/internal/core_MediaSession.h>

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
      #pragma mark IMediaSessionForCallTransport
      #pragma mark
      
      //-------------------------------------------------------------------------
      MediaSessionPtr IMediaSessionForCallTransport::create(IMessageQueuePtr queue, IMediaSessionDelegatePtr delegate)
      {
        MediaSessionPtr pThis(new MediaSession(queue, delegate));
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaSession
      #pragma mark
      
      //-----------------------------------------------------------------------
      MediaSession::MediaSession(IMessageQueuePtr queue, IMediaSessionDelegatePtr delegate) :
        MessageQueueAssociator(queue),
        mVoiceRecordFile("")
      {
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaSession => IMediaSession
      #pragma mark
      
      //-------------------------------------------------------------------------
      String MediaSession::getCNAME()
      {
        
      }
      
      //-------------------------------------------------------------------------
      IMediaSessionPtr MediaSession::clone()
      {
        
      }
      
      //-------------------------------------------------------------------------
      MediaStreamListPtr MediaSession::getAudioStreams()
      {
        
      }
      
      //-------------------------------------------------------------------------
      MediaStreamListPtr MediaSession::getVideoStreams()
      {
        
      }
      
      //-------------------------------------------------------------------------
      void MediaSession::addStream(IMediaStreamPtr stream)
      {
        if (typeid(*stream) == typeid(AudioStream))
          mAudioStreams[stream->getSSRC()] = stream;
        else if (typeid(*stream) == typeid(VideoStream))
          mVideoStreams[stream->getSSRC()] = stream;
      }
      
      //-------------------------------------------------------------------------
      void MediaSession::removeStream(IMediaStreamPtr stream)
      {
        if (typeid(*stream) == typeid(AudioStream))
          mAudioStreams.erase(stream->getSSRC());
        else if (typeid(*stream) == typeid(VideoStream))
          mVideoStreams.erase(stream->getSSRC());
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaSession => IMediaSessionForCall
      #pragma mark

      //-------------------------------------------------------------------------
      void MediaSession::setVoiceRecordFile(String fileName)
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("set voice record file - value: ") + fileName)
        
        mVoiceRecordFile = fileName;
      }
      
      //-------------------------------------------------------------------------
      String MediaSession::getVoiceRecordFile() const
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("get voice record file - value: ") + mVoiceRecordFile)
        
        return mVoiceRecordFile;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaSession => (internal)
      #pragma mark
      
      //-----------------------------------------------------------------------
      String MediaSession::log(const char *message) const
      {
        return String("MediaSession [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }
      

    }
  }
}