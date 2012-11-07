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

#include <hookflash/stack/hookflashTypes.h>
#include <hookflash/stack/IPublicationMetaData.h>
#include <zsLib/Proxy.h>


namespace hookflash
{
  namespace stack
  {
    interaction IPublicationRepository
    {
      typedef IPublicationMetaData::SubscribeToRelationshipsMap SubscribeToRelationshipsMap;

      virtual IPublicationPublisherPtr publish(
                                               IPublicationPublisherDelegatePtr delegate,
                                               IPublicationPtr publication
                                               ) = 0;

      virtual IPublicationFetcherPtr fetch(
                                           IPublicationFetcherDelegatePtr delegate,
                                           IPublicationMetaDataPtr metaData
                                           ) = 0;


      virtual IPublicationRemoverPtr remove(
                                            IPublicationRemoverDelegatePtr delegate,
                                            IPublicationPtr publication
                                            ) = 0;

      virtual IPublicationSubscriptionPtr subscribeLocal(
                                                         IPublicationSubscriptionDelegatePtr delegate,
                                                         const char *publicationPath,
                                                         const SubscribeToRelationshipsMap &relationships
                                                         ) = 0;

      virtual IPublicationSubscriptionPtr subscribeFinder(
                                                          IPublicationSubscriptionDelegatePtr delegate,
                                                          const char *publicationPath,
                                                          const SubscribeToRelationshipsMap &relationships
                                                          ) = 0;

      virtual IPublicationSubscriptionPtr subscribePeer(
                                                        IPublicationSubscriptionDelegatePtr delegate,
                                                        const char *publicationPath,
                                                        const SubscribeToRelationshipsMap &relationships,
                                                        const char *peerSourceContactID,
                                                        const char *peerSourceLocationID
                                                        ) = 0;
    };

    interaction IPublicationPublisher
    {
      typedef zsLib::WORD WORD;
      typedef zsLib::String String;

      virtual void cancel() = 0;
      virtual bool isComplete() const = 0;

      virtual bool wasSuccessful() const = 0;

      virtual WORD getErrorResult() const = 0;
      virtual String getErrorReason() const = 0;

      virtual IPublicationPtr getPublication() const = 0;
    };

    interaction IPublicationPublisherDelegate
    {
      virtual void onPublicationPublisherComplete(IPublicationPublisherPtr fetcher) = 0;
    };

    interaction IPublicationFetcher
    {
      typedef zsLib::WORD WORD;
      typedef zsLib::String String;

      virtual void cancel() = 0;
      virtual bool isComplete() const = 0;

      virtual bool wasSuccessful() const = 0;

      virtual WORD getErrorResult() const = 0;
      virtual String getErrorReason() const = 0;

      virtual IPublicationPtr getFetchedPublication() const = 0;

      virtual IPublicationMetaDataPtr getPublicationMetaData() const = 0;
    };

    interaction IPublicationFetcherDelegate
    {
      virtual void onPublicationFetcherComplete(IPublicationFetcherPtr fetcher) = 0;
    };

    interaction IPublicationRemover
    {
      typedef zsLib::WORD WORD;
      typedef zsLib::String String;

      virtual void cancel() = 0;
      virtual bool isComplete() const = 0;

      virtual bool wasSuccessful() const = 0;

      virtual WORD getErrorResult() const = 0;
      virtual String getErrorReason() const = 0;

      virtual IPublicationPtr getPublication() const = 0;
    };

    interaction IPublicationRemoverDelegate
    {
      virtual void onPublicationRemoverComplete(IPublicationRemoverPtr fetcher) = 0;
    };

    interaction IPublicationSubscription
    {
      enum PublicationSubscriptionStates
      {
        PublicationSubscriptionState_Pending,
        PublicationSubscriptionState_Established,
        PublicationSubscriptionState_ShuttingDown,
        PublicationSubscriptionState_Shutdown,
      };

      static const char *toString(PublicationSubscriptionStates state);

      virtual void cancel() = 0;

      virtual PublicationSubscriptionStates getState() const = 0;
      virtual IPublicationMetaDataPtr getSource() const = 0;
    };

    interaction IPublicationSubscriptionDelegate
    {
      typedef IPublicationSubscription::PublicationSubscriptionStates PublicationSubscriptionStates;

      virtual void onPublicationSubscriptionStateChanged(
                                                         IPublicationSubscriptionPtr subcription,
                                                         PublicationSubscriptionStates state
                                                         ) = 0;

      virtual void onPublicationSubscriptionPublicationUpdated(
                                                               IPublicationSubscriptionPtr subscription,
                                                               IPublicationMetaDataPtr metaData
                                                               ) = 0;

      virtual void onPublicationSubscriptionPublicationGone(
                                                            IPublicationSubscriptionPtr subscription,
                                                            IPublicationMetaDataPtr metaData
                                                            ) = 0;
    };

    interaction IPublicationRepositoryPeerCache
    {
      typedef zsLib::ULONG ULONG;

      virtual bool getNextVersionToNotifyAboutAndMarkNotified(
                                                              IPublicationPtr publication,
                                                              ULONG &ioMaxSizeAvailableInBytes,
                                                              ULONG &outNotifyFromVersion,
                                                              ULONG &outNotifyToVersion
                                                              ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IPublicationPublisherDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationPublisherPtr, IPublicationPublisherPtr)
ZS_DECLARE_PROXY_METHOD_1(onPublicationPublisherComplete, IPublicationPublisherPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IPublicationFetcherDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationFetcherPtr, IPublicationFetcherPtr)
ZS_DECLARE_PROXY_METHOD_1(onPublicationFetcherComplete, IPublicationFetcherPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IPublicationRemoverDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationRemoverPtr, IPublicationRemoverPtr)
ZS_DECLARE_PROXY_METHOD_1(onPublicationRemoverComplete, IPublicationRemoverPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IPublicationSubscriptionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationMetaDataPtr, IPublicationMetaDataPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationSubscriptionPtr, IPublicationSubscriptionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPublicationSubscriptionDelegate::PublicationSubscriptionStates, PublicationSubscriptionStates)
ZS_DECLARE_PROXY_METHOD_2(onPublicationSubscriptionStateChanged, IPublicationSubscriptionPtr, PublicationSubscriptionStates)
ZS_DECLARE_PROXY_METHOD_2(onPublicationSubscriptionPublicationUpdated, IPublicationSubscriptionPtr, IPublicationMetaDataPtr)
ZS_DECLARE_PROXY_METHOD_2(onPublicationSubscriptionPublicationGone, IPublicationSubscriptionPtr, IPublicationMetaDataPtr)
ZS_DECLARE_PROXY_END()
