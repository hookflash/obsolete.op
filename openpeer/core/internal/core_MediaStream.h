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
      #pragma mark MediaStream
      #pragma mark
      
      class MediaStream : public virtual IMediaStream
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
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocalMediaStreamForCallTransport
      #pragma mark
      
      interaction ILocalMediaStreamForCallTransport
      {
        ILocalMediaStreamForCallTransport &forCallTransport() {return *this;}
        const ILocalMediaStreamForCallTransport &forCallTransport() const {return *this;}
        
        virtual LocalAudioTrackListPtr getAudioTracks() = 0;
        virtual LocalVideoTrackListPtr getVideoTracks() = 0;
        
        virtual void addAudioTrack(LocalAudioTrackPtr track) = 0;
        virtual void removeAudioTrack(LocalAudioTrackPtr track) = 0;
        virtual void addVideoTrack(LocalVideoTrackPtr track) = 0;
        virtual void removeVideoTrack(LocalVideoTrackPtr track) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LocalMediaStream
      #pragma mark
      
      class LocalMediaStream : public virtual MediaStream,
                               public virtual ILocalMediaStream,
                               public virtual ILocalMediaStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction ILocalMediaStream;
        friend interaction ILocalMediaStreamForCallTransport;
        
      protected:
        LocalMediaStream(
                         IMessageQueuePtr queue,
                         IMediaStreamDelegatePtr delegate
                         );
        
      public:
        virtual ~LocalMediaStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalMediaStream => ILocalMediaStream
        #pragma mark
        virtual LocalAudioTrackListPtr getAudioTracks();
        virtual LocalVideoTrackListPtr getVideoTracks();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LocalMediaStream => ILocalMediaStreamForCallTransport
        #pragma mark
        virtual void addAudioTrack(LocalAudioTrackPtr track);
        virtual void removeAudioTrack(LocalAudioTrackPtr track);
        virtual void addVideoTrack(LocalVideoTrackPtr track);
        virtual void removeVideoTrack(LocalVideoTrackPtr track);
      };
      
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteMediaStreamForCallTransport
      #pragma mark
      
      interaction IRemoteMediaStreamForCallTransport
      {
        IRemoteMediaStreamForCallTransport &forCallTransport() {return *this;}
        const IRemoteMediaStreamForCallTransport &forCallTransport() const {return *this;}
        
        virtual RemoteAudioTrackListPtr getAudioTracks() = 0;
        virtual RemoteVideoTrackListPtr getVideoTracks() = 0;
        
        virtual void addAudioTrack(RemoteAudioTrackPtr track) = 0;
        virtual void removeAudioTrack(RemoteAudioTrackPtr track) = 0;
        virtual void addVideoTrack(RemoteVideoTrackPtr track) = 0;
        virtual void removeVideoTrack(RemoteVideoTrackPtr track) = 0;
      };
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteMediaStream
      #pragma mark
      
      class RemoteMediaStream : public virtual MediaStream,
                                public virtual IRemoteMediaStream,
                                public virtual IRemoteMediaStreamForCallTransport
      {
      public:
        friend interaction IMediaStream;
        friend interaction IRemoteMediaStream;
        friend interaction IRemoteMediaStreamForCallTransport;
        
      protected:
        RemoteMediaStream(
                          IMessageQueuePtr queue,
                          IMediaStreamDelegatePtr delegate
                          );
        
      public:
        virtual ~RemoteMediaStream();
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteMediaStream => IRemoteMediaStream
        #pragma mark
        virtual RemoteAudioTrackListPtr getAudioTracks();
        virtual RemoteVideoTrackListPtr getVideoTracks();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteMediaStream => IRemoteMediaStreamForCallTransport
        #pragma mark
        virtual void addAudioTrack(RemoteAudioTrackPtr track);
        virtual void removeAudioTrack(RemoteAudioTrackPtr track);
        virtual void addVideoTrack(RemoteVideoTrackPtr track);
        virtual void removeVideoTrack(RemoteVideoTrackPtr track);
      };
    }
  }
}
