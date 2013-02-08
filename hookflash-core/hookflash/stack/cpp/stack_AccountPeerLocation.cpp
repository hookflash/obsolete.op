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
#include <hookflash/stack/internal/stack_AccountPeerLocation.h>
#include <hookflash/stack/internal/stack_AccountFinder.h>
#include <hookflash/stack/internal/stack_MessageRequester.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/message/PeerToPeerPeerIdentifyRequest.h>
#include <hookflash/stack/message/PeerToPeerPeerIdentifyResult.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindResult.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindReply.h>
#include <hookflash/stack/message/PeerKeepAliveRequest.h>
#include <hookflash/stack/message/PeerKeepAliveResult.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>

#ifndef _WIN32
#include <unistd.h>
#endif //_WIN32

#define HOOKFLASH_STACK_PEER_TO_PEER_RUDP_CONNECTION_INFO "text/x-openpeer-xml-plain"
#define HOOKFLASH_STACK_PEER_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS (2*60)
#define HOOKFLASH_STACK_CONNECTION_MANAGER_PEER_IDENTIFY_EXPIRES_IN_SECONDS (2*60)

#define HOOKFLASH_STACK_ACCOUNT_PEER_LOCATION_SEND_ICE_KEEP_ALIVE_INDICATIONS_IN_SECONDS (15)
#define HOOKFLASH_STACK_ACCOUNT_PEER_LOCATION_EXPECT_SESSION_DATA_IN_SECONDS (50)

#define HOOKFLASH_STACK_ACCOUNT_PEER_LOCATION_BACKGROUNDING_TIMEOUT_IN_SECONDS (HOOKFLASH_STACK_ACCOUNT_PEER_LOCATION_EXPECT_SESSION_DATA_IN_SECONDS + 40)

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::String String;
      typedef zsLib::BYTE BYTE;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::CSTR CSTR;
      typedef zsLib::IPAddress IPAddress;
      typedef zsLib::Time Time;
      typedef zsLib::Seconds Seconds;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::Document Document;
      typedef services::IICESocket IICESocket;
      typedef services::IRUDPICESocket IRUDPICESocket;
      typedef services::IRUDPMessaging IRUDPMessaging;
      typedef services::IRUDPICESocketSession IRUDPICESocketSession;
      typedef services::IRUDPICESocketPtr IRUDPICESocketPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountPeerLocation
      #pragma mark

      //-----------------------------------------------------------------------
      const char *IAccountPeerLocation::toString(IAccountPeerLocation::AccountPeerLocationStates state)
      {
        switch (state) {
          case AccountPeerLocationState_Pending:      return "Pending";
          case AccountPeerLocationState_Ready:        return "Ready";
          case AccountPeerLocationState_ShuttingDown: return "Shutting down";
          case AccountPeerLocationState_Shutdown:     return "Shutdown";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation
      #pragma mark

      //-----------------------------------------------------------------------
      AccountPeerLocation::AccountPeerLocation(IMessageQueuePtr queue) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(AccountPeerLocationState_Pending),
        mShouldRefindNow(false),
        mLastActivity(zsLib::now())
      {
        ZS_LOG_BASIC(log("created"))
      }

      //---------------------------------------------------------------------
      void AccountPeerLocation::init()
      {
        ZS_LOG_DEBUG(log("initialized") + ", contact ID=" + mContactID + ", location ID=" + mLocation.mID + ", has remote public peer file=" + (mPeerFilePublic ? "true" : "false"))
      }

      //---------------------------------------------------------------------
      AccountPeerLocation::~AccountPeerLocation()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //---------------------------------------------------------------------
      AccountPeerLocationPtr AccountPeerLocation::create(
                                                         IMessageQueuePtr queue,
                                                         IAccountPeerLocationDelegatePtr delegate,
                                                         IAccountForAccountPeerLocationPtr outer,
                                                         IPeerFilePublicPtr publicPeerFile,
                                                         const char *contactID,
                                                         const Location &location
                                                         )
      {
        AccountPeerLocationPtr pThis(new AccountPeerLocation(queue));
        pThis->mThisWeak = pThis;
        pThis->mDelegate = IAccountPeerLocationDelegateProxy::createWeak(delegate);
        pThis->mOuter = outer;
        pThis->mPeerFilePublic = publicPeerFile;
        pThis->mContactID = contactID;
        pThis->mLocation = location;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation => IPeerLocation
      #pragma mark

      //-----------------------------------------------------------------------
      String AccountPeerLocation::getContactID()
      {
        AutoRecursiveLock lock(getLock());
        return mContactID;
      }

      //-----------------------------------------------------------------------
      String AccountPeerLocation::getLocationID()
      {
        AutoRecursiveLock lock(getLock());
        return mLocation.mID;
      }

      //-----------------------------------------------------------------------
      Location AccountPeerLocation::getLocation()
      {
        AutoRecursiveLock lock(getLock());
        return mLocation;
      }

      //-----------------------------------------------------------------------
      IPeerFilePublicPtr AccountPeerLocation::getPeerFilePublic() const
      {
        AutoRecursiveLock lock(getLock());
        return mPeerFilePublic;
      }

      //-----------------------------------------------------------------------
      bool AccountPeerLocation::sendPeerMesage(message::MessagePtr message)
      {
        return sendPeerMesage(message, true);
      }

      //-----------------------------------------------------------------------
      IMessageRequesterPtr AccountPeerLocation::sendPeerRequest(
                                                                IMessageRequesterDelegatePtr delegate,
                                                                message::MessagePtr requestMessage,
                                                                Duration timeout
                                                                )
      {
        return sendPeerRequest(delegate, requestMessage, timeout, true);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation => IAccountPeerLocation
      #pragma mark

      //-----------------------------------------------------------------------
      const String &AccountPeerLocation::getContactID() const
      {
        AutoRecursiveLock lock(getLock());
        return mContactID;
      }

      //-----------------------------------------------------------------------
      const String &AccountPeerLocation::getLocationID() const
      {
        AutoRecursiveLock lock(getLock());
        return mLocation.mID;
      }

      //-----------------------------------------------------------------------
      const Location &AccountPeerLocation::getLocation() const
      {
        AutoRecursiveLock lock(getLock());
        return mLocation;
      }

      //-----------------------------------------------------------------------
      IPeerLocationPtr AccountPeerLocation::convertIPeerLocation()
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::shutdown()
      {
        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      IAccountPeerLocation::AccountPeerLocationStates AccountPeerLocation::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      bool AccountPeerLocation::shouldRefindNow() const
      {
        AutoRecursiveLock lock(getLock());
        bool refind = mShouldRefindNow;
        mShouldRefindNow = false;
        return refind;
      }

      //-----------------------------------------------------------------------
      bool AccountPeerLocation::isConnected() const
      {
        AutoRecursiveLock lock(getLock());
        return isReady();
      }

      //-----------------------------------------------------------------------
      Time AccountPeerLocation::getTimeOfLastActivity() const
      {
        AutoRecursiveLock lock(getLock());
        return mLastActivity;
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::connectLocation(
                                                const CandidateList &candidates,
                                                ICEControls control
                                                )
      {
        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("received request to connect but location is already shutting down/shutdown"))
          return;
        }

        IRUDPICESocketPtr socket = getSocket();
        if (!socket) {
          ZS_LOG_WARNING(Detail, log("no ICE socket found for peer location"))
          return;
        }

        ZS_LOG_DETAIL(log("creating session from remote candidates") + ", total candidates=" + Stringize<size_t>(candidates.size()).string())

        mSocketSession = socket->createSessionFromRemoteCandidates(mThisWeak.lock(), candidates, control);

        if (mSocketSession) {
          ZS_LOG_DEBUG(log("setting keep alive properties for socket session"))
          mSocketSession->setKeepAliveProperties(
                                                 Seconds(HOOKFLASH_STACK_ACCOUNT_PEER_LOCATION_SEND_ICE_KEEP_ALIVE_INDICATIONS_IN_SECONDS),
                                                 Seconds(HOOKFLASH_STACK_ACCOUNT_PEER_LOCATION_EXPECT_SESSION_DATA_IN_SECONDS),
                                                 Duration(),
                                                 Seconds(HOOKFLASH_STACK_ACCOUNT_PEER_LOCATION_BACKGROUNDING_TIMEOUT_IN_SECONDS)
                                                 );
        } else {
          ZS_LOG_ERROR(Detail, log("failed to create socket session"))
        }

        (IAccountPeerLocationAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::incomingRespondWhenCandidatesReady(message::PeerToFinderPeerLocationFindRequestPtr request)
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;

        mPendingRequests.push_back(request);
        (IAccountPeerLocationAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
      }

      //-----------------------------------------------------------------------
      bool AccountPeerLocation::hasReceivedCandidateInformation() const
      {
        AutoRecursiveLock lock(getLock());
        return mSocketSession;
      }

      //-----------------------------------------------------------------------
      bool AccountPeerLocation::sendMessage(
                                            DocumentPtr document,
                                            bool onlySendIfReady
                                            )
      {
        AutoRecursiveLock lock(getLock());

        if (onlySendIfReady) {
          if (!isReady()) {
            ZS_LOG_WARNING(Detail, log("attempted to send a message but the location is not ready"))
            return false;
          }
        }

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("attempted to send a message but the location is shutdown"))
          return false;
        }

        if (!mMessaging) {
          ZS_LOG_WARNING(Detail, log("requested to send a message but messaging is not ready"))
          return false;
        }

        boost::shared_array<char> output;
        ULONG length = 0;
        output = document->write(&length);

        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log("> > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > >"))
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log("PEER SEND MESSAGE") + "=" + "\n" + ((CSTR)(output.get())) + "\n")
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
        ZS_LOG_DETAIL(log("> > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > > >"))
        ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

        ZS_LOG_DETAIL(log("v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v"))
        ZS_LOG_DETAIL(log("||| MESSAGE INFO |||") + " to contact ID=" + mContactID + ", to location ID=" + mLocation.mID)
        ZS_LOG_DETAIL(log("^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^"))

        mLastActivity = zsLib::now();
        mMessaging->send((const BYTE *)(output.get()), length);
        return true;
      }

      //-----------------------------------------------------------------------
      bool AccountPeerLocation::sendPeerMesage(
                                               message::MessagePtr message,
                                               bool onlySendIfReady
                                               )
      {
        IAccountForAccountPeerLocationPtr outer = getOuter();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot send peer message as account is gone"))
          return false;
        }

        DocumentPtr doc = message->encode(outer->getPeerFiles());
        return sendMessage(doc, onlySendIfReady);
      }

      //-----------------------------------------------------------------------
      IMessageRequesterPtr AccountPeerLocation::sendPeerRequest(
                                                                IMessageRequesterDelegatePtr delegate,
                                                                message::MessagePtr requestMessage,
                                                                Duration timeout,
                                                                bool onlySendIfReady
                                                                )
      {
        IMessageRequesterPtr requester = IMessageRequester::monitorRequest(delegate, requestMessage, timeout);
        if (!requester) return IMessageRequesterPtr();

        bool result = sendPeerMesage(requestMessage, onlySendIfReady);
        if (result) return requester;

        IMessageRequesterForAccountPeerLocationPtr requesterForPeer = IMessageRequesterForAccountPeerLocation::convert(requester);

        // notify that the message requester failed to send the message...
        requesterForPeer->notifyMessageSendFailed();

        return requester;
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::sendKeepAlive()
      {
        AutoRecursiveLock lock(getLock());

        if (mKeepAliveRequester) {
          ZS_LOG_WARNING(Detail, log("keep alive requester is already in progress"))
          return;
        }

        IAccountForAccountPeerLocationPtr outer = getOuter();
        if (!outer) {
          ZS_LOG_ERROR(Debug, log("stack account appears to be gone thus cannot keep alive"))
          return;
        }

        message::PeerKeepAliveRequestPtr request = message::PeerKeepAliveRequest::create();

        mKeepAliveRequester = sendPeerRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_PEER_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS), false);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation => IAccountPeerLocationAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountPeerLocation::onStep()
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation => IRUDPICESocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountPeerLocation::onRUDPICESocketStateChanged(
                                                            IRUDPICESocketPtr socket,
                                                            RUDPICESocketStates state
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
      #pragma mark AccountPeerLocation => IRUDPICESocketSessionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountPeerLocation::onRUDPICESocketSessionStateChanged(
                                                                   IRUDPICESocketSessionPtr session,
                                                                   RUDPICESocketSessionStates state
                                                                   )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!session)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("received socket session state after already shutdown") + ", session ID=" + Stringize<PUID>(session->getID()).string())
          return;
        }

        if (session != mSocketSession) {
          ZS_LOG_WARNING(Detail, log("received socket session state changed from an obsolete session") + ", session ID=" + Stringize<PUID>(session->getID()).string())
          return;
        }

        if ((IRUDPICESocketSession::RUDPICESocketSessionState_ShuttingDown == state) ||
            (IRUDPICESocketSession::RUDPICESocketSessionState_Shutdown == state))
        {
          IRUDPICESocketSession::RUDPICESocketSessionShutdownReasons reason = session->getShutdownReason();
          ZS_LOG_WARNING(Detail, log("notified RUDP ICE socket session is shutdown") + ", reason=" + IRUDPICESocketSession::toString(reason))
          if ((IRUDPICESocketSession::RUDPICESocketSessionShutdownReason_Timeout == reason) ||
              (IRUDPICESocketSession::RUDPICESocketSessionShutdownReason_BackgroundingTimeout)) {
            mShouldRefindNow = true;
          }
          cancel();
          return;
        }

        step();
      }

      //---------------------------------------------------------------------
      void AccountPeerLocation::onRUDPICESocketSessionChannelWaiting(IRUDPICESocketSessionPtr session)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!session)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) return;
        if (isShuttingDown()) {
          ZS_LOG_WARNING(Debug, log("incoming RUDP session ignored during shutdown process"))
          return; // ignore incoming channels during the shutdown
        }

        if (session != mSocketSession) return;

        if (mMessaging) {
          if (IRUDPMessaging::RUDPMessagingState_Connected != mMessaging->getState())
          {
            ZS_LOG_WARNING(Debug, log("incoming RUDP session replacing existing RUDP session since the outgoing RUDP session was not connected"))
            // out messagine was not connected so use this one instead since it is connected
            IRUDPMessagingPtr messaging = IRUDPMessaging::acceptChannel(getAssociatedMessageQueue(), mSocketSession, mThisWeak.lock());
            if (!messaging) return;

            if (HOOKFLASH_STACK_PEER_TO_PEER_RUDP_CONNECTION_INFO != messaging->getRemoteConnectionInfo()) {
              ZS_LOG_WARNING(Detail, log("received unknown incoming connection type thus shutting down incoming connection") + ", type=" + messaging->getRemoteConnectionInfo())
              messaging->shutdown();
              return;
            }

            if (mIdentityRequester) {
              mIdentityRequester->cancel();
              mIdentityRequester.reset();
            }

            mMessaging->shutdown();
            mMessaging.reset();

            mMessaging = messaging;

            step();
            return;
          }

          ZS_LOG_WARNING(Detail, log("incoming RUDP session ignored since an outgoing RUDP session is already established"))

          // we already have a connected channel, so dump this one...
          IRUDPMessagingPtr messaging = IRUDPMessaging::acceptChannel(getAssociatedMessageQueue(), mSocketSession, mThisWeak.lock());
          messaging->shutdown();

          ZS_LOG_WARNING(Detail, log("sending keep alive as it is likely the remote party connection request is valid and our current session is stale"))
          sendKeepAlive();
          return;
        }

        ZS_LOG_DEBUG(log("incoming RUDP session being answered"))

        // no messaging present, accept this incoming channel
        mMessaging = IRUDPMessaging::acceptChannel(getAssociatedMessageQueue(), mSocketSession, mThisWeak.lock());

        if (HOOKFLASH_STACK_PEER_TO_PEER_RUDP_CONNECTION_INFO != mMessaging->getRemoteConnectionInfo()) {
          ZS_LOG_WARNING(Detail, log("received unknown incoming connection type thus shutting down incoming connection") + ", type=" + mMessaging->getRemoteConnectionInfo())

          mMessaging->shutdown();
          mMessaging.reset();
          return;
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation => IRUDPMessagingDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void AccountPeerLocation::onRUDPMessagingStateChanged(
                                                            IRUDPMessagingPtr messaging,
                                                            RUDPMessagingStates state
                                                            )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;

        if (messaging != mMessaging) {
          ZS_LOG_WARNING(Detail, log("received messaging state changed from an obsolete RUDP messaging") + ", messaging ID=" + Stringize<PUID>(messaging->getID()).string())
          return;
        }

        if ((IRUDPMessaging::RUDPMessagingState_ShuttingDown == state) ||
            (IRUDPMessaging::RUDPMessagingState_Shutdown == state))
        {
          IRUDPMessaging::RUDPMessagingShutdownReasons reason = messaging->getShutdownReason();
          ZS_LOG_WARNING(Detail, log("notified messaging shutdown") + ", reason=" + IRUDPMessaging::toString(reason))
          if (IRUDPMessaging::RUDPMessagingShutdownReason_Timeout == reason) {
            mShouldRefindNow = true;
          }
          cancel();
          return;
        }

        step();
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::onRUDPMessagingReadReady(IRUDPMessagingPtr session)
      {
        typedef IRUDPMessaging::MessageBuffer MessageBuffer;

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) return;

        if (session != mMessaging) {
          ZS_LOG_WARNING(Debug, log("messaging ready ready arrived for obsolete messaging"))
          return;
        }

        while (true) {
          MessageBuffer buffer = mMessaging->getBufferLargeEnoughForNextMessage();
          if (!buffer) return;

          mMessaging->receive(buffer.get());

          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("< < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < <"))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("PEER RECEIVED MESSAGE=") + "\n" + ((CSTR)(buffer.get())) + "\n")
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("< < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < < <"))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

          mLastActivity = zsLib::now();

          DocumentPtr document = Document::create();
          document->parse((CSTR)(buffer.get()));
          message::MessagePtr message = message::Message::create(document);

          if (!message) {
            ZS_LOG_WARNING(Detail, log("failed to create a message object from incoming message"))
            return;
          }

          ZS_LOG_DETAIL(log("v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v"))
          ZS_LOG_DETAIL(log("||| MESSAGE INFO |||") + " from contact ID=" + mContactID + ", from location ID=" + mLocation.mID + ", type=" + message::Message::toString(message->messageType()) + ", method=" + message->methodAsString())
          ZS_LOG_DETAIL(log("^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^"))

          if (IMessageRequester::handleMessage(message)) {
            ZS_LOG_DEBUG(log("handled message via message handler"))
            return;
          }

          // this is something new/incoming from the remote server...
          IAccountForAccountPeerLocationPtr outer = getOuter();
          if (!outer) {
            ZS_LOG_WARNING(Detail, log("peer message is ignored since account is now gone"))
            return;
          }

          if (message::Message::MessageType_Request == message->messageType()) {
            if (message::MessageFactoryStack::Method_PeerKeepAlive == (message::MessageFactoryStack::Methods)message->method()) {
              ZS_LOG_DEBUG(log("handling incoming peer keep alive request"))

              message::PeerKeepAliveRequestPtr request = message::PeerKeepAliveRequest::convert(message);

              message::PeerKeepAliveResultPtr actualResult = message::PeerKeepAliveResult::create(request);

              message::MessagePtr result = actualResult;
              if (!mPeerFilePublic) {
                result = message::MessageResult::create(request, 403, "Forbidden");
              }

              sendMessage(result->encode(outer->getPeerFiles()), false);
              return;
            }

            if (message::MessageFactoryStack::Method_PeerToPeer_PeerIdentify == (message::MessageFactoryStack::Methods)message->method()) {
              ZS_LOG_DEBUG(log("handling peer identify message"))

              message::PeerToPeerPeerIdentifyRequestPtr request = message::PeerToPeerPeerIdentifyRequest::convert(message);

              mPeerFilePublic = request->peerFilePublic();

              // we now go into the ready state
              setState(AccountPeerLocationState_Ready);

              message::PeerToPeerPeerIdentifyResultPtr actualResult = message::PeerToPeerPeerIdentifyResult::create(request);

              message::MessagePtr result = actualResult;
              if (!mPeerFilePublic) {
                result = message::MessageResult::create(request, 403, "Forbidden");
              }

              sendMessage(result->encode(outer->getPeerFiles()), false);

              if (!mPeerFilePublic) {
                ZS_LOG_WARNING(Detail, log("peer identify didn't contain a proper public peer file"))
                cancel();
                return;
              }

              step();
              return;
            } else {
              ZS_LOG_DEBUG(log("unknown message, will forward message to account"))
            }
          }

          // can't process requests until the incoming is received, drop the message
          if (!mPeerFilePublic) {
            ZS_LOG_WARNING(Detail, log("unable to forward message to account since message arrived before peer was identified"))
            return;
          }

          // send the message to the outer (asynchronously)
          IAccountForAccountPeerLocationProxy::create(outer->getAssociatedMessageQueue(), outer)->onAccountPeerLocationIncomingMessage(mThisWeak.lock(), message);
        }
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::onRUDPMessagingWriteReady(IRUDPMessagingPtr session)
      {
        ZS_LOG_TRACE(log("RUDP messaging write ready (ignored)"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation => IMessageReuqester
      #pragma mark

      //-----------------------------------------------------------------------
      bool AccountPeerLocation::handleMessageRequesterMessageReceived(
                                                                      IMessageRequesterPtr requester,
                                                                      message::MessagePtr message
                                                                      )
      {
        ZS_LOG_DEBUG(log("handling message received to message requester") + ", id=" + requester->getMonitoredMessageID())

        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("received a reply to requester after peer location shutdown (reply is ignored)"))
          return false;
        }

        if (requester == mIdentityRequester) {
          ZS_LOG_DEBUG(log("identity requester completed"))
          (IAccountPeerLocationAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
          return true;
        }

        if (requester == mKeepAliveRequester) {
          ZS_LOG_DEBUG(log("keep alive completed"))
          mKeepAliveRequester->cancel();
          mKeepAliveRequester.reset();
          return true;
        }

        ZS_LOG_DEBUG(log("message recieved does not match requester (thus ignoring)") + ", id=" + requester->getMonitoredMessageID())
        return false;
      }

      //---------------------------------------------------------------------
      void AccountPeerLocation::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        ZS_LOG_WARNING(Detail, log("message requester timed out") + ", id=" + requester->getMonitoredMessageID())
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("received timeout on requester after peer location shutdown (timeout ignored)"))
          return;
        }

        if (requester == mIdentityRequester) {
          ZS_LOG_WARNING(Detail, log("identity requester failed"))
          // could not connect
          cancel();
          return;
        }

        if (requester == mKeepAliveRequester) {
          ZS_LOG_WARNING(Detail, log("keep alive request to peer failed"))
          cancel();
          return;
        }

        ZS_LOG_WARNING(Detail, log("a requester timed out but does not match any known requesters (thus ignored)"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &AccountPeerLocation::getLock() const
      {
        IAccountForAccountPeerLocationPtr outer = getOuter();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      IAccountForAccountPeerLocationPtr AccountPeerLocation::getOuter() const
      {
        return mOuter.lock();
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketPtr AccountPeerLocation::getSocket() const
      {
        IAccountForAccountPeerLocationPtr outer = getOuter();
        if (!outer) return IRUDPICESocketPtr();
        return outer->getSocket();
      }

      String AccountPeerLocation::log(const char *message)
      {
        return String("AccountPeerLocation [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::cancel()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("cancel called"))

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        setState(AccountPeerLocationState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mIdentityRequester) {
          mIdentityRequester->cancel();
          // mIdentityRequester.reset(); // do NOT reset, used to determine state of peer object
        }

        if (mKeepAliveRequester) {
          mKeepAliveRequester->cancel();
          mKeepAliveRequester.reset();
        }

        if (mSocketSubscription) {
          mSocketSubscription->cancel();
          mSocketSubscription.reset();
        }

        if (mGracefulShutdownReference) {

          if (mMessaging) {
            ZS_LOG_DEBUG(log("requesting messaging shutdown"))
            mMessaging->shutdown();

            if (IRUDPMessaging::RUDPMessagingState_Shutdown != mMessaging->getState()) {
              ZS_LOG_DEBUG(log("waiting for RUDP messaging to shutdown"))
              return;
            }
          }

          if (mSocketSession) {
            ZS_LOG_DEBUG(log("requesting RUDP socket session shutdown"))
            mSocketSession->shutdown();

            if (IRUDPICESocketSession::RUDPICESocketSessionState_Shutdown != mSocketSession->getState()) {
              ZS_LOG_DEBUG(log("waiting for RUDP ICE socket session to shutdown"))
              return;
            }
          }

        }

        setState(AccountPeerLocationState_Shutdown);

        mGracefulShutdownReference.reset();
        mDelegate.reset();

        mPeerFilePublic.reset();

        mPendingRequests.clear();

        if (mMessaging) {
          ZS_LOG_DEBUG(log("hard shutdown of RUDP messaging"))
          mMessaging->shutdown();
          mMessaging.reset();
        }

        if (mSocketSession) {
          ZS_LOG_DEBUG(log("hard shutdown of RUDP ICE socket session"))
          mSocketSession->shutdown();
          mSocketSession.reset();
        }

        ZS_LOG_DEBUG(log("cancel completed"))
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::step()
      {
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("step forwarding to cancel since shutting down/shutdown"))
          cancel();
          return;
        }

        IAccountForAccountPeerLocationPtr outer = getOuter();
        if (!outer) {
          ZS_LOG_ERROR(Debug, log("stack account appears to be gone thus shutting down"))
          cancel();
          return;
        }

        IRUDPICESocketPtr socket = getSocket();
        if (!socket) {
          ZS_LOG_ERROR(Debug, log("attempted to get RUDP ICE socket but ICE socket is gone"))
          cancel();
          return;
        }

        if (!mSocketSubscription) {
          mSocketSubscription = socket->subscribe(mThisWeak.lock());
          socket->wakeup();
        }

        if (!mSocketSubscription) {
          ZS_LOG_ERROR(Detail, log("failed to subscribe to the socket connection thus shutting down"))
          cancel();
          return;
        }

        if (IRUDPICESocket::RUDPICESocketState_Ready != socket->getState()) {
          ZS_LOG_DEBUG(log("waiting for RUDP ICE socket to wake up"))
          return;
        }

        if (mPendingRequests.size() > 0) {
          const String &locationID = outer->getFinderLocationID();
          bool ready = outer->isFinderReady();

          if ((locationID.isEmpty()) ||
              (!ready)) {
            ZS_LOG_WARNING(Detail, log("cannot respond to pending find request if the finder is not ready thus shutting down"))
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

          char buffer[256];
          memset(&(buffer[0]), 0, sizeof(buffer));

          gethostname(&(buffer[0]), (sizeof(buffer)*sizeof(char))-sizeof(char));

          while (mPendingRequests.size() > 0) {
            message::PeerToFinderPeerLocationFindRequestPtr pendingRequest = mPendingRequests.front();
            mPendingRequests.pop_front();

            CandidateList remoteCandidates;
            remoteCandidates = pendingRequest->location().mCandidates;

            // local candidates are now preparerd, the request can be answered
            message::PeerToFinderPeerLocationFindReplyPtr reply = message::PeerToFinderPeerLocationFindReply::create(pendingRequest);
            Location location;
            location.mID = locationID;
            location.mIPAddress = ipAddress;
            location.mUserAgent = outer->getUserAgent();
            location.mOS = outer->getOS();
            location.mSystem = outer->getSystem();
            location.mHost = &(buffer[0]);
            location.mCandidates = candidates;
            reply->location(location);

            if (mPendingRequests.size() == 0) {
              // this is the final location, use this location as the connection point as it likely to be the latest of all the requests (let's hope)
              connectLocation(remoteCandidates, IICESocket::ICEControl_Controlled);
            }

            outer->sendFinderMessage(reply);
          }
        }

        if (!mSocketSession) {
          ZS_LOG_DEBUG(log("waiting for a RUDP ICE socket session connection"))
          return;
        }

        if (IRUDPICESocketSession::RUDPICESocketSessionState_Ready != mSocketSession->getState()) {
          ZS_LOG_DEBUG(log("waiting for RUDP ICE socket session to complete"))
          return;
        }

        if (!mPeerFilePublic) {
          ZS_LOG_DEBUG(log("waiting for an incoming connection and identification"))
          return;
        }

        if (!mMessaging) {
          ZS_LOG_DEBUG(log("requesting messaging channel open"))
          mMessaging = IRUDPMessaging::openChannel(
                                                   getAssociatedMessageQueue(),
                                                   mSocketSession,
                                                   mThisWeak.lock(),
                                                   HOOKFLASH_STACK_PEER_TO_PEER_RUDP_CONNECTION_INFO
                                                   );

          if (!mMessaging) {
            ZS_LOG_DEBUG(log("unable to open a messaging channel to remote peer thus shutting down"))
            cancel();
            return;
          }
        }

        if (IRUDPMessaging::RUDPMessagingState_Connected != mMessaging->getState()) {
          ZS_LOG_DEBUG(log("waiting for the RUDP messaging to connect"))
          return;
        }

        if (isReady()) {
          // nothing more to do...
          return;
        }

        if (mIdentityRequester) {
          if (!mIdentityRequester->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for identification request to complete"))
            return;
          }

          ZS_LOG_DEBUG(log("account is ready"))
          setState(AccountPeerLocationState_Ready);
          return;
        }

        ZS_LOG_DETAIL(log("Peer Location connected, sending identity..."))

        // we have connected, perform the identity request since we made the connection outgoing...
        message::PeerToPeerPeerIdentifyRequestPtr request = message::PeerToPeerPeerIdentifyRequest::create();

        request->findSecret(mPeerFilePublic->getFindSecret());
        request->locationID(outer->getFinderLocationID());
        request->expires(IAccount::getAdjustedTime() + Seconds(HOOKFLASH_STACK_CONNECTION_MANAGER_PEER_IDENTIFY_EXPIRES_IN_SECONDS));
        request->userAgent(outer->getUserAgent());

        mIdentityRequester = sendPeerRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_CONNECTION_MANAGER_PEER_IDENTIFY_EXPIRES_IN_SECONDS), false);
      }

      //-----------------------------------------------------------------------
      void AccountPeerLocation::setState(AccountPeerLocationStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        if (!mDelegate) return;

        AccountPeerLocationPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onAccountPeerLocationStateChanged(mThisWeak.lock(), state);
          } catch (IAccountPeerLocationDelegateProxy::Exceptions::DelegateGone &) {
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
