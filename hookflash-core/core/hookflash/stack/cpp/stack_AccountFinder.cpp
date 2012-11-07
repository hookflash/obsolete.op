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

#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_AccountFinder.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_MessageRequester.h>
#include <hookflash/stack/message/PeerToFinderSessionDeleteRequest.h>
#include <hookflash/stack/message/PeerToFinderSessionCreateRequest.h>
#include <hookflash/stack/message/PeerToFinderSessionCreateResult.h>
#include <hookflash/stack/message/PeerKeepAliveRequest.h>
#include <hookflash/stack/message/PeerKeepAliveResult.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/services/IHelper.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

#include <boost/shared_array.hpp>

#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>

#ifndef _WIN32
#include <unistd.h>
#endif //_WIN32

#define HOOKFLASH_STACK_SESSION_CREATE_REQUEST_TIMEOUT_IN_SECONDS (60)
#define HOOKFLASH_STACK_SESSION_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS (60)
#define HOOKFLASH_STACK_PEER_LOCATION_REGISTER_REQUEST_TIMEOUT_IN_SECONDS (60)
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

      typedef zsLib::BYTE BYTE;
      typedef zsLib::WORD WORD;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::CSTR CSTR;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef zsLib::Duration Duration;
      typedef zsLib::Seconds Seconds;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::Timer Timer;
      typedef zsLib::IPAddress IPAddress;
      typedef zsLib::XML::Document Document;
      typedef CryptoPP::AutoSeededRandomPool AutoSeededRandomPool;
      typedef CryptoPP::SHA256 SHA256;
      typedef services::IICESocket IICESocket;
      typedef services::IRUDPICESocket IRUDPICESocket;
      typedef services::IRUDPICESocketPtr IRUDPICESocketPtr;
      typedef services::IRUDPICESocketSession IRUDPICESocketSession;
      typedef services::IRUDPMessaging IRUDPMessaging;


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static const String &getBogusPassword()
      {
        static String bogus;
        return bogus;
      }

      //---------------------------------------------------------------------
      String convertToBase64(
                             const BYTE *buffer,
                             ULONG bufferLengthInBytes
                             );

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinder
      #pragma mark

      //-----------------------------------------------------------------------
      const char *IAccountFinder::toString(AccountFinder::AccountFinderStates state)
      {
        switch (state) {
          case AccountFinderState_Pending:       return "Pending";
          case AccountFinderState_Ready:         return "Ready";
          case AccountFinderState_ShuttingDown:  return "Shutting down";
          case AccountFinderState_Shutdown:      return "Shutdonw";
        }
        return "UNDEFINED";
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
                                   IAccountForAccountFinderPtr outer
                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IAccountFinderDelegateProxy::createWeak(delegate)),
        mOuter(outer),
        mCurrentState(AccountFinderState_Pending),
        mLocationID(outer->getLocationID()),
        mLocationSalt(32)
      {
        ZS_LOG_BASIC(log("created"))
        AutoSeededRandomPool rng;
        rng.GenerateBlock(mLocationSalt, mLocationSalt.size());
      }

      //---------------------------------------------------------------------
      void AccountFinder::init()
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //---------------------------------------------------------------------
      AccountFinder::~AccountFinder()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //---------------------------------------------------------------------
      AccountFinderPtr AccountFinder::create(
                                             IMessageQueuePtr queue,
                                             IAccountFinderDelegatePtr delegate,
                                             IAccountForAccountFinderPtr outer
                                             )
      {
        AccountFinderPtr pThis(new AccountFinder(queue, delegate, outer));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IAccountFinder
      #pragma mark

      //-----------------------------------------------------------------------
      IAccountFinder::AccountFinderStates AccountFinder::getState() const
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
      String AccountFinder::getLocationID() const
      {
        AutoRecursiveLock lock(getLock());
        return mLocationID;
      }

      //---------------------------------------------------------------------
      bool AccountFinder::requestSendMessage(
                                             DocumentPtr message,
                                             bool sendOnlyIfReady
                                             )
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("request send message"))

        if (sendOnlyIfReady) {
          if (!isReady()) {
            ZS_LOG_DEBUG(log("cannot request to send a message as finder is not ready"))
            return false;
          }
        }

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("cannot request to send a message while shutdown"))
          return false;
        }

        boost::shared_array<char> output;
        ULONG length = 0;
        output = message->write(&length);

        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log("> > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > >"))
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log("FINDER SEND MESSAGE=") + "\n" + String((CSTR)output.get()) + "\n")
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log("> > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > >"))
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        return mMessaging->send((const BYTE *)(output.get()), length);
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
      #pragma mark AccountFinder => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountFinder::onBootstrappedNetworkStateChanged(
                                                            IBootstrappedNetworkPtr bootstrapper,
                                                            BootstrappedNetworkStates state
                                                            )
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

        if (isShutdown()) return;

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

          DocumentPtr document = Document::create();
          document->parse((CSTR)(buffer.get()));
          message::MessagePtr message = message::Message::create(document);

          if (!message) {
            ZS_LOG_WARNING(Detail, log("failed to create a message from the document"))
            return;
          }

          ZS_LOG_DETAIL(log("v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v"))
          ZS_LOG_DETAIL(log("||| MESSAGE INFO |||") + " type=" + message::Message::toString(message->messageType()) + ", method=" + message->methodAsString())
          ZS_LOG_DETAIL(log("^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^"))

          if (IMessageRequester::handleMessage(message)) {
            ZS_LOG_DEBUG(log("message requester handled the message"))
            return;
          }

          // this is something new/incoming from the remote server...
          IAccountForAccountFinderPtr outer = getOuter();
          if (!outer) {
            ZS_LOG_WARNING(Debug, log("failed to obtain account"))
            return;
          }

          ZS_LOG_DEBUG(log("notifying the account about the incoming finder message"))
          outer->notifyAccountFinderIncomingMessage(mThisWeak.lock(), message);
        }
      }

      //-----------------------------------------------------------------------
      void AccountFinder::onRUDPMessagingWriteReady(IRUDPMessagingPtr session)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_TRACE(log("RUDP messaging write ready (ignored)"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder => IMessageRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool AccountFinder::handleMessageRequesterMessageReceived(
                                                                IMessageRequesterPtr requester,
                                                                message::MessagePtr message
                                                                )
      {
        ZS_LOG_DEBUG(log("requester message received") + ", id=" + requester->getMonitoredMessageID())

        AutoRecursiveLock lock(getLock());
        if (AccountFinderState_Shutdown == mCurrentState) {
          ZS_LOG_DEBUG(log("requester message received but ignored as shutdown has already occured"))
          return false;
        }

        if (requester == mSessionCreateRequester) {
          ZS_LOG_DEBUG(log("requester message received for session create"))

          if (message::Message::MessageType_Result != message->messageType()) {
            ZS_LOG_WARNING(Detail, log("requester message received was not a result (ignored)"))
            return false;
          }

          // scope: check for error
          {
            message::MessageResultPtr result = message::MessageResult::convert(message);
            if (result->hasAttribute(message::MessageResult::AttributeType_ErrorCode)) {
              ZS_LOG_DEBUG(log("requester message session create received error reply") + ", error=" + Stringize<WORD>(result->errorCode()).string())

              cancel();
              return true;
            }
          }

          if (message::MessageFactoryStack::Method_PeerToFinder_SessionCreate != (message::MessageFactoryStack::Methods)message->method()) {
            ZS_LOG_ERROR(Detail, log("requester message response was not a session create response"))

            cancel();
            return false;
          }

          message::PeerToFinderSessionCreateResultPtr result = message::PeerToFinderSessionCreateResult::convert(message);
          IAccountForAccountFinder::adjustToServerTime(result->time());

          mSessionCreateRequester.reset();

          Time expires = result->expires();
          if (Time() == expires) {
            expires = zsLib::now();
          }
          Time current = IAccount::getAdjustedTime();
          Duration difference = expires - current;

          if (difference < Seconds(120))
            difference = Seconds(120);

          difference -= Seconds(60); // timeout one minute before expiry

          if (mKeepAliveTimer) {
            mKeepAliveTimer->cancel();
            mKeepAliveTimer.reset();
          }

          mKeepAliveTimer = Timer::create(mThisWeak.lock(), difference);

          setState(AccountFinderState_Ready);

          (IAccountFinderAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
          return true;
        }

        if (requester == mSessionKeepAliveRequester) {
          ZS_LOG_DEBUG(log("requester message received for keep alive"))

          if (message::Message::MessageType_Result != message->messageType()) {
            ZS_LOG_WARNING(Detail, log("keep alive result was not a result type (ignored)"))
            return false;
          }

          mSessionKeepAliveRequester.reset();

          // scope: check for error
          {
            message::MessageResultPtr result = message::MessageResult::convert(message);
            if (result->hasAttribute(message::MessageResult::AttributeType_ErrorCode)) {
              ZS_LOG_DEBUG(log("requester message keep alive received error reply") + ", error=" + Stringize<WORD>(result->errorCode()).string())
              return true;
            }
          }

          if (message::MessageFactoryStack::Method_PeerKeepAlive != (message::MessageFactoryStack::Methods)message->method()) {
            ZS_LOG_WARNING(Detail, log("keep alive result was not a keep alive message type"))
            return false;
          }

          // adjust the local clock to the server's clock
          message::PeerKeepAliveResultPtr result = message::PeerKeepAliveResult::convert(message);
          IAccountForAccountFinder::adjustToServerTime(result->time());

          step();
          return true;
        }

        if (requester == mSessionDeleteRequester) {
          ZS_LOG_DEBUG(log("requester message received for session delete"))

          // mark the requester as being complete
          mSessionDeleteRequester->cancel();

          cancel();
          return true;
        }

        return false;
      }

      //-----------------------------------------------------------------------
      void AccountFinder::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        ZS_LOG_WARNING(Detail, log("message requester timed out") + ", id=" + requester->getMonitoredMessageID())
        AutoRecursiveLock lock(getLock());
        if (requester == mSessionCreateRequester) {
          ZS_LOG_DEBUG(log("requester message received timeout for session create"))

          if (isPending()) {
            cancel();
            return;
          }

          // already received a successful response
          mSessionCreateRequester.reset();

          step();
          return;
        }

        if (requester == mSessionDeleteRequester) {
          ZS_LOG_DEBUG(log("requester message received timeout for session delete"))

          cancel();
          return;
        }

        if (requester == mSessionKeepAliveRequester) {
          ZS_LOG_DEBUG(log("requester message received timeout for session keep alive"))
          mSessionKeepAliveRequester.reset();
        }
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

        if (mSessionKeepAliveRequester) return;

        ZS_LOG_DEBUG(log("sending out keep alive request"))

        message::PeerKeepAliveRequestPtr request = message::PeerKeepAliveRequest::create();

        mSessionKeepAliveRequester = sendRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_SESSION_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS));
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
        IAccountForAccountFinderPtr outer = getOuter();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      IAccountForAccountFinderPtr AccountFinder::getOuter() const
      {
        return mOuter.lock();
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketPtr AccountFinder::getSocket() const
      {
        IAccountForAccountFinderPtr outer = getOuter();
        if (!outer) return IRUDPICESocketPtr();
        return outer->getSocket();
      }

      //-----------------------------------------------------------------------
      IBootstrappedNetworkForAccountFinderPtr AccountFinder::getBootstrapper() const
      {
        IAccountForAccountFinderPtr outer = getOuter();
        if (!outer) return IBootstrappedNetworkForAccountFinderPtr();
        return outer->getBootstrapper();
      }

      //-----------------------------------------------------------------------
      IPeerFilesPtr AccountFinder::getPeerFiles() const
      {
        IAccountForAccountFinderPtr outer = getOuter();
        if (!outer) return IPeerFilesPtr();
        return outer->getPeerFiles();
      }

      //-----------------------------------------------------------------------
      String AccountFinder::log(const char *message) const
      {
        return String("AccountFinder [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      IMessageRequesterPtr AccountFinder::sendRequest(
                                                      IMessageRequesterDelegatePtr delegate,
                                                      message::MessagePtr requestMessage,
                                                      Duration timeout,
                                                      bool onlyIfReady
                                                      )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!requestMessage)

        IMessageRequesterPtr requester = IMessageRequester::monitorRequest(delegate, requestMessage, timeout);
        if (!requester) return IMessageRequesterPtr();

        IPeerFilesPtr peerFiles = getPeerFiles();
        if (peerFiles) {
          bool result = requestSendMessage(requestMessage->encode(peerFiles), onlyIfReady);
          if (result) return requester;
        }

        IMessageRequesterForAccountPtr requesterForAccount = IMessageRequesterForAccount::convert(requester);

        // notify that the message requester failed to send the message...
        requesterForAccount->notifyMessageSendFailed();

        return requester;
      }

      //-----------------------------------------------------------------------
      void AccountFinder::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());    // just in case

        if (isShutdown()) return;

        setState(AccountFinderState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mKeepAliveTimer) {
          ZS_LOG_DEBUG(log("cancel stopping keep alive timer"))

          mKeepAliveTimer->cancel();
          mKeepAliveTimer.reset();
        }

        if (mSessionCreateRequester) {
          ZS_LOG_DEBUG(log("shutdown for create session request"))

          mSessionCreateRequester->cancel();
          mSessionCreateRequester.reset();
        }

        if (mSessionKeepAliveRequester) {
          ZS_LOG_DEBUG(log("shutdown for keep alive session request"))

          mSessionKeepAliveRequester->cancel();
          mSessionKeepAliveRequester.reset();
        }

        if (mGracefulShutdownReference) {

          bool wasReady = isReady();

          if (mMessaging) {
            if (wasReady) {
              if (!mSessionDeleteRequester) {
                ZS_LOG_DEBUG(log("sending delete session request"))
                message::PeerToFinderSessionDeleteRequestPtr request = message::PeerToFinderSessionDeleteRequest::create();

                mSessionDeleteRequester = sendRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_SESSION_DELETE_REQUEST_TIMEOUT_IN_SECONDS), false);
              }
              return;
            }
          }

          if (mSessionDeleteRequester) {
            if (!mSessionDeleteRequester->isComplete()) {
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

        setState(AccountFinderState_Shutdown);

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

        if (mSessionDeleteRequester) {
          ZS_LOG_DEBUG(log("hard shutdown for delete session request"))

          mSessionDeleteRequester->cancel();
          mSessionDeleteRequester.reset();
        }
        ZS_LOG_DEBUG(log("shutdown complete"))
      }

      //-----------------------------------------------------------------------
      void AccountFinder::step()
      {
        if ((isShuttingDown()) ||
            (isShutdown())) {
          cancel();
          return;
        }

        if (isReady()) {
          return;
        }

        IBootstrappedNetworkForAccountFinderPtr bootstapper = getBootstrapper();
        if (!bootstapper) {
          ZS_LOG_DEBUG(log("cannot obtain bootstrapper"))
          cancel();
          return;
        }

        if (!mBootstrapperSubscription) {
          mBootstrapperSubscription = bootstapper->subscribe(mThisWeak.lock());
        }

        if (IBootstrappedNetwork::BootstrappedNetworkState_Ready != bootstapper->getState()) {
          ZS_LOG_DEBUG(log("waiting for bootstrapper to be ready"))
          return;
        }

        IRUDPICESocketPtr socket = getSocket();
        if (!socket) {
          ZS_LOG_WARNING(Detail, log("failed to obtain socket"))
          cancel();
          return;
        }

        if (!mSocketSubscription) {
          ZS_LOG_DEBUG(log("subscribing to the socket state"))
          mSocketSubscription = socket->subscribe(mThisWeak.lock());

          // ensure the socket has been woken up during the subscription process
          socket->wakeup();
        }

        if (IRUDPICESocket::RUDPICESocketState_Ready != socket->getState()) {
          ZS_LOG_DEBUG(log("waiting for RUDP ICE socket to wake up"))
          return;
        }

        // we need to wake up the socket if it is sleeping because we can't connect to the server unless we have local candidates
        if (!mSocketSession) {
          IPAddress nextIP = bootstapper->getNextPeerFinder();
          if (nextIP.isAddressEmpty()) {
            ZS_LOG_WARNING(Detail, log("no server address to connect"))
            cancel();
            return;
          }

          // found an IP, put into a candidate structure

          Candidate candidate;
          candidate.mType = IICESocket::Type_Unknown;
          candidate.mIPAddress = nextIP;
          candidate.mPriority = 0;
          candidate.mLocalPreference = 0;

          CandidateList candidateList;
          candidateList.push_back(candidate);

          // ready for the next time if we need to prepare again...

          ZS_LOG_DEBUG(log("reqesting to connect to server") + ", ip=" + nextIP.string())

          // create the socket session now
          mSocketSession =  socket->createSessionFromRemoteCandidates(mThisWeak.lock(), candidateList, IICESocket::ICEControl_Controlling);
          if (mSocketSession) {
            ZS_LOG_DEBUG(log("setting keep alive properties for socket session"))
            mSocketSession->setKeepAliveProperties(
                                                   Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_SEND_ICE_KEEP_ALIVE_INDICATIONS_IN_SECONDS),
                                                   Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_EXPECT_SESSION_DATA_IN_SECONDS),
                                                   Duration(),
                                                   Seconds(HOOKFLASH_STACK_ACCOUNT_BACKGROUNDING_TIMEOUT_IN_SECONDS)
                                                   );
          } else {
            ZS_LOG_ERROR(Detail, log("failed to create socket session"))
          }

          // well, this is bad...
          if (!mSocketSession) {
            ZS_LOG_ERROR(Detail, log("count not create a socket session"))
            cancel();
            return;
          }
        }

        if (IRUDPICESocketSession::RUDPICESocketSessionState_Ready != mSocketSession->getState()) {
          ZS_LOG_DEBUG(log("waiting for the RUDP ICE socket socket session to be ready"))
          return;
        }

        if (!mMessaging) {
          mMessaging = IRUDPMessaging::openChannel(getAssociatedMessageQueue(), mSocketSession, mThisWeak.lock(), "text/x-openpeer-xml-plain");
          if (!mMessaging) {
            ZS_LOG_WARNING(Detail, log("failed to open messaging channel"))
            return;
          }
        }

        if (IRUDPMessaging::RUDPMessagingState_Connected != mMessaging->getState()) {
          ZS_LOG_DEBUG(log("waiting for RUDP messaging to be connected"))
          return;
        }

        if (mSessionCreateRequester) {
          ZS_LOG_DEBUG(log("waiting for session create request to complete"))
          cancel();
          return;
        }

        IPeerFilesPtr peerFiles = getPeerFiles();
        if (!peerFiles) {
          ZS_LOG_ERROR(Detail, log("no peer files found for session"))
          cancel();
          return;
        }

        IPeerFilePublicPtr peerPublic = peerFiles->getPublic();
        if (!peerPublic) {
          ZS_LOG_ERROR(Detail, log("no public peer files found for session"))
          cancel();
          return;
        }

        IAccountForAccountFinderPtr outer = getOuter();
        if (!peerPublic) {
          ZS_LOG_ERROR(Detail, log("account was destroyed"))
          cancel();
          return;
        }

        CandidateList candidates;
        socket->getLocalCandidates(candidates);

        IPAddress ipAddress;
        for (CandidateList::iterator iter = candidates.begin(); iter != candidates.end(); ++iter) {
          Candidate &candidate = (*iter);
          if (IICESocket::Type_Local == candidate.mType) {
            ipAddress = (*iter).mIPAddress;
          }
          if (IICESocket::Type_ServerReflexive == candidate.mType) {
            break;
          }
        }

        message::PeerToFinderSessionCreateRequestPtr request = message::PeerToFinderSessionCreateRequest::create();

        request->finderID(services::IHelper::randomString(20));
        request->contactID(peerPublic->getContactID());
        request->expires(IAccount::getAdjustedTime() + Seconds(600));

        SecureByteBlock hash(32);

        SHA256 shaProof;
        shaProof.Update((const BYTE *)"proof:", strlen("proof:"));
        shaProof.Update(mLocationSalt, mLocationSalt.size());
        shaProof.Update((const BYTE *)":", strlen(":"));
        String findSecret = peerPublic->getFindSecret();
        shaProof.Update((const BYTE *)findSecret.c_str(), findSecret.size());
        shaProof.Final(hash);

        char buffer[256];
        memset(&(buffer[0]), 0, sizeof(buffer));

        gethostname(&(buffer[0]), (sizeof(buffer)*sizeof(char))-sizeof(char));

        Location location;
        location.mID = mLocationID;
        location.mIPAddress = ipAddress;
        location.mUserAgent = outer->getUserAgent();
        location.mOS = outer->getOS();
        location.mSystem = outer->getSystem();
        location.mHost = &(buffer[0]);
        location.mLocationSalt = convertToBase64(mLocationSalt, mLocationSalt.size());
        location.mLocationFindSecretProof = convertToBase64(hash, hash.size());

        request->location(location);

        ZS_LOG_DEBUG(log("sending create session"))
        mSessionCreateRequester = sendRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_SESSION_CREATE_REQUEST_TIMEOUT_IN_SECONDS), false);
      }

      //-----------------------------------------------------------------------
      void AccountFinder::setState(AccountFinderStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("current state changed") + ", old=" + toString(mCurrentState) + ", new=" + toString(state))
        mCurrentState = state;

        if (!mDelegate) return;

        AccountFinderPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onAccountFinderStateChanged(mThisWeak.lock(), state);
          } catch(IAccountFinderDelegateProxy::Exceptions::DelegateGone &) {
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
