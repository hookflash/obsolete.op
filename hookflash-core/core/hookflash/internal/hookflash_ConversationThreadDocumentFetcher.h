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

#include <hookflash/internal/hookflashTypes.h>

#include <hookflash/stack/hookflashTypes.h>
#include <hookflash/stack/IPublicationRepository.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>

namespace hookflash
{
  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadDocumentFetcher
    #pragma mark

    interaction IConversationThreadDocumentFetcher
    {
      typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
      typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IPeerLocationPtr IPeerLocationPtr;

      static IConversationThreadDocumentFetcherPtr create(
                                                          IMessageQueuePtr queue,
                                                          IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                          IPublicationRepositoryPtr repository
                                                          );

      virtual void cancel() = 0;

      virtual void notifyPublicationUpdated(
                                            IPeerLocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData
                                            ) = 0;

      virtual void notifyPublicationGone(
                                         IPeerLocationPtr peerLocation,
                                         IPublicationMetaDataPtr metaData
                                         ) = 0;

      virtual void notifyPeerDisconnected(IPeerLocationPtr peerLocation) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadDocumentFetcherDelegate
    #pragma mark

    interaction IConversationThreadDocumentFetcherDelegate
    {
      typedef stack::IPublicationPtr IPublicationPtr;
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IPeerLocationPtr IPeerLocationPtr;

      virtual void onConversationThreadDocumentFetcherPublicationUpdated(
                                                                         IConversationThreadDocumentFetcherPtr fetcher,
                                                                         IPeerLocationPtr peerLocation,
                                                                         IPublicationPtr publication
                                                                         ) = 0;

      virtual void onConversationThreadDocumentFetcherPublicationGone(
                                                                      IConversationThreadDocumentFetcherPtr fetcher,
                                                                      IPeerLocationPtr peerLocation,
                                                                      IPublicationMetaDataPtr metaData
                                                                      ) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThreadDocumentFetcher
    #pragma mark

    class ConversationThreadDocumentFetcher  : public zsLib::MessageQueueAssociator,
                                               public IConversationThreadDocumentFetcher,
                                               public stack::IPublicationFetcherDelegate
    {
    public:
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
      typedef zsLib::PUID PUID;
      typedef zsLib::String String;

      typedef stack::IPublicationFetcherPtr IPublicationFetcherPtr;

    protected:
      ConversationThreadDocumentFetcher(
                                        IMessageQueuePtr queue,
                                        IConversationThreadDocumentFetcherDelegatePtr delegate,
                                        IPublicationRepositoryPtr repository
                                        );

      void init();

    public:
      ~ConversationThreadDocumentFetcher();

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher => IConversationThreadDocumentFetcher
      #pragma mark

      static ConversationThreadDocumentFetcherPtr create(
                                                         IMessageQueuePtr queue,
                                                         IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                         IPublicationRepositoryPtr repository
                                                         );

      // (duplicate) virtual void cancel();

      virtual void notifyPublicationUpdated(
                                            IPeerLocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData
                                            );

      virtual void notifyPublicationGone(
                                         IPeerLocationPtr peerLocation,
                                         IPublicationMetaDataPtr metaData
                                         );

      virtual void notifyPeerDisconnected(IPeerLocationPtr peerLocation);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher => IPublicationFetcherDelegate
      #pragma mark

      virtual void onPublicationFetcherComplete(IPublicationFetcherPtr fetcher);

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher => (internal)
      #pragma mark

      virtual bool isShutdown() const;

      String log(const char *message) const;

      RecursiveLock &getLock() const {return mLock;}

      void cancel();
      void step();

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadDocumentFetcher => (data)
      #pragma mark

      mutable RecursiveLock mLock;
      PUID mID;
      ConversationThreadDocumentFetcherWeakPtr mThisWeak;

      IConversationThreadDocumentFetcherDelegatePtr mDelegate;

      IPublicationRepositoryPtr mRepository;

      IPeerLocationPtr mFetcherPeerLocation;
      IPublicationFetcherPtr mFetcher;

      typedef std::pair<IPeerLocationPtr, IPublicationMetaDataPtr> PeerLocationPublicationPair;
      typedef std::list<PeerLocationPublicationPair> PendingPublicationList;
      PendingPublicationList mPendingPublications;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::IConversationThreadDocumentFetcherDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::internal::IConversationThreadDocumentFetcherPtr, IConversationThreadDocumentFetcherPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationMetaDataPtr, IPublicationMetaDataPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationPtr, IPublicationPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPeerLocationPtr, IPeerLocationPtr)
ZS_DECLARE_PROXY_METHOD_3(onConversationThreadDocumentFetcherPublicationUpdated, IConversationThreadDocumentFetcherPtr, IPeerLocationPtr, IPublicationPtr)
ZS_DECLARE_PROXY_METHOD_3(onConversationThreadDocumentFetcherPublicationGone, IConversationThreadDocumentFetcherPtr, IPeerLocationPtr, IPublicationMetaDataPtr)
ZS_DECLARE_PROXY_END()
