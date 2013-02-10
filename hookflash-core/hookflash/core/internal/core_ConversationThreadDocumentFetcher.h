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

#include <hookflash/core/internal/types.h>

#include <hookflash/stack/types.h>
#include <hookflash/stack/IPublicationRepository.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadDocumentFetcher
      #pragma mark

      interaction IConversationThreadDocumentFetcher
      {
        static String toDebugString(IConversationThreadDocumentFetcherPtr fetcher, bool includeCommaPrefix = true);

        static IConversationThreadDocumentFetcherPtr create(
                                                            IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                            IPublicationRepositoryPtr repository
                                                            );

        virtual void cancel() = 0;

        virtual void notifyPublicationUpdated(
                                              ILocationPtr peerLocation,
                                              IPublicationMetaDataPtr metaData
                                              ) = 0;

        virtual void notifyPublicationGone(
                                           ILocationPtr peerLocation,
                                           IPublicationMetaDataPtr metaData
                                           ) = 0;

        virtual void notifyPeerDisconnected(ILocationPtr peerLocation) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadDocumentFetcherDelegate
      #pragma mark

      interaction IConversationThreadDocumentFetcherDelegate
      {
        virtual void onConversationThreadDocumentFetcherPublicationUpdated(
                                                                           IConversationThreadDocumentFetcherPtr fetcher,
                                                                           ILocationPtr peerLocation,
                                                                           IPublicationPtr publication
                                                                           ) = 0;

        virtual void onConversationThreadDocumentFetcherPublicationGone(
                                                                        IConversationThreadDocumentFetcherPtr fetcher,
                                                                        ILocationPtr peerLocation,
                                                                        IPublicationMetaDataPtr metaData
                                                                        ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher
      #pragma mark

      class ConversationThreadDocumentFetcher  : public MessageQueueAssociator,
                                                 public IConversationThreadDocumentFetcher,
                                                 public stack::IPublicationFetcherDelegate
      {
      public:
        friend interaction IConversationThreadDocumentFetcherFactory;
        friend interaction IConversationThreadDocumentFetcher;

        typedef std::pair<ILocationPtr, IPublicationMetaDataPtr> PeerLocationPublicationPair;
        typedef std::list<PeerLocationPublicationPair> PendingPublicationList;

      protected:
        ConversationThreadDocumentFetcher(
                                          IMessageQueuePtr queue,
                                          IConversationThreadDocumentFetcherDelegatePtr delegate,
                                          IPublicationRepositoryPtr repository
                                          );

        void init();

      public:
        ~ConversationThreadDocumentFetcher();

        static ConversationThreadDocumentFetcherPtr convert(IConversationThreadDocumentFetcherPtr fetcher);

        static String toDebugString(ConversationThreadDocumentFetcherPtr fetcher, bool includeCommaPrefix = true);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadDocumentFetcher => IConversationThreadDocumentFetcher
        #pragma mark

        static ConversationThreadDocumentFetcherPtr create(
                                                           IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                           IPublicationRepositoryPtr repository
                                                           );

        // (duplicate) virtual void cancel();

        virtual void notifyPublicationUpdated(
                                              ILocationPtr peerLocation,
                                              IPublicationMetaDataPtr metaData
                                              );

        virtual void notifyPublicationGone(
                                           ILocationPtr peerLocation,
                                           IPublicationMetaDataPtr metaData
                                           );

        virtual void notifyPeerDisconnected(ILocationPtr peerLocation);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadDocumentFetcher => IPublicationFetcherDelegate
        #pragma mark

        virtual void onPublicationFetcherCompleted(IPublicationFetcherPtr fetcher);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadDocumentFetcher => (internal)
        #pragma mark

        virtual bool isShutdown() const;

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        RecursiveLock &getLock() const {return mLock;}

        void cancel();
        void step();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadDocumentFetcher => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        PUID mID;
        ConversationThreadDocumentFetcherWeakPtr mThisWeak;

        IConversationThreadDocumentFetcherDelegatePtr mDelegate;

        IPublicationRepositoryPtr mRepository;

        ILocationPtr mFetcherPeerLocation;
        IPublicationFetcherPtr mFetcher;

        PendingPublicationList mPendingPublications;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadDocumentFetcherFactory
      #pragma mark

      interaction IConversationThreadDocumentFetcherFactory
      {
        static IConversationThreadDocumentFetcherFactory &singleton();

        virtual ConversationThreadDocumentFetcherPtr create(
                                                            IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                            IPublicationRepositoryPtr repository
                                                            );
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::internal::IConversationThreadDocumentFetcherDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::internal::IConversationThreadDocumentFetcherPtr, IConversationThreadDocumentFetcherPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationMetaDataPtr, IPublicationMetaDataPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationPtr, IPublicationPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::ILocationPtr, ILocationPtr)
ZS_DECLARE_PROXY_METHOD_3(onConversationThreadDocumentFetcherPublicationUpdated, IConversationThreadDocumentFetcherPtr, ILocationPtr, IPublicationPtr)
ZS_DECLARE_PROXY_METHOD_3(onConversationThreadDocumentFetcherPublicationGone, IConversationThreadDocumentFetcherPtr, ILocationPtr, IPublicationMetaDataPtr)
ZS_DECLARE_PROXY_END()
