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

#include <hookflash/stack/internal/stack_MessageRequester.h>
#include <hookflash/stack/internal/stack_MessageRequesterManager.h>
#include <hookflash/stack/message/Message.h>
#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::String String;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::Timer Timer;

      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      IMessageRequesterForAccountPtr IMessageRequesterForAccount::convert(IMessageRequesterPtr requester)
      {
        return boost::dynamic_pointer_cast<MessageRequester>(requester);
      }

      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      IMessageRequesterForAccountPeerLocationPtr IMessageRequesterForAccountPeerLocation::convert(IMessageRequesterPtr requester)
      {
        return boost::dynamic_pointer_cast<MessageRequester>(requester);
      }

      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      MessageRequester::MessageRequester(IMessageQueuePtr queue) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mWasHandled(false),
        mTimeoutFired(false),
        mPendingHandled(0)
      {
      }

      //---------------------------------------------------------------------
      void MessageRequester::init(Duration timeout)
      {
        AutoRecursiveLock lock(getLock());

        mTimer = Timer::create(mThisWeak.lock(), timeout, false);

        MessageRequesterManagerPtr manager = MessageRequesterManager::singleton();
        manager->monitorStart(mThisWeak.lock(), mMessageID);
      }

      //---------------------------------------------------------------------
      MessageRequester::~MessageRequester()
      {
        AutoRecursiveLock lock(getLock());
        mThisWeak.reset();
        cancel();
      }

      //---------------------------------------------------------------------
      IMessageRequesterPtr MessageRequester::monitorRequest(
                                                            IMessageRequesterDelegatePtr delegate,
                                                            message::MessagePtr requestMessage,
                                                            Duration timeout
                                                            )
      {
        if (!requestMessage) return IMessageRequesterPtr();

        IMessageQueuePtr queue = IMessageRequesterDelegateProxy::getAssociatedMessageQueue(delegate);
        ZS_THROW_INVALID_ARGUMENT_MSG_IF(!queue, "delegate must be a proxy or object must be associated with a message queue")

        MessageRequesterPtr pThis(new MessageRequester(queue));
        pThis->mThisWeak = pThis;
        pThis->mMessageID = requestMessage->messageID();
        pThis->mOriginalMessage = requestMessage;
        pThis->mDelegate = IMessageRequesterDelegateProxy::createWeak(queue, delegate);
        pThis->init(timeout);

        ZS_THROW_INVALID_USAGE_IF(!pThis->mDelegate)

        return pThis;
      }

      //---------------------------------------------------------------------
      bool MessageRequester::wasHandled() const
      {
        AutoRecursiveLock lock(getLock());
        return mWasHandled;
      }

      //---------------------------------------------------------------------
      bool MessageRequester::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //---------------------------------------------------------------------
      void MessageRequester::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (!mDelegate) return;

        MessageRequesterManagerPtr manager = MessageRequesterManager::singleton();
        manager->monitorEnd(mMessageID);

        if (mTimer) {
          mTimer->cancel();
          mTimer.reset();
        }

        mDelegate.reset();
      }

      //---------------------------------------------------------------------
      message::MessagePtr MessageRequester::getMonitoredMessage()
      {
        AutoRecursiveLock lock(getLock());
        return mOriginalMessage;
      }

      String MessageRequester::getMonitoredMessageID()
      {
        AutoRecursiveLock lock(getLock());
        return mOriginalMessage->messageID();
      }

      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      void MessageRequester::onHandleMessage(
                                             IMessageRequesterDelegatePtr delegate,
                                             message::MessagePtr message
                                             )
      {
        bool handled = delegate->handleMessageRequesterMessageReceived(mThisWeak.lock(), message);

        {
          AutoRecursiveLock lock(getLock());
          --mPendingHandled;

          mWasHandled = handled;

          if (handled) {
            cancel();
            return;
          }

          if (mTimeoutFired) {
            delegate->onMessageRequesterTimedOut(mThisWeak.lock());

            cancel();
            return;
          }
        }
      }

      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      void MessageRequester::onTimer(TimerPtr timer)
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) return;

        mTimeoutFired = true;
        if (0 != mPendingHandled) return;

        try {
          mDelegate->onMessageRequesterTimedOut(mThisWeak.lock());
        } catch (IMessageRequesterDelegateProxy::Exceptions::DelegateGone &) {
        }

        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      void MessageRequester::notifyMessageSendFailed()
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) return;

        try {
          mDelegate->onMessageRequesterTimedOut(mThisWeak.lock());
        } catch (IMessageRequesterDelegateProxy::Exceptions::DelegateGone &) {
        }

        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      RecursiveLock &MessageRequester::getLock() const
      {
        MessageRequesterManagerPtr manager = MessageRequesterManager::singleton();
        return manager->getLock();
      }

      //---------------------------------------------------------------------
      bool MessageRequester::handleMessage(message::MessagePtr message)
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) return false;
        if (!message) return false;
        if (mMessageID != message->messageID()) return false;
        if (mTimeoutFired) return false;  // message arrived too late

        // create a strong reference proxy from a weak reference proxy
        IMessageRequesterDelegatePtr delegate = IMessageRequesterDelegateProxy::create(mDelegate);
        if (!delegate) return false;

        ++mPendingHandled;
        (IMessageRequesterAsyncDelegateProxy::create(mThisWeak.lock()))->onHandleMessage(delegate, message);

        // NOTE:  Do not cancel the requester since it is possible to receive
        //        more than one request/notify/response for each message ID
        return true;
      }
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    IMessageRequesterPtr IMessageRequester::monitorRequest(
                                                           IMessageRequesterDelegatePtr delegate,
                                                           message::MessagePtr requestMessage,
                                                           Duration timeout
                                                           )
    {
      return internal::MessageRequester::monitorRequest(delegate, requestMessage, timeout);
    }

    bool IMessageRequester::handleMessage(message::MessagePtr message)
    {
      return (internal::MessageRequesterManager::singleton())->handleMessage(message);
    }
  }
}
