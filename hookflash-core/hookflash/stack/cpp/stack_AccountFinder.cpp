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

#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_AccountFinder.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_Location.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/internal/stack_MessageMonitor.h>
#include <hookflash/stack/internal/stack_Stack.h>
#include <hookflash/stack/message/peer-finder/SessionDeleteRequest.h>
#include <hookflash/stack/message/peer-finder/SessionCreateRequest.h>
#include <hookflash/stack/message/peer-finder/SessionCreateResult.h>
#include <hookflash/stack/message/peer-finder/SessionKeepAliveRequest.h>
#include <hookflash/stack/message/peer-finder/SessionKeepAliveResult.h>
#include <hookflash/stack/message/peer-finder/PeerLocationFindRequest.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerFilePublic.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/XML.h>

#include <boost/shared_array.hpp>

#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>

#ifndef _WIN32
#include <unistd.h>
#endif //_WIN32

#define HOOKFLASH_STACK_SESSION_CREATE_REQUEST_TIMEOUT_IN_SECONDS (60)

#define HOOKFLASH_STACK_SESSION_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS (60)

#define HOOKFLASH_STACK_SESSION_DELETE_REQUEST_TIMEOUT_IN_SECONDS (5)

#define HOOKFLASH_STACK_ACCOUNT_FINDER_SEND_ICE_KEEP_ALIVE_INDICATIONS_IN_SECONDS (20)
#define HOOKFLASH_STACK_ACCOUNT_FINDER_EXPECT_SESSION_DATA_IN_SECONDS (90)

#define HOOKFLASH_STACK_ACCOUNT_BACKGROUNDING_TIMEOUT_IN_SECONDS (HOOKFLASH_STACK_ACCOUNT_FINDER_EXPECT_SESSION_DATA_IN_SECONDS + 40)

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      using message::peer_finder::SessionCreateRequest;
      using message::peer_finder::SessionCreateRequestPtr;
      using message::peer_finder::SessionKeepAliveRequest;
      using message::peer_finder::SessionKeepAliveRequestPtr;
      using message::peer_finder::SessionDeleteRequest;
      using message::peer_finder::SessionDeleteRequestPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinderForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      AccountFinderPtr IAccountFinderForAccount::create(
                                                        IAccountFinderDelegatePtr delegate,
                                                        AccountPtr outer
                                                        )
      {
        return AccountFinder::create(delegate, outer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder
      #pragma mark


      //-----------------------------------------------------------------------
      AccountFinder::AccountFinder(
                                   IMessageQueuePtr queue,
                                   IAccountFinderDelegatePtr delegate,
                                   AccountPtr outer
                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IAccountFinderDelegateProxy::createWeak(IStackForInternal::queueStack(), delegate)),
        mOuter(outer),
        mCurrentState(IAccount::AccountState_Pending)
      {
        ZS_LOG_BASIC(log("created"))
      }

      //---------------------------------------------------------------------
      void AccountFinder::init()
      {
        IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //---------------------------------------------------------------------
      AccountFinder::~AccountFinder()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      String AccountFinder::toDebugString(AccountFinderPtr finder, bool includeCommaPrefix)
      {
        if (!finder) return includeCommaPrefix ? String(", finder=(null)") : String("finder=(null)");
        return finder->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IAccountFinderForAccount
      #pragma mark

      //---------------------------------------------------------------------
      AccountFinderPtr AccountFinder::create(
                                             IAccountFinderDelegatePtr delegate,
                                             AccountPtr outer
                                             )
      {
        AccountFinderPtr pThis(new AccountFinder(IStackForInternal::queueStack(), delegate, outer));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IAccount::AccountStates AccountFinder::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //---------------------------------------------------------------------
      void AccountFinder::shutdown()
      {
        ZS_LOG_DEBUG(log("shutdown requested"))
        cancel();
      }

      //---------------------------------------------------------------------
      bool AccountFinder::send(MessagePtr message) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!message)

        AutoRecursiveLock lock(getLock());
        if (!message) {
          ZS_LOG_ERROR(Detail, log("message to send was NULL"))
          return false;
        }

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("attempted to send a message but the location is shutdown"))
          return false;
        }

        if (!isReady()) {
          if (!isShuttingDown()) {
            SessionCreateRequestPtr sessionCreateRequest = SessionCreateRequest::convert(message);
            if (!sessionCreateRequest) {
              ZS_LOG_WARNING(Detail, log("attempted to send a message when the finder is not ready"))
              return false;
            }
          } else {
            SessionDeleteRequestPtr sessionDeleteRequest = SessionDeleteRequest::convert(message);
            if (!sessionDeleteRequest) {
              ZS_LOG_WARNING(Detail, log("attempted to send a message when the finder is not ready"))
              return false;
            }
          }
        }

        if (!mMessaging) {
          ZS_LOG_WARNING(Detail, log("requested to send a message but messaging is not ready"))
          return false;
        }

        DocumentPtr document = message->encode();

        boost::shared_array<char> output;
        ULONG length = 0;
        output = document->writeAsJSON(&length);

        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log(">> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >"))
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log("FINDER SEND MESSAGE") + "=" + "\n" + ((CSTR)(output.get())) + "\n")
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log(">> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >> >"))
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

        return mMessaging->send((const BYTE *)(output.get()), length);
      }

      //---------------------------------------------------------------------
      IMessageMonitorPtr AccountFinder::sendRequest(
                                                    IMessageMonitorDelegatePtr delegate,
                                                    MessagePtr requestMessage,
                                                    Duration timeout
                                                    ) const
      {
        IMessageMonitorPtr monitor = IMessageMonitor::monitor(delegate, requestMessage, timeout);
        if (!monitor) {
          ZS_LOG_WARNING(Detail, log("failed to create monitor"))
          return IMessageMonitorPtr();
        }

        bool result = send(requestMessage);
        if (!result) {
          // notify that the message requester failed to send the message...
          MessageMonitor::convert(monitor)->forAccountFinder().notifyMessageSendFailed();
          return monitor;
        }

        ZS_LOG_DEBUG(log("request successfully created"))
        return monitor;
      }

      //---------------------------------------------------------------------
      Finder AccountFinder::getCurrentFinder(
                                             String *outServerAgent,
                                             IPAddress *outIPAddress
                                             ) const
      {
        if (outServerAgent) *outServerAgent = mServerAgent;
        if (outIPAddress) *outIPAddress = mFinderIP;
        return mFinder;
      }

      //-----------------------------------------------------------------------
      void AccountFinder::notifyFinderDNSComplete()
      {
        ZS_LOG_DEBUG(log("notified finder DNS complete"))
        IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IAccountFinderAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountFinder::onStep()
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IRUDPICESocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountFinder::onRUDPICESocketStateChanged(
                                                      IRUDPICESocketPtr socket,
                                                      RUDPICESocketStates state
                                                      )
      {
        ZS_LOG_DEBUG(log("notified RUDP ICE socket state changed"))
        AutoRecursiveLock lock(getLock());

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IRUDPICESocketSessionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountFinder::onRUDPICESocketSessionStateChanged(
                                                             IRUDPICESocketSessionPtr session,
                                                             RUDPICESocketSessionStates state
                                                             )
      {
        ZS_LOG_DEBUG(log("notified RUDP ICE socket session state changed"))
        AutoRecursiveLock lock(getLock());

        if (session != mSocketSession) {
          ZS_LOG_DEBUG(log("notified about an obsolete RUDP ICE socket session"))
          return;
        }

        if ((IRUDPICESocketSession::RUDPICESocketSessionState_ShuttingDown == state) ||
            (IRUDPICESocketSession::RUDPICESocketSessionState_Shutdown == state)) {
          ZS_LOG_DEBUG(log("notified about RUDP ICE socket session is closing or closed"))
          cancel();
          return;
        }

        step();
      }

      //-----------------------------------------------------------------------
      void AccountFinder::onRUDPICESocketSessionChannelWaiting(IRUDPICESocketSessionPtr session)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_WARNING(Detail, log("notify channel session waiting (not expected)"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IRUDPMessagingDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountFinder::onRUDPMessagingStateChanged(
                                                      IRUDPMessagingPtr session,
                                                      RUDPMessagingStates state
                                                      )
      {
        ZS_LOG_DEBUG(log("notified that RUDP messaging state has changed") + IRUDPMessaging::toString(state))
        AutoRecursiveLock lock(getLock());

        if (session != mMessaging) {
          ZS_LOG_DEBUG(log("notified about an obsolete ICE socket session"))
          return;
        }

        if ((IRUDPMessaging::RUDPMessagingState_ShuttingDown == state) ||
            (IRUDPMessaging::RUDPMessagingState_Shutdown == state)) {
          ZS_LOG_DEBUG(log("notified about RUDP messaging is closing or closed"))
          cancel();
          return;
        }

        step();
      }

      //-----------------------------------------------------------------------
      void AccountFinder::onRUDPMessagingReadReady(IRUDPMessagingPtr session)
      {
        typedef IRUDPMessaging::MessageBuffer MessageBuffer;

        ZS_LOG_TRACE(log("RUDP messaging read ready"))

        AutoRecursiveLock lock(getLock());

        if (session != mMessaging) {
          ZS_LOG_DEBUG(log("RUDP messaging ready came in about obsolete messaging"))
          return;
        }

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("message arrived after shutdown"))
          return;
        }

        AccountPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("account is gone thus cannot read message"))
          return;
        }

        while (true) {
          MessageBuffer buffer = mMessaging->getBufferLargeEnoughForNextMessage();
          if (!buffer) return;

          mMessaging->receive(buffer.get());

          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("< < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < <"))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("FINDER RECEIVED MESSAGE=") + "\n" + ((CSTR)(buffer.get())) + "\n")
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("< < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < <"))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

          DocumentPtr document = Document::createFromParsedXML((CSTR)(buffer.get()));
          message::MessagePtr message = Message::create(document, ILocationForAccount::getForFinder(outer));

          if (!message) {
            ZS_LOG_WARNING(Detail, log("failed to create a message from the document"))
            return;
          }

          ZS_LOG_DETAIL(log("v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v"))
          ZS_LOG_DETAIL(log("||| MESSAGE INFO |||") + Message::toDebugString(message))
          ZS_LOG_DETAIL(log("^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^"))

          if (IMessageMonitor::handleMessageReceived(message)) {
            ZS_LOG_DEBUG(log("message requester handled the message"))
            continue;
          }

          try {
            mDelegate->onAccountFinderMessageIncoming(mThisWeak.lock(), message);
          } catch(IAccountFinderDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      void AccountFinder::onRUDPMessagingWriteReady(IRUDPMessagingPtr session)
      {
        ZS_LOG_TRACE(log("RUDP messaging write ready (ignored)"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IMessageMonitorResultDelegate<SessionCreateResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool AccountFinder::handleMessageMonitorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             SessionCreateResultPtr result
                                                             )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSessionCreateMonitor) {
          ZS_LOG_WARNING(Detail, log("received an obsolete session create event"))
          return false;
        }

        mSessionCreatedTime = zsLib::now();

        mSessionCreateMonitor.reset();

        setTimeout(result->expires());
        mServerAgent = result->serverAgent();

        (IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      bool AccountFinder::handleMessageMonitorErrorResultReceived(
                                                                  IMessageMonitorPtr monitor,
                                                                  SessionCreateResultPtr ignore, // will always be NULL
                                                                  MessageResultPtr result
                                                                  )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSessionCreateMonitor) {
          ZS_LOG_WARNING(Detail, log("received an obsolete session create error event"))
          return false;
        }

        ZS_LOG_DEBUG(log("requester message session create received error reply") + Message::toDebugString(result))

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IMessageMonitorResultDelegate<SessionKeepAliveResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool AccountFinder::handleMessageMonitorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             SessionKeepAliveResultPtr result
                                                             )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSessionKeepAliveMonitor) {
          ZS_LOG_WARNING(Detail, log("received an obsolete keep alive event"))
          return false;
        }

        setTimeout(result->expires());

        (IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      bool AccountFinder::handleMessageMonitorErrorResultReceived(
                                                                  IMessageMonitorPtr monitor,
                                                                  SessionKeepAliveResultPtr ignore, // will always be NULL
                                                                  MessageResultPtr result
                                                                  )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSessionKeepAliveMonitor) {
          ZS_LOG_WARNING(Detail, log("received an obsolete session keep alive error event"))
          return false;
        }

        ZS_LOG_DEBUG(log("requester message session keep alive received error reply") + Message::toDebugString(result))
        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IMessageMonitorResultDelegate<SessionDeleteResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool AccountFinder::handleMessageMonitorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             SessionDeleteResultPtr result
                                                             )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSessionDeleteMonitor) {
          ZS_LOG_WARNING(Detail, log("received an obsolete session delete event"))
          return false;
        }

        (IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      bool AccountFinder::handleMessageMonitorErrorResultReceived(
                                                                  IMessageMonitorPtr monitor,
                                                                  SessionDeleteResultPtr ignore, // will always be NULL
                                                                  MessageResultPtr result
                                                                  )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSessionDeleteMonitor) {
          ZS_LOG_WARNING(Detail, log("received an obsolete session delete event"))
          return false;
        }

        (IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountFinder::onTimer(TimerPtr timer)
      {
        ZS_LOG_TRACE(log("timer fired"))

        if (!isReady()) return;
        if (timer != mKeepAliveTimer) return;

        if (mSessionKeepAliveMonitor) return;

        ZS_LOG_DEBUG(log("sending out keep alive request"))

        AccountPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("account object is gone"))
          return;
        }

        SessionKeepAliveRequestPtr request = SessionKeepAliveRequest::create();
        request->domain(outer->forAccountFinder().getDomain());

        mSessionKeepAliveMonitor = sendRequest(IMessageMonitorResultDelegate<SessionKeepAliveResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SESSION_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS));
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &AccountFinder::getLock() const
      {
        AccountPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->forAccountFinder().getLock();
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketPtr AccountFinder::getSocket() const
      {
        AccountPtr outer = mOuter.lock();
        if (!outer) return IRUDPICESocketPtr();
        return outer->forAccountFinder().getSocket();
      }

      //-----------------------------------------------------------------------
      void AccountFinder::setTimeout(Time expires)
      {
        Time tick = zsLib::now();

        if (Time() == expires) {
          expires = tick;
        }

        if (tick > expires) {
          expires = tick;
        }

        Duration difference = expires - tick;

        if (difference < Seconds(120))
          difference = Seconds(120);

        difference -= Seconds(60); // timeout one minute before expiry

        if (mKeepAliveTimer) {
          mKeepAliveTimer->cancel();
          mKeepAliveTimer.reset();
        }

        mKeepAliveTimer = Timer::create(mThisWeak.lock(), difference);
      }

      //-----------------------------------------------------------------------
      String AccountFinder::log(const char *message) const
      {
        return String("AccountFinder [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String AccountFinder::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("finder id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("state", IAccount::toString(mCurrentState), firstTime) +
               Helper::getDebugValue("rudp ice socket subscription id", mSocketSubscription ? Stringize<typeof(PUID)>(mSocketSubscription->getID()).string() : String(), firstTime) +
               Helper::getDebugValue("rudp ice socket session id", mSocketSession ? Stringize<typeof(PUID)>(mSocketSession->getID()).string() : String(), firstTime) +
               Helper::getDebugValue("rudp messagine id", mMessaging ? Stringize<typeof(PUID)>(mMessaging->getID()).string() : String(), firstTime) +
               mFinder.getDebugValueString() +
               Helper::getDebugValue("finder IP", !mFinderIP.isAddressEmpty() ? mFinderIP.string() : String(), firstTime) +
               Helper::getDebugValue("server agent", mServerAgent, firstTime) +
               Helper::getDebugValue("created time", Time() != mSessionCreatedTime ? IMessageHelper::timeToString(mSessionCreatedTime) : String(), firstTime) +
               Helper::getDebugValue("session create monitor", mSessionCreateMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("session keep alive monitor", mSessionKeepAliveMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("session delete monitor", mSessionDeleteMonitor ? String("true") : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void AccountFinder::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());    // just in case

        if (isShutdown()) return;

        setState(IAccount::AccountState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mKeepAliveTimer) {
          ZS_LOG_DEBUG(log("cancel stopping keep alive timer"))

          mKeepAliveTimer->cancel();
          mKeepAliveTimer.reset();
        }

        if (mSessionCreateMonitor) {
          ZS_LOG_DEBUG(log("shutdown for create session request"))

          mSessionCreateMonitor->cancel();
          mSessionCreateMonitor.reset();
        }

        if (mSessionKeepAliveMonitor) {
          ZS_LOG_DEBUG(log("shutdown for keep alive session request"))

          mSessionKeepAliveMonitor->cancel();
          mSessionKeepAliveMonitor.reset();
        }

        AccountPtr outer = mOuter.lock();

        if (mGracefulShutdownReference) {

          bool wasReady = isReady();

          if (mMessaging) {
            if (wasReady) {
              if ((!mSessionDeleteMonitor) &&
                  (outer)) {
                ZS_LOG_DEBUG(log("sending delete session request"))
                SessionDeleteRequestPtr request = SessionDeleteRequest::create();
                request->domain(outer->forAccountFinder().getDomain());

                mSessionDeleteMonitor = sendRequest(IMessageMonitorResultDelegate<SessionDeleteResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SESSION_DELETE_REQUEST_TIMEOUT_IN_SECONDS));
              }
              return;
            }
          }

          if (mSessionDeleteMonitor) {
            if (!mSessionDeleteMonitor->isComplete()) {
              ZS_LOG_DEBUG(log("shutting down waiting for delete session request to complete"))
              return;
            }
          }

          if (mMessaging) {
            ZS_LOG_DEBUG(log("shutting down messaging"))
            mMessaging->shutdown();

            if (IRUDPMessaging::RUDPMessagingState_Shutdown != mMessaging->getState()) {
              ZS_LOG_DEBUG(log("waiting for messaging to complete"))
              return;
            }
          }

          if (mSocketSession) {
            ZS_LOG_DEBUG(log("shutting down socket session"))

            mSocketSession->shutdown();
            if (IRUDPICESocketSession::RUDPICESocketSessionState_Shutdown != mSocketSession->getState()) {
              ZS_LOG_DEBUG(log("waiting for RUDP ICE socket session to complete"))
              return;
            }
          }

        }

        setState(IAccount::AccountState_Shutdown);

        mGracefulShutdownReference.reset();

        mOuter.reset();

        if (mMessaging) {
          ZS_LOG_DEBUG(log("hard shutdown for messaging"))

          mMessaging->shutdown();
          mMessaging.reset();
        }

        if (mSocketSession) {
          ZS_LOG_DEBUG(log("hard shutdown for socket session"))

          mSocketSession->shutdown();
          mSocketSession.reset();
        }

        if (mSessionDeleteMonitor) {
          ZS_LOG_DEBUG(log("hard shutdown for delete session request"))

          mSessionDeleteMonitor->cancel();
          mSessionDeleteMonitor.reset();
        }

        ZS_LOG_DEBUG(log("shutdown complete"))
      }

      //-----------------------------------------------------------------------
      void AccountFinder::step()
      {
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("step forwarding to cancel"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (isReady()) {
          ZS_LOG_DEBUG(log("finder is already ready"))
          return;
        }

        AccountPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_DEBUG(log("account object is gone thus shutting down"))
          cancel();
          return;
        }

        IRUDPICESocketPtr socket = getSocket();
        if (!socket) {
          ZS_LOG_WARNING(Detail, log("failed to obtain socket"))
          cancel();
          return;
        }

        if (!stepSocketSubscription(socket)) return;
        if (!stepSocketSession(socket)) return;
        if (!stepMessaging()) return;
        if (!stepCreateSession()) return;

        setState(IAccount::AccountState_Ready);

        ZS_LOG_DEBUG(log("step complete") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      bool AccountFinder::stepSocketSubscription(IRUDPICESocketPtr socket)
      {
        if (mSocketSubscription) {
          socket->wakeup();

          if (IRUDPICESocket::RUDPICESocketState_Ready != socket->getState()) {
            ZS_LOG_DEBUG(log("waiting for RUDP ICE socket to wake up"))
            return false;
          }

          ZS_LOG_DEBUG(log("RUDP socket is awake"))
          return true;
        }

        ZS_LOG_DEBUG(log("subscribing to the socket state"))
        mSocketSubscription = socket->subscribe(mThisWeak.lock());
        if (!mSocketSubscription) {
          ZS_LOG_ERROR(Detail, log("failed to subscribe to socket"))
          cancel();
          return false;
        }

        // ensure the socket has been woken up during the subscription process
        IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
        return false;
      }

      //-----------------------------------------------------------------------
      bool AccountFinder::stepSocketSession(IRUDPICESocketPtr socket)
      {
        if (mSocketSession) {
          if (IRUDPICESocketSession::RUDPICESocketSessionState_Ready == mSocketSession->getState()) {
            ZS_LOG_DEBUG(log("RUDP ICE socket socket is ready"))
            return true;
          }

          ZS_LOG_DEBUG(log("waiting for the RUDP ICE socket socket session to be ready"))
          return false;
        }

        AccountPtr outer = mOuter.lock();
        ZS_THROW_BAD_STATE_IF(!outer)

        if (!outer->forAccountFinder().extractNextFinder(mFinder, mFinderIP)) {
          ZS_LOG_DEBUG(log("waiting for account to obtain a finder"))
          return false;
        }

        // found an IP, put into a candidate structure

        Candidate candidate;
        candidate.mType = IICESocket::Type_Unknown;
        candidate.mIPAddress = mFinderIP;
        candidate.mPriority = 0;
        candidate.mLocalPreference = 0;

        CandidateList candidateList;
        candidateList.push_back(candidate);

        // ready for the next time if we need to prepare again...

        ZS_LOG_DEBUG(log("reqesting to connect to server") + ", ip=" + mFinderIP.string())

        // create the socket session now
        mSocketSession =  socket->createSessionFromRemoteCandidates(mThisWeak.lock(), candidateList, IICESocket::ICEControl_Controlling);
        // well, this is bad...
        if (!mSocketSession) {
          ZS_LOG_ERROR(Detail, log("cannot create a socket session"))
          cancel();
          return false;
        }

        ZS_LOG_DEBUG(log("setting keep alive properties for socket session"))
        mSocketSession->setKeepAliveProperties(
                                               Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_SEND_ICE_KEEP_ALIVE_INDICATIONS_IN_SECONDS),
                                               Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_EXPECT_SESSION_DATA_IN_SECONDS),
                                               Duration(),
                                               Seconds(HOOKFLASH_STACK_ACCOUNT_BACKGROUNDING_TIMEOUT_IN_SECONDS)
                                               );

        return false;
      }

      //-----------------------------------------------------------------------
      bool AccountFinder::stepMessaging()
      {
        if (mMessaging) {
          if (IRUDPMessaging::RUDPMessagingState_Connected != mMessaging->getState()) {
            ZS_LOG_DEBUG(log("waiting for RUDP messaging to be connected"))
            return false;
          }
          ZS_LOG_DEBUG(log("RUDP messaging is connected"))
          return true;
        }

        mMessaging = IRUDPMessaging::openChannel(IStackForInternal::queueServices(), mSocketSession, mThisWeak.lock(), "text/x-openpeer-xml-plain");
        if (!mMessaging) {
          ZS_LOG_WARNING(Detail, log("failed to open messaging channel"))
          cancel();
          return false;
        }

        ZS_LOG_DEBUG(log("RUDP messaging object created"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool AccountFinder::stepCreateSession()
      {
        if (mSessionCreateMonitor) {
          ZS_LOG_DEBUG(log("waiting for session create request to complete"))
          return false;
        }

        if (Time() != mSessionCreatedTime) {
          ZS_LOG_DEBUG(log("session already created"))
          return true;
        }

        AccountPtr outer = mOuter.lock();
        ZS_THROW_BAD_STATE_IF(!outer)

        IPeerFilesPtr peerFiles = outer->forAccountFinder().getPeerFiles();
        if (!peerFiles) {
          ZS_LOG_ERROR(Detail, log("no peer files found for session"))
          cancel();
          return false;
        }

        SessionCreateRequestPtr request = SessionCreateRequest::create();
        request->domain(outer->forAccountFinder().getDomain());

        request->finderID(mFinder.mID);

        LocationPtr selfLocation = ILocationForAccount::getForLocal(outer);
        LocationInfoPtr locationInfo = selfLocation->forAccount().getLocationInfo();
        locationInfo->mCandidates.clear();
        request->locationInfo(*locationInfo);
        request->peerFiles(peerFiles);

        ZS_LOG_DEBUG(log("sending session create request"))
        mSessionCreateMonitor = sendRequest(IMessageMonitorResultDelegate<SessionCreateResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SESSION_CREATE_REQUEST_TIMEOUT_IN_SECONDS));

        return false;
      }

      //-----------------------------------------------------------------------
      void AccountFinder::setState(IAccount::AccountStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("current state changed") + ", old=" + IAccount::toString(mCurrentState) + ", new=" + IAccount::toString(state) + getDebugValueString())
        mCurrentState = state;

        if (!mDelegate) return;

        AccountFinderPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onAccountFinderStateChanged(mThisWeak.lock(), state);
          } catch(IAccountFinderDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
