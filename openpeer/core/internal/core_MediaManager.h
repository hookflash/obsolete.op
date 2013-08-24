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
#include <openpeer/core/IMediaManager.h>
#include <openpeer/core/internal/core_MediaSession.h>
#include <openpeer/core/internal/core_MediaStream.h>

#include <zsLib/MessageQueueAssociator.h>

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
      #pragma mark IMediaManagerForStack
      #pragma mark
      
      interaction IMediaManagerForStack
      {
        IMediaManagerForStack &forStack() {return *this;}
        const IMediaManagerForStack &forStack() const {return *this;}
        
        static void setup();
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaManagerForCall
      #pragma mark
      
      interaction IMediaManagerForCall
      {
        IMediaManagerForCall &forCall() {return *this;}
        const IMediaManagerForCall &forCall() const {return *this;}
        
        static MediaManagerPtr singleton();
        
        virtual MediaSessionList getReceiveMediaSessions() = 0;
        virtual MediaSessionList getSendMediaSessions() = 0;
        virtual void addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams = true) = 0;
        virtual void removeMediaSession(IMediaSessionPtr session) = 0;
        
        virtual IMediaStreamForCallTransport::MediaConstraintList getVideoConstraints(ILocalSendVideoStreamForCall::CameraTypes cameraType) = 0;
        virtual IMediaStreamForCallTransport::MediaConstraintList getAudioConstraints() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaManagerForCallTransport
      #pragma mark
      
      interaction IMediaManagerForCallTransport
      {
        typedef webrtc::Transport Transport;
        
        IMediaManagerForCallTransport &forCallTransport() {return *this;}
        const IMediaManagerForCallTransport &forCallTransport() const {return *this;}
        
        static MediaManagerPtr singleton();
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaManagerAsync
      #pragma mark

      interaction IMediaManagerAsync
      {
//        virtual void onMediaManagerSessionAdded(IMediaSessionPtr session) = 0;
//        virtual void onMediaManagerSessionRemoved(IMediaSessionPtr session) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaManager
      #pragma mark
      
      class MediaManager : public MessageQueueAssociator,
                           public IMediaManager,
                           public IMediaManagerAsync,
                           public IMediaManagerForStack,
                           public IMediaManagerForCall,
                           public IMediaManagerForCallTransport
      {
      public:
        friend interaction IMediaManager;
        friend interaction IMediaManagerAsync;
        friend interaction IMediaManagerForStack;
        friend interaction IMediaManagerForCall;
        friend interaction IMediaManagerForCallTransport;
        
      protected:
        
        MediaManager(
                     IMessageQueuePtr queue,
                     IMediaManagerDelegatePtr delegate
                     );
        
        void init();
        
        static MediaManagerPtr create(IMediaManagerDelegatePtr delegate);
        
      public:
        ~MediaManager();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaManager => IMediaManager
        #pragma mark
        
      protected:
        static MediaManagerPtr singleton(IMediaManagerDelegatePtr delegate = IMediaManagerDelegatePtr());
        
        virtual void setDefaultVideoOrientation(VideoOrientations orientation);
        virtual VideoOrientations getDefaultVideoOrientation();
        virtual void setRecordVideoOrientation(VideoOrientations orientation);
        virtual VideoOrientations getRecordVideoOrientation();
        virtual void setVideoOrientation();
        
        virtual void setMuteEnabled(bool enabled);
        virtual bool getMuteEnabled();
        virtual void setLoudspeakerEnabled(bool enabled);
        virtual bool getLoudspeakerEnabled();
        virtual OutputAudioRoutes getOutputAudioRoute();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaManager => IMediaManagerForStack
        #pragma mark
        
      protected:
        static void setup();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaManager => IMediaManagerForCall
        #pragma mark
        
      protected:
        virtual MediaSessionList getReceiveMediaSessions();
        virtual MediaSessionList getSendMediaSessions();
        virtual void addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams = true);
        virtual void removeMediaSession(IMediaSessionPtr session);
        
        virtual IMediaStream::MediaConstraintList getVideoConstraints(ILocalSendVideoStreamForCall::CameraTypes cameraType);
        virtual IMediaStream::MediaConstraintList getAudioConstraints();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaManager => IMediaManagerForCallTransport
        #pragma mark
        

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaManager => (internal)
        #pragma mark

      private:
        String log(const char *message) const;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MediaEngine => (data)
        #pragma mark
        
      protected:
        PUID mID;
        mutable RecursiveLock mLock;
        MediaManagerWeakPtr mThisWeak;
        IMediaManagerDelegatePtr mDelegate;

        int mError;

      };
    }
  }
}
