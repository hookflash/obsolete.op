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

#pragma once

#include <hookflash/stack/internal/types.h>
#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/IPeer.h>
#include <hookflash/stack/IPeerSubscription.h>
#include <hookflash/stack/internal/stack_AccountFinder.h>
#include <hookflash/stack/internal/stack_AccountPeerLocation.h>
#include <hookflash/stack/internal/stack_ServicePeerContactSession.h>
#include <hookflash/stack/IMessageMonitor.h>
#include <hookflash/services/IRUDPICESocket.h>

#include <zsLib/MessageQueueAssociator.h>

#include <zsLib/Timer.h>

#include <map>

namespace hookflash
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
      #pragma mark IAccountForAccountFinder
      #pragma mark

      interaction IAccountForAccountFinder
      {
        IAccountForAccountFinder &forAccountFinder() {return *this;}
        const IAccountForAccountFinder &forAccountFinder() const {return *this;}

        virtual RecursiveLock &getLock() const = 0;

        virtual String getDomain() const = 0;

        virtual IRUDPICESocketPtr getSocket() const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;

        virtual bool extractNextFinder(
                                       Finder &outFinder,
                                       IPAddress &outFinderIP
                                       ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForAccountPeerLocation
      #pragma mark

      interaction IAccountForAccountPeerLocation
      {
        IAccountForAccountPeerLocation &forAccountPeerLocation() {return *this;}
        const IAccountForAccountPeerLocation &forAccountPeerLocation() const {return *this;}

        virtual RecursiveLock &getLock() const = 0;

        virtual String getDomain() const = 0;

        virtual IRUDPICESocketPtr getSocket() const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;

        virtual bool isFinderReady() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForLocation
      #pragma mark

      interaction IAccountForLocation
      {
        IAccountForLocation &forLocation() {return *this;}
        const IAccountForLocation &forLocation() const {return *this;}

        virtual LocationPtr findExistingOrUse(LocationPtr location) = 0;
        virtual LocationPtr getLocationForLocal() const = 0;
        virtual LocationPtr getLocationForFinder() const = 0;
        virtual void notifyDestroyed(Location &location) = 0;

        virtual const String &getLocationID() const = 0;
        virtual PeerPtr getPeerForLocal() const = 0;

        virtual LocationInfoPtr getLocationInfo(LocationPtr location) const = 0;

        virtual ILocation::LocationConnectionStates getConnectionState(LocationPtr location) const = 0;

        virtual bool send(
                          LocationPtr location,
                          MessagePtr message
                          ) const = 0;

        virtual void hintNowAvailable(LocationPtr location) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForMessageIncoming
      #pragma mark

      interaction IAccountForMessageIncoming
      {
        IAccountForMessageIncoming &forMessageIncoming() {return *this;}
        const IAccountForMessageIncoming &forMessageIncoming() const {return *this;}

        virtual bool send(
                          LocationPtr location,
                          MessagePtr response
                          ) const = 0;
        virtual void notifyMessageIncomingResponseNotSent(MessageIncoming &messageIncoming) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForMessages
      #pragma mark

      interaction IAccountForMessages
      {
        IAccountForMessages &forMessages() {return *this;}
        const IAccountForMessages &forMessages() const {return *this;}

        virtual IPeerFilesPtr getPeerFiles() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForPeer
      #pragma mark

      interaction IAccountForPeer
      {
        IAccountForPeer &forPeer() {return *this;}
        const IAccountForPeer &forPeer() const {return *this;}

        virtual PeerPtr findExistingOrUse(PeerPtr peer) = 0;
        virtual void notifyDestroyed(Peer &peer) = 0;

        virtual RecursiveLock &getLock() const = 0;

        virtual IPeer::PeerFindStates getPeerState(const String &peerURI) const = 0;
        virtual LocationListPtr getPeerLocations(
                                                 const String &peerURI,
                                                 bool includeOnlyConnectedLocations
                                                 ) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForPeerSubscription
      #pragma mark

      interaction IAccountForPeerSubscription
      {
        IAccountForPeerSubscription &forPeerSubscription() {return *this;}
        const IAccountForPeerSubscription &forPeerSubscription() const {return *this;}

        virtual void subscribe(PeerSubscriptionPtr subscription) = 0;
        virtual void notifyDestroyed(PeerSubscription &subscription) = 0;

        virtual RecursiveLock &getLock() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForPublicationRepository
      #pragma mark

      interaction IAccountForPublicationRepository
      {
        IAccountForPublicationRepository &forRepo() {return *this;}
        const IAccountForPublicationRepository &forRepo() const {return *this;}

        virtual PublicationRepositoryPtr getRepository() const = 0;

        virtual String getDomain() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForServicePeerContactSession
      #pragma mark

      interaction IAccountForServicePeerContactSession
      {
        IAccountForServicePeerContactSession &forServicePeerContactSession() {return *this;}
        const IAccountForServicePeerContactSession &forServicePeerContactSession() const {return *this;}

        virtual void notifyServicePeerContactSessionStateChanged() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountAsyncDelegate
      #pragma mark

      interaction IAccountAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account
      #pragma mark

      class Account : public Noop,
                      public MessageQueueAssociator,
                      public IAccount,
                      public IAccountForAccountFinder,
                      public IAccountForAccountPeerLocation,
                      public IAccountForLocation,
                      public IAccountForMessageIncoming,
                      public IAccountForMessages,
                      public IAccountForPeer,
                      public IAccountForPeerSubscription,
                      public IAccountForPublicationRepository,
                      public IAccountForServicePeerContactSession,
                      public IAccountAsyncDelegate,
                      public IAccountFinderDelegate,
                      public IAccountPeerLocationDelegate,
                      public IDNSDelegate,
                      public IRUDPICESocketDelegate,
                      public IMessageMonitorDelegate,
                      public ITimerDelegate
      {
      public:
        friend interaction IAccountFactory;
        friend interaction IAccount;

        typedef IAccount::AccountStates AccountStates;

        struct PeerInfo;
        friend struct PeerInfo;
        typedef boost::shared_ptr<PeerInfo> PeerInfoPtr;
        typedef boost::weak_ptr<PeerInfo> PeerInfoWeakPtr;

        typedef String PeerURI;
        typedef String LocationID;
        typedef PUID PeerSubscriptionID;
        typedef std::pair<PeerURI, LocationID> PeerLocationIDPair;

        typedef std::map<PeerURI, PeerWeakPtr> PeerMap;
        typedef std::map<PeerURI, PeerInfoPtr> PeerInfoMap;

        typedef std::map<PeerSubscriptionID, PeerSubscriptionWeakPtr> PeerSubscriptionMap;

        typedef std::map<PeerLocationIDPair, LocationWeakPtr> LocationMap;

      protected:
        Account(
                IMessageQueuePtr queue,
                IAccountDelegatePtr delegate,
                ServicePeerContactSessionPtr peerContactSession
                );
        
        Account(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~Account();

        static AccountPtr convert(IAccountPtr account);// {return AccountPtr();}

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccount
        #pragma mark

        static String toDebugString(IAccountPtr account, bool includeCommaPrefix = true);

        static AccountPtr create(
                                 IAccountDelegatePtr delegate,
                                 IServicePeerContactSessionPtr peerContactSession
                                 );

        virtual PUID getID() const {return mID;}

        virtual AccountStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const;

        virtual IServicePeerContactSessionPtr getPeerContactSession() const;

        virtual void getNATServers(
                                   String &outTURNServer,
                                   String &outTURNUsername,
                                   String &outTURNPassword,
                                   String &outSTUNServer
                                   ) const;

        virtual void shutdown();


        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForAccountFinder
        #pragma mark

        // (duplicate) virtual RecursiveLock &getLock() const;

        virtual String getDomain() const;

        virtual IRUDPICESocketPtr getSocket() const;

        // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;

        virtual bool extractNextFinder(
                                       Finder &outFinder,
                                       IPAddress &outFinderIP
                                       );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForAccountPeerLocation
        #pragma mark

        // (duplicate) virtual RecursiveLock &getLock() const;

        // (duplicate) virtual String getDomain() const;

        // (duplicate) virtual IRUDPICESocketPtr getSocket() const;

        // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;

        virtual bool isFinderReady() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForLocation
        #pragma mark

        virtual LocationPtr findExistingOrUse(LocationPtr location);
        virtual LocationPtr getLocationForLocal() const;
        virtual LocationPtr getLocationForFinder() const;
        virtual void notifyDestroyed(Location &location);

        virtual const String &getLocationID() const;
        virtual PeerPtr getPeerForLocal() const;

        virtual LocationInfoPtr getLocationInfo(LocationPtr location) const;

        virtual ILocation::LocationConnectionStates getConnectionState(LocationPtr location) const;

        virtual bool send(
                          LocationPtr location,
                          MessagePtr message
                          ) const;

        virtual void hintNowAvailable(LocationPtr location);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForMessageIncoming
        #pragma mark

        // (duplicate) virtual bool send(
        //                              LocationPtr location,
        //                              MessagePtr response
        //                              ) const;
        virtual void notifyMessageIncomingResponseNotSent(MessageIncoming &messageIncoming);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForMessages
        #pragma mark

        virtual IPeerFilesPtr getPeerFiles() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForPeer
        #pragma mark

        virtual PeerPtr findExistingOrUse(PeerPtr peer);
        virtual void notifyDestroyed(Peer &peer);

        virtual RecursiveLock &getLock() const;

        virtual IPeer::PeerFindStates getPeerState(const String &peerURI) const;
        virtual LocationListPtr getPeerLocations(
                                                 const String &peerURI,
                                                 bool includeOnlyConnectedLocations
                                                 ) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForPeerSubscription
        #pragma mark

        virtual void subscribe(PeerSubscriptionPtr subscription);
        virtual void notifyDestroyed(PeerSubscription &subscription);

        // (duplicate) virtual RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForPublicationRepository
        #pragma mark

        virtual PublicationRepositoryPtr getRepository() const;

        // (duplicate) virtual String getDomain() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForServicePeerContactSession
        #pragma mark

        virtual void notifyServicePeerContactSessionStateChanged();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountFinderDelegate
        #pragma mark

        virtual void onAccountFinderStateChanged(
                                                 AccountFinderPtr finder,
                                                 AccountStates state
                                                 );

        virtual void onAccountFinderMessageIncoming(
                                                    AccountFinderPtr peerLocation,
                                                    MessagePtr message
                                                    );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountPeerLocationDelegate
        #pragma mark

        virtual void onAccountPeerLocationStateChanged(
                                                       AccountPeerLocationPtr peerLocation,
                                                       AccountStates state
                                                       );

        virtual void onAccountPeerLocationMessageIncoming(
                                                          AccountPeerLocationPtr peerLocation,
                                                          MessagePtr message
                                                          );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IDNSDelegate
        #pragma mark

        virtual void onLookupCompleted(IDNSQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IRUDPICESocketDelegate
        #pragma mark

        virtual void onRUDPICESocketStateChanged(
                                                 IRUDPICESocketPtr socket,
                                                 RUDPICESocketStates state
                                                 );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IMessageMonitorDelegate
        #pragma mark

        virtual bool handleMessageMonitorMessageReceived(
                                                         IMessageMonitorPtr requester,
                                                         MessagePtr message
                                                         );

        virtual void onMessageMonitorTimedOut(IMessageMonitorPtr requester);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => (internal)
        #pragma mark

        bool isPending() const      {return AccountState_Pending == mCurrentState;}
        bool isReady() const        {return AccountState_Ready == mCurrentState;}
        bool isShuttingDown() const {return AccountState_ShuttingDown ==  mCurrentState;}
        bool isShutdown() const     {return AccountState_Shutdown ==  mCurrentState;}

        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void cancel();
        void step();
        bool stepTimer();
        bool stepRepository();
        bool stepPeerContactSession();
        bool stepLocations();
        bool stepSocket();
        bool stepFinder();
        bool stepPeers();

        void setState(AccountStates accountState);
        void setError(WORD errorCode, const char *reason = NULL);

        void setFindState(
                          PeerInfo &peerInfo,
                          IPeer::PeerFindStates state
                          );

        bool shouldFind(
                        const String &peerURI,
                        const PeerInfoPtr &peerInfo
                        ) const;

        bool shouldShutdownInactiveLocations(
                                             const String &contactID,
                                             const PeerInfoPtr &peer
                                             ) const;

        void shutdownPeerLocationsNotNeeded(
                                            const String &peerURI,
                                            PeerInfoPtr &peerInfo
                                            );

        void sendPeerKeepAlives(
                                const String &peerURI,
                                PeerInfoPtr &peerInfo
                                );
        void performPeerFind(
                             const String &peerURI,
                             PeerInfoPtr &peerInfo
                             );

        void handleFindRequestComplete(IMessageMonitorPtr requester);

        void handleFinderRelatedFailure();

        void notifySubscriptions(
                                 LocationPtr location,
                                 ILocation::LocationConnectionStates state
                                 );

        void notifySubscriptions(
                                 PeerPtr peer,
                                 IPeer::PeerFindStates state
                                 );

        void notifySubscriptions(MessageIncomingPtr messageIncoming);

      public:

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::PeerInfo
        #pragma mark

        struct PeerInfo
        {
          typedef std::map<LocationID, AccountPeerLocationPtr> PeerLocationMap;     // every location needs a session
          typedef std::map<LocationID, LocationID> FindingBecauseOfLocationIDMap;   // using this to track the reason why the find needs to be initated or reinitated

          static String toDebugString(PeerInfoPtr peerInfo, bool includeCommaPrefix = true);

          static PeerInfoPtr create();
          void findTimeReset();
          void findTimeScheduleNext();
          String getDebugValueString(bool includeCommaPrefix = true) const;

          PUID mID;
          bool mFindAtNextPossibleMoment;

          PeerPtr mPeer;
          PeerLocationMap mLocations;                                 // list of connecting/connected peer locations

          IMessageMonitorPtr mPeerFindMonitor;                        // the request monitor when a search is being conducted
          FindingBecauseOfLocationIDMap mPeerFindBecauseOfLocations;  // peer find is being done because of locations that are known but not yet discovered

          FindingBecauseOfLocationIDMap mPeerFindNeedsRedoingBecauseOfLocations;  // peer find needs to be redone as soon as complete because of locations that are known but not yet discovered

          IPeer::PeerFindStates mCurrentFindState;
          ULONG mTotalSubscribers;                                    // total number of external subscribers to this peer

          // If a peer location was NOT found, we need to keep trying the search periodically but with exponential back off.
          // These variables keep track of that backoff. We don't need to do any finds once connecting/connected to a single location
          // because the peer location will notify us of other peer locations for the existing peer.
          // NOTE: Presence can also give us a hint to when we should redo the search.
          Time mNextScheduledFind;                                 // if peer was not found, schedule finds to try again
          Duration mLastScheduleFindDuration;                      // how long was the duration between finds (used because it will double each time a search is completed)

          bool mPreventCrazyRefindNextTime;
        };

      protected:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        AccountWeakPtr mThisWeak;
        AccountPtr mGracefulShutdownReference;

        AccountStates mCurrentState;
        WORD mLastError;
        String mLastErrorReason;

        IAccountDelegatePtr mDelegate;

        TimerPtr mTimer;
        Time mLastTimerFired;
        Time mBlockLocationShutdownsUntil;

        ServicePeerContactSessionPtr mPeerContactSession;
        Service::MethodPtr mTURN;
        Service::MethodPtr mSTUN;

        IRUDPICESocketPtr mSocket;

        String mLocationID;
        PeerPtr mSelfPeer;
        LocationPtr mSelfLocation;
        LocationPtr mFinderLocation;

        PublicationRepositoryPtr mRepository;

        AccountFinderPtr mFinder;

        Time mFinderRetryAfter;
        Duration mLastRetryFinderAfterDuration;

        FinderList mAvailableFinders;
        IDNS::SRVResultPtr mAvailableFinderSRVResult;
        IMessageMonitorPtr mFindersGetMonitor;
        IDNSQueryPtr mFinderDNSLookup;

        PeerInfoMap mPeerInfos;

        PeerSubscriptionMap mPeerSubscriptions;

        PeerMap mPeers;
        LocationMap mLocations;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFactory
      #pragma mark

      interaction IAccountFactory
      {
        static IAccountFactory &singleton();

        virtual AccountPtr create(
                                  IAccountDelegatePtr delegate,
                                  IServicePeerContactSessionPtr peerContactSession
                                  );
      };

    }
  }
}


ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
