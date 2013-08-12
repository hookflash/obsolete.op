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

#include <openpeer/stack/internal/stack_MessageMonitor.h>
#include <openpeer/stack/internal/stack_MessageMonitorManager.h>
#include <openpeer/stack/internal/stack_Stack.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/ILocation.h>
#include <openpeer/stack/IBootstrappedNetwork.h>
#include <openpeer/stack/message/Message.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>


namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorForAccount
      #pragma mark


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMontiorForAccountPeerLocation
      #pragma mark


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark
      #pragma mark

      //-----------------------------------------------------------------------
      MessageMonitor::MessageMonitor(IMessageQueuePtr queue) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mWasHandled(false),
        mTimeoutFired(false),
        mPendingHandled(0)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void MessageMonitor::init(Duration timeout)
      {
        AutoRecursiveLock lock(getLock());

        mTimer = Timer::create(mThisWeak.lock(), timeout, false);

        MessageMonitorManagerPtr manager = IMessageMonitorManagerForMessageMonitor::singleton();
        manager->forMessageMonitor().monitorStart(mThisWeak.lock(), mMessageID);
      }

      //-----------------------------------------------------------------------
      MessageMonitor::~MessageMonitor()
      {
        if(isNoop()) return;
        
        AutoRecursiveLock lock(getLock());
        mThisWeak.reset();
        cancel();

        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      MessageMonitorPtr MessageMonitor::convert(IMessageMonitorPtr monitor)
      {
        return boost::dynamic_pointer_cast<MessageMonitor>(monitor);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitor => IMessageMonitor
      #pragma mark

      //-----------------------------------------------------------------------
      String MessageMonitor::toDebugString(IMessageMonitorPtr monitor, bool includeCommaPrefix)
      {
        if (!monitor) return includeCommaPrefix ? String(", monitor=(null)") : String("monitor=(null)");
        return MessageMonitor::convert(monitor)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      MessageMonitorPtr MessageMonitor::monitor(
                                                IMessageMonitorDelegatePtr delegate,
                                                message::MessagePtr requestMessage,
                                                Duration timeout
                                                )
      {
        if (!requestMessage) return MessageMonitorPtr();

        MessageMonitorPtr pThis(new MessageMonitor(IStackForInternal::queueStack()));
        pThis->mThisWeak = pThis;
        pThis->mMessageID = requestMessage->messageID();
        pThis->mOriginalMessage = requestMessage;
        pThis->mDelegate = IMessageMonitorDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate);
        pThis->init(timeout);

        ZS_THROW_INVALID_USAGE_IF(!pThis->mDelegate)

        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageMonitorPtr MessageMonitor::monitorAndSendToLocation(
                                                                 IMessageMonitorDelegatePtr delegate,
                                                                 ILocationPtr peerLocation,
                                                                 message::MessagePtr message,
                                                                 Duration timeout
                                                                 )
      {
        if (!message) return MessageMonitorPtr();

        ZS_THROW_INVALID_ARGUMENT_IF(!peerLocation)

        MessageMonitorPtr pThis = monitor(delegate, message, timeout);
        if (!peerLocation->sendMessage(message)) {
          pThis->notifyMessageSendFailed();
        }
        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageMonitorPtr MessageMonitor::monitorAndSendToService(
                                                                IMessageMonitorDelegatePtr delegate,
                                                                IBootstrappedNetworkPtr bootstrappedNetwork,
                                                                const char *serviceType,
                                                                const char *serviceMethodName,
                                                                message::MessagePtr message,
                                                                Duration timeout
                                                                )
      {
        if (!message) return MessageMonitorPtr();

        ZS_THROW_INVALID_ARGUMENT_IF(!bootstrappedNetwork)

        MessageMonitorPtr pThis = monitor(delegate, message, timeout);
        if (!bootstrappedNetwork->sendServiceMessage(serviceType, serviceMethodName, message)) {
          pThis->notifyMessageSendFailed();
        }
        return pThis;
      }

      //-----------------------------------------------------------------------
      bool MessageMonitor::handleMessageReceived(message::MessagePtr message)
      {
        return (IMessageMonitorManagerForMessageMonitor::singleton())->forMessageMonitor().handleMessage(message);
      }

      //-----------------------------------------------------------------------
      bool MessageMonitor::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      bool MessageMonitor::wasHandled() const
      {
        AutoRecursiveLock lock(getLock());
        return mWasHandled;
      }

      //-----------------------------------------------------------------------
      void MessageMonitor::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (!mDelegate) return;

        MessageMonitorManagerPtr manager = IMessageMonitorManagerForMessageMonitor::singleton();
        manager->forMessageMonitor().monitorEnd(mMessageID);

        if (mTimer) {
          mTimer->cancel();
          mTimer.reset();
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      message::MessagePtr MessageMonitor::getMonitoredMessage()
      {
        AutoRecursiveLock lock(getLock());
        return mOriginalMessage;
      }

      //-----------------------------------------------------------------------
      String MessageMonitor::getMonitoredMessageID()
      {
        AutoRecursiveLock lock(getLock());
        return mOriginalMessage->messageID();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitor => IMessageMonitorAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void MessageMonitor::onHandleMessage(
                                           IMessageMonitorDelegatePtr delegate,
                                           message::MessagePtr message
                                           )
      {
        bool handled = delegate->handleMessageMonitorMessageReceived(mThisWeak.lock(), message);

        {
          AutoRecursiveLock lock(getLock());
          --mPendingHandled;

          mWasHandled = handled;

          if (handled) {
            cancel();
            return;
          }

          if (mTimeoutFired) {
            delegate->onMessageMonitorTimedOut(mThisWeak.lock());

            cancel();
            return;
          }
        }
      }

      //-----------------------------------------------------------------------
      void MessageMonitor::onAutoHandleFailureResult(MessageResultPtr result)
      {
        ZS_LOG_DEBUG(log("auto handle error") + Message::toDebugString(result))

        handleMessageReceived(result);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitor => IMessageMonitorForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      void MessageMonitor::notifyMessageSendFailed()
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("cannot notify of send failure as delegate is gone"))
          return;
        }

        MessageResultPtr result = MessageResult::create(mOriginalMessage, IHTTP::HTTPStatusCode_Networkconnecttimeouterror);
        if (result) {
          // create a fake error result since the send failed
          IMessageMonitorAsyncDelegateProxy::create(IStackForInternal::queueStack(), mThisWeak.lock())->onAutoHandleFailureResult(result);
          return;
        }

        try {
          // could not create a result so treat it as a monitor timeout
          mDelegate->onMessageMonitorTimedOut(mThisWeak.lock());
        } catch (IMessageMonitorDelegateProxy::Exceptions::DelegateGone &) {
        }

        cancel();
      }

      //---------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitor => IMessageMonitorForMessageMonitorManager
      #pragma mark

      //-----------------------------------------------------------------------
      bool MessageMonitor::handleMessage(message::MessagePtr message)
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) return false;
        if (!message) return false;
        if (mMessageID != message->messageID()) return false;
        if (mTimeoutFired) return false;  // message arrived too late

        // create a strong reference proxy from a weak reference proxy
        IMessageMonitorDelegatePtr delegate = IMessageMonitorDelegateProxy::create(IStackForInternal::queueDelegate(), mDelegate);
        if (!delegate) return false;

        ++mPendingHandled;
        (IMessageMonitorAsyncDelegateProxy::create(mThisWeak.lock()))->onHandleMessage(delegate, message);

        // NOTE:  Do not cancel the requester since it is possible to receive
        //        more than one request/notify/response for each message ID
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitor => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void MessageMonitor::onTimer(TimerPtr timer)
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) return;

        mTimeoutFired = true;
        if (0 != mPendingHandled) return;

        try {
          mDelegate->onMessageMonitorTimedOut(mThisWeak.lock());
        } catch (IMessageMonitorDelegateProxy::Exceptions::DelegateGone &) {
        }

        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      String MessageMonitor::log(const char *message) const
      {
        return String("MessageMonitor [" + string(mID) + "] " + message);
      }

      //-----------------------------------------------------------------------
      String MessageMonitor::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("monitor id", string(mID), firstTime) +
               Helper::getDebugValue("message id", mMessageID, firstTime) +
               Helper::getDebugValue("handled", mWasHandled ? String("true") : String(), firstTime) +
               Helper::getDebugValue("timeout", mTimeoutFired ? String("true") : String(), firstTime) +
               Helper::getDebugValue("pending", 0 != mPendingHandled ? string(mPendingHandled) : String(), firstTime) +
               Helper::getDebugValue("timer", mTimer ? String("true") : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &MessageMonitor::getLock() const
      {
        MessageMonitorManagerPtr manager = IMessageMonitorManagerForMessageMonitor::singleton();
        return manager->forMessageMonitor().getLock();
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageMonitor
    #pragma mark

    //-------------------------------------------------------------------------
    String IMessageMonitor::toDebugString(IMessageMonitorPtr monitor, bool includeCommaPrefix)
    {
      return internal::MessageMonitor::toDebugString(monitor, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IMessageMonitorPtr IMessageMonitor::monitor(
                                                IMessageMonitorDelegatePtr delegate,
                                                message::MessagePtr requestMessage,
                                                Duration timeout
                                                )
    {
      return internal::IMessageMonitorFactory::singleton().monitor(delegate, requestMessage, timeout);
    }

    //-------------------------------------------------------------------------
    IMessageMonitorPtr IMessageMonitor::monitorAndSendToLocation(
                                                                 IMessageMonitorDelegatePtr delegate,
                                                                 ILocationPtr peerLocation,
                                                                 message::MessagePtr message,
                                                                 Duration timeout
                                                                 )
    {
      return internal::IMessageMonitorFactory::singleton().monitorAndSendToLocation(delegate, peerLocation, message, timeout);
    }

    //-------------------------------------------------------------------------
    IMessageMonitorPtr IMessageMonitor::monitorAndSendToService(
                                                                IMessageMonitorDelegatePtr delegate,
                                                                IBootstrappedNetworkPtr bootstrappedNetwork,
                                                                const char *serviceType,
                                                                const char *serviceMethodName,
                                                                message::MessagePtr message,
                                                                Duration timeout
                                                                )
    {
      return internal::IMessageMonitorFactory::singleton().monitorAndSendToService(delegate, bootstrappedNetwork, serviceType, serviceMethodName, message, timeout);
    }

    //-------------------------------------------------------------------------
    bool IMessageMonitor::handleMessageReceived(message::MessagePtr message)
    {
      return internal::MessageMonitor::handleMessageReceived(message);
    }
  }
}
