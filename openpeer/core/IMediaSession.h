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

#include <openpeer/core/types.h>

namespace openpeer
{
  namespace core
  {
    interaction IMediaSession
    {
      virtual MediaStreamListPtr getAudioStreams() = 0;
      virtual MediaStreamListPtr getVideoStreams() = 0;
      
      virtual String getCNAME() = 0;
      virtual bool getImmutable() = 0;
      
      virtual void addStream(IMediaStreamPtr stream) = 0;
      virtual void removeStream(IMediaStreamPtr stream) = 0;

      virtual void setVoiceRecordFile(String fileName) = 0;
      virtual String getVoiceRecordFile() const = 0;
    };
    
    interaction IMediaSessionDelegate
    {
      virtual void onMediaSessionStreamAdded(IMediaStreamPtr track) = 0;
      virtual void onMediaSessionStreamRemoved(IMediaStreamPtr track) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::IMediaSessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::core::IMediaStreamPtr, IMediaStreamPtr)
ZS_DECLARE_PROXY_METHOD_1(onMediaSessionStreamAdded, IMediaStreamPtr)
ZS_DECLARE_PROXY_METHOD_1(onMediaSessionStreamRemoved, IMediaStreamPtr)
ZS_DECLARE_PROXY_END()
