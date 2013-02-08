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

#include <hookflash/stack/internal/stack_PublicationRepository.h>
#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_Publication.h>
#include <hookflash/stack/internal/stack_PublicationMetaData.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/internal/stack_Location.h>
#include <hookflash/stack/internal/stack_Peer.h>
#include <hookflash/stack/internal/stack_Stack.h>
#include <hookflash/stack/internal/stack_Diff.h>
#include <hookflash/stack/IMessageIncoming.h>

#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/message/peer-common/PeerPublishRequest.h>
#include <hookflash/stack/message/peer-common/PeerPublishResult.h>
#include <hookflash/stack/message/peer-common/PeerGetRequest.h>
#include <hookflash/stack/message/peer-common/PeerGetResult.h>
#include <hookflash/stack/message/peer-common/PeerDeleteRequest.h>
#include <hookflash/stack/message/peer-common/PeerDeleteResult.h>
#include <hookflash/stack/message/peer-common/PeerSubscribeRequest.h>
#include <hookflash/stack/message/peer-common/PeerSubscribeResult.h>
#include <hookflash/stack/message/peer-common/PeerPublishNotifyRequest.h>
#include <hookflash/stack/message/peer-common/PeerPublishNotifyResult.h>

#include <hookflash/stack/message/MessageResult.h>

#include <zsLib/XML.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>

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

      using message::IMessageHelper;

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
      #pragma mark
      #pragma mark (heleprs)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationRepositoryForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepositoryPtr IPublicationRepositoryForAccount::create(AccountPtr account)
      {
        return PublicationRepository::create(account);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::CacheCompare
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::CacheCompare::operator()(const PublicationMetaDataPtr &x, const PublicationMetaDataPtr &y) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!x)
        ZS_THROW_INVALID_ARGUMENT_IF(!y)
        return x->forRepo().isLessThan(y->forRepo().toPublicationMetaData());
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
                                                   AccountPtr account
                                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mAccount(account)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::init()
      {
        AccountPtr account = mAccount.lock();
        ZS_THROW_BAD_STATE_IF(!account)

        mPeerSubscription = IPeerSubscription::subscribeAll(account, mThisWeak.lock());
        mExpiresTimer = Timer::create(mThisWeak.lock(), Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRES_TIMER_IN_SECONDS));

        ZS_LOG_BASIC(log("init"))
      }

      //-----------------------------------------------------------------------
      PublicationRepository::~PublicationRepository()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      PublicationRepositoryPtr PublicationRepository::convert(IPublicationRepositoryPtr repository)
      {
        return boost::dynamic_pointer_cast<PublicationRepository>(repository);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => IPublicationRepositoryForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationRepositoryPtr PublicationRepository::create(AccountPtr account)
      {
        PublicationRepositoryPtr pThis(new PublicationRepository(IStackForInternal::queueStack(), account));
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
      String PublicationRepository::toDebugString(IPublicationRepositoryPtr repository, bool includeCommaPrefix)
      {
        if (!repository) return includeCommaPrefix ? String(", repository=(null)") : String("repository=(null)");
        return PublicationRepository::convert(repository)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      PublicationRepositoryPtr PublicationRepository::getFromAccount(IAccountPtr inAccount)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)
        AccountPtr account = Account::convert(inAccount);

        return account->forRepo().getRepository();
      }

      //-----------------------------------------------------------------------
      IPublicationPublisherPtr PublicationRepository::publish(
                                                              IPublicationPublisherDelegatePtr delegate,
                                                              IPublicationPtr inPublication
                                                              )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!inPublication)

        AutoRecursiveLock lock(getLock());

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot publish document as account object is gone"))
          return IPublicationPublisherPtr();
        }

        PublicationPtr publication = Publication::convert(inPublication);

        PublisherPtr publisher = Publisher::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, publication);

        LocationPtr publishedLocation = publication->forRepo().getPublishedLocation();

        if (ILocation::LocationType_Local == publishedLocation->forRepo().getLocationType()) {
          ZS_LOG_DEBUG(log("publicaton is local thus can publish immediately") + publication->forRepo().getDebugValuesString())

          publication->forRepo().setBaseVersion(inPublication->getVersion());

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
            CachedPublicationPermissionMap::iterator found = mCachedPermissionDocuments.find(publication->forRepo().getName());
            if (found != mCachedPermissionDocuments.end()) {
              ZS_LOG_DEBUG(log("previous permission publication found thus removing old map entry"))
              mCachedPermissionDocuments.erase(found);
            }
          }

          mCachedLocalPublications[publication] = publication;
          mCachedPermissionDocuments[publication->forRepo().getName()] = publication;

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

        ZS_LOG_DEBUG(log("publication requires publishing to an external source") + publication->forRepo().getDebugValuesString())

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

        PublicationMetaDataPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(inMetaData);

        ZS_LOG_DEBUG(log("requesting to fetch publication") + metaData->forRepo().getDebugValuesString())

        FetcherPtr fetcher = Fetcher::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, metaData);

        LocationPtr publishedLocation = metaData->forRepo().getPublishedLocation();

        if (ILocation::LocationType_Local == publishedLocation->forRepo().getLocationType()) {
          CachedPublicationMap::iterator found = mCachedLocalPublications.find(metaData);
          if (found != mCachedLocalPublications.end()) {
            PublicationPtr existingPublication = (*found).second;
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

        PublicationPtr publication = Publication::convert(inPublication);

        ZS_LOG_DEBUG(log("requesting to remove publication") + publication->forRepo().getDebugValuesString())

        RemoverPtr remover = Remover::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, publication);

        AccountPtr account = mAccount.lock();
        if (!account) {
          remover->cancel();  // sorry, this could not be completed...
          return remover;
        }

        LocationPtr publishedLocation = publication->forRepo().getPublishedLocation();

        switch (publishedLocation->forRepo().getLocationType()) {
          case ILocation::LocationType_Local: {

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
              CachedPublicationPermissionMap::iterator found = mCachedPermissionDocuments.find(publication->forRepo().getName());
              if (found != mCachedPermissionDocuments.end()) {
                PublicationPtr &existingPublication = (*found).second;

                if (existingPublication->forRepo().getLineage() == publication->forRepo().getLineage()) {
                  ZS_LOG_DEBUG(log("found permission publication to erase and removing now"))
                  mCachedPermissionDocuments.erase(found);
                  wasErased = true;
                } else {
                  ZS_LOG_DEBUG(log("found permission publication but it doesn't have the same lineage thus will not erase") + existingPublication->forRepo().getDebugValuesString())
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
          case ILocation::LocationType_Finder:
          case ILocation::LocationType_Peer:
          {
            PeerDeleteRequestPtr request = PeerDeleteRequest::create();
            request->publicationMetaData(publication->forRepo().toPublicationMetaData());
            request->domain(account->forRepo().getDomain());

            ZS_LOG_DEBUG(log("requesting to remove remote finder publication"))

            remover->setMonitor(IMessageMonitor::monitorAndSendToLocation(remover, publishedLocation, request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
            break;
          }
        }

        return remover;
      }

      //-----------------------------------------------------------------------
      IPublicationSubscriptionPtr PublicationRepository::subscribe(
                                                                   IPublicationSubscriptionDelegatePtr delegate,
                                                                   ILocationPtr inSubscribeToLocation,
                                                                   const char *publicationPath,
                                                                   const SubscribeToRelationshipsMap &relationships
                                                                   )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!inSubscribeToLocation)
        ZS_THROW_INVALID_ARGUMENT_IF(!publicationPath)

        AutoRecursiveLock lock(getLock());

        LocationPtr subscribeToLocation = Location::convert(inSubscribeToLocation);

        ZS_LOG_DEBUG(log("creating subcription") + ", publication path=" + Stringize<CSTR>(publicationPath).string() + subscribeToLocation->forRepo().getDebugValueString())

        AccountPtr account = mAccount.lock();

        switch (subscribeToLocation->forRepo().getLocationType()) {
          case ILocation::LocationType_Local: {
            SubscriptionLocalPtr subscriber = SubscriptionLocal::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, publicationPath, relationships);
            if (!account) {
              ZS_LOG_WARNING(Detail, log("subscription must be cancelled as account is gone"))
              subscriber->cancel();  // sorry, this could not be completed...
              return subscriber;
            }
            for (CachedPublicationMap::iterator iter = mCachedLocalPublications.begin(); iter != mCachedLocalPublications.end(); ++iter)
            {
              PublicationPtr publication = (*iter).second;
              ZS_LOG_TRACE(log("notifying location subcription about document") + publication->forRepo().getDebugValuesString())
              subscriber->notifyUpdated(publication);
            }
            return subscriber;
          }
          case ILocation::LocationType_Finder:
          case ILocation::LocationType_Peer:  {
            if (!account) {
              SubscriptionLocalPtr subscriber = SubscriptionLocal::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, publicationPath, relationships);
              ZS_LOG_WARNING(Detail, log("subscription must be cancelled as account is gone"))
              return subscriber;
            }

            LocationPtr localLocation = ILocationForPublicationRepository::getForLocal(account);

            PublicationMetaDataPtr metaData = IPublicationMetaDataForPublicationRepository::create(0, 0, 0, localLocation, publicationPath, "", IPublicationMetaData::Encoding_JSON, relationships, subscribeToLocation);

            PeerSubscriptionOutgoingPtr subscriber = PeerSubscriptionOutgoing::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, metaData);

            PeerSubscribeRequestPtr request = PeerSubscribeRequest::create();
            request->domain(account->forRepo().getDomain());

            request->publicationMetaData(metaData->forRepo().toPublicationMetaData());

            subscriber->setMonitor(IMessageMonitor::monitorAndSendToLocation(subscriber, subscribeToLocation, request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));

            mPeerSubscriptionsOutgoing[subscriber->getID()] = subscriber;
            ZS_LOG_TRACE(log("outgoing subscription is created"))
            return subscriber;
          }
        }

        ZS_LOG_WARNING(Detail, log("subscribing to unknown location type") + subscribeToLocation->forRepo().getDebugValueString())
        return IPublicationSubscriptionPtr();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository => IPeerSubscriptionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void PublicationRepository::onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription)
      {
        AutoRecursiveLock lock(getLock());

        if (subscription != mPeerSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring connection subscription shutdown on obsolete subscription"))
          return;
        }

        ZS_LOG_DEBUG(log("connection subscription shutdown"))
        mPeerSubscription.reset();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onPeerSubscriptionFindStateChanged(
                                                                     IPeerSubscriptionPtr subscription,
                                                                     IPeerPtr peer,
                                                                     PeerFindStates state
                                                                     )
      {
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onPeerSubscriptionLocationConnectionStateChanged(
                                                                                   IPeerSubscriptionPtr subscription,
                                                                                   ILocationPtr inLocation,
                                                                                   LocationConnectionStates state
                                                                                   )
      {
        AutoRecursiveLock lock(getLock());
        if (subscription != mPeerSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring peer subscription location connection state change on obsolete subscription"))
          return;
        }

        Time recommendedExpires = zsLib::now() + Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_EXPIRE_DISCONNECTED_REMOTE_PUBLICATIONS_IN_SECONDS);

        LocationPtr location = Location::convert(inLocation);

        ZS_LOG_DEBUG(log("peer location state changed") + ", state=" + ILocation::toString(state) + location->forRepo().getDebugValueString())
        switch (state)
        {
          case ILocation::LocationConnectionState_Pending:        break;
          case ILocation::LocationConnectionState_Connected:      {

            // remove every remote document expiry for the peer...
            for (CachedPublicationMap::iterator iter = mCachedRemotePublications.begin(); iter != mCachedRemotePublications.end(); ++iter)
            {
              PublicationPtr &publication = (*iter).second;

              const char *ignoredReaon = NULL;
              if (0 == ILocationForPublicationRepository::locationCompare(location, publication->forRepo().getPublishedLocation(), ignoredReaon)) {
                ZS_LOG_TRACE(log("removing expiry time on document") + ", recommend=" + Stringize<Time>(recommendedExpires).string() + publication->forRepo().getDebugValuesString())
                publication->forRepo().setExpires(Time());
              }
            }

            // remove the peer source expiry
            {
              PeerSourcePtr peerSource = IPublicationMetaDataForPublicationRepository::createForSource(location);
              CachedPeerSourceMap::iterator found = mCachedPeerSources.find(peerSource);
              if (found != mCachedPeerSources.end()) {
                PeerCachePtr &peerCache = (*found).second;

                ZS_LOG_DEBUG(log("removing the peer source cache expiry") + ", recommended=" + Stringize<Time>(recommendedExpires).string())
                peerCache->setExpires(Time());
              }
            }
            break;
          }
          case ILocation::LocationConnectionState_Disconnecting:
          case ILocation::LocationConnectionState_Disconnected:   {

            // everything published from a remote peer into the local cache must be removed
            for (CachedPublicationMap::iterator pubIter = mCachedLocalPublications.begin(); pubIter != mCachedLocalPublications.end(); )
            {
              CachedPublicationMap::iterator current = pubIter;
              ++pubIter;

              PublicationPtr &publication = (*current).second;

              const char *ignoredReaon = NULL;

              if (0 == ILocationForPublicationRepository::locationCompare(publication->forRepo().getCreatorLocation(), location, ignoredReaon)) {
                ZS_LOG_DEBUG(log("removing publication published by peer") + publication->forRepo().getDebugValuesString())

                // remove this document from the cache
                mCachedLocalPublications.erase(current);
              }
            }

            // set every remote document downloaded from peer to expire after a period of time for the peer...
            for (CachedPublicationMap::iterator iter = mCachedRemotePublications.begin(); iter != mCachedRemotePublications.end(); ++iter)
            {
              PublicationPtr &publication = (*iter).second;

              const char *ignoredReaon = NULL;
              if (0 == ILocationForPublicationRepository::locationCompare(publication->forRepo().getPublishedLocation(), location, ignoredReaon)) {
                ZS_LOG_TRACE(log("setting expiry time on document") + ", recommend=" + Stringize<Time>(recommendedExpires).string() + publication->forRepo().getDebugValuesString())
                publication->forRepo().setExpires(recommendedExpires);
              }
            }

            // set the peer source expiry (peer source represents what this side believes the remote side has downloaded from this peer and cached already)
            {
              PeerSourcePtr peerSource = IPublicationMetaDataForPublicationRepository::createForSource(location);
              CachedPeerSourceMap::iterator found = mCachedPeerSources.find(peerSource);
              if (found != mCachedPeerSources.end()) {

                if (ILocation::LocationType_Finder == location->forRepo().getLocationType()) {
                  // the finder would immediately forget all downloaded publications from this local location upon disconnect so remove the entire cache representation...
                  mCachedPeerSources.erase(found);
                } else {
                  PeerCachePtr &peerCache = (*found).second;

                  ZS_LOG_DEBUG(log("setting the peer source cache to expire at the recommended time") + ", recommended=" + Stringize<Time>(recommendedExpires).string())
                  peerCache->setExpires(recommendedExpires);
                }
              }
            }

            // clean all outgoing subscriptions going to the peer...
            for (PeerSubscriptionOutgoingMap::iterator subIter = mPeerSubscriptionsOutgoing.begin(); subIter != mPeerSubscriptionsOutgoing.end(); )
            {
              PeerSubscriptionOutgoingMap::iterator current = subIter;
              ++subIter;

              PeerSubscriptionOutgoingPtr &outgoing = (*current).second;
              IPublicationMetaDataPtr source = outgoing->getSource();

              const char *ignoredReaon = NULL;
              if (0 == ILocationForPublicationRepository::locationCompare(source->getPublishedLocation(), location, ignoredReaon)) {
                // cancel this subscription since its no longer valid
                ZS_LOG_DEBUG(log("shutting down outgoing peer subscription") + ", id=" + Stringize<PUID>(outgoing->getID()).string())

                outgoing->cancel();
                mPeerSubscriptionsOutgoing.erase(current);
              }
            }

            // clean all incoming subscriptions coming from the peer...
            for (PeerSubscriptionIncomingMap::iterator subIter = mPeerSubscriptionsIncoming.begin(); subIter != mPeerSubscriptionsIncoming.end(); )
            {
              PeerSubscriptionIncomingMap::iterator current = subIter;
              ++subIter;

              PeerSubscriptionIncomingPtr &incoming = (*current).second;
              IPublicationMetaDataPtr source = incoming->getSource();
              const char *ignoredReaon = NULL;
              if (0 == ILocationForPublicationRepository::locationCompare(source->getCreatorLocation(), location, ignoredReaon)) {
                // cancel this subscription since its no longer valid
                ZS_LOG_DEBUG(log("shutting down incoming subscriptions coming from the peer") + ", id=" + Stringize<PUID>(incoming->getID()).string())
                incoming->cancel();
                mPeerSubscriptionsIncoming.erase(current);
              }
            }
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onPeerSubscriptionMessageIncoming(
                                                                    IPeerSubscriptionPtr subscription,
                                                                    IMessageIncomingPtr messageIncoming
                                                                    )
      {
        LocationPtr location = Location::convert(messageIncoming->getLocation());
        message::MessagePtr message = messageIncoming->getMessage();
        ZS_LOG_TRACE(log("received notification of incoming message") + ", message ID=" + message->messageID() +  + ", type=" + message::Message::toString(message->messageType()) + ", method=" + message->methodAsString() + location->forRepo().getDebugValueString())

        AutoRecursiveLock lock(getLock());
        if (subscription != mPeerSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring peer subscription incoming message on obsolete subscription"))
          return;
        }

        switch (message->messageType()) {
          case message::Message::MessageType_Request:
          case message::Message::MessageType_Notify:  break;
          default:                                    {
            ZS_LOG_WARNING(Detail, log("ignoring message that is neither a request nor a notification"))
            return;
          }
        }

        switch ((MessageFactoryPeerCommon::Methods)message->method()) {
          case MessageFactoryPeerCommon::Method_PeerPublish:        onMessageIncoming(messageIncoming, PeerPublishRequest::convert(message)); break;
          case MessageFactoryPeerCommon::Method_PeerGet:            onMessageIncoming(messageIncoming, PeerGetRequest::convert(message)); break;
          case MessageFactoryPeerCommon::Method_PeerDelete:         onMessageIncoming(messageIncoming, PeerDeleteRequest::convert(message)); break;
          case MessageFactoryPeerCommon::Method_PeerSubscribe:      onMessageIncoming(messageIncoming, PeerSubscribeRequest::convert(message)); break;
          case MessageFactoryPeerCommon::Method_PeerPublishNotify:  onMessageIncoming(messageIncoming, PeerPublishNotifyRequest::convert(message)); break;
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

          PublicationPtr &publication = (*current).second;

          Time expires = publication->forRepo().getExpires();
          Time cacheExpires = publication->forRepo().getCacheExpires();

          if (Time() == expires) {
            expires = cacheExpires;
          }
          if (Time() == cacheExpires) {
            cacheExpires = expires;
          }

          if (Time() == expires) {
            ZS_LOG_TRACE(log("publication does not have an expiry") + publication->forRepo().getDebugValuesString())
            continue;
          }

          if (cacheExpires < expires)
            expires = cacheExpires;

          if (expires < tick) {
            ZS_LOG_DEBUG(log("document is now expiring") + publication->forRepo().getDebugValuesString())
            mCachedRemotePublications.erase(current);
            continue;
          }

          ZS_LOG_TRACE(log("publication is not expirying yet") + publication->forRepo().getDebugValuesString())
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
            ZS_LOG_TRACE(log("peer source does not have an expiry") + peerSource->forRepo().getDebugValuesString())
            continue;
          }

          if (expires < tick) {
            ZS_LOG_DEBUG(log("peer source is now expiring") + peerSource->forRepo().getDebugValuesString())
            mCachedPeerSources.erase(current);
            continue;
          }

          ZS_LOG_TRACE(log("peer source is not expirying yet") + peerSource->forRepo().getDebugValuesString())
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

        PublicationPtr publication;

        // find and remove the fetcher from the pending list...
        for (PendingPublisherList::iterator iter = mPendingPublishers.begin(); iter != mPendingPublishers.end(); ++iter)
        {
          PublisherPtr &foundPublisher = (*iter);
          if (foundPublisher->getID() == publisher->getID()) {
            publication = Publication::convert(foundPublisher->getPublication());
            ZS_LOG_DEBUG(log("removing remote publisher") + ", publisher ID=" + Stringize<PUID>(publisher->getID()).string() + publication->forRepo().getDebugValuesString())
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

        PublicationMetaDataPtr metaData = PublicationMetaData::convert(fetcher->getPublicationMetaData());

        ZS_LOG_DEBUG(log("publication was fetched") + ", fetcher ID=" + Stringize<PUID>(fetcher->getID()).string() + metaData->forRepo().getDebugValuesString())

        PublicationPtr publication = Publication::convert(fetcher->getFetchedPublication());

        // force the creator/published to to be the correct locations
        publication->forRepo().setCreatorLocation(metaData->forRepo().getCreatorLocation());
        publication->forRepo().setPublishedLocation(metaData->forRepo().getPublishedLocation());

        CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);
        if (found != mCachedRemotePublications.end()) {
          PublicationPtr &existingPublication = (*found).second;

          ZS_LOG_DEBUG(log("existing internal publication found thus updating") + existingPublication->forRepo().getDebugValuesString())
          try {
            existingPublication->forRepo().updateFromFetchedPublication(publication);

            // override what the fetcher thinks is returned and replace with the existing document
            fetcher->setPublication(existingPublication);
          } catch(IPublicationForPublicationRepository::Exceptions::VersionMismatch &) {
            ZS_LOG_ERROR(Detail, log("version fetched is not compatible with the version already known"))
          }
        } else {
          ZS_LOG_DEBUG(log("new entry for cache will be created since existing publication in cache was not found") + publication->forRepo().getDebugValuesString())
          mCachedRemotePublications[publication] = publication;

          ZS_LOG_DEBUG(log("publication inserted into remote cache") + ", remote cache total=" + Stringize<size_t>(mCachedRemotePublications.size()).string())
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::notifyFetcherCancelled(FetcherPtr fetcher)
      {
        AutoRecursiveLock lock(getLock());

        PublicationMetaDataPtr metaData;

        // find and remove the fetcher from the pending list...
        for (PendingFetcherList::iterator iter = mPendingFetchers.begin(); iter != mPendingFetchers.end(); ++iter)
        {
          FetcherPtr &foundFetcher = (*iter);
          if (foundFetcher->getID() == fetcher->getID()) {
            ZS_LOG_DEBUG(log("fetcher is being removed from pending fetchers list") + ", fetcher ID=" + Stringize<PUID>(fetcher->getID()).string())
            metaData = PublicationMetaData::convert(fetcher->getPublicationMetaData());
            mPendingFetchers.erase(iter);
            break;
          }
        }

        if (metaData) {
          ZS_LOG_DEBUG(log("will attempt to activate next fetcher based on previous fetch's publication meta data") + metaData->forRepo().getDebugValuesString())
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
        PublicationMetaDataPtr metaData = PublicationMetaData::convert(remover->getPublication());
        CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);
        if (found == mCachedRemotePublications.end()) {
          ZS_LOG_DEBUG(log("unable to locate publication in 'remote' cache") + metaData->forRepo().getDebugValuesString())
          return;
        }

        PublicationPtr &existingPublication = (*found).second;

        ZS_LOG_DEBUG(log("removing remotely cached publication") + existingPublication->forRepo().getDebugValuesString())
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
        typedef String PeerURI;
        typedef std::map<PeerURI, PeerURI> PeerURIMap;

        typedef IPublicationMetaData::DocumentName DocumentName;
        typedef IPublicationMetaData::PermissionAndPeerURIListPair PermissionAndPeerURIListPair;
        typedef IPublicationMetaData::PeerURIList PeerURIList;

        PeerURIMap contacts;

        for (PublishToRelationshipsMap::const_iterator iter = publishToRelationships.begin(); iter != publishToRelationships.end(); ++iter)
        {
          DocumentName name = (*iter).first;

          ZS_LOG_TRACE(log("resolving relationships for document") + ", name=" + name)

          const PermissionAndPeerURIListPair &permissionPair = (*iter).second;
          const PeerURIList &diffContacts = permissionPair.second;

          PublicationPtr relationshipsPublication;

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

          RelationshipListPtr docContacts = relationshipsPublication->forRepo().getAsContactList();

          switch (permissionPair.first) {
            case IPublicationMetaData::Permission_All:    {
              for (RelationshipList::iterator relIter = (*docContacts).begin(); relIter != (*docContacts).end(); ++relIter) {
                ZS_LOG_TRACE(log("adding all contacts found in relationships document") + ", name=" + name + ", peer URI=" + (*relIter))
                contacts[(*relIter)] = (*relIter);
              }
              break;
            }
            case IPublicationMetaData::Permission_None:   {
              for (RelationshipList::iterator relIter = (*docContacts).begin(); relIter != (*docContacts).end(); ++relIter) {
                PeerURIMap::iterator found = contacts.find(*relIter);
                if (found == contacts.end()) {
                  ZS_LOG_TRACE(log("failed to remove all contacts found in relationships document") + ", name=" + name + ", peer URI=" + (*relIter))
                  continue;
                }
                ZS_LOG_TRACE(log("removing all contacts found in relationships document") + ", name=" + name + ", peer URI=" + (*relIter))
                contacts.erase(found);
              }
              break;
            }
            case IPublicationMetaData::Permission_Add:
            case IPublicationMetaData::Permission_Some:   {
              for (RelationshipList::const_iterator diffIter = diffContacts.begin(); diffIter != diffContacts.end(); ++diffIter) {
                PeerURIList::const_iterator found = find((*docContacts).begin(), (*docContacts).end(), (*diffIter));
                if (found == (*docContacts).end()) {
                  ZS_LOG_TRACE(log("cannot add some of the contacts found in relationships document") + ", name=" + name + ", peer URI=" + (*diffIter))
                  continue; // cannot add anyone that isn't part of the relationship list
                }
                ZS_LOG_TRACE(log("adding some of the contacts found in relationships document") + ", name=" + name + ", peer URI=" + (*diffIter))
                contacts[(*diffIter)] = (*diffIter);
              }
              break;
            }
            case IPublicationMetaData::Permission_Remove: {
              for (RelationshipList::const_iterator diffIter = diffContacts.begin(); diffIter != diffContacts.end(); ++diffIter) {
                PeerURIList::const_iterator found = find((*docContacts).begin(), (*docContacts).end(), (*diffIter));
                if (found == (*docContacts).end()) {
                  ZS_LOG_TRACE(log("cannot remove some of the contacts found in relationships document") + ", name=" + name + ", peer URI=" + (*diffIter))
                  continue; // cannot remove anyone that isn't part of the relationship list
                }

                PeerURIMap::iterator foundExisting = contacts.find(*diffIter);
                if (foundExisting == contacts.end()) {
                  ZS_LOG_TRACE(log("cannot removing some of the contacts found as the contact was never added to relationships document") + ", name=" + name + ", peer URI=" + (*diffIter))
                  continue;
                }
                ZS_LOG_TRACE(log("removing some of the contacts found in relationships document") + ", name=" + name + ", peer URI=" + (*diffIter))
                contacts.erase(foundExisting);
              }
              break;
            }
          }
        }

        for (PeerURIMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
          ZS_LOG_TRACE(log("final list of the resolved relationships contains this contact") + ", peer URI=" + (*iter).first)
          outContacts.push_back((*iter).first);
        }
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::canFetchPublication(
                                                      const PublishToRelationshipsMap &publishToRelationships,
                                                      LocationPtr location
                                                      ) const
      {
        switch (location->forRepo().getLocationType()) {
          case ILocation::LocationType_Local:   return true;    // local is always allowed to fetch the publication
          case ILocation::LocationType_Finder:  return false;   // finder cannot fetch publications
          case ILocation::LocationType_Peer:    break;
        }

        PeerPtr peer = location->forRepo().getPeer();
        if (!peer) {
          ZS_LOG_ERROR(Detail, log("peer contact on incoming message was empty") + location->forRepo().getDebugValueString())
          return false;
        }

        String peerURI = peer->forRepo().getPeerURI();

        RelationshipList publishToContacts;  // all these contacts are being published to
        resolveRelationships(publishToRelationships, publishToContacts);

        bool found = false;
        // the document must publish to this contact or its ignored...
        for (RelationshipList::iterator iter = publishToContacts.begin(); iter != publishToContacts.end(); ++iter)
        {
          if ((*iter) == peerURI) {
            found = true;
            break;
          }
        }

        if (!found) {
          ZS_LOG_WARNING(Detail, log("publication is not published to this fetcher contact") + ", fetcher peer URI=" + peerURI)
          return false; // does not publish to this contact...
        }

        ZS_LOG_TRACE(log("publication is publishing to this fetcher contact (thus safe to fetch)") + ", fetcher peer URI=" + peerURI)
        return true;
      }

      //-----------------------------------------------------------------------
      bool PublicationRepository::canSubscribeToPublisher(
                                                          LocationPtr publicationCreatorLocation,
                                                          const PublishToRelationshipsMap &publishToRelationships,
                                                          LocationPtr subscriberLocation,
                                                          const SubscribeToRelationshipsMap &subscribeToRelationships
                                                          ) const
      {
        RelationshipList publishToContacts;  // all these contacts are being published to
        resolveRelationships(publishToRelationships, publishToContacts);

        PeerPtr publicationPeer = publicationCreatorLocation->forRepo().getPeer();
        PeerPtr subscriberPeer = subscriberLocation->forRepo().getPeer();

        if ((!publicationPeer) ||
            (!subscriberPeer)) {
          ZS_LOG_TRACE(log("publisher is not publishing to this subscriber contact") + ", publication: " + publicationCreatorLocation->forRepo().getDebugValueString(false) + ", subscriber: " + subscriberLocation->forRepo().getDebugValueString(false))
          return false;
        }

        bool found = false;
        // the document must publish to this contact or its ignored...
        for (RelationshipList::iterator iter = publishToContacts.begin(); iter != publishToContacts.end(); ++iter)
        {
          if ((*iter) == subscriberPeer->forRepo().getPeerURI()) {
            found = true;
            break;
          }
        }

        if (!found) {
          ZS_LOG_TRACE(log("publisher is not publishing to this subscriber contact") + ", publication: " + publicationCreatorLocation->forRepo().getDebugValueString(false) + ", subscriber: " + subscriberLocation->forRepo().getDebugValueString(false))
          return false; // does not publish to this contact...
        }

        RelationshipList subscribeToContacts; // all these cotnacts are being subscribed to
        resolveRelationships(subscribeToRelationships, subscribeToContacts);

        found = false;
        // the document must publish to this contact or its ignored...
        for (RelationshipList::iterator iter = subscribeToContacts.begin(); iter != subscribeToContacts.end(); ++iter)
        {
          if ((*iter) == publicationPeer->forRepo().getPeerURI()) {
            found = true;
            break;
          }
        }

        if (!found) {
          ZS_LOG_TRACE(log("subscriber is not subscribing to this publisher") + ", publication: " + publicationCreatorLocation->forRepo().getDebugValueString(false) + ", subscriber: " + subscriberLocation->forRepo().getDebugValueString(false))
          return false; // does not publish to this contact...
        }

        ZS_LOG_TRACE(log("subscriber is subscribing to this publication's creator and creator is publishing this publication to the subscriber") + ", publication: " + publicationCreatorLocation->forRepo().getDebugValueString(false) + ", subscriber: " + subscriberLocation->forRepo().getDebugValueString(false))
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
        return String("PublicationRepository [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String PublicationRepository::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("repository id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("cached local", mCachedLocalPublications.size() > 0 ? Stringize<size_t>(mCachedLocalPublications.size()).string() : String(), firstTime) +
               Helper::getDebugValue("cached remote", mCachedRemotePublications.size() > 0 ? Stringize<size_t>(mCachedRemotePublications.size()).string() : String(), firstTime) +
               IPeerSubscription::toDebugString(mPeerSubscription) +
               Helper::getDebugValue("cached permissions", mCachedPermissionDocuments.size() > 0 ? Stringize<size_t>(mCachedPermissionDocuments.size()).string() : String(), firstTime) +
               Helper::getDebugValue("subscriptions local", mSubscriptionsLocal.size() > 0 ? Stringize<size_t>(mSubscriptionsLocal.size()).string() : String(), firstTime) +
               Helper::getDebugValue("subscriptions incoming", mPeerSubscriptionsIncoming.size() > 0 ? Stringize<size_t>(mPeerSubscriptionsIncoming.size()).string() : String(), firstTime) +
               Helper::getDebugValue("subscriptions outgoing", mPeerSubscriptionsOutgoing.size() > 0 ? Stringize<size_t>(mPeerSubscriptionsOutgoing.size()).string() : String(), firstTime) +
               Helper::getDebugValue("pending fetchers", mPendingFetchers.size() > 0 ? Stringize<size_t>(mPendingFetchers.size()).string() : String(), firstTime) +
               Helper::getDebugValue("pending publishers", mPendingPublishers.size() > 0 ? Stringize<size_t>(mPendingPublishers.size()).string() : String(), firstTime) +
               Helper::getDebugValue("cached peer sources", mCachedPeerSources.size() > 0 ? Stringize<size_t>(mCachedPeerSources.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::activateFetcher(PublicationMetaDataPtr metaData)
      {
        AutoRecursiveLock lock(getLock());

        if (mPendingFetchers.size() < 1) {
          ZS_LOG_DEBUG(log("no pending fetchers to activate..."))
          return;
        }

        ZS_LOG_DEBUG(log("activating next fetcher found with this meta data") + metaData->forRepo().getDebugValuesString())

        for (PendingFetcherList::iterator iter = mPendingFetchers.begin(); iter != mPendingFetchers.end(); ++iter)
        {
          FetcherPtr &fetcher = (*iter);

          PublicationMetaDataPtr fetcherMetaData = PublicationMetaData::convert(fetcher->getPublicationMetaData());

          ZS_LOG_TRACE(log("comparing against fetcher's meta data") + fetcherMetaData->forRepo().getDebugValuesString())

          if (!metaData->forRepo().isMatching(fetcherMetaData->forRepo().toPublicationMetaData())) {
            ZS_LOG_TRACE(log("activation meta data does not match fetcher"))
            continue;
          }

          // this is an exact match...
          ZS_LOG_DEBUG(log("an exact match of the fetcher's meta data was found thus will attempt a fetch now"))

          // if already has a monitor then already activated
          if (fetcher->getMonitor()) {
            ZS_LOG_DEBUG(log("cannot activate next fetcher as fetcher is already activated"))
            return;
          }

          CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);

          if (found != mCachedRemotePublications.end()) {
            PublicationPtr &existingPublication = (*found).second;

            ZS_LOG_TRACE(log("existing publication found in 'remote' cache for meta data") + existingPublication->forRepo().getDebugValuesString())
            ULONG fetchingVersion = metaData->forRepo().getVersion();
            if (existingPublication->forRepo().getVersion() >= fetchingVersion) {
              ZS_LOG_DETAIL(log("short circuit the fetch since the document is already in our cache") + existingPublication->forRepo().getDebugValuesString())

              fetcher->setPublication(existingPublication);
              fetcher->notifyCompleted();
              return;
            }

            metaData->forRepo().setVersion(existingPublication->forRepo().getVersion());
          } else {
            ZS_LOG_TRACE(log("existing publication was not found in 'remote' cache"))
            metaData->forRepo().setVersion(0);
          }

          // find the contacts to publish to...
          AccountPtr account = mAccount.lock();
          if (!account) {
            ZS_LOG_WARNING(Detail, log("cannot fetch publication as account object is gone"))
            fetcher->cancel();  // sorry, this could not be completed...
            mPendingFetchers.erase(iter);
            return;
          }
          PeerGetRequestPtr request = PeerGetRequest::create();
          request->domain(account->forRepo().getDomain());
          request->publicationMetaData(metaData->forRepo().toPublicationMetaData());

          fetcher->setMonitor(IMessageMonitor::monitorAndSendToLocation(fetcher, metaData->forRepo().getPublishedLocation(), request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
          return;
        }

        ZS_LOG_DEBUG(log("no pending fetchers of this type were found"))
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::activatePublisher(PublicationPtr publication)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("attempting to activate next publisher for publication") + publication->forRepo().getDebugValuesString())

        for (PendingPublisherList::iterator iter = mPendingPublishers.begin(); iter != mPendingPublishers.end(); ++iter)
        {
          PublisherPtr &publisher = (*iter);
          PublicationPtr publisherPublication = Publication::convert(publisher->getPublication());

          if (publication->forRepo().getID() != publisherPublication->forRepo().getID()) {
            ZS_LOG_TRACE(log("pending publication is not the correct one to activate") + Stringize<PUID>(publisherPublication->forRepo().getID()).string())
            continue;
          }

          ZS_LOG_DEBUG(log("found the correct publisher to activate and will attempt to activate it now"))

          AccountPtr account = mAccount.lock();
          if (!account) {
            ZS_LOG_WARNING(Detail, log("cannot activate next publisher as account object is gone"))
            mPendingPublishers.erase(iter);
            publisher->cancel();  // sorry, this could not be completed...
            return;
          }

          if (publisher->getMonitor()) {
            ZS_LOG_DEBUG(log("cannot active the next publisher as it is already activated"))
            return;
          }

          PeerPublishRequestPtr request = PeerPublishRequest::create();
          request->domain(account->forRepo().getDomain());
          request->publication(publication);
          request->publishedFromVersion(publication->forRepo().getBaseVersion());
          request->publishedToVersion(publication->forRepo().getVersion());

          publisher->setMonitor(IMessageMonitor::monitorAndSendToLocation(publisher, publication->forRepo().getPublishedLocation(), request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS)));
          return;
        }

        ZS_LOG_DEBUG(log("no pending publishers of this type were found"))
      }

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionIncomingPtr PublicationRepository::findIncomingSubscription(PublicationMetaDataPtr inMetaData) const
      {
        LocationPtr location = inMetaData->forRepo().getCreatorLocation();
        ZS_LOG_TRACE(log("finding incoming peer subscription with publication path, publication name=") + inMetaData->forRepo().getName() + location->forRepo().getDebugValueString())

        for (PeerSubscriptionIncomingMap::const_iterator iter = mPeerSubscriptionsIncoming.begin(); iter != mPeerSubscriptionsIncoming.end(); ++iter)
        {
          const PeerSubscriptionIncomingPtr &subscription = (*iter).second;
          IPublicationMetaDataPtr metaData = subscription->getSource();

          if (inMetaData->forRepo().getName() != metaData->getName()) continue;

          const char *ignoreReason = NULL;
          if (0 != ILocationForPublicationRepository::locationCompare(inMetaData->forRepo().getCreatorLocation(), metaData->getCreatorLocation(), ignoreReason)) continue;

          ZS_LOG_TRACE(log("incoming peer subscription was found"))
          return subscription;
        }
        return PeerSubscriptionIncomingPtr();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onMessageIncoming(
                                                    IMessageIncomingPtr messageIncoming,
                                                    PeerPublishRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        ZS_LOG_DEBUG(log("incoming peer publish request"))

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer publish request as account object is gone"))
          return;
        }

        PublicationPtr publication = Publication::convert(request->publication());
        publication->forRepo().setCreatorLocation(Location::convert(messageIncoming->getLocation()));
        publication->forRepo().setPublishedLocation(ILocationForPublicationRepository::getForLocal(account));

        ZS_LOG_TRACE(log("incoming request to publish document") + publication->forRepo().getDebugValuesString())

        CachedPublicationMap::iterator found = mCachedLocalPublications.find(publication);

        PublicationMetaDataPtr responceMetaData;
        if (found != mCachedLocalPublications.end()) {
          PublicationPtr &existingPublication = (*found).second;

          ZS_LOG_DEBUG(log("updating existing publication in 'local' cache with remote publication") + existingPublication->forRepo().getDebugValuesString())
          try {
            existingPublication->forRepo().updateFromFetchedPublication(publication);
          } catch(IPublicationForPublicationRepository::Exceptions::VersionMismatch &) {
            ZS_LOG_WARNING(Detail, log("cannot update with the publication published since the versions are incompatible"))
            message::MessageResultPtr errorResult = message::MessageResult::create(request, 409, "Conflict");
            messageIncoming->sendResponse(errorResult);
            return;
          }

          responceMetaData = existingPublication;
        } else {
          ZS_LOG_DEBUG(log("creating new entry in 'local' cache for remote publication") + publication->forRepo().getDebugValuesString())
          mCachedLocalPublications[publication] = publication;
          responceMetaData = publication;

          ZS_LOG_DEBUG(log("publication inserted into local cache") + ", local cache total=" + Stringize<size_t>(mCachedLocalPublications.size()).string())
        }

        PeerPublishResultPtr reply = PeerPublishResult::create(request);
        reply->publicationMetaData(responceMetaData->forRepo().toPublicationMetaData());
        messageIncoming->sendResponse(reply);

        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        // HERE - notify other subscriptions?
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onMessageIncoming(
                                                    IMessageIncomingPtr messageIncoming,
                                                    PeerGetRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer get request as account object is gone"))
          return;
        }

        LocationPtr location = Location::convert(messageIncoming->getLocation());

        PublicationMetaDataPtr metaData = PublicationMetaData::convert(request->publicationMetaData());

        PeerSourcePtr sourceMetaData = IPublicationMetaDataForPublicationRepository::createForSource(location);

        // the publication must have be published to "this" repository...
        metaData->forRepo().setPublishedLocation(ILocationForPublicationRepository::getForLocal(account));

        ZS_LOG_DEBUG(log("incoming request to get a document published to the local cache") + metaData->forRepo().getDebugValuesString())

        CachedPublicationMap::iterator found = mCachedLocalPublications.find(metaData);
        if (found == mCachedLocalPublications.end()) {
          ZS_LOG_WARNING(Detail, log("failed to find publicatin thus ignoring get request"))
          return;
        }

        PublicationPtr &existingPublication = (*found).second;

        if (!canFetchPublication(
                                 existingPublication->forRepo().getRelationships(),
                                 location)) {
          ZS_LOG_WARNING(Detail, log("publication is not published to the peer requesting the document (thus unable to reply to fetch request)"))
          return;
        }

        ZS_LOG_TRACE(log("requesting peer has authorization to get the requested document"))

        ZS_LOG_TRACE(log("incoming get request to will return this document") + existingPublication->forRepo().getDebugValuesString())

        PeerCachePtr peerCache = PeerCache::find(sourceMetaData, mThisWeak.lock());
        peerCache->notifyFetched(existingPublication);

        PeerGetResultPtr reply = PeerGetResult::create(request);
        reply->publication(existingPublication);
        messageIncoming->sendResponse(reply);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onMessageIncoming(
                                                    IMessageIncomingPtr messageIncoming,
                                                    PeerDeleteRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer delete request as account object is gone"))
          return;
        }

        IPublicationMetaDataPtr requestMetaData = request->publicationMetaData();
        PublicationMetaDataPtr metaData = PublicationMetaData::convert(requestMetaData);

        metaData->forRepo().setCreatorLocation(Location::convert(messageIncoming->getLocation()));
        metaData->forRepo().setPublishedLocation(ILocationForPublicationRepository::getForLocal(account));

        ZS_LOG_DEBUG(log("incoming request to delete document") + metaData->forRepo().getDebugValuesString())

        CachedPublicationMap::iterator found = mCachedLocalPublications.find(metaData);
        if (found == mCachedLocalPublications.end()) {
          ZS_LOG_WARNING(Detail, log("failed to find publicatin thus ignoring delete request"))
          return;
        }

        PublicationPtr &existingPublication = (*found).second;

        ZS_LOG_DEBUG(log("delete request will delete this publication") + existingPublication->forRepo().getDebugValuesString())

        mCachedLocalPublications.erase(found);

        PeerDeleteResultPtr reply = PeerDeleteResult::create(request);
        messageIncoming->sendResponse(reply);

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
      void PublicationRepository::onMessageIncoming(
                                                    IMessageIncomingPtr messageIncoming,
                                                    PeerSubscribeRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer subscribe request as account object is gone"))
          return;
        }

        LocationPtr location = Location::convert(messageIncoming->getLocation());

        PublicationMetaDataPtr metaData = PublicationMetaData::convert(request->publicationMetaData());

        if (!location->forRepo().getPeer()) {
          ZS_LOG_WARNING(Detail, log("incoming request from a location that does not have a peer (thus ignoring)") + ", location: " + location->forRepo().getDebugValueString() + ", publication: " + metaData->forRepo().getDebugValuesString(false))
          return;
        }

        metaData->forRepo().setCreatorLocation(location);
        metaData->forRepo().setPublishedLocation(ILocationForPublicationRepository::getForLocal(account));

        ZS_LOG_DEBUG(log("incoming request to subscribe to document path") + metaData->forRepo().getDebugValuesString())

        PeerSubscriptionIncomingPtr existingSubscription = findIncomingSubscription(metaData);
        if (existingSubscription) {
          ZS_LOG_TRACE(log("removing existing subscription of the same source"))

          PeerSubscriptionIncomingMap::iterator found = mPeerSubscriptionsIncoming.find(existingSubscription->getID());
          ZS_THROW_BAD_STATE_IF(found == mPeerSubscriptionsIncoming.end())

          mPeerSubscriptionsIncoming.erase(found);
        }

        // send the reply now...
        PeerSubscribeResultPtr reply = PeerSubscribeResult::create(request);
        reply->publicationMetaData(metaData->forRepo().toPublicationMetaData());
        messageIncoming->sendResponse(reply);

        const IPublicationMetaData::PublishToRelationshipsMap &relationships = metaData->forRepo().getRelationships();
        if (relationships.size() > 0) {
          ZS_LOG_TRACE(log("incoming subscription is being created"))

          PeerSourcePtr sourceMetaData = IPublicationMetaDataForPublicationRepository::createForSource(location);

          // if there are relationships then this is an addition not a removal
          PeerSubscriptionIncomingPtr incoming = PeerSubscriptionIncoming::create(getAssociatedMessageQueue(), mThisWeak.lock(), sourceMetaData, metaData);

          mPeerSubscriptionsIncoming[incoming->getID()] = incoming;

          ZS_LOG_TRACE(log("notifying of all cached publications published to the new incoming party (so that all notifications can arrive in one message)"))
          incoming->notifyUpdated(mCachedLocalPublications);
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::onMessageIncoming(
                                                    IMessageIncomingPtr messageIncoming,
                                                    PeerPublishNotifyRequestPtr request
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!request)

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_TRACE(log("cannot respond to incoming peer notify request as account object is gone"))
          return;
        }

        // send the reply now...
        PeerPublishNotifyResultPtr reply = PeerPublishNotifyResult::create(request);
        messageIncoming->sendResponse(reply);

        typedef PeerPublishNotifyRequest::PublicationList PublicationList;

        const PublicationList &publicationList = request->publicationList();

        LocationPtr location = Location::convert(messageIncoming->getLocation());

        ZS_LOG_DEBUG(log("received publish notification") + ", total publications=" + Stringize<size_t>(publicationList.size()).string() + location->forRepo().getDebugValueString())

        for (PublicationList::const_iterator iter = publicationList.begin(); iter != publicationList.end(); ++iter) {
          const IPublicationMetaDataPtr &requestMetaData = (*iter);
          PublicationMetaDataPtr metaData = PublicationMetaData::convert(requestMetaData);

          ZS_LOG_DEBUG(log("received notification of document change") + metaData->forRepo().getDebugValuesString())

          metaData->forRepo().setPublishedLocation(location);

          PublicationPtr publication = Publication::convert(metaData->forRepo().toPublication());
          if (publication) {
            ZS_LOG_DEBUG(log("publication was included with notification") + publication->forRepo().getDebugValuesString())

            // not only is meta data available an update to the publication is available, search the cache for the document
            CachedPublicationMap::iterator found = mCachedRemotePublications.find(metaData);;
            if (found != mCachedRemotePublications.end()) {
              PublicationPtr &existingPublication = (*found).second;
              ZS_LOG_DEBUG(log("existing internal publication found thus updating (if possible)") + existingPublication->forRepo().getDebugValuesString())
              try {
                existingPublication->forRepo().updateFromFetchedPublication(publication);
              } catch(IPublicationForPublicationRepository::Exceptions::VersionMismatch &) {
                ZS_LOG_WARNING(Detail, log("version from the notify does not match our last version (thus ignoring change)"))
              }
            } else {
              bool okayToCreate = true;
              AutoRecursiveLockPtr docLock;
              DocumentPtr doc = publication->forRepo().getJSON(docLock);
              if (doc) {
                ElementPtr diffEl = doc->findFirstChildElement(HOOKFLASH_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME);
                okayToCreate = !diffEl;
                if (!okayToCreate) {
                  ZS_LOG_WARNING(Detail, log("new entry for remote cache cannot be created for publication which only contains diff updates") + publication->forRepo().getDebugValuesString())
                }
              }
              if (okayToCreate) {
                ZS_LOG_DEBUG(log("new entry for remote cache will be created since existing publication in cache was not found") + publication->forRepo().getDebugValuesString())
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

        if (mPeerSubscription) {
          mPeerSubscription->cancel();
          mPeerSubscription.reset();
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
            PublicationPtr &publication = (*iter).second;

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
      PeerCachePtr PublicationRepository::PeerCache::convert(IPublicationRepositoryPeerCachePtr cache)
      {
        return boost::dynamic_pointer_cast<PeerCache>(cache);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerCache => IPublicationRepositoryPeerCache
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::PeerCache::toDebugString(IPublicationRepositoryPeerCachePtr cache, bool includeCommaPrefix)
      {
        if (!cache) return includeCommaPrefix ? ", peer cache=(null" : "peer cache=(null)";
        return PeerCache::convert(cache)->getDebugValueString();
      }

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

        PublicationPtr publication = Publication::convert(inPublication);

        CachedPeerPublicationMap::iterator found = mCachedPublications.find(publication);
        if (found != mCachedPublications.end()) {

          // this document was fetched before...
          PublicationMetaDataPtr &metaData = (*found).second;
          metaData->forRepo().setExpires(publication->forRepo().getExpires());  // not used yet but could be used to remember when this document will expire

          ULONG nextVersionToNotify = metaData->forRepo().getVersion() + 1;
          if (nextVersionToNotify > publication->forRepo().getVersion()) {
            ZS_LOG_WARNING(Detail, log("already fetched/notified of this version so why is it being notified? (probably okay and likely because of peer disconnection)") + publication->forRepo().getDebugValuesString())
            return false;
          }

          ULONG outputSize = 0;
          publication->forRepo().getDiffVersionsOutputSize(nextVersionToNotify, publication->forRepo().getVersion(), outputSize);

          if (outputSize > ioMaxSizeAvailableInBytes) {
            ZS_LOG_WARNING(Detail, log("diff document is too large for notify") + ", output size=" + Stringize<ULONG>(outputSize).string() + ", max size=" + Stringize<ULONG>(ioMaxSizeAvailableInBytes).string())
            return false;
          }

          outNotifyFromVersion = nextVersionToNotify;
          outNotifyToVersion = publication->forRepo().getVersion();

          ioMaxSizeAvailableInBytes -= outputSize;

          metaData->forRepo().setVersion(outNotifyToVersion);

          ZS_LOG_DETAIL(log("recommend notify about diff version") +
                        ", from=" + Stringize<ULONG>(outNotifyFromVersion).string() +
                        ", to=" + Stringize<ULONG>(outNotifyToVersion).string()  +
                        ", size=" + Stringize<ULONG>(outputSize).string() +
                        ", remaining=" + Stringize<ULONG>(ioMaxSizeAvailableInBytes).string())
          return true;
        }

        ULONG outputSize = 0;
        publication->forRepo().getEntirePublicationOutputSize(outputSize);

        if (outputSize > ioMaxSizeAvailableInBytes) {
          ZS_LOG_WARNING(Detail, log("diff document is too large for notify") + ", output size=" + Stringize<ULONG>(outputSize).string() + ", max size=" + Stringize<ULONG>(ioMaxSizeAvailableInBytes).string())
          return false;
        }

        PublicationMetaDataPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(publication->forRepo().toPublicationMetaData());
        metaData->forRepo().setExpires(publication->forRepo().getExpires());  // not used yet but could be used to remember when this document will expire
        metaData->forRepo().setBaseVersion(0);
        metaData->forRepo().setVersion(publication->forRepo().getVersion());

        mCachedPublications[metaData] = metaData;

        outNotifyFromVersion = 0;
        outNotifyToVersion = publication->forRepo().getVersion();
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
      void PublicationRepository::PeerCache::notifyFetched(PublicationPtr publication)
      {
        AutoRecursiveLock lock(getLock());

        CachedPeerPublicationMap::iterator found = mCachedPublications.find(publication);
        if (found != mCachedPublications.end()) {
          PublicationMetaDataPtr &metaData = (*found).second;

          // remember up to which version was last fetched
          metaData->forRepo().setVersion(publication->forRepo().getVersion());
          return;
        }

        PublicationMetaDataPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(publication->forRepo().toPublicationMetaData());
        metaData->forRepo().setBaseVersion(0);
        metaData->forRepo().setVersion(publication->forRepo().getVersion());

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
      String PublicationRepository::PeerCache::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("peer cache id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               IPublicationMetaData::toDebugString(mPeerSource) +
               Helper::getDebugValue("expires", Time() != mExpires ? IMessageHelper::timeToString(mExpires) : String(), firstTime) +
               Helper::getDebugValue("cached remote", mCachedPublications.size() > 0 ? Stringize<size_t>(mCachedPublications.size()).string() : String(), firstTime);
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
                                                  PublicationPtr publication
                                                  ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IPublicationPublisherDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mPublication(publication),
        mSucceeded(false),
        mErrorCode(0)
      {
        ZS_LOG_DEBUG(log("created new publisher") + publication->forRepo().getDebugValuesString())
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
                                                                                   PublicationPtr publication
                                                                                   )
      {
        PublisherPtr pThis(new Publisher(queue, outer, delegate, publication));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::setMonitor(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        mMonitor = monitor;
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
      PublisherPtr PublicationRepository::Publisher::convert(IPublicationPublisherPtr publisher)
      {
        return boost::dynamic_pointer_cast<Publisher>(publisher);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Publisher => IPublicationPublisher
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::Publisher::toDebugString(IPublicationPublisherPtr publisher, bool includeCommaPrefix)
      {
        if (!publisher) return includeCommaPrefix ? ", publisher=(null" : "publisher=(null)";
        return Publisher::convert(publisher)->getDebugValueString();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::cancel()
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("cancel called"))

        if (mMonitor) {
          mMonitor->cancel();
          mMonitor.reset();
        }

        PublisherPtr pThis = mThisWeak.lock();

        if (pThis) {
          if (mDelegate) {
            try {
              mDelegate->onPublicationPublisherCompleted(mThisWeak.lock());
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
      bool PublicationRepository::Publisher::wasSuccessful(
                                                           WORD *outErrorResult,
                                                           String *outReason
                                                           ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outErrorResult) *outErrorResult = mErrorCode;
        if (outReason) *outReason = mErrorReason;
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      IPublicationPtr PublicationRepository::Publisher::getPublication() const
      {
        AutoRecursiveLock lock(getLock());
        return mPublication;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Publisher => IMessageMonitorDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::Publisher::handleMessageMonitorMessageReceived(
                                                                                 IMessageMonitorPtr monitor,
                                                                                 message::MessagePtr message
                                                                                 )
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received result but publisher is already cancelled"))
          return false;
        }
        if (mMonitor != monitor) {
          ZS_LOG_WARNING(Detail, log("received result but not for the correct monitor"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expected result but received something else"))
          return false;
        }

        if ((MessageFactoryPeerCommon::Methods)message->method() != MessageFactoryPeerCommon::Method_PeerPublish) {
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

        PeerPublishResultPtr publishResult = PeerPublishResult::convert(result);

        PeerPublishRequestPtr originalRequest = PeerPublishRequest::convert(monitor->getMonitoredMessage());

        // now published from the original base version to the current version
        // so the base is now the published version plus one...
        mPublication->forRepo().setBaseVersion(originalRequest->publishedToVersion()+1);

        notifyCompleted();

        PublicationRepositoryPtr outer = mOuter.lock();
        if (outer) {
          outer->notifyPublished(mThisWeak.lock());
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Publisher::onMessageMonitorTimedOut(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mMonitor) {
          ZS_LOG_DEBUG(log("ignoring publish request time out since it doesn't match monitor"))
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
      String PublicationRepository::Publisher::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("publisher id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               IPublication::toDebugString(mPublication) +
               IMessageMonitor::toDebugString(mMonitor) +
               Helper::getDebugValue("succeeded", mSucceeded ? String("true") : String(), firstTime) +
               Helper::getDebugValue("error code", 0 != mErrorCode ? Stringize<typeof(mErrorCode)>(mErrorCode).string() : String(), firstTime);
               Helper::getDebugValue("error reason", mErrorReason, firstTime);
      }

      //-----------------------------------------------------------------------
      IMessageMonitorPtr PublicationRepository::Publisher::getMonitor() const
      {
        AutoRecursiveLock lock(getLock());
        return mMonitor;
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
                                              PublicationMetaDataPtr metaData
                                              ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IPublicationFetcherDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mPublicationMetaData(metaData),
        mSucceeded(false),
        mErrorCode(0)
      {
        ZS_LOG_DEBUG(log("created") + metaData->forRepo().getDebugValuesString())
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
                                                                               PublicationMetaDataPtr metaData
                                                                               )
      {
        FetcherPtr pThis(new Fetcher(queue, outer, delegate, metaData));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::setPublication(PublicationPtr publication)
      {
        AutoRecursiveLock lock(getLock());
        mFetchedPublication = publication;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::setMonitor(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        mMonitor = monitor;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::notifyCompleted()
      {
        AutoRecursiveLock lock(getLock());
        mSucceeded = true;
        cancel();
      }

      FetcherPtr PublicationRepository::Fetcher::convert(IPublicationFetcherPtr fetcher)
      {
        return boost::dynamic_pointer_cast<Fetcher>(fetcher);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Fetcher => IPublicationFetcher
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::Fetcher::toDebugString(IPublicationFetcherPtr fetcher, bool includeCommaPrefix)
      {
        if (!fetcher) return includeCommaPrefix ? ", publisher=(null" : "publisher=(null)";
        return Fetcher::convert(fetcher)->getDebugValueString();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Fetcher::cancel()
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("cancel called"))

        if (mMonitor) {
          mMonitor->cancel();
          mMonitor.reset();
        }

        FetcherPtr pThis = mThisWeak.lock();

        if (pThis) {
          if (mDelegate) {
            try {
              mDelegate->onPublicationFetcherCompleted(mThisWeak.lock());
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
      bool PublicationRepository::Fetcher::wasSuccessful(
                                                         WORD *outErrorResult,
                                                         String *outReason
                                                         ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outErrorResult) *outErrorResult = mErrorCode;
        if (outReason) *outReason = mErrorReason;
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      IPublicationPtr PublicationRepository::Fetcher::getFetchedPublication() const
      {
        AutoRecursiveLock lock(getLock());
        return mFetchedPublication;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataPtr PublicationRepository::Fetcher::getPublicationMetaData() const
      {
        AutoRecursiveLock lock(getLock());
        return mPublicationMetaData->forRepo().toPublicationMetaData();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Fetcher => IMessageMonitorDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::Fetcher::handleMessageMonitorMessageReceived(
                                                                               IMessageMonitorPtr monitor,
                                                                               message::MessagePtr message
                                                                               )
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received result but fetcher is shutdown"))
          return false;
        }
        if (mMonitor != monitor) {
          ZS_LOG_WARNING(Detail, log("received result but monitor does not match"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expecting result but received something else"))
          return false;
        }

        if ((MessageFactoryPeerCommon::Methods)message->method() != MessageFactoryPeerCommon::Method_PeerGet) {
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

        PeerGetResultPtr getResult = PeerGetResult::convert(result);
        mFetchedPublication = Publication::convert(getResult->publication());

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
      void PublicationRepository::Fetcher::onMessageMonitorTimedOut(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mMonitor) return;
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
      String PublicationRepository::Fetcher::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("fetcher id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               IPublicationMetaData::toDebugString(mPublicationMetaData) +
               IMessageMonitor::toDebugString(mMonitor) +
               Helper::getDebugValue("succeeded", mSucceeded ? String("true") : String(), firstTime) +
               Helper::getDebugValue("error code", 0 != mErrorCode ? Stringize<typeof(mErrorCode)>(mErrorCode).string() : String(), firstTime);
               Helper::getDebugValue("error reason", mErrorReason, firstTime) +
               IPublication::toDebugString(mFetchedPublication);
      }

      //-----------------------------------------------------------------------
      IMessageMonitorPtr PublicationRepository::Fetcher::getMonitor() const
      {
        AutoRecursiveLock lock(getLock());
        return mMonitor;
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
                                              PublicationPtr publication
                                              ) :
        MessageQueueAssociator(queue),
        mOuter(outer),
        mDelegate(IPublicationRemoverDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mPublication(publication),
        mSucceeded(false),
        mErrorCode(0)
      {
        ZS_LOG_DEBUG(log("created") + publication->forRepo().getDebugValuesString())
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
                                                                               PublicationPtr publication
                                                                               )
      {
        RemoverPtr pThis(new Remover(queue, outer, delegate, publication));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::setMonitor(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        mMonitor = monitor;
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
      RemoverPtr PublicationRepository::Remover::convert(IPublicationRemoverPtr remover)
      {
        return boost::dynamic_pointer_cast<Remover>(remover);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Remover => IPublicationRemover
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::Remover::toDebugString(IPublicationRemoverPtr remover, bool includeCommaPrefix)
      {
        if (!remover) return includeCommaPrefix ? ", remover=(null" : "remover=(null)";
        return Remover::convert(remover)->getDebugValueString();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        if (mMonitor) {
          mMonitor->cancel();
          mMonitor.reset();
        }

        RemoverPtr pThis = mThisWeak.lock();

        if (pThis) {
          if (mDelegate) {
            try {
              mDelegate->onPublicationRemoverCompleted(pThis);
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
      bool PublicationRepository::Remover::wasSuccessful(
                                                         WORD *outErrorResult,
                                                         String *outReason
                                                         ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outErrorResult) *outErrorResult = mErrorCode;
        if (outReason) *outReason = mErrorReason;
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      IPublicationPtr PublicationRepository::Remover::getPublication() const
      {
        AutoRecursiveLock lock(getLock());
        return mPublication;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::Remover => IMessageMonitorDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::Remover::handleMessageMonitorMessageReceived(
                                                                               IMessageMonitorPtr monitor,
                                                                               message::MessagePtr message
                                                                               )
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received result but already cancelled"))
          return false;
        }
        if (mMonitor != monitor) {
          ZS_LOG_WARNING(Detail, log("received result but monitor does not match"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expected result but received something else"))
          return false;
        }

        if ((MessageFactoryPeerCommon::Methods)message->method() != MessageFactoryPeerCommon::Method_PeerDelete) {
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

        PeerDeleteResultPtr deleteResult = PeerDeleteResult::convert(result);
        mSucceeded = true;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (outer) {
          outer->notifyRemoved(mThisWeak.lock());
        }

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::Remover::onMessageMonitorTimedOut(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mMonitor) return;
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
      String PublicationRepository::Remover::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("remove id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               IPublication::toDebugString(mPublication) +
               IMessageMonitor::toDebugString(mMonitor) +
               Helper::getDebugValue("succeeded", mSucceeded ? String("true") : String(), firstTime) +
               Helper::getDebugValue("error code", 0 != mErrorCode ? Stringize<typeof(mErrorCode)>(mErrorCode).string() : String(), firstTime);
               Helper::getDebugValue("error reason", mErrorReason, firstTime);
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
        mDelegate(IPublicationSubscriptionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mCurrentState(IPublicationSubscription::PublicationSubscriptionState_Pending)
      {
        AccountPtr account = outer->getAccount();
        LocationPtr localLocation;
        if (account) {
          localLocation = Location::convert(ILocation::getForLocal(account));
        }

        mSubscriptionInfo = IPublicationMetaDataForPublicationRepository::create(0, 0, 0, localLocation, publicationPath, "", IPublicationMetaData::Encoding_JSON, relationships, localLocation);
        ZS_LOG_DEBUG(log("created") + mSubscriptionInfo->forRepo().getDebugValuesString())
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
      void PublicationRepository::SubscriptionLocal::notifyUpdated(PublicationPtr publication)
      {
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("receive notification of updated document but location subscription is cancelled"))
          return;
        }

        ZS_LOG_TRACE(log("publication is updated") + publication->forRepo().getDebugValuesString())

        String name = publication->forRepo().getName();
        String path = mSubscriptionInfo->forRepo().getName();

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

        AccountPtr account = outer->getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication update notification as account is gone"))
          return;
        }

        LocationPtr localLocation = ILocationForPublicationRepository::getForLocal(account);

        if (!outer->canSubscribeToPublisher(
                                            publication->forRepo().getCreatorLocation(),
                                            publication->forRepo().getRelationships(),
                                            localLocation,
                                            mSubscriptionInfo->forRepo().getRelationships())) {
          ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
          return;
        }

        ZS_LOG_DEBUG(log("notifying about publication update to local subscriber") + publication->forRepo().getDebugValuesString())

        // valid to notify about this document...
        try {
          mDelegate->onPublicationSubscriptionPublicationUpdated(mThisWeak.lock(), publication->forRepo().toPublicationMetaData());
        } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::SubscriptionLocal::notifyGone(PublicationPtr publication)
      {
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received notification that publication is gone but local subscription is already cancelled"))
          return;
        }

        ZS_LOG_TRACE(log("notified publication is gone") + publication->forRepo().getDebugValuesString())

        String name = publication->forRepo().getName();
        String path = mSubscriptionInfo->forRepo().getName();

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

        AccountPtr account = outer->getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication gone notification as account is gone"))
          return;
        }

        LocationPtr localLocation = ILocationForPublicationRepository::getForLocal(account);

        if (!outer->canSubscribeToPublisher(
                                            publication->forRepo().getCreatorLocation(),
                                            publication->forRepo().getRelationships(),
                                            localLocation,
                                            mSubscriptionInfo->forRepo().getRelationships())) {
          ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
          return;
        }

        ZS_LOG_DEBUG(log("notifying about publication gone to local subscriber") + publication->forRepo().getDebugValuesString())

        // valid to notify about this document...
        try {
          mDelegate->onPublicationSubscriptionPublicationGone(mThisWeak.lock(), publication->forRepo().toPublicationMetaData());
        } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      SubscriptionLocalPtr PublicationRepository::SubscriptionLocal::convert(IPublicationSubscriptionPtr subscription)
      {
        return boost::dynamic_pointer_cast<SubscriptionLocal>(subscription);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::SubscriptionLocal => IPublicationSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::SubscriptionLocal::toDebugString(SubscriptionLocalPtr subscription, bool includeCommaPrefix)
      {
        if (!subscription) return includeCommaPrefix ? ", subscription local=(null" : "subscription local=(null)";
        return subscription->getDebugValueString();
      }

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
        return mSubscriptionInfo->forRepo().toPublicationMetaData();
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
      String PublicationRepository::SubscriptionLocal::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("subscription local id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               IPublicationMetaData::toDebugString(mSubscriptionInfo) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime);
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
                                                                                PublicationMetaDataPtr subscriptionInfo
                                                                                ) :
      MessageQueueAssociator(queue),
      mID(zsLib::createPUID()),
      mOuter(outer),
      mPeerSource(peerSource),
      mSubscriptionInfo(subscriptionInfo)
      {
        ZS_LOG_DEBUG(log("created") + subscriptionInfo->forRepo().getDebugValuesString())
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
      String PublicationRepository::PeerSubscriptionIncoming::toDebugString(PeerSubscriptionIncomingPtr subscription, bool includeCommaPrefix)
      {
        if (!subscription) return includeCommaPrefix ? ", subscription incoming=(null" : "subscription incoming=(null)";
        return subscription->getDebugValueString();
      }

      //-----------------------------------------------------------------------
      PublicationRepository::PeerSubscriptionIncomingPtr PublicationRepository::PeerSubscriptionIncoming::create(
                                                                                                                 IMessageQueuePtr queue,
                                                                                                                 PublicationRepositoryPtr outer,
                                                                                                                 PeerSourcePtr peerSource,
                                                                                                                 PublicationMetaDataPtr subscriptionInfo
                                                                                                                 )
      {
        PeerSubscriptionIncomingPtr pThis(new PeerSubscriptionIncoming(queue, outer, peerSource, subscriptionInfo));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyUpdated(PublicationPtr publication)
      {
        CachedPublicationMap tempCache;
        tempCache[publication] = publication;

        notifyUpdated(tempCache);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyGone(PublicationPtr publication)
      {
        CachedPublicationMap tempCache;
        tempCache[publication] = publication;

        notifyGone(tempCache);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyUpdated(const CachedPublicationMap &cachedPublications)
      {
        typedef PeerPublishNotifyRequest::PublicationList PublicationList;

        AutoRecursiveLock lock(getLock());

        PublicationList list;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication update notification as publication respository is gone"))
          return;
        }

        AccountPtr account = outer->getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication update notification as account is gone"))
          return;
        }

        for (CachedPublicationMap::const_iterator iter = cachedPublications.begin(); iter != cachedPublications.end(); ++iter)
        {
          const PublicationPtr &publication = (*iter).second;
          ZS_LOG_TRACE(log("notified of updated publication") + publication->forRepo().getDebugValuesString())

          String name = publication->forRepo().getName();
          String path = mSubscriptionInfo->forRepo().getName();

          if (name.length() < path.length()) {
            ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }

          if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
            ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }

          LocationPtr subscriberLocation = mSubscriptionInfo->forRepo().getCreatorLocation(); // the subscriber is the person who created this subscription

          if (!outer->canSubscribeToPublisher(
                                              publication->forRepo().getCreatorLocation(),
                                              publication->forRepo().getRelationships(),
                                              subscriberLocation,
                                              mSubscriptionInfo->forRepo().getRelationships())) {
            ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
            continue;
          }

          ZS_LOG_DEBUG(log("notifying about publication updated to subscriber") + publication->forRepo().getDebugValuesString())

          list.push_back(publication->forRepo().toPublicationMetaData());
        }

        if (list.size() < 1) {
          ZS_LOG_TRACE(log("no publications updates are needed to be sent to this subscriber") + mSubscriptionInfo->forRepo().getDebugValuesString())
          return;
        }

        ZS_LOG_TRACE(log("publications will be notified to this subscriber") + ", total  publications=" + Stringize<size_t>(list.size()).string() + mSubscriptionInfo->forRepo().getDebugValuesString())

        PeerPublishNotifyRequestPtr request = PeerPublishNotifyRequest::create();
        request->domain(account->forRepo().getDomain());
        request->publicationList(list);
        request->peerCache(PeerCache::find(mPeerSource, outer));

        IMessageMonitorPtr monitor = IMessageMonitor::monitorAndSendToLocation(mThisWeak.lock(), mSubscriptionInfo->forRepo().getCreatorLocation(), request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));

        mNotificationMonitors.push_back(monitor);
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::notifyGone(const CachedPublicationMap &cachedPublications)
      {
        typedef PeerPublishNotifyRequest::PublicationList PublicationList;

        AutoRecursiveLock lock(getLock());

        PublicationList list;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication gone notification as publication respository is gone"))
          return;
        }

        AccountPtr account = outer->getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot hanlde publication gone notification as account is gone"))
          return;
        }

        for (CachedPublicationMap::const_iterator iter = cachedPublications.begin(); iter != cachedPublications.end(); ++iter)
        {
          const PublicationPtr &publication = (*iter).second;

          String name = publication->forRepo().getName();
          String path = mSubscriptionInfo->forRepo().getName();

          if (name.length() < path.length()) {
            ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }
          if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
            ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
            continue;
          }

          LocationPtr subscriptionPublishedLocation = mSubscriptionInfo->forRepo().getPublishedLocation();

          if (!outer->canSubscribeToPublisher(
                                              publication->forRepo().getCreatorLocation(),
                                              publication->forRepo().getRelationships(),
                                              subscriptionPublishedLocation,
                                              mSubscriptionInfo->forRepo().getRelationships())) {
            ZS_LOG_TRACE(log("publication/subscriber do not publish/subscribe to each other (thus ignoring notification)"))
            continue;
          }

          ZS_LOG_DEBUG(log("notifying about publication gone to subscriber") + publication->forRepo().getDebugValuesString())

          PublicationMetaDataPtr metaData = IPublicationMetaDataForPublicationRepository::createFrom(publication->forRepo().toPublicationMetaData());
          metaData->forRepo().setVersion(0);
          metaData->forRepo().setBaseVersion(0);

          list.push_back(metaData->forRepo().toPublicationMetaData());
        }

        if (list.size() < 1) {
          ZS_LOG_TRACE(log("no 'publications are gone' notification will be send to this subscriber") + mSubscriptionInfo->forRepo().getDebugValuesString())
          return;
        }

        ZS_LOG_TRACE(log("'publications are gone' notification will be notified to this subscriber") + ", total  publications=" + Stringize<size_t>(list.size()).string() + mSubscriptionInfo->forRepo().getDebugValuesString())

        PeerPublishNotifyRequestPtr request = PeerPublishNotifyRequest::create();
        request->domain(account->forRepo().getDomain());
        request->publicationList(list);

        IMessageMonitorPtr monitor = IMessageMonitor::monitorAndSendToLocation(mThisWeak.lock(), mSubscriptionInfo->forRepo().getCreatorLocation(), request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));

        mNotificationMonitors.push_back(monitor);
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataPtr PublicationRepository::PeerSubscriptionIncoming::getSource() const
      {
        return mSubscriptionInfo->forRepo().toPublicationMetaData();
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        for (NotificationMonitorList::iterator iter = mNotificationMonitors.begin(); iter != mNotificationMonitors.end(); ++iter)
        {
          IMessageMonitorPtr &notifyMonitor = (*iter);
          notifyMonitor->cancel();
        }
        mNotificationMonitors.clear();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionIncoming => IMessageMonitorDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::PeerSubscriptionIncoming::handleMessageMonitorMessageReceived(
                                                                                                IMessageMonitorPtr monitor,
                                                                                                message::MessagePtr message
                                                                                                )
      {
        ZS_LOG_DEBUG(log("received notification that notification was sent"))

        for (NotificationMonitorList::iterator iter = mNotificationMonitors.begin(); iter != mNotificationMonitors.end(); ++iter)
        {
          IMessageMonitorPtr &notifyMonitor = (*iter);
          if (notifyMonitor == monitor) {
            ZS_LOG_TRACE(log("found monitor for subscription notification request"))
            // doesn't matter if it was successful or not because there is nothing we can do either way...
            mNotificationMonitors.erase(iter);
            return true;
          }
        }
        ZS_LOG_WARNING(Detail, log("unable to find monitor for subscription notification request"))
        return false;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionIncoming::onMessageMonitorTimedOut(IMessageMonitorPtr monitor)
      {
        ZS_LOG_DEBUG(log("subscription notification time out"))
        for (NotificationMonitorList::iterator iter = mNotificationMonitors.begin(); iter != mNotificationMonitors.end(); ++iter)
        {
          IMessageMonitorPtr &notifyMonitor = (*iter);
          if (notifyMonitor == monitor) {
            ZS_LOG_DEBUG(log("subscription notification found thus removing"))

            // doesn't matter if it was successful or not because there is nothing we can do either way...
            mNotificationMonitors.erase(iter);
            return;
          }
        }
        ZS_LOG_DEBUG(log("unable to find monitor matching subscription notification request after time out"))
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
      String PublicationRepository::PeerSubscriptionIncoming::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("peer subscription incoming id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               ", source: " + IPublicationMetaData::toDebugString(mPeerSource, false) +
               ", subscription info: " + IPublicationMetaData::toDebugString(mSubscriptionInfo, false) +
               Helper::getDebugValue("notification monitors", mNotificationMonitors.size() > 0 ? Stringize<typeof(mID)>(mNotificationMonitors.size()).string() : String(), firstTime);
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
                                                                                PublicationMetaDataPtr subscriptionInfo
                                                                                ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IPublicationSubscriptionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mCurrentState(PublicationSubscriptionState_Pending),
        mSubscriptionInfo(subscriptionInfo)
      {
        ZS_LOG_DEBUG(log("created") + mSubscriptionInfo->forRepo().getDebugValuesString())
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
                                                                                                                 PublicationMetaDataPtr subscriptionInfo
                                                                                                                 )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!subscriptionInfo)

        PeerSubscriptionOutgoingPtr pThis(new PeerSubscriptionOutgoing(queue, outer, delegate, subscriptionInfo));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::setMonitor(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        mMonitor = monitor;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::notifyUpdated(PublicationMetaDataPtr metaData)
      {
        AutoRecursiveLock lock(getLock());

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("notification up an updated document after cancel called") + metaData->forRepo().getDebugValuesString())
          return;
        }

        String name = metaData->forRepo().getName();
        String path = mSubscriptionInfo->forRepo().getName();

        if (name.length() < path.length()) {
          ZS_LOG_TRACE(log("name is too short for subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        if (0 != strncmp(name.c_str(), path.c_str(), path.length())) {
          ZS_LOG_TRACE(log("name does not match subscription path") + ", name=" + name + ", path=" + path)
          return;
        }

        const char *ignoreReason = NULL;
        if (0 != ILocationForPublicationRepository::locationCompare(mSubscriptionInfo->forRepo().getPublishedLocation(), metaData->forRepo().getPublishedLocation(), ignoreReason)) {
          ZS_LOG_TRACE(log("publication update/gone notification for a source other than where subscription was placed (thus ignoring)") + metaData->forRepo().getDebugValuesString())
          return;
        }

        ZS_LOG_DEBUG(log("publication update/gone notification is being notified to outgoing subscription delegate") + metaData->forRepo().getDebugValuesString())

        // this appears to be a match thus notify the subscriber...
        try {
          if (0 == metaData->forRepo().getVersion()) {
            mDelegate->onPublicationSubscriptionPublicationGone(mThisWeak.lock(), metaData->forRepo().toPublicationMetaData());
          } else {
            mDelegate->onPublicationSubscriptionPublicationUpdated(mThisWeak.lock(), metaData->forRepo().toPublicationMetaData());
          }
        } catch(IPublicationSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      PeerSubscriptionOutgoingPtr PublicationRepository::PeerSubscriptionOutgoing::convert(IPublicationSubscriptionPtr subscription)
      {
        return boost::dynamic_pointer_cast<PeerSubscriptionOutgoing>(subscription);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionOutgoing => IPublicationSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationRepository::PeerSubscriptionOutgoing::toDebugString(PeerSubscriptionOutgoingPtr subscription, bool includeCommaPrefix)
      {
        if (!subscription) return includeCommaPrefix ? ", subscription outgoing=(null" : "subscription outgoing=(null)";
        return subscription->getDebugValueString();
      }

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
        return mSubscriptionInfo->forRepo().toPublicationMetaData();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationRepository::PeerSubscriptionOutgoing => IMessageMonitorDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationRepository::PeerSubscriptionOutgoing::handleMessageMonitorMessageReceived(
                                                                                                IMessageMonitorPtr monitor,
                                                                                                message::MessagePtr message
                                                                                                )
      {
        AutoRecursiveLock lock(getLock());

        if (monitor == mCancelMonitor) {
          ZS_LOG_DEBUG(log("cancel monitor completed"))
          mCancelMonitor->cancel();
          cancel();
          return true;
        }

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("received subscription notification reply but cancel already called"))
          return false;
        }
        if (mMonitor != monitor) {
          ZS_LOG_WARNING(Detail, log("received subscription notification reply but monitor does not match reply"))
          return false;
        }

        if (message->messageType() != message::Message::MessageType_Result) {
          ZS_LOG_WARNING(Detail, log("expecting to receive result but received something else"))
          return false;
        }

        if ((MessageFactoryPeerCommon::Methods)message->method() != MessageFactoryPeerCommon::Method_PeerSubscribe) {
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

        PeerSubscribeResultPtr subscribeResult = PeerSubscribeResult::convert(result);
        mSucceeded = true;

        PublicationRepositoryPtr outer = mOuter.lock();
        if (outer) {
          outer->notifySubscribed(mThisWeak.lock());
        }

        setState(IPublicationSubscription::PublicationSubscriptionState_Established);
        if (mMonitor) {
          mMonitor->cancel();
          mMonitor.reset();
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void PublicationRepository::PeerSubscriptionOutgoing::onMessageMonitorTimedOut(IMessageMonitorPtr monitor)
      {
        AutoRecursiveLock lock(getLock());
        if (monitor == mCancelMonitor)
        {
          ZS_LOG_DEBUG(log("cancel monitor timeout received"))
          cancel();
          return;
        }
        if (monitor != mMonitor) {
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
      String PublicationRepository::PeerSubscriptionOutgoing::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("peer subscription outgoing id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               IPublicationMetaData::toDebugString(mSubscriptionInfo) +
               IMessageMonitor::toDebugString(mMonitor) +
               IMessageMonitor::toDebugString(mCancelMonitor) +
               Helper::getDebugValue("succeeded", mSucceeded ? String("true") : String(), firstTime) +
               Helper::getDebugValue("error code", 0 != mErrorCode ? Stringize<typeof(mErrorCode)>(mErrorCode).string() : String(), firstTime);
               Helper::getDebugValue("error reason", mErrorReason, firstTime);
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

        if ((!mCancelMonitor) &&
            (pThis)) {

          PublicationRepositoryPtr outer = mOuter.lock();

          if (outer) {
            AccountPtr account = outer->getAccount();

            if (account) {
              ZS_LOG_DEBUG(log("sending request to cancel outgoing subscription"))

              PeerSubscribeRequestPtr request = PeerSubscribeRequest::create();
              request->domain(account->forRepo().getDomain());

              IPublicationMetaData::SubscribeToRelationshipsMap empty;
              request->publicationMetaData(mSubscriptionInfo->forRepo().toPublicationMetaData());

              mCancelMonitor = IMessageMonitor::monitorAndSendToLocation(pThis, mSubscriptionInfo->forRepo().getPublishedLocation(), request, Seconds(HOOKFLASH_STACK_PUBLICATIONREPOSITORY_REQUEST_TIMEOUT_IN_SECONDS));
              return;
            }
          }
        }

        if (pThis) {
          if (mCancelMonitor)  {
            if (!mCancelMonitor->isComplete()) {
              ZS_LOG_DEBUG(log("waiting for cancel monitor to complete"))
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

    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    String IPublicationRepository::toDebugString(IPublicationRepositoryPtr repository, bool includeCommaPrefix)
    {
      return internal::PublicationRepository::toDebugString(repository, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IPublicationRepositoryPtr IPublicationRepository::getFromAccount(IAccountPtr account)
    {
      return internal::PublicationRepository::getFromAccount(account);
    }

    //-------------------------------------------------------------------------
    String IPublicationPublisher::toDebugString(IPublicationPublisherPtr publisher, bool includeCommaPrefix)
    {
      return internal::PublicationRepository::Publisher::toDebugString(publisher, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    String IPublicationFetcher::toDebugString(IPublicationFetcherPtr fetcher, bool includeCommaPrefix)
    {
      return internal::PublicationRepository::Fetcher::toDebugString(fetcher, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    String IPublicationRemover::toDebugString(IPublicationRemoverPtr remover, bool includeCommaPrefix)
    {
      return internal::PublicationRepository::Remover::toDebugString(remover, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    String IPublicationSubscription::toDebugString(IPublicationSubscriptionPtr subscription, bool includeCommaPrefix)
    {
      internal::PublicationRepository::SubscriptionLocalPtr localSubscription = internal::PublicationRepository::SubscriptionLocal::convert(subscription);
      if (localSubscription) {
        return internal::PublicationRepository::SubscriptionLocal::toDebugString(localSubscription);
      }
      return internal::PublicationRepository::PeerSubscriptionOutgoing::toDebugString(internal::PublicationRepository::PeerSubscriptionOutgoing::convert(subscription));
    }

    //-------------------------------------------------------------------------
    String IPublicationRepositoryPeerCache::toDebugString(IPublicationRepositoryPeerCachePtr cache, bool includeCommaPrefix)
    {
      return internal::PublicationRepository::PeerCache::toDebugString(cache, includeCommaPrefix);
    }
  }
}
