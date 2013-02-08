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


#include <hookflash/internal/hookflash_ConversationThreadDocumentFetcher.h>

#include <hookflash/stack/IPeerLocation.h>

#include <zsLib/Log.h>
#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>

namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }

using namespace zsLib::XML;
using zsLib::Stringize;

namespace hookflash
{
  namespace internal
  {
    typedef zsLib::String String;
    typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
    typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
    typedef stack::IPublicationMetaData IPublicationMetaData;

    //-------------------------------------------------------------------------
    static bool isSamePublication(IPublicationMetaDataPtr obj1, IPublicationMetaDataPtr obj2)
    {
      if (obj1->getSource() != obj2->getSource()) return false;
      if (obj1->getCreatorContactID() != obj2->getCreatorContactID()) return false;
      if (obj1->getCreatorLocationID() != obj2->getCreatorLocationID()) return false;
      if ((IPublicationMetaData::Source_Finder == obj1->Source_Finder) ||
          (IPublicationMetaData::Source_Finder == obj1->Source_Local)) return true;
      if (obj1->getPublishedToContactID() != obj2->getPublishedToContactID()) return false;
      if (obj1->getPublishedToLocationID() != obj2->getPublishedToLocationID()) return false;

      return true;
    }

    //-------------------------------------------------------------------------
    static bool isFromPeer(
                           IPublicationMetaDataPtr metaData,
                           const char *contactID,
                           const char *locationID
                           )
    {
      if (IPublicationMetaData::Source_Peer != metaData->getSource()) return false;

      if (metaData->getPublishedToContactID() != contactID) return false;
      if (metaData->getPublishedToLocationID() == locationID) return false;
      return true;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadDocumentFetcher
    #pragma mark

    //-------------------------------------------------------------------------
    IConversationThreadDocumentFetcherPtr IConversationThreadDocumentFetcher::create(
                                                                                     IMessageQueuePtr queue,
                                                                                     IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                                                     IPublicationRepositoryPtr repository
                                                                                     )
    {
      return ConversationThreadDocumentFetcher::create(queue, delegate, repository);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThreadDocumentFetcher
    #pragma mark

    //-------------------------------------------------------------------------
    ConversationThreadDocumentFetcher::ConversationThreadDocumentFetcher(
                                                                         IMessageQueuePtr queue,
                                                                         IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                                         IPublicationRepositoryPtr repository
                                                                         ) :
      MessageQueueAssociator(queue),
      mID(zsLib::createPUID()),
      mRepository(repository),
      mDelegate(IConversationThreadDocumentFetcherDelegateProxy::createWeak(queue, delegate))
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!repository)
      ZS_LOG_BASIC(log("created"))
    }

    //-------------------------------------------------------------------------
    void ConversationThreadDocumentFetcher::init()
    {
    }

    //-------------------------------------------------------------------------
    ConversationThreadDocumentFetcher::~ConversationThreadDocumentFetcher()
    {
      mThisWeak.reset();
      ZS_LOG_BASIC(log("destroyed"))
      cancel();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThreadDocumentFetcher => IConversationThreadDocumentFetcher
    #pragma mark

    //-------------------------------------------------------------------------
    ConversationThreadDocumentFetcherPtr ConversationThreadDocumentFetcher::create(
                                                                                   IMessageQueuePtr queue,
                                                                                   IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                                                   IPublicationRepositoryPtr repository
                                                                                   )
    {
      ConversationThreadDocumentFetcherPtr pThis(new ConversationThreadDocumentFetcher(queue, delegate, repository));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }

    //-------------------------------------------------------------------------
    void ConversationThreadDocumentFetcher::notifyPublicationUpdated(
                                                                     IPeerLocationPtr peerLocation,
                                                                     IPublicationMetaDataPtr metaData
                                                                     )
    {
      AutoRecursiveLock lock(getLock());
      if (isShutdown()) return;

      // find all with the same publication name/peer and remove them and replace with latest
      for (PendingPublicationList::iterator pendingIter = mPendingPublications.begin(); pendingIter != mPendingPublications.end(); )
      {
        PendingPublicationList::iterator current = pendingIter;
        ++pendingIter;

        IPublicationMetaDataPtr &pending = (*current).second;
        if (isSamePublication(pending, metaData)) {
          ZS_LOG_DEBUG(log("publication removed because publication is updated again") + ", name=" + pending->getName() + ", peer contact ID=" + pending->getPublishedToContactID() + ", peer location ID=" + pending->getPublishedToLocationID())
          mPendingPublications.erase(current); // no need to fetch this publication since a new one is replacing it now
        }
      }

      mPendingPublications.push_back(PeerLocationPublicationPair(peerLocation, metaData));
      step();
    }

    //-------------------------------------------------------------------------
    void ConversationThreadDocumentFetcher::notifyPublicationGone(
                                                                  IPeerLocationPtr peerLocation,
                                                                  IPublicationMetaDataPtr metaData
                                                                  )
    {
      AutoRecursiveLock lock(getLock());
      if (isShutdown()) return;

      if (mFetcher) {
        if (isSamePublication(mFetcher->getPublicationMetaData(), metaData)) {
          ZS_LOG_DEBUG(log("publication removed because publication is gone") + ", name=" + mFetcher->getPublicationMetaData()->getName() + ", peer contact ID=" + mFetcher->getPublicationMetaData()->getPublishedToContactID() + ", peer location ID=" + mFetcher->getPublicationMetaData()->getPublishedToLocationID())

          mFetcherPeerLocation.reset();

          mFetcher->cancel();
          mFetcher.reset();
        }
      }

      // find all with the same publication name/peer and remove them
      for (PendingPublicationList::iterator pendingIter = mPendingPublications.begin(); pendingIter != mPendingPublications.end(); )
      {
        PendingPublicationList::iterator current = pendingIter;
        ++pendingIter;

        IPublicationMetaDataPtr &pending = (*current).second;
        if (isSamePublication(pending, metaData)) {
          ZS_LOG_DEBUG(log("publication removed because publication is gone") + ", name=" + pending->getName() + ", peer contact ID=" + pending->getPublishedToContactID() + ", peer location ID=" + pending->getPublishedToLocationID())
          mPendingPublications.erase(current); // no need to fetch this publication since a new one is replacing it now
        }
      }

      // notify the delegate that it's all done
      try {
        mDelegate->onConversationThreadDocumentFetcherPublicationGone(mThisWeak.lock(), peerLocation, metaData);
      } catch(IConversationThreadDocumentFetcherDelegateProxy::Exceptions::DelegateGone &) {
      }

      step();
    }

    //-------------------------------------------------------------------------
    void ConversationThreadDocumentFetcher::notifyPeerDisconnected(IPeerLocationPtr peerLocation)
    {
      AutoRecursiveLock lock(getLock());

      bool removeFetcher = false;
      bool alreadyNotifiedAboutDocument = false;

      if (mFetcher) {
        if (isFromPeer(mFetcher->getPublicationMetaData(), peerLocation->getContactID(), peerLocation->getLocationID())) {
          ZS_LOG_DEBUG(log("publication removed because peer is gone") + ", name=" + mFetcher->getPublicationMetaData()->getName() + ", peer contact ID=" + mFetcher->getPublicationMetaData()->getPublishedToContactID() + ", peer location ID=" + mFetcher->getPublicationMetaData()->getPublishedToLocationID())
          removeFetcher = true;
        }
      }

      for (PendingPublicationList::iterator pendingIter = mPendingPublications.begin(); pendingIter != mPendingPublications.end(); )
      {
        PendingPublicationList::iterator current = pendingIter;
        ++pendingIter;

        IPeerLocationPtr &pendingPeerLocation = (*current).first;
        IPublicationMetaDataPtr &pending = (*current).second;
        if (isFromPeer(pending, peerLocation->getContactID(), peerLocation->getLocationID())) {
          ZS_LOG_DEBUG(log("publication removed because peer is gone") + ", name=" + pending->getName() + ", peer contact ID=" + pending->getPublishedToContactID() + ", peer location ID=" + pending->getPublishedToLocationID())
          if ((mFetcher) &&
              (!alreadyNotifiedAboutDocument)) {
            if (isSamePublication(mFetcher->getPublicationMetaData(), pending)) {
              alreadyNotifiedAboutDocument = true;
            }
          }
          try {
            mDelegate->onConversationThreadDocumentFetcherPublicationGone(mThisWeak.lock(), pendingPeerLocation, pending);
          } catch(IConversationThreadDocumentFetcherDelegateProxy::Exceptions::DelegateGone &) {
          }
          mPendingPublications.erase(current);
        }
      }

      if (removeFetcher) {
        if (!alreadyNotifiedAboutDocument) {
          ZS_LOG_DEBUG(log("notifying publication removed because peer is gone") + ", name=" + mFetcher->getPublicationMetaData()->getName() + ", peer contact ID=" + mFetcher->getPublicationMetaData()->getPublishedToContactID() + ", peer location ID=" + mFetcher->getPublicationMetaData()->getPublishedToLocationID())
          try {
            mDelegate->onConversationThreadDocumentFetcherPublicationGone(mThisWeak.lock(), mFetcherPeerLocation, mFetcher->getPublicationMetaData());
          } catch(IConversationThreadDocumentFetcherDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        mFetcherPeerLocation.reset();

        mFetcher->cancel();
        mFetcher.reset();
      }

      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThreadDocumentFetcher => IPublicationFetcherDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void ConversationThreadDocumentFetcher::onPublicationFetcherComplete(IPublicationFetcherPtr fetcher)
    {
      AutoRecursiveLock lock(getLock());
      if (isShutdown()) return;

      if (fetcher != mFetcher) {
        ZS_LOG_WARNING(Detail, log("publication fetched on obsolete fetcher") + ", name=" + fetcher->getPublicationMetaData()->getName() + ", peer contact ID=" + fetcher->getPublicationMetaData()->getPublishedToContactID() + ", peer location ID=" + fetcher->getPublicationMetaData()->getPublishedToLocationID())
        return;
      }

      // fetcher is now complete
      if (fetcher->wasSuccessful()) {
        try {
          mDelegate->onConversationThreadDocumentFetcherPublicationUpdated(mThisWeak.lock(), mFetcherPeerLocation, mFetcher->getFetchedPublication());
        } catch(IConversationThreadDocumentFetcherDelegateProxy::Exceptions::DelegateGone &) {
        }
      } else {
        ZS_LOG_DEBUG(log("publication removed because peer is gone") + ", name=" + mFetcher->getPublicationMetaData()->getName() + ", peer contact ID=" + mFetcher->getPublicationMetaData()->getPublishedToContactID() + ", peer location ID=" + mFetcher->getPublicationMetaData()->getPublishedToLocationID())
      }

      mFetcherPeerLocation.reset();
      mFetcher.reset();

      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThreadDocumentFetcher => (internal)
    #pragma mark

    //-------------------------------------------------------------------------
    bool ConversationThreadDocumentFetcher::isShutdown() const
    {
      AutoRecursiveLock lock(getLock());
      return !mDelegate;
    }

    //-------------------------------------------------------------------------
    String ConversationThreadDocumentFetcher::log(const char *message) const
    {
      return String("ConversationThreadDocumentFetcher [") + Stringize<PUID>(mID).string() + "] " + message;
    }

    //-------------------------------------------------------------------------
    void ConversationThreadDocumentFetcher::cancel()
    {
      if (isShutdown()) return;

      ZS_LOG_DEBUG(log("cancel called"))

      ConversationThreadDocumentFetcherPtr pThis = mThisWeak.lock();

      if (pThis) {
        try {
          if (mFetcher) {
            mDelegate->onConversationThreadDocumentFetcherPublicationGone(pThis, mFetcherPeerLocation, mFetcher->getPublicationMetaData());
          }

          for (PendingPublicationList::iterator iter = mPendingPublications.begin(); iter != mPendingPublications.end(); ++iter)
          {
            IPeerLocationPtr &pendingPeerLocation = (*iter).first;
            IPublicationMetaDataPtr &pending = (*iter).second;
            mDelegate->onConversationThreadDocumentFetcherPublicationGone(pThis, pendingPeerLocation, pending);
          }
        } catch(IConversationThreadDocumentFetcherDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      mFetcherPeerLocation.reset();
      if (mFetcher) {
        mFetcher->cancel();
        mFetcher.reset();
      }

      mPendingPublications.clear();

      mDelegate.reset();
    }

    //-------------------------------------------------------------------------
    void ConversationThreadDocumentFetcher::step()
    {
      if (isShutdown()) {cancel();}

      if (mFetcher) {
        ZS_LOG_DEBUG(log("fetcher already active"))
        return;
      }

      if (mPendingPublications.size() < 1) {
        ZS_LOG_DEBUG(log("no publications to fetch"))
        return;
      }

      IPeerLocationPtr peerLocation = mPendingPublications.front().first;
      IPublicationMetaDataPtr metaData = mPendingPublications.front().second;
      mPendingPublications.pop_front();

      mFetcherPeerLocation = peerLocation;
      mFetcher = mRepository->fetch(mThisWeak.lock(), metaData);
      if (mFetcher) {
        ZS_LOG_DEBUG(log("fetching next publication") + ", name=" + mFetcher->getPublicationMetaData()->getName() + ", peer contact ID=" + mFetcher->getPublicationMetaData()->getPublishedToContactID() + ", peer location ID=" + mFetcher->getPublicationMetaData()->getPublishedToLocationID())
      } else{
        ZS_LOG_ERROR(Detail, log("fetching next publication failed to return fetcher") + ", name=" + mFetcher->getPublicationMetaData()->getName() + ", peer contact ID=" + mFetcher->getPublicationMetaData()->getPublishedToContactID() + ", peer location ID=" + mFetcher->getPublicationMetaData()->getPublishedToLocationID())
        notifyPublicationGone(peerLocation, metaData);
      }
    }
  }
}
