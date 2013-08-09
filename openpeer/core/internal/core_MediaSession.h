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
#include <openpeer/core/IMediaSession.h>

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
      #pragma mark IMediaSessionForCallTransport
      #pragma mark
      
      interaction IMediaSessionForCallTransport
      {
        IMediaSessionForCallTransport &forCallTransport() {return *this;}
        const IMediaSessionForCallTransport &forCallTransport() const {return *this;}
        
        virtual MediaStreamListPtr getAudioStreams() = 0;
        virtual MediaStreamListPtr getVideoStreams() = 0;
        
        virtual String getCNAME() = 0;
        
        virtual void addAudioStream(IMediaStreamPtr stream) = 0;
        virtual void removeAudioStream(IMediaStreamPtr stream) = 0;
        virtual void addVideoStream(IMediaStreamPtr stream) = 0;
        virtual void removeVideoStream(IMediaStreamPtr stream) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaSession
      #pragma mark
      
      class MediaSession : public MessageQueueAssociator,
                           public virtual IMediaSession,
                           public virtual IMediaSessionForCallTransport
      {
      public:
        friend interaction IMediaSession;
        friend interaction IMediaSessionForCallTransport;
        
      protected:
        MediaSession(
                     IMessageQueuePtr queue,
                     IMediaSessionPtr delegate
                     );
        
      public:
        virtual ~MediaSession();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaSession => IMediaSession
        #pragma mark
        
        virtual MediaStreamListPtr getAudioStreams();
        virtual MediaStreamListPtr getVideoStreams();
        
        virtual String getCNAME();
        virtual bool getImmutable();
        virtual IMediaSessionPtr clone();

        virtual void addAudioStream(IMediaStreamPtr stream);
        virtual void removeAudioStream(IMediaStreamPtr stream);
        virtual void addVideoStream(IMediaStreamPtr stream);
        virtual void removeVideoStream(IMediaStreamPtr stream);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaSession => IMediaSessionForCallTransport
        #pragma mark
        
      };
    }
  }
}
