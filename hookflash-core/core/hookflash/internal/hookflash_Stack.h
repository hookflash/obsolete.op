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

#include <hookflash/IStack.h>
#include <hookflash/internal/hookflashTypes.h>

namespace hookflash
{
  namespace internal
  {
    interaction IStackShutdownCheckAgain;
    typedef boost::shared_ptr<IStackShutdownCheckAgain> IStackShutdownCheckAgainPtr;
    typedef boost::weak_ptr<IStackShutdownCheckAgain> IStackShutdownCheckAgainWeakPtr;

    class Stack : public IStack
    {
    public:
      typedef zsLib::String String;

    protected:
      friend class hookflash::internal::Account;

    protected:
      Stack(
            IStackDelegatePtr stackDelegate,
            IMediaEngineDelegatePtr mediaEngineDelegate,
            IConversationThreadDelegatePtr conversationThreadDelegate,
            ICallDelegatePtr callDelegate,
            const char *deviceID,
            const char *userAgent,
            const char *os,
            const char *system
            );

    public:

      static StackPtr convert(IStackPtr stack);

      static StackPtr create(
                             IStackDelegatePtr stackDelegate,
                             IMediaEngineDelegatePtr mediaEngineDelegate,
                             IConversationThreadDelegatePtr conversationThreadDelegate,
                             ICallDelegatePtr callDelegate,
                             const char *deviceID,
                             const char *userAgent,
                             const char *os,
                             const char *system
                             );
      virtual void startShutdown();

      void onShutdownCheckAgain();

      const zsLib::String &getDeviceID() const {return mDeviceID;}
      const zsLib::String &getUserAgent() const {return mUserAgent;}
      const zsLib::String &getOS() const {return mOS;}
      const zsLib::String &getSystem() const {return mSystem;}

    protected:
      zsLib::RecursiveLock mLock;

      StackWeakPtr mThisWeak;

      IStackShutdownCheckAgainPtr mCheckShutdown;

      IStackDelegatePtr              mStackDelegate;
      IMediaEngineDelegatePtr        mMediaEngineDelegate;
      IConversationThreadDelegatePtr mConversationThreadDelegate;
      ICallDelegatePtr               mCallDelegate;

      String mDeviceID;
      String mUserAgent;
      String mOS;
      String mSystem;
    };
  }
}
