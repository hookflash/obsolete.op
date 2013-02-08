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

#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/IMessageRequester.h>
#include <hookflash/stack/message/hookflashTypes.h>

#include <hookflash/services/IDNS.h>
#include <hookflash/services/IHTTP.h>

#include <zsLib/MessageQueueAssociator.h>

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
      #pragma mark IBootstrappedNetworkForAccount
      #pragma mark

      interaction IBootstrappedNetworkForAccount
      {
        typedef zsLib::String String;

        static IBootstrappedNetworkForAccountPtr convert(IBootstrappedNetworkPtr network);

        virtual IBootstrappedNetwork::BootstrappedNetworkStates getState() const = 0;

        virtual IBootstrappedNetworkSubscriptionPtr subscribe(IBootstrappedNetworkDelegatePtr delegate) = 0;

        virtual void close() = 0;

        virtual void getTURNAndSTUNServers(
                                           String &outTURNServer,
                                           String &outTURNServerUsername,
                                           String &outTURNServerPassword,
                                           String &outSTUNServer
                                           ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForAccountFinder
      #pragma mark

      interaction IBootstrappedNetworkForAccountFinder
      {
        typedef zsLib::IPAddress IPAddress;

        static IBootstrappedNetworkForAccountFinderPtr convert(IBootstrappedNetworkPtr network);
        static IBootstrappedNetworkForAccountFinderPtr convert(IBootstrappedNetworkForAccountPtr network);

        virtual IBootstrappedNetwork::BootstrappedNetworkStates getState() const = 0;

        virtual IBootstrappedNetworkSubscriptionPtr subscribe(IBootstrappedNetworkDelegatePtr delegate) = 0;
        virtual IPAddress getNextPeerFinder() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkAsyncDelegate
      #pragma mark

      interaction IBootstrappedNetworkAsyncDelegate;
      typedef boost::shared_ptr<IBootstrappedNetworkAsyncDelegate> IBootstrappedNetworkAsyncDelegatePtr;
      typedef boost::weak_ptr<IBootstrappedNetworkAsyncDelegate> IBootstrappedNetworkAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IBootstrappedNetworkAsyncDelegate> IBootstrappedNetworkAsyncDelegateProxy;

      interaction IBootstrappedNetworkAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork
      #pragma mark

      class BootstrappedNetwork : public zsLib::MessageQueueAssociator,
                                  public IBootstrappedNetwork,
                                  public IBootstrappedNetworkForAccount,
                                  public IBootstrappedNetworkForAccountFinder,
                                  public services::IDNSDelegate,
                                  public services::IHTTPQueryDelegate,
                                  public IMessageRequesterDelegate,
                                  public IBootstrappedNetworkAsyncDelegate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::UINT UINT;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef zsLib::XML::ElementPtr ElementPtr;
        typedef services::IDNSQueryPtr IDNSQueryPtr;
        typedef services::IDNS::SRVResultPtr SRVResultPtr;
        typedef services::IHTTPQueryPtr IHTTPQueryPtr;
        typedef services::IDNS IDNS;
        typedef services::IDNSQuery IDNSQuery;

        class Subscription;
        typedef boost::shared_ptr<Subscription> SubscriptionPtr;
        typedef boost::weak_ptr<Subscription> SubscriptionWeakPtr;
        friend class Subscription;

        class Query;
        typedef boost::shared_ptr<Query> QueryPtr;
        typedef boost::weak_ptr<Query> QueryWeakPtr;
        friend class Query;

      protected:
        BootstrappedNetwork(IMessageQueuePtr queue);
        void init();

      public:
        ~BootstrappedNetwork();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetwork
        #pragma mark

        static BootstrappedNetworkPtr create(
                                             IMessageQueuePtr queue,
                                             stack::IBootstrappedNetworkDelegatePtr delegate,
                                             const char *userAgent,
                                             const char *networkURI,
                                             const char *turnServer,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             const char *stunServer
                                             );

        virtual BootstrappedNetworkStates getState() const;
        virtual BootstrappedNetworkErrors getLastError() const {return BootstrappedNetworkError_None;}

        virtual IBootstrappedNetworkSubscriptionPtr subscribe(IBootstrappedNetworkDelegatePtr delegate);

        // NOTE:  Caller does not have to wait until the class is "ready"
        //        before calling this method.
        virtual IBootstrappedNetworkFetchSignedSaltQueryPtr fetchSignedSalt(
                                                                            IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr delegate,
                                                                            UINT totalToFetch = 1
                                                                            );

        virtual void close();


        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetworkForAccount
        #pragma mark

        // (duplicate) virtual BootstrappedNetworkStates getState() const;
        // (duplicate) virtual IBootstrappedNetworkSubscriptionPtr subscribe(IBootstrappedNetworkDelegatePtr delegate);
        // (duplicate) virtual void close();
        virtual void getTURNAndSTUNServers(
                                           String &outTURNServer,
                                           String &outTURNServerUsername,
                                           String &outTURNServerPassword,
                                           String &outSTUNServer
                                           );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetworkForAccountFinder
        #pragma mark

        // (duplicate) virtual BootstrappedNetworkStates getState() const;
        // (duplicate) virtual IBootstrappedNetworkSubscriptionPtr subscribe(IBootstrappedNetworkDelegatePtr delegate);
        virtual IPAddress getNextPeerFinder();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IDNSDelegate
        #pragma mark

        virtual void onLookupCompleted(IDNSQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IHTTPQueryDelegate
        #pragma mark

        virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
        virtual void onHTTPComplete(IHTTPQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IMessageRequesterDelegate
        #pragma mark

        virtual bool handleMessageRequesterMessageReceived(
                                                           IMessageRequesterPtr requester,
                                                           message::MessagePtr message
                                                           );

        virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetworkAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => friend class Subscription
        #pragma mark

        void removeSubscription(SubscriptionPtr subscription);
        // (duplicate) RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => friend class Query
        #pragma mark

        void removeQuery(QueryPtr query);
        // (duplicate) RecursiveLock &getLock() const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => (internal)
        #pragma mark

        bool isPending() const {return BootstrappedNetworkState_Pending == mCurrentState;}
        bool isReady() const {return BootstrappedNetworkState_Ready == mCurrentState;}
        bool isShuttingDown() const {return BootstrappedNetworkState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return BootstrappedNetworkState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const {return mLock;}

        String log(const char *message) const;

        void cancel();
        void step();

        void setCurrentState(BootstrappedNetworkStates state);
        void setLastError(BootstrappedNetworkErrors error);

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork::Subscription
        #pragma mark

        class Subscription : public IBootstrappedNetworkSubscription
        {
        public:
          friend class BootstrappedNetwork;

        protected:
          Subscription();

        public:
          ~Subscription();

          static SubscriptionPtr create(BootstrappedNetworkPtr network);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Subscription => IBootstrappedNetworkSubscription
          #pragma mark

          virtual void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Query => friend BootstrappedNetwork
          #pragma mark

          PUID getID() const {return mID;}

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Subscription => (internal)
          #pragma mark

          RecursiveLock &getLock() const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Subscription => (data)
          #pragma mark

          mutable RecursiveLock mBogusLock;
          PUID mID;
          SubscriptionWeakPtr mThisWeak;

          BootstrappedNetworkWeakPtr mOuter;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork::Query
        #pragma mark

        class Query : public zsLib::MessageQueueAssociator,
                      public IBootstrappedNetworkFetchSignedSaltQuery,
                      public IHTTPQueryDelegate
        {
        public:
          typedef message::SaltBundleList SaltBundleList;

          friend class BootstrappedNetwork;

        protected:
          Query(
                IMessageQueuePtr queue,
                BootstrappedNetworkPtr outer,
                IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr delegate,
                UINT totalToFetch
                );

        public:
          ~Query();

          static QueryPtr create(
                                 IMessageQueuePtr queue,
                                 BootstrappedNetworkPtr network,
                                 IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr delegate,
                                 UINT totalToFetch
                                 );

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Query => IBootstrappedNetworkFetchSignedSaltQueryDelegate
          #pragma mark

          virtual bool isComplete() const;
          virtual UINT getTotalSignedSaltsAvailable() const;
          virtual ElementPtr getNextSignedSalt();

          virtual void cancel();

          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Query => IHTTPQueryDelegate
          #pragma mark

          virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
          virtual void onHTTPComplete(IHTTPQueryPtr query);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Query => friend BootstrappedNetwork
          #pragma mark

          PUID getID() const {return mID;}
          void notify(
                      const String &userAgent,
                      const String &saltURL
                      );

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Query => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark BootstrappedNetwork::Query => (data)
          #pragma mark

          mutable RecursiveLock mBogusLock;
          PUID mID;
          QueryWeakPtr mThisWeak;

          BootstrappedNetworkWeakPtr mOuter;

          IHTTPQueryPtr mSaltQuery;

          UINT mRequestingSalts;
          IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr mDelegate;

          SaltBundleList mSaltBundles;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        BootstrappedNetworkWeakPtr mThisWeak;

        BootstrappedNetworkStates mCurrentState;
        BootstrappedNetworkErrors mLastError;

        typedef PUID SubscriptionID;
        typedef std::map<SubscriptionID, IBootstrappedNetworkDelegatePtr> DelegateMap;
        DelegateMap mDelegates;

        typedef PUID QueryID;
        typedef std::map<QueryID, QueryPtr> PendingQueriesMap;
        PendingQueriesMap mPendingQueries;

        String mUserAgent;
        String mURI;
        String mTURNServer;
        String mTURNServerUsername;
        String mTURNServerPassword;
        String mSTUNServer;

        String mBootstrapperURI;
        String mSaltURI;

        IHTTPQueryPtr mServiceQuery;
        IMessageRequesterPtr mServiceRequester;

        IHTTPQueryPtr mFindersQuery;
        IMessageRequesterPtr mFindersRequester;

        SRVResultPtr mSRVPeerFinderFinal;
        SRVResultPtr mSRVClonedPeerFinders;

        typedef IDNSQuery::PUID DNSQueryID;
        typedef std::map<DNSQueryID, IDNSQueryPtr> FinderLookupMap;
        FinderLookupMap mFinderLookups;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IBootstrappedNetworkAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
