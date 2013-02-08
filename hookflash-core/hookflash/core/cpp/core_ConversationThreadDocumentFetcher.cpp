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


#include <hookflash/core/internal/core_ConversationThreadDocumentFetcher.h>
#include <hookflash/core/internal/core_Stack.h>
#include <hookflash/core/internal/core_Helper.h>

#include <hookflash/stack/ILocation.h>

#include <zsLib/Log.h>
#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>

namespace hookflash { namespace core { ZS_DECLARE_SUBSYSTEM(hookflash_core) } }

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      using zsLib::Stringize;
      
      using stack::IPublicationFetcher;

      //-----------------------------------------------------------------------
      static bool isSamePublication(IPublicationMetaDataPtr obj1, IPublicationMetaDataPtr obj2)
      {
        if (obj1->getCreatorLocation() != obj2->getCreatorLocation()) return false;
        if (obj1->getPublishedLocation() != obj2->getPublishedLocation()) return false;

        return true;
      }

      //-----------------------------------------------------------------------
      static bool isFromPeer(
                             IPublicationMetaDataPtr metaData,
                             ILocationPtr location
                             )
      {
        if (metaData->getPublishedLocation() != location) return false;
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadDocumentFetcher
      #pragma mark

      //-----------------------------------------------------------------------
      String IConversationThreadDocumentFetcher::toDebugString(IConversationThreadDocumentFetcherPtr fetcher, bool includeCommaPrefix)
      {
        if (!fetcher) return includeCommaPrefix ? ", conversation thread fetcher=(null" : "conversation thread fetcher=(null)";
        return ConversationThreadDocumentFetcher::convert(fetcher)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      IConversationThreadDocumentFetcherPtr IConversationThreadDocumentFetcher::create(
                                                                                       IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                                                       IPublicationRepositoryPtr repository
                                                                                       )
      {
        return ConversationThreadDocumentFetcher::create(delegate, repository);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher
      #pragma mark

      //-----------------------------------------------------------------------
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

      //-----------------------------------------------------------------------
      void ConversationThreadDocumentFetcher::init()
      {
      }

      //-----------------------------------------------------------------------
      ConversationThreadDocumentFetcher::~ConversationThreadDocumentFetcher()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      ConversationThreadDocumentFetcherPtr ConversationThreadDocumentFetcher::convert(IConversationThreadDocumentFetcherPtr fetcher)
      {
        return boost::dynamic_pointer_cast<ConversationThreadDocumentFetcher>(fetcher);
      }

      //-----------------------------------------------------------------------
      String ConversationThreadDocumentFetcher::toDebugString(ConversationThreadDocumentFetcherPtr fetcher, bool includeCommaPrefix)
      {
        if (!fetcher) return includeCommaPrefix ? ", conversation thread fetcher=(null" : "conversation thread fetcher=(null)";
        return fetcher->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher => IConversationThreadDocumentFetcher
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadDocumentFetcherPtr ConversationThreadDocumentFetcher::create(
                                                                                     IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                                                     IPublicationRepositoryPtr repository
                                                                                     )
      {
        ConversationThreadDocumentFetcherPtr pThis(new ConversationThreadDocumentFetcher(IStackForInternal::queueCore(), delegate, repository));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadDocumentFetcher::notifyPublicationUpdated(
                                                                       ILocationPtr peerLocation,
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
            ZS_LOG_DEBUG(log("publication removed because publication is updated again") + IPublicationMetaData::toDebugString(pending))
            mPendingPublications.erase(current); // no need to fetch this publication since a new one is replacing it now
          }
        }

        mPendingPublications.push_back(PeerLocationPublicationPair(peerLocation, metaData));
        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadDocumentFetcher::notifyPublicationGone(
                                                                    ILocationPtr peerLocation,
                                                                    IPublicationMetaDataPtr metaData
                                                                    )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;

        if (mFetcher) {
          if (isSamePublication(mFetcher->getPublicationMetaData(), metaData)) {
            ZS_LOG_DEBUG(log("publication removed because publication is gone") + IPublicationFetcher::toDebugString(mFetcher))

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
            ZS_LOG_DEBUG(log("publication removed because publication is gone") + IPublicationMetaData::toDebugString(pending))
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

      //-----------------------------------------------------------------------
      void ConversationThreadDocumentFetcher::notifyPeerDisconnected(ILocationPtr peerLocation)
      {
        AutoRecursiveLock lock(getLock());

        bool removeFetcher = false;
        bool alreadyNotifiedAboutDocument = false;

        if (mFetcher) {
          if (isFromPeer(mFetcher->getPublicationMetaData(), peerLocation)) {
            ZS_LOG_DEBUG(log("publication removed because peer is gone") + IPublicationFetcher::toDebugString(mFetcher))
            removeFetcher = true;
          }
        }

        for (PendingPublicationList::iterator pendingIter = mPendingPublications.begin(); pendingIter != mPendingPublications.end(); )
        {
          PendingPublicationList::iterator current = pendingIter;
          ++pendingIter;

          ILocationPtr &pendingPeerLocation = (*current).first;
          IPublicationMetaDataPtr &pending = (*current).second;
          if (isFromPeer(pending, peerLocation)) {
            ZS_LOG_DEBUG(log("publication removed because peer is gone") + IPublicationMetaData::toDebugString(pending))
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
            ZS_LOG_DEBUG(log("notifying publication removed because peer is gone") + IPublicationFetcher::toDebugString(mFetcher))
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

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher => IPublicationFetcherDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadDocumentFetcher::onPublicationFetcherCompleted(IPublicationFetcherPtr fetcher)
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;

        if (fetcher != mFetcher) {
          ZS_LOG_WARNING(Detail, log("publication fetched on obsolete fetcher") + IPublicationFetcher::toDebugString(fetcher))
          return;
        }

        // fetcher is now complete
        if (fetcher->wasSuccessful()) {
          try {
            mDelegate->onConversationThreadDocumentFetcherPublicationUpdated(mThisWeak.lock(), mFetcherPeerLocation, mFetcher->getFetchedPublication());
          } catch(IConversationThreadDocumentFetcherDelegateProxy::Exceptions::DelegateGone &) {
          }
        } else {
          ZS_LOG_DEBUG(log("publication removed because peer is gone") + IPublicationFetcher::toDebugString(mFetcher))
        }

        mFetcherPeerLocation.reset();
        mFetcher.reset();

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      bool ConversationThreadDocumentFetcher::isShutdown() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadDocumentFetcher::log(const char *message) const
      {
        return String("ConversationThreadDocumentFetcher [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadDocumentFetcher::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("conversation thread document fetcher id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               ILocation::toDebugString(mFetcherPeerLocation) +
               IPublicationFetcher::toDebugString(mFetcher) +
               Helper::getDebugValue("pending publications", mPendingPublications.size() > 0 ? Stringize<size_t>(mPendingPublications.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
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
              ILocationPtr &pendingPeerLocation = (*iter).first;
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

      //-----------------------------------------------------------------------
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

        ILocationPtr peerLocation = mPendingPublications.front().first;
        IPublicationMetaDataPtr metaData = mPendingPublications.front().second;
        mPendingPublications.pop_front();

        mFetcherPeerLocation = peerLocation;
        mFetcher = mRepository->fetch(mThisWeak.lock(), metaData);
        if (mFetcher) {
          ZS_LOG_DEBUG(log("fetching next publication") + IPublicationFetcher::toDebugString(mFetcher))
        } else{
          ZS_LOG_ERROR(Detail, log("fetching next publication failed to return fetcher") + getDebugValueString())
          notifyPublicationGone(peerLocation, metaData);
        }
      }
    }
  }
}
