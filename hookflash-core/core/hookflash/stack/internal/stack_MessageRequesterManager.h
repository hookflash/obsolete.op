/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/stack/internal/hookflashTypes.h>

#include <map>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      class MessageRequesterManager
      {
      protected:
        friend class MessageRequester;
        friend interaction IMessageRequester;

      protected:
        MessageRequesterManager();

        void monitorStart(
                          MessageRequesterPtr requester,
                          const zsLib::String &requestID
                          );
        void monitorEnd(const zsLib::String &requestID);

      public:
        ~MessageRequesterManager();

        static MessageRequesterManagerPtr singleton();
        static MessageRequesterManagerPtr create();

#ifdef __linux__
      public:
#elif __APPLE__
      public:
#else
      protected:
#endif		
        zsLib::RecursiveLock &getLock() const {return mLock;}
        bool handleMessage(message::MessagePtr message);

      protected:
        zsLib::PUID mID;
        mutable zsLib::RecursiveLock mLock;
        MessageRequesterManagerWeakPtr mThisWeak;

        typedef std::map<zsLib::String, MessageRequesterWeakPtr> RequesterMap;
        RequesterMap mRequests;
      };
    }
  }
}
