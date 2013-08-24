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
#include <openpeer/core/internal/core_MediaStream.h>

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
      #pragma mark IMediaSession
      #pragma mark
      
      interaction IMediaSession
      {
        virtual MediaStreamListPtr getAudioStreams() = 0;
        virtual MediaStreamListPtr getVideoStreams() = 0;
        
        virtual String getCNAME() = 0;
        
        virtual void addStream(IMediaStreamPtr stream) = 0;
        virtual void removeStream(IMediaStreamPtr stream) = 0;
      };
   
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaSessionForCall
      #pragma mark
      
      interaction IMediaSessionForCall
      {
        IMediaSessionForCall &forCall() {return *this;}
        const IMediaSessionForCall &forCall() const {return *this;}

        virtual bool getImmutable() = 0;
        
        virtual void setVoiceRecordFile(String fileName) = 0;
        virtual String getVoiceRecordFile() const = 0;
      };

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
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaSessionDelegate
      #pragma mark

      interaction IMediaSessionDelegate
      {
        virtual void onMediaSessionStreamAdded(IMediaStreamPtr stream) = 0;
        virtual void onMediaSessionStreamRemoved(IMediaStreamPtr stream) = 0;
      };

      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaSession
      #pragma mark
      
      class MediaSession : public MessageQueueAssociator,
                           public IMediaSessionForCall,
                           public IMediaSessionForCallTransport
      {
      public:
        friend interaction IMediaSession;
        friend interaction IMediaSessionForCall;
        friend interaction IMediaSessionForCallTransport;
        
        typedef std::map<ULONG, IMediaStreamPtr> MediaStreamMap;

      protected:
        MediaSession(
                     IMessageQueuePtr queue,
                     IMediaSessionDelegatePtr delegate
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
        virtual IMediaSessionPtr clone();
        
        virtual void addStream(IMediaStreamPtr stream);
        virtual void removeStream(IMediaStreamPtr stream);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaSession => IMediaSessionForCall
        #pragma mark
        
        virtual bool getImmutable();
        
        virtual void setVoiceRecordFile(String fileName);
        virtual String getVoiceRecordFile() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaSession => IMediaSessionForCallTransport
        #pragma mark
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaSession => (internal)
        #pragma mark
        
      private:
        String log(const char *message) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaSession => (data)
        #pragma mark
        
      protected:
        PUID mID;
        mutable RecursiveLock mLock;
        MediaSessionWeakPtr mThisWeak;
        
        int mError;

        String mCNAME;
        bool mImmutable;
        
        MediaStreamMap mAudioStreams;
        MediaStreamMap mVideoStreams;

        String mVoiceRecordFile;

      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::internal::IMediaSessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::core::internal::IMediaStreamPtr, IMediaStreamPtr)
ZS_DECLARE_PROXY_METHOD_1(onMediaSessionStreamAdded, IMediaStreamPtr)
ZS_DECLARE_PROXY_METHOD_1(onMediaSessionStreamRemoved, IMediaStreamPtr)
ZS_DECLARE_PROXY_END()
