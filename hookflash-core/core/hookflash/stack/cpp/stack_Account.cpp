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
#include <hookflash/stack/internal/stack_AccountPeerLocation.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_PeerSubscription.h>
#include <hookflash/stack/internal/stack_MessageRequester.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPublicationRepository.h>
#include <hookflash/stack/internal/stack_PublicationRepository.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindResult.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindReply.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/services/IRUDPICESocket.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

#include <algorithm>

//#define HOOKFLASH_STACK_SESSION_USERAGENT "hookflash/1.0.1001a (iOS/iPad)"
#define HOOKFLASH_STACK_PEER_LOCATION_FIND_TIMEOUT_IN_SECONDS (60*2)
#define HOOKFLASH_STACK_PEER_LOCATION_FIND_RETRY_IN_SECONDS (30)
#define HOOKFLASH_STACK_PEER_LOCATION_INACTIVITY_TIMEOUT_IN_SECONDS (10*60)
#define HOOKFLASH_STACK_PEER_LOCATION_KEEP_ALIVE_TIME_IN_SECONDS    (5*60)
#define HOOKFLASH_STACK_ACCOUNT_TIMER_FIRES_IN_SECONDS (15)
#define HOOKFLASH_STACK_ACCOUNT_TIMER_DETECTED_BACKGROUNDING_TIME_IN_SECONDS (40)
#define HOOKFLASH_STACK_ACCOUNT_PREVENT_LOCATION_SHUTDOWNS_AFTER_BACKGROUNDING_FOR_IN_SECONDS (15)
#define HOOKFLASH_STACK_ACCOUNT_FINDER_STARTING_RETRY_AFTER_IN_SECONDS (1)
#define HOOKFLASH_STACK_ACCOUNT_FINDER_MAX_RETRY_AFTER_TIME_IN_SECONDS (60)


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    typedef zsLib::Time Time;

    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::WORD WORD;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef zsLib::Timer Timer;
      typedef zsLib::TimerPtr TimerPtr;
      typedef zsLib::Seconds Seconds;
      typedef zsLib::IPAddress IPAddress;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef services::IICESocket IICESocket;
      typedef services::IRUDPICESocket IRUDPICESocket;
      typedef services::IRUDPICESocketPtr IRUDPICESocketPtr;
      typedef services::IICESocket::CandidateList CandidateList;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AdjustedTimeSingleton
      #pragma mark

      class AdjustedTimeSingleton
      {
      public:
        typedef zsLib::Time Time;
        typedef zsLib::Duration Duration;
        typedef zsLib::Seconds Seconds;

      protected:
        AdjustedTimeSingleton() :
          mDifference(Seconds(0))
        {
        }

      public:
        //---------------------------------------------------------------------
        static Time getAdjustedTime()
        {
          AdjustedTimeSingleton &global = getGlobal();
          return zsLib::now() + global.getAdjustment();
        }

        //---------------------------------------------------------------------
        static void setServerTime(Time time)
        {
          AdjustedTimeSingleton &global = getGlobal();
          global.adjustTime(time);
        }

      private:
        //---------------------------------------------------------------------
        static AdjustedTimeSingleton &getGlobal()
        {
          static AdjustedTimeSingleton global;
          return global;
        }

        //---------------------------------------------------------------------
        Duration getAdjustment()
        {
          AutoRecursiveLock lock(mLock);
          return mDifference;
        }

        //---------------------------------------------------------------------
        void adjustTime(Time timeServer)
        {
          AutoRecursiveLock lock(mLock);
          if (Time() == timeServer) {
            ZS_LOG_WARNING(Detail, "unable to adjust to server time as server time is not valid")
            return;
          }

          mDifference = (timeServer - zsLib::now());
        }

        RecursiveLock mLock;
        Duration mDifference;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static IConnectionSubscription::ConnectionStates toConnectionState(IAccountFinder::AccountFinderStates state)
      {
        switch (state) {
          case IAccountFinder::AccountFinderState_Pending:      return IConnectionSubscription::ConnectionState_Pending;
          case IAccountFinder::AccountFinderState_Ready:        return IConnectionSubscription::ConnectionState_Connected;
          case IAccountFinder::AccountFinderState_ShuttingDown: return IConnectionSubscription::ConnectionState_Disconnecting;
          case IAccountFinder::AccountFinderState_Shutdown:     return IConnectionSubscription::ConnectionState_Disconnected;
        }
        return IConnectionSubscription::ConnectionState_Disconnected;
      }

      //-----------------------------------------------------------------------
      static IConnectionSubscription::ConnectionStates toConnectionState(IAccountPeerLocation::AccountPeerLocationStates state)
      {
        switch (state) {
          case IAccountPeerLocation::AccountPeerLocationState_Pending:      return IConnectionSubscription::ConnectionState_Pending;
          case IAccountPeerLocation::AccountPeerLocationState_Ready:        return IConnectionSubscription::ConnectionState_Connected;
          case IAccountPeerLocation::AccountPeerLocationState_ShuttingDown: return IConnectionSubscription::ConnectionState_Disconnecting;
          case IAccountPeerLocation::AccountPeerLocationState_Shutdown:     return IConnectionSubscription::ConnectionState_Disconnected;
        }
        return IConnectionSubscription::ConnectionState_Disconnected;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForAccountFinder
      #pragma mark

      //-----------------------------------------------------------------------
      void IAccountForAccountFinder::adjustToServerTime(Time time)
      {
        AdjustedTimeSingleton::setServerTime(time);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::Account(IMessageQueuePtr queue) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mLocationID(services::IHelper::randomString(32)),
        mCurrentState(IAccount::AccountState_Pending),
        mLastError(IAccount::AccountError_None),
        mBlockLocationShutdownsUntil(zsLib::now()),
        mFinderRetryAfter(zsLib::now()),
        mLastRetryFinderAfterDuration(Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_STARTING_RETRY_AFTER_IN_SECONDS))
      {
        ZS_LOG_BASIC(log("created"))
        message::MessageFactoryStack::singleton();
      }

      //-----------------------------------------------------------------------
      void Account::init()
      {
        ZS_LOG_DEBUG(log("inited"))
        AutoRecursiveLock lock(getLock());

        step();
      }

      //-----------------------------------------------------------------------
      Account::~Account()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccount
      #pragma mark

      //-----------------------------------------------------------------------
      IAccountPtr Account::create(
                                  IMessageQueuePtr queue,
                                  IBootstrappedNetworkPtr bootstrapper,
                                  IAccountDelegatePtr delegate,
                                  IPeerFilesPtr peerFiles,
                                  const char * password,
                                  const char *deviceID,
                                  const char *userAgent,
                                  const char *os,
                                  const char *system
                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!userAgent);
        AccountPtr pThis(new Account(queue));
        pThis->mThisWeak = pThis;
        pThis->mDelegate = IAccountDelegateProxy::createWeak(delegate);
        pThis->mPeerFiles = peerFiles;
        pThis->mPassword = password;
        pThis->mDeviceID = deviceID;
        pThis->mUserAgent = zsLib::String(zsLib::Stringize<const char *>(userAgent).string());
        pThis->mOS = os;
        pThis->mSystem = system;
        pThis->mBootstrapper = IBootstrappedNetworkForAccount::convert(bootstrapper);
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IAccount::AccountStates Account::getState() const
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("get account state") + ", returning state=" + toString(mCurrentState))
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IAccount::AccountErrors Account::getLastError() const
      {
        AutoRecursiveLock lock(getLock());
        return mLastError;
      }

      //-----------------------------------------------------------------------
      IPublicationRepositoryPtr Account::getRepository() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mRepository) return IPublicationRepositoryPtr();
        return mRepository->convertIPublicationRepository();
      }

      //-----------------------------------------------------------------------
      bool Account::sendFinderMessage(message::MessagePtr message)
      {
        ZS_LOG_DEBUG(log("send finder message"))
        AutoRecursiveLock lock(getLock());

        if (!mFinder) {
          ZS_LOG_WARNING(Debug, log("finder not available"))
          return false;
        }

        if (AccountFinder::AccountFinderState_Ready != mFinder->getState()) {
          ZS_LOG_WARNING(Debug, log("finder not ready"))
          return false;
        }

        DocumentPtr doc = message->encode(mPeerFiles);
        mFinder->requestSendMessage(doc);
        return true;
      }

      //-----------------------------------------------------------------------
      IConnectionSubscriptionPtr Account::subscribeToAllConnections(IConnectionSubscriptionDelegatePtr delegate)
      {
        AutoRecursiveLock lock(getLock());
        ConnectionSubscriptionPtr subscription = ConnectionSubscription::create(mThisWeak.lock(), delegate);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Debug, log("subscription happened during shutdown") + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
          // during the graceful shutdown or post shutdown process, new subscriptions must not be created...
          subscription->cancel();
          return subscription;
        }

        mConnectionSubscriptions[subscription->getID()] = subscription;

        if (mFinder) {
          subscription->notifyFinderStateChanged(toConnectionState(mFinder->getState()));
        }

        for (PeerMap::iterator iter = mPeers.begin(); iter != mPeers.end(); ++iter) {
          PeerPtr &peer = (*iter).second;

          for (Peer::LocationIDToPeerLocationMap::iterator iterLocation = peer->mLocations.begin(); iterLocation != peer->mLocations.end(); ++iterLocation) {
            IAccountPeerLocationPtr &peerLocation = (*iterLocation).second;
            subscription->notifyPeerStateChanged(peerLocation->convertIPeerLocation(), toConnectionState(peerLocation->getState()));
          }
        }

        return subscription;
      }

      //-----------------------------------------------------------------------
      IPeerSubscriptionPtr Account::subscribePeerLocations(
                                                           IPeerFilePublicPtr remotePartyPublicPeerFile,
                                                           IPeerSubscriptionDelegatePtr delegate
                                                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!remotePartyPublicPeerFile)

        AutoRecursiveLock lock(getLock());

        String contactID = remotePartyPublicPeerFile->getContactID();
        ZS_LOG_DEBUG(log("subscribe peer to peer locations") + ", contact=" + contactID)

        IPeerSubscriptionForAccountPtr subscription = PeerSubscription::create(mThisWeak.lock(), contactID, delegate);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Debug, log("peer subscription happened during shutdown") + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
          // during the graceful shutdown or post shutdown process, new subscriptions must not be created...
          subscription->notifyAccountNotifyPeerSubscriptionShutdown();
          return subscription->convertIPeerSubscription();
        }

        PeerPtr peer;

        PeerMap::iterator found = mPeers.find(contactID);

        if (mPeers.end() == found) {
          // need to create a new peer from scratch and do the find
          peer = Peer::create();
          ZS_LOG_DEBUG(log("subscribing to new peer") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
          peer->mPeerFilePublic = remotePartyPublicPeerFile;
          mPeers[contactID] = peer;
        } else {
          peer = (*found).second;
          if (!peer->mPeerFilePublic) {
            ZS_LOG_DEBUG(log("peer did not have remote public peer file thus setting"))
            peer->mPeerFilePublic = remotePartyPublicPeerFile;
          }
          ZS_LOG_DEBUG(log("subscribing to existing peer") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
        }

        peer->mPeerSubscriptions[subscription] = subscription;
        if (peer->mLocations.size() > 0) {
          // check to see if any of the locations are actually connected (we do not notify subscribers until a connection is established)
          bool established = false;

          for (Peer::LocationIDToPeerLocationMap::iterator iter = peer->mLocations.begin(); iter != peer->mLocations.end(); ++iter) {
            IAccountPeerLocationPtr &peerLocation = (*iter).second;
            if (peerLocation->isConnected()) {
              established = true;
              break;  // only one location found is enough to cause the event to fire
            }
          }

          // notify that the locations has changed since there is at least one session going on...
          if (established) {
            ZS_LOG_DEBUG(log("peer subscription notifying about existing connection") + ", peer=" + Stringize<PUID>(peer->mID).string())
            subscription->notifyPeerSubscriptionLocationsChanged();
          }
        }

        (IAccountAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
        return subscription->convertIPeerSubscription();
      }

      //-----------------------------------------------------------------------
      IPeerLocationPtr Account::getPeerLocation(
                                                const char *contactID,
                                                const char *locationID
                                                ) const
      {
        ZS_LOG_DEBUG(log("getting peer location") + ", contact=" + contactID + ", location=" + locationID)
        AutoRecursiveLock lock(getLock());

        PeerMap::const_iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) {
          ZS_LOG_DEBUG(log("request to fetch peer's location where peer is not being subscribed"))
          return IPeerLocationPtr();
        }

        const PeerPtr &peer = (*found).second;
        Peer::LocationIDToPeerLocationMap::const_iterator locationFound = peer->mLocations.find(locationID);
        if (locationFound == peer->mLocations.end()) {
          ZS_LOG_DEBUG(log("request to fetch peer's location but location is not known"))
          return IPeerLocationPtr();
        }

        const IAccountPeerLocationPtr &location = (*locationFound).second;

        return location->convertIPeerLocation();
      }

      //-----------------------------------------------------------------------
      void Account::hintAboutNewPeerLocation(
                                             const char *contactID,
                                             const char *locationID
                                             )
      {
        ZS_LOG_DEBUG(log("received hint about peer location") + ", contact=" + contactID + ", location=" + locationID)
        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_WARNING(Detail, log("hint about new location when shutting down/shutdown is ignored"))
          return;
        }

        PeerMap::iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) {
          ZS_LOG_WARNING(Detail, log("received hint about peer location that is not being subscribed"))
          return;  // ignore the hint since there is no subscription to this location
        }

        PeerPtr &peer = (*found).second;
        Peer::LocationIDToPeerLocationMap::iterator locationFound = peer->mLocations.find(locationID);
        if (locationFound != peer->mLocations.end()) {
          ZS_LOG_WARNING(Detail, log("received hint about peer location that is already known") + ", peer=" + Stringize<PUID>(peer->mID).string())
          return;  // thanks for the tip but we already know about this location...
        }

        // scope: see if we are in the middle of already searching for this peer location
        {
          Peer::FindingBecauseOfLocationIDMapList::iterator findingBecauseOfFound = peer->mPeerFindBecauseOfLocations.find(locationID);
          if (findingBecauseOfFound != peer->mPeerFindBecauseOfLocations.end()) {
            ZS_LOG_DEBUG(log("received hint about peer location for location that is already being searched because of a previous hint") + "], peer=" + Stringize<PUID>(peer->mID).string())
            return; // we've already received this tip...
          }
        }

        // scope: see if we already will redo a search because of this peer location
        {
          Peer::FindingBecauseOfLocationIDMapList::iterator findingBecauseOfFound = peer->mPeerFindNeedsRedoingBecauseOfLocations.find(locationID);
          if (findingBecauseOfFound != peer->mPeerFindNeedsRedoingBecauseOfLocations.end()) {
            ZS_LOG_WARNING(Detail, log("received hint about peer location for location that has already been given a hint") + ", peer=" + Stringize<PUID>(peer->mID).string())
            return; // we've already received this tip...
          }
        }

        // we will redo the search after this has completed because there are more locations that need to be found - THANKS FOR THE TIP!!
        peer->mPeerFindNeedsRedoingBecauseOfLocations[locationID] = locationID;
        peer->findTimeReset();

        ZS_LOG_DEBUG(log("received hint about peer location that will be added to hint search") + ", peer=" + Stringize<PUID>(peer->mID).string())
        (IAccountAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
      }

      //-----------------------------------------------------------------------
      void Account::shutdown()
      {
        ZS_LOG_DEBUG(log("requested to shutdown"))
        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForAccountFinder
      #pragma mark

      //-----------------------------------------------------------------------
      IRUDPICESocketPtr Account::getSocket() const
      {
        AutoRecursiveLock lock(getLock());
        return mSocket;
      }

      //-----------------------------------------------------------------------
      IBootstrappedNetworkForAccountFinderPtr Account::getBootstrapper() const
      {
        AutoRecursiveLock lock(getLock());
        return IBootstrappedNetworkForAccountFinder::convert(mBootstrapper);
      }

      //-----------------------------------------------------------------------
      IPeerFilesPtr Account::getPeerFiles() const
      {
        AutoRecursiveLock lock(getLock());
        return mPeerFiles;
      }

      //-----------------------------------------------------------------------
      const String &Account::getPassword() const
      {
        AutoRecursiveLock lock(getLock());
        return mPassword;
      }

      //-----------------------------------------------------------------------
      const String &Account::getLocationID() const
      {
        AutoRecursiveLock lock(getLock());
        return mLocationID;
      }

      //-----------------------------------------------------------------------
      const String &Account::getDeviceID() const
      {
        AutoRecursiveLock lock(getLock());
        return mDeviceID;
      }

      //-----------------------------------------------------------------------
      const String &Account::getUserAgent() const
      {
        AutoRecursiveLock lock(getLock());
        return mUserAgent;
      }

      //-----------------------------------------------------------------------
      const String &Account::getOS() const
      {
        AutoRecursiveLock lock(getLock());
        return mOS;
      }

      //-----------------------------------------------------------------------
      const String &Account::getSystem() const
      {
        AutoRecursiveLock lock(getLock());
        return mSystem;
      }

      //-----------------------------------------------------------------------
      void Account::notifyAccountFinderIncomingMessage(
                                                       IAccountFinderPtr finder,
                                                       message::MessagePtr message
                                                       )
      {
        AutoRecursiveLock lock(getLock());

        if (!message) {
          ZS_LOG_DEBUG(log("ignoring incoming message as message is NULL"))
          return;
        }

        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("ignoring incoming finder request because shutting down or shutdown"))
          return;
        }

        if (finder != mFinder) {
          ZS_LOG_DEBUG(log("finder does not match current finder (ignoring request)"))
          return;
        }

        if (message::Message::MessageType_Request == message->messageType()) {
          if (message::MessageFactoryStack::Method_PeerToFinder_PeerLocationFind == (message::MessageFactoryStack::Methods)message->method()) {

            ZS_LOG_DEBUG(log("notifying the account about the incoming peer find request"))
            message::PeerToFinderPeerLocationFindRequestPtr request = message::PeerToFinderPeerLocationFindRequest::convert(message);

            ZS_LOG_DEBUG(log("receiving incoming find peer location request"))

            Location location = request->location();
            String contactID = location.mContactID;

            ZS_LOG_DEBUG(log("received incoming peer find request") + ", contact ID=" + contactID + ", location=" + location.mID)

            PeerPtr peer;

            PeerMap::iterator foundPeer = mPeers.find(contactID);
            if (foundPeer != mPeers.end()) {
              peer = (*foundPeer).second;
              ZS_LOG_DEBUG(log("received incoming peer find request from known peer") + ", peer=" + Stringize<PUID>(peer->mID).string())
            } else {
              peer = Peer::create();
              mPeers[contactID] = peer;
              ZS_LOG_DEBUG(log("received incoming peer find request from unknown peer") + ", peer=" + Stringize<PUID>(peer->mID).string())
            }

            IAccountPeerLocationPtr peerLocation;
            Peer::LocationIDToPeerLocationMap::iterator foundLocation = peer->mLocations.find(location.mID);
            if (foundLocation != peer->mLocations.end()) {
              // already conducting a search for this location?
              peerLocation = (*foundLocation).second;
              ZS_LOG_DEBUG(log("received incoming peer find request from known peer location") + ", peer=" + Stringize<PUID>(peer->mID).string())

              if (peerLocation->hasReceivedCandidateInformation()) {
                ZS_LOG_WARNING(Detail, log("receiving candidate information for the same peer location thus shutting down current location") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", peer location object ID=" + Stringize<PUID>(peerLocation->getID()).string() + ", location=" + location.mID)
                peer->mLocations.erase(foundLocation);
                peerLocation->shutdown();

                // notify the subscribers about the state change
                for (ConnectionSubscriptionMap::iterator conIter = mConnectionSubscriptions.begin(); conIter != mConnectionSubscriptions.end(); )
                {
                  ConnectionSubscriptionMap::iterator current = conIter;
                  ++conIter;

                  ConnectionSubscriptionPtr &subscription = (*current).second;
                  subscription->notifyPeerStateChanged(peerLocation->convertIPeerLocation(), IConnectionSubscription::ConnectionState_Disconnected);
                }

                for (Peer::PeerSubscriptionMapList::iterator iter = peer->mPeerSubscriptions.begin(); iter != peer->mPeerSubscriptions.end(); ++iter) {
                  IPeerSubscriptionForAccountPtr &subscription = (*iter).second;

                  ZS_LOG_DEBUG(log("notifying subscription peer locations changed") + ", contact ID=" + contactID + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
                  subscription->notifyPeerSubscriptionLocationsChanged();
                }

                foundLocation = peer->mLocations.end();
                peerLocation.reset();
              }
            }

            if (foundLocation == peer->mLocations.end()) {
              peerLocation = AccountPeerLocation::create(
                                                         getAssociatedMessageQueue(),
                                                         mThisWeak.lock(),
                                                         mThisWeak.lock(),
                                                         IPeerFilePublicPtr(),
                                                         contactID,
                                                         location
                                                         );
              peer->mLocations[location.mID] = peerLocation;
              ZS_LOG_DEBUG(log("received incoming peer find request from unknown peer location") + ", peer=" + Stringize<PUID>(peer->mID).string())
            }

            peerLocation->incomingRespondWhenCandidatesReady(request);
            return;
          }
        }

        ULONG total = mConnectionSubscriptions.size();

        if (0 == total) {
          message::MessageResultPtr result = message::MessageResult::create(message, 404, "Not found");
          sendFinderMessage(result);
          return;
        }

        SubscriptionNotificationHelperPtr helper = SubscriptionNotificationHelper::create(mConnectionSubscriptions.size());

        // notify all the connection level subscriptions about the message
        for (ConnectionSubscriptionMap::iterator conIter = mConnectionSubscriptions.begin(); conIter != mConnectionSubscriptions.end(); )
        {
          ConnectionSubscriptionMap::iterator current = conIter;
          ++conIter;

          ConnectionSubscriptionPtr &subscriber = (*current).second;
          SubscriptionMessagePtr subscriptionMessage = SubscriptionMessage::createForFinder(subscriber, message, helper);
          subscriber->notifyIncomingMessage(subscriptionMessage);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForAccountPeerLocation
      #pragma mark

      //-----------------------------------------------------------------------
      bool Account::isFinderReady() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mFinder) return false;
        return IAccountFinder::AccountFinderState_Ready == mFinder->getState();
      }

      //-----------------------------------------------------------------------
      String Account::getFinderLocationID() const
      {
        AutoRecursiveLock lock(getLock());
        return mLocationID;
      }

      //-----------------------------------------------------------------------
      void Account::onAccountPeerLocationIncomingMessage(
                                                         IAccountPeerLocationPtr peerLocation,
                                                         MessagePtr message
                                                         )
      {
        AutoRecursiveLock lock(getLock());

        if ((isShuttingDown()) ||
            (isShutdown())) return;

        String contactID = peerLocation->getContactID();

        PeerMap::iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) return;

        PeerPtr peer = (*found).second;

        ULONG total = (peer->mPeerSubscriptions.size() + mConnectionSubscriptions.size());

        if (0 == total) {
          message::MessageResultPtr result = message::MessageResult::create(message, 404, "Not found");
          sendPeerMessage(contactID, peerLocation->getLocationID(), result);
          return;
        }

        SubscriptionNotificationHelperPtr helper = SubscriptionNotificationHelper::create(total);

        // notify the subscriptions for the peer
        for (Peer::PeerSubscriptionMapList::iterator iter = peer->mPeerSubscriptions.begin(); iter != peer->mPeerSubscriptions.end(); ++iter) {
          IPeerSubscriptionForAccountPtr &subscription = (*iter).second;

          ZS_LOG_DEBUG(log("notifying subscription about incoming message") + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
          SubscriptionMessagePtr subscriptionMessage = SubscriptionMessage::create(subscription->convertIPeerSubscription(), contactID, peerLocation->getLocationID(), message, helper);
          subscription->notifyPeerSubscriptionMessage(contactID, peerLocation->getLocationID(), subscriptionMessage);
        }

        // notify all the connection level subscriptions about the message
        for (ConnectionSubscriptionMap::iterator conIter = mConnectionSubscriptions.begin(); conIter != mConnectionSubscriptions.end(); )
        {
          ConnectionSubscriptionMap::iterator current = conIter;
          ++conIter;

          ConnectionSubscriptionPtr &subscriber = (*current).second;
          SubscriptionMessagePtr subscriptionMessage = SubscriptionMessage::createForPeer(subscriber, contactID, peerLocation->getLocationID(), message, helper);
          subscriber->notifyIncomingMessage(subscriptionMessage);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountFinderDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onAccountFinderStateChanged(
                                                IAccountFinderPtr finder,
                                                AccountFinderStates state
                                                )
      {
        ZS_THROW_BAD_STATE_IF(!finder)

        ZS_LOG_DETAIL(log("received notification finder state changed") + ", state=" + IAccountFinder::toString(state))
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) return;

        if (finder != mFinder) {
          ZS_LOG_WARNING(Detail, log("received state change on obsolete finder"))
          return;
        }

        Time tick = zsLib::now();

        // notify the subscribers
        for (ConnectionSubscriptionMap::iterator conIter = mConnectionSubscriptions.begin(); conIter != mConnectionSubscriptions.end(); )
        {
          ConnectionSubscriptionMap::iterator current = conIter;
          ++conIter;

          ConnectionSubscriptionPtr &subscription = (*current).second;
          subscription->notifyFinderStateChanged(toConnectionState(state));
        }

        if (IAccountFinder::AccountFinderState_Ready == state) {
          mLastRetryFinderAfterDuration = Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_STARTING_RETRY_AFTER_IN_SECONDS);
        }

        if ((IAccountFinder::AccountFinderState_ShuttingDown == state) ||
            (IAccountFinder::AccountFinderState_Shutdown == state)) {

          mFinder.reset();

          if (!isShuttingDown()) {
            ZS_LOG_WARNING(Detail, log("did not expect finder to shutdown"))

            mFinderRetryAfter = tick + mLastRetryFinderAfterDuration;
            mLastRetryFinderAfterDuration = mLastRetryFinderAfterDuration * 2;

            if (mLastRetryFinderAfterDuration > Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_MAX_RETRY_AFTER_TIME_IN_SECONDS)) {
              mLastRetryFinderAfterDuration = Seconds(HOOKFLASH_STACK_ACCOUNT_FINDER_MAX_RETRY_AFTER_TIME_IN_SECONDS);
            }
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountPeerLocationDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onAccountPeerLocationStateChanged(
                                                      IAccountPeerLocationPtr peerLocation,
                                                      AccountPeerLocationStates state
                                                      )
      {
        ZS_THROW_BAD_STATE_IF(!peerLocation)

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("notified account peer location state changed when account was shutdown") + ", peer ID=" + Stringize<PUID>(peerLocation->getID()).string() + ", peer state=" + IAccountPeerLocation::toString(state))
          return;
        }

        ZS_LOG_DETAIL(log("notified account peer location state changed") + ", peer ID=" + Stringize<PUID>(peerLocation->getID()).string() + ", peer state=" + IAccountPeerLocation::toString(state))

        Time tick = zsLib::now();

        String contactID = peerLocation->getContactID();
        String locationID = peerLocation->getLocationID();

        ZS_LOG_DEBUG(log("notified peer location is shutdown") + ", contact ID=" + contactID + ", location=" + locationID)

        PeerMap::iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) {
          ZS_LOG_WARNING(Debug, log("notified peer location is shutdown but peer was not found"))
          return;
        }

        PeerPtr &peer = (*found).second;
        Peer::LocationIDToPeerLocationMap::iterator foundLocation = peer->mLocations.find(locationID);
        if (foundLocation == peer->mLocations.end()) {
          ZS_LOG_WARNING(Debug, log("could not find peer location") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + locationID)
          return;
        }

        IAccountPeerLocationPtr &foundPeerLocation = (*foundLocation).second;
        if (foundPeerLocation != peerLocation) {
          ZS_LOG_WARNING(Detail, log("notification of peer state on obsolete peer location") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + locationID)
          return;
        }

        IConnectionSubscription::ConnectionStates connectionState = toConnectionState(state);

        // notify the subscribers about the state change
        for (ConnectionSubscriptionMap::iterator conIter = mConnectionSubscriptions.begin(); conIter != mConnectionSubscriptions.end(); )
        {
          ConnectionSubscriptionMap::iterator current = conIter;
          ++conIter;

          ConnectionSubscriptionPtr &subscription = (*current).second;
          subscription->notifyPeerStateChanged(peerLocation->convertIPeerLocation(), connectionState);
        }

        switch (state) {
          case IAccountPeerLocation::AccountPeerLocationState_Ready:
          {
            if (!peer->mPeerFilePublic) {
              ZS_LOG_DEBUG(log("attempting to get public peer file from peer location"))
              peer->mPeerFilePublic = peerLocation->getPeerFilePublic();
            }
            peer->mPreventCrazyRefindNextTime = false;

            for (Peer::PeerSubscriptionMapList::iterator iter = peer->mPeerSubscriptions.begin(); iter != peer->mPeerSubscriptions.end(); ++iter) {
              IPeerSubscriptionForAccountPtr &subscription = (*iter).second;

              ZS_LOG_DEBUG(log("notifying subscription peer locations changed") + ", contact ID=" + contactID + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
              subscription->notifyPeerSubscriptionLocationsChanged();
            }
            break;
          }
          case IAccountPeerLocation::AccountPeerLocationState_Shutdown:
          {
            bool findAgain = peerLocation->shouldRefindNow();

            // found the peer location, clear it out...
            peer->mLocations.erase(foundLocation);
            ZS_LOG_DEBUG(log("peer location is shutdown") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + locationID + ", total remaining locations=" + Stringize<size_t>(peer->mLocations.size()).string())

            if (findAgain) {
              ZS_LOG_DETAIL(log("need to refind peer at next opportunity") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + locationID)
              peer->mFindAtNextPossibleMoment = true;
            }

            if (peer->mLocations.size() < 1) {
              if (peer->mPreventCrazyRefindNextTime) {
                // prevent the peer from going into a crazy finding/connecting/remote side closed/find again loop
                if (peer->mNextScheduledFind < tick) {
                  ZS_LOG_WARNING(Detail, log("preventing crazy finding/connecting/remote side refused/find again loop for peer"))
                  peer->findTimeScheduleNext();
                }
              } else {
                ZS_LOG_DETAIL(log("will prevent crazy finding/connecting/remote side refused/find again loop for peer next time"))
                peer->mPreventCrazyRefindNextTime = true;
              }
            }

            for (Peer::PeerSubscriptionMapList::iterator iter = peer->mPeerSubscriptions.begin(); iter != peer->mPeerSubscriptions.end(); ++iter) {
              IPeerSubscriptionForAccountPtr &subscription = (*iter).second;

              ZS_LOG_DEBUG(log("notifying subscription peer locations changed") + ", contact ID=" + contactID + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
              subscription->notifyPeerSubscriptionLocationsChanged();
            }
            break;
          }
          default:  break;
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onBootstrappedNetworkStateChanged(
                                                      IBootstrappedNetworkPtr bootstrapper,
                                                      BootstrappedNetworkStates state
                                                      )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!bootstrapper)

        AutoRecursiveLock lock(getLock());

        if (IBootstrappedNetworkForAccount::convert(bootstrapper) != mBootstrapper) {
          ZS_LOG_DEBUG(log("bootstrapper stage change event came in from old bootstrapper (ignored)"))
          return;
        }

        if (IBootstrappedNetwork::BootstrappedNetworkState_Shutdown == mBootstrapper->getState()) {

          if ((!isShuttingDown()) &&
              (!isShutdown())) {
            ZS_LOG_ERROR(Detail, log("bootstrapper shutdown unexpectedly thus shutting down account"))
            setLastError(IAccount::AccountError_BootstrappedNetworkFailed);
            cancel();
            return;
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IRUDPICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onRUDPICESocketStateChanged(
                                                IRUDPICESocketPtr socket,
                                                RUDPICESocketStates state
                                                )
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("notification that RUDP ICE state changed after shutdown"))
          return;
        }

        if (socket != mSocket) {
          ZS_LOG_WARNING(Debug, log("notification about obsolete RUDP ICE socket"))
          return;
        }

        switch (state) {
          case IRUDPICESocket::RUDPICESocketState_Ready:
          {
            ZS_LOG_DEBUG(log("notification that RUDP ICE candidates are ready"))
            break;
          }
          case IRUDPICESocket::RUDPICESocketState_Shutdown:
          {
            ZS_LOG_DEBUG(log("notification that RUDP ICE has been shutdown"))
            if (!isShuttingDown()) {
              ZS_LOG_ERROR(Debug, log("notified RUDP ICE socket is shutdown unexpected"))
              setLastError(IAccount::AccountError_SocketUnexpectedlyClosed);
              cancel();
              return;
            }
            break;
          }
          default: break;
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IMessageReuqesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool Account::handleMessageRequesterMessageReceived(
                                                          IMessageRequesterPtr requester,
                                                          message::MessagePtr message
                                                          )
      {
        ZS_LOG_DEBUG(log("message requester received response") + ", id=" + requester->getMonitoredMessageID())
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("received response after already shutdown"))
          return false;
        }

        for (PeerMap::iterator iter = mPeers.begin(); iter != mPeers.end(); ++iter) {
          const String &contactID = (*iter).first;
          PeerPtr &peer = (*iter).second;

          if (requester != peer->mPeerFindRequester) {
            continue;
          }

          switch (message->messageType()) {
            case message::Message::MessageType_Result:  {
              message::MessageResultPtr result = message::MessageResult::convert(message);
              if (result->hasAttribute(message::MessageResult::AttributeType_ErrorCode)) {
                ZS_LOG_ERROR(Debug, log("received requester received response error") + ", error=" + Stringize<WORD>(result->errorCode()).string())
                handleFindRequestComplete(requester);
                return true;
              }

              if (message::MessageFactoryStack::Method_PeerToFinder_PeerLocationFind != (message::MessageFactoryStack::Methods)message->method()) {
                ZS_LOG_ERROR(Debug, log("received unexpected response (ignoring)"))
                return false;
              }

              message::PeerToFinderPeerLocationFindResultPtr findResult = message::PeerToFinderPeerLocationFindResult::convert(result);
              if (!findResult) {
                ZS_LOG_ERROR(Debug, log("was unable to cast to a find result (ignoring)"))
                return false;
              }

              // prepare all the locations in the result hat do not yet exist
              const LocationList &locations = findResult->locations();
              if (locations.size() < 1) {
                ZS_LOG_DEBUG(log("finder reply contained no location thus there are no location replies possible"))
                handleFindRequestComplete(requester);
                return true;
              }

              for (LocationList::const_iterator iterLocation = locations.begin(); iterLocation != locations.end(); ++iterLocation) {
                const Location &location = (*iterLocation);
                Peer::LocationIDToPeerLocationMap::iterator found = peer->mLocations.find(location.mID);
                if (found != peer->mLocations.end()) {
                  ZS_LOG_DEBUG(log("already knows about this location thus ignoring find result") + ", location=" + location.mID)
                  continue;   // already know about this location? if so just skip it
                }

                ZS_LOG_DEBUG(log("receiced find result for location") + ", location=" + location.mID)

                // scope: see if this would cause a seach redo later (if so, stop it from happening)
                {
                  Peer::FindingBecauseOfLocationIDMapList::iterator redoFound = peer->mPeerFindNeedsRedoingBecauseOfLocations.find(location.mID);
                  if (redoFound != peer->mPeerFindNeedsRedoingBecauseOfLocations.end()) {
                    ZS_LOG_DEBUG(log("receiced find result for location that would be searched later thus removing from later search"))
                    peer->mPeerFindNeedsRedoingBecauseOfLocations.erase(redoFound);
                  }
                }

                // don't know this location, remember it for later
                AccountPeerLocationPtr peerLocation = AccountPeerLocation::create(
                                                                                  getAssociatedMessageQueue(),
                                                                                  mThisWeak.lock(),
                                                                                  mThisWeak.lock(),
                                                                                  peer->mPeerFilePublic,
                                                                                  (*iter).first,
                                                                                  location
                                                                                  );
                peer->mLocations[location.mID] = peerLocation;

                // the act of finding a peer does not cause notification to the subscribers as only the establishment of a peer connection notifies the subscribers
              }
              break;
            }
            case message::Message::MessageType_Reply:   {
              if (message::MessageFactoryStack::Method_PeerToFinder_PeerLocationFind != (message::MessageFactoryStack::Methods)message->method()) {
                ZS_LOG_ERROR(Debug, log("receiced received a find reply but it was not for a peer location") + ", peer=" + Stringize<PUID>(peer->mID).string())
                return false;
              }
              message::PeerToFinderPeerLocationFindReplyPtr findReply = message::PeerToFinderPeerLocationFindReply::convert(message);
              if (!findReply) {
                ZS_LOG_ERROR(Debug, log("receiced received a find reply but was unable to cast object to a reply object") + ", peer=" + Stringize<PUID>(peer->mID).string())
                return false;
              }

              const Location &location = findReply->location();

              const CandidateList &candidates = location.mCandidates;
              if (candidates.size() < 1) {
                ZS_LOG_ERROR(Debug, log("receiced received a find reply but it did not contain any candidates (thus ignoring reply)") + ", peer=" + Stringize<PUID>(peer->mID).string())
                return false;
              }

              Peer::LocationIDToPeerLocationMap::iterator found = peer->mLocations.find(location.mID);

              IAccountPeerLocationPtr peerLocation;
              if (found != peer->mLocations.end()) {
                peerLocation = (*found).second;
                ZS_LOG_DEBUG(log("receiced received a find reply to an existing known peer location") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", peer location object ID=" + Stringize<PUID>(peerLocation->getID()).string() + ", location=" + location.mID)

                if (peerLocation->hasReceivedCandidateInformation()) {
                  ZS_LOG_WARNING(Detail, log("receiving candidate information for the same peer location thus shutting down current location") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", peer location object ID=" + Stringize<PUID>(peerLocation->getID()).string() + ", location=" + location.mID)
                  peer->mLocations.erase(found);
                  peerLocation->shutdown();

                  // notify the subscribers about the state change
                  for (ConnectionSubscriptionMap::iterator conIter = mConnectionSubscriptions.begin(); conIter != mConnectionSubscriptions.end(); )
                  {
                    ConnectionSubscriptionMap::iterator current = conIter;
                    ++conIter;

                    ConnectionSubscriptionPtr &subscription = (*current).second;
                    subscription->notifyPeerStateChanged(peerLocation->convertIPeerLocation(), IConnectionSubscription::ConnectionState_Disconnected);
                  }

                  for (Peer::PeerSubscriptionMapList::iterator iter = peer->mPeerSubscriptions.begin(); iter != peer->mPeerSubscriptions.end(); ++iter) {
                    IPeerSubscriptionForAccountPtr &subscription = (*iter).second;

                    ZS_LOG_DEBUG(log("notifying subscription peer locations changed") + ", contact ID=" + contactID + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
                    subscription->notifyPeerSubscriptionLocationsChanged();
                  }

                  found = peer->mLocations.end();
                  peerLocation.reset();
                }
              }

              if (found == peer->mLocations.end()) {
                ZS_LOG_DEBUG(log("receiced received a find reply to a new unknown peer") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + location.mID)

                peerLocation = AccountPeerLocation::create(
                                                           getAssociatedMessageQueue(),
                                                           mThisWeak.lock(),
                                                           mThisWeak.lock(),
                                                           peer->mPeerFilePublic,
                                                           (*iter).first,
                                                           location
                                                           );
                peer->mLocations[location.mID] = peerLocation;

                // scope: see if this would cause a seach redo later (if so, stop it from happening)
                {
                  Peer::FindingBecauseOfLocationIDMapList::iterator redoFound = peer->mPeerFindNeedsRedoingBecauseOfLocations.find(location.mID);
                  if (redoFound != peer->mPeerFindNeedsRedoingBecauseOfLocations.end()) {
                    ZS_LOG_DEBUG(log("receiced find reply for location that would be searched later thus removing from later search") + ", peer=" + Stringize<PUID>(peer->mID).string())
                    peer->mPeerFindNeedsRedoingBecauseOfLocations.erase(redoFound);
                  }
                }

                // in this case not only was a location found but the request to find the location came back from the remote party with a reply but it's not sufficient reason
                // to notify any subscribers since they only care when a connection is actually established.
              }

              peerLocation->connectLocation(candidates, IICESocket::ICEControl_Controlling);

              bool locationRemainThatHaveNotReplied = false;
              // scope: check to see if their are remaining locations yet to return their replies
              {
                for (Peer::LocationIDToPeerLocationMap::iterator iterLocation = peer->mLocations.begin(); iterLocation != peer->mLocations.end(); ++iterLocation) {
                  IAccountPeerLocationPtr &peerLocation = (*iterLocation).second;
                  if (!peerLocation->hasReceivedCandidateInformation()) {
                    ZS_LOG_DEBUG(log("at least one peer still waiting for result") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + peerLocation->getLocationID())
                    locationRemainThatHaveNotReplied = true;
                    break;
                  }
                }
              }

              if (locationRemainThatHaveNotReplied) {
                ZS_LOG_DEBUG(log("at least one location has not replied so waiting to see if the reply will eventually arrive..."))
                return false;
              }

              ZS_LOG_DEBUG(log("all known locations have already replied thus no need to continue search"))

              handleFindRequestComplete(requester);

              peer->findTimeReset();
              return true;
            }
            default: return false;  // ignore these since we aren't expecting them
          }
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void Account::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        ZS_LOG_WARNING(Detail, log("message requester timed out") + ", id=" + requester->getMonitoredMessageID())

        handleFindRequestComplete(requester);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForPeerSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onPeerSubscriptionShutdown(IPeerSubscriptionForAccountPtr subscription)
      {
        ZS_LOG_DEBUG(log("notifying subscription being closed") + ", subscription=" + Stringize<PUID>(subscription->getID()).string() + ", contact=" + subscription->convertIPeerSubscription()->getContactID())
        AutoRecursiveLock lock(getLock());

        PeerMap::iterator found = mPeers.find(subscription->convertIPeerSubscription()->getContactID());
        if (found == mPeers.end()) {
          ZS_LOG_WARNING(Debug, log("did not find peer to which subscription refers") + ", subscription=" + Stringize<PUID>(subscription->getID()).string() + ", contact=" + subscription->convertIPeerSubscription()->getContactID())
          return;
        }

        PeerPtr &peer = (*found).second;

        Peer::PeerSubscriptionMapList::iterator foundSubscription = peer->mPeerSubscriptions.find(subscription);
        if (foundSubscription == peer->mPeerSubscriptions.end()) {
          ZS_LOG_WARNING(Debug, log("did not find subscription for peer (probably okay)") + ", subscription=" + Stringize<PUID>(subscription->getID()).string() + ", contact=" + subscription->convertIPeerSubscription()->getContactID() + ", peer=" + Stringize<PUID>(peer->mID).string())
          return;
        }

        ZS_LOG_DEBUG(log("accepted subscription shutdown for peer") + ", subscription=" + Stringize<PUID>(subscription->getID()).string() + ", contact=" + subscription->convertIPeerSubscription()->getContactID() + ", peer=" + Stringize<PUID>(peer->mID).string())
        peer->mPeerSubscriptions.erase(foundSubscription);
        step();
      }

      //-----------------------------------------------------------------------
      bool Account::isFinding(const String &contactID) const
      {
        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("peers are never searched during shutdown"))
          return false;
        }

        PeerMap::const_iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) {
          ZS_LOG_DEBUG(log("find is not in progress to unknown peer") + ", contact ID=" + contactID)
          return false;
        }

        const PeerPtr &peer = (*found).second;
        if (peer->mPeerFindRequester) {
          ZS_LOG_DEBUG(log("find is currently in progress") + ", contact ID=" + contactID)
          return true;
        }

        if (shouldFind(contactID, peer)) {
          ZS_LOG_DEBUG(log("while a find is not technically in progress currently, it will be soon") + ", contact ID=" + contactID)
          return true;
        }

        ZS_LOG_DEBUG(log("no reason to do a find to this contact") + ", contact ID=" + contactID)
        return false;
      }

      //-----------------------------------------------------------------------
      void Account::getPeerLocations(
                                     const String &contactID,
                                     IPeerSubscription::LocationList &outLocations,
                                     bool includeOnlyConnectedLocations
                                     ) const
      {
        AutoRecursiveLock lock(getLock());
        PeerMap::const_iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) return;

        const PeerPtr &peer = (*found).second;
        for (Peer::LocationIDToPeerLocationMap::const_iterator iter = peer->mLocations.begin(); iter != peer->mLocations.end(); ++iter) {
          IAccountPeerLocationPtr peerLocation = (*iter).second;
          if ((peerLocation->isConnected()) ||
              (includeOnlyConnectedLocations)) {
            // only push back sessions that are actually connected
            outLocations.push_back(peerLocation->getLocation());
          }
        }
      }

      //-----------------------------------------------------------------------
      void Account::getPeerLocations(
                                     const String &contactID,
                                     IPeerSubscription::PeerLocationList &outLocations,
                                     bool includeOnlyConnectedLocations
                                     ) const
      {
        AutoRecursiveLock lock(getLock());
        PeerMap::const_iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) return;

        const PeerPtr &peer = (*found).second;
        for (Peer::LocationIDToPeerLocationMap::const_iterator iter = peer->mLocations.begin(); iter != peer->mLocations.end(); ++iter) {
          IAccountPeerLocationPtr peerLocation = (*iter).second;
          if ((peerLocation->isConnected()) ||
              (!includeOnlyConnectedLocations)) {
            // only push back sessions that are actually connected
            outLocations.push_back(peerLocation->convertIPeerLocation());
          }
        }
      }

      //-----------------------------------------------------------------------
      bool Account::sendPeerMessage(
                                    const String &contactID,
                                    const char *locationID,
                                    message::MessagePtr message
                                    )
      {
        AutoRecursiveLock lock(getLock());

        PeerMap::iterator found = mPeers.find(contactID);
        if (found == mPeers.end()) return false;

        PeerPtr &peer = (*found).second;
        Peer::LocationIDToPeerLocationMap::iterator foundLocation = peer->mLocations.find(locationID);
        if (foundLocation == peer->mLocations.end()) return false;

        IAccountPeerLocationPtr peerLocation = (*foundLocation).second;

        DocumentPtr doc = message->encode(mPeerFiles);
        return peerLocation->sendMessage(doc);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      String Account::getContactID() const
      {
        AutoRecursiveLock lock(getLock());
        return mPeerFiles->getPublic()->getContactID();
      }

      //-----------------------------------------------------------------------
      IMessageRequesterPtr Account::sendFinderRequest(
                                                      IMessageRequesterDelegatePtr delegate,
                                                      message::MessagePtr requestMessage,
                                                      Duration timeout
                                                      )
      {

        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("received request to send to finder (will forward)"))

        if (!mFinder) {
          ZS_LOG_WARNING(Detail, log("no finder present thus notifying request is cancelled immediately"))
          IMessageRequesterPtr requester = IMessageRequester::monitorRequest(delegate, requestMessage, timeout);
          IMessageRequesterForAccountPtr internalRequester = IMessageRequesterForAccount::convert(requester);

          internalRequester->notifyMessageSendFailed();
          return requester;
        }

        return mFinder->sendRequest(delegate, requestMessage, timeout);
      }

      //-----------------------------------------------------------------------
      IMessageRequesterPtr Account::sendPeerRequest(
                                                    IMessageRequesterDelegatePtr delegate,
                                                    message::MessagePtr requestMessage,
                                                    const char *contactID,
                                                    const char *locationID,
                                                    Duration timeout
                                                    )
      {
        AutoRecursiveLock lock(getLock());

        // scope: try to find peer and location and send to it
        {
          PeerMap::iterator found = mPeers.find(contactID);

          if (found == mPeers.end()) {
            ZS_LOG_WARNING(Detail, log("could not find the requested peer contact to send the request") + ", contact ID=" + contactID + ", location ID=" + locationID)
            goto send_peer_request_failure;
          }

          PeerPtr peer = (*found).second;

          Peer::LocationIDToPeerLocationMap::iterator foundLocation = peer->mLocations.find(locationID);
          if (foundLocation == peer->mLocations.end()) {
            ZS_LOG_WARNING(Detail, log("could not find the requested peer location to send the request") + ", contact ID=" + contactID + ", location ID=" + locationID)
            goto send_peer_request_failure;
          }

          IAccountPeerLocationPtr &peerLocation = (*foundLocation).second;

          return peerLocation->sendPeerRequest(delegate, requestMessage, timeout);
        }

      send_peer_request_failure:
        IMessageRequesterPtr requester = IMessageRequester::monitorRequest(delegate, requestMessage, timeout);
        IMessageRequesterForAccountPtr internalRequester = IMessageRequesterForAccount::convert(requester);
        internalRequester->notifyMessageSendFailed();
        return requester;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onStep()
      {
        ZS_LOG_DEBUG(log("step"))
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onTimer(TimerPtr timer)
      {
        ZS_LOG_TRACE(log("tick"))
        AutoRecursiveLock lock(getLock());

        if (timer != mTimer) {
          ZS_LOG_WARNING(Detail, log("received timer notification on obsolete timer (probably okay)") + ", timer ID=" + Stringize<PUID>(timer->getID()).string())
          return;
        }

        Time tick = zsLib::now();

        if (mLastTimerFired + Seconds(HOOKFLASH_STACK_ACCOUNT_TIMER_DETECTED_BACKGROUNDING_TIME_IN_SECONDS) < tick) {
          ZS_LOG_WARNING(Detail, log("account timer detected account when into background"))

          mBlockLocationShutdownsUntil = tick + Seconds(HOOKFLASH_STACK_ACCOUNT_PREVENT_LOCATION_SHUTDOWNS_AFTER_BACKGROUNDING_FOR_IN_SECONDS);
        }

        mLastTimerFired = tick;
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => friend ConnectionSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::notifyConnectionSubscriptionShutdown(ConnectionSubscriptionPtr subscription)
      {
        ConnectionSubscriptionMap::iterator found = mConnectionSubscriptions.find(subscription->getID());
        if (found == mConnectionSubscriptions.end()) return;
        mConnectionSubscriptions.erase(found);
      }

      //-----------------------------------------------------------------------
      IConnectionSubscription::ConnectionStates Account::getFinderConnectionState() const
      {
        if (!mFinder) return IConnectionSubscription::ConnectionState_Pending;
        return toConnectionState(mFinder->getState());
      }

      //-----------------------------------------------------------------------
      IConnectionSubscription::ConnectionStates Account::getPeerLocationConnectionState(
                                                                                        const char *contactID,
                                                                                        const char *locationID
                                                                                        ) const
      {
        PeerMap::const_iterator foundPeer = mPeers.find(contactID);
        if (foundPeer == mPeers.end()) return IConnectionSubscription::ConnectionState_Disconnected;

        const PeerPtr &peer = (*foundPeer).second;

        Peer::LocationIDToPeerLocationMap::const_iterator foundLocation = peer->mLocations.find(locationID);

        if (foundLocation == peer->mLocations.end()) {
          Peer::FindingBecauseOfLocationIDMapList::const_iterator foundFindDone = peer->mPeerFindBecauseOfLocations.find(locationID);
          if (foundFindDone != peer->mPeerFindBecauseOfLocations.end()) return IConnectionSubscription::ConnectionState_Pending;

          Peer::FindingBecauseOfLocationIDMapList::const_iterator foundRedoing = peer->mPeerFindNeedsRedoingBecauseOfLocations.find(locationID);
          if (foundRedoing != peer->mPeerFindNeedsRedoingBecauseOfLocations.end()) return IConnectionSubscription::ConnectionState_Pending;

          return IConnectionSubscription::ConnectionState_Disconnected;
        }

        const IAccountPeerLocationPtr &peerLocation = (*foundLocation).second;
        return toConnectionState(peerLocation->getState());
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Account::log(const char *message) const
      {
        return String("stack::Account [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void Account::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());  // just in case

        if (isShutdown()) return;

        setState(IAccount::AccountState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mRepository) {
          mRepository->cancel();
          mRepository.reset();
        }

        // scope: kill all the connection subscriptions
        {
          for (ConnectionSubscriptionMap::iterator conIter = mConnectionSubscriptions.begin(); conIter != mConnectionSubscriptions.end(); )
          {
            ConnectionSubscriptionMap::iterator current = conIter;
            ++conIter;

            ConnectionSubscriptionPtr &subscriber = (*current).second;
            subscriber->cancel();
          }
        }

        // scope: kill all the subscriptions and the find requests since they are not needed anymore
        {
          for (PeerMap::iterator iter = mPeers.begin(); iter != mPeers.end(); ++iter) {
            PeerPtr &peer = (*iter).second;

            if (peer->mPeerFindRequester) {
              setFindState(*(peer.get()), IPeerSubscription::PeerSubscriptionFindState_Completed);
              setFindState(*(peer.get()), IPeerSubscription::PeerSubscriptionFindState_Idle);
            }

            for (Peer::PeerSubscriptionMapList::iterator subscriptionIter = peer->mPeerSubscriptions.begin(); subscriptionIter != peer->mPeerSubscriptions.end(); ++subscriptionIter) {
              IPeerSubscriptionForAccountPtr &subscription = (*subscriptionIter).second;

              ZS_LOG_DEBUG(log("cancelling / shutting down subscription") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", subscription=" + Stringize<PUID>(subscription->getID()).string())
              subscription->notifyAccountNotifyPeerSubscriptionShutdown();
            }

            // after this point all subscriptions are considered "dead" and no new ones are allowed to be created
            peer->mPeerSubscriptions.clear();

            if (peer->mPeerFindRequester) {
              ZS_LOG_DEBUG(log("cancelling / stopping find request for peer") + ", peer=" + Stringize<PUID>(peer->mID).string())

              // we have to kill all requests to find more peer locations (simply ignore the results from the requests)
              peer->mPeerFindRequester->cancel();
              peer->mPeerFindRequester.reset();
            }
          }
        }

        if (mGracefulShutdownReference) {

          if (mPeers.size() > 0) {
            for (PeerMap::iterator peerIter = mPeers.begin(); peerIter != mPeers.end(); ) {
              PeerMap::iterator current = peerIter;
              ++peerIter;

              PeerPtr peer = (*current).second;

              if (peer->mLocations.size() > 0) {
                for (Peer::LocationIDToPeerLocationMap::iterator iterLocation = peer->mLocations.begin(); iterLocation != peer->mLocations.end(); ++iterLocation) {
                  // send a shutdown request to each outstanding location
                  IAccountPeerLocationPtr peerLocation = (*iterLocation).second;

                  ZS_LOG_DEBUG(log("cancel shutting down peer location") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + peerLocation->getLocationID())
                  peerLocation->shutdown();
                }
              } else {
                // we don't need to remember this connection anymore since no locations were found or established
                ZS_LOG_DEBUG(log("cancel closing down peer") + "], peer=" + Stringize<PUID>(peer->mID).string())
                mPeers.erase(current);
              }
            }
          }

          if (mFinder) {
            ZS_LOG_DEBUG(log("shutting down peer finder") + ", finder=" + Stringize<PUID>(mFinder->getID()).string())
            mFinder->shutdown();
          }

          if (mPeers.size() > 0) {
            ZS_LOG_DEBUG(log("shutdown still waiting for all peer locations to shutdown..."))
            return;
          }

          if (mFinder) {
            if (IAccountFinder::AccountFinderState_Shutdown != mFinder->getState()) {
              ZS_LOG_DEBUG(log("shutdown still waiting for finder to shutdown"))
              return;
            }
          }

          if (mSocket) {
            ZS_LOG_DEBUG(log("shutting down socket"))
            mSocket->shutdown();

            if (IRUDPICESocket::RUDPICESocketState_Shutdown != mSocket->getState()) {
              ZS_LOG_DEBUG(log("shutdown still waiting for RUDP socket to shutdown"))
              return;
            }
          }

        }

        setState(IAccount::AccountState_Shutdown);

        mGracefulShutdownReference.reset();

        mPassword.clear();

        // scope: clear out peers that have not had their locations shutdown
        {
          for (PeerMap::iterator iter = mPeers.begin(); iter != mPeers.end(); ++iter) {
            PeerPtr &peer = (*iter).second;

            for (Peer::LocationIDToPeerLocationMap::iterator iterLocation = peer->mLocations.begin(); iterLocation != peer->mLocations.end(); ++iterLocation) {
              IAccountPeerLocationPtr peerLocation = (*iterLocation).second;
              ZS_LOG_DEBUG(log("hard shutdown of peer location") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + peerLocation->getLocationID())
              peerLocation->shutdown();
            }
            peer->mLocations.clear();
          }

          mPeers.clear();
        }

        if (mTimer) {
          mTimer->cancel();
          mTimer.reset();
        }

        if (mFinder) {
          ZS_LOG_DEBUG(log("hard shutdown of peer finder"))
          mFinder->shutdown();
          mFinder.reset();
        }

        if (mSocket) {
          ZS_LOG_DEBUG(log("hard shutdown of socket"))
          mSocket->shutdown();
          mSocket.reset();
        }

        if (mBootstrapperSubscription) {
          ZS_LOG_DEBUG(log("bootstrapper subscription closed"))
          mBootstrapperSubscription->cancel();
          mBootstrapperSubscription.reset();
        }

        if (mBootstrapper) {
          ZS_LOG_DEBUG(log("bootstrapper closed"))
          mBootstrapper->close();
          mBootstrapper.reset();
        }

        mPeerFiles.reset();
        ZS_LOG_DEBUG(log("shutdown complete"))
      }

      //-----------------------------------------------------------------------
      void Account::step()
      {
        typedef zsLib::Time Time;

        if ((isShuttingDown()) ||
            (isShutdown())) {
          cancel();
          return;
        }

        if (!mTimer) {
          mLastTimerFired = zsLib::now();
          mTimer = Timer::create(mThisWeak.lock(), Seconds(HOOKFLASH_STACK_ACCOUNT_TIMER_FIRES_IN_SECONDS));
        }

        if (!mRepository) {
          mRepository = IPublicationRepositoryForAccount::create(mThisWeak.lock());
        }

        if (!mBootstrapper) {
          ZS_LOG_WARNING(Detail, log("bootstrapper is not valid thus shutting down"))
          setLastError(IAccount::AccountError_BootstrappedNetworkFailed);
          cancel();
          return;
        }

        if (!mBootstrapperSubscription) {
          mBootstrapperSubscription = mBootstrapper->subscribe(mThisWeak.lock());
          if (!mBootstrapperSubscription) {
            ZS_LOG_WARNING(Detail, log("could not subscribe to bootstrapper"))
            setLastError(IAccount::AccountError_BootstrappedNetworkFailed);
            cancel();
            return;
          }
        }

        if (IBootstrappedNetwork::BootstrappedNetworkState_Ready != mBootstrapper->getState()) {
          ZS_LOG_DEBUG(log("waiting on bootstrapper to be ready"))
          return;
        }

        if (!mSocket) {
          String turnServer;
          String turnServerUsername;
          String turnServerPassword;
          String stunServer;

          ZS_LOG_DEBUG(log("creating RUDP ICE socket"))

          mBootstrapper->getTURNAndSTUNServers(turnServer, turnServerUsername, turnServerPassword, stunServer);
          mSocket = IRUDPICESocket::create(
                                           getAssociatedMessageQueue(),
                                           mThisWeak.lock(),
                                           turnServer,
                                           turnServerUsername,
                                           turnServerPassword,
                                           stunServer
                                           );
          if (!mSocket) {
            ZS_LOG_ERROR(Detail, log("failed to create RUDP ICE socket thus shutting down"))
            setLastError(IAccount::AccountError_InternalError);
            cancel();
            return;
          }
        }

        IRUDPICESocket::RUDPICESocketStates socketState = mSocket->getState();

        if ((IRUDPICESocket::RUDPICESocketState_Ready != socketState) &&
            (IRUDPICESocket::RUDPICESocketState_Sleeping != socketState)) {
          ZS_LOG_DEBUG(log("waiting for the socket to wake up or to go to sleep"))
          return;
        }

        Time tick = zsLib::now();

        if (mFinderRetryAfter > tick) {
          ZS_LOG_DEBUG(log("waiting a bit before retrying finder connection..."))
          return;
        }

        if (!mFinder) {
          ZS_LOG_DEBUG(log("creating finder instance"))
          mFinder = AccountFinder::create(getAssociatedMessageQueue(), mThisWeak.lock(), mThisWeak.lock());
          if (!mFinder) {
            ZS_LOG_ERROR(Detail, log("step failed to create finder thus shutting down"))
            setLastError(IAccount::AccountError_InternalError);
            cancel();
            return;
          }
        }

        if (IAccountFinder::AccountFinderState_Ready != mFinder->getState()) {
          ZS_LOG_DEBUG(log("waiting for the finder to connect"))
          return;
        }

        setState(AccountState_Ready);

        for (PeerMap::iterator peerIter = mPeers.begin(); peerIter != mPeers.end(); )
        {
          PeerMap::iterator current = peerIter;
          ++peerIter;

          const String &contactID = (*current).first;
          PeerPtr &peer = (*current).second;

          if (shouldShutdownInactiveLocations(contactID, peer)) {

            if (mBlockLocationShutdownsUntil < tick) {  // prevent shutdowns immediately after backgrounding (to give time to see which will self-cancel due to timeout)

              // scope: the peer is not incoming and all subscriptions are gone therefor it is safe to shutdown the peer locations entirely
              if (peer->mLocations.size() > 0) {
                ZS_LOG_DEBUG(log("checking to see which locations for this peer should be shutdown due to inactivity") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)

                for (Peer::LocationIDToPeerLocationMap::iterator locationIter = peer->mLocations.begin(); locationIter != peer->mLocations.end(); ) {
                  Peer::LocationIDToPeerLocationMap::iterator locationCurrentIter = locationIter;
                  ++locationIter;

                  const String &locationID = (*locationCurrentIter).first;
                  IAccountPeerLocationPtr &peerLocation = (*locationCurrentIter).second;

                  Time lastActivityTime = peerLocation->getTimeOfLastActivity();

                  if (lastActivityTime + Seconds(HOOKFLASH_STACK_PEER_LOCATION_INACTIVITY_TIMEOUT_IN_SECONDS) > tick) {
                    ZS_LOG_DEBUG(log("peer location is still considered active at this time (thus keeping connection alive)") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID + ", location=" + locationID + ", last activity=" + Stringize<Time>(lastActivityTime).string())
                    continue;
                  }

                  ZS_LOG_DEBUG(log("shutting down non incoming peer location that does not have a subscription") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID + ", location=" + locationID + ", last activity=" + Stringize<Time>(lastActivityTime).string())

                  // signal the shutdown now...
                  peerLocation->shutdown();
                }
              }

              if (peer->mLocations.size() > 0) {
                ZS_LOG_DEBUG(log("some location are still connected thus do not shutdown the peer yet") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
                continue;
              }

              // erase the peer now...
              ZS_LOG_DEBUG(log("no locations at this peer thus shutting down now") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
              mPeers.erase(current);
            }
            continue;
          } else {
            if (peer->mLocations.size() > 0) {
              ZS_LOG_DEBUG(log("checking to see which locations should fire a keep alive timer") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)

              for (Peer::LocationIDToPeerLocationMap::iterator locationIter = peer->mLocations.begin(); locationIter != peer->mLocations.end(); ) {
                Peer::LocationIDToPeerLocationMap::iterator locationCurrentIter = locationIter;
                ++locationIter;

                const String &locationID = (*locationCurrentIter).first;
                IAccountPeerLocationPtr &peerLocation = (*locationCurrentIter).second;

                Time lastActivityTime = peerLocation->getTimeOfLastActivity();

                if (lastActivityTime + Seconds(HOOKFLASH_STACK_PEER_LOCATION_KEEP_ALIVE_TIME_IN_SECONDS) > tick) {
                  ZS_LOG_TRACE(log("peer location is not requiring a keep alive yet") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID + ", location=" + locationID + ", last activity=" + Stringize<Time>(lastActivityTime).string())
                  continue;
                }

                ZS_LOG_DEBUG(log("peer location is still needed thus sending keep alive now (if possible)...") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID + ", location=" + locationID + ", last activity=" + Stringize<Time>(lastActivityTime).string())
                peerLocation->sendKeepAlive();
              }
            }
          }

          if (!shouldFind(contactID, peer)) {
            ZS_LOG_DEBUG(log("peer find should not be conducted at this time..."))
            continue;
          }

          mSocket->wakeup();
          if (IRUDPICESocket::RUDPICESocketState_Ready != mSocket->getState()) {
            ZS_LOG_DEBUG(log("should issue find request but must wait until ICE candidates are fully ready") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
            continue;
          }

          ZS_LOG_DEBUG(log("peer is conducting a peer find search for locations") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)

          // remember which hints caused this search to happen
          peer->mPeerFindBecauseOfLocations = peer->mPeerFindNeedsRedoingBecauseOfLocations;
          peer->mPeerFindNeedsRedoingBecauseOfLocations.clear();  // we no longer need to redo because of these hints since we are now doing the search

          message::PeerToFinderPeerLocationFindRequestPtr request = message::PeerToFinderPeerLocationFindRequest::create();

          message::PeerToFinderPeerLocationFindRequest::ExcludedLocationList exclude;
          for (Peer::LocationIDToPeerLocationMap::iterator iter = peer->mLocations.begin(); iter != peer->mLocations.end(); ++iter) {
            if ((*iter).second->hasReceivedCandidateInformation())
              // do not conduct a search for locations that already are connected or in the process of connecting...
              ZS_LOG_DEBUG(log("peer find will exclude location in search since location is already connecting or connected") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID + ", excluding locatin=" + (*iter).first)
              exclude.push_back((*iter).first);
          }

          CandidateList candidates;
          mSocket->getLocalCandidates(candidates);

          IPAddress ipAddress;
          for (CandidateList::iterator iter = candidates.begin(); iter != candidates.end(); ++iter) {
            hookflash::services::IICESocket::Candidate &candidate = (*iter);
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

          request->remoteContactID(contactID);
          Location location;
          location.mID = mLocationID;
          location.mContactID = mPeerFiles->getPublic()->getContactID();
          location.mIPAddress = ipAddress;
          location.mDeviceID = getDeviceID();
          location.mUserAgent = getUserAgent();
          location.mOS = getOS();
          location.mSystem = getSystem();
          location.mHost = &(buffer[0]);
          location.mCandidates = candidates;
          request->location(location);
          request->excludeLocations(exclude);

          peer->mPeerFindRequester = mFinder->sendRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_PEER_LOCATION_FIND_TIMEOUT_IN_SECONDS));

          setFindState(*(peer.get()), IPeerSubscription::PeerSubscriptionFindState_Finding);
        }
      }

      //-----------------------------------------------------------------------
      void Account::setState(IAccount::AccountStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        if (!mDelegate) return;

        AccountPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onAccountStateChanged(pThis, mCurrentState);
          } catch(IAccountDelegateProxy::Exceptions::DelegateGone &) {
          }
        }
      }

      //-----------------------------------------------------------------------
      void Account::setLastError(IAccount::AccountErrors error)
      {
        if (mLastError == error) return;

        ZS_LOG_ERROR(Basic, log("account error") + ", error=" + toString(error))

        mLastError = error;
      }

      //-----------------------------------------------------------------------
      void Account::setFindState(
                                 Peer &peer,
                                 IPeerSubscription::PeerSubscriptionFindStates state
                                 )
      {
        if (peer.mCurrentFindState == state) return;

        ZS_LOG_DEBUG(log("find state changed") + ", old state=" + IPeerSubscription::toString(peer.mCurrentFindState) + ", new state=" + IPeerSubscription::toString(state) + ", total subscriptions to notify=" + Stringize<size_t>(peer.mPeerSubscriptions.size()).string())

        peer.mCurrentFindState = state;

        // notify all the peer subscriptions of the search being conducted...
        for (Peer::PeerSubscriptionMapList::iterator iter = peer.mPeerSubscriptions.begin(); iter != peer.mPeerSubscriptions.end(); ++iter) {
          IPeerSubscriptionForAccountPtr subscription = (*iter).second;
          subscription->notifyAccountPeerFindStateChanged(state);
        }
      }

      //-----------------------------------------------------------------------
      bool Account::shouldFind(
                               const String &contactID,
                               const PeerPtr &peer
                               ) const
      {
        typedef zsLib::Time Time;

        Time tick = zsLib::now();

        if (peer->mPeerFindRequester) {
          ZS_LOG_DEBUG(log("peer has peer find in progress thus no need to conduct new search") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
          return false;
        }

        if (peer->mFindAtNextPossibleMoment) {
          ZS_LOG_DEBUG(log("told to refind at next possible moment"))
          return true;
        }

        if (peer->mPeerSubscriptions.size() < 1) {
          ZS_LOG_DEBUG(log("no subscribers required so no need to subscribe to this location"))
          return false;
        }

        if (peer->mPeerFindNeedsRedoingBecauseOfLocations.size() > 0) {
          ZS_LOG_DEBUG(log("peer has hints of new locations thus search needs redoing") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
          return true;
        }

        if (peer->mLocations.size() > 0) {
          ZS_LOG_DEBUG(log("peer has locations and no hints to suggest new locations thus no need to conduct new search") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
          return false;
        }

        // we have subscriptions but no locations, see if it is okay to find again "now"...
        if (tick < peer->mNextScheduledFind) {
          ZS_LOG_DEBUG(log("not time yet to conduct a new search") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
          return false;
        }

        if (!peer->mPeerFilePublic) {
          ZS_LOG_WARNING(Detail, log("cannot find a peer where the public peer file is not known"))
          return false;
        }

        ZS_LOG_DEBUG(log("peer search should be conducted") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
        return true;
      }

      //-----------------------------------------------------------------------
      bool Account::shouldShutdownInactiveLocations(
                                                    const String &contactID,
                                                    const PeerPtr &peer
                                                    ) const
      {
        if (peer->mPeerFindRequester) {
          ZS_LOG_DEBUG(log("peer has peer active find in progress thus its location should not be shutdown") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
          return false;
        }

        if (peer->mPeerSubscriptions.size() > 0) {
          ZS_LOG_DEBUG(log("peer has subscriptions thus no need to shutdown") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
          return false;
        }

        if (peer->mFindAtNextPossibleMoment) {
          ZS_LOG_DEBUG(log("do not shutdown inactive as there is a desire to find at next possible moment") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
          return false;
        }

        ZS_LOG_DEBUG(log("should shutdown this peer's location that are non-active") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", contact ID=" + contactID)
        return true;
      }

      //-----------------------------------------------------------------------
      void Account::handleFindRequestComplete(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());

        for (PeerMap::iterator iter = mPeers.begin(); iter != mPeers.end(); ++iter) {
          PeerPtr peer = (*iter).second;

          if (requester != peer->mPeerFindRequester) continue;

          ZS_LOG_DEBUG(log("find request is now complete") + ", peer=" + Stringize<PUID>(peer->mID).string())

          // the search is now complete, stop looking for new searches
          peer->mPeerFindRequester.reset();
          peer->mPeerFindBecauseOfLocations.clear();  // all hints are effectively destroyed now since we've completed the search

          setFindState(*(peer.get()), IPeerSubscription::PeerSubscriptionFindState_Completed);
          setFindState(*(peer.get()), IPeerSubscription::PeerSubscriptionFindState_Idle);

          bool foundValid = false;

          // scope: check to see which of these has not received candidate information and close the location since we could not contact to this location
          {
            for (Peer::LocationIDToPeerLocationMap::iterator iterLocation = peer->mLocations.begin(); iterLocation != peer->mLocations.end(); ++iterLocation) {
              IAccountPeerLocationPtr &peerLocation = (*iterLocation).second;
              if (!peerLocation->hasReceivedCandidateInformation()) {
                ZS_LOG_DEBUG(log("shutting down peer location as did not receive ICE candidates") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + peerLocation->getLocationID())
                peerLocation->shutdown();
              } else {
                ZS_LOG_DEBUG(log("find location is valid") + ", peer=" + Stringize<PUID>(peer->mID).string() + ", location=" + peerLocation->getLocationID())
                foundValid = true;
                peer->findTimeReset();
              }
            }
          }

          if (!foundValid) {
            peer->findTimeScheduleNext();
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::ConnectionSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      Account::ConnectionSubscription::ConnectionSubscription(
                                                              AccountPtr outer,
                                                              IConnectionSubscriptionDelegatePtr delegate
                                                              ) :
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IConnectionSubscriptionDelegateProxy::createWeak(delegate)),
        mLastFinderState(ConnectionState_Pending)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void Account::ConnectionSubscription::init()
      {
      }

      //-----------------------------------------------------------------------
      Account::ConnectionSubscription::~ConnectionSubscription()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::ConnectionSubscription => friend Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::ConnectionSubscriptionPtr Account::ConnectionSubscription::create(
                                                                                 AccountPtr outer,
                                                                                 IConnectionSubscriptionDelegatePtr delegate
                                                                                 )
      {
        ConnectionSubscriptionPtr pThis(new ConnectionSubscription(outer, delegate));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void Account::ConnectionSubscription::notifyFinderStateChanged(ConnectionStates state)
      {
        if (state == mLastFinderState) return;

        ZS_LOG_DEBUG(log("notifying connection subscription of finder state change") + ", old state=" + toString(mLastFinderState) + ", new state=" + toString(state))

        mLastFinderState = state;

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("no delegate to notify the finder state changed"))
          return;
        }

        ConnectionSubscriptionPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onConnectionSubscriptionFinderConnectionStateChanged(pThis, state);
          } catch (IConnectionSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate is gone"))
            cancel();
          }
        }
      }

      //-----------------------------------------------------------------------
      void Account::ConnectionSubscription::notifyPeerStateChanged(
                                                                   IPeerLocationPtr location,
                                                                   ConnectionStates state
                                                                   )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!location)

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("no delegate to notify the peer state changed") + ", contact ID=" + location->getContactID() + ", location ID=" + location->getLocationID())
          return;
        }

        ConnectionSubscriptionPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            ZS_LOG_TRACE(log("notifying about peer state change") + ", state=" + toString(state) + ", contact ID=" + location->getContactID() + ", location ID=" + location->getLocationID())
            mDelegate->onConnectionSubscriptionPeerLocationConnectionStateChanged(pThis, location, state);
          } catch (IConnectionSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate is gone") + ", contact ID=" + location->getContactID() + ", location ID=" + location->getLocationID())
            cancel();
          }
        }
      }

      //-----------------------------------------------------------------------
      void Account::ConnectionSubscription::notifyIncomingMessage(IConnectionSubscriptionMessagePtr message)
      {
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("no delegate to notify about incoming message") + ", source=" + IConnectionSubscriptionMessage::toString(message->getSource()) + ", contact ID=" + message->getPeerContactID() + ", location ID=" + message->getPeerContactID())
          return;
        }

        try {
          ZS_LOG_TRACE(log("notified of incoming message") + ", source=" + IConnectionSubscriptionMessage::toString(message->getSource()) + ", contact ID=" + message->getPeerContactID() + ", location ID=" + message->getPeerContactID())
          mDelegate->onConnectionSubscriptionIncomingMessage(mThisWeak.lock(), message);
        } catch (IConnectionSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate is gone") + ", source=" + IConnectionSubscriptionMessage::toString(message->getSource()) + ", contact ID=" + message->getPeerContactID() + ", location ID=" + message->getPeerContactID())
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::ConnectionSubscription => IConnectionSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      bool Account::ConnectionSubscription::isShutdown() const
      {
        AutoRecursiveLock lock(getLock());
        return mDelegate;
      }

      //-----------------------------------------------------------------------
      IConnectionSubscription::ConnectionStates Account::ConnectionSubscription::getFinderConnectionState() const
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr outer = mOuter.lock();
        if (!outer) return IConnectionSubscription::ConnectionState_Disconnected;
        return outer->getFinderConnectionState();
      }

      //-----------------------------------------------------------------------
      IConnectionSubscription::ConnectionStates Account::ConnectionSubscription::getPeerLocationConnectionState(
                                                                                                                const char *contactID,
                                                                                                                const char *locationID
                                                                                                                ) const
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr outer = mOuter.lock();
        if (!outer) return IConnectionSubscription::ConnectionState_Disconnected;
        return outer->getPeerLocationConnectionState(contactID, locationID);
      }

      //-----------------------------------------------------------------------
      void Account::ConnectionSubscription::getPeerLocations(
                                                             const char *contactID,
                                                             PeerLocations &outLocations,
                                                             bool includeOnlyConnectedLocations
                                                             ) const
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr outer = mOuter.lock();
        if (!outer) return;
        return outer->getPeerLocations(contactID, outLocations, includeOnlyConnectedLocations);
      }

      //-----------------------------------------------------------------------
      bool Account::ConnectionSubscription::sendFinderMessage(message::MessagePtr message)
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr outer = mOuter.lock();
        if (!outer) return false;
        return outer->sendFinderMessage(message);
      }

      //-----------------------------------------------------------------------
      bool Account::ConnectionSubscription::sendPeerMessage(
                                                            const char *contactID,
                                                            const char *locationID,
                                                            message::MessagePtr message
                                                            )
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr outer = mOuter.lock();
        if (!outer) return false;
        return outer->sendPeerMessage(contactID, locationID, message);
      }

      //-----------------------------------------------------------------------
      void Account::ConnectionSubscription::cancel()
      {
        if (mDelegate) {
          try {
            mDelegate->onConnectionSubscriptionShutdown(mThisWeak.lock());
          } catch (IConnectionSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        mDelegate.reset();

        ConnectionSubscriptionPtr pThis = mThisWeak.lock();
        AccountPtr outer = mOuter.lock();
        if ((outer) &&
            (pThis)) {
          outer->notifyConnectionSubscriptionShutdown(pThis);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::ConnectionSubscription => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Account::ConnectionSubscription::getLock() const
      {
        AccountPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String Account::ConnectionSubscription::log(const char *message) const
      {
        return String("stack::Account::ConnectionSubscription [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      Account::SubscriptionMessage::SubscriptionMessage(
                                                        IPeerSubscriptionPtr peerSubscription,
                                                        IConnectionSubscriptionPtr connectionSubscription,
                                                        const char *contactID,
                                                        const char *locationID,
                                                        message::MessagePtr message,
                                                        SubscriptionNotificationHelperPtr helper
                                                        ) :
        mID(zsLib::createPUID()),
        mSource(NULL == contactID ? IConnectionSubscriptionMessage::Source_Finder : IConnectionSubscriptionMessage::Source_Peer),
        mConnectionSubscription(connectionSubscription),
        mPeerSubscription(peerSubscription),
        mContactID(contactID ? contactID : ""),
        mLocationID(locationID ? locationID : ""),
        mMessage(message),
        mNotificationHelper(helper)
      {
        ZS_LOG_TRACE(log("created"))
      }

      //-----------------------------------------------------------------------
      Account::SubscriptionMessage::~SubscriptionMessage()
      {
        ZS_LOG_TRACE(log("destroyed"))
        mNotificationHelper->notifyPeerSubscriptionMessageDestroyed(this);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionMessage => friend Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::SubscriptionMessagePtr Account::SubscriptionMessage::create(
                                                                           IPeerSubscriptionPtr outer,
                                                                           const char *contactID,
                                                                           const char *locationID,
                                                                           message::MessagePtr message,
                                                                           SubscriptionNotificationHelperPtr helper
                                                                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!outer)
        ZS_THROW_INVALID_ARGUMENT_IF(!message)
        ZS_THROW_INVALID_ARGUMENT_IF(!helper)

        SubscriptionMessagePtr pThis(new SubscriptionMessage(outer, IConnectionSubscriptionPtr(), contactID, locationID, message, helper));
        return pThis;
      }

      //-----------------------------------------------------------------------
      Account::SubscriptionMessagePtr Account::SubscriptionMessage::createForFinder(
                                                                                    IConnectionSubscriptionPtr outer,
                                                                                    message::MessagePtr message,
                                                                                    SubscriptionNotificationHelperPtr helper
                                                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!outer)
        ZS_THROW_INVALID_ARGUMENT_IF(!message)
        ZS_THROW_INVALID_ARGUMENT_IF(!helper)

        SubscriptionMessagePtr pThis(new SubscriptionMessage(IPeerSubscriptionPtr(), outer, NULL, NULL, message, helper));
        return pThis;
      }

      //-----------------------------------------------------------------------
      Account::SubscriptionMessagePtr Account::SubscriptionMessage::createForPeer(
                                                                                  IConnectionSubscriptionPtr outer,
                                                                                  const char *contactID,
                                                                                  const char *locationID,
                                                                                  message::MessagePtr message,
                                                                                  SubscriptionNotificationHelperPtr helper
                                                                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!outer)
        ZS_THROW_INVALID_ARGUMENT_IF(!message)
        ZS_THROW_INVALID_ARGUMENT_IF(!helper)
        ZS_THROW_INVALID_ARGUMENT_IF(!contactID)
        ZS_THROW_INVALID_ARGUMENT_IF(!locationID)

        SubscriptionMessagePtr pThis(new SubscriptionMessage(IPeerSubscriptionPtr(), outer, contactID, locationID, message, helper));
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionMessage => IPeerSubscriptionMessage
      #pragma mark

      //-----------------------------------------------------------------------
      IPeerSubscriptionPtr Account::SubscriptionMessage::getPeerSubscription() const
      {
        return mPeerSubscription;
      }

      //-----------------------------------------------------------------------
      String Account::SubscriptionMessage::getContactID() const
      {
        return mContactID;
      }

      //-----------------------------------------------------------------------
      String Account::SubscriptionMessage::getLocationID() const
      {
        return mLocationID;
      }

      //-----------------------------------------------------------------------
      message::MessagePtr Account::SubscriptionMessage::getMessage() const
      {
        return mMessage;
      }

      //-----------------------------------------------------------------------
      bool Account::SubscriptionMessage::sendResponse(message::MessagePtr message)
      {
        ZS_LOG_TRACE(log("sending response"))

        ZS_THROW_INVALID_ARGUMENT_IF(!message)

        mNotificationHelper->notifyHandledBySubscriber();
        if (mPeerSubscription)
          return mPeerSubscription->sendPeerMesage(mLocationID, message);

        return (IConnectionSubscriptionMessage::Source_Finder == mSource ? mConnectionSubscription->sendFinderMessage(message) : mConnectionSubscription->sendPeerMessage(mContactID, mLocationID, message));
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionMessage => IConnectionSubscriptionMessage
      #pragma mark

      //-----------------------------------------------------------------------
      IConnectionSubscriptionPtr Account::SubscriptionMessage::getConnectionSubscription() const
      {
        return mConnectionSubscription;
      }

      //-----------------------------------------------------------------------
      IConnectionSubscriptionMessage::Sources Account::SubscriptionMessage::getSource() const
      {
        return mSource;
      }

      //-----------------------------------------------------------------------
      String Account::SubscriptionMessage::getPeerContactID() const
      {
        return mContactID;
      }

      //-----------------------------------------------------------------------
      String Account::SubscriptionMessage::getPeerLocationID() const
      {
        return mLocationID;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionMessage => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Account::SubscriptionMessage::log(const char *message) const
      {
        return String("stack::Account::SubscriptionMessage [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionNotificationHelper
      #pragma mark

      //-----------------------------------------------------------------------
      Account::SubscriptionNotificationHelper::SubscriptionNotificationHelper(ULONG totalExpectedHandlers) :
        mID(zsLib::createPUID()),
        mTotalExpectedHandlers(totalExpectedHandlers),
        mHandled(false)
      {
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionNotificationHelper => friend Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::SubscriptionNotificationHelperPtr Account::SubscriptionNotificationHelper::create(ULONG totalExpectedHandlers)
      {
        if (0 == totalExpectedHandlers) return SubscriptionNotificationHelperPtr();

        SubscriptionNotificationHelperPtr pThis(new SubscriptionNotificationHelper(totalExpectedHandlers));
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionNotificationHelper => friend Account::PeerSubscriptionMessage
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::SubscriptionNotificationHelper::notifyHandledBySubscriber()
      {
        AutoRecursiveLock lock(mLock);
        mHandled = true;
      }

      //-----------------------------------------------------------------------
      void Account::SubscriptionNotificationHelper::notifyPeerSubscriptionMessageDestroyed(Account::SubscriptionMessage *peerSubscriptionMessage)
      {
        ULONG count = 0;

        // scope: decrement the total count of how many outstanding peer subscription messages exist
        {
          AutoRecursiveLock lock(mLock);
          ZS_THROW_BAD_STATE_IF(mTotalExpectedHandlers < 1)

          --mTotalExpectedHandlers;
          count = mTotalExpectedHandlers;
        }

        if (0 != count) {
          ZS_LOG_TRACE(log("will not send a reply as other subscriptions might reply instead") + ", count=" + Stringize<ULONG>(count).string())
          // only the final peer subscription message needs to handle if it should send a reply or not all others can ignore
          return;
        }

        // scope: check to see if anyone handled this message
        {
          AutoRecursiveLock lock(mLock);
          if (mHandled) {
            ZS_LOG_DEBUG(log("will not automatically send a reply because a reply was already sent"))
            return;
          }
        }

        message::MessagePtr originalMessage = peerSubscriptionMessage->getMessage();
        ZS_THROW_INVALID_ASSUMPTION_IF(!originalMessage)

        switch (originalMessage->messageType())
        {
          case message::Message::MessageType_Invalid:
          case message::Message::MessageType_Result:
          case message::Message::MessageType_Reply:
          {
            ZS_LOG_DEBUG(log("no need to auto-reply to a result/reply"))
            return;
          }
          case message::Message::MessageType_Request:
          case message::Message::MessageType_Notify:
          {
            break;
          }
        }

        ZS_LOG_DEBUG(log("automatically sending a reply that the object requested was not found since nothing replied to the request"))

        message::MessageResultPtr result = message::MessageResult::create(originalMessage, 404, "Not found");
        peerSubscriptionMessage->sendResponse(result);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::SubscriptionNotificationHelper => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Account::SubscriptionNotificationHelper::log(const char *message) const
      {
        return String("PeerSubscriptionNotificationHelper [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::Peer
      #pragma mark

      //-----------------------------------------------------------------------
      Account::PeerPtr Account::Peer::create()
      {
        PeerPtr pThis(new Peer);
        pThis->mID = zsLib::createPUID();
        pThis->mFindAtNextPossibleMoment = false;
        pThis->findTimeReset();
        pThis->mCurrentFindState = IPeerSubscription::PeerSubscriptionFindState_Idle;
        pThis->mPreventCrazyRefindNextTime = false;
        return pThis;
      }

      //-----------------------------------------------------------------------
      void Account::Peer::findTimeReset()
      {
        mNextScheduledFind = zsLib::now();
        mLastScheduleFindDuration = Seconds(HOOKFLASH_STACK_PEER_LOCATION_FIND_RETRY_IN_SECONDS/2);
        if (mLastScheduleFindDuration < Seconds(1))
          mLastScheduleFindDuration = Seconds(1);
      }

      //-----------------------------------------------------------------------
      void Account::Peer::findTimeScheduleNext()
      {
        mLastScheduleFindDuration = mLastScheduleFindDuration * 2;
        mNextScheduledFind = zsLib::now() + mLastScheduleFindDuration;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccount
    #pragma mark

    //-------------------------------------------------------------------------
    IAccountPtr IAccount::create(
                                 IMessageQueuePtr queue,
                                 IBootstrappedNetworkPtr bootstrapper,
                                 IAccountDelegatePtr delegate,
                                 IPeerFilesPtr peerFiles,
                                 const char *password,
                                 const char *deviceID,
                                 const char *userAgent,
                                 const char *os,
                                 const char *system
                                 )
    {
      return internal::Account::create(queue, bootstrapper, delegate, peerFiles, password, deviceID, userAgent, os, system);
    }

    //-------------------------------------------------------------------------
    Time IAccount::getAdjustedTime()
    {
      return internal::AdjustedTimeSingleton::getAdjustedTime();
    }

    //-------------------------------------------------------------------------
    const char *IAccount::toString(AccountStates state)
    {
      switch (state) {
        case AccountState_Pending:      return "Pending";
        case AccountState_Ready:        return "Ready";
        case AccountState_ShuttingDown: return "Shutting down";
        case AccountState_Shutdown:     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IAccount::toString(AccountErrors state)
    {
      switch (state) {
        case AccountError_None:                       return "None";

        case AccountError_InternalError:              return "Internal error";

        case AccountError_BootstrappedNetworkFailed:  return "Bootstrapped network failed";
        case AccountError_SocketUnexpectedlyClosed:   return "Socket unexpectedly closed";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConnectionSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IConnectionSubscription::toString(ConnectionStates state)
    {
      switch (state) {
        case stack::IConnectionSubscription::ConnectionState_Pending:       return "Pending";
        case stack::IConnectionSubscription::ConnectionState_Connected:     return "Connected";
        case stack::IConnectionSubscription::ConnectionState_Disconnecting: return "Disconnecting";
        case stack::IConnectionSubscription::ConnectionState_Disconnected:  return "Disconnected";
      }
      return "UNDEFINED";
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConnectionSubscriptionMessage
    #pragma mark

    const char *IConnectionSubscriptionMessage::toString(Sources source)
    {
      switch (source)
      {
        case Source_Finder: return "Finder";
        case Source_Peer:   return "Peer";
      }
      return "UNDEFINED";
    }
  }
}
