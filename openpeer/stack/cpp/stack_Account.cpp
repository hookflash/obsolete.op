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

#include <openpeer/stack/internal/stack_Account.h>
#include <openpeer/stack/internal/stack_AccountFinder.h>
#include <openpeer/stack/internal/stack_AccountPeerLocation.h>
#include <openpeer/stack/internal/stack_BootstrappedNetwork.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/internal/stack_Location.h>
#include <openpeer/stack/internal/stack_MessageMonitor.h>
#include <openpeer/stack/internal/stack_MessageIncoming.h>
#include <openpeer/stack/internal/stack_Peer.h>
#include <openpeer/stack/internal/stack_PeerSubscription.h>
#include <openpeer/stack/internal/stack_PublicationRepository.h>
#include <openpeer/stack/internal/stack_ServiceLockboxSession.h>
#include <openpeer/stack/internal/stack_Stack.h>
#include <openpeer/stack/internal/stack_IFinderRelayChannel.h>

#include <openpeer/stack/message/MessageResult.h>

#include <openpeer/stack/message/peer-finder/PeerLocationFindRequest.h>
#include <openpeer/stack/message/peer-finder/PeerLocationFindResult.h>
#include <openpeer/stack/message/peer-finder/PeerLocationFindNotify.h>
#include <openpeer/stack/message/peer-finder/ChannelMapNotify.h>

#include <openpeer/stack/message/bootstrapped-finder/FindersGetRequest.h>
#include <openpeer/stack/message/bootstrapped-finder/FindersGetResult.h>

#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPublicationRepository.h>

#include <openpeer/services/IHelper.h>
#include <openpeer/services/IRUDPICESocket.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/XML.h>

#include <algorithm>

#define OPENPEER_STACK_PEER_LOCATION_FIND_TIMEOUT_IN_SECONDS (60*2)
#define OPENPEER_STACK_PEER_LOCATION_FIND_RETRY_IN_SECONDS (30)
#define OPENPEER_STACK_PEER_LOCATION_INACTIVITY_TIMEOUT_IN_SECONDS (10*60)
#define OPENPEER_STACK_PEER_LOCATION_KEEP_ALIVE_TIME_IN_SECONDS    (5*60)

#define OPENPEER_STACK_FINDERS_GET_TOTAL_SERVERS_TO_GET (2)
#define OPENPEER_STACK_FINDERS_GET_TIMEOUT_IN_SECONDS (60)

#define OPENPEER_STACK_ACCOUNT_TIMER_FIRES_IN_SECONDS (15)
#define OPENPEER_STACK_ACCOUNT_TIMER_DETECTED_BACKGROUNDING_TIME_IN_SECONDS (40)
#define OPENPEER_STACK_ACCOUNT_PREVENT_LOCATION_SHUTDOWNS_AFTER_BACKGROUNDING_FOR_IN_SECONDS (15)
#define OPENPEER_STACK_ACCOUNT_FINDER_STARTING_RETRY_AFTER_IN_SECONDS (1)
#define OPENPEER_STACK_ACCOUNT_FINDER_MAX_RETRY_AFTER_TIME_IN_SECONDS (60)

#define OPENPEER_STACK_ACCOUNT_RUDP_TRANSPORT_PROTOCOL_TYPE "rudp/udp"
#define OPENPEER_STACK_ACCOUNT_MULTIPLEXED_JSON_TCP_TRANSPORT_PROTOCOL_TYPE "multiplexed-json/tcp"

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using services::IHelper;

      using services::IWakeDelegateProxy;

      using message::peer_finder::PeerLocationFindRequest;
      using message::peer_finder::PeerLocationFindRequestPtr;
      using message::peer_finder::PeerLocationFindResult;
      using message::peer_finder::PeerLocationFindResultPtr;
      using message::peer_finder::PeerLocationFindNotify;
      using message::peer_finder::PeerLocationFindNotifyPtr;
      using message::peer_finder::ChannelMapNotify;
      using message::peer_finder::ChannelMapNotifyPtr;

      using message::bootstrapped_finder::FindersGetRequest;
      using message::bootstrapped_finder::FindersGetRequestPtr;
      using message::bootstrapped_finder::FindersGetResult;
      using message::bootstrapped_finder::FindersGetResultPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      ILocation::LocationConnectionStates toLocationConnectionState(IAccount::AccountStates state)
      {
        switch (state) {
          case IAccount::AccountState_Pending:      return ILocation::LocationConnectionState_Pending;
          case IAccount::AccountState_Ready:        return ILocation::LocationConnectionState_Connected;
          case IAccount::AccountState_ShuttingDown: return ILocation::LocationConnectionState_Disconnecting;
          case IAccount::AccountState_Shutdown:     return ILocation::LocationConnectionState_Disconnected;
        }
        return ILocation::LocationConnectionState_Disconnected;
      }

      //-----------------------------------------------------------------------
      static String getMultiplexedJsonTCPTransport(const Finder &finder)
      {
        for (Finder::ProtocolList::const_iterator iter = finder.mProtocols.begin(); iter != finder.mProtocols.end(); ++iter)
        {
          const Finder::Protocol &protocol = (*iter);

          if (OPENPEER_STACK_ACCOUNT_MULTIPLEXED_JSON_TCP_TRANSPORT_PROTOCOL_TYPE == protocol.mTransport) {
            return protocol.mHost;
          }
        }
        return String();
      }

      //-----------------------------------------------------------------------
      static String calculatePassphrase(
                                        const String &masterPassword,
                                        const String &remotePeerURI,
                                        const char *phrase
                                        )
      {
        return IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(masterPassword), String(phrase) + ":" + remotePeerURI, IHelper::HashAlgorthm_SHA256));
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::Account(
                       IMessageQueuePtr queue,
                       IAccountDelegatePtr delegate,
                       ServiceLockboxSessionPtr lockboxSession
                       ) :
        MessageQueueAssociator(queue),
        mLocationID(IHelper::randomString(32)),
        mCurrentState(IAccount::AccountState_Pending),
        mLastError(0),
        mDelegate(IAccountDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mBlockLocationShutdownsUntil(zsLib::now()),
        mLockboxSession(lockboxSession),
        mMasterPeerSecret(IHelper::randomString(32*8/5)),
        mFinderRetryAfter(zsLib::now()),
        mLastRetryFinderAfterDuration(Seconds(OPENPEER_STACK_ACCOUNT_FINDER_STARTING_RETRY_AFTER_IN_SECONDS))
      {
        ZS_LOG_BASIC(log("created"))
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
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      AccountPtr Account::convert(IAccountPtr account)
      {
        return boost::dynamic_pointer_cast<Account>(account);
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
      String Account::toDebugString(IAccountPtr account, bool includeCommaPrefix)
      {
        if (!account) return String(includeCommaPrefix ? ", account=(null)" : "account=(null)");
        AccountPtr pThis = Account::convert(account);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      AccountPtr Account::create(
                                 IAccountDelegatePtr delegate,
                                 IServiceLockboxSessionPtr peerContactSession
                                 )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate);
        ZS_THROW_INVALID_ARGUMENT_IF(!peerContactSession);

        AccountPtr pThis(new Account(IStackForInternal::queueStack(), delegate, ServiceLockboxSession::convert(peerContactSession)));
        pThis->mThisWeak = pThis;
        pThis->mDelegate = IAccountDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate);
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IAccount::AccountStates Account::getState(
                                                WORD *outLastErrorCode,
                                                String *outLastErrorReason
                                                ) const
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("get account state") + getDebugValueString())
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IServiceLockboxSessionPtr Account::getLockboxSession() const
      {
        AutoRecursiveLock lock(getLock());
        return mLockboxSession;
      }

      //-----------------------------------------------------------------------
      void Account::getNATServers(
                                  String &outTURNServer,
                                  String &outTURNUsername,
                                  String &outTURNPassword,
                                  String &outSTUNServer
                                  ) const
      {
        AutoRecursiveLock lock(getLock());
        outTURNServer = mTURN ? mTURN->mURI : String();
        outTURNUsername = mTURN ? mTURN->mUsername : String();
        outTURNPassword = mTURN ? mTURN->mPassword : String();
        outSTUNServer = (mSTUN ? mSTUN->mURI : (mTURN ? mTURN->mURI : String()));
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
      String Account::getDomain() const
      {
        AutoRecursiveLock lock(getLock());

        ZS_THROW_BAD_STATE_IF(!mLockboxSession)

        BootstrappedNetworkPtr network = mLockboxSession->forAccount().getBootstrappedNetwork();

        ZS_THROW_BAD_STATE_IF(!network)

        return network->forAccount().getDomain();
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketPtr Account::getSocket() const
      {
        AutoRecursiveLock lock(getLock());
        return mSocket;
      }

      //-----------------------------------------------------------------------
      bool Account::extractNextFinder(
                                      Finder &outFinder,
                                      IPAddress &outFinderIP
                                      )
      {
        const char *reason = NULL;

        if ((mAvailableFinders.size() < 1) ||
            (!mAvailableFinderSRVResult)) {
          reason = "no finders available";
          goto extract_failure;
        }

        if (!IDNS::extractNextIP(mAvailableFinderSRVResult, outFinderIP)) {
          reason = "unable to extract next IP (no more IPs available)";
          goto extract_failure;
        }
        if (outFinderIP.isAddressEmpty()) {
          reason = "extracted IP address is empty";
          goto extract_failure;
        }

        outFinder = mAvailableFinders.front();
        return true;

      extract_failure:
        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
        mAvailableFinderSRVResult.reset();
        ZS_LOG_WARNING(Detail, log("extract next IP failed") + ", reason=" + reason)
        return false;
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
        return IAccount::AccountState_Ready == mFinder->forAccount().getState();
      }

      //-----------------------------------------------------------------------
      String Account::getLocalContextID(const String &peerURI) const
      {
        return calculatePassphrase(mMasterPeerSecret, peerURI, "context");
      }

      //-----------------------------------------------------------------------
      String Account::getLocalPassword(const String &peerURI) const
      {
        return calculatePassphrase(mMasterPeerSecret, peerURI, "password");
      }

      //-----------------------------------------------------------------------
      bool Account::sendViaRelay(
                                 const String &peerURI,
                                 AccountPeerLocationPtr peerLocation,
                                 const BYTE *buffer,
                                 ULONG bufferSizeInBytes
                                 ) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerLocation)

        ZS_LOG_DEBUG(log("attempting to send via relay") + ", peer URI=" + peerURI + ", peer location ID=" + string(peerLocation->forAccount().getID()) + ", buffer length=" + string(bufferSizeInBytes))

        PeerInfoMap::const_iterator foundPeer = mPeerInfos.find(peerURI);
        if (foundPeer == mPeerInfos.end()) {
          ZS_LOG_WARNING(Detail, log("peer URI is not known thus cannot send message"))
          return false;
        }

        const PeerInfoPtr &peerInfo = (*foundPeer).second;

        for (RelayInfoMap::const_iterator relayIter = peerInfo->mRelayInfos.begin(); relayIter != peerInfo->mRelayInfos.end(); ++relayIter)
        {
          const RelayInfoPtr &relayInfo = (*relayIter).second;
          if (relayInfo->mAccountPeerLocation->forAccount().getID() != peerLocation->forAccount().getID()) continue;

          ZS_LOG_DEBUG(log("sending via relay") + relayInfo->getDebugValueString())

          if (!relayInfo->mSendStream) {
            ZS_LOG_WARNING(Detail, log("found relay info but send stream is not valid"))
            return false;
          }

          relayInfo->mSendStream->write(buffer, bufferSizeInBytes);
        }

        ZS_LOG_WARNING(Debug, log("did not find any relay for this peer location"))
        return false;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForLocation
      #pragma mark

      //-----------------------------------------------------------------------
      LocationPtr Account::findExistingOrUse(LocationPtr location)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!location)

        AutoRecursiveLock lock(getLock());

        String locationID = location->forAccount().getLocationID();
        String peerURI = location->forAccount().getPeerURI();

        PeerLocationIDPair index(locationID, peerURI);

        LocationMap::iterator found = mLocations.find(index);

        if (found != mLocations.end()) {
          LocationPtr existingLocation = (*found).second.lock();
          if (existingLocation) {
            ZS_LOG_DEBUG(log("found existing location to use") + ILocation::toDebugString(existingLocation))
            return existingLocation;
          }
          ZS_LOG_WARNING(Detail, log("existing location in map is now gone") + ILocation::toDebugString(location))
        }

        ZS_LOG_DEBUG(log("using newly created location") + ILocation::toDebugString(location))
        mLocations[index] = location;
        return location;
      }

      //-----------------------------------------------------------------------
      LocationPtr Account::getLocationForLocal() const
      {
        AutoRecursiveLock lock(getLock());
        return mSelfLocation;
      }

      //-----------------------------------------------------------------------
      LocationPtr Account::getLocationForFinder() const
      {
        AutoRecursiveLock lock(getLock());
        return mFinderLocation;
      }

      //-----------------------------------------------------------------------
      void Account::notifyDestroyed(Location &location)
      {
        AutoRecursiveLock lock(getLock());

        String locationID = location.forAccount().getLocationID();
        String peerURI = location.forAccount().getPeerURI();

        PeerLocationIDPair index(locationID, peerURI);

        LocationMap::iterator found = mLocations.find(index);
        if (found == mLocations.end()) {
          ZS_LOG_WARNING(Detail, log("existing location in map was already gone (probably okay)") + location.forAccount().getDebugValueString())
          return;
        }

        ZS_LOG_DEBUG(log("notified location is destroyed") + location.forAccount().getDebugValueString())
        mLocations.erase(found);
      }

      //-----------------------------------------------------------------------
      const String &Account::getLocationID() const
      {
        AutoRecursiveLock lock(getLock());
        return mLocationID;
      }

      //-----------------------------------------------------------------------
      PeerPtr Account::getPeerForLocal() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mSelfLocation) {
          ZS_LOG_WARNING(Detail, log("obtained peer for local before peer was ready"))
        }
        return mSelfPeer;
      }

      //-----------------------------------------------------------------------
      LocationInfoPtr Account::getLocationInfo(LocationPtr location) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!location)

        AutoRecursiveLock lock(getLock());

        LocationInfoPtr info(new LocationInfo);
        info->mLocation = location;

        if (location == mSelfLocation) {
          if (mSocket) {
            IICESocket::CandidateList candidates;
            mSocket->getLocalCandidates(candidates);

            for (IICESocket::CandidateList::iterator iter = candidates.begin(); iter != candidates.end(); ++iter) {
              IICESocket::Candidate &candidate = (*iter);
              if (IICESocket::Type_Local == candidate.mType) {
                info->mIPAddress = (*iter).mIPAddress;
              }
              Candidate convert(candidate);
              convert.mNamespace = OPENPEER_STACK_CANDIDATE_NAMESPACE_ICE_CANDIDATES;
              convert.mTransport = OPENPEER_STACK_TRANSPORT_JSON_MLS_RUDP;
              info->mCandidates.push_back(convert);
            }
          }

          char buffer[256];
          memset(&(buffer[0]), 0, sizeof(buffer));

          gethostname(&(buffer[0]), (sizeof(buffer)*sizeof(char))-sizeof(char));

          info->mDeviceID = IStackForInternal::deviceID();
          info->mUserAgent = IStackForInternal::userAgent();
          info->mOS = IStackForInternal::os();
          info->mSystem = IStackForInternal::system();
          info->mHost = &(buffer[0]);
          return info;
        }

        if (location == mFinderLocation) {
          if (!mFinder) {
            ZS_LOG_WARNING(Detail, log("obtaining finder info before finder created"))
            return info;
          }

          Finder finder = mFinder->forAccount().getCurrentFinder(&(info->mUserAgent), &(info->mIPAddress));

          info->mDeviceID = finder.mID;
          info->mHost = getMultiplexedJsonTCPTransport(finder);
          return info;
        }

        PeerInfoMap::const_iterator found = mPeerInfos.find(location->forAccount().getPeerURI());
        if (found == mPeerInfos.end()) {
          ZS_LOG_WARNING(Detail, log("could not find location information for non-connected peer") + ILocation::toDebugString(location))
          return info;
        }

        PeerInfoPtr peerInfo = (*found).second;

        PeerInfo::PeerLocationMap::const_iterator foundLocation = peerInfo->mLocations.find(location->forAccount().getLocationID());
        if (foundLocation == peerInfo->mLocations.end()) {
          ZS_LOG_WARNING(Detail, log("could not find peer location information for non-connected peer location") + ILocation::toDebugString(location))
          return info;
        }

        AccountPeerLocationPtr accountPeerLocation = (*foundLocation).second;

        const LocationInfo &sourceInfo = accountPeerLocation->forAccount().getLocationInfo();

        info->mDeviceID = sourceInfo.mDeviceID;
        info->mIPAddress = sourceInfo.mIPAddress;
        info->mUserAgent = sourceInfo.mUserAgent;
        info->mOS = sourceInfo.mOS;
        info->mSystem = sourceInfo.mSystem;
        info->mHost = sourceInfo.mHost;
        info->mCandidates = sourceInfo.mCandidates;

        return info;
      }

      //-----------------------------------------------------------------------
      ILocation::LocationConnectionStates Account::getConnectionState(LocationPtr location) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!location)

        AutoRecursiveLock lock(getLock());

        if (location == mSelfLocation) {
          return toLocationConnectionState(getState());
        }

        if (location == mFinderLocation) {
          if (!mFinder) {
            if ((isShuttingDown()) ||
                (isShutdown())) return ILocation::LocationConnectionState_Disconnected;

            return ILocation::LocationConnectionState_Pending;
          }

          return toLocationConnectionState(mFinder->forAccount().getState());
        }

        PeerInfoMap::const_iterator found = mPeerInfos.find(location->forAccount().getPeerURI());
        if (found == mPeerInfos.end()) {
          ZS_LOG_DEBUG(log("peer is not connected") + ILocation::toDebugString(location))
          return ILocation::LocationConnectionState_Disconnected;
        }

        PeerInfoPtr peerInfo = (*found).second;

        PeerInfo::PeerLocationMap::const_iterator foundLocation = peerInfo->mLocations.find(location->forAccount().getLocationID());
        if (foundLocation == peerInfo->mLocations.end()) {
          ZS_LOG_WARNING(Detail, log("could not find peer location information for non-connected peer location") + ILocation::toDebugString(location))
          return ILocation::LocationConnectionState_Disconnected;
        }

        AccountPeerLocationPtr accountPeerLocation = (*foundLocation).second;

        return toLocationConnectionState(accountPeerLocation->forAccount().getState());
      }

      //-----------------------------------------------------------------------
      bool Account::send(
                         LocationPtr location,
                         MessagePtr message
                         ) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!location)

        AutoRecursiveLock lock(getLock());

        if (location == mSelfLocation) {
          ZS_LOG_ERROR(Detail, log("attempting to send message to self") + ILocation::toDebugString(location))
          return false;
        }

        if (location == mFinderLocation) {
          if (!mFinder) {
            ZS_LOG_WARNING(Detail, log("attempting to send to finder") + ILocation::toDebugString(location))
            return false;
          }

          return mFinder->forAccount().send(message);
        }

        PeerInfoMap::const_iterator found = mPeerInfos.find(location->forAccount().getPeerURI());
        if (found == mPeerInfos.end()) {
          ZS_LOG_DEBUG(log("peer is not connected") + ILocation::toDebugString(location))
          return false;
        }

        PeerInfoPtr peerInfo = (*found).second;

        PeerInfo::PeerLocationMap::const_iterator foundLocation = peerInfo->mLocations.find(location->forAccount().getLocationID());
        if (foundLocation == peerInfo->mLocations.end()) {
          ZS_LOG_WARNING(Detail, log("could not find peer location information for non-connected peer location") + ILocation::toDebugString(location))
          return ILocation::LocationConnectionState_Disconnected;
        }

        AccountPeerLocationPtr accountPeerLocation = (*foundLocation).second;

        return accountPeerLocation->forAccount().send(message);
      }

      //-----------------------------------------------------------------------
      void Account::hintNowAvailable(LocationPtr location)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!location)

        ZS_LOG_DEBUG(log("received hint about peer location") + ILocation::toDebugString(location))

        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_WARNING(Detail, log("hint about new location when shutting down/shutdown is ignored") + ILocation::toDebugString(location))
          return;
        }

        PeerInfoMap::iterator found = mPeerInfos.find(location->forAccount().getPeerURI());
        if (found == mPeerInfos.end()) {
          ZS_LOG_WARNING(Detail, log("received hint about peer location that is not being subscribed") + ILocation::toDebugString(location))
          return;  // ignore the hint since there is no subscription to this location
        }

        String locationID = location->forAccount().getLocationID();

        PeerInfoPtr &peerInfo = (*found).second;
        PeerInfo::PeerLocationMap::iterator locationFound = peerInfo->mLocations.find(locationID);
        if (locationFound != peerInfo->mLocations.end()) {
          ZS_LOG_WARNING(Detail, log("received hint about peer location that is already known") + PeerInfo::toDebugString(peerInfo) + ILocation::toDebugString(location))
          return;  // thanks for the tip but we already know about this location...
        }

        // scope: see if we are in the middle of already searching for this peer location
        {
          PeerInfo::FindingBecauseOfLocationIDMap::iterator findingBecauseOfFound = peerInfo->mPeerFindBecauseOfLocations.find(locationID);
          if (findingBecauseOfFound != peerInfo->mPeerFindBecauseOfLocations.end()) {
            ZS_LOG_DEBUG(log("received hint about peer location for location that is already being searched because of a previous hint") + PeerInfo::toDebugString(peerInfo))
            return; // we've already received this tip...
          }
        }

        // scope: see if we already will redo a search because of this peer location
        {
          PeerInfo::FindingBecauseOfLocationIDMap::iterator findingBecauseOfFound = peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.find(locationID);
          if (findingBecauseOfFound != peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.end()) {
            ZS_LOG_WARNING(Detail, log("received hint about peer location for location that has already been given a hint") + PeerInfo::toDebugString(peerInfo))
            return; // we've already received this tip...
          }
        }

        // we will redo the search after this has completed because there are more locations that need to be found - THANKS FOR THE TIP!!
        peerInfo->mPeerFindNeedsRedoingBecauseOfLocations[locationID] = locationID;
        peerInfo->findTimeReset();

        ZS_LOG_DEBUG(log("received hint about peer location that will be added to hint search") + PeerInfo::toDebugString(peerInfo))
        (IWakeDelegateProxy::create(mThisWeak.lock()))->onWake();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForMessageIncoming
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::notifyMessageIncomingResponseNotSent(MessageIncoming &messageIncoming)
      {
        AutoRecursiveLock lock(getLock());

        LocationPtr location = messageIncoming.forAccount().getLocation();
        MessagePtr message = messageIncoming.forAccount().getMessage();

        MessageResultPtr result = MessageResult::create(message, IHTTP::HTTPStatusCode_NotFound);
        if (!result) {
          ZS_LOG_WARNING(Detail, log("automatic reply to incoming message could not be created") + messageIncoming.forAccount().getDebugValueString())
          return;
        }

        send(location, result);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      IPeerFilesPtr Account::getPeerFiles() const
      {
        AutoRecursiveLock lock(getLock());

        if (!mLockboxSession) {
          ZS_LOG_WARNING(Detail, log("peer files are not available on account as peer contact session does not exist") + getDebugValueString())
          return IPeerFilesPtr();
        }

        return mLockboxSession->forAccount().getPeerFiles();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForPeer
      #pragma mark

      //-----------------------------------------------------------------------
      PeerPtr Account::findExistingOrUse(PeerPtr peer)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peer)

        AutoRecursiveLock lock(getLock());

        String peerURI = peer->forAccount().getPeerURI();

        PeerMap::iterator found = mPeers.find(peerURI);

        if (found != mPeers.end()) {
          PeerPtr existingPeer = (*found).second.lock();
          if (existingPeer) {
            ZS_LOG_DEBUG(log("found existing peer to use") + IPeer::toDebugString(existingPeer))
            return existingPeer;
          }
          ZS_LOG_WARNING(Detail, log("existing peer in map is now gone") + ", peer URI=" + peerURI)
        }

        ZS_LOG_DEBUG(log("using newly created peer") + IPeer::toDebugString(peer))
        mPeers[peerURI] = peer;
        return peer;
      }

      //-----------------------------------------------------------------------
      void Account::notifyDestroyed(Peer &peer)
      {
        AutoRecursiveLock lock(getLock());

        String peerURI = peer.forAccount().getPeerURI();

        PeerMap::iterator found = mPeers.find(peerURI);
        if (found == mPeers.end()) {
          ZS_LOG_WARNING(Detail, log("existing location in map was already gone (probably okay)") + peer.forAccount().getDebugValueString())
          return;
        }

        ZS_LOG_DEBUG(log("notified peer is destroyed") + peer.forAccount().getDebugValueString())
        mPeers.erase(found);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &Account::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      IPeer::PeerFindStates Account::getPeerState(const String &peerURI) const
      {
        AutoRecursiveLock lock(getLock());
        PeerInfoMap::const_iterator found = mPeerInfos.find(peerURI);
        if (found == mPeerInfos.end()) {
          ZS_LOG_DEBUG(log("no state to get as peer URI was not found") + ", peer URI=" + peerURI)
          return IPeer::PeerFindState_Idle;
        }

        PeerInfoPtr peerInfo = (*found).second;
        return peerInfo->mCurrentFindState;
      }

      //-----------------------------------------------------------------------
      LocationListPtr Account::getPeerLocations(
                                                const String &peerURI,
                                                bool includeOnlyConnectedLocations
                                                ) const
      {
        AutoRecursiveLock lock(getLock());

        LocationListPtr result(new LocationList);

        PeerInfoMap::const_iterator found = mPeerInfos.find(peerURI);
        if (found == mPeerInfos.end()) {
          ZS_LOG_DEBUG(log("no peer locations as peer was not found") + ", peer URI=" + peerURI)
          return result;
        }

        const PeerInfoPtr &peerInfo = (*found).second;
        for (PeerInfo::PeerLocationMap::const_iterator iter = peerInfo->mLocations.begin(); iter != peerInfo->mLocations.end(); ++iter) {
          AccountPeerLocationPtr peerLocation = (*iter).second;
          if ((!includeOnlyConnectedLocations) ||
              (peerLocation->forAccount().isConnected())) {
            // only push back sessions that are actually connected (unless all are desired)
            ZS_LOG_TRACE(log("returning location") + AccountPeerLocation::toDebugString(peerLocation))
            result->push_back(peerLocation->forAccount().getLocation());
          }
        }

        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForPeerSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::subscribe(PeerSubscriptionPtr subscription)
      {
        AutoRecursiveLock lock(getLock());

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Debug, log("subscription happened during shutdown") + ", subscription=" + IPeerSubscription::toDebugString(subscription))
          // during the graceful shutdown or post shutdown process, new subscriptions must not be created...
          subscription->forAccount().notifyShutdown();
          return;
        }

        mPeerSubscriptions[subscription->forAccount().getID()] = subscription;

        if ((mFinder) &&
            (mFinderLocation)) {
          subscription->forAccount().notifyLocationConnectionStateChanged(mFinderLocation, toLocationConnectionState(mFinder->forAccount().getState()));
        }

        for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter) {
          PeerInfoPtr &peer = (*iter).second;

          subscription->forAccount().notifyFindStateChanged(peer->mPeer, peer->mCurrentFindState);

          for (PeerInfo::PeerLocationMap::iterator iterLocation = peer->mLocations.begin(); iterLocation != peer->mLocations.end(); ++iterLocation) {
            AccountPeerLocationPtr &peerLocation = (*iterLocation).second;
            LocationPtr location = peerLocation->forAccount().getLocation();

            if (location) {
              subscription->forAccount().notifyLocationConnectionStateChanged((location), toLocationConnectionState(peerLocation->forAccount().getState()));
            }
          }
        }

        PeerPtr subscribingToPeer = Peer::convert(subscription->forAccount().getSubscribedToPeer());
        if (!subscribingToPeer) {
          ZS_LOG_DEBUG(log("subscription was for all peers and not a specific peer"))
          return;
        }

        PeerURI uri = subscribingToPeer->forAccount().getPeerURI();

        PeerInfoPtr peerInfo;

        PeerInfoMap::iterator found = mPeerInfos.find(uri);

        if (mPeerInfos.end() == found) {
          // need to create a new peer from scratch and do the find
          peerInfo = PeerInfo::create();
          peerInfo->mPeer = subscribingToPeer;
          ZS_LOG_DEBUG(log("subscribing to new peer") + PeerInfo::toDebugString(peerInfo) + IPeerSubscription::toDebugString(subscription))
          mPeerInfos[uri] = peerInfo;
        } else {
          peerInfo = (*found).second;
          ZS_LOG_DEBUG(log("subscribing to existing peer") + PeerInfo::toDebugString(peerInfo) + IPeerSubscription::toDebugString(subscription))
        }

        ++(peerInfo->mTotalSubscribers);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      void Account::notifyDestroyed(PeerSubscription &subscription)
      {
        PeerSubscriptionMap::iterator found = mPeerSubscriptions.find(subscription.forAccount().getID());
        if (found == mPeerSubscriptions.end()) {
          ZS_LOG_WARNING(Detail, log("notification of destruction of unknown subscription (probably okay)") + subscription.forAccount().getDebugValueString())
          return;
        }

        ZS_LOG_DEBUG(log("notification of destruction of subscription") + subscription.forAccount().getDebugValueString())

        PeerPtr subscribingToPeer = Peer::convert(subscription.forAccount().getSubscribedToPeer());
        if (!subscribingToPeer) {
          ZS_LOG_DEBUG(log("subscription was for all peers and not a specific peer"))
          return;
        }

        PeerURI uri = subscribingToPeer->forAccount().getPeerURI();

        PeerInfoPtr peerInfo;

        PeerInfoMap::iterator foundPeerInfo = mPeerInfos.find(uri);

        if (mPeerInfos.end() != foundPeerInfo) {
          peerInfo = (*foundPeerInfo).second;
          ZS_LOG_DEBUG(log("unsubscribing from existing peer") + PeerInfo::toDebugString(peerInfo) + subscription.forAccount().getDebugValueString())

          --(peerInfo->mTotalSubscribers);
        }

        mPeerSubscriptions.erase(found);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepositoryPtr Account::getRepository() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mRepository) return PublicationRepositoryPtr();
        return mRepository;
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
                                                AccountFinderPtr finder,
                                                AccountStates state
                                                )
      {
        ZS_THROW_BAD_STATE_IF(!finder)

        ZS_LOG_DETAIL(log("received notification finder state changed") + ", notified state=" + toString(state) + AccountFinder::toDebugString(finder))
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("notification of finder state changed after shutdown"))
          return;
        }

        if (finder != mFinder) {
          ZS_LOG_WARNING(Detail, log("received state change on obsolete finder") + AccountFinder::toDebugString(finder))
          return;
        }

        notifySubscriptions(mFinderLocation, toLocationConnectionState(state));

        if (IAccount::AccountState_Ready == state) {
          mLastRetryFinderAfterDuration = Seconds(OPENPEER_STACK_ACCOUNT_FINDER_STARTING_RETRY_AFTER_IN_SECONDS);
        }

        if ((IAccount::AccountState_ShuttingDown == state) ||
            (IAccount::AccountState_Shutdown == state)) {

          mFinder.reset();

          if (!isShuttingDown()) {
            ZS_LOG_WARNING(Detail, log("did not expect finder to shutdown") + getDebugValueString())

            handleFinderRelatedFailure();
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      void Account::onAccountFinderMessageIncoming(
                                                   AccountFinderPtr finder,
                                                   MessagePtr message
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

        PeerLocationFindRequestPtr peerLocationFindRequest = PeerLocationFindRequest::convert(message);

        if (peerLocationFindRequest) {
          ZS_LOG_DEBUG(log("receiving incoming find peer location request"))

          LocationInfo fromLocationInfo = peerLocationFindRequest->locationInfo();

          LocationPtr fromLocation = Location::convert(fromLocationInfo.mLocation);

          PeerPtr fromPeer;
          if (fromLocation) {
            fromPeer = Peer::convert(fromLocation->forAccount().getPeer());
          }

          if ((!fromLocation) ||
              (!fromPeer)){
            ZS_LOG_WARNING(Detail, log("invalid request received") + ILocation::toDebugString(fromLocation))
            MessageResultPtr result = MessageResult::create(message, IHTTP::HTTPStatusCode_BadRequest);
            send(mFinderLocation, result);
            return;
          }

          ZS_LOG_DEBUG(log("received incoming peer find request") + ILocation::toDebugString(fromLocation))

          PeerInfoPtr peerInfo;

          PeerInfoMap::iterator foundPeer = mPeerInfos.find(fromLocation->forAccount().getPeerURI());
          if (foundPeer != mPeerInfos.end()) {
            peerInfo = (*foundPeer).second;
            ZS_LOG_DEBUG(log("received incoming peer find request from known peer") + PeerInfo::toDebugString(peerInfo))
          } else {
            peerInfo = PeerInfo::create();
            peerInfo->mPeer = fromPeer;
            mPeerInfos[fromPeer->forAccount().getPeerURI()] = peerInfo;
            ZS_LOG_DEBUG(log("received incoming peer find request from unknown peer") + PeerInfo::toDebugString(peerInfo))
          }

          AccountPeerLocationPtr peerLocation;
          PeerInfo::PeerLocationMap::iterator foundLocation = peerInfo->mLocations.find(fromLocation->forAccount().getLocationID());
          if (foundLocation != peerInfo->mLocations.end()) {
            // already conducting a search for this location?
            peerLocation = (*foundLocation).second;
            ZS_LOG_DEBUG(log("received incoming peer find request from known peer location") + PeerInfo::toDebugString(peerInfo))

            if (peerLocation->forAccount().hasReceivedCandidateInformation()) {
              ZS_LOG_WARNING(Detail, log("receiving candidate information for the same peer location thus shutting down current location") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation) + ILocation::toDebugString(fromLocation))
              peerInfo->mLocations.erase(foundLocation);
              peerLocation->forAccount().shutdown();

              notifySubscriptions(fromLocation, ILocation::LocationConnectionState_Disconnected);

              foundLocation = peerInfo->mLocations.end();
              peerLocation.reset();
            }
          }

          if (foundLocation == peerInfo->mLocations.end()) {
            peerLocation = IAccountPeerLocationForAccount::create(
                                                                  mThisWeak.lock(),
                                                                  mThisWeak.lock(),
                                                                  fromLocationInfo
                                                                  );

            peerInfo->mLocations[fromLocation->forAccount().getLocationID()] = peerLocation;
            ZS_LOG_DEBUG(log("received incoming peer find request from unknown peer location") + PeerInfo::toDebugString(peerInfo))
          }

          peerLocation->forAccount().incomingRespondWhenCandidatesReady(peerLocationFindRequest);
          return;
        }

        ChannelMapNotifyPtr channelMapNotify = ChannelMapNotify::convert(message);
        if (channelMapNotify) {
          ZS_LOG_DEBUG(log("channel map notification received") + ", channel number=" + string(channelMapNotify->channelNumber()))

          // HERE
          IPAddress finderIP;
          String relayAccessToken;
          String relayAccessSecret;
          mFinder->forAccount().getFinderRelayInformation(finderIP, relayAccessToken, relayAccessSecret);

          String nonce = channelMapNotify->nonce();
          String localContext = channelMapNotify->localContext();
          String remoteContext = channelMapNotify->remoteContext();
          ChannelMapNotify::ChannelNumber channel = channelMapNotify->channelNumber();
          Time expires = channelMapNotify->relayAccessSecretProofExpires();
          String hash = channelMapNotify->relayAccessSecretProof();

          //  hex(hash("proof:" + <client-nonce> + ":" + <remote-context> + ":" + <channel-number> + ":" + <expires> + ":" + hex(hmac(<relay-access-secret>, "finder-relay-access-validate:" + <relay-access-token> + ":" + <local-context> + ":channel-map"))))

          // hex(hmac(<relay-access-secret>, "finder-relay-access-validate:" + <relay-access-token> + ":" + <local-context> + ":channel-map")
          String innerHash = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(relayAccessSecret), "finder-relay-access-validate:" + relayAccessToken + ":" + localContext + ":channel-map"));

          //  hex(hash("proof:" + <client-nonce> + ":" + <remote-context> + ":" + <channel-number> + ":" + <expires> + ":" + <innerHash>))
          String calculatedProof = IHelper::convertToHex(*IHelper::hash("proof:" + nonce + ":" + remoteContext + ":" + string(channel) + ":" + IHelper::timeToString(expires)));

          if (calculatedProof != channelMapNotify->relayAccessSecretProof()) {
            ZS_LOG_WARNING(Detail, log("channel map notify proof failed") + ", calculated proof=" + calculatedProof + ", received=" + channelMapNotify->relayAccessSecretProof())
            return;
          }

          // proof passed, this is a proper relay map notification
          for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter)
          {
            const String &peerURI = (*iter).first;
            PeerInfoPtr &peerInfo = (*iter).second;

            String peerContext = getLocalContextID(peerURI);

            if (peerContext == localContext) {
              RelayInfoPtr relayInfo = RelayInfo::create();
              relayInfo->mChannel = channel;
              relayInfo->mLocalContext = localContext;
              relayInfo->mRemoteContext = remoteContext;

              peerInfo->mRelayInfos[channel] = relayInfo;

              ZS_LOG_DEBUG(log("peer relay context added") + relayInfo->getDebugValueString() + peerInfo->getDebugValueString())
              break;
            }
          }

          return;
        }

        MessageIncomingPtr messageIncoming = IMessageIncomingForAccount::create(mThisWeak.lock(), mFinderLocation, message);
        notifySubscriptions(messageIncoming);
      }

      //-----------------------------------------------------------------------
      void Account::onAccountFinderIncomingRelayChannel(
                                                        AccountFinderPtr finder,
                                                        IFinderRelayChannelPtr relayChannel,
                                                        ITransportStreamPtr receiveStream,
                                                        ITransportStreamPtr sendStream,
                                                        ChannelNumber channelNumber
                                                        )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!relayChannel)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        AutoRecursiveLock lock(getLock());

        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("ignoring incoming finder request because shutting down or shutdown"))
          return;
        }

        if (finder != mFinder) {
          ZS_LOG_DEBUG(log("finder does not match current finder (ignoring request)"))
          return;
        }

        for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter)
        {
          PeerInfoPtr &peerInfo = (*iter).second;

          RelayInfoMap::iterator found = peerInfo->mRelayInfos.find(channelNumber);
          if (found == peerInfo->mRelayInfos.end()) continue;

          RelayInfoPtr &relayInfo = (*found).second;

          relayInfo->mRelayChannel = relayChannel;
          relayInfo->mReceiveStream = receiveStream->getReader();
          relayInfo->mSendStream = sendStream->getWriter();

          relayInfo->mRelayChannelSubscription = relayChannel->subscribe(mThisWeak.lock());
          relayInfo->mReceiveStreamSubscription = relayInfo->mReceiveStream->subscribe(mThisWeak.lock());
          relayInfo->mSendStreamSubscription = relayInfo->mSendStream->subscribe(mThisWeak.lock());

          ZS_LOG_DEBUG(log("incoming finder relay channel setup") + relayInfo->getDebugValueString() + peerInfo->getDebugValueString())
        }
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
                                                      AccountPeerLocationPtr peerLocation,
                                                      AccountStates state
                                                      )
      {
        ZS_THROW_BAD_STATE_IF(!peerLocation)

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("notified account peer location state changed when account was shutdown") + ", notified state=" + toString(state) + AccountPeerLocation::toDebugString(peerLocation))
          return;
        }

        ZS_LOG_DETAIL(log("notified account peer location state changed") + ", notified state=" + toString(state) + AccountPeerLocation::toDebugString(peerLocation))

        Time tick = zsLib::now();

        LocationPtr location = peerLocation->forAccount().getLocation();

        ZS_LOG_DEBUG(log("notified about peer location") + ILocation::toDebugString(location))

        PeerInfoMap::iterator found = mPeerInfos.find(location->forAccount().getPeerURI());
        if (found == mPeerInfos.end()) {
          ZS_LOG_WARNING(Debug, log("notified peer location state changed but was not found but peer was not found"))
          return;
        }

        PeerInfoPtr &peerInfo = (*found).second;
        PeerInfo::PeerLocationMap::iterator foundLocation = peerInfo->mLocations.find(location->forAccount().getLocationID());
        if (foundLocation == peerInfo->mLocations.end()) {
          ZS_LOG_WARNING(Debug, log("could not find peer location") + PeerInfo::toDebugString(peerInfo) + ILocation::toDebugString(location))
          return;
        }

        AccountPeerLocationPtr &foundPeerLocation = (*foundLocation).second;
        if (foundPeerLocation != peerLocation) {
          ZS_LOG_WARNING(Detail, log("notification of peer state on obsolete peer location") + PeerInfo::toDebugString(peerInfo) + ILocation::toDebugString(location))
          return;
        }

        notifySubscriptions(location, toLocationConnectionState(state));

        switch (state) {
          case IAccount::AccountState_Ready:
          {
            peerInfo->mPreventCrazyRefindNextTime = false;
            break;
          }
          case IAccount::AccountState_Shutdown:
          {
            bool findAgain = peerLocation->forAccount().shouldRefindNow();

            // found the peer location, clear it out...
            peerInfo->mLocations.erase(foundLocation);
            ZS_LOG_DEBUG(log("peer location is shutdown") + PeerInfo::toDebugString(peerInfo) + ILocation::toDebugString(location))

            if (findAgain) {
              ZS_LOG_DETAIL(log("need to refind peer at next opportunity") + PeerInfo::toDebugString(peerInfo) + ILocation::toDebugString(location))
              peerInfo->mFindAtNextPossibleMoment = true;
            }

            if (peerInfo->mLocations.size() < 1) {
              if (peerInfo->mPreventCrazyRefindNextTime) {
                // prevent the peer from going into a crazy finding/connecting/remote side closed/find again loop
                if (peerInfo->mNextScheduledFind < tick) {
                  ZS_LOG_WARNING(Detail, log("preventing crazy finding/connecting/remote side refused/find again loop for peer"))
                  peerInfo->findTimeScheduleNext();
                }
              } else {
                ZS_LOG_DETAIL(log("will prevent crazy finding/connecting/remote side refused/find again loop for peer next time"))
                peerInfo->mPreventCrazyRefindNextTime = true;
              }
            }
            break;
          }
          default:  break;
        }

        step();
      }

      //-----------------------------------------------------------------------
      void Account::onAccountPeerLocationMessageIncoming(
                                                         AccountPeerLocationPtr peerLocation,
                                                         MessagePtr message
                                                         )
      {
        AutoRecursiveLock lock(getLock());

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("message incoming ignored as account shutdown"))
          return;
        }

        LocationPtr location = peerLocation->forAccount().getLocation();

        PeerInfoMap::iterator found = mPeerInfos.find(location->forAccount().getPeerURI());
        if (found == mPeerInfos.end()) {
          ZS_LOG_WARNING(Detail, log("incoming message coming in from unknown peer location") + AccountPeerLocation::toDebugString(peerLocation) + ILocation::toDebugString(location))
          return;
        }

        PeerInfoPtr peerInfo = (*found).second;

        ZS_LOG_DEBUG(log("handling message") + PeerInfo::toDebugString(peerInfo))

        MessageIncomingPtr messageIncoming = IMessageIncomingForAccount::create(mThisWeak.lock(), location, message);
        notifySubscriptions(messageIncoming);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForServiceLockboxSession
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::notifyServiceLockboxSessionStateChanged()
      {
        AutoRecursiveLock lock(getLock());
        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IDNSDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onLookupCompleted(IDNSQueryPtr query)
      {
        AutoRecursiveLock lock(getLock());
        if (query != mFinderDNSLookup) {
          ZS_LOG_DEBUG(log("notified about obsolete DNS query"))
          return;
        }

        mAvailableFinderSRVResult = query->getSRV();

        mFinderDNSLookup->cancel();
        mFinderDNSLookup.reset();

        if (mFinder) {
          ZS_LOG_DEBUG(log("notifying existing finder DNS lookup is complete"))
          mFinder->forAccount().notifyFinderDNSComplete();
        }

        if (!mAvailableFinderSRVResult) {
          ZS_LOG_ERROR(Detail, log("SRV DNS lookoup failed to return result"))

          mAvailableFinders.pop_front();
          if (mAvailableFinders.size() < 1) {
            ZS_LOG_ERROR(Detail, log("all finders failed to resolve or connect"))
            handleFinderRelatedFailure();
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IRUDPICESocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onRUDPICESocketStateChanged(
                                                IRUDPICESocketPtr socket,
                                                RUDPICESocketStates state
                                                )
      {
        ZS_LOG_DEBUG(log("on rudp ice socket state changed"))

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      void Account::onRUDPICESocketCandidatesChanged(IRUDPICESocketPtr socket)
      {
        ZS_LOG_DEBUG(log("on rudp ice socket candidates changed"))

        AutoRecursiveLock lock(getLock());
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
      bool Account::handleMessageMonitorMessageReceived(
                                                        IMessageMonitorPtr monitor,
                                                        MessagePtr message
                                                        )
      {
        ZS_LOG_DEBUG(log("message requester received response") + IMessageMonitor::toDebugString(monitor))
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("received response after already shutdown"))
          return false;
        }

        if (monitor == mFindersGetMonitor) {
          mFindersGetMonitor->cancel();
          mFindersGetMonitor.reset();

          FindersGetResultPtr result = FindersGetResult::convert(message);
          if (!result) {
            MessageResultPtr result = MessageResult::convert(message);
            ZS_LOG_ERROR(Detail, log("finders get failed, will try later") + Message::toDebugString(message))

            handleFinderRelatedFailure();
            IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
            return true;
          }

          mAvailableFinders = result->finders();
          if (mAvailableFinders.size() < 1) {
            ZS_LOG_ERROR(Detail, log("finders get failed to return any finders"))
            handleFinderRelatedFailure();
          }

          IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
          return true;
        }

        for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter) {
          PeerInfoPtr &peerInfo = (*iter).second;

          if (monitor != peerInfo->mPeerFindMonitor) {
            continue;
          }

          switch (message->messageType()) {
            case Message::MessageType_Result:  {
              MessageResultPtr result = MessageResult::convert(message);
              if (result->hasAttribute(MessageResult::AttributeType_ErrorCode)) {
                ZS_LOG_ERROR(Debug, log("received requester received response error") + Message::toDebugString(result))
                handleFindRequestComplete(monitor);
                return true;
              }

              PeerLocationFindResultPtr findResult = PeerLocationFindResult::convert(result);
              if (!findResult) {
                ZS_LOG_ERROR(Debug, log("was unable to cast to a find result (ignoring)"))
                return false;
              }

              // prepare all the locations in the result hat do not yet exist
              const LocationInfoList &locations = findResult->locations();
              if (locations.size() < 1) {
                ZS_LOG_DEBUG(log("finder reply contained no location thus there are no location replies possible"))
                handleFindRequestComplete(monitor);
                return true;
              }

              for (LocationInfoList::const_iterator iterLocation = locations.begin(); iterLocation != locations.end(); ++iterLocation) {
                const LocationInfo &locationInfo = (*iterLocation);

                LocationPtr location = Location::convert(locationInfo.mLocation);

                if (!location) {
                  ZS_LOG_WARNING(Detail, log("location info missing location object"))
                  continue;
                }
                PeerInfo::PeerLocationMap::iterator found = peerInfo->mLocations.find(location->forAccount().getLocationID());
                if (found != peerInfo->mLocations.end()) {
                  ZS_LOG_DEBUG(log("already knows about this location thus ignoring find result") + ILocation::toDebugString(location))
                  continue;   // already know about this location? if so just skip it
                }

                ZS_LOG_DEBUG(log("receiced find result for location") + ILocation::toDebugString(location))

                // scope: see if this would cause a seach redo later (if so, stop it from happening)
                {
                  PeerInfo::FindingBecauseOfLocationIDMap::iterator redoFound = peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.find(location->forAccount().getLocationID());
                  if (redoFound != peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.end()) {
                    ZS_LOG_DEBUG(log("receiced find result for location that would be searched later thus removing from later search"))
                    peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.erase(redoFound);
                  }
                }

                // don't know this location, remember it for later
                AccountPeerLocationPtr peerLocation = IAccountPeerLocationForAccount::create(
                                                                                             mThisWeak.lock(),
                                                                                             mThisWeak.lock(),
                                                                                             locationInfo
                                                                                             );
                peerInfo->mLocations[location->forAccount().getLocationID()] = peerLocation;

                // the act of finding a peer does not cause notification to the subscribers as only the establishment of a peer connection notifies the subscribers
              }
              break;
            }
            case Message::MessageType_Notify:   {
              PeerLocationFindNotifyPtr findReply = PeerLocationFindNotify::convert(message);
              if (!findReply) {
                ZS_LOG_ERROR(Debug, log("receiced received a find reply but was unable to cast object to a reply object") + PeerInfo::toDebugString(peerInfo))
                return false;
              }

              const LocationInfo &locationInfo = findReply->locationInfo();

              LocationPtr location = Location::convert(locationInfo.mLocation);

              CandidateList candidates = locationInfo.mCandidates;
              if (candidates.size() < 1) {
                ZS_LOG_ERROR(Debug, log("receiced received a find reply but it did not contain any candidates (thus ignoring reply)") + PeerInfo::toDebugString(peerInfo))
                return false;
              }

              PeerInfo::PeerLocationMap::iterator found = peerInfo->mLocations.find(location->forAccount().getLocationID());

              AccountPeerLocationPtr peerLocation;
              if (found != peerInfo->mLocations.end()) {
                peerLocation = (*found).second;
                ZS_LOG_DEBUG(log("receiced received a find reply to an existing known peer location") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation) + ILocation::toDebugString(location))

                if (peerLocation->forAccount().hasReceivedCandidateInformation()) {
                  ZS_LOG_WARNING(Detail, log("receiving candidate information for the same peer location thus shutting down current location") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation) + ILocation::toDebugString(location))
                  peerInfo->mLocations.erase(found);
                  peerLocation->forAccount().shutdown();

                  notifySubscriptions(location, ILocation::LocationConnectionState_Disconnected);

                  found = peerInfo->mLocations.end();
                  peerLocation.reset();
                }
              }

              if (found == peerInfo->mLocations.end()) {
                ZS_LOG_DEBUG(log("receiced received a find reply to a new unknown peer") + PeerInfo::toDebugString(peerInfo) + ILocation::toDebugString(location))

                peerLocation = IAccountPeerLocationForAccount::create(
                                                                      mThisWeak.lock(),
                                                                      mThisWeak.lock(),
                                                                      locationInfo
                                                                      );
                peerInfo->mLocations[location->forAccount().getLocationID()] = peerLocation;

                // scope: see if this would cause a seach redo later (if so, stop it from happening)
                {
                  PeerInfo::FindingBecauseOfLocationIDMap::iterator redoFound = peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.find(location->forAccount().getLocationID());
                  if (redoFound != peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.end()) {
                    ZS_LOG_DEBUG(log("receiced find reply for location that would be searched later thus removing from later search") + PeerInfo::toDebugString(peerInfo))
                    peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.erase(redoFound);
                  }
                }

                // in this case not only was a location found but the request to find the location came back from the remote party with a reply but it's not sufficient reason
                // to notify any subscribers since they only care when a connection is actually established.
              }

              PeerLocationFindRequestPtr request = PeerLocationFindRequest::convert(monitor->getMonitoredMessage());

              String remoteContext = findReply->context();
              String remotePassword = findReply->peerSecret();
              String remoteICEUsernameFrag = findReply->iceUsernameFrag();
              String remoteICEPassword = findReply->icePassword();

              // scope: match any relay channels to the remote context
              {
                for (RelayInfoMap::iterator relayIter = peerInfo->mRelayInfos.begin(); relayIter != peerInfo->mRelayInfos.end(); ++relayIter)
                {
                  RelayInfoPtr &relayInfo = (*relayIter).second;

                  if (relayInfo->mRemoteContext != remoteContext) continue;

                  ZS_LOG_DEBUG(log("found relay associated to remote context") + relayInfo->getDebugValueString())

                  relayInfo->mAccountPeerLocation = peerLocation;
                  if (relayInfo->mRelayChannel) {
                    peerLocation->forAccount().notifyIncomingRelayChannel(relayInfo->mRelayChannel);
                  }
                  break;
                }
              }

              peerLocation->forAccount().connectLocation(remoteContext, remotePassword, remoteICEUsernameFrag, remoteICEPassword, candidates, IICESocket::ICEControl_Controlling);

              bool locationRemainThatHaveNotReplied = false;
              // scope: check to see if their are remaining locations yet to return their replies
              {
                for (PeerInfo::PeerLocationMap::iterator iterLocation = peerInfo->mLocations.begin(); iterLocation != peerInfo->mLocations.end(); ++iterLocation) {
                  AccountPeerLocationPtr &peerLocation = (*iterLocation).second;
                  if (!peerLocation->forAccount().hasReceivedCandidateInformation()) {
                    ZS_LOG_DEBUG(log("at least one peer still waiting for result") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
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

              handleFindRequestComplete(monitor);

              peerInfo->findTimeReset();
              return true;
            }
            default: return false;  // ignore these since we aren't expecting them
          }
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void Account::onMessageMonitorTimedOut(IMessageMonitorPtr monitor)
      {
        ZS_LOG_WARNING(Detail, log("message requester timed out") + IMessageMonitor::toDebugString(monitor))

        handleFindRequestComplete(monitor);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IWakeDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onWake()
      {
        ZS_LOG_DEBUG(log("on wake"))
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IFinderRelayChannelDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onFinderRelayChannelStateChanged(
                                                     IFinderRelayChannelPtr channel,
                                                     IFinderRelayChannel::SessionStates state
                                                     )
      {
        ZS_LOG_DEBUG(log("on finder relay channel state changed"))

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("received response after already shutdown"))
          return;
        }

        switch (state) {
          case IFinderRelayChannel::SessionState_Pending:
          case IFinderRelayChannel::SessionState_Connected: {
            ZS_LOG_DEBUG(log("ignoring pending / connected state on relay channel") + ", state=" + IFinderRelayChannel::toString(state) + IFinderRelayChannel::toDebugString(channel))
            break;
          }
          case IFinderRelayChannel::SessionState_Shutdown: {
            ZS_LOG_DEBUG(log("finder relay channel shutdown") + ", state=" + IFinderRelayChannel::toString(state) + IFinderRelayChannel::toDebugString(channel))
            break;
          }
        }

        // HERE
        for (PeerInfoMap::iterator peerIter = mPeerInfos.begin(); peerIter != mPeerInfos.end(); ++peerIter) {
          PeerInfoPtr &peerInfo = (*peerIter).second;

          for (RelayInfoMap::iterator relayIter = peerInfo->mRelayInfos.begin(); relayIter != peerInfo->mRelayInfos.end(); ++relayIter)
          {
            RelayInfoPtr &relayInfo = (*relayIter).second;
            if (relayInfo->mRelayChannel->getID() == channel->getID()) {
              ZS_LOG_DEBUG(log("found shutdown relay channel") + relayInfo->getDebugValueString())

              relayInfo->cancel();
              peerInfo->mRelayInfos.erase(relayIter);
              return;
            }
          }
        }

        ZS_LOG_WARNING(Detail, log("relay channel not associated with peer") + IFinderRelayChannel::toDebugString(channel))
      }

      //-----------------------------------------------------------------------
      void Account::onFinderRelayChannelNeedsContext(IFinderRelayChannelPtr channel)
      {
        ZS_LOG_DEBUG(log("on finder relay channel needs context"))
        AutoRecursiveLock lock(getLock());

        for (PeerInfoMap::iterator peerIter = mPeerInfos.begin(); peerIter != mPeerInfos.end(); ++peerIter) {
          const String &peerURI = (*peerIter).first;
          PeerInfoPtr &peerInfo = (*peerIter).second;

          for (RelayInfoMap::iterator relayIter = peerInfo->mRelayInfos.begin(); relayIter != peerInfo->mRelayInfos.end(); ++relayIter)
          {
            RelayInfoPtr &relayInfo = (*relayIter).second;
            if (relayInfo->mRelayChannel->getID() == channel->getID()) {
              ZS_LOG_DEBUG(log("found relay channel needing context") + relayInfo->getDebugValueString() + IFinderRelayChannel::toDebugString(channel))

              channel->setIncomingContext(relayInfo->mLocalContext, getLocalPassword(peerURI), peerInfo->mPeer);
              return;
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => ITransportStreamWriterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
      {
        ZS_LOG_DEBUG(log("on stream writer ready"))
        AutoRecursiveLock lock(getLock());

        // HERE
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => ITransportStreamReaderDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
      {
        ZS_LOG_DEBUG(log("on stream reader ready"))
        AutoRecursiveLock lock(getLock());

        for (PeerInfoMap::iterator peerIter = mPeerInfos.begin(); peerIter != mPeerInfos.end(); ++peerIter) {
          PeerInfoPtr &peerInfo = (*peerIter).second;

          for (RelayInfoMap::iterator relayIter = peerInfo->mRelayInfos.begin(); relayIter != peerInfo->mRelayInfos.end(); ++relayIter)
          {
            RelayInfoPtr &relayInfo = (*relayIter).second;
            if (relayInfo->mReceiveStream->getID() == reader->getID()) {
              ZS_LOG_DEBUG(log("reading relay stream") + relayInfo->getDebugValueString() + ITransportStream::toDebugString(reader->getStream()))

              while (true) {
                SecureByteBlockPtr buffer = reader->read();
                if (!buffer) {
                  ZS_LOG_TRACE(log("no more data to read"))
                }

                // HERE NOW

                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
                ZS_LOG_DETAIL(log("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<"))
                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
                ZS_LOG_DETAIL(log("RELAY RECEIVED MESSAGE=") + "\n" + ((CSTR)buffer->BytePtr()) + "\n")
                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
                ZS_LOG_DETAIL(log("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<"))
                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

                DocumentPtr document = Document::createFromAutoDetect(((CSTR)buffer->BytePtr()));
                MessagePtr message = Message::create(
                                                     document,
                                                     relayInfo->mAccountPeerLocation ?
                                                      relayInfo->mAccountPeerLocation->forAccount().getLocation() :
                                                      ILocationForAccount::getForFinder(mThisWeak.lock())
                                                     );

                if (IMessageMonitor::handleMessageReceived(message)) {
                  ZS_LOG_DEBUG(log("handled message via message handler"))
                  return;
                }

                if (relayInfo->mAccountPeerLocation) {
                  relayInfo->mAccountPeerLocation->forAccount().handleMessage(message, true);
                  continue;
                }

                // HERE NOW
              }
            }
          }
        }
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
          ZS_LOG_WARNING(Detail, log("received timer notification on obsolete timer (probably okay)") + ", timer ID=" + string(timer->getID()))
          return;
        }

        Time tick = zsLib::now();

        if (mLastTimerFired + Seconds(OPENPEER_STACK_ACCOUNT_TIMER_DETECTED_BACKGROUNDING_TIME_IN_SECONDS) < tick) {
          ZS_LOG_WARNING(Detail, log("account timer detected account when into background"))

          mBlockLocationShutdownsUntil = tick + Seconds(OPENPEER_STACK_ACCOUNT_PREVENT_LOCATION_SHUTDOWNS_AFTER_BACKGROUNDING_FOR_IN_SECONDS);
        }

        mLastTimerFired = tick;
        step();
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
        return String("stack::Account [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      String Account::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());

        String turn;
        String username;
        String password;
        String stun;
        getNATServers(turn, username, password, stun);

        bool firstTime = !includeCommaPrefix;
        return
        Helper::getDebugValue("stack account id", string(mID), firstTime) +
        Helper::getDebugValue("state", IAccount::toString(mCurrentState), firstTime) +
        Helper::getDebugValue("location ID", mLocationID, firstTime) +
        Helper::getDebugValue("error code", 0 != mLastError ? string(mLastError) : String(), firstTime) +
        Helper::getDebugValue("error reason", mLastErrorReason, firstTime) +
        Helper::getDebugValue("timer last fired", Time() != mLastTimerFired ? IHelper::timeToString(mLastTimerFired) : String(), firstTime) +
        Helper::getDebugValue("block until", Time() != mBlockLocationShutdownsUntil ? IHelper::timeToString(mBlockLocationShutdownsUntil) : String(), firstTime) +
        Helper::getDebugValue("turn", turn, firstTime) +
        Helper::getDebugValue("turn username", username, firstTime) +
        Helper::getDebugValue("turn password", password, firstTime) +
        Helper::getDebugValue("stun", stun, firstTime) +
        //mSelfLocation->forAccount().getDebugValueString() +
        Helper::getDebugValue("master peer secret", mMasterPeerSecret, firstTime) +
        Helper::getDebugValue("finder", mFinder ? String("true") : String(), firstTime) +
        Helper::getDebugValue("finder retry after", Time() != mFinderRetryAfter ? IHelper::timeToString(mFinderRetryAfter) : String(), firstTime) +
        Helper::getDebugValue("peer infos", mPeers.size() > 0 ? string(mPeers.size()) : String(), firstTime) +
        Helper::getDebugValue("peer infos", mPeerInfos.size() > 0 ? string(mPeerInfos.size()) : String(), firstTime) +
        Helper::getDebugValue("subscribers", mPeerSubscriptions.size() > 0 ? string(mPeerSubscriptions.size()) : String(), firstTime) +
        Helper::getDebugValue("locations", mLocations.size() > 0 ? string(mLocations.size()) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void Account::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called") + getDebugValueString())

        AutoRecursiveLock lock(getLock());  // just in case

        if (isShutdown()) return;

        setState(IAccount::AccountState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mRepository) {
          mRepository->forAccount().cancel();
          mRepository.reset();
        }

        if (mFindersGetMonitor) {
          mFindersGetMonitor->cancel();
          mFindersGetMonitor.reset();
        }

        if (mFinderDNSLookup) {
          mFinderDNSLookup->cancel();
          mFinderDNSLookup.reset();
        }

        // scope: kill all the connection subscriptions
        {
          for (PeerSubscriptionMap::iterator iter = mPeerSubscriptions.begin(); iter != mPeerSubscriptions.end(); )
          {
            PeerSubscriptionMap::iterator current = iter;
            ++iter;

            PeerSubscriptionPtr subscriber = (*current).second.lock();
            subscriber->forAccount().notifyShutdown();
          }
        }

        // scope: kill all the subscriptions and the find requests since they are not needed anymore
        {
          for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter) {
            PeerInfoPtr &peerInfo = (*iter).second;

            if (peerInfo->mPeerFindMonitor) {
              setFindState(*(peerInfo.get()), IPeer::PeerFindState_Completed);
              setFindState(*(peerInfo.get()), IPeer::PeerFindState_Idle);
            }

            // after this point all subscriptions are considered "dead" and no new ones are allowed to be created
            peerInfo->mTotalSubscribers = 0;

            if (peerInfo->mPeerFindMonitor) {
              ZS_LOG_DEBUG(log("cancelling / stopping find request for peer") + PeerInfo::toDebugString(peerInfo))

              // we have to kill all requests to find more peer locations (simply ignore the results from the requests)
              peerInfo->mPeerFindMonitor->cancel();
              peerInfo->mPeerFindMonitor.reset();
            }
          }
        }

        if (mGracefulShutdownReference) {

          if (mPeerInfos.size() > 0) {
            for (PeerInfoMap::iterator peerIter = mPeerInfos.begin(); peerIter != mPeerInfos.end(); ) {
              PeerInfoMap::iterator current = peerIter;
              ++peerIter;

              PeerInfoPtr peerInfo = (*current).second;

              if (peerInfo->mLocations.size() > 0) {
                for (PeerInfo::PeerLocationMap::iterator iterLocation = peerInfo->mLocations.begin(); iterLocation != peerInfo->mLocations.end(); ++iterLocation) {
                  // send a shutdown request to each outstanding location
                  AccountPeerLocationPtr peerLocation = (*iterLocation).second;

                  ZS_LOG_DEBUG(log("cancel shutting down peer location") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
                  peerLocation->forAccount().shutdown();
                }
              } else {
                // we don't need to remember this connection anymore since no locations were found or established
                ZS_LOG_DEBUG(log("cancel closing down peer") + PeerInfo::toDebugString(peerInfo))
                mPeerInfos.erase(current);
              }
            }
          }

          if (mFinder) {
            ZS_LOG_DEBUG(log("shutting down peer finder") + AccountFinder::toDebugString(mFinder))
            mFinder->forAccount().shutdown();
          }

          if (mPeerInfos.size() > 0) {
            ZS_LOG_DEBUG(log("shutdown still waiting for all peer locations to shutdown..."))
            return;
          }

          if (mFinder) {
            if (IAccount::AccountState_Shutdown != mFinder->forAccount().getState()) {
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

        // scope: clear out peers that have not had their locations shutdown
        {
          for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter) {
            PeerInfoPtr &peerInfo = (*iter).second;

            for (PeerInfo::PeerLocationMap::iterator iterLocation = peerInfo->mLocations.begin(); iterLocation != peerInfo->mLocations.end(); ++iterLocation) {
              AccountPeerLocationPtr peerLocation = (*iterLocation).second;
              ZS_LOG_DEBUG(log("hard shutdown of peer location") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
              peerLocation->forAccount().shutdown();
            }
            peerInfo->mLocations.clear();
          }

          mPeerInfos.clear();
        }

        if (mTimer) {
          mTimer->cancel();
          mTimer.reset();
        }

        if (mFinder) {
          ZS_LOG_DEBUG(log("hard shutdown of peer finder"))
          mFinder->forAccount().shutdown();
          mFinder.reset();
        }

        if (mSocket) {
          ZS_LOG_DEBUG(log("hard shutdown of socket"))
          mSocket->shutdown();
          mSocket.reset();
        }

        ZS_LOG_DEBUG(log("service lockbox disconnected"))
        mLockboxSession.reset();

        ZS_LOG_DEBUG(log("shutdown complete"))
      }

      //-----------------------------------------------------------------------
      void Account::step()
      {
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("step forwarding to cancel"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!stepTimer()) return;
        if (!stepRepository()) return;
        if (!stepLockboxSession()) return;
        if (!stepLocations()) return;
        if (!stepSocket()) return;
        if (!stepFinder()) return;

        setState(AccountState_Ready);

        if (!stepPeers()) return;

        ZS_LOG_DEBUG(log("step complete") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      bool Account::stepTimer()
      {
        if (mTimer) {
          ZS_LOG_TRACE(log("already have timer"))
          return true;
        }

        mLastTimerFired = zsLib::now();
        mTimer = Timer::create(mThisWeak.lock(), Seconds(OPENPEER_STACK_ACCOUNT_TIMER_FIRES_IN_SECONDS));
        ZS_LOG_TRACE(log("created timer") + ", timer ID=" + string(mTimer->getID()))
        return true;
      }

      //-----------------------------------------------------------------------
      bool Account::stepRepository()
      {
        if (mRepository) {
          ZS_LOG_TRACE(log("already have repository"))
          return true;
        }

        mRepository = IPublicationRepositoryForAccount::create(mThisWeak.lock());
        ZS_LOG_TRACE(log("repository created") + ", respository ID=" + string(mRepository->forAccount().getID()))
        return true;
      }

      //-----------------------------------------------------------------------
      bool Account::stepLockboxSession()
      {
        ZS_THROW_BAD_STATE_IF(!mLockboxSession)

        if ((mSTUN) ||
            (mTURN)) {
          ZS_LOG_TRACE(log("lockbox session is already setup"))
          return true;
        }

        IServiceLockboxSession::SessionStates state = mLockboxSession->forAccount().getState();
        switch (state) {
          case IServiceLockboxSession::SessionState_Pending:
          case IServiceLockboxSession::SessionState_PendingPeerFilesGeneration:
          {
            ZS_LOG_TRACE(log("lockbox session pending"))
            return false;
          }
          case IServiceLockboxSession::SessionState_Shutdown:
          {
            ZS_LOG_ERROR(Detail, log("lockbox session is shutdown thus account must shutdown"))
            WORD errorCode = 0;
            String reason;
            mLockboxSession->forAccount().getState(&errorCode, &reason);
            setError(errorCode, reason);
            return false;
          }
          case IServiceLockboxSession::SessionState_Ready:  {
            ZS_LOG_TRACE(log("lockbox session is ready"))
            break;
          }
        }

        if (!mTURN) {
          ZS_LOG_DEBUG(log("creating TURN session"))
          mTURN = mLockboxSession->forAccount().findServiceMethod("turn", "turn");
        }
        if (!mSTUN) {
          mSTUN = mLockboxSession->forAccount().findServiceMethod("stun", "stun");
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool Account::stepLocations()
      {
        if (mSelfPeer) {
          ZS_LOG_TRACE(log("already have a self peer"))
          ZS_THROW_BAD_STATE_IF(!mSelfLocation)
          ZS_THROW_BAD_STATE_IF(!mFinderLocation)
          return true;
        }

        IPeerFilesPtr peerFiles = mLockboxSession->forAccount().getPeerFiles();
        if (!peerFiles) {
          ZS_LOG_ERROR(Detail, log("peer files are missing"))
          setError(IHTTP::HTTPStatusCode_PreconditionFailed, "Peer files are missing");
          cancel();
          return false;
        }

        ZS_LOG_TRACE(log("peer files are ready"))

        IPeerFilePublicPtr peerFilePublic = peerFiles->getPeerFilePublic();
        IPeerFilePrivatePtr peerFilePrivate = peerFiles->getPeerFilePrivate();

        ZS_THROW_BAD_STATE_IF(!peerFilePublic)
        ZS_THROW_BAD_STATE_IF(!peerFilePrivate)

        mSelfPeer = IPeerForAccount::create(mThisWeak.lock(), peerFilePublic);
        mSelfLocation = ILocationForAccount::getForLocal(mThisWeak.lock());
        mFinderLocation = ILocationForAccount::getForFinder(mThisWeak.lock());
        return true;
      }

      //-----------------------------------------------------------------------
      bool Account::stepSocket()
      {
        if (mSocket) {
          IRUDPICESocket::RUDPICESocketStates socketState = mSocket->getState();

          if (IRUDPICESocket::RUDPICESocketState_Shutdown == socketState) {
            ZS_LOG_ERROR(Debug, log("notified RUDP ICE socket is shutdown unexpected"))
            setError(IHTTP::HTTPStatusCode_Networkconnecttimeouterror, "RUDP ICE Socket Session shutdown unexpectedly");
            cancel();
            return false;
          }

          if ((IRUDPICESocket::RUDPICESocketState_Ready != socketState) &&
              (IRUDPICESocket::RUDPICESocketState_Sleeping != socketState)) {
            ZS_LOG_TRACE(log("waiting for the socket to wake up or to go to sleep"))
            return true;
          }

          ZS_LOG_TRACE(log("sockets are ready"))
          return true;
        }

        String turnServer;
        String turnServerUsername;
        String turnServerPassword;
        String stunServer;
        getNATServers(turnServer, turnServerUsername, turnServerPassword, stunServer);

        ZS_LOG_DEBUG(log("creating RUDP ICE socket") + ", turn=" + turnServer + ", username=" + turnServerUsername + ", password=" + turnServerPassword + ", stun=" + stunServer)

        mSocket = IRUDPICESocket::create(
                                         IStackForInternal::queueServices(),
                                         mThisWeak.lock(),
                                         turnServer,
                                         turnServerUsername,
                                         turnServerPassword,
                                         stunServer
                                         );
        if (mSocket) {
          ZS_LOG_TRACE(log("socket created thus need to wait for socket to be ready"))
          return true;
        }

        ZS_LOG_ERROR(Detail, log("failed to create RUDP ICE socket thus shutting down"))
        setError(IHTTP::HTTPStatusCode_InternalServerError, "Failed to create RUDP ICE Socket");
        cancel();
        return false;
      }

      //-----------------------------------------------------------------------
      bool Account::stepFinder()
      {
        if (mFindersGetMonitor) {
          ZS_LOG_TRACE(log("waiting for finders get monitor to complete"))
          return false;
        }

        if (mFinderDNSLookup) {
          ZS_LOG_TRACE(log("waiting for finder DNS lookup to complete"))
          return false;
        }

        if (mFinder) {
          if (IAccount::AccountState_Ready != mFinder->forAccount().getState()) {
            ZS_LOG_TRACE(log("waiting for the finder to connect"))
            return false;
          }
          ZS_LOG_TRACE(log("finder already created"))
          return true;
        }

        Time tick = zsLib::now();

        if (mFinderRetryAfter > tick) {
          ZS_LOG_TRACE(log("waiting a bit before retrying finder connection..."))
          return false;
        }

        if (mAvailableFinders.size() < 1) {
          ZS_THROW_BAD_STATE_IF(!mLockboxSession)

          BootstrappedNetworkPtr network = mLockboxSession->forAccount().getBootstrappedNetwork();

          ZS_THROW_BAD_STATE_IF(!network)

          FindersGetRequestPtr request = FindersGetRequest::create();
          request->domain(getDomain());
          request->totalFinders(OPENPEER_STACK_FINDERS_GET_TOTAL_SERVERS_TO_GET);

          mFindersGetMonitor = IMessageMonitor::monitorAndSendToService(mThisWeak.lock(), network, "bootstrapped-finders", "finders-get", request, Seconds(OPENPEER_STACK_FINDERS_GET_TIMEOUT_IN_SECONDS));

          ZS_LOG_TRACE(log("attempting to get finders"))
          return false;
        }

        if (!mAvailableFinderSRVResult) {
          Finder &finder = mAvailableFinders.front();
          String srv = getMultiplexedJsonTCPTransport(finder);
          if (srv.isEmpty()) {
            ZS_LOG_ERROR(Detail, log("finder missing SRV name"))
            mAvailableFinders.pop_front();
            if (mAvailableFinders.size() < 1) {
              handleFinderRelatedFailure();
            }
            IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
            return false;
          }

          mFinderDNSLookup = IDNS::lookupSRV(mThisWeak.lock(), srv, "_finder", "_tcp");
          ZS_LOG_TRACE(log("performing DNS lookup on finder"))
          return false;
        }

        ZS_LOG_TRACE(log("creating finder instance"))
        mFinder = IAccountFinderForAccount::create(mThisWeak.lock(), mThisWeak.lock());

        if (mFinder) {
          ZS_LOG_TRACE(log("waiting for finder to be ready"))
          return false;
        }

        ZS_LOG_ERROR(Detail, log("step failed to create finder thus shutting down"))
        setError(IHTTP::HTTPStatusCode_InternalServerError, "Failed to create Account Finder");
        cancel();
        return false;
      }

      //-----------------------------------------------------------------------
      bool Account::stepPeers()
      {
        if (mPeerInfos.size() < 1) {
          ZS_LOG_TRACE(log("step peers complete as no peers exist"))
          return true;
        }

        for (PeerInfoMap::iterator peerIter = mPeerInfos.begin(); peerIter != mPeerInfos.end(); )
        {
          PeerInfoMap::iterator current = peerIter;
          ++peerIter;

          const String &peerURI = (*current).first;
          PeerInfoPtr &peerInfo = (*current).second;

          if (shouldShutdownInactiveLocations(peerURI, peerInfo)) {

            shutdownPeerLocationsNotNeeded(peerURI, peerInfo);

            if (peerInfo->mLocations.size() > 0) {
              ZS_LOG_DEBUG(log("some location are still connected thus do not shutdown the peer yet") + PeerInfo::toDebugString(peerInfo))
              continue;
            }

            // erase the peer now...
            ZS_LOG_DEBUG(log("no locations at this peer thus shutting down now") + PeerInfo::toDebugString(peerInfo))
            mPeerInfos.erase(current);
            continue;
          }

          sendPeerKeepAlives(peerURI, peerInfo);

          performPeerFind(peerURI, peerInfo);
        }

        ZS_LOG_TRACE(log("step peers complete"))
        return true;
      }

      //-----------------------------------------------------------------------
      void Account::setState(IAccount::AccountStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state) + getDebugValueString())

        mCurrentState = state;

        if (!mDelegate) return;

        AccountPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onAccountStateChanged(pThis, mCurrentState);
          } catch(IAccountDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        notifySubscriptions(mSelfLocation, toLocationConnectionState(mCurrentState));
      }

      //-----------------------------------------------------------------------
      void Account::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());

        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        mLastError = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_ERROR(Basic, log("account error") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      CandidatePtr Account::getRelayCandidate(const String &peerURI) const
      {
        if (!mFinder) return CandidatePtr();

        IPAddress finderIP;
        String relayAccessToken;
        String relayAccessSecret;

        mFinder->forAccount().getFinderRelayInformation(finderIP, relayAccessToken, relayAccessSecret);
        String localContext = getLocalContextID(peerURI);

        if ((relayAccessToken.isEmpty()) ||
            (relayAccessSecret.isEmpty())) return CandidatePtr();

        CandidatePtr candidate(new Candidate);

        candidate->mNamespace = OPENPEER_STACK_CANDIDATE_NAMESPACE_FINDER_RELAY;
        candidate->mTransport = OPENPEER_STACK_TRANSPORT_MULTIPLEXED_JSON_MLS_TCP;
        candidate->mAccessToken = relayAccessToken;
        candidate->mIPAddress = finderIP;

        // hex(hmac(<relay-access-secret>, "finder-relay-access-validate:" + <relay-access-token> + ":" + <local-context> + ":channel-map")
        String hashString ="finder-relay-access-validate:" + relayAccessToken + ":" + localContext + ":channel-map";
        String proofHash = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(relayAccessSecret), hashString));

        ZS_LOG_TRACE(log("get relay candidate hash calculation") + ", relay acceess secret=" + relayAccessSecret + ", hmac input=" + hashString + ", relay access secret proof=" + proofHash)


        candidate->mAccessSecretProof = proofHash;
        return candidate;
      }

      //-----------------------------------------------------------------------
      void Account::setFindState(
                                 PeerInfo &peerInfo,
                                 IPeer::PeerFindStates state
                                 )
      {
        if (peerInfo.mCurrentFindState == state) return;

        ZS_LOG_DEBUG(log("find state changed") + ", old state=" + IPeer::toString(peerInfo.mCurrentFindState) + ", new state=" + IPeer::toString(state) + peerInfo.getDebugValueString())

        peerInfo.mCurrentFindState = state;

        notifySubscriptions(peerInfo.mPeer, state);
      }

      //-----------------------------------------------------------------------
      bool Account::shouldFind(
                               const String &peerURI,
                               const PeerInfoPtr &peerInfo
                               ) const
      {
        typedef zsLib::Time Time;

        Time tick = zsLib::now();

        if (peerInfo->mPeerFindMonitor) {
          ZS_LOG_DEBUG(log("peer has peer find in progress thus no need to conduct new search") + PeerInfo::toDebugString(peerInfo))
          return false;
        }

        if (peerInfo->mFindAtNextPossibleMoment) {
          ZS_LOG_DEBUG(log("told to refind at next possible moment"))
          return true;
        }

        if (peerInfo->mTotalSubscribers < 1) {
          ZS_LOG_DEBUG(log("no subscribers required so no need to subscribe to this location"))
          return false;
        }

        if (peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.size() > 0) {
          ZS_LOG_DEBUG(log("peer has hints of new locations thus search needs redoing") + PeerInfo::toDebugString(peerInfo))
          return true;
        }

        if (peerInfo->mLocations.size() > 0) {
          ZS_LOG_DEBUG(log("peer has locations and no hints to suggest new locations thus no need to conduct new search") + PeerInfo::toDebugString(peerInfo))
          return false;
        }

        // we have subscriptions but no locations, see if it is okay to find again "now"...
        if (tick < peerInfo->mNextScheduledFind) {
          ZS_LOG_DEBUG(log("not time yet to conduct a new search") + PeerInfo::toDebugString(peerInfo))
          return false;
        }

        if (!peerInfo->mPeer->forAccount().getPeerFilePublic()) {
          ZS_LOG_WARNING(Detail, log("cannot find a peer where the public peer file is not known"))
          return false;
        }

        ZS_LOG_DEBUG(log("peer search should be conducted") + PeerInfo::toDebugString(peerInfo))
        return true;
      }

      //-----------------------------------------------------------------------
      bool Account::shouldShutdownInactiveLocations(
                                                    const String &peerURI,
                                                    const PeerInfoPtr &peerInfo
                                                    ) const
      {
        if (peerInfo->mPeerFindMonitor) {
          ZS_LOG_DEBUG(log("peer has peer active find in progress thus its location should not be shutdown") + ", peer=" + string(peerInfo->mID) + ", peer URI=" + peerURI)
          return false;
        }

        if (peerInfo->mTotalSubscribers > 0) {
          ZS_LOG_DEBUG(log("peer has subscriptions thus no need to shutdown") + PeerInfo::toDebugString(peerInfo))
          return false;
        }

        if (peerInfo->mFindAtNextPossibleMoment) {
          ZS_LOG_DEBUG(log("do not shutdown inactive as there is a desire to find at next possible moment") + PeerInfo::toDebugString(peerInfo))
          return false;
        }

        ZS_LOG_DEBUG(log("should shutdown this peer's location that are non-active") + PeerInfo::toDebugString(peerInfo))
        return true;
      }

      //-----------------------------------------------------------------------
      void Account::shutdownPeerLocationsNotNeeded(
                                                   const String &peerURI,
                                                   PeerInfoPtr &peerInfo
                                                   )
      {
        Time tick = zsLib::now();

        if (mBlockLocationShutdownsUntil > tick) {
          // prevent shutdowns immediately after backgrounding (to give time to see which will self-cancel due to timeout)
          ZS_LOG_DEBUG(log("not allowing peer locations to shutdown"))
          return;
        }

        // scope: the peer is not incoming and all subscriptions are gone therefor it is safe to shutdown the peer locations entirely
        if (peerInfo->mLocations.size() > 0) {
          ZS_LOG_DEBUG(log("checking to see which locations for this peer should be shutdown due to inactivity") + PeerInfo::toDebugString(peerInfo))

          for (PeerInfo::PeerLocationMap::iterator locationIter = peerInfo->mLocations.begin(); locationIter != peerInfo->mLocations.end(); ) {
            PeerInfo::PeerLocationMap::iterator locationCurrentIter = locationIter;
            ++locationIter;

            // const String &locationID = (*locationCurrentIter).first;
            AccountPeerLocationPtr &peerLocation = (*locationCurrentIter).second;

            Time lastActivityTime = peerLocation->forAccount().getTimeOfLastActivity();

            if (lastActivityTime + Seconds(OPENPEER_STACK_PEER_LOCATION_INACTIVITY_TIMEOUT_IN_SECONDS) > tick) {
              ZS_LOG_DEBUG(log("peer location is still considered active at this time (thus keeping connection alive)") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
              continue;
            }

            ZS_LOG_DEBUG(log("shutting down non incoming peer location that does not have a subscription") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))

            // signal the shutdown now...
            peerLocation->forAccount().shutdown();
          }
        }
      }

      //-----------------------------------------------------------------------
      void Account::sendPeerKeepAlives(
                                       const String &peerURI,
                                       PeerInfoPtr &peerInfo
                                       )
      {
        if (peerInfo->mLocations.size() < 1) return;

        Time tick = zsLib::now();

        ZS_LOG_DEBUG(log("checking to see which locations should fire a keep alive timer") + PeerInfo::toDebugString(peerInfo))

        for (PeerInfo::PeerLocationMap::iterator locationIter = peerInfo->mLocations.begin(); locationIter != peerInfo->mLocations.end(); ) {
          PeerInfo::PeerLocationMap::iterator locationCurrentIter = locationIter;
          ++locationIter;

          // const String &locationID = (*locationCurrentIter).first;
          AccountPeerLocationPtr &peerLocation = (*locationCurrentIter).second;

          Time lastActivityTime = peerLocation->forAccount().getTimeOfLastActivity();

          if (lastActivityTime + Seconds(OPENPEER_STACK_PEER_LOCATION_KEEP_ALIVE_TIME_IN_SECONDS) > tick) {
            ZS_LOG_TRACE(log("peer location is not requiring a keep alive yet") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
            continue;
          }

          ZS_LOG_DEBUG(log("peer location is still needed thus sending keep alive now (if possible)...") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
          peerLocation->forAccount().sendKeepAlive();
        }
      }

      //-----------------------------------------------------------------------
      void Account::performPeerFind(
                                    const String &peerURI,
                                    PeerInfoPtr &peerInfo
                                    )
      {
        IPeerFilesPtr peerFiles = mLockboxSession->forAccount().getPeerFiles();
        ZS_THROW_BAD_STATE_IF(!peerFiles)

        if (!shouldFind(peerURI, peerInfo)) {
          ZS_LOG_DEBUG(log("peer find should not be conducted at this time") + PeerInfo::toDebugString(peerInfo))
          return;
        }

        mSocket->wakeup();
        if (IRUDPICESocket::RUDPICESocketState_Ready != mSocket->getState()) {
          ZS_LOG_DEBUG(log("should issue find request but must wait until ICE candidates are fully ready") + PeerInfo::toDebugString(peerInfo))
          return;
        }

        ZS_LOG_DEBUG(log("peer is conducting a peer find search for locations") + PeerInfo::toDebugString(peerInfo))

        // remember which hints caused this search to happen
        peerInfo->mPeerFindBecauseOfLocations = peerInfo->mPeerFindNeedsRedoingBecauseOfLocations;
        peerInfo->mPeerFindNeedsRedoingBecauseOfLocations.clear();  // we no longer need to redo because of these hints since we are now doing the search

        PeerLocationFindRequestPtr request = PeerLocationFindRequest::create();
        request->domain(getDomain());

        PeerLocationFindRequest::ExcludedLocationList exclude;
        for (PeerInfo::PeerLocationMap::iterator iter = peerInfo->mLocations.begin(); iter != peerInfo->mLocations.end(); ++iter) {

          AccountPeerLocationPtr peerLocation = (*iter).second;

          if (peerLocation->forAccount().hasReceivedCandidateInformation())
            // do not conduct a search for locations that already are connected or in the process of connecting...
            ZS_LOG_DEBUG(log("peer find will exclude location in search since location is already connecting or connected") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
            exclude.push_back((*iter).first);
        }

        LocationInfoPtr locationInfo = getLocationInfo(mSelfLocation);

        CandidatePtr relayCandidate = getRelayCandidate(peerURI);
        if (relayCandidate) {
          locationInfo->mCandidates.push_front(*relayCandidate);
        }

        request->findPeer(peerInfo->mPeer);
        request->context(getLocalContextID(peerURI));
        request->peerSecret(getLocalPassword(peerURI));
        request->iceUsernameFrag(mSocket->getUsernameFrag());
        request->icePassword(mSocket->getPassword());
        request->excludeLocations(exclude);
        request->locationInfo(*locationInfo);
        request->peerFiles(peerFiles);

        peerInfo->mPeerFindMonitor = mFinder->forAccount().sendRequest(mThisWeak.lock(), request, Seconds(OPENPEER_STACK_PEER_LOCATION_FIND_TIMEOUT_IN_SECONDS));

        setFindState(*(peerInfo.get()), IPeer::PeerFindState_Finding);
      }

      //-----------------------------------------------------------------------
      void Account::handleFindRequestComplete(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());

        for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter) {
          PeerInfoPtr peerInfo = (*iter).second;

          if (monitor != peerInfo->mPeerFindMonitor) continue;

          ZS_LOG_DEBUG(log("find request is now complete") + PeerInfo::toDebugString(peerInfo))

          // the search is now complete, stop looking for new searches
          peerInfo->mPeerFindMonitor.reset();
          peerInfo->mPeerFindBecauseOfLocations.clear();  // all hints are effectively destroyed now since we've completed the search

          setFindState(*(peerInfo.get()), IPeer::PeerFindState_Completed);
          setFindState(*(peerInfo.get()), IPeer::PeerFindState_Idle);

          bool foundValid = false;

          // scope: check to see which of these has not received candidate information and close the location since we could not contact to this location
          {
            for (PeerInfo::PeerLocationMap::iterator iterLocation = peerInfo->mLocations.begin(); iterLocation != peerInfo->mLocations.end(); ++iterLocation) {
              AccountPeerLocationPtr &peerLocation = (*iterLocation).second;
              if (!peerLocation->forAccount().hasReceivedCandidateInformation()) {
                ZS_LOG_DEBUG(log("shutting down peer location as did not receive ICE candidates") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
                peerLocation->forAccount().shutdown();
              } else {
                ZS_LOG_DEBUG(log("find location is valid") + PeerInfo::toDebugString(peerInfo) + AccountPeerLocation::toDebugString(peerLocation))
                foundValid = true;
                peerInfo->findTimeReset();
              }
            }
          }

          if (!foundValid) {
            peerInfo->findTimeScheduleNext();
          }
        }
      }

      //-----------------------------------------------------------------------
      void Account::handleFinderRelatedFailure()
      {
        ZS_LOG_TRACE(log("handling finder related failure"))

        Time tick = zsLib::now();

        mFinderRetryAfter = tick + mLastRetryFinderAfterDuration;
        mLastRetryFinderAfterDuration = mLastRetryFinderAfterDuration * 2;

        if (mLastRetryFinderAfterDuration > Seconds(OPENPEER_STACK_ACCOUNT_FINDER_MAX_RETRY_AFTER_TIME_IN_SECONDS)) {
          mLastRetryFinderAfterDuration = Seconds(OPENPEER_STACK_ACCOUNT_FINDER_MAX_RETRY_AFTER_TIME_IN_SECONDS);
        }

        // HERE - all peer locations should be told the finder relay has been closed

        for (PeerInfoMap::iterator iter = mPeerInfos.begin(); iter != mPeerInfos.end(); ++iter)
        {
          PeerInfoPtr &peerInfo = (*iter).second;

          for (RelayInfoMap::iterator relayIter = peerInfo->mRelayInfos.begin(); relayIter != peerInfo->mRelayInfos.end(); ++relayIter)
          {
            RelayInfoPtr &relayInfo = (*relayIter).second;

            ZS_LOG_DEBUG(log("shutting down peer's relay") + relayInfo->getDebugValueString())

            relayInfo->cancel();
          }

          peerInfo->mRelayInfos.clear();
        }
      }

      //-----------------------------------------------------------------------
      void Account::notifySubscriptions(
                                        LocationPtr location,
                                        ILocation::LocationConnectionStates state
                                        )
      {
        for (PeerSubscriptionMap::iterator iter = mPeerSubscriptions.begin(); iter != mPeerSubscriptions.end(); )
        {
          PeerSubscriptionMap::iterator current = iter;
          ++iter;

          PUID subscriptionID = (*current).first;
          PeerSubscriptionPtr subscription = (*current).second.lock();
          if (!subscription) {
            ZS_LOG_WARNING(Detail, log("peer subscription is gone") + ", subscription ID=" = string(subscriptionID))
            mPeerSubscriptions.erase(current);
            continue;
          }

          ZS_LOG_DEBUG(log("notifying subscription peer locations changed") + IPeerSubscription::toDebugString(subscription) + ", state=" + ILocation::toString(state) + ILocation::toDebugString(location))
          subscription->forAccount().notifyLocationConnectionStateChanged(location, state);
        }
      }

      //-----------------------------------------------------------------------
      void Account::notifySubscriptions(
                                        PeerPtr peer,
                                        IPeer::PeerFindStates state
                                        )
      {
        for (PeerSubscriptionMap::iterator iter = mPeerSubscriptions.begin(); iter != mPeerSubscriptions.end(); )
        {
          PeerSubscriptionMap::iterator current = iter;
          ++iter;

          PUID subscriptionID = (*current).first;
          PeerSubscriptionPtr subscription = (*current).second.lock();
          if (!subscription) {
            ZS_LOG_WARNING(Detail, log("peer subscription is gone") + ", subscription ID=" = string(subscriptionID))
            mPeerSubscriptions.erase(current);
            continue;
          }

          ZS_LOG_DEBUG(log("notifying subscription peer find state changed") + IPeerSubscription::toDebugString(subscription) + ", state=" + IPeer::toString(state) + IPeer::toDebugString(peer))
          subscription->forAccount().notifyFindStateChanged(peer, state);
        }
      }

      //-----------------------------------------------------------------------
      void Account::notifySubscriptions(MessageIncomingPtr messageIncoming)
      {
        for (PeerSubscriptionMap::iterator iter = mPeerSubscriptions.begin(); iter != mPeerSubscriptions.end(); )
        {
          PeerSubscriptionMap::iterator current = iter;
          ++iter;

          PUID subscriptionID = (*current).first;
          PeerSubscriptionPtr subscription = (*current).second.lock();
          if (!subscription) {
            ZS_LOG_WARNING(Detail, log("peer subscription is gone") + ", subscription ID=" = string(subscriptionID))
            mPeerSubscriptions.erase(current);
            continue;
          }

          ZS_LOG_DEBUG(log("notifying subscription of incoming message") + IPeerSubscription::toDebugString(subscription) + IMessageIncoming::toDebugString(messageIncoming))
          subscription->forAccount().notifyMessageIncoming(messageIncoming);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::PeerInfo
      #pragma mark

      //-----------------------------------------------------------------------
      Account::RelayInfoPtr Account::RelayInfo::create()
      {
        RelayInfoPtr pThis(new RelayInfo);
        pThis->mChannel = 0;
        return pThis;
      }

      //-----------------------------------------------------------------------
      Account::RelayInfo::~RelayInfo()
      {
        cancel();
      }

      //-----------------------------------------------------------------------
      String Account::RelayInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return
        Helper::getDebugValue("relay info id", string(mID), firstTime) +
        Helper::getDebugValue("channel", 0 != mChannel ? string(mChannel) : String(), firstTime) +
        Helper::getDebugValue("local context", mLocalContext, firstTime) +
        Helper::getDebugValue("remote context", mRemoteContext, firstTime) +
        Helper::getDebugValue("relay channel id", mRelayChannel ? string(mRelayChannel->getID()) : String(), firstTime) +
        Helper::getDebugValue("send stream id", mSendStream ? string(mSendStream->getID()) : String(), firstTime) +
        Helper::getDebugValue("receive stream id", mReceiveStream ? string(mReceiveStream->getID()) : String(), firstTime) +
        Helper::getDebugValue("peer location", mAccountPeerLocation ? string(mAccountPeerLocation->forAccount().getID()) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void Account::RelayInfo::cancel()
      {
        if (mRelayChannelSubscription) {
          mRelayChannelSubscription->cancel();
          mRelayChannelSubscription.reset();
        }
        if (mReceiveStreamSubscription) {
          mReceiveStreamSubscription->cancel();
          mReceiveStreamSubscription.reset();
        }
        if (mSendStreamSubscription) {
          mSendStreamSubscription->cancel();
          mSendStreamSubscription.reset();
        }
        if (mRelayChannel) {
          mRelayChannel->cancel();
          mRelayChannel.reset();
        }
        if (mReceiveStream) {
          mReceiveStream->cancel();
          mReceiveStream.reset();
        }
        if (mSendStream) {
          mSendStream->cancel();
          mSendStream.reset();
        }

        mAccountPeerLocation.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::PeerInfo
      #pragma mark

      //-----------------------------------------------------------------------
      String Account::PeerInfo::toDebugString(PeerInfoPtr peerInfo, bool includeCommaPrefix)
      {
        if (!peerInfo) return includeCommaPrefix ? String(", peer info=(null)") : String("peer info=(null");
        return peerInfo->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      Account::PeerInfoPtr Account::PeerInfo::create()
      {
        PeerInfoPtr pThis(new PeerInfo);
        pThis->mFindAtNextPossibleMoment = false;
        pThis->findTimeReset();
        pThis->mCurrentFindState = IPeer::PeerFindState_Idle;
        pThis->mTotalSubscribers = 0;
        pThis->mPreventCrazyRefindNextTime = false;
        return pThis;
      }

      //-----------------------------------------------------------------------
      void Account::PeerInfo::findTimeReset()
      {
        mNextScheduledFind = zsLib::now();
        mLastScheduleFindDuration = Seconds(OPENPEER_STACK_PEER_LOCATION_FIND_RETRY_IN_SECONDS/2);
        if (mLastScheduleFindDuration < Seconds(1))
          mLastScheduleFindDuration = Seconds(1);
      }

      //-----------------------------------------------------------------------
      void Account::PeerInfo::findTimeScheduleNext()
      {
        mLastScheduleFindDuration = mLastScheduleFindDuration * 2;
        mNextScheduledFind = zsLib::now() + mLastScheduleFindDuration;
      }

      //-----------------------------------------------------------------------
      String Account::PeerInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return
        Helper::getDebugValue("peer info id", string(mID), firstTime) +
        Helper::getDebugValue("find next moment", mFindAtNextPossibleMoment ? String("true") : String(), firstTime) +
        IPeer::toDebugString(mPeer) +
        Helper::getDebugValue("locations", mLocations.size() > 0 ? string(mLocations.size()) : String(), firstTime) +
        Helper::getDebugValue("find monitor", mPeerFindMonitor ? String("true") : String(), firstTime) +
        Helper::getDebugValue("find because", mPeerFindBecauseOfLocations.size() > 0 ? string(mPeerFindBecauseOfLocations.size()) : String(), firstTime) +
        Helper::getDebugValue("find redo because", mPeerFindNeedsRedoingBecauseOfLocations.size() > 0 ? string(mPeerFindNeedsRedoingBecauseOfLocations.size()) : String(), firstTime) +
        Helper::getDebugValue("find state", IPeer::toString(mCurrentFindState), firstTime) +
        Helper::getDebugValue("subscribers", 0 != mTotalSubscribers ? string(mTotalSubscribers) : String(), firstTime) +
        Helper::getDebugValue("next find", Time() != mNextScheduledFind ? IHelper::timeToString(mNextScheduledFind) : String(), firstTime) +
        Helper::getDebugValue("last duration", 0 != mLastScheduleFindDuration.total_milliseconds() ? string(mLastScheduleFindDuration.total_milliseconds()) : String(), firstTime) +
        Helper::getDebugValue("prevent crazy refind", mPreventCrazyRefindNextTime ? String("true") : String(), firstTime);
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
    String IAccount::toDebugString(IAccountPtr account, bool includeCommaPrefix)
    {
      return internal::Account::toDebugString(account, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IAccountPtr IAccount::create(
                                 IAccountDelegatePtr delegate,
                                 IServiceLockboxSessionPtr peerContactSession
                                 )
    {
      return internal::IAccountFactory::singleton().create(delegate, peerContactSession);
    }
  }
}
