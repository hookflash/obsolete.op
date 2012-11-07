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

#include <hookflash/stack/IMessageRequester.h>
#include <hookflash/stack/internal/hookflashTypes.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>
#include <zsLib/String.h>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      interaction IMessageRequesterAsyncDelegate
      {
        virtual void onHandleMessage(
                                     IMessageRequesterDelegatePtr delegate,
                                     message::MessagePtr message
                                     ) = 0;
      };

      interaction IMessageRequesterForAccount
      {
        static IMessageRequesterForAccountPtr convert(IMessageRequesterPtr requester);

        virtual void notifyMessageSendFailed() = 0;
      };

      interaction IMessageRequesterForAccountPeerLocation
      {
        static IMessageRequesterForAccountPeerLocationPtr convert(IMessageRequesterPtr requester);

        virtual void notifyMessageSendFailed() = 0;
      };

      class MessageRequester : public zsLib::MessageQueueAssociator,
                               public IMessageRequester,
                               public IMessageRequesterAsyncDelegate,
                               public zsLib::ITimerDelegate,
                               public IMessageRequesterForAccount,
                               public IMessageRequesterForAccountPeerLocation
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::String String;
        typedef zsLib::Duration Duration;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef zsLib::TimerPtr TimerPtr;

      protected:
        friend class MessageRequesterManager;

      protected:
        MessageRequester(IMessageQueuePtr queue);

        void init(Duration timeout);

      public:
        ~MessageRequester();

        // IMessageRequester
        static IMessageRequesterPtr monitorRequest(
                                                   IMessageRequesterDelegatePtr delegate,
                                                   message::MessagePtr requestMessage,
                                                   Duration timeout
                                                   );

        virtual bool wasHandled() const;
        virtual bool isComplete() const;

        virtual void cancel();

        virtual message::MessagePtr getMonitoredMessage();
        virtual String getMonitoredMessageID();

        // IMessageRequesterAsyncDelegate
        virtual void onHandleMessage(
                                     IMessageRequesterDelegatePtr delegate,
                                     message::MessagePtr message
                                     );

        // ITimerDelegate
        virtual void onTimer(TimerPtr timer);

        // IMessageRequesterForAccountPeerLocation
        virtual void notifyMessageSendFailed();

      protected:
        RecursiveLock &getLock() const;
        bool handleMessage(message::MessagePtr message);

      protected:
        PUID mID;
        mutable RecursiveLock mBogusLock;
        MessageRequesterWeakPtr mThisWeak;

        IMessageRequesterDelegatePtr mDelegate;

        bool mWasHandled;
        bool mTimeoutFired;
        ULONG mPendingHandled;

        TimerPtr mTimer;
        String mMessageID;
        message::MessagePtr mOriginalMessage;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IMessageRequesterAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_2(onHandleMessage, hookflash::stack::IMessageRequesterDelegatePtr, hookflash::stack::message::MessagePtr)
ZS_DECLARE_PROXY_END()
