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

#include <hookflash/stack/internal/stack_PublicationRepository.h>
#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_Publication.h>
#include <hookflash/stack/internal/stack_PublicationMetaData.h>

#include <hookflash/stack/message/PeerPublishRequest.h>
#include <hookflash/stack/message/PeerPublishResult.h>
#include <hookflash/stack/message/PeerGetRequest.h>
#include <hookflash/stack/message/PeerGetResult.h>
#include <hookflash/stack/message/PeerDeleteRequest.h>
#include <hookflash/stack/message/PeerDeleteResult.h>
#include <hookflash/stack/message/PeerSubscribeRequest.h>
#include <hookflash/stack/message/PeerSubscribeResult.h>
#include <hookflash/stack/message/PeerPublishNotifyRequest.h>
#include <hookflash/stack/message/PeerPublishNotifyResult.h>

#include <hookflash/stack/message/MessageResult.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>

#include <algorithm>

#define HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS (60)
#define HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRES_TIMER_IN_SECONDS (60)

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
// HERE - DO NOT LEAVE THIS SET TO THE TEMP FOR PRODUCTION!
//#define HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRE_DISCONNECTED_REMOTE_PUBLICATIONS_IN_SECONDS (60)  // temp expire in 1 minute
#define HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRE_DISCONNECTED_REMOTE_PUBLICATIONS_IN_SECONDS (2*(60*60))  // expire in 2 hrs


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::WORD WORD;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::String String;
      typedef zsLib::Seconds Seconds;
      typedef zsLib::CSTR CSTR;
      typedef zsLib::Time Time;
      typedef zsLib::Timer Timer;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef IPublication::AutoRecursiveLockPtr AutoRecursiveLockPtr;
      typedef PublicationRepository::PublisherPtr PublisherPtr;
      typedef PublicationRepository::FetcherPtr FetcherPtr;
      typedef PublicationRepository::RemoverPtr RemoverPtr;
      typedef PublicationRepository::SubscriptionLocalPtr SubscriptionLocalPtr;
      typedef PublicationRepository::PeerSubscriptionIncomingPtr PeerSubscriptionIncomingPtr;
      typedef PublicationRepository::PeerSubscriptionOutgoingPtr PeerSubscriptionOutgoingPtr;
      typedef PublicationRepository::PeerCachePtr PeerCachePtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      static IPublicationMetaData::Sources toSource(IConnectionSubscriptionMessage::Sources source)
      {
        switch (source)
        {
          case IConnectionSubscriptionMessage::Source_Finder: return IPublicationMetaData::Source_Finder;
          case IConnectionSubscriptionMessage::Source_Peer:   return IPublicationMetaData::Source_Peer;
        }
        return IPublicationMetaData::Source_Local;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationRepositoryForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationRepositoryForAccountPtr IPublicationRepositoryForAccount::create(IAccountForPublicationRepositoryPtr outer)
      {
        return PublicationRepository::create(outer);
      }

      IPublicationRepositoryForAccountPtr IPublicationRepositoryForAccount::convert(IPublicationRepositoryPtr repository)
      {
        return boost::dynamic_pointer_cast<IPublicationRepositoryForAccount>(repository);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::CacheCompare
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::CacheCompare::operator()(const IPublicationMetaDataForPublicationRepositoryPtr &x, const IPublicationMetaDataForPublicationRepositoryPtr &y) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!x)
        ZS_THROW_INVALID_ARGUMENT_IF(!y)
        return x->isLessThan(y);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::PublicationRepository(
                                                   IMessageQueuePtr queue,
                                                   IAccountForPublicationRepositoryPtr outer
                                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer)
      {
        ZS_LOG_DEBUG(log("constructor"))
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::init()
      {
        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        ZS_THROW_BAD_STATE_IF(!outer)

        IAccountPtr account = outer->convertIAccount();
        ZS_THROW_BAD_STATE_IF(!account)

        mConnectionSubscription = account->subscribeToAllConnections(mThisWeak.lock());

        mExpiresTimer = Timer::create(mThisWeak.lock(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRES_TIMER_IN_SECONDS));

        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      PublicationRepository::~PublicationRepository()
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
      #pragma mark PublicationRepository => IPublicationRepositoryForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepositoryPtr PublicationRepository::create(IAccountForPublicationRepositoryPtr outer)
      {
        PublicationRepositoryPtr pThis(new PublicationRepository(outer->getAssociatedMessageQueue(), outer));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => IPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationPublisherPtr PublicationRepository::publish(
                                                              IPublicationPublisherDelegatePtr delegate,
                                                              IPublicationPtr inPublication
                                                              )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!inPublication)

        AutoRecursiveLock lock(getLock());

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot publish document as account object is gone"))
          return IPublicationPublisherPtr();
        }

        IPublicationForPublicationRepositoryPtr publication = IPublicationForPublicationRepository::convert(inPublication);

        PublisherPtr publisher = Publisher::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, publication);

        if (IPublicationMetaData::Source_Local == publication->getSource()) {
          ZS_LOG_DEBUG(log("publicaton is local thus can publish immediately") + publication->getDebugValuesString())

          publication->setPublishedToContact(outer->getContactID(), outer->getLocationID());
          publication->setBaseVersion(inPublication->getVersion());

          // scope: remove old cached publication
          {
            CachedPublicationMap::iterator found = mCachedLocalPublications.find(publication);
            if (found != mCachedLocalPublications.end()) {
              ZS_LOG_DEBUG(log("previous publication found thus removing old map entry"))
              mCachedLocalPublications.erase(found);
            }
          }
          // scope: remove old permission publication
          {
            CachedPublicationPermissionMap::iterator found = mCachedPermissionDocuments.find(publication->getName());
            if (found != mCachedPermissionDocuments.end()) {
              ZS_LOG_DEBUG(log("previous permission publication found thus removing old map entry"))
              mCachedPermissionDocuments.erase(found);
            }
          }

          mCachedLocalPublications[publication] = publication;
          mCachedPermissionDocuments[publication->getName()] = publication;

          ZS_LOG_DEBUG(log("publication inserted into local cache") + ", local cache total=" + Stringize<size_t>(mCachedLocalPublications.size()).string() + ", local permissions total=" + Stringize<size_t>(mCachedPermissionDocuments.size()).string())

          publisher->notifyCompleted();

          ZS_LOG_DEBUG(log("notifying local subscribers of publish") + ", total=" + Stringize<size_t>(mSubscriptionsLocal.size()).string())

          // notify subscriptions about updated publication
          for (SubscriptionLocalMap::iterator iter = mSubscriptionsLocal.begin(); iter != mSubscriptionsLocal.end(); ++iter) {
            ZS_LOG_TRACE(log("notifying local subscription of publish") + ", subscriber ID=" + Stringize<PUID>((*iter).first).string())
            (*iter).second->notifyUpdated(publication);
          }

          ZS_LOG_DEBUG(log("notifying incoming subscribers of publish") + ", total=" + Stringize<size_t>(mPeerSubscriptionsIncoming.size()).string())

          for (PeerSubscriptionIncomingMap::iterator iter = mPeerSubscriptionsIncoming.begin(); iter != mPeerSubscriptionsIncoming.end(); ++iter) {
            ZS_LOG_TRACE(log("notifying peer subscription of publish") + ", subscriber ID=" + Stringize<PUID>((*iter).first).string())
            (*iter).second->notifyUpdated(publication);
          }

          return publisher;
        }

        ZS_LOG_DEBUG(log("publication requires publishing to an external source") + publication->getDebugValuesString())

        mPendingPublishers.push_back(publisher);
        activatePublisher(publication);

        return publisher;
      }

      //-----------------------------------------------------------------------
      IPublicationFetcherPtr PublicationRepository::fetch(
                                                          IPublicationFetcherDelegatePtr delegate,
                                                          IPublicationMetaDataPtr inMetaData
                                                          )


      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!inMetaData)

        AutoRecursiveLock lock(getLock());

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(inMetaData);

        ZS_LOG_DEBUG(log("requesting to fetch publication") + metaData->getDebugValuesString())

        FetcherPtr fetcher = Fetcher::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, metaData);

        if (IPublicationMetaData::Source_Local == metaData->getSource()) {
          CachedPublicationMap::iterator found = mCachedLocalPublications.find(metaData);
          if (found != mCachedLocalPublications.end()) {
            IPublicationForPublicationRepositoryPtr existingPublication = (*found).second;
            ZS_LOG_WARNING(Detail, log("local publication was found"))

            fetcher->setPublication(existingPublication);
            fetcher->notifyCompleted();
          } else {
            ZS_LOG_DEBUG(log("local publication was not found"))

            fetcher->cancel();
          }
          return fetcher;
        }

        ZS_LOG_DEBUG(log("will push fetch request to back of fetching list"))

        mPendingFetchers.push_back(fetcher);

        activateFetcher(metaData);
        return fetcher;
      }

      //-----------------------------------------------------------------------
      IPublicationRemoverPtr PublicationRepository::remove(
                                                           IPublicationRemoverDelegatePtr delegate,
                                                           IPublicationPtr inPublication
                                                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!inPublication)

        AutoRecursiveLock lock(getLock());

        IPublicationForPublicationRepositoryPtr publication = IPublicationForPublicationRepository::convert(inPublication);

        ZS_LOG_DEBUG(log("requesting to remove publication") + publication->getDebugValuesString())

        RemoverPtr remover = Remover::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, publication);

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          remover->cancel();  // sorry, this could not be completed...
          return remover;
        }

        switch (publication->getSource()) {
          case IPublicationMetaData::Source_Local: {

            bool wasErased = false;

            // erase from local cache
            {
              CachedPublicationMap::iterator found = mCachedLocalPublications.find(publication);
              if (found != mCachedLocalPublications.end()) {

                ZS_LOG_DEBUG(log("found publication to erase and removing now"))

                // found the document to be erased
                mCachedLocalPublications.erase(found);
                wasErased = true;
              } else {
                ZS_LOG_WARNING(Detail, log("publication was not found in local cache"))
              }
            }

            // find in permission cache
            {
              CachedPublicationPermissionMap::iterator found = mCachedPermissionDocuments.find(publication->getName());
              if (found != mCachedPermissionDocuments.end()) {
                IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;

                if (existingPublication->getLineage() == publication->getLineage()) {
                  ZS_LOG_DEBUG(log("found permission publication to erase and removing now"))
                  mCachedPermissionDocuments.erase(found);
                  wasErased = true;
                } else {
                  ZS_LOG_DEBUG(log("found permission publication but it doesn't have the same lineage thus will not erase") + existingPublication->getDebugValuesString())
                }
              } else {
                ZS_LOG_DEBUG(log("did not find permisison document to remove"))
              }
            }

            //*****************************************************************
            //*****************************************************************
            //*****************************************************************
            //*****************************************************************
            // HERE - notify subscribers the document is gone?

            if (wasErased) {
              remover->notifyCompleted();
            }
            remover->cancel();
          }
          case IPublicationMetaData::Source_Finder: {
            message::PeerDeleteRequestPtr message = message::PeerDeleteRequest::create();
            message->publicationMetaData(publication->convertIPublication());

            ZS_LOG_DEBUG(log("requesting to remove remote finder publication"))

            remover->setRequester(outer->sendFinderRequest(remover, message, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
            break;
          }
          case IPublicationMetaData::Source_Peer: {
            message::PeerPublishRequestPtr message = message::PeerPublishRequest::create();
            message->publication(publication->convertIPublication());

            ZS_LOG_DEBUG(log("requesting to remove remote peer publication"))

            remover->setRequester(outer->sendPeerRequest(remover, message, publication->getPublishedToContactID(), publication->getPublishedToLocationID(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
            break;
          }
        }

        return remover;
      }

      //-----------------------------------------------------------------------
      IPublicationSubscriptionPtr PublicationRepository::subscribeLocal(
                                                                        IPublicationSubscriptionDelegatePtr delegate,
                                                                        const char *publicationPath,
                                                                        const SubscribeToRelationshipsMap &relationships
                                                                        )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!publicationPath)

        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("creating location subcription") + ", publication path=" + Stringize<CSTR>(publicationPath).string())

        SubscriptionLocalPtr subscriber = SubscriptionLocal::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, publicationPath, relationships);

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          subscriber->cancel();  // sorry, this could not be completed...
          return subscriber;
        }

        for (CachedPublicationMap::iterator iter = mCachedLocalPublications.begin(); iter != mCachedLocalPublications.end(); ++iter)
        {
          IPublicationForPublicationRepositoryPtr publication = (*iter).second;
          ZS_LOG_TRACE(log("notifying location subcription about document") + publication->getDebugValuesString())
          subscriber->notifyUpdated(publication);
        }

        return subscriber;
      }

      //-----------------------------------------------------------------------
      IPublicationSubscriptionPtr PublicationRepository::subscribeFinder(
                                                                         IPublicationSubscriptionDelegatePtr delegate,
                                                                         const char *publicationPath,
                                                                         const SubscribeToRelationshipsMap &relationships
                                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!publicationPath)

        AutoRecursiveLock lock(getLock());

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot subscribe finder as account object is gone"))
          return IPublicationSubscriptionPtr();
        }

        ZS_LOG_DEBUG(log("creating finder subcription") + ", publication path=" + Stringize<CSTR>(publicationPath).string())

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::create(0, 0, 0, IPublicationMetaData::Source_Finder, outer->getContactID(), outer->getLocationID(), publicationPath, "", IPublicationMetaData::Encoding_XML, relationships);
        PeerSubscriptionOutgoingPtr subscriber = PeerSubscriptionOutgoing::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, metaData);

        message::PeerSubscribeRequestPtr message = message::PeerSubscribeRequest::create();

        message->publicationMetaData(metaData->convertIPublicationMetaData());

        subscriber->setRequester(outer->sendFinderRequest(subscriber, message, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));

        mPeerSubscriptionsOutgoing[subscriber->getID()] = subscriber;
        return subscriber;
      }

      //-----------------------------------------------------------------------
      IPublicationSubscriptionPtr PublicationRepository::subscribePeer(
                                                                       IPublicationSubscriptionDelegatePtr delegate,
                                                                       const char *publicationPath,
                                                                       const SubscribeToRelationshipsMap &relationships,
                                                                       const char *peerSourceContactID,
                                                                       const char *peerSourceLocationID
                                                                       )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!publicationPath)
        ZS_THROW_INVALID_ARGUMENT_IF(!peerSourceContactID)
        ZS_THROW_INVALID_ARGUMENT_IF(!peerSourceLocationID)

        AutoRecursiveLock lock(getLock());

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot subscribe peer as account object is gone"))
          return IPublicationSubscriptionPtr();
        }

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::create(0, 0, 0, IPublicationMetaData::Source_Peer, outer->getContactID(), outer->getLocationID(), publicationPath, "", IPublicationMetaData::Encoding_XML, relationships, peerSourceContactID, peerSourceLocationID);

        ZS_LOG_DEBUG(log("creating peer subcription") + ", publication path=" + Stringize<CSTR>(publicationPath).string() + ", peer contact ID=" + Stringize<CSTR>(peerSourceContactID).string() + ", peer location ID=" + Stringize<CSTR>(peerSourceLocationID).string())

        PeerSubscriptionOutgoingPtr subscriber = PeerSubscriptionOutgoing::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, metaData);

        message::PeerSubscribeRequestPtr message = message::PeerSubscribeRequest::create();

        message->publicationMetaData(metaData->convertIPublicationMetaData());

        subscriber->setRequester(outer->sendPeerRequest(subscriber, message, peerSourceContactID, peerSourceLocationID, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));

        mPeerSubscriptionsOutgoing[subscriber->getID()] = subscriber;
        ZS_LOG_TRACE(log("outgoing subscription is created"))
        return subscriber;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => IConnectionSubscriptionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::onConnectionSubscriptionShutdown(IConnectionSubscriptionPtr subscription)
      {
        AutoRecursiveLock lock(getLock());

        if (subscription != mConnectionSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring connection subscription shutdown on obsolete subscription"))
          return;
        }

        ZS_LOG_DEBUG(log("connection subscription shutdown"))
        mConnectionSubscription.reset();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onConnectionSubscriptionFinderConnectionStateChanged(
                                                                                       IConnectionSubscriptionPtr subscription,
                                                                                       ConnectionStates state
                                                                                       )
      {
        AutoRecursiveLock lock(getLock());
        if (subscription != mConnectionSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring connection subscription finder state change on obsolete subscription"))
          return;
        }

        ZS_LOG_DEBUG(log("finder connection state changed") + ", state=" + IConnectionSubscription::toString(state))

        Time recommendedExpires = zsLib::now() + Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRE_DISCONNECTED_REMOTE_PUBLICATIONS_IN_SECONDS);

        switch (state) {
          case IConnectionSubscription::ConnectionState_Pending:        break;
          case IConnectionSubscription::ConnectionState_Connected:      {

            // removing remote publications expiry since the finder has reconnected
            for (CachedPublicationMap::iterator iter = mCachedRemotePublications.begin(); iter != mCachedRemotePublications.end(); ++iter)
            {
              IPublicationForPublicationRepositoryPtr &publication = (*iter).second;

              if (IPublicationMetaData::Source_Finder == publication->getSource()) {
                ZS_LOG_TRACE(log("removing expiry time on document") + ", recommend=" + Stringize<Time>(recommendedExpires).string() + publication->getDebugValuesString())
                publication->setCacheExpires(Time());
              }
            }

            // remove the finder source expiry
            {
              PeerSourcePtr peerSource = IPublicationMetaDataForPublicationRepository::createFinderSource();
              CachedPeerSourceMap::iterator found = mCachedPeerSources.find(peerSource);
              if (found != mCachedPeerSources.end()) {
                PeerCachePtr &peerCache = (*found).second;

                ZS_LOG_DEBUG(log("removing the finder source expiry"))
                peerCache->setExpires(Time());
              }
            }
            break;
          }
          case IConnectionSubscription::ConnectionState_Disconnecting:
          case IConnectionSubscription::ConnectionState_Disconnected:   {

            // set remote publications to expire since this peer has disconnected
            for (CachedPublicationMap::iterator iter = mCachedRemotePublications.begin(); iter != mCachedRemotePublications.end(); ++iter)
            {
              IPublicationForPublicationRepositoryPtr &publication = (*iter).second;

              if (IPublicationMetaData::Source_Finder == publication->getSource()) {
                ZS_LOG_TRACE(log("setting expiry time on document") + ", recommend=" + Stringize<Time>(recommendedExpires).string() + publication->getDebugValuesString())
                publication->setCacheExpires(recommendedExpires);
              }
            }

            // set the finder source expiry
            {
              PeerSourcePtr peerSource = IPublicationMetaDataForPublicationRepository::createFinderSource();
              CachedPeerSourceMap::iterator found = mCachedPeerSources.find(peerSource);
              if (found != mCachedPeerSources.end()) {
                PeerCachePtr &peerCache = (*found).second;

                ZS_LOG_DEBUG(log("setting the finder source to expire at the recommended time") + ", recommended=" + Stringize<Time>(recommendedExpires).string())
                peerCache->setExpires(recommendedExpires);
              }
            }

            // clean all outgoing subscriptions going to the finder...
            for (PeerSubscriptionOutgoingMap::iterator subIter = mPeerSubscriptionsOutgoing.begin(); subIter != mPeerSubscriptionsOutgoing.end(); )
            {
              PeerSubscriptionOutgoingMap::iterator current = subIter;
              ++subIter;

              PeerSubscriptionOutgoingPtr &outgoing = (*current).second;
              IPublicationMetaDataPtr source = outgoing->getSource();
              if (IPublicationMetaData::Source_Finder == source->getSource()) {
                ZS_LOG_DEBUG(log("shutting down outgoing finder subscription") + ", id=" + Stringize<PUID>(outgoing->getID()).string())
                // cancel this subscription since its no longer valid
                outgoing->cancel();
                mPeerSubscriptionsOutgoing.erase(current);
              }
            }

            // clean all incoming subscriptions coming from the finder...
            for (PeerSubscriptionIncomingMap::iterator subIter = mPeerSubscriptionsIncoming.begin(); subIter != mPeerSubscriptionsIncoming.end(); )
            {
              PeerSubscriptionIncomingMap::iterator current = subIter;
              ++subIter;

              PeerSubscriptionIncomingPtr &incoming = (*current).second;
              IPublicationMetaDataPtr source = incoming->getSource();
              if (IPublicationMetaData::Source_Finder == source->getSource()) {
                ZS_LOG_DEBUG(log("shutting down incoming subscriptions coming from the finder") + ", id=" + Stringize<PUID>(incoming->getID()).string())

                // cancel this subscription since its no longer valid
                incoming->cancel();
                mPeerSubscriptionsIncoming.erase(current);
              }
            }
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onConnectionSubscriptionPeerLocationConnectionStateChanged(
                                                                                             IConnectionSubscriptionPtr subscription,
                                                                                             IPeerLocationPtr location,
                                                                                             ConnectionStates state
                                                                                             )
      {
        AutoRecursiveLock lock(getLock());
        if (subscription != mConnectionSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring connection subscription peer location state change on obsolete subscription"))
          return;
        }

        ZS_LOG_DEBUG(log("peer connection state changed") + ", state=" + IConnectionSubscription::toString(state) + ", peer contact ID=" + location->getContactID() + ", peer location ID=" + location->getLocationID())

        Time recommendedExpires = zsLib::now() + Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRE_DISCONNECTED_REMOTE_PUBLICATIONS_IN_SECONDS);

        switch (state) {
          case IConnectionSubscription::ConnectionState_Pending:        break;
          case IConnectionSubscription::ConnectionState_Connected:      {
            // remove every remote document expiry for the peer...
            for (CachedPublicationMap::iterator iter = mCachedRemotePublications.begin(); iter != mCachedRemotePublications.end(); ++iter)
            {
              IPublicationForPublicationRepositoryPtr &publication = (*iter).second;

              if (IPublicationMetaData::Source_Peer == publication->getSource()) {
                if ((publication->getPublishedToContactID() == location->getContactID()) &&
                    (publication->getPublishedToLocationID() == location->getLocationID())) {
                  ZS_LOG_TRACE(log("removing expiry time on document") + ", recommend=" + Stringize<Time>(recommendedExpires).string() + publication->getDebugValuesString())
                  publication->setExpires(Time());
                }
              }
            }

            // remove the peer source expiry
            {
              PeerSourcePtr peerSource = IPublicationMetaDataForPublicationRepository::createPeerSource(location->getContactID(), location->getLocationID());
              CachedPeerSourceMap::iterator found = mCachedPeerSources.find(peerSource);
              if (found != mCachedPeerSources.end()) {
                PeerCachePtr &peerCache = (*found).second;

                ZS_LOG_DEBUG(log("setting the peer source to expire at the recommended time") + ", recommended=" + Stringize<Time>(recommendedExpires).string())
                peerCache->setExpires(recommendedExpires);
              }
            }

            break;
          }
          case IConnectionSubscription::ConnectionState_Disconnecting:
          case IConnectionSubscription::ConnectionState_Disconnected:   {

            // clean the cache of everything having to do with the peer...
            for (CachedPublicationMap::iterator pubIter = mCachedLocalPublications.begin(); pubIter != mCachedLocalPublications.end(); )
            {
              CachedPublicationMap::iterator current = pubIter;
              ++pubIter;

              IPublicationForPublicationRepositoryPtr &publication = (*current).second;

              if (IPublicationMetaData::Source_Peer == publication->getSource()) {
                if ((publication->getPublishedToContactID() == location->getContactID()) &&
                    (publication->getPublishedToLocationID() == location->getLocationID())) {
                  ZS_LOG_DEBUG(log("removing publication published to peer") + publication->getDebugValuesString())

                  // remove this document from the cache
                  mCachedLocalPublications.erase(current);
                }
              }
            }

            // set every remote document to expire after a period of time for the peer...
            for (CachedPublicationMap::iterator iter = mCachedRemotePublications.begin(); iter != mCachedRemotePublications.end(); ++iter)
            {
              IPublicationForPublicationRepositoryPtr &publication = (*iter).second;

              if (IPublicationMetaData::Source_Peer == publication->getSource()) {
                if ((publication->getPublishedToContactID() == location->getContactID()) &&
                    (publication->getPublishedToLocationID() == location->getLocationID())) {
                  ZS_LOG_TRACE(log("setting expiry time on document") + ", recommend=" + Stringize<Time>(recommendedExpires).string() + publication->getDebugValuesString())
                  publication->setExpires(recommendedExpires);
                }
              }
            }

            // set the peer source expiry
            {
              PeerSourcePtr peerSource = IPublicationMetaDataForPublicationRepository::createPeerSource(location->getContactID(), location->getLocationID());
              CachedPeerSourceMap::iterator found = mCachedPeerSources.find(peerSource);
              if (found != mCachedPeerSources.end()) {
                PeerCachePtr &peerCache = (*found).second;

                ZS_LOG_DEBUG(log("setting the peer source to expire at the recommended time") + ", recommended=" + Stringize<Time>(recommendedExpires).string())
                peerCache->setExpires(recommendedExpires);
              }
            }

            // clean all outgoing subscriptions going to the peer...
            for (PeerSubscriptionOutgoingMap::iterator subIter = mPeerSubscriptionsOutgoing.begin(); subIter != mPeerSubscriptionsOutgoing.end(); )
            {
              PeerSubscriptionOutgoingMap::iterator current = subIter;
              ++subIter;

              PeerSubscriptionOutgoingPtr &outgoing = (*current).second;
              IPublicationMetaDataPtr source = outgoing->getSource();
              if (IPublicationMetaData::Source_Peer == source->getSource()) {
                if ((source->getPublishedToContactID() == location->getContactID()) &&
                    (source->getPublishedToLocationID() == location->getLocationID())) {
                  // cancel this subscription since its no longer valid
                  ZS_LOG_DEBUG(log("shutting down outgoing peer subscription") + ", id=" + Stringize<PUID>(outgoing->getID()).string())

                  outgoing->cancel();
                  mPeerSubscriptionsOutgoing.erase(current);
                }
              }
            }

            // clean all incoming subscriptions coming from the finder...
            for (PeerSubscriptionIncomingMap::iterator subIter = mPeerSubscriptionsIncoming.begin(); subIter != mPeerSubscriptionsIncoming.end(); )
            {
              PeerSubscriptionIncomingMap::iterator current = subIter;
              ++subIter;

              PeerSubscriptionIncomingPtr &incoming = (*current).second;
              IPublicationMetaDataPtr source = incoming->getSource();
              if (IPublicationMetaData::Source_Peer == source->getSource()) {
                if ((source->getCreatorContactID() == location->getContactID()) &&
                    (source->getCreatorLocationID() == location->getLocationID())) {
                  // cancel this subscription since its no longer valid
                  ZS_LOG_DEBUG(log("shutting down incoming subscriptions coming from the peer") + ", id=" + Stringize<PUID>(incoming->getID()).string())
                  incoming->cancel();
                  mPeerSubscriptionsIncoming.erase(current);
                }
              }
            }
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onConnectionSubscriptionIncomingMessage(
                                                                          IConnectionSubscriptionPtr subscription,
                                                                          IConnectionSubscriptionMessagePtr incomingMessage
                                                                          )
      {
        ZS_LOG_TRACE(log("received notification of incoming message") + ", source=" + IConnectionSubscriptionMessage::toString(incomingMessage->getSource()) + ", contact ID=" + incomingMessage->getPeerContactID() + ", location ID=" + incomingMessage->getPeerLocationID())

        AutoRecursiveLock lock(getLock());
        if (subscription != mConnectionSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring connection subscription message on obsolete subscription"))
          return;
        }

        message::MessagePtr message = incomingMessage->getMessage();

        ZS_LOG_TRACE(log("incoming message received") + ", type=" + message::Message::toString(message->messageType()) + ", method=" + message->methodAsString())

        switch (message->messageType()) {
          case message::Message::MessageType_Request:
          case message::Message::MessageType_Notify:  break;
          default:                                    {
            ZS_LOG_WARNING(Detail, log("ignoring message that is neither a request nor a notification"))
            return;
          }
        }

        switch ((message::MessageFactoryStack::Methods)message->method()) {
          case message::MessageFactoryStack::Method_PeerPublish:        onIncomingMessage(incomingMessage, message::PeerPublishRequest::convert(message)); break;
          case message::MessageFactoryStack::Method_PeerGet:            onIncomingMessage(incomingMessage, message::PeerGetRequest::convert(message)); break;
          case message::MessageFactoryStack::Method_PeerDelete:         onIncomingMessage(incomingMessage, message::PeerDeleteRequest::convert(message)); break;
          case message::MessageFactoryStack::Method_PeerSubscribe:      onIncomingMessage(incomingMessage, message::PeerSubscribeRequest::convert(message)); break;
          case message::MessageFactoryStack::Method_PeerPublishNotify:  onIncomingMessage(incomingMessage, message::PeerPublishNotifyRequest::convert(message)); break;
          default:                                          {
            ZS_LOG_TRACE(log("method was not understood (thus ignoring)"))
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::onTimer(TimerPtr timer)
      {
        ZS_LOG_TRACE(log("tick"))

        AutoRecursiveLock lock(getLock());
        if (timer != mExpiresTimer) {
          ZS_LOG_WARNING(Detail, log("received notification of an obsolete timer"))
          return;
        }

        Time tick = zsLib::now();

        // go through the remote cache and expire the documents now...
        for (CachedPublicationMap::iterator cacheIter = mCachedRemotePublications.begin(); cacheIter != mCachedRemotePublications.end(); )
        {
          CachedPublicationMap::iterator current = cacheIter;
          ++cacheIter;

          IPublicationForPublicationRepositoryPtr &publication = (*current).second;

          Time expires = publication->getExpires();
          Time cacheExpires = publication->getCacheExpires();

          if (Time() == expires) {
            expires = cacheExpires;
          }
          if (Time() == cacheExpires) {
            cacheExpires = expires;
          }

          if (Time() == expires) {
            ZS_LOG_TRACE(log("publication does not have an expiry") + publication->getDebugValuesString())
            continue;
          }

          if (cacheExpires < expires)
            expires = cacheExpires;

          if (expires < tick) {
            ZS_LOG_DEBUG(log("document is now expiring") + publication->getDebugValuesString())
            mCachedRemotePublications.erase(current);
            continue;
          }

          ZS_LOG_TRACE(log("publication is not expirying yet") + publication->getDebugValuesString())
        }

        // go through the peer sources and see if any should expire...
        for (CachedPeerSourceMap::iterator cacheIter = mCachedPeerSources.begin(); cacheIter != mCachedPeerSources.end(); )
        {
          CachedPeerSourceMap::iterator current = cacheIter;
          ++cacheIter;

          const PeerSourcePtr &peerSource = (*current).first;
          PeerCachePtr peerCache = (*current).second;

          Time expires = peerCache->getExpires();
          if (Time() == expires) {
            ZS_LOG_TRACE(log("peer source does not have an expiry") + peerSource->getDebugValuesString())
            continue;
          }

          if (expires < tick) {
            ZS_LOG_DEBUG(log("peer source is now expiring") + peerSource->getDebugValuesString())
            mCachedPeerSources.erase(current);
            continue;
          }

          ZS_LOG_TRACE(log("peer source is not expirying yet") + peerSource->getDebugValuesString())
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => friend Publisher
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyPublished(PublisherPtr publisher)
      {
        AutoRecursiveLock lock(getLock());
        // nothing special to do (although may decide to cache the document published in the future to prevent re-fetching)
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyPublisherCancelled(PublisherPtr publisher)
      {
        AutoRecursiveLock lock(getLock());

        IPublicationForPublicationRepositoryPtr publication;

        // find and remove the fetcher from the pending list...
        for (PendingPublisherList::iterator iter = mPendingPublishers.begin(); iter != mPendingPublishers.end(); ++iter)
        {
          PublisherPtr &foundPublisher = (*iter);
          if (foundPublisher->getID() == publisher->getID()) {
            publication = IPublicationForPublicationRepository::convert(foundPublisher->getPublication());
            ZS_LOG_DEBUG(log("removing remote publisher") + ", publisher ID=" + Stringize<PUID>(publisher->getID()).string() + publication->getDebugValuesString())
            mPendingPublishers.erase(iter);
            break;
          }
        }

        if (publication) {
          // ensure that only one fectcher for the same publication is activated at a time...
          activatePublisher(publication);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => friend Fetcher
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyFetched(FetcherPtr fetcher)
      {
        AutoRecursiveLock lock(getLock());

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::convert(fetcher->getPublicationMetaData());

        ZS_LOG_DEBUG(log("publication was fetched") + ", fetcher ID=" + Stringize<PUID>(fetcher->getID()).string() + metaData->getDebugValuesString())

        IPublicationForPublicationRepositoryPtr publication = IPublicationForPublicationRepository::convert(fetcher->getFetchedPublication());

        publication->setCreatorContact(metaData->getCreatorContactID(), metaData->getCreatorLocationID());
        publication->setSource(metaData->getSource());
        if (IPublicationMetaData::Source_Peer == publication->getSource()) {
          publication->setPublishedToContact(metaData->getPublishedToContactID(), metaData->getPublishedToLocationID());
        }

        CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);
        if (found != mCachedRemotePublications.end()) {
          IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;

          ZS_LOG_DEBUG(log("existing internal publication found thus updating") + existingPublication->getDebugValuesString())
          try {
            existingPublication->updateFromFetchedPublication(publication);

            // override what the fetcher thinks is returned and replace with the existing document
            fetcher->setPublication(existingPublication);
          } catch(IPublicationForPublicationRepository::Exceptions::VersionMismatch &) {
            ZS_LOG_ERROR(Detail, log("version fetched is not compatible with the version already known"))
          }
        } else {
          ZS_LOG_DEBUG(log("new entry for cache will be created since existing publication in cache was not found") + publication->getDebugValuesString())
          mCachedRemotePublications[publication] = publication;

          ZS_LOG_DEBUG(log("publication inserted into remote cache") + ", remote cache total=" + Stringize<size_t>(mCachedRemotePublications.size()).string())
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyFetcherCancelled(FetcherPtr fetcher)
      {
        AutoRecursiveLock lock(getLock());

        IPublicationMetaDataForPublicationRepositoryPtr metaData;

        // find and remove the fetcher from the pending list...
        for (PendingFetcherList::iterator iter = mPendingFetchers.begin(); iter != mPendingFetchers.end(); ++iter)
        {
          FetcherPtr &foundFetcher = (*iter);
          if (foundFetcher->getID() == fetcher->getID()) {
            ZS_LOG_DEBUG(log("fetcher is being removed from pending fetchers list") + ", fetcher ID=" + Stringize<PUID>(fetcher->getID()).string())
            metaData = IPublicationMetaDataForPublicationRepository::convert(fetcher->getPublicationMetaData());
            mPendingFetchers.erase(iter);
            break;
          }
        }

        if (metaData) {
          ZS_LOG_DEBUG(log("will attempt to activate next fetcher based on previous fetch's publication meta data") + metaData->getDebugValuesString())
          // ensure that only one fectcher for the same publication is activated at a time...
          activateFetcher(metaData);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => friend Remover
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyRemoved(RemoverPtr remover)
      {
        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::convert(remover->getPublication());
        CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);
        if (found == mCachedRemotePublications.end()) {
          ZS_LOG_DEBUG(log("unable to locate publication in 'remote' cache") + metaData->getDebugValuesString())
          return;
        }

        IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;

        ZS_LOG_DEBUG(log("removing remotely cached publication") + existingPublication->getDebugValuesString())
        mCachedRemotePublications.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => friend PeerSubscriptionOutgoing
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::notifySubscribed(PeerSubscriptionOutgoingPtr subscriber)
      {
        ZS_LOG_DEBUG(log("notify outoing subscription is subscribed") + ", subscriber ID=" + Stringize<PUID>(subscriber->getID()).string())
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyPeerOutgoingSubscriptionShutdown(PeerSubscriptionOutgoingPtr subscription)
      {
        PeerSubscriptionOutgoingMap::iterator found = mPeerSubscriptionsOutgoing.find(subscription->getID());
        if (found == mPeerSubscriptionsOutgoing.end()) {
          ZS_LOG_WARNING(Detail, log("outoing subscription was shutdown but it was not found in local subscription map") + ", subscription ID=" + Stringize<PUID>(subscription->getID()).string())
          return;
        }

        ZS_LOG_DEBUG(log("outgoing subscription was shutdown") + Stringize<PUID>(subscription->getID()).string())
        mPeerSubscriptionsOutgoing.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => friend SubscriptionLocal
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyLocalSubscriptionShutdown(SubscriptionLocalPtr subscription)
      {
        SubscriptionLocalMap::iterator found = mSubscriptionsLocal.find(subscription->getID());
        if (found == mSubscriptionsLocal.end()) {
          ZS_LOG_WARNING(Detail, log("local subscription was shutdown but it was not found in local subscription map") + ", subscription ID=" + Stringize<PUID>(subscription->getID()).string())
          return;
        }

        ZS_LOG_DEBUG(log("local subscription was shutdown") + Stringize<PUID>(subscription->getID()).string())
        mSubscriptionsLocal.erase(found);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::resolveRelationships(
                                                       const PublishToRelationshipsMap &publishToRelationships,
                                                       RelationshipList &outContacts
                                                       ) const
      {
        typedef String ContactID;
        typedef std::map<ContactID, ContactID> ContactMap;

        typedef IPublicationMetaData::DocumentName DocumentName;
        typedef IPublicationMetaData::PermissionAndContactIDListPair PermissionAndContactIDListPair;
        typedef IPublicationMetaData::ContactIDList ContactIDList;

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot resolve relationships as account object is gone"))
          return;
        }

        ContactMap contacts;

        for (PublishToRelationshipsMap::const_iterator iter = publishToRelationships.begin(); iter != publishToRelationships.end(); ++iter)
        {
          DocumentName name = (*iter).first;

          ZS_LOG_TRACE(log("resolving relationships for document") + ", name=" + name)

          const PermissionAndContactIDListPair &permissionPair = (*iter).second;
          const ContactIDList &diffContacts = permissionPair.second;

          IPublicationForPublicationRepositoryPtr relationshipsPublication;

          // scope: find the permission document
          {
            CachedPublicationPermissionMap::const_iterator found = mCachedPermissionDocuments.find(name);
            if (found != mCachedPermissionDocuments.end()) {
              relationshipsPublication = (*found).second;
            }
          }

          if (!relationshipsPublication) {
            ZS_LOG_WARNING(Detail, log("failed to find relationships document for resolving") + ", name=" + name)
            continue;
          }

          RelationshipList docContacts;
          relationshipsPublication->getAsContactList(docContacts);

          switch (permissionPair.first) {
            case IPublicationMetaData::Permission_All:    {
              for (RelationshipList::iterator relIter = docContacts.begin(); relIter != docContacts.end(); ++relIter) {
                ZS_LOG_TRACE(log("adding all contacts found in relationships document") + ", name=" + name + ", contact ID=" + (*relIter))
                contacts[(*relIter)] = (*relIter);
              }
              break;
            }
            case IPublicationMetaData::Permission_None:   {
              for (RelationshipList::iterator relIter = docContacts.begin(); relIter != docContacts.end(); ++relIter) {
                ContactMap::iterator found = contacts.find(*relIter);
                if (found == contacts.end()) {
                  ZS_LOG_TRACE(log("failed to remove all contacts found in relationships document") + ", name=" + name + ", contact ID=" + (*relIter))
                  continue;
                }
                ZS_LOG_TRACE(log("removing all contacts found in relationships document") + ", name=" + name + ", contact ID=" + (*relIter))
                contacts.erase(found);
              }
              break;
            }
            case IPublicationMetaData::Permission_Add:
            case IPublicationMetaData::Permission_Some:   {
              for (RelationshipList::const_iterator diffIter = diffContacts.begin(); diffIter != diffContacts.end(); ++diffIter) {
                ContactIDList::const_iterator found = find(docContacts.begin(), docContacts.end(), (*diffIter));
                if (found == docContacts.end()) {
                  ZS_LOG_TRACE(log("cannot add some of the contacts found in relationships document") + ", name=" + name + ", contact ID=" + (*diffIter))
                  continue; // cannot add anyone that isn't part of the relationship list
                }
                ZS_LOG_TRACE(log("adding some of the contacts found in relationships document") + ", name=" + name + ", contact ID=" + (*diffIter))
                contacts[(*diffIter)] = (*diffIter);
              }
              break;
            }
            case IPublicationMetaData::Permission_Remove: {
              for (RelationshipList::const_iterator diffIter = diffContacts.begin(); diffIter != diffContacts.end(); ++diffIter) {
                ContactIDList::const_iterator found = find(docContacts.begin(), docContacts.end(), (*diffIter));
                if (found == docContacts.end()) {
                  ZS_LOG_TRACE(log("cannot remove some of the contacts found in relationships document") + ", name=" + name + ", contact ID=" + (*diffIter))
                  continue; // cannot remove anyone that isn't part of the relationship list
                }

                ContactMap::iterator foundExisting = contacts.find(*diffIter);
                if (foundExisting == contacts.end()) {
                  ZS_LOG_TRACE(log("cannot removing some of the contacts found as the contact was never added to relationships document") + ", name=" + name + ", contact ID=" + (*diffIter))
                  continue;
                }
                ZS_LOG_TRACE(log("removing some of the contacts found in relationships document") + ", name=" + name + ", contact ID=" + (*diffIter))
                contacts.erase(foundExisting);
              }
              break;
            }
          }
        }

        for (ContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
          ZS_LOG_TRACE(log("final list of the resolved relationships contains this contact") + ", contact ID=" + (*iter).first)
          outContacts.push_back((*iter).first);
        }
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::canFetchPublication(
                                                      const PublishToRelationshipsMap &publishToRelationships,
                                                      const String &fetcherContactID
                                                      ) const
      {
        RelationshipList publishToContacts;  // all these contacts are being published to
        resolveRelationships(publishToRelationships, publishToContacts);

        bool found = false;
        // the document must publish to this contact or its ignored...
        for (RelationshipList::iterator iter = publishToContacts.begin(); iter != publishToContacts.end(); ++iter)
        {
          if ((*iter) == fetcherContactID) {
            found = true;
            break;
          }
        }

        if (!found) {
          ZS_LOG_WARNING(Detail, log("publication is not published to this fetcher contact") + ", fetcher contact ID=" + fetcherContactID)
          return false; // does not publish to this contact...
        }

        ZS_LOG_TRACE(log("publication is publishing to this fetcher contact (thus safe to fetch)") + ", fetcher contact ID=" + fetcherContactID)
        return true;
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::canSubscribeToPublisher(
                                                           const String &publicationCreatorContactID,
                                                           const PublishToRelationshipsMap &publishToRelationships,
                                                           const String &subscriberContactID,
                                                           const SubscribeToRelationshipsMap &subscribeToRelationships
                                                           ) const
      {
        RelationshipList publishToContacts;  // all these contacts are being published to
        resolveRelationships(publishToRelationships, publishToContacts);

        bool found = false;
        // the document must publish to this contact or its ignored...
        for (RelationshipList::iterator iter = publishToContacts.begin(); iter != publishToContacts.end(); ++iter)
        {
          if ((*iter) == subscriberContactID) {
            found = true;
            break;
          }
        }

        if (!found) {
          ZS_LOG_TRACE(log("publisher is not publishing to this subscriber contact") + ", publisher contact ID=" + publicationCreatorContactID + ", subscriber contact ID=" + subscriberContactID)
          return false; // does not publish to this contact...
        }

        RelationshipList subscribeToContacts; // all these cotnacts are being subscribed to
        resolveRelationships(subscribeToRelationships, subscribeToContacts);

        found = false;
        // the document must publish to this contact or its ignored...
        for (RelationshipList::iterator iter = subscribeToContacts.begin(); iter != subscribeToContacts.end(); ++iter)
        {
          if ((*iter) == publicationCreatorContactID) {
            found = true;
            break;
          }
        }

        if (!found) {
          ZS_LOG_TRACE(log("subscriber is not subscribing to this publisher") + ", publisher contact ID=" + publicationCreatorContactID + ", subscriber contact ID=" + subscriberContactID)
          return false; // does not publish to this contact...
        }

        ZS_LOG_TRACE(log("subscriber is subscribing to this creator and creator is publishing to the subscriber") + ", publisher contact ID=" + publicationCreatorContactID + ", subscriber contact ID=" + subscriberContactID)
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::log(const char *message) const
      {
        return String("PublicationRepository [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::activateFetcher(IPublicationMetaDataForPublicationRepositoryPtr metaData)
      {
        AutoRecursiveLock lock(getLock());

        if (mPendingFetchers.size() < 1) {
          ZS_LOG_DEBUG(log("no pending fetchers to activate..."))
          return;
        }

        ZS_LOG_DEBUG(log("activating next fetcher found with this meta data") + metaData->getDebugValuesString())

        for (PendingFetcherList::iterator iter = mPendingFetchers.begin(); iter != mPendingFetchers.end(); ++iter)
        {
          FetcherPtr &fetcher = (*iter);

          IPublicationMetaDataForPublicationRepositoryPtr fetcherMetaData = IPublicationMetaDataForPublicationRepository::convert(fetcher->getPublicationMetaData());

          ZS_LOG_TRACE(log("comparing against fetcher's meta data") + fetcherMetaData->getDebugValuesString())

          if (!metaData->isMatching(fetcherMetaData)) {
            ZS_LOG_TRACE(log("activation meta data does not match fetcher"))
            continue;
          }

          // this is an exact match...
          ZS_LOG_DEBUG(log("an exact match of the fetcher's meta data was found thus will attempt a fetch now"))

          // if already has a requester then already activated
          if (fetcher->getRequester()) {
            ZS_LOG_DEBUG(log("cannot activate next fetcher as fetcher is already activated"))
            return;
          }

          CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);

          if (found != mCachedRemotePublications.end()) {
            IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;

            ZS_LOG_TRACE(log("existing publication found in 'remote' cache for meta data") + existingPublication->getDebugValuesString())
            ULONG fetchingVersion = metaData->getVersion();
            if (existingPublication->getVersion() >= fetchingVersion) {
              ZS_LOG_DETAIL(log("short circuit the fetch since the document is already in our cache") + existingPublication->getDebugValuesString())

              fetcher->setPublication(existingPublication);
              fetcher->notifyCompleted();
              return;
            }

            metaData->setVersion(existingPublication->getVersion());
          } else {
            ZS_LOG_TRACE(log("existing publication was not found in 'remote' cache"))
            metaData->setVersion(0);
          }

          message::PeerGetRequestPtr message = message::PeerGetRequest::create();
          message->publicationMetaData(metaData->convertIPublicationMetaData());

          // find the contacts to publish to...
          IAccountForPublicationRepositoryPtr outer = mOuter.lock();
          if (!outer) {
            ZS_LOG_WARNING(Detail, log("cannot fetch publication as account object is gone"))
            fetcher->cancel();  // sorry, this could not be completed...
            mPendingFetchers.erase(iter);
            return;
          }

          // activate the fetcher...
          switch (metaData->getSource()) {
            case IPublicationMetaData::Source_Local:  ZS_THROW_BAD_STATE("local fetchers should never be pending") break;
            case IPublicationMetaData::Source_Finder: {
              fetcher->setRequester(outer->sendFinderRequest(fetcher, message, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
              break;
            }
            case IPublicationMetaData::Source_Peer:   {
              fetcher->setRequester(outer->sendPeerRequest(fetcher, message, metaData->getPublishedToContactID(), metaData->getPublishedToLocationID(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
              break;
            }
          }

          return;
        }
        ZS_LOG_DEBUG(log("no pending fetchers of this type were found"))
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::activatePublisher(IPublicationForPublicationRepositoryPtr publication)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("attempting to activate next publisher for publication") + publication->getDebugValuesString())

        for (PendingPublisherList::iterator iter = mPendingPublishers.begin(); iter != mPendingPublishers.end(); ++iter)
        {
          PublisherPtr &publisher = (*iter);
          IPublicationForPublicationRepositoryPtr publisherPublication = IPublicationForPublicationRepository::convert(publisher->getPublication());

          if (publication->getID() != publisherPublication->getID()) {
            ZS_LOG_TRACE(log("pending publication is not the correct one to activate") + Stringize<PUID>(publisherPublication->getID()).string())
            continue;
          }

          ZS_LOG_DEBUG(log("found the correct publisher to activate and will attempt to activate it now"))

          IAccountForPublicationRepositoryPtr outer = mOuter.lock();
          if (!outer) {
            ZS_LOG_WARNING(Detail, log("cannot activate next publisher as account object is gone"))
            mPendingPublishers.erase(iter);
            publisher->cancel();  // sorry, this could not be completed...
            return;
          }

          if (publisher->getRequester()) {
            ZS_LOG_DEBUG(log("cannot active the next publisher as it is already activated"))
            return;
          }

          switch (publication->getSource()) {
            case IPublicationMetaData::Source_Local:  ZS_THROW_BAD_STATE("not possible the publication is local") break;
            case IPublicationMetaData::Source_Finder: {
              message::PeerPublishRequestPtr message = message::PeerPublishRequest::create();
              message->publication(publication->convertIPublication());
              message->publishedFromVersion(publication->getBaseVersion());
              message->publishedToVersion(publication->getVersion());

              publisher->setRequester(outer->sendFinderRequest(publisher, message, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
              break;
            }
            case IPublicationMetaData::Source_Peer:   {
              message::PeerPublishRequestPtr message = message::PeerPublishRequest::create();
              message->publication(publication->convertIPublication());
              message->publishedFromVersion(publication->getBaseVersion());
              message->publishedToVersion(publication->getVersion());

              publisher->setRequester(outer->sendPeerRequest(publisher, message, publication->getPublishedToContactID(), publication->getPublishedToLocationID(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
              break;
            }
          }

          return;
        }
      }

      //-----------------------------------------------------------------------
      PeerSubscriptionIncomingPtr PublicationRepository::findFinderIncomingSubscription(const char *publicationName) const
      {
        ZS_LOG_TRACE(log("finding incoming finder subscription with publication path") + Stringize<CSTR>(publicationName).string())

        for (PeerSubscriptionIncomingMap::const_iterator iter = mPeerSubscriptionsIncoming.begin(); iter != mPeerSubscriptionsIncoming.end(); ++iter)
        {
          const PeerSubscriptionIncomingPtr &subscription = (*iter).second;
          IPublicationMetaDataPtr metaData = subscription->getSource();
          if (metaData->getSource() != IPublicationMetaData::Source_Finder) continue;
          if (metaData->getName() != publicationName) continue;

          ZS_LOG_TRACE(log("incoming finder subscription was found"))
          return subscription;
        }

        ZS_LOG_TRACE(log("failed to find finder subscription"))
        return PeerSubscriptionIncomingPtr();
      }

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionIncomingPtr PublicationRepository::findPeerIncomingSubscription(
                                                                                                             const char *publicationName,
                                                                                                             const char *creatorContactID,
                                                                                                             const char *creatorLocationID
                                                                                                             ) const
      {
        ZS_LOG_TRACE(log("finding incoming peer subscription with publication path") + Stringize<CSTR>(publicationName).string() + " peer contact ID=" + Stringize<CSTR>(creatorContactID).string() + ", peer location ID=" + Stringize<CSTR>(creatorLocationID).string())

        for (PeerSubscriptionIncomingMap::const_iterator iter = mPeerSubscriptionsIncoming.begin(); iter != mPeerSubscriptionsIncoming.end(); ++iter)
        {
          const PeerSubscriptionIncomingPtr &subscription = (*iter).second;
          IPublicationMetaDataPtr metaData = subscription->getSource();
          if (metaData->getSource() != IPublicationMetaData::Source_Peer) continue;
          if (metaData->getName() != publicationName) continue;
          if (metaData->getCreatorContactID() != creatorContactID) continue;
          if (metaData->getCreatorLocationID() != creatorLocationID) continue;

          ZS_LOG_TRACE(log("incoming peer subscription was found"))
          return subscription;
        }
        ZS_LOG_TRACE(log("could not to find peer subscription (probably okay)"))
        return PeerSubscriptionIncomingPtr();
      }

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionIncomingPtr PublicationRepository::findIncomingSubscription(IPublicationMetaDataForPublicationRepositoryPtr metaData) const
      {
        switch (metaData->getSource()) {
          case IPublicationMetaData::Source_Local:  ZS_THROW_BAD_STATE(log("why is an incoming local subscription attempting to be found?"))
          case IPublicationMetaData::Source_Finder: return findFinderIncomingSubscription(metaData->getName());
          case IPublicationMetaData::Source_Peer:   return findPeerIncomingSubscription(metaData->getName(), metaData->getCreatorContactID(), metaData->getCreatorLocationID());
        }
        return PeerSubscriptionIncomingPtr();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onIncomingMessage(
                                                    IConnectionSubscriptionMessagePtr incomingMessage,
                                                    message::PeerPublishRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        ZS_LOG_DEBUG(log("incoming peer publish request"))

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer publish request as account object is gone"))
          return;
        }

        IPublicationForPublicationRepositoryPtr publication = IPublicationForPublicationRepository::convert(request->publication());
        publication->setSource(toSource(incomingMessage->getSource()));
        if (IConnectionSubscriptionMessage::Source_Peer == incomingMessage->getSource()) {
          publication->setCreatorContact(incomingMessage->getPeerContactID(), incomingMessage->getPeerLocationID());
        }
        publication->setPublishedToContact(outer->getContactID(), outer->getLocationID());

        ZS_LOG_TRACE(log("incoming request to publish document") + publication->getDebugValuesString())

        CachedPublicationMap::iterator found = mCachedLocalPublications.find(publication);

        IPublicationMetaDataForPublicationRepositoryPtr responceMetaData;
        if (found != mCachedLocalPublications.end()) {
          IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;

          ZS_LOG_DEBUG(log("updating existing publication in 'local' cache with remote publication") + existingPublication->getDebugValuesString())
          try {
            existingPublication->updateFromFetchedPublication(publication);
          } catch(IPublicationForPublicationRepository::Exceptions::VersionMismatch &) {
            ZS_LOG_WARNING(Detail, log("cannot update with the publication published since the versions are incompatible"))
            message::MessageResultPtr errorResult = message::MessageResult::create(request, 409, "Conflict");
            incomingMessage->sendResponse(errorResult);
            return;
          }

          responceMetaData = existingPublication;
        } else {
          ZS_LOG_DEBUG(log("creating new entry in 'local' cache for remote publication") + publication->getDebugValuesString())
          mCachedLocalPublications[publication] = publication;
          responceMetaData = publication;

          ZS_LOG_DEBUG(log("publication inserted into local cache") + ", local cache total=" + Stringize<size_t>(mCachedLocalPublications.size()).string())
        }

        message::PeerPublishResultPtr reply = message::PeerPublishResult::create(request);
        reply->publicationMetaData(responceMetaData->convertIPublicationMetaData());
        incomingMessage->sendResponse(reply);

        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        // HERE - notify other subscriptions?
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onIncomingMessage(
                                                    IConnectionSubscriptionMessagePtr incomingMessage,
                                                    message::PeerGetRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer get request as account object is gone"))
          return;
        }

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::convert(request->publicationMetaData());

        PeerSourcePtr sourceMetaData;
        if (IConnectionSubscriptionMessage::Source_Finder == incomingMessage->getSource()) {
          sourceMetaData = IPublicationMetaDataForPublicationRepository::createFinderSource();
        } else {
          sourceMetaData = IPublicationMetaDataForPublicationRepository::createPeerSource(incomingMessage->getPeerContactID(), incomingMessage->getPeerLocationID());
        }

        if (metaData->getCreatorContactID() == outer->getContactID()) {
          ZS_LOG_TRACE(log("fetching a publication published created by 'this' user (thus setting source to local)"))
          // requesting a publication published by "ourself"
          metaData->setSource(IPublicationMetaData::Source_Local);
        } else {
          ZS_LOG_TRACE(log("fetching a publication created by another peer but published to the local document cache"))
          // fetching a document created by a another peer but published to "this" repository...
          metaData->setSource(toSource(incomingMessage->getSource()));
        }

        // the publication must have be published to "this" repository...
        metaData->setPublishedToContact(outer->getContactID(), outer->getLocationID());

        ZS_LOG_DEBUG(log("incoming request to get a document published to the local cache") + metaData->getDebugValuesString())

        CachedPublicationMap::iterator found = mCachedLocalPublications.find(metaData);
        if (found == mCachedLocalPublications.end()) {
          ZS_LOG_WARNING(Detail, log("failed to find publicatin thus ignoring get request"))
          return;
        }

        IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;

        if (IConnectionSubscriptionMessage::Source_Peer == incomingMessage->getSource()) {
          String currentContactID = incomingMessage->getPeerContactID();

          if (!canFetchPublication(
                                   existingPublication->getRelationships(),
                                   currentContactID)) {
            ZS_LOG_WARNING(Detail, log("publication is not published to the peer requesting the document (thus unable to reply to fetch request)"))
            return;
          }
          ZS_LOG_TRACE(log("requesting peer has authorization to get the requested document"))
        }

        ZS_LOG_TRACE(log("incoming get request to will return this document") + existingPublication->getDebugValuesString())

        PeerCachePtr peerCache = PeerCache::find(sourceMetaData, mThisWeak.lock());
        peerCache->notifyFetched(existingPublication);

        message::PeerGetResultPtr reply = message::PeerGetResult::create(request);
        reply->publication(existingPublication->convertIPublication());
        incomingMessage->sendResponse(reply);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onIncomingMessage(
                                                    IConnectionSubscriptionMessagePtr incomingMessage,
                                                    message::PeerDeleteRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer delete request as account object is gone"))
          return;
        }

        IPublicationMetaDataPtr requestMetaData = request->publicationMetaData();
        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::convert(requestMetaData);

        metaData->setSource(toSource(incomingMessage->getSource()));
        if (IConnectionSubscriptionMessage::Source_Peer == incomingMessage->getSource()) {
          metaData->setCreatorContact(incomingMessage->getPeerContactID(), incomingMessage->getPeerLocationID());
        }
        metaData->setPublishedToContact(outer->getContactID(), outer->getLocationID());

        ZS_LOG_DEBUG(log("incoming request to delete document") + metaData->getDebugValuesString())

        CachedPublicationMap::iterator found = mCachedLocalPublications.find(metaData);
        if (found == mCachedLocalPublications.end()) {
          ZS_LOG_WARNING(Detail, log("failed to find publicatin thus ignoring delete request"))
          return;
        }

        IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;

        ZS_LOG_DEBUG(log("delete request will delete this publication") + existingPublication->getDebugValuesString())

        mCachedLocalPublications.erase(found);

        message::PeerDeleteResultPtr reply = message::PeerDeleteResult::create(request);
        incomingMessage->sendResponse(reply);

        // notify all the subscribers that the document is gone
        for (SubscriptionLocalMap::iterator iter = mSubscriptionsLocal.begin(); iter != mSubscriptionsLocal.end(); ++iter)
        {
          SubscriptionLocalPtr &subscriber = (*iter).second;
          subscriber->notifyGone(existingPublication);
        }

        for (PeerSubscriptionIncomingMap::iterator iter = mPeerSubscriptionsIncoming.begin(); iter != mPeerSubscriptionsIncoming.end(); ++iter)
        {
          PeerSubscriptionIncomingPtr &subscriber = (*iter).second;
          subscriber->notifyGone(existingPublication);
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onIncomingMessage(
                                                    IConnectionSubscriptionMessagePtr incomingMessage,
                                                    message::PeerSubscribeRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer subscribe request as account object is gone"))
          return;
        }

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::convert(request->publicationMetaData());

        metaData->setSource(toSource(incomingMessage->getSource()));
        if (IConnectionSubscriptionMessage::Source_Peer == incomingMessage->getSource()) {
          metaData->setCreatorContact(incomingMessage->getPeerContactID(), incomingMessage->getPeerLocationID());
        }
        metaData->setPublishedToContact(outer->getContactID(), outer->getLocationID());

        ZS_LOG_DEBUG(log("incoming request to subscribe to document path") + metaData->getDebugValuesString())

        PeerSubscriptionIncomingPtr existingSubscription = findIncomingSubscription(metaData);
        if (existingSubscription) {
          ZS_LOG_TRACE(log("removing existing subscription of the same source"))

          PeerSubscriptionIncomingMap::iterator found = mPeerSubscriptionsIncoming.find(existingSubscription->getID());
          ZS_THROW_BAD_STATE_IF(found == mPeerSubscriptionsIncoming.end())

          mPeerSubscriptionsIncoming.erase(found);
        }

        // send the reply now...
        message::PeerSubscribeResultPtr reply = message::PeerSubscribeResult::create(request);
        reply->publicationMetaData(metaData->convertIPublicationMetaData());
        incomingMessage->sendResponse(reply);

        const IPublicationMetaData::PublishToRelationshipsMap &relationships = metaData->getRelationships();
        if (relationships.size() > 0) {
          ZS_LOG_TRACE(log("incoming subscription is being created"))

          PeerSourcePtr sourceMetaData;
          if (IConnectionSubscriptionMessage::Source_Finder == incomingMessage->getSource()) {
            sourceMetaData = IPublicationMetaDataForPublicationRepository::createFinderSource();
          } else {
            sourceMetaData = IPublicationMetaDataForPublicationRepository::createPeerSource(incomingMessage->getPeerContactID(), incomingMessage->getPeerLocationID());
          }

          // if there are relationships then this is an addition not a removal
          PeerSubscriptionIncomingPtr incoming = PeerSubscriptionIncoming::create(getAssociatedMessageQueue(), mThisWeak.lock(), sourceMetaData, metaData);

          mPeerSubscriptionsIncoming[incoming->getID()] = incoming;

          ZS_LOG_TRACE(log("notifying of all cached publications published to the new incoming party (so that all notifications can arrive in one message)"))
          incoming->notifyUpdated(mCachedLocalPublications);
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onIncomingMessage(
                                                    IConnectionSubscriptionMessagePtr incomingMessage,
                                                    message::PeerPublishNotifyRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        IAccountForPublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer notify request as account object is gone"))
          return;
        }

        // send the reply now...
        message::PeerPublishNotifyResultPtr reply = message::PeerPublishNotifyResult::create(request);
        incomingMessage->sendResponse(reply);

        typedef message::PeerPublishNotifyRequest::PublicationList PublicationList;

        const PublicationList &publicationList = request->publicationList();

        ZS_LOG_DEBUG(log("received publish notification") + ", total publications=" + Stringize<size_t>(publicationList.size()).string())

        for (PublicationList::const_iterator iter = publicationList.begin(); iter != publicationList.end(); ++iter) {
          const IPublicationMetaDataPtr &requestMetaData = (*iter);
          IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::convert(requestMetaData);

          ZS_LOG_DEBUG(log("received notification of document change") + metaData->getDebugValuesString())

          metaData->setSource(toSource(incomingMessage->getSource()));
          if (IConnectionSubscriptionMessage::Source_Peer == incomingMessage->getSource()) {
            metaData->setPublishedToContact(incomingMessage->getPeerContactID(), incomingMessage->getPeerLocationID());
          }

          IPublicationForPublicationRepositoryPtr publication = IPublicationForPublicationRepository::convert(metaData->convertIPublicationMetaData()->getPublication());
          if (publication) {
            ZS_LOG_DEBUG(log("publication was included with notification") + publication->getDebugValuesString())

            // not only is meta data available an update to the publication is available, search the cache for the document
            CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);;
            if (found != mCachedRemotePublications.end()) {
              IPublicationForPublicationRepositoryPtr &existingPublication = (*found).second;
              ZS_LOG_DEBUG(log("existing internal publication found thus updating (if possible)") + existingPublication->getDebugValuesString())
              try {
                existingPublication->updateFromFetchedPublication(publication);
              } catch(IPublicationForPublicationRepository::Exceptions::VersionMismatch &) {
                ZS_LOG_WARNING(Detail, log("version from the notify does not match our last version (thus ignoring change)"))
              }
            } else {
              bool okayToCreate = true;
              AutoRecursiveLockPtr docLock;
              DocumentPtr doc = publication->getXML(docLock);
              if (doc) {
                ElementPtr xdsEl = doc->findFirstChildElement("xds");
                okayToCreate = !xdsEl;
                if (!okayToCreate) {
                  ZS_LOG_WARNING(Detail, log("new entry for remote cache cannot be created for publication which only contains diff updates") + publication->getDebugValuesString())
                }
              }
              if (okayToCreate) {
                ZS_LOG_DEBUG(log("new entry for remote cache will be created since existing publication in cache was not found") + publication->getDebugValuesString())
                mCachedRemotePublications[publication] = publication;

                ZS_LOG_DEBUG(log("publication inserted into remote cache") + ", remote cache total=" + Stringize<size_t>(mCachedRemotePublications.size()).string())
              }
            }
          }

          for (PeerSubscriptionOutgoingMap::iterator subIter = mPeerSubscriptionsOutgoing.begin(); subIter != mPeerSubscriptionsOutgoing.end(); ++subIter)
          {
            PeerSubscriptionOutgoingPtr &subscriber = (*subIter).second;
            ZS_LOG_TRACE(log("notifying outgoing subscription of change") + ", susbcriber ID=" + Stringize<PUID>(subscriber->getID()).string())
            subscriber->notifyUpdated(metaData);
          }
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::cancel()
      {
        ZS_LOG_DETAIL(log("publication repository cancel is called"))

        if (mConnectionSubscription) {
          mConnectionSubscription->cancel();
          mConnectionSubscription.reset();
        }

        if (mExpiresTimer) {
          mExpiresTimer->cancel();
          mExpiresTimer.reset();
        }

        // clear out all pending fetchers
        {
          // need to copy to a temporary list and clean out the original pending list to prevent cancelling of one activating another...
          PendingFetcherList pending = mPendingFetchers;
          mPendingFetchers.clear();

          for (PendingFetcherList::iterator iter = pending.begin(); iter != pending.end(); ++iter)
          {
            FetcherPtr &fetcher = (*iter);
            ZS_LOG_DEBUG(log("cancelling pending fetcher") + Stringize<PUID>(fetcher->getID()).string())
            fetcher->cancel();
          }
        }

        // clear out all pending publishers
        {
          // need to copy to a temporary list and clean out the original pending list to prevent cancelling of one activating another...
          PendingPublisherList pending = mPendingPublishers;
          mPendingPublishers.clear();

          for (PendingPublisherList::iterator iter = pending.begin(); iter != pending.end(); ++iter)
          {
            PublisherPtr &publisher = (*iter);
            ZS_LOG_DEBUG(log("cancelling pending publisher") + Stringize<PUID>(publisher->getID()).string())
            publisher->cancel();
          }
        }

        // clear out all the cached publications...
        {
          for (CachedPublicationMap::iterator iter = mCachedLocalPublications.begin(); iter != mCachedLocalPublications.end(); ++iter)
          {
            IPublicationForPublicationRepositoryPtr &publication = (*iter).second;

            // notify all the local subscribers that the documents are now gone...
            for (SubscriptionLocalMap::iterator subscriberIter = mSubscriptionsLocal.begin(); subscriberIter != mSubscriptionsLocal.end(); ++subscriberIter)
            {
              SubscriptionLocalPtr &subscriber = (*subscriberIter).second;
              ZS_LOG_DEBUG(log("notifying local subscriber that publication is gone during cancel") + Stringize<PUID>(subscriber->getID()).string())
              subscriber->notifyGone(publication);
            }

            // notify all the incoming subscriptions that the documents are now gone...
            for (PeerSubscriptionIncomingMap::iterator subscriberIter = mPeerSubscriptionsIncoming.begin(); subscriberIter != mPeerSubscriptionsIncoming.end(); ++subscriberIter)
            {
              PeerSubscriptionIncomingPtr &subscriber = (*subscriberIter).second;
              ZS_LOG_DEBUG(log("notifying incoming subscriber that publication is gone during cancel") + Stringize<PUID>(subscriber->getID()).string())
              subscriber->notifyGone(publication);
            }
          }

          mCachedLocalPublications.clear();
          mCachedRemotePublications.clear();
        }

        // clear out all the local subscriptions...
        for (SubscriptionLocalMap::iterator subIter = mSubscriptionsLocal.begin(); subIter != mSubscriptionsLocal.end(); )
        {
          SubscriptionLocalMap::iterator current = subIter;
          ++subIter;

          SubscriptionLocalPtr &subscriber = (*current).second;
          ZS_LOG_DEBUG(log("cancelling local subscription") + Stringize<PUID>(subscriber->getID()).string())
          subscriber->cancel();
        }
        mSubscriptionsLocal.clear();

        // clear out all the incoming subscriptions
        for (PeerSubscriptionIncomingMap::iterator iter = mPeerSubscriptionsIncoming.begin(); iter != mPeerSubscriptionsIncoming.end(); ++iter)
        {
          PeerSubscriptionIncomingPtr &subscriber = (*iter).second;
          ZS_LOG_DEBUG(log("cancelling incoming subscription") + Stringize<PUID>(subscriber->getID()).string())
          subscriber->cancel();
        }
        mPeerSubscriptionsIncoming.clear();

        // clear out all the outgoing subscriptions
        for (PeerSubscriptionOutgoingMap::iterator subIter = mPeerSubscriptionsOutgoing.begin(); subIter != mPeerSubscriptionsOutgoing.end(); )
        {
          PeerSubscriptionOutgoingMap::iterator current = subIter;
          ++subIter;

          PeerSubscriptionOutgoingPtr &subscriber = (*current).second;
          ZS_LOG_DEBUG(log("cancelling outgoing subscription") + Stringize<PUID>(subscriber->getID()).string())
          subscriber->cancel();
        }
        mPeerSubscriptionsOutgoing.clear();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerCache
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::PeerCache::PeerCache(
                                                  PeerSourcePtr peerSource,
                                                  PublicationRepositoryPtr repository
                                                  ) :
        mID(zsLib::createPUID()),
        mOuter(repository),
        mPeerSource(peerSource)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerCache::init()
      {
      }

      //-----------------------------------------------------------------------
      PeerCachePtr PublicationRepository::PeerCache::create(
                                                            PeerSourcePtr peerSource,
                                                            PublicationRepositoryPtr repository
                                                            )
      {
        PeerCachePtr pThis(new PeerCache(peerSource, repository));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      PublicationRepository::PeerCache::~PeerCache()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerCache => IPublicationRepositoryPeerCache
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::PeerCache::getNextVersionToNotifyAboutAndMarkNotified(
                                                                                        IPublicationPtr inPublication,
                                                                                        ULONG &ioMaxSizeAvailableInBytes,
                                                                                        ULONG &outNotifyFromVersion,
                                                                                        ULONG &outNotifyToVersion
                                                                                        )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inPublication)

        AutoRecursiveLock lock(getLock());

        IPublicationForPublicationRepositoryPtr publication = IPublicationForPublicationRepository::convert(inPublication);

        CachedPeerPublicationMap::iterator found = mCachedPublications.find(publication);
        if (found != mCachedPublications.end()) {

          // this document was fetched before...
          IPublicationMetaDataForPublicationRepositoryPtr &metaData = (*found).second;
          metaData->setExpires(publication->getExpires());  // not used yet but could be used to remember when this document will expire

          ULONG nextVersionToNotify = metaData->getVersion() + 1;
          if (nextVersionToNotify > publication->getVersion()) {
            ZS_LOG_WARNING(Detail, log("already fetched/notified of this version so why is it being notified? (probably okay and likely because of peer disconnection)") + publication->getDebugValuesString())
            return false;
          }

          ULONG outputSize = 0;
          publication->getDiffVersionsOutputSize(nextVersionToNotify, publication->getVersion(), outputSize);

          if (outputSize > ioMaxSizeAvailableInBytes) {
            ZS_LOG_WARNING(Detail, log("diff document is too large for notify") + ", output size=" + Stringize<ULONG>(outputSize).string() + ", max size=" + Stringize<ULONG>(ioMaxSizeAvailableInBytes).string())
            return false;
          }

          outNotifyFromVersion = nextVersionToNotify;
          outNotifyToVersion = publication->getVersion();

          ioMaxSizeAvailableInBytes -= outputSize;

          metaData->setVersion(outNotifyToVersion);

          ZS_LOG_DETAIL(log("recommend notify about diff version") +
                            ", from=" + Stringize<ULONG>(outNotifyFromVersion).string() +
                            ", to=" + Stringize<ULONG>(outNotifyToVersion).string()  +
                            ", size=" + Stringize<ULONG>(outputSize).string() +
                            ", remaining=" + Stringize<ULONG>(ioMaxSizeAvailableInBytes).string())
          return true;
        }

        ULONG outputSize = 0;
        publication->getEntirePublicationOutputSize(outputSize);

        if (outputSize > ioMaxSizeAvailableInBytes) {
          ZS_LOG_WARNING(Detail, log("diff document is too large for notify") + ", output size=" + Stringize<ULONG>(outputSize).string() + ", max size=" + Stringize<ULONG>(ioMaxSizeAvailableInBytes).string())
          return false;
        }

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(publication->convertIPublicationMetaData());
        metaData->setExpires(publication->getExpires());  // not used yet but could be used to remember when this document will expire
        metaData->setBaseVersion(0);
        metaData->setVersion(publication->getVersion());

        mCachedPublications[metaData] = metaData;

        outNotifyFromVersion = 0;
        outNotifyToVersion = publication->getVersion();
        ioMaxSizeAvailableInBytes -= outputSize;

        ZS_LOG_DETAIL(log("recommend notify about entire document") +
                     ", from=" + Stringize<ULONG>(outNotifyFromVersion).string() +
                     ", to=" + Stringize<ULONG>(outNotifyToVersion).string()  +
                     ", size=" + Stringize<ULONG>(outputSize).string() +
                     ", remaining=" + Stringize<ULONG>(ioMaxSizeAvailableInBytes).string())
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerCache => friend PublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PeerCachePtr PublicationRepository::PeerCache::find(
                                                          PeerSourcePtr peerSource,
                                                          PublicationRepositoryPtr repository
                                                          )
      {
        AutoRecursiveLock lock(repository->getLock());

        CachedPeerSourceMap &cache = repository->getCachedPeerSources();
        CachedPeerSourceMap::iterator found = cache.find(peerSource);
        if (found != cache.end()) {
          PeerCachePtr &peerCache = (*found).second;
          return peerCache;
        }
        PeerCachePtr pThis = PeerCache::create(peerSource, repository);
        cache[peerSource] = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerCache::notifyFetched(IPublicationForPublicationRepositoryPtr publication)
      {
        AutoRecursiveLock lock(getLock());

        CachedPeerPublicationMap::iterator found = mCachedPublications.find(publication);
        if (found != mCachedPublications.end()) {
          IPublicationMetaDataForPublicationRepositoryPtr &metaData = (*found).second;

          // remember up to which version was last fetched
          metaData->setVersion(publication->getVersion());
          return;
        }

        IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(publication->convertIPublicationMetaData());
        metaData->setBaseVersion(0);
        metaData->setVersion(publication->getVersion());

        mCachedPublications[metaData] = metaData;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerCache => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::PeerCache::log(const char *message) const
      {
        return String("PublicationRepository::PeerCache [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      RecursiveLock &PublicationRepository::PeerCache::getLock() const
      {
        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Publisher
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::Publisher::Publisher(
                                                  IMessageQueuePtr queue,
                                                  PublicationRepositoryPtr outer,
                                                  IPublicationPublisherDelegatePtr delegate,
                                                  IPublicationForPublicationRepositoryPtr publication
                                                  ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IPublicationPublisherDelegateProxy::createWeak(delegate)),
        mPublication(publication),
        mSucceeded(false),
        mErrorCode(0)
      {
        ZS_LOG_DEBUG(log("created new publisher") + publication->getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::init()
      {
      }

      //-----------------------------------------------------------------------
      PublicationRepository::Publisher::~Publisher()
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
      #pragma mark PublicationRepository::Publisher => friend PublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::PublisherPtr PublicationRepository::Publisher::create(
                                                                                   IMessageQueuePtr queue,
                                                                                   PublicationRepositoryPtr outer,
                                                                                   IPublicationPublisherDelegatePtr delegate,
                                                                                   IPublicationForPublicationRepositoryPtr publication
                                                                                   )
      {
        PublisherPtr pThis(new Publisher(queue, outer, delegate, publication));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::setRequester(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        mRequester = requester;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::notifyCompleted()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("publisher succeeded"))
        mSucceeded = true;
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Publisher => IPublicationPublisher
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::cancel()
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("cancel called"))

        if (mRequester) {
          mRequester->cancel();
          mRequester.reset();
        }

        PublisherPtr pThis = mThisWeak.lock();

        if (pThis) {
          if (mDelegate) {
            try {
              mDelegate->onPublicationPublisherComplete(mThisWeak.lock());
            } catch(IPublicationPublisherDelegateProxy::Exceptions::DelegateGone &) {
            }
          }

          PublicationRepositoryPtr outer = mOuter.lock();
          if (outer) {
            outer->notifyPublisherCancelled(pThis);
          }
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::Publisher::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::Publisher::wasSuccessful() const
      {
        AutoRecursiveLock lock(getLock());
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      WORD PublicationRepository::Publisher::getErrorResult() const
      {
        AutoRecursiveLock lock(getLock());
        return mErrorCode;
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::Publisher::getErrorReason() const
      {
        AutoRecursiveLock lock(getLock());
        return mErrorReason;
      }

      //-----------------------------------------------------------------------
      IPublicationPtr PublicationRepository::Publisher::getPublication() const
      {
        AutoRecursiveLock lock(getLock());
        return mPublication->convertIPublication();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Publisher => IMessageRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::Publisher::handleMessageRequesterMessageReceived(
                                                                                   IMessageRequesterPtr requester,
                                                                                   message::MessagePtr message
                                                                                   )
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received result but publisher is already cancelled"))
          return false;
        }
        if (mRequester != requester) {
          ZS_LOG_WARNING(Detail, log("received result but not for the correct requester"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expected result but received something else"))
          return false;
        }

        if ((message::MessageFactoryStack::Methods)message->method() != message::MessageFactoryStack::Method_PeerPublish) {
          ZS_LOG_WARNING(Detail, log("expecting peer publish method but received something else"))
          return false;
        }

        message::MessageResultPtr result = message::MessageResult::convert(message);
        if (result->hasError()) {
          ZS_LOG_WARNING(Detail, log("publish failed"))
          mSucceeded = false;
          mErrorCode = result->errorCode();
          mErrorReason = result->errorReason();
          cancel();
          return true;
        }

        ZS_LOG_DEBUG(log("received publish result"))

        message::PeerPublishResultPtr publishResult = message::PeerPublishResult::convert(result);

        message::PeerPublishRequestPtr originalRequest = message::PeerPublishRequest::convert(requester->getMonitoredMessage());

        // now published from the original base version to the current version
        // so the base is now the published version plus one...
        mPublication->setBaseVersion(originalRequest->publishedToVersion()+1);

        notifyCompleted();

        PublicationRepositoryPtr outer = mOuter.lock();
        if (outer) {
          outer->notifyPublished(mThisWeak.lock());
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        if (requester != mRequester) {
          ZS_LOG_DEBUG(log("ignoring publish request time out since it doesn't match requester"))
          return;
        }
        ZS_LOG_DEBUG(log("publish request timed out"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Publisher => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::Publisher::log(const char *message) const
      {
        return String("PublicationRepository::Publisher [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      IMessageRequesterPtr PublicationRepository::Publisher::getRequester() const
      {
        AutoRecursiveLock lock(getLock());
        return mRequester;
      }

      //-----------------------------------------------------------------------
      RecursiveLock &PublicationRepository::Publisher::getLock() const
      {
        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Fetcher
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::Fetcher::Fetcher(
                                              IMessageQueuePtr queue,
                                              PublicationRepositoryPtr outer,
                                              IPublicationFetcherDelegatePtr delegate,
                                              IPublicationMetaDataForPublicationRepositoryPtr metaData
                                              ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IPublicationFetcherDelegateProxy::createWeak(delegate)),
        mPublicationMetaData(metaData),
        mSucceeded(false),
        mErrorCode(0)
      {
        ZS_LOG_DEBUG(log("created") + metaData->getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::init()
      {
      }

      //-----------------------------------------------------------------------
      PublicationRepository::Fetcher::~Fetcher()
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
      #pragma mark PublicationRepository::Fetcher => friend PublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::FetcherPtr PublicationRepository::Fetcher::create(
                                                                               IMessageQueuePtr queue,
                                                                               PublicationRepositoryPtr outer,
                                                                               IPublicationFetcherDelegatePtr delegate,
                                                                               IPublicationMetaDataForPublicationRepositoryPtr metaData
                                                                               )
      {
        FetcherPtr pThis(new Fetcher(queue, outer, delegate, metaData));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::setPublication(IPublicationForPublicationRepositoryPtr publication)
      {
        AutoRecursiveLock lock(getLock());
        mFetchedPublication = publication;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::setRequester(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        mRequester = requester;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::notifyCompleted()
      {
        AutoRecursiveLock lock(getLock());
        mSucceeded = true;
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Fetcher => IPublicationFetcher
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::cancel()
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("cancel called"))

        if (mRequester) {
          mRequester->cancel();
          mRequester.reset();
        }

        FetcherPtr pThis = mThisWeak.lock();

        if (pThis) {
          if (mDelegate) {
            try {
              mDelegate->onPublicationFetcherComplete(mThisWeak.lock());
            } catch(IPublicationFetcherDelegateProxy::Exceptions::DelegateGone &) {
            }
          }

          PublicationRepositoryPtr outer = mOuter.lock();
          if (outer) {
            outer->notifyFetcherCancelled(pThis);
          }
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::Fetcher::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::Fetcher::wasSuccessful() const
      {
        AutoRecursiveLock lock(getLock());
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      WORD PublicationRepository::Fetcher::getErrorResult() const
      {
        AutoRecursiveLock lock(getLock());
        return mErrorCode;
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::Fetcher::getErrorReason() const
      {
        AutoRecursiveLock lock(getLock());
        return mErrorReason;
      }

      //-----------------------------------------------------------------------
      IPublicationPtr PublicationRepository::Fetcher::getFetchedPublication() const
      {
        AutoRecursiveLock lock(getLock());
        return mFetchedPublication->convertIPublication();
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataPtr PublicationRepository::Fetcher::getPublicationMetaData() const
      {
        AutoRecursiveLock lock(getLock());
        return mPublicationMetaData->convertIPublicationMetaData();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Fetcher => IMessageRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::Fetcher::handleMessageRequesterMessageReceived(
                                                                                 IMessageRequesterPtr requester,
                                                                                 message::MessagePtr message
                                                                                 )
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received result but fetcher is shutdown"))
          return false;
        }
        if (mRequester != requester) {
          ZS_LOG_WARNING(Detail, log("received result but requester does not match"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expecting result but received something else"))
          return false;
        }

        if ((message::MessageFactoryStack::Methods)message->method() != message::MessageFactoryStack::Method_PeerGet) {
          ZS_LOG_WARNING(Detail, log("expecting peer get result but received some other method"))
          return false;
        }

        message::MessageResultPtr result = message::MessageResult::convert(message);
        if (result->hasError()) {
          ZS_LOG_WARNING(Detail, log("received a result but result had an error"))
          mSucceeded = false;
          mErrorCode = result->errorCode();
          mErrorReason = result->errorReason();
          cancel();
          return true;
        }

        message::PeerGetResultPtr getResult = message::PeerGetResult::convert(result);
        mFetchedPublication = IPublicationForPublicationRepository::convert(getResult->publication());

        mSucceeded = (bool)(mFetchedPublication);
        if (!mSucceeded) {
          ZS_LOG_WARNING(Detail, log("received a result but result had no document"))
          mErrorCode = 404;
          mErrorReason = "Not found";
          cancel();
          return true;
        }

        PublicationRepositoryPtr outer = mOuter.lock();
        if (outer) {
          outer->notifyFetched(mThisWeak.lock());
        }

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        if (requester != mRequester) return;
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Fetcher => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::Fetcher::log(const char *message) const
      {
        return String("PublicationRepository::Fetcher [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      IMessageRequesterPtr PublicationRepository::Fetcher::getRequester() const
      {
        AutoRecursiveLock lock(getLock());
        return mRequester;
      }

      //-----------------------------------------------------------------------
      RecursiveLock &PublicationRepository::Fetcher::getLock() const
      {
        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Remover
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::Remover::Remover(
                                              IMessageQueuePtr queue,
                                              PublicationRepositoryPtr outer,
                                              IPublicationRemoverDelegatePtr delegate,
                                              IPublicationForPublicationRepositoryPtr publication
                                              ) :
        MessageQueueAssociator(queue),
        mOuter(outer),
        mDelegate(IPublicationRemoverDelegateProxy::createWeak(delegate)),
        mPublication(publication),
        mSucceeded(false),
        mErrorCode(0)
      {
        ZS_LOG_DEBUG(log("created") + publication->getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::init()
      {
      }

      //-----------------------------------------------------------------------
      PublicationRepository::Remover::~Remover()
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
      #pragma mark PublicationRepository::Remover => friend PublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::RemoverPtr PublicationRepository::Remover::create(
                                                                               IMessageQueuePtr queue,
                                                                               PublicationRepositoryPtr outer,
                                                                               IPublicationRemoverDelegatePtr delegate,
                                                                               IPublicationForPublicationRepositoryPtr publication
                                                                               )
      {
        RemoverPtr pThis(new Remover(queue, outer, delegate, publication));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::setRequester(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        mRequester = requester;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::notifyCompleted()
      {
        ZS_LOG_DEBUG(log("notified complete"))
        AutoRecursiveLock lock(getLock());
        mSucceeded = true;
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Remover => IPublicationRemover
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        if (mRequester) {
          mRequester->cancel();
          mRequester.reset();
        }

        RemoverPtr pThis = mThisWeak.lock();

        if (pThis) {
          if (mDelegate) {
            try {
              mDelegate->onPublicationRemoverComplete(pThis);
            } catch(IPublicationRemoverDelegateProxy::Exceptions::DelegateGone &) {
            }
          }
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::Remover::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::Remover::wasSuccessful() const
      {
        AutoRecursiveLock lock(getLock());
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      WORD PublicationRepository::Remover::getErrorResult() const
      {
        AutoRecursiveLock lock(getLock());
        return mErrorCode;
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::Remover::getErrorReason() const
      {
        AutoRecursiveLock lock(getLock());
        return mErrorReason;
      }

      //-----------------------------------------------------------------------
      IPublicationPtr PublicationRepository::Remover::getPublication() const
      {
        AutoRecursiveLock lock(getLock());
        return mPublication->convertIPublication();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Remover => IMessageRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::Remover::handleMessageRequesterMessageReceived(
                                                                                 IMessageRequesterPtr requester,
                                                                                 message::MessagePtr message
                                                                                 )
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received result but already cancelled"))
          return false;
        }
        if (mRequester != requester) {
          ZS_LOG_WARNING(Detail, log("received result but requester does not match"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expected result but received something else"))
          return false;
        }

        if ((message::MessageFactoryStack::Methods)message->method() != message::MessageFactoryStack::Method_PeerDelete) {
          ZS_LOG_WARNING(Detail, log("expected peer delete result but received something else"))
          return false;
        }

        message::MessageResultPtr result = message::MessageResult::convert(message);
        if (result->hasError()) {
          ZS_LOG_WARNING(Detail, log("received error in result"))
          mSucceeded = false;
          mErrorCode = result->errorCode();
          mErrorReason = result->errorReason();
          return true;
        }

        message::PeerDeleteResultPtr deleteResult = message::PeerDeleteResult::convert(result);
        mSucceeded = true;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (outer) {
          outer->notifyRemoved(mThisWeak.lock());
        }

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        if (requester != mRequester) return;
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Remover => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &PublicationRepository::Remover::getLock() const
      {
        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::Remover::log(const char *message) const
      {
        return String("PublicationRepository::Remover [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::SubscriptionLocal
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::SubscriptionLocal::SubscriptionLocal(
                                                                  IMessageQueuePtr queue,
                                                                  PublicationRepositoryPtr outer,
                                                                  IPublicationSubscriptionDelegatePtr delegate,
                                                                  const char *publicationPath,
                                                                  const SubscribeToRelationshipsMap &relationships
                                                                  ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IPublicationSubscriptionDelegateProxy::createWeak(delegate)),
        mSubscriptionInfo(IPublicationMetaDataForPublicationRepository::create(0, 0, 0, IPublicationMetaData::Source_Local, "", "", publicationPath, "", IPublicationMetaData::Encoding_XML, relationships)),
        mCurrentState(IPublicationSubscription::PublicationSubscriptionState_Pending)
      {
        ZS_LOG_DEBUG(log("created") + mSubscriptionInfo->getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::SubscriptionLocal::init()
      {
        setState(IPublicationSubscription::PublicationSubscriptionState_Pending);
      }

      //-----------------------------------------------------------------------
      PublicationRepository::SubscriptionLocal::~SubscriptionLocal()
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
      #pragma mark PublicationRepository::SubscriptionLocal => friend SubscriptionLocal
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::SubscriptionLocalPtr PublicationRepository::SubscriptionLocal::create(
                                                                                                   IMessageQueuePtr queue,
                                                                                                   PublicationRepositoryPtr outer,
                                                                                                   IPublicationSubscriptionDelegatePtr delegate,
                                                                                                   const char *publicationPath,
                                                                                                   const SubscribeToRelationshipsMap &relationships
                                                                                                   )
      {
        SubscriptionLocalPtr pThis(new SubscriptionLocal(queue, outer, delegate, publicationPath, relationships));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::SubscriptionLocal::notifyUpdated(IPublicationForPublicationRepositoryPtr publication)
      {
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("receive notification of updated document but location subscription is cancelled"))
          return;
        }

        ZS_LOG_TRACE(log("publication is updated") + publication->getDebugValuesString())

        String name = publication->convertIPublication()->getName();
        String path = mSubscriptionInfo->getName();

        if (name.length() < path.length()) {
          ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
          ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication update notification as publication respository is gone"))
          return;
        }

        IAccountForPublicationRepositoryPtr account = outer->getOuter();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication update notification as account is gone"))
          return;
        }

        String subscriberContactID = account->getContactID(); // the subscriber is "ourself"

        if (!outer->canSubscribeToPublisher(
                                            publication->getCreatorContactID(),
                                            publication->getRelationships(),
                                            subscriberContactID,
                                            mSubscriptionInfo->getRelationships())) {
          ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
          return;
        }

        ZS_LOG_DEBUG(log("notifying about publication update to local subscriber") + publication->getDebugValuesString())

        // valid to notify about this document...
        try {
          mDelegate->onPublicationSubscriptionPublicationUpdated(mThisWeak.lock(), publication->convertIPublication());
        } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::SubscriptionLocal::notifyGone(IPublicationForPublicationRepositoryPtr publication)
      {
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received notification that publication is gone but local subscription is already cancelled"))
          return;
        }

        ZS_LOG_TRACE(log("notified publication is gone") + publication->getDebugValuesString())

        String name = publication->getName();
        String path = mSubscriptionInfo->getName();

        if (name.length() < path.length()) {
          ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
          ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication gone notification as publication respository is gone"))
          return;
        }

        IAccountForPublicationRepositoryPtr account = outer->getOuter();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication gone notification as account is gone"))
          return;
        }

        String currentContactID = account->getContactID();

        if (!outer->canSubscribeToPublisher(
                                            publication->getCreatorContactID(),
                                            publication->getRelationships(),
                                            currentContactID,
                                            mSubscriptionInfo->getRelationships())) {
          ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
          return;
        }

        ZS_LOG_DEBUG(log("notifying about publication gone to local subscriber") + publication->getDebugValuesString())

        // valid to notify about this document...
        try {
          mDelegate->onPublicationSubscriptionPublicationGone(mThisWeak.lock(), publication->convertIPublication());
        } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::SubscriptionLocal => IPublicationSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::SubscriptionLocal::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))
        AutoRecursiveLock lock(getLock());
        setState(IPublicationSubscription::PublicationSubscriptionState_Shutdown);
        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      IPublicationSubscription::PublicationSubscriptionStates PublicationRepository::SubscriptionLocal::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataPtr PublicationRepository::SubscriptionLocal::getSource() const
      {
        AutoRecursiveLock lock(getLock());
        return mSubscriptionInfo->convertIPublicationMetaData();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::SubscriptionLocal => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &PublicationRepository::SubscriptionLocal::getLock() const
      {
        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::SubscriptionLocal::log(const char *message) const
      {
        return String("PublicationRepository::SubscriptionLocal [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::SubscriptionLocal::setState(PublicationSubscriptionStates state)
      {
        if (mCurrentState == state) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + IPublicationSubscription::toString(mCurrentState) + ", new state=" + IPublicationSubscription::toString(state))

        mCurrentState = state;

        SubscriptionLocalPtr pThis = mThisWeak.lock();

        if (IPublicationSubscription::PublicationSubscriptionState_Shutdown == mCurrentState) {
          PublicationRepositoryPtr outer = mOuter.lock();
          if ((outer) &&
              (pThis)) {
            outer->notifyLocalSubscriptionShutdown(pThis);
          }
        }

        if (!mDelegate) {
          ZS_LOG_DEBUG(log("unable to notify subscriber of state change as delegate object is null"))
          return;
        }

        if (pThis) {
          try {
            mDelegate->onPublicationSubscriptionStateChanged(pThis, state);
          } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_DEBUG(log("unable to notify subscriber of state change as delegate is gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionIncoming
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionIncoming::PeerSubscriptionIncoming(
                                                                                IMessageQueuePtr queue,
                                                                                PublicationRepositoryPtr outer,
                                                                                PeerSourcePtr peerSource,
                                                                                IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                                                                ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mPeerSource(peerSource),
        mSubscriptionInfo(subscriptionInfo)
      {
        ZS_LOG_DEBUG(log("created") + subscriptionInfo->getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::init()
      {
      }

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionIncoming::~PeerSubscriptionIncoming()
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
      #pragma mark PublicationRepository::PeerSubscriptionIncoming => friend PublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionIncomingPtr PublicationRepository::PeerSubscriptionIncoming::create(
                                                                                                                 IMessageQueuePtr queue,
                                                                                                                 PublicationRepositoryPtr outer,
                                                                                                                 PeerSourcePtr peerSource,
                                                                                                                 IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                                                                                                 )
      {
        PeerSubscriptionIncomingPtr pThis(new PeerSubscriptionIncoming(queue, outer, peerSource, subscriptionInfo));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyUpdated(IPublicationForPublicationRepositoryPtr publication)
      {
        CachedPublicationMap tempCache;
        tempCache[publication] = publication;

        notifyUpdated(tempCache);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyGone(IPublicationForPublicationRepositoryPtr publication)
      {
        CachedPublicationMap tempCache;
        tempCache[publication] = publication;

        notifyGone(tempCache);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyUpdated(const CachedPublicationMap &cachedPublications)
      {
        typedef message::PeerPublishNotifyRequest::PublicationList PublicationList;

        AutoRecursiveLock lock(getLock());

        PublicationList list;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication update notification as publication respository is gone"))
          return;
        }

        IAccountForPublicationRepositoryPtr account = outer->getOuter();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication update notification as account is gone"))
          return;
        }

        for (CachedPublicationMap::const_iterator iter = cachedPublications.begin(); iter != cachedPublications.end(); ++iter)
        {
          const IPublicationForPublicationRepositoryPtr &publication = (*iter).second;
          ZS_LOG_TRACE(log("notified of updated publication") + publication->getDebugValuesString())

          String name = publication->getName();
          String path = mSubscriptionInfo->getName();

          if (name.length() < path.length()) {
            ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }

          if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
            ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }

          String subscriberContactID = mSubscriptionInfo->getCreatorContactID(); // the subscriber is the person who created this subscription

          if (!outer->canSubscribeToPublisher(
                                              publication->getCreatorContactID(),
                                              publication->getRelationships(),
                                              subscriberContactID,
                                              mSubscriptionInfo->getRelationships())) {
            ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
            continue;
          }

          ZS_LOG_DEBUG(log("notifying about publication updated to subscriber") + publication->getDebugValuesString())

          list.push_back(publication->convertIPublication());
        }

        if (list.size() < 1) {
          ZS_LOG_TRACE(log("no publications updates are needed to be sent to this subscriber") + mSubscriptionInfo->getDebugValuesString())
          return;
        }

        ZS_LOG_TRACE(log("publications will be notified to this subscriber") + ", total  publications=" + Stringize<size_t>(list.size()).string() + mSubscriptionInfo->getDebugValuesString())

        message::PeerPublishNotifyRequestPtr request = message::PeerPublishNotifyRequest::create();
        request->publicationList(list);
        request->peerCache(PeerCache::find(mPeerSource, outer));

        IMessageRequesterPtr requester;
        if (mSubscriptionInfo->getSource() == IPublicationMetaData::Source_Finder) {
          requester = account->sendFinderRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));
        } else {
          requester = account->sendPeerRequest(mThisWeak.lock(), request, mSubscriptionInfo->getCreatorContactID(), mSubscriptionInfo->getCreatorLocationID(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));
        }
        mNotificationRequesters.push_back(requester);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyGone(const CachedPublicationMap &cachedPublications)
      {
        typedef message::PeerPublishNotifyRequest::PublicationList PublicationList;

        AutoRecursiveLock lock(getLock());

        PublicationList list;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication gone notification as publication respository is gone"))
          return;
        }

        IAccountForPublicationRepositoryPtr account = outer->getOuter();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication gone notification as account is gone"))
          return;
        }

        for (CachedPublicationMap::const_iterator iter = cachedPublications.begin(); iter != cachedPublications.end(); ++iter)
        {
          const IPublicationForPublicationRepositoryPtr &publication = (*iter).second;

          String name = publication->getName();
          String path = mSubscriptionInfo->getName();

          if (name.length() < path.length()) {
            ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }
          if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
            ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }

          String currentContactID = mSubscriptionInfo->getPublishedToContactID();

          if (!outer->canSubscribeToPublisher(
                                              publication->getCreatorContactID(),
                                              publication->getRelationships(),
                                              currentContactID,
                                              mSubscriptionInfo->getRelationships())) {
            ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
            continue;
          }

          ZS_LOG_DEBUG(log("notifying about publication gone to subscriber") + publication->getDebugValuesString())

          IPublicationMetaDataForPublicationRepositoryPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(publication->convertIPublicationMetaData());
          metaData->setVersion(0);
          metaData->setBaseVersion(0);

          list.push_back(metaData->convertIPublicationMetaData());
        }

        if (list.size() < 1) {
          ZS_LOG_TRACE(log("no 'publications are gone' notification will be send to this subscriber") + mSubscriptionInfo->getDebugValuesString())
          return;
        }

        ZS_LOG_TRACE(log("'publications are gone' notification will be notified to this subscriber") + ", total  publications=" + Stringize<size_t>(list.size()).string() + mSubscriptionInfo->getDebugValuesString())

        message::PeerPublishNotifyRequestPtr request = message::PeerPublishNotifyRequest::create();
        request->publicationList(list);

        message::PeerPublishNotifyRequestPtr message = message::PeerPublishNotifyRequest::create();

        IMessageRequesterPtr requester;
        if (mSubscriptionInfo->getSource() == IPublicationMetaData::Source_Finder) {
          requester = account->sendFinderRequest(mThisWeak.lock(), message, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));
        } else {
          requester = account->sendPeerRequest(mThisWeak.lock(), request, mSubscriptionInfo->getCreatorContactID(), mSubscriptionInfo->getCreatorLocationID(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));
        }
        mNotificationRequesters.push_back(requester);
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataPtr PublicationRepository::PeerSubscriptionIncoming::getSource() const
      {
        return mSubscriptionInfo->convertIPublicationMetaData();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        for (NotificationRequesterList::iterator iter = mNotificationRequesters.begin(); iter != mNotificationRequesters.end(); ++iter)
        {
          IMessageRequesterPtr &notifyRequester = (*iter);
          notifyRequester->cancel();
        }
        mNotificationRequesters.clear();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionIncoming => IMessageRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::PeerSubscriptionIncoming::handleMessageRequesterMessageReceived(
                                                                                                  IMessageRequesterPtr requester,
                                                                                                  message::MessagePtr message
                                                                                                  )
      {
        ZS_LOG_DEBUG(log("received notification that notification was sent"))

        for (NotificationRequesterList::iterator iter = mNotificationRequesters.begin(); iter != mNotificationRequesters.end(); ++iter)
        {
          IMessageRequesterPtr &notifyRequester = (*iter);
          if (notifyRequester == requester) {
            ZS_LOG_TRACE(log("found requester for subscription notification request"))
            // doesn't matter if it was successful or not because there is nothing we can do either way...
            mNotificationRequesters.erase(iter);
            return true;
          }
        }
        ZS_LOG_WARNING(Detail, log("unable to find requester for subscription notification request"))
        return false;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        ZS_LOG_DEBUG(log("subscription notification time out"))
        for (NotificationRequesterList::iterator iter = mNotificationRequesters.begin(); iter != mNotificationRequesters.end(); ++iter)
        {
          IMessageRequesterPtr &notifyRequester = (*iter);
          if (notifyRequester == requester) {
            ZS_LOG_DEBUG(log("subscription notification found thus removing"))

            // doesn't matter if it was successful or not because there is nothing we can do either way...
            mNotificationRequesters.erase(iter);
            return;
          }
        }
        ZS_LOG_DEBUG(log("unable to find requester matching subscription notification request after time out"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionIncoming => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &PublicationRepository::PeerSubscriptionIncoming::getLock() const
      {
        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::PeerSubscriptionIncoming::log(const char *message) const
      {
        return String("PublicationRepository::PeerSubscriptionIncoming [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionOutgoing
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionOutgoing::PeerSubscriptionOutgoing(
                                                                                IMessageQueuePtr queue,
                                                                                PublicationRepositoryPtr outer,
                                                                                IPublicationSubscriptionDelegatePtr delegate,
                                                                                IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                                                                ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IPublicationSubscriptionDelegateProxy::createWeak(delegate)),
        mCurrentState(PublicationSubscriptionState_Pending),
        mSubscriptionInfo(subscriptionInfo)
      {
        ZS_LOG_DEBUG(log("created") + mSubscriptionInfo->getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::init()
      {
      }

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionOutgoing::~PeerSubscriptionOutgoing()
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
      #pragma mark PublicationRepository::PeerSubscriptionOutgoing => friend PublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionOutgoingPtr PublicationRepository::PeerSubscriptionOutgoing::create(
                                                                                                                 IMessageQueuePtr queue,
                                                                                                                 PublicationRepositoryPtr outer,
                                                                                                                 IPublicationSubscriptionDelegatePtr delegate,
                                                                                                                 IPublicationMetaDataForPublicationRepositoryPtr subscriptionInfo
                                                                                                                 )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!subscriptionInfo)

        PeerSubscriptionOutgoingPtr pThis(new PeerSubscriptionOutgoing(queue, outer, delegate, subscriptionInfo));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::setRequester(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        mRequester = requester;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::notifyUpdated(IPublicationMetaDataForPublicationRepositoryPtr metaData)
      {
        AutoRecursiveLock lock(getLock());

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("notification up an updated document after cancel called") + metaData->getDebugValuesString())
          return;
        }

        String name = metaData->getName();
        String path = mSubscriptionInfo->getName();

        if (name.length() < path.length()) {
          ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
          ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        if (mSubscriptionInfo->getSource() != metaData->getSource()) {
          ZS_LOG_TRACE(log("publication update/gone notification for a source other than where subscription was placed (thus ignoring)") + metaData->getDebugValuesString())
          return;
        }

        if (IPublicationMetaData::Source_Peer == mSubscriptionInfo->getSource())
        {
          if ((mSubscriptionInfo->getPublishedToContactID() != metaData->getPublishedToContactID()) ||
              (mSubscriptionInfo->getPublishedToLocationID() != metaData->getPublishedToLocationID()))
          {
            ZS_LOG_TRACE(log("publication update/gone notification arrived for a peer subscription other than where subscription was placed (thus ignoring)") + metaData->getDebugValuesString())
            return;
          }
        }

        ZS_LOG_DEBUG(log("publication update/gone notification is being notified to outgoing subscription delegate") + metaData->getDebugValuesString())

        // this appears to be a match thus notify the subscriber...
        try {
          if (0 == metaData->getVersion()) {
            mDelegate->onPublicationSubscriptionPublicationGone(mThisWeak.lock(), metaData->convertIPublicationMetaData());
          } else {
            mDelegate->onPublicationSubscriptionPublicationUpdated(mThisWeak.lock(), metaData->convertIPublicationMetaData());
          }
        } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionOutgoing => IPublicationSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationSubscription::PublicationSubscriptionStates PublicationRepository::PeerSubscriptionOutgoing::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataPtr PublicationRepository::PeerSubscriptionOutgoing::getSource() const
      {
        AutoRecursiveLock lock(getLock());
        return mSubscriptionInfo->convertIPublicationMetaData();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionOutgoing => IMessageRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::PeerSubscriptionOutgoing::handleMessageRequesterMessageReceived(
                                                                                                  IMessageRequesterPtr requester,
                                                                                                  message::MessagePtr message
                                                                                                  )
      {
        AutoRecursiveLock lock(getLock());

        if (requester == mCancelRequester) {
          ZS_LOG_DEBUG(log("cancel requester completed"))
          mCancelRequester->cancel();
          cancel();
          return true;
        }

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received subscription notification reply but cancel already called"))
          return false;
        }
        if (mRequester != requester) {
          ZS_LOG_WARNING(Detail, log("received subscription notification reply but requester does not match reply"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expecting to receive result but received something else"))
          return false;
        }

        if ((message::MessageFactoryStack::Methods)message->method() != message::MessageFactoryStack::Method_PeerSubscribe) {
          ZS_LOG_WARNING(Detail, log("expecting to receive peer subscribe result but received something else"))
          return false;
        }

        message::MessageResultPtr result = message::MessageResult::convert(message);
        if (result->hasError()) {
          ZS_LOG_WARNING(Detail, log("failed to place outgoing subscription"))
          mSucceeded = false;
          mErrorCode = result->errorCode();
          mErrorReason = result->errorReason();
          cancel();
          return true;
        }

        ZS_LOG_DEBUG(log("outgoing subscription succeeded"))

        message::PeerSubscribeResultPtr subscribeResult = message::PeerSubscribeResult::convert(result);
        mSucceeded = true;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (outer) {
          outer->notifySubscribed(mThisWeak.lock());
        }

        setState(IPublicationSubscription::PublicationSubscriptionState_Established);
        if (mRequester) {
          mRequester->cancel();
          mRequester.reset();
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());
        if (requester == mCancelRequester)
        {
          ZS_LOG_DEBUG(log("cancel requester timeout received"))
          cancel();
          return;
        }
        if (requester != mRequester) {
          ZS_LOG_WARNING(Detail, log("received timeout on subscription request but it wasn't for the subscription request sent"))
          return;
        }
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionOutgoing => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &PublicationRepository::PeerSubscriptionOutgoing::getLock() const
      {
        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::PeerSubscriptionOutgoing::log(const char *message) const
      {
        return String("PublicationRepository::PeerSubscriptionOutgoing [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::setState(PublicationSubscriptionStates state)
      {
        if (mCurrentState == state) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + IPublicationSubscription::toString(mCurrentState) + ", new state=" + IPublicationSubscription::toString(state))

        mCurrentState = state;

        PeerSubscriptionOutgoingPtr pThis = mThisWeak.lock();

        if (IPublicationSubscription::PublicationSubscriptionState_Shutdown == mCurrentState) {
          PublicationRepositoryPtr outer = mOuter.lock();
          if ((outer) &&
              (pThis)) {
            outer->notifyPeerOutgoingSubscriptionShutdown(pThis);
          }
        }

        if (!mDelegate) return;

        if (pThis) {
          try {
            mDelegate->onPublicationSubscriptionStateChanged(pThis, state);
          } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          }
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        setState(IPublicationSubscription::PublicationSubscriptionState_ShuttingDown);

        PeerSubscriptionOutgoingPtr pThis = mThisWeak.lock();

        if (!mGracefulShutdownReference) mGracefulShutdownReference = pThis;

        if ((!mCancelRequester) &&
            (pThis)) {

          PublicationRepositoryPtr outerRepo = mOuter.lock();

          if (outerRepo) {
            IAccountForPublicationRepositoryPtr outer = outerRepo->getOuter();

            if (outer) {
              ZS_LOG_DEBUG(log("sending request to cancel outgoing subscription"))

              message::PeerSubscribeRequestPtr message = message::PeerSubscribeRequest::create();

              IPublicationMetaData::SubscribeToRelationshipsMap empty;
              message->publicationMetaData(mSubscriptionInfo->convertIPublicationMetaData());

              if (IPublicationMetaData::Source_Finder == mSubscriptionInfo->getSource()) {
                mCancelRequester = outer->sendFinderRequest(pThis, message, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));
              } else {
                mCancelRequester = outer->sendPeerRequest(pThis, message, mSubscriptionInfo->getPublishedToContactID(), mSubscriptionInfo->getPublishedToLocationID(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));
              }
              return;
            }
          }
        }

        if (pThis) {
          if (mCancelRequester)  {
            if (!mCancelRequester->isComplete()) {
              ZS_LOG_DEBUG(log("waiting for cancel requester to complete"))
              return;
            }
          }
        }

        setState(IPublicationSubscription::PublicationSubscriptionState_Shutdown);
        mDelegate.reset();

        mGracefulShutdownReference.reset();
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPublicationSubscription
    #pragma mark

    const char *IPublicationSubscription::toString(PublicationSubscriptionStates state)
    {
      switch (state) {
        case PublicationSubscriptionState_Pending:        return "Pending";
        case PublicationSubscriptionState_Established:    return "Established";
        case PublicationSubscriptionState_ShuttingDown:   return "Shutting down";
        case PublicationSubscriptionState_Shutdown:       return "Shutdown";
      }
      return "UNDEFINED";
    }
  }
}
