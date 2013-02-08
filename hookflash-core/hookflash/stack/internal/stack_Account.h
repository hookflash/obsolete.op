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

#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/IPeerSubscription.h>
#include <hookflash/stack/IConnectionSubscription.h>
#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_AccountFinder.h>
#include <hookflash/stack/internal/stack_AccountPeerLocation.h>
#include <hookflash/stack/IMessageRequester.h>
#include <hookflash/services/IRUDPICESocket.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>
#include <zsLib/Proxy.h>
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
      #pragma mark IAccountForPeerSubscription
      #pragma mark

      interaction IAccountForPeerSubscription
      {
        typedef zsLib::String String;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef message::MessagePtr MessagePtr;

        virtual void onPeerSubscriptionShutdown(IPeerSubscriptionForAccountPtr peerSubscription) = 0;

        virtual IMessageQueuePtr getAssociatedMessageQueue() const = 0;

        virtual RecursiveLock &getLock() const = 0;

        virtual bool isFinding(const String &contactID) const = 0;

        virtual void getPeerLocations(
                                      const String &contactID,
                                      IPeerSubscription::LocationList &outLocations,
                                      bool includeOnlyConnectedLocation
                                      ) const = 0;

        virtual void getPeerLocations(
                                      const String &contactID,
                                      IPeerSubscription::PeerLocationList &outLocations,
                                      bool includeOnlyConnectedLocation
                                      ) const = 0;

        virtual bool sendPeerMessage(
                                     const String &contactID,
                                     const char *locationID,
                                     MessagePtr message
                                     ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForAccountFinder
      #pragma mark

      interaction IAccountForAccountFinder
      {
        typedef zsLib::Time Time;
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef message::MessagePtr MessagePtr;
        typedef services::IRUDPICESocketPtr IRUDPICESocketPtr;

        static void adjustToServerTime(Time time);

        virtual RecursiveLock &getLock() const = 0;
        virtual IRUDPICESocketPtr getSocket() const = 0;
        virtual IBootstrappedNetworkForAccountFinderPtr getBootstrapper() const = 0;
        virtual IPeerFilesPtr getPeerFiles() const = 0;
        virtual const String &getPassword() const = 0;

        virtual const String &getLocationID() const = 0;
        virtual const String &getDeviceID() const = 0;   // stable device ID that doesn't change between reboots, e.g. device uuid "7bff560b84328f161494eabcba5f8b47a316be8b"
        virtual const String &getUserAgent() const = 0;  // e.g. "hookflash/1.0.1001a (iOS/iPad)"
        virtual const String &getOS() const = 0;         // e.g. "iOS 5.0.3"
        virtual const String &getSystem() const = 0;     // e.g. "iPad 2"

        virtual void notifyAccountFinderIncomingMessage(
                                                        IAccountFinderPtr finder,
                                                        MessagePtr request
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
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef services::IRUDPICESocketPtr IRUDPICESocketPtr;
        typedef message::MessagePtr MessagePtr;

        virtual IMessageQueuePtr getAssociatedMessageQueue() const = 0;

        virtual RecursiveLock &getLock() const = 0;
        virtual IRUDPICESocketPtr getSocket() const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;
        virtual const String &getPassword() const = 0;

        virtual const String &getDeviceID() const = 0;  // stable device ID that doesn't change between reboots, e.g. device uuid "7bff560b84328f161494eabcba5f8b47a316be8b"
        virtual const String &getUserAgent() const = 0; // "hookflash/1.0.1001a (iOS/iPad)"
        virtual const String &getOS() const = 0;        // "iOS 5.0.3"
        virtual const String &getSystem() const = 0;    // "iPad 2"

        virtual bool isFinderReady() const = 0;
        virtual String getFinderLocationID() const = 0;

        virtual bool sendFinderMessage(MessagePtr message) = 0;

        virtual void onAccountPeerLocationIncomingMessage(
                                                          IAccountPeerLocationPtr peerLocation,
                                                          MessagePtr message
                                                          ) = 0;
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
        typedef zsLib::Duration Duration;
        typedef zsLib::String String;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef message::MessagePtr MessagePtr;

        virtual IAccountPtr convertIAccount() const = 0;
        virtual IMessageQueuePtr getAssociatedMessageQueue() const = 0;
        virtual String getContactID() const = 0;
        virtual const String &getLocationID() const = 0;

        virtual IMessageRequesterPtr sendFinderRequest(
                                                       IMessageRequesterDelegatePtr delegate,
                                                       MessagePtr requestMessage,
                                                       Duration timeout
                                                       ) = 0;

        virtual IMessageRequesterPtr sendPeerRequest(
                                                     IMessageRequesterDelegatePtr delegate,
                                                     MessagePtr requestMessage,
                                                     const char *contactID,
                                                     const char *locationID,
                                                     Duration timeout
                                                     ) = 0;
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

      class Account : public zsLib::MessageQueueAssociator,
                      public IAccount,
                      public IAccountAsyncDelegate,
                      public IAccountForPeerSubscription,
                      public IAccountFinderDelegate,
                      public IAccountForAccountFinder,
                      public IAccountForAccountPeerLocation,
                      public IAccountPeerLocationDelegate,
                      public IAccountForPublicationRepository,
                      public IBootstrappedNetworkDelegate,
                      public services::IRUDPICESocketDelegate,
                      public IMessageRequesterDelegate,
                      public zsLib::ITimerDelegate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::Time Time;
        typedef zsLib::TimerPtr TimerPtr;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef services::IRUDPICESocketPtr IRUDPICESocketPtr;
        typedef message::MessagePtr MessagePtr;

        class ConnectionSubscription;
        typedef boost::shared_ptr<ConnectionSubscription> ConnectionSubscriptionPtr;
        typedef boost::weak_ptr<ConnectionSubscription> ConnectionSubscriptionWeakPtr;

        struct Peer;
        typedef boost::shared_ptr<Peer> PeerPtr;
        typedef boost::weak_ptr<Peer> PeerWeakPtr;
        friend class Peer;

        class SubscriptionMessage;
        typedef boost::shared_ptr<SubscriptionMessage> SubscriptionMessagePtr;
        typedef boost::weak_ptr<SubscriptionMessage> SubscriptionMessageWeakPtr;
        friend class SubscriptionMessage;

        class SubscriptionNotificationHelper;
        typedef boost::shared_ptr<SubscriptionNotificationHelper> SubscriptionNotificationHelperPtr;
        typedef boost::weak_ptr<SubscriptionNotificationHelper> SubscriptionNotificationHelperWeakPtr;
        friend class SubscriptionNotificationHelper;

        friend interaction IAccount;

      protected:
        Account(IMessageQueuePtr queue);

        void init();

      public:
        ~Account();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccount
        #pragma mark

        static IAccountPtr create(
                                  IMessageQueuePtr queue,
                                  IBootstrappedNetworkPtr network,
                                  IAccountDelegatePtr delegate,
                                  IPeerFilesPtr peerFiles,
                                  const char *password,
                                  const char *deviceID,
                                  const char *userAgent,
                                  const char *os,
                                  const char *system
                                  );

        virtual AccountStates getState() const;
        virtual AccountErrors getLastError() const;

        virtual IPublicationRepositoryPtr getRepository() const;

        // (duplicate) virtual const String &getLocationID() const;

        virtual bool sendFinderMessage(MessagePtr message);

        virtual IConnectionSubscriptionPtr subscribeToAllConnections(IConnectionSubscriptionDelegatePtr delegate);

        virtual IPeerSubscriptionPtr subscribePeerLocations(
                                                            IPeerFilePublicPtr remotePartyPublicPeerFile,
                                                            IPeerSubscriptionDelegatePtr delegate
                                                            );

        virtual IPeerLocationPtr getPeerLocation(
                                                 const char *contactID,
                                                 const char *locationID
                                                 ) const;

        virtual void hintAboutNewPeerLocation(
                                              const char *contactID,
                                              const char *locationID
                                              );

        virtual void shutdown();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForAccountFinder
        #pragma mark

        // (duplicate) virtual RecursiveLock &getLock() const;
        virtual services::IRUDPICESocketPtr getSocket() const;
        virtual IBootstrappedNetworkForAccountFinderPtr getBootstrapper() const;
        virtual IPeerFilesPtr getPeerFiles() const;
        virtual const String &getPassword() const;

        virtual const String &getLocationID() const;
        virtual const String &getDeviceID() const;
        virtual const String &getUserAgent() const;
        virtual const String &getOS() const;
        virtual const String &getSystem() const;

        virtual void notifyAccountFinderIncomingMessage(
                                                        IAccountFinderPtr finder,
                                                        MessagePtr request
                                                        );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForAccountPeerLocation
        #pragma mark

        // (duplicate) virtual IMessageQueuePtr getAssociatedMessageQueue() const;

        // (duplicate) virtual RecursiveLock &getLock() const;
        // (duplicate) virtual IRUDPICESocketPtr getSocket() const;

        // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;
        // (duplicate) virtual const String &getPassword() const;

        // (duplicate) virtual const String &getDeviceID() const;
        // (duplicate) virtual const String &getUserAgent() const;
        // (duplicate) virtual const String &getOS() const;
        // (duplicate) virtual const String &getSystem() const;

        virtual bool isFinderReady() const;
        virtual String getFinderLocationID() const;

        // (duplicate) virtual bool sendFinderMessage(MessagePtr message);

        virtual void onAccountPeerLocationIncomingMessage(
                                                          IAccountPeerLocationPtr peerLocation,
                                                          MessagePtr message
                                                          );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountFinderDelegate
        #pragma mark

        virtual void onAccountFinderStateChanged(
                                                 IAccountFinderPtr finder,
                                                 AccountFinderStates state
                                                 );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountPeerLocationDelegate
        #pragma mark

        virtual void onAccountPeerLocationStateChanged(
                                                       IAccountPeerLocationPtr peerLocation,
                                                       AccountPeerLocationStates state
                                                       );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IBootstrappedNetworkDelegate
        #pragma mark

        virtual void onBootstrappedNetworkStateChanged(
                                                       IBootstrappedNetworkPtr bootstrapper,
                                                       BootstrappedNetworkStates state
                                                       );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IRUDPICESocket
        #pragma mark

        virtual void onRUDPICESocketStateChanged(
                                                 IRUDPICESocketPtr socket,
                                                 RUDPICESocketStates state
                                                 );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IMessageReuqesterDelegate
        #pragma mark

        virtual bool handleMessageRequesterMessageReceived(
                                                           IMessageRequesterPtr requester,
                                                           MessagePtr message
                                                           );

        virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForPeerSubscription
        #pragma mark

        virtual void onPeerSubscriptionShutdown(IPeerSubscriptionForAccountPtr peerSubscription);

        // (duplicate) virtual IMessageQueuePtr getAssociatedMessageQueue() const;

        virtual RecursiveLock &getLock() const {return mLock;}

        virtual bool isFinding(const String &contactID) const;

        virtual void getPeerLocations(
                                      const String &contactID,
                                      IPeerSubscription::LocationList &outLocations,
                                      bool includeOnlyConnectedLocations
                                      ) const;
        virtual void getPeerLocations(
                                      const String &contactID,
                                      IPeerSubscription::PeerLocationList &outLocations,
                                      bool includeOnlyConnectedLocations
                                      ) const;

        virtual bool sendPeerMessage(
                                     const String &contactID,
                                     const char *locationID,
                                     MessagePtr message
                                     );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForPublicationRepository
        #pragma mark

        virtual IAccountPtr convertIAccount() const {return mThisWeak.lock();}
        virtual IMessageQueuePtr getAssociatedMessageQueue() const {return MessageQueueAssociator::getAssociatedMessageQueue();}

        virtual String getContactID() const;
        // (duplicate) virtual const String &getLocationID() const;

        virtual IMessageRequesterPtr sendFinderRequest(
                                                       IMessageRequesterDelegatePtr delegate,
                                                       MessagePtr requestMessage,
                                                       Duration timeout
                                                       );

        virtual IMessageRequesterPtr sendPeerRequest(
                                                     IMessageRequesterDelegatePtr delegate,
                                                     MessagePtr requestMessage,
                                                     const char *contactID,
                                                     const char *locationID,
                                                     Duration timeout
                                                     );

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

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => friend ConnectionSubscription
        #pragma mark

        void notifyConnectionSubscriptionShutdown(ConnectionSubscriptionPtr subscription);
        IConnectionSubscription::ConnectionStates getFinderConnectionState() const;
        IConnectionSubscription::ConnectionStates getPeerLocationConnectionState(
                                                                                 const char *contactID,
                                                                                 const char *locationID
                                                                                 ) const;

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

        void cancel();
        void step();

        void setState(IAccount::AccountStates accountState);
        void setLastError(IAccount::AccountErrors error);

        void setFindState(
                          Peer &peer,
                          IPeerSubscription::PeerSubscriptionFindStates state
                          );

        bool shouldFind(
                        const String &contactID,
                        const PeerPtr &peer
                        ) const;
        bool shouldShutdownInactiveLocations(
                                             const String &contactID,
                                             const PeerPtr &peer
                                             ) const;

        void handleFindRequestComplete(IMessageRequesterPtr requester);

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ConnectionSubscription
        #pragma mark

        class ConnectionSubscription : public IConnectionSubscription
        {
        public:
          friend class Account;

        protected:
          ConnectionSubscription(
                                 AccountPtr outer,
                                 IConnectionSubscriptionDelegatePtr delegate
                                 );

          void init();

        public:
          ~ConnectionSubscription();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ConnectionSubscription => friend Account
          #pragma mark

          static ConnectionSubscriptionPtr create(
                                                  AccountPtr outer,
                                                  IConnectionSubscriptionDelegatePtr delegate
                                                  );

          PUID getID() const {return mID;}
          void notifyFinderStateChanged(ConnectionStates state);
          void notifyPeerStateChanged(
                                      IPeerLocationPtr location,
                                      ConnectionStates state
                                      );
          void notifyIncomingMessage(IConnectionSubscriptionMessagePtr message);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ConnectionSubscription => IConnectionSubscription
          #pragma mark

          virtual bool isShutdown() const;

          virtual ConnectionStates getFinderConnectionState() const;

          virtual ConnectionStates getPeerLocationConnectionState(
                                                                  const char *contactID,
                                                                  const char *locationID
                                                                  ) const;

          virtual void getPeerLocations(
                                        const char *contactID,
                                        PeerLocations &outLocations,
                                        bool includeOnlyConnectedLocations
                                        ) const;

          virtual bool sendFinderMessage(MessagePtr message);

          virtual bool sendPeerMessage(
                                       const char *contactID,
                                       const char *locationID,
                                       MessagePtr message
                                       );

          virtual void cancel();

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ConnectionSubscription => (internal)
          #pragma mark

          RecursiveLock &getLock() const;

          String log(const char *message) const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ConnectionSubscription => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          ConnectionSubscriptionWeakPtr mThisWeak;

          AccountWeakPtr mOuter;

          IConnectionSubscriptionDelegatePtr mDelegate;
          ConnectionStates mLastFinderState;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::SubscriptionMessage
        #pragma mark

        class SubscriptionMessage : public IPeerSubscriptionMessage,
                                    public IConnectionSubscriptionMessage
        {
        public:
          friend class Account;

        protected:
          SubscriptionMessage(
                              IPeerSubscriptionPtr peerSubscription,
                              IConnectionSubscriptionPtr connectionSubscription,
                              const char *contactID,
                              const char *locationID,
                              MessagePtr message,
                              SubscriptionNotificationHelperPtr helper
                              );

        public:
          ~SubscriptionMessage();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionMessage => friend Account
          #pragma mark

          static SubscriptionMessagePtr create(
                                               IPeerSubscriptionPtr outer,
                                               const char *contactID,
                                               const char *locationID,
                                               MessagePtr message,
                                               SubscriptionNotificationHelperPtr helper
                                               );

          static SubscriptionMessagePtr createForFinder(
                                                        IConnectionSubscriptionPtr outer,
                                                        MessagePtr message,
                                                        SubscriptionNotificationHelperPtr helper
                                                        );

          static SubscriptionMessagePtr createForPeer(
                                                      IConnectionSubscriptionPtr outer,
                                                      const char *contactID,
                                                      const char *locationID,
                                                      MessagePtr message,
                                                      SubscriptionNotificationHelperPtr helper
                                                      );

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionMessage => IPeerSubscriptionMessage
          #pragma mark

          virtual IPeerSubscriptionPtr getPeerSubscription() const;
          virtual String getContactID() const;
          virtual String getLocationID() const;
          virtual MessagePtr getMessage() const;

          virtual bool sendResponse(MessagePtr message);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionMessage => IConnectionSubscriptionMessage
          #pragma mark

          virtual IConnectionSubscriptionPtr getConnectionSubscription() const;

          virtual Sources getSource() const;

          virtual String getPeerContactID() const;
          virtual String getPeerLocationID() const;
          // (duplicate) virtual MessagePtr getMessage() const;

          // (duplicate) virtual bool sendResponse(MessagePtr message);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionMessage => (internal)
          #pragma mark

          String log(const char *message) const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionMessage => (data)
          #pragma mark

          PUID mID;

          Sources mSource;
          IConnectionSubscriptionPtr mConnectionSubscription;

          IPeerSubscriptionPtr mPeerSubscription;
          String mContactID;
          String mLocationID;
          MessagePtr mMessage;

          SubscriptionNotificationHelperPtr mNotificationHelper;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::SubscriptionNotificationHelper
        #pragma mark

        class SubscriptionNotificationHelper
        {
        public:
          typedef zsLib::ULONG ULONG;

          friend class Account;
          friend class Account::SubscriptionMessage;

        protected:
          SubscriptionNotificationHelper(ULONG totalExpectedHandlers);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionNotificationHelper => friend Account
          #pragma mark

          static SubscriptionNotificationHelperPtr create(ULONG totalExpectedHandlers);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionNotificationHelper => friend Account::PeerSubscriptionMessage
          #pragma mark

          void notifyHandledBySubscriber();
          void notifyPeerSubscriptionMessageDestroyed(Account::SubscriptionMessage *peerSubscriptionMessage);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionNotificationHelper => (internal)
          #pragma mark

          String log(const char *message) const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::SubscriptionNotificationHelper => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mLock;

          bool mHandled;
          ULONG mTotalExpectedHandlers;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::Peer
        #pragma mark

        struct Peer
        {
          typedef zsLib::Time Time;
          typedef String LocationID;

          typedef std::map<LocationID, IAccountPeerLocationPtr> LocationIDToPeerLocationMap;                          // every location needs a session
          typedef std::map<IPeerSubscriptionForAccountPtr, IPeerSubscriptionForAccountPtr> PeerSubscriptionMapList;   // using to keep track of all subscriptions for a peer
          typedef std::map<LocationID, LocationID> FindingBecauseOfLocationIDMapList;                                 // using this to track the reason why the find needs to be initated or reinitated

          static PeerPtr create();
          void findTimeReset();
          void findTimeScheduleNext();

          PUID mID;
          bool mFindAtNextPossibleMoment;

          IPeerFilePublicPtr mPeerFilePublic;                             // peer file to "find"
          IMessageRequesterPtr mPeerFindRequester;                        // the request monitor when a search is being conducted
          FindingBecauseOfLocationIDMapList mPeerFindBecauseOfLocations;  // peer find is being done because of locations that are known but not yet discovered

          FindingBecauseOfLocationIDMapList mPeerFindNeedsRedoingBecauseOfLocations;  // peer find needs to be redone as soon as complete because of locations that are known but not yet discovered

          LocationIDToPeerLocationMap mLocations;                         // list of connecting/connected peer locations

          IPeerSubscription::PeerSubscriptionFindStates mCurrentFindState;
          PeerSubscriptionMapList mPeerSubscriptions;                     // subscriptions to this peer, once peer subscriptions disappear the peer itself can be disconnected

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
        AccountErrors mLastError;

        IAccountDelegatePtr mDelegate;
        String mPassword;

        String mDeviceID;
        String mUserAgent;
        String mOS;
        String mSystem;

        TimerPtr mTimer;
        Time mLastTimerFired;
        Time mBlockLocationShutdownsUntil;

        IBootstrappedNetworkForAccountPtr mBootstrapper;
        IBootstrappedNetworkSubscriptionPtr mBootstrapperSubscription;

        IRUDPICESocketPtr mSocket;

        IPeerFilesPtr mPeerFiles;
        String mLocationID;

        IPublicationRepositoryForAccountPtr mRepository;
        IAccountFinderPtr mFinder;
        Time mFinderRetryAfter;
        Duration mLastRetryFinderAfterDuration;

        // peer contact ID to peer map
        typedef String ContactID;
        typedef std::map<ContactID, PeerPtr> PeerMap;
        PeerMap mPeers;

        typedef PUID ConnectionSubscriptionID;
        typedef std::map<ConnectionSubscriptionID, ConnectionSubscriptionPtr> ConnectionSubscriptionMap;
        ConnectionSubscriptionMap mConnectionSubscriptions;
      };
    }
  }
}


ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountForPeerSubscription)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::IMessageQueuePtr, IMessageQueuePtr)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::RecursiveLock, RecursiveLock)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::String, String)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::IPeerSubscriptionForAccountPtr, IPeerSubscriptionForAccountPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPeerSubscription, IPeerSubscription)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::message::MessagePtr, MessagePtr)
ZS_DECLARE_PROXY_METHOD_1(onPeerSubscriptionShutdown, IPeerSubscriptionForAccountPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getAssociatedMessageQueue, IMessageQueuePtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getLock, RecursiveLock &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_1(isFinding, bool, const String &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_3(getPeerLocations, const String &, IPeerSubscription::LocationList &, bool)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_3(getPeerLocations, const String &, IPeerSubscription::PeerLocationList &, bool)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_3(sendPeerMessage, bool, const String &, const char *, MessagePtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountForAccountPeerLocation)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::String, String)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::RecursiveLock, RecursiveLock)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::IMessageQueuePtr, IMessageQueuePtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IRUDPICESocketPtr, IRUDPICESocketPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPeerFilesPtr, IPeerFilesPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::message::MessagePtr, MessagePtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::IAccountPeerLocationPtr, IAccountPeerLocationPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getAssociatedMessageQueue, IMessageQueuePtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getLock, RecursiveLock &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getSocket, IRUDPICESocketPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getPeerFiles, IPeerFilesPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getPassword, const String &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getDeviceID, const String &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getUserAgent, const String &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getOS, const String &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getSystem, const String &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(isFinderReady, bool)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getFinderLocationID, String)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_1(sendFinderMessage, bool, MessagePtr)
ZS_DECLARE_PROXY_METHOD_2(onAccountPeerLocationIncomingMessage, IAccountPeerLocationPtr, MessagePtr)
ZS_DECLARE_PROXY_END()

