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

#include <openpeer/stack/internal/types.h>
#include <openpeer/stack/IPublicationRepository.h>
#include <openpeer/stack/IPublication.h>
#include <openpeer/stack/IMessageMonitor.h>
#include <openpeer/stack/IPeerSubscription.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

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
      #pragma mark IPublicationRepositoryForAccount
      #pragma mark

      interaction IPublicationRepositoryForAccount
      {
        IPublicationRepositoryForAccount &forAccount() {return *this;}
        const IPublicationRepositoryForAccount &forAccount() const {return *this;}

        static PublicationRepositoryPtr create(AccountPtr account);

        virtual PUID getID() const = 0;

        virtual void cancel() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository
      #pragma mark

      class PublicationRepository : public Noop,
                                    public MessageQueueAssociator,
                                    public IPublicationRepository,
                                    public IPublicationRepositoryForAccount,
                                    public IPeerSubscriptionDelegate,
                                    public ITimerDelegate
      {
      public:
        friend interaction IPublicationRepositoryFactory;
        friend interaction IPublicationRepository;

        struct CacheCompare
        {
          bool operator()(const PublicationMetaDataPtr &x, const PublicationMetaDataPtr &y) const;
        };

      public:
        typedef IPublication::RelationshipList RelationshipList;
        typedef IPublication::RelationshipListPtr RelationshipListPtr;
        typedef IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;
        typedef std::map<PublicationMetaDataPtr, PublicationPtr, CacheCompare> CachedPublicationMap;
        typedef String PublicationName;
        typedef std::map<PublicationName, PublicationPtr> CachedPublicationPermissionMap;
        typedef PublicationMetaDataPtr PeerSourcePtr;
        typedef std::map<PublicationMetaDataPtr, PublicationMetaDataPtr, CacheCompare> CachedPeerPublicationMap;
        typedef message::peer_common::MessageFactoryPeerCommon MessageFactoryPeerCommon;
        typedef message::peer_common::PeerPublishRequest PeerPublishRequest;
        typedef message::peer_common::PeerPublishRequestPtr PeerPublishRequestPtr;
        typedef message::peer_common::PeerPublishResult PeerPublishResult;
        typedef message::peer_common::PeerPublishResultPtr PeerPublishResultPtr;
        typedef message::peer_common::PeerGetRequest PeerGetRequest;
        typedef message::peer_common::PeerGetRequestPtr PeerGetRequestPtr;
        typedef message::peer_common::PeerGetResult PeerGetResult;
        typedef message::peer_common::PeerGetResultPtr PeerGetResultPtr;
        typedef message::peer_common::PeerDeleteRequest PeerDeleteRequest;
        typedef message::peer_common::PeerDeleteRequestPtr PeerDeleteRequestPtr;
        typedef message::peer_common::PeerDeleteResult PeerDeleteResult;
        typedef message::peer_common::PeerDeleteResultPtr PeerDeleteResultPtr;
        typedef message::peer_common::PeerSubscribeRequest PeerSubscribeRequest;
        typedef message::peer_common::PeerSubscribeRequestPtr PeerSubscribeRequestPtr;
        typedef message::peer_common::PeerSubscribeResult PeerSubscribeResult;
        typedef message::peer_common::PeerSubscribeResultPtr PeerSubscribeResultPtr;
        typedef message::peer_common::PeerPublishNotifyRequest PeerPublishNotifyRequest;
        typedef message::peer_common::PeerPublishNotifyRequestPtr PeerPublishNotifyRequestPtr;
        typedef message::peer_common::PeerPublishNotifyResult PeerPublishNotifyResult;
        typedef message::peer_common::PeerPublishNotifyResultPtr PeerPublishNotifyResultPtr;

        class PeerCache;
        typedef boost::shared_ptr<PeerCache> PeerCachePtr;
        typedef boost::weak_ptr<PeerCache> PeerCacheWeakPtr;
        friend class PeerCache;

        typedef std::map<PeerSourcePtr, PeerCachePtr, CacheCompare> CachedPeerSourceMap;

        class Publisher;
        typedef boost::shared_ptr<Publisher> PublisherPtr;
        typedef boost::weak_ptr<Publisher> PublisherWeakPtr;
        friend class Publisher;

        class Fetcher;
        typedef boost::shared_ptr<Fetcher> FetcherPtr;
        typedef boost::weak_ptr<Fetcher> FetcherWeakPtr;
        friend class Fetcher;

        class Remover;
        typedef boost::shared_ptr<Remover> RemoverPtr;
        typedef boost::weak_ptr<Remover> RemoverWeakPtr;
        friend class Remover;

        class SubscriptionLocal;
        typedef boost::shared_ptr<SubscriptionLocal> SubscriptionLocalPtr;
        typedef boost::weak_ptr<SubscriptionLocal> SubscriptionLocalWeakPtr;
        friend class SubscriptionLocal;

        class PeerSubscriptionIncoming;
        typedef boost::shared_ptr<PeerSubscriptionIncoming> PeerSubscriptionIncomingPtr;
        typedef boost::weak_ptr<PeerSubscriptionIncoming> PeerSubscriptionIncomingWeakPtr;
        friend class PeerSubscriptionIncoming;

        class PeerSubscriptionOutgoing;
        typedef boost::shared_ptr<PeerSubscriptionOutgoing> PeerSubscriptionOutgoingPtr;
        typedef boost::weak_ptr<PeerSubscriptionOutgoing> PeerSubscriptionOutgoingWeakPtr;
        friend class PeerSubscriptionOutgoing;

        typedef PUID SubscriptionLocationID;
        typedef std::map<SubscriptionLocationID, SubscriptionLocalPtr> SubscriptionLocalMap;

        typedef PUID PeerSubscriptionIncomingID;
        typedef std::map<PeerSubscriptionIncomingID, PeerSubscriptionIncomingPtr> PeerSubscriptionIncomingMap;

        typedef PUID PeerSubscriptionOutgoingID;
        typedef std::map<PeerSubscriptionOutgoingID, PeerSubscriptionOutgoingPtr> PeerSubscriptionOutgoingMap;

        typedef std::list<FetcherPtr> PendingFetcherList;
        typedef std::list<PublisherPtr> PendingPublisherList;

      protected:
        PublicationRepository(
                              IMessageQueuePtr queue,
                              AccountPtr account
                              );
        
        PublicationRepository(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~PublicationRepository();

        static PublicationRepositoryPtr convert(IPublicationRepositoryPtr repository);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => IPublicationRepositoryForAccount
        #pragma mark

        static PublicationRepositoryPtr create(AccountPtr account);

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => IPublicationRepository
        #pragma mark

        static String toDebugString(IPublicationRepositoryPtr repository, bool includeCommaPrefix = true);

        static PublicationRepositoryPtr getFromAccount(IAccountPtr account);

        virtual PUID getID() const {return mID;}

        virtual IPublicationPublisherPtr publish(
                                                 IPublicationPublisherDelegatePtr delegate,
                                                 IPublicationPtr publication
                                                 );

        virtual IPublicationFetcherPtr fetch(
                                             IPublicationFetcherDelegatePtr delegate,
                                             IPublicationMetaDataPtr metaData
                                             );

        virtual IPublicationRemoverPtr remove(
                                              IPublicationRemoverDelegatePtr delegate,
                                              IPublicationPtr publication
                                              );

        virtual IPublicationSubscriptionPtr subscribe(
                                                      IPublicationSubscriptionDelegatePtr delegate,
                                                      ILocationPtr subscribeToLocation,
                                                      const char *publicationPath,
                                                      const SubscribeToRelationshipsMap &relationships
                                                      );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => IPeerSubscriptionDelegate
        #pragma mark

        virtual void onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription);

        virtual void onPeerSubscriptionFindStateChanged(
                                                        IPeerSubscriptionPtr subscription,
                                                        IPeerPtr peer,
                                                        PeerFindStates state
                                                        );

        virtual void onPeerSubscriptionLocationConnectionStateChanged(
                                                                      IPeerSubscriptionPtr subscription,
                                                                      ILocationPtr location,
                                                                      LocationConnectionStates state
                                                                      );

        virtual void onPeerSubscriptionMessageIncoming(
                                                       IPeerSubscriptionPtr subscription,
                                                       IMessageIncomingPtr message
                                                       );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend PeerCache
        #pragma mark

        CachedPeerSourceMap &getCachedPeerSources() {return mCachedPeerSources;}

        // (duplicate) RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend Publisher
        #pragma mark

        void notifyPublished(PublisherPtr publisher);
        void notifyPublisherCancelled(PublisherPtr publisher);

        // (duplicate) RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend Fetcher
        #pragma mark

        void notifyFetched(FetcherPtr fetcher);
        void notifyFetcherCancelled(FetcherPtr fetcher);

        // (duplicate) RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend Remover
        #pragma mark

        void notifyRemoved(RemoverPtr remover);

        // (duplicate) RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend PeerSubscriptionOutgoing
        #pragma mark

        void notifySubscribed(PeerSubscriptionOutgoingPtr subscriber);
        void notifyPeerOutgoingSubscriptionShutdown(PeerSubscriptionOutgoingPtr subscription);

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) IAccountForPublicationRepositoryPtr getOuter() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend SubscriptionLocal
        #pragma mark

        void notifyLocalSubscriptionShutdown(SubscriptionLocalPtr subscription);

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) IAccountForPublicationRepositoryPtr getOuter() const;

        void resolveRelationships(
                                  const PublishToRelationshipsMap &publishToRelationships,
                                  RelationshipList &outContacts
                                  ) const;

        bool canFetchPublication(
                                 const PublishToRelationshipsMap &publishToRelationships,
                                 LocationPtr location
                                 ) const;

        bool canSubscribeToPublisher(
                                     LocationPtr publicationCreatorLocation,
                                     const PublishToRelationshipsMap &publishToRelationships,
                                     LocationPtr subscriberLocation,
                                     const SubscribeToRelationshipsMap &subscribeToRelationships
                                     ) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend PeerSubscriptionIncoming
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) IAccountForPublicationRepositoryPtr getOuter() const;

        // (duplicate) void resolveRelationships(
        //                                       const PublishToRelationshipsMap &publishToRelationships,
        //                                       RelationshipList &outContacts
        //                                       ) const;

      private:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => (internal)
        #pragma mark

        RecursiveLock &getLock() const {return mLock;}
        AccountPtr getAccount() const {return mAccount.lock();}

        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void activateFetcher(PublicationMetaDataPtr metaData);
        void activatePublisher(PublicationPtr publication);


        PeerSubscriptionIncomingPtr findIncomingSubscription(PublicationMetaDataPtr metaData) const;

        void onMessageIncoming(
                               IMessageIncomingPtr messageIncoming,
                               PeerPublishRequestPtr request
                               );

        void onMessageIncoming(
                               IMessageIncomingPtr messageIncoming,
                               PeerGetRequestPtr request
                               );

        void onMessageIncoming(
                               IMessageIncomingPtr messageIncoming,
                               PeerDeleteRequestPtr request
                               );

        void onMessageIncoming(
                               IMessageIncomingPtr messageIncoming,
                               PeerSubscribeRequestPtr request
                               );

        void onMessageIncoming(
                               IMessageIncomingPtr messageIncoming,
                               PeerPublishNotifyRequestPtr request
                               );

        void cancel();

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::PeerCache
        #pragma mark

        class PeerCache : public IPublicationRepositoryPeerCache
        {
        public:
          friend class PublicationRepository;

        protected:
          PeerCache(
                    PeerSourcePtr peerSource,
                    PublicationRepositoryPtr repository
                    );

          void init();

          static PeerCachePtr create(
                                     PeerSourcePtr peerSource,
                                     PublicationRepositoryPtr repository
                                     );

        public:
          ~PeerCache();

          static PeerCachePtr convert(IPublicationRepositoryPeerCachePtr cache);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerCache => IPublicationRepositoryPeerCache
          #pragma mark

          static String toDebugString(IPublicationRepositoryPeerCachePtr cache, bool includeCommaPrefix = true);

          virtual bool getNextVersionToNotifyAboutAndMarkNotified(
                                                                  IPublicationPtr publication,
                                                                  ULONG &ioMaxSizeAvailableInBytes,
                                                                  ULONG &outNotifyFromVersion,
                                                                  ULONG &outNotifyToVersion
                                                                  );

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerCache => friend PublicationRepository
          #pragma mark

          static PeerCachePtr find(
                                   PeerSourcePtr peerSource,
                                   PublicationRepositoryPtr repository
                                   );

          void notifyFetched(PublicationPtr publication);

          Time getExpires() const       {return mExpires;}
          void setExpires(Time expires) {mExpires = expires;}

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerCache => (internal)
          #pragma mark

          String log(const char *message) const;
          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          RecursiveLock &getLock() const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerCache => (data)
          #pragma mark

          mutable RecursiveLock mBogusLock;
          PUID mID;
          PeerCacheWeakPtr mThisWeak;
          PublicationRepositoryWeakPtr mOuter;

          PeerSourcePtr mPeerSource;

          Time mExpires;

          CachedPeerPublicationMap mCachedPublications;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::Publisher
        #pragma mark

        class Publisher : public MessageQueueAssociator,
                          public IPublicationPublisher,
                          public IMessageMonitorDelegate
        {
        public:
          friend class PublicationRepository;

        protected:
          Publisher(
                    IMessageQueuePtr queue,
                    PublicationRepositoryPtr outer,
                    IPublicationPublisherDelegatePtr delegate,
                    PublicationPtr publication
                    );

          void init();

        public:
          ~Publisher();

          static PublisherPtr convert(IPublicationPublisherPtr);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => friend PublicationRepository
          #pragma mark

          static String toDebugString(IPublicationPublisherPtr publisher, bool includeCommaPrefix = true);

          static PublisherPtr create(
                                     IMessageQueuePtr queue,
                                     PublicationRepositoryPtr outer,
                                     IPublicationPublisherDelegatePtr delegate,
                                     PublicationPtr publication
                                     );

          // PUID getID() const;

          void setMonitor(IMessageMonitorPtr monitor);
          void notifyCompleted();

          // (duplicate) virtual IPublicationPtr getPublication() const;
          // (duplicate) virtual void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => IPublicationPublisher
          #pragma mark

          virtual void cancel();
          virtual bool isComplete() const;

          virtual bool wasSuccessful(
                                     WORD *outErrorResult = NULL,
                                     String *outReason = NULL
                                     ) const;

          virtual IPublicationPtr getPublication() const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => IMessageMonitorDelegate
          #pragma mark

          virtual bool handleMessageMonitorMessageReceived(
                                                           IMessageMonitorPtr monitor,
                                                           message::MessagePtr message
                                                           );

          virtual void onMessageMonitorTimedOut(IMessageMonitorPtr monitor);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          String log(const char *message) const;

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          IMessageMonitorPtr getMonitor() const;

          RecursiveLock &getLock() const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          PublicationRepositoryWeakPtr mOuter;

          PublisherWeakPtr mThisWeak;

          IPublicationPublisherDelegatePtr mDelegate;

          PublicationPtr mPublication;

          IMessageMonitorPtr mMonitor;

          bool mSucceeded;
          WORD mErrorCode;
          String mErrorReason;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::Fetcher
        #pragma mark

        class Fetcher : public MessageQueueAssociator,
                        public IPublicationFetcher,
                        public IMessageMonitorDelegate
        {
        public:
          friend class PublicationRepository;

        protected:
          Fetcher(
                  IMessageQueuePtr queue,
                  PublicationRepositoryPtr outer,
                  IPublicationFetcherDelegatePtr delegate,
                  PublicationMetaDataPtr metaData
                  );

          void init();

        public:
          ~Fetcher();

          static FetcherPtr convert(IPublicationFetcherPtr fetcher);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => friend PublicationRepository
          #pragma mark

          static String toDebugString(IPublicationFetcherPtr fetcher, bool includeCommaPrefix = true);

          static FetcherPtr create(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   IPublicationFetcherDelegatePtr delegate,
                                   PublicationMetaDataPtr metaData
                                   );

          void setPublication(PublicationPtr publication);
          void setMonitor(IMessageMonitorPtr monitor);
          void notifyCompleted();

          // (duplicate) virtual void cancel();
          // (duplicate) virtual IPublicationMetaDataPtr getPublicationMetaData() const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => IPublicationFetcher
          #pragma mark

          virtual void cancel();
          virtual bool isComplete() const;

          virtual bool wasSuccessful(
                                     WORD *outErrorResult = NULL,
                                     String *outReason = NULL
                                     ) const;

          virtual IPublicationPtr getFetchedPublication() const;

          virtual IPublicationMetaDataPtr getPublicationMetaData() const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => IMessageMonitorDelegate
          #pragma mark

          virtual bool handleMessageMonitorMessageReceived(
                                                           IMessageMonitorPtr monitor,
                                                           message::MessagePtr message
                                                           );

          virtual void onMessageMonitorTimedOut(IMessageMonitorPtr monitor);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          String log(const char *message) const;

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          IMessageMonitorPtr getMonitor() const;

          RecursiveLock &getLock() const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          PublicationRepositoryWeakPtr mOuter;

          FetcherWeakPtr mThisWeak;

          IPublicationFetcherDelegatePtr mDelegate;

          PublicationMetaDataPtr mPublicationMetaData;

          IMessageMonitorPtr mMonitor;

          bool mSucceeded;
          WORD mErrorCode;
          String mErrorReason;

          PublicationPtr mFetchedPublication;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::Remover
        #pragma mark

        class Remover : public MessageQueueAssociator,
                        public IPublicationRemover,
                        public IMessageMonitorDelegate
        {
        public:
          friend class PublicationRepository;

        protected:
          Remover(
                  IMessageQueuePtr queue,
                  PublicationRepositoryPtr outer,
                  IPublicationRemoverDelegatePtr delegate,
                  PublicationPtr publication
                  );

          void init();

        public:
          ~Remover();

          static RemoverPtr convert(IPublicationRemoverPtr remover);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => friend PublicationRepository
          #pragma mark

          static String toDebugString(IPublicationRemoverPtr remover, bool includeCommaPrefix = true);

          static RemoverPtr create(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   IPublicationRemoverDelegatePtr delegate,
                                   PublicationPtr publication
                                   );

          void setMonitor(IMessageMonitorPtr monitor);

          void notifyCompleted();

          // (duplicate) virtual void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => IPublicationRemover
          #pragma mark

          virtual void cancel();
          virtual bool isComplete() const;

          virtual bool wasSuccessful(
                                     WORD *outErrorResult = NULL,
                                     String *outReason = NULL
                                     ) const;

          virtual IPublicationPtr getPublication() const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => IMessageMonitorDelegate
          #pragma mark

          virtual bool handleMessageMonitorMessageReceived(
                                                           IMessageMonitorPtr monitor,
                                                           message::MessagePtr message
                                                           );

          virtual void onMessageMonitorTimedOut(IMessageMonitorPtr monitor);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          PublicationRepositoryWeakPtr mOuter;

          RemoverWeakPtr mThisWeak;
          IPublicationRemoverDelegatePtr mDelegate;

          PublicationPtr mPublication;

          IMessageMonitorPtr mMonitor;

          bool mSucceeded;
          WORD mErrorCode;
          String mErrorReason;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::SubscriptionLocal
        #pragma mark

        class SubscriptionLocal : public MessageQueueAssociator,
                                  public IPublicationSubscription
        {
        public:
          typedef IPublicationMetaData::SubscribeToRelationshipsMap SubscribeToRelationshipsMap;

          friend class PublicationRepository;

        protected:
          SubscriptionLocal(
                            IMessageQueuePtr queue,
                            PublicationRepositoryPtr outer,
                            IPublicationSubscriptionDelegatePtr delegate,
                            const char *publicationPath,
                            const SubscribeToRelationshipsMap &relationships
                            );

          void init();

        public:
          ~SubscriptionLocal();

          static SubscriptionLocalPtr convert(IPublicationSubscriptionPtr subscription);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::SubscriptionLocal => friend SubscriptionLocal
          #pragma mark

          static String toDebugString(SubscriptionLocalPtr subscription, bool includeCommaPrefix = true);

          static SubscriptionLocalPtr create(
                                             IMessageQueuePtr queue,
                                             PublicationRepositoryPtr outer,
                                             IPublicationSubscriptionDelegatePtr delegate,
                                             const char *publicationPath,
                                             const SubscribeToRelationshipsMap &relationships
                                             );

          void notifyUpdated(PublicationPtr publication);
          void notifyGone(PublicationPtr publication);

          // (duplicate) virtual void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::SubscriptionLocal => IPublicationSubscription
          #pragma mark

          virtual void cancel();

          virtual PublicationSubscriptionStates getState() const;
          virtual IPublicationMetaDataPtr getSource() const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::SubscriptionLocal => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          RecursiveLock &getLock() const;
          String log(const char *message) const;

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          void setState(PublicationSubscriptionStates state);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::SubscriptionLocal => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          PublicationRepositoryWeakPtr mOuter;

          SubscriptionLocalWeakPtr mThisWeak;

          IPublicationSubscriptionDelegatePtr mDelegate;

          PublicationMetaDataPtr mSubscriptionInfo;
          PublicationSubscriptionStates mCurrentState;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::PeerSubscriptionIncoming
        #pragma mark

        class PeerSubscriptionIncoming : public MessageQueueAssociator,
                                         public IMessageMonitorDelegate
        {
        public:
          typedef IPublicationMetaData::SubscribeToRelationshipsMap SubscribeToRelationshipsMap;

          friend class PublicationRepository;

        protected:
          PeerSubscriptionIncoming(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   PeerSourcePtr peerSource,
                                   PublicationMetaDataPtr subscriptionInfo
                                   );

          void init();

        public:
          ~PeerSubscriptionIncoming();

          static String toDebugString(PeerSubscriptionIncomingPtr subscription, bool includeCommaPrefix = true);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionIncoming => friend PublicationRepository
          #pragma mark

          static PeerSubscriptionIncomingPtr create(
                                                    IMessageQueuePtr queue,
                                                    PublicationRepositoryPtr outer,
                                                    PeerSourcePtr peerSource,
                                                    PublicationMetaDataPtr subscriptionInfo
                                                    );

          // (duplicate) PUID getID() const;

          void notifyUpdated(PublicationPtr publication);
          void notifyGone(PublicationPtr publication);

          void notifyUpdated(const CachedPublicationMap &cachedPublications);
          void notifyGone(const CachedPublicationMap &publication);

          IPublicationMetaDataPtr getSource() const;

          void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionIncoming => IMessageMonitorDelegate
          #pragma mark

          virtual bool handleMessageMonitorMessageReceived(
                                                           IMessageMonitorPtr monitor,
                                                           message::MessagePtr message
                                                           );

          virtual void onMessageMonitorTimedOut(IMessageMonitorPtr monitor);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionIncoming => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          RecursiveLock &getLock() const;
          String log(const char *message) const;

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionIncoming => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          PublicationRepositoryWeakPtr mOuter;

          PeerSubscriptionIncomingWeakPtr mThisWeak;

          PeerSourcePtr mPeerSource;
          PublicationMetaDataPtr mSubscriptionInfo;

          typedef std::list<IMessageMonitorPtr> NotificationMonitorList;
          NotificationMonitorList mNotificationMonitors;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::PeerSubscriptionOutgoing
        #pragma mark

        class PeerSubscriptionOutgoing : public MessageQueueAssociator,
                                         public IPublicationSubscription,
                                         public IMessageMonitorDelegate
        {
        public:
          typedef IPublicationMetaData::SubscribeToRelationshipsMap SubscribeToRelationshipsMap;

          friend class PublicationRepository;

        protected:
          PeerSubscriptionOutgoing(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   IPublicationSubscriptionDelegatePtr delegate,
                                   PublicationMetaDataPtr subscriptionInfo
                                   );

          void init();

        public:
          ~PeerSubscriptionOutgoing();

          static PeerSubscriptionOutgoingPtr convert(IPublicationSubscriptionPtr subscription);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => friend PublicationRepository
          #pragma mark

          static String toDebugString(PeerSubscriptionOutgoingPtr subscription, bool includeCommaPrefix = true);

          static PeerSubscriptionOutgoingPtr create(
                                                    IMessageQueuePtr queue,
                                                    PublicationRepositoryPtr outer,
                                                    IPublicationSubscriptionDelegatePtr delegate,
                                                    PublicationMetaDataPtr subscriptionInfo
                                                    );

          // (duplicate) PUID getID() const;
          // (duplicate) virtual void cancel();

          // (duplicate) virtual IPublicationMetaDataPtr getSource() const;

          void setMonitor(IMessageMonitorPtr monitor);
          void notifyUpdated(PublicationMetaDataPtr metaData);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => IPublicationSubscription
          #pragma mark

          // (duplicate) virtual void cancel();

          virtual PublicationSubscriptionStates getState() const;
          virtual IPublicationMetaDataPtr getSource() const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => IMessageMonitorDelegate
          #pragma mark

          virtual bool handleMessageMonitorMessageReceived(
                                                           IMessageMonitorPtr monitor,
                                                           message::MessagePtr message
                                                           );

          virtual void onMessageMonitorTimedOut(IMessageMonitorPtr monitor);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          RecursiveLock &getLock() const;
          String log(const char *message) const;

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          bool isPending() const {return PublicationSubscriptionState_Pending == mCurrentState;}
          bool isEstablished() const {return PublicationSubscriptionState_Established == mCurrentState;}
          bool isShuttingDown() const {return PublicationSubscriptionState_ShuttingDown == mCurrentState;}
          bool isShutdown() const {return PublicationSubscriptionState_Shutdown == mCurrentState;}

          void setState(PublicationSubscriptionStates state);

          void cancel();

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          PublicationRepositoryWeakPtr mOuter;

          PeerSubscriptionOutgoingWeakPtr mThisWeak;
          PeerSubscriptionOutgoingPtr mGracefulShutdownReference;

          IPublicationSubscriptionDelegatePtr mDelegate;

          PublicationMetaDataPtr mSubscriptionInfo;

          IMessageMonitorPtr mMonitor;
          IMessageMonitorPtr mCancelMonitor;

          bool mSucceeded;
          WORD mErrorCode;
          String mErrorReason;

          PublicationSubscriptionStates mCurrentState;
        };

      private:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::PeerSubscriptionOutgoing => (data)
        #pragma mark

        AutoPUID mID;
        mutable RecursiveLock mLock;
        PublicationRepositoryWeakPtr mThisWeak;

        AccountWeakPtr mAccount;

        TimerPtr mExpiresTimer;

        IPeerSubscriptionPtr mPeerSubscription;

        CachedPublicationMap mCachedLocalPublications;        // documents that have been published from a source to the local repository
        CachedPublicationMap mCachedRemotePublications;       // documents that have been fetched from a remote repository

        CachedPublicationPermissionMap mCachedPermissionDocuments;

        SubscriptionLocalMap mSubscriptionsLocal;

        PeerSubscriptionIncomingMap mPeerSubscriptionsIncoming;

        PeerSubscriptionOutgoingMap mPeerSubscriptionsOutgoing;

        PendingFetcherList mPendingFetchers;

        PendingPublisherList mPendingPublishers;

        CachedPeerSourceMap mCachedPeerSources;               // represents the document notification state of each peer subscribing to this location
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationRepositoryFactory
      #pragma mark

      interaction IPublicationRepositoryFactory
      {
        static IPublicationRepositoryFactory &singleton();

        virtual PublicationRepositoryPtr createPublicationRepository(AccountPtr account);
      };

    }
  }
}
