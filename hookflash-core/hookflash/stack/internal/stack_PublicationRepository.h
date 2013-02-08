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

#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/IPublicationRepository.h>
#include <hookflash/stack/IPublication.h>
#include <hookflash/stack/IMessageRequester.h>
#include <hookflash/stack/IConnectionSubscription.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

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
      #pragma mark IPublicationRepositoryForAccount
      #pragma mark

      interaction IPublicationRepositoryForAccount
      {
        static IPublicationRepositoryForAccountPtr create(IAccountForPublicationRepositoryPtr outer);

        static IPublicationRepositoryForAccountPtr convert(IPublicationRepositoryPtr repository);
        virtual IPublicationRepositoryPtr convertIPublicationRepository() const = 0;

        virtual void cancel() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository
      #pragma mark

      class PublicationRepository : public zsLib::MessageQueueAssociator,
                                    public IPublicationRepository,
                                    public IPublicationRepositoryForAccount,
                                    public IConnectionSubscriptionDelegate,
                                    public zsLib::ITimerDelegate
      {
      public:
        struct CacheCompare
        {
          bool operator()(const IPublicationMetaDataForPublicationRepositoryPtr &x, const IPublicationMetaDataForPublicationRepositoryPtr &y) const;
        };

      public:
        typedef zsLib::WORD WORD;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::String String;
        typedef zsLib::PUID PUID;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef zsLib::TimerPtr TimerPtr;
        typedef IPublicationMetaData::Sources Sources;
        typedef IPublication::RelationshipList RelationshipList;
        typedef IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;
        typedef std::map<IPublicationMetaDataForPublicationRepositoryPtr, IPublicationForPublicationRepositoryPtr, CacheCompare> CachedPublicationMap;
        typedef String PublicationName;
        typedef std::map<PublicationName, IPublicationForPublicationRepositoryPtr> CachedPublicationPermissionMap;
        typedef IPublicationMetaDataForPublicationRepositoryPtr PeerSourcePtr;
        typedef std::map<IPublicationMetaDataForPublicationRepositoryPtr, IPublicationMetaDataForPublicationRepositoryPtr, CacheCompare> CachedPeerPublicationMap;

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

      protected:
        PublicationRepository(
                              IMessageQueuePtr queue,
                              IAccountForPublicationRepositoryPtr outer
                              );

        void init();

      public:
        ~PublicationRepository();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => IPublicationRepositoryForAccount
        #pragma mark

        static PublicationRepositoryPtr create(IAccountForPublicationRepositoryPtr outer);
        virtual IPublicationRepositoryPtr convertIPublicationRepository() const {return mThisWeak.lock();}
        // (duplicate) virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => IPublicationRepository
        #pragma mark

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

        virtual IPublicationSubscriptionPtr subscribeLocal(
                                                           IPublicationSubscriptionDelegatePtr delegate,
                                                           const char *publicationPath,
                                                           const SubscribeToRelationshipsMap &relationships
                                                           );

        virtual IPublicationSubscriptionPtr subscribeFinder(
                                                            IPublicationSubscriptionDelegatePtr delegate,
                                                            const char *publicationPath,
                                                            const SubscribeToRelationshipsMap &relationships
                                                            );

        virtual IPublicationSubscriptionPtr subscribePeer(
                                                          IPublicationSubscriptionDelegatePtr delegate,
                                                          const char *publicationPath,
                                                          const SubscribeToRelationshipsMap &relationships,
                                                          const char *peerSourceContactID,
                                                          const char *peerSourceLocationID
                                                          );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => IConnectionSubscriptionDelegate
        #pragma mark

        virtual void onConnectionSubscriptionShutdown(IConnectionSubscriptionPtr subscription);

        virtual void onConnectionSubscriptionFinderConnectionStateChanged(
                                                                          IConnectionSubscriptionPtr subscription,
                                                                          ConnectionStates state
                                                                          );

        virtual void onConnectionSubscriptionPeerLocationConnectionStateChanged(
                                                                                IConnectionSubscriptionPtr subscription,
                                                                                IPeerLocationPtr location,
                                                                                ConnectionStates state
                                                                                );

        virtual void onConnectionSubscriptionIncomingMessage(
                                                             IConnectionSubscriptionPtr subscription,
                                                             IConnectionSubscriptionMessagePtr message
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      protected:
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
                                 const String &fetcherContactID
                                 ) const;

        bool canSubscribeToPublisher(
                                     const String &publicationCreatorContactID,
                                     const PublishToRelationshipsMap &publishToRelationships,
                                     const String &subscriberContactID,
                                     const SubscribeToRelationshipsMap &subscribeToRelationships
                                     ) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => friend PeerSubscriptionIncoming
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) IAccountForPublicationRepositoryPtr getOuter() const;

        // void resolveRelationships(
        //                           const PublishToRelationshipsMap &publishToRelationships,
        //                           RelationshipList &outContacts
        //                           ) const;

      private:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository => (internal)
        #pragma mark

        RecursiveLock &getLock() const {return mLock;}
        IAccountForPublicationRepositoryPtr getOuter() const {return mOuter.lock();}

        String log(const char *message) const;

        void activateFetcher(IPublicationMetaDataForPublicationRepositoryPtr metaData);
        void activatePublisher(IPublicationForPublicationRepositoryPtr publication);

//        IPublicationForPublicationRepositoryPtr findPermissionDocument(const char *publicationName) const;

        /*
        IPublicationForPublicationRepositoryPtr findDocument(
                                                             const CachedPublicationMap &cache,
                                                             IPublicationMetaDataForPublicationRepositoryPtr metaData,
                                                             bool ignoreLineage = false
                                                             ) const;

        IPublicationForPublicationRepositoryPtr findDocumentFromLocalCache(
                                                                           IPublicationMetaDataPtr metaData,
                                                                           bool ignoreLineage
                                                                           ) const;
        IPublicationForPublicationRepositoryPtr findDocumentFromRemoteCache(
                                                                            IPublicationMetaDataPtr metaData,
                                                                            bool ignoreLineage
                                                                            ) const;

        IPublicationForPublicationRepositoryPtr findDocumentFromLocalCache(
                                                                           IPublicationMetaDataForPublicationRepositoryPtr metaData,
                                                                           bool ignoreLineage
                                                                           ) const;
        IPublicationForPublicationRepositoryPtr findDocumentFromRemoteCache(
                                                                            IPublicationMetaDataForPublicationRepositoryPtr metaData,
                                                                            bool ignoreLineage
                                                                            ) const;
         */

        PeerSubscriptionIncomingPtr findFinderIncomingSubscription(const char *publicationName) const;

        PeerSubscriptionIncomingPtr findPeerIncomingSubscription(
                                                                 const char *publicationName,
                                                                 const char *creatorContactID,
                                                                 const char *creatorLocationID
                                                                 ) const;

        PeerSubscriptionIncomingPtr findIncomingSubscription(IPublicationMetaDataForPublicationRepositoryPtr metaData) const;

        void onIncomingMessage(
                               IConnectionSubscriptionMessagePtr incomingMessage,
                               message::PeerPublishRequestPtr request
                               );

        void onIncomingMessage(
                               IConnectionSubscriptionMessagePtr incomingMessage,
                               message::PeerGetRequestPtr request
                               );

        void onIncomingMessage(
                               IConnectionSubscriptionMessagePtr incomingMessage,
                               message::PeerDeleteRequestPtr request
                               );

        void onIncomingMessage(
                               IConnectionSubscriptionMessagePtr incomingMessage,
                               message::PeerSubscribeRequestPtr request
                               );

        void onIncomingMessage(
                               IConnectionSubscriptionMessagePtr incomingMessage,
                               message::PeerPublishNotifyRequestPtr request
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
          typedef zsLib::Time Time;

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

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerCache => IPublicationRepositoryPeerCache
          #pragma mark

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

          void notifyFetched(IPublicationForPublicationRepositoryPtr publication);

          Time getExpires() const       {return mExpires;}
          void setExpires(Time expires) {mExpires = expires;}

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerCache => (internal)
          #pragma mark

          String log(const char *message) const;
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

        class Publisher : public zsLib::MessageQueueAssociator,
                          public IPublicationPublisher,
                          public IMessageRequesterDelegate
        {
        public:
          typedef String String;

          friend class PublicationRepository;

        protected:
          Publisher(
                    IMessageQueuePtr queue,
                    PublicationRepositoryPtr outer,
                    IPublicationPublisherDelegatePtr delegate,
                    IPublicationForPublicationRepositoryPtr publication
                    );

          void init();

        public:
          ~Publisher();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => friend PublicationRepository
          #pragma mark

          static PublisherPtr create(
                                     IMessageQueuePtr queue,
                                     PublicationRepositoryPtr outer,
                                     IPublicationPublisherDelegatePtr delegate,
                                     IPublicationForPublicationRepositoryPtr publication
                                     );

          // PUID getID() const;

          void setRequester(IMessageRequesterPtr requester);
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

          virtual bool wasSuccessful() const;

          virtual WORD getErrorResult() const;
          virtual String getErrorReason() const;

          virtual IPublicationPtr getPublication() const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => IMessageRequesterDelegate
          #pragma mark

          virtual bool handleMessageRequesterMessageReceived(
                                                             IMessageRequesterPtr requester,
                                                             message::MessagePtr message
                                                             );

          virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Publisher => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          String log(const char *message) const;

          IMessageRequesterPtr getRequester() const;

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

          IPublicationForPublicationRepositoryPtr mPublication;

          IMessageRequesterPtr mRequester;

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

        class Fetcher : public zsLib::MessageQueueAssociator,
                        public IPublicationFetcher,
                        public IMessageRequesterDelegate
        {
        public:
          typedef zsLib::String String;

          friend class PublicationRepository;

        protected:
          Fetcher(
                  IMessageQueuePtr queue,
                  PublicationRepositoryPtr outer,
                  IPublicationFetcherDelegatePtr delegate,
                  IPublicationMetaDataForPublicationRepositoryPtr metaData
                  );

          void init();

        public:
          ~Fetcher();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => friend PublicationRepository
          #pragma mark

          static FetcherPtr create(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   IPublicationFetcherDelegatePtr delegate,
                                   IPublicationMetaDataForPublicationRepositoryPtr metaData
                                   );

          void setPublication(IPublicationForPublicationRepositoryPtr publication);
          void setRequester(IMessageRequesterPtr requester);
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

          virtual bool wasSuccessful() const;

          virtual WORD getErrorResult() const;
          virtual String getErrorReason() const;

          virtual IPublicationPtr getFetchedPublication() const;

          virtual IPublicationMetaDataPtr getPublicationMetaData() const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => IMessageRequesterDelegate
          #pragma mark

          virtual bool handleMessageRequesterMessageReceived(
                                                             IMessageRequesterPtr requester,
                                                             message::MessagePtr message
                                                             );

          virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Fetcher => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          String log(const char *message) const;

          IMessageRequesterPtr getRequester() const;

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

          IPublicationMetaDataForPublicationRepositoryPtr mPublicationMetaData;

          IMessageRequesterPtr mRequester;

          bool mSucceeded;
          WORD mErrorCode;
          String mErrorReason;

          IPublicationForPublicationRepositoryPtr mFetchedPublication;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::Remover
        #pragma mark

        class Remover : public zsLib::MessageQueueAssociator,
                        public IPublicationRemover,
                        public IMessageRequesterDelegate
        {
        public:
          typedef zsLib::String String;

          friend class PublicationRepository;

        protected:
          Remover(
                  IMessageQueuePtr queue,
                  PublicationRepositoryPtr outer,
                  IPublicationRemoverDelegatePtr delegate,
                  IPublicationForPublicationRepositoryPtr publication
                  );

          void init();

        public:
          ~Remover();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => friend PublicationRepository
          #pragma mark

          static RemoverPtr create(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   IPublicationRemoverDelegatePtr delegate,
                                   IPublicationForPublicationRepositoryPtr publication
                                   );

          void setRequester(IMessageRequesterPtr requester);

          void notifyCompleted();

          // (duplicate) virtual void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => IPublicationRemover
          #pragma mark

          virtual void cancel();
          virtual bool isComplete() const;

          virtual bool wasSuccessful() const;

          virtual WORD getErrorResult() const;
          virtual String getErrorReason() const;

          virtual IPublicationPtr getPublication() const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => IMessageRequesterDelegate
          #pragma mark

          virtual bool handleMessageRequesterMessageReceived(
                                                             IMessageRequesterPtr requester,
                                                             message::MessagePtr message
                                                             );

          virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::Remover => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

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

          IPublicationForPublicationRepositoryPtr mPublication;

          IMessageRequesterPtr mRequester;

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

        class SubscriptionLocal : public zsLib::MessageQueueAssociator,
                                  public IPublicationSubscription
        {
        public:
          typedef zsLib::String String;
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

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::SubscriptionLocal => friend SubscriptionLocal
          #pragma mark

          static SubscriptionLocalPtr create(
                                             IMessageQueuePtr queue,
                                             PublicationRepositoryPtr outer,
                                             IPublicationSubscriptionDelegatePtr delegate,
                                             const char *publicationPath,
                                             const SubscribeToRelationshipsMap &relationships
                                             );

          void notifyUpdated(IPublicationForPublicationRepositoryPtr publication);
          void notifyGone(IPublicationForPublicationRepositoryPtr publication);

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

          IPublicationMetaDataForPublicationRepositoryPtr mSubscriptionInfo;
          PublicationSubscriptionStates mCurrentState;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::PeerSubscriptionIncoming
        #pragma mark

        class PeerSubscriptionIncoming : public zsLib::MessageQueueAssociator,
                                         public IMessageRequesterDelegate
        {
        public:
          typedef zsLib::String String;
          typedef IPublicationMetaData::SubscribeToRelationshipsMap SubscribeToRelationshipsMap;

          friend class PublicationRepository;

        protected:
          PeerSubscriptionIncoming(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   PeerSourcePtr peerSource,
                                   IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                   );

          void init();

        public:
          ~PeerSubscriptionIncoming();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionIncoming => friend PublicationRepository
          #pragma mark

          static PeerSubscriptionIncomingPtr create(
                                                    IMessageQueuePtr queue,
                                                    PublicationRepositoryPtr outer,
                                                    PeerSourcePtr peerSource,
                                                    IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                                    );

          // (duplicate) PUID getID() const;

          void notifyUpdated(IPublicationForPublicationRepositoryPtr publication);
          void notifyGone(IPublicationForPublicationRepositoryPtr publication);

          void notifyUpdated(const CachedPublicationMap &cachedPublications);
          void notifyGone(const CachedPublicationMap &publication);

          IPublicationMetaDataPtr getSource() const;

          void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionIncoming => IMessageRequesterDelegate
          #pragma mark

          virtual bool handleMessageRequesterMessageReceived(
                                                             IMessageRequesterPtr requester,
                                                             message::MessagePtr message
                                                             );

          virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionIncoming => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          RecursiveLock &getLock() const;
          String log(const char *message) const;

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
          IPublicationMetaDataForPublicationRepositoryPtr mSubscriptionInfo;

          typedef std::list<IMessageRequesterPtr> NotificationRequesterList;
          NotificationRequesterList mNotificationRequesters;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationRepository::PeerSubscriptionOutgoing
        #pragma mark

        class PeerSubscriptionOutgoing : public zsLib::MessageQueueAssociator,
                                         public IPublicationSubscription,
                                         public IMessageRequesterDelegate
        {
        public:
          typedef zsLib::String String;
          typedef IPublicationMetaData::SubscribeToRelationshipsMap SubscribeToRelationshipsMap;

          friend class PublicationRepository;

        protected:
          PeerSubscriptionOutgoing(
                                   IMessageQueuePtr queue,
                                   PublicationRepositoryPtr outer,
                                   IPublicationSubscriptionDelegatePtr delegate,
                                   IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                   );

          void init();

        public:
          ~PeerSubscriptionOutgoing();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => friend PublicationRepository
          #pragma mark

          static PeerSubscriptionOutgoingPtr create(
                                                    IMessageQueuePtr queue,
                                                    PublicationRepositoryPtr outer,
                                                    IPublicationSubscriptionDelegatePtr delegate,
                                                    IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                                    );

          // (duplicate) PUID getID() const;
          // (duplicate) virtual void cancel();

          // (duplicate) virtual IPublicationMetaDataPtr getSource() const;

          void setRequester(IMessageRequesterPtr requester);
          void notifyUpdated(IPublicationMetaDataForPublicationRepositoryPtr metaData);

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
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => IMessageRequesterDelegate
          #pragma mark

          virtual bool handleMessageRequesterMessageReceived(
                                                             IMessageRequesterPtr requester,
                                                             message::MessagePtr message
                                                             );

          virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PublicationRepository::PeerSubscriptionOutgoing => (internal)
          #pragma mark

          PUID getID() const {return mID;}
          RecursiveLock &getLock() const;
          String log(const char *message) const;

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

          IPublicationMetaDataForPublicationRepositoryPtr mSubscriptionInfo;

          IMessageRequesterPtr mRequester;
          IMessageRequesterPtr mCancelRequester;

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

        PUID mID;
        mutable RecursiveLock mLock;
        PublicationRepositoryWeakPtr mThisWeak;

        IAccountForPublicationRepositoryWeakPtr mOuter;

        TimerPtr mExpiresTimer;

        IConnectionSubscriptionPtr mConnectionSubscription;

        CachedPublicationMap mCachedLocalPublications;        // documents that have been published from a source tot he local repository
        CachedPublicationMap mCachedRemotePublications;       // documents that have been fetched from a remote repository

        CachedPublicationPermissionMap mCachedPermissionDocuments;

        typedef PUID SubscriptionLocationID;
        typedef std::map<SubscriptionLocationID, SubscriptionLocalPtr> SubscriptionLocalMap;
        SubscriptionLocalMap mSubscriptionsLocal;

        typedef PUID PeerSubscriptionIncomingID;
        typedef std::map<PeerSubscriptionIncomingID, PeerSubscriptionIncomingPtr> PeerSubscriptionIncomingMap;
        PeerSubscriptionIncomingMap mPeerSubscriptionsIncoming;

        typedef PUID PeerSubscriptionOutgoingID;
        typedef std::map<PeerSubscriptionOutgoingID, PeerSubscriptionOutgoingPtr> PeerSubscriptionOutgoingMap;
        PeerSubscriptionOutgoingMap mPeerSubscriptionsOutgoing;

        typedef std::list<FetcherPtr> PendingFetcherList;
        PendingFetcherList mPendingFetchers;

        typedef std::list<PublisherPtr> PendingPublisherList;
        PendingPublisherList mPendingPublishers;

        CachedPeerSourceMap mCachedPeerSources;
      };
    }
  }
}
