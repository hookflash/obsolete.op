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

#include <hookflash/stack/message/hookflashTypes.h>
#include <zsLib/zsTypes.h>
#include <zsLib/Proxy.h>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      class Account;
      typedef boost::shared_ptr<Account> AccountPtr;
      typedef boost::weak_ptr<Account> AccountWeakPtr;

      interaction IAccountAsyncDelegate;
      typedef boost::shared_ptr<IAccountAsyncDelegate> IAccountAsyncDelegatePtr;
      typedef boost::weak_ptr<IAccountAsyncDelegate> IAccountAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountAsyncDelegate> IAccountAsyncDelegateProxy;

      interaction IAccountForPeerSubscription;
      typedef boost::shared_ptr<IAccountForPeerSubscription> IAccountForPeerSubscriptionPtr;
      typedef boost::weak_ptr<IAccountForPeerSubscription> IAccountForPeerSubscriptionWeakPtr;
      typedef zsLib::Proxy<IAccountForPeerSubscription> IAccountForPeerSubscriptionProxy;

      interaction IAccountForAccountFinder;
      typedef boost::shared_ptr<IAccountForAccountFinder> IAccountForAccountFinderPtr;
      typedef boost::weak_ptr<IAccountForAccountFinder> IAccountForAccountFinderWeakPtr;

      interaction IAccountForAccountPeerLocation;
      typedef boost::shared_ptr<IAccountForAccountPeerLocation> IAccountForAccountPeerLocationPtr;
      typedef boost::weak_ptr<IAccountForAccountPeerLocation> IAccountForAccountPeerLocationWeakPtr;
      typedef zsLib::Proxy<IAccountForAccountPeerLocation> IAccountForAccountPeerLocationProxy;

      interaction IAccountForPublicationRepository;
      typedef boost::shared_ptr<IAccountForPublicationRepository> IAccountForPublicationRepositoryPtr;
      typedef boost::weak_ptr<IAccountForPublicationRepository> IAccountForPublicationRepositoryWeakPtr;

      interaction IAccountFinder;
      typedef boost::shared_ptr<IAccountFinder> IAccountFinderPtr;
      typedef boost::weak_ptr<IAccountFinder> IAccountFinderWeakPtr;

      interaction IAccountFinderDelegate;
      typedef boost::shared_ptr<IAccountFinderDelegate> IAccountFinderDelegatePtr;
      typedef boost::weak_ptr<IAccountFinderDelegate> IAccountFinderDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountFinderDelegate> IAccountFinderDelegateProxy;

      interaction IAccountFinderAsyncDelegate;
      typedef boost::shared_ptr<IAccountFinderAsyncDelegate> IAccountFinderAsyncDelegatePtr;
      typedef boost::weak_ptr<IAccountFinderAsyncDelegate> IAccountFinderAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountFinderAsyncDelegate> IAccountFinderAsyncDelegateProxy;

      class AccountFinder;
      typedef boost::shared_ptr<AccountFinder> AccountFinderPtr;
      typedef boost::weak_ptr<AccountFinder> AccountFinderWeakPtr;

      interaction IAccountPeerLocation;
      typedef boost::shared_ptr<IAccountPeerLocation> IAccountPeerLocationPtr;
      typedef boost::weak_ptr<IAccountPeerLocation> IAccountPeerLocationWeakPtr;

      interaction IAccountPeerLocationDelegate;
      typedef boost::shared_ptr<IAccountPeerLocationDelegate> IAccountPeerLocationDelegatePtr;
      typedef boost::weak_ptr<IAccountPeerLocationDelegate> IAccountPeerLocationDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountPeerLocationDelegate> IAccountPeerLocationDelegateProxy;

      interaction IAccountPeerLocationAsyncDelegate;
      typedef boost::shared_ptr<IAccountPeerLocationAsyncDelegate> IAccountPeerLocationAsyncDelegatePtr;
      typedef boost::weak_ptr<IAccountPeerLocationAsyncDelegate> IAccountPeerLocationAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountPeerLocationAsyncDelegate> IAccountPeerLocationAsyncDelegateProxy;

      class AccountPeerLocation;
      typedef boost::shared_ptr<AccountPeerLocation> AccountPeerLocationPtr;
      typedef boost::weak_ptr<AccountPeerLocation> AccountPeerLocationWeakPtr;

      class BootstrappedNetwork;
      typedef boost::shared_ptr<BootstrappedNetwork> BootstrappedNetworkPtr;
      typedef boost::weak_ptr<BootstrappedNetwork> BootstrappedNetworkWeakPtr;

      interaction IBootstrappedNetworkForAccount;
      typedef boost::shared_ptr<IBootstrappedNetworkForAccount> IBootstrappedNetworkForAccountPtr;
      typedef boost::weak_ptr<IBootstrappedNetworkForAccount> IBootstrappedNetworkForAccountWeakPtr;

      interaction IBootstrappedNetworkForAccountFinder;
      typedef boost::shared_ptr<IBootstrappedNetworkForAccountFinder> IBootstrappedNetworkForAccountFinderPtr;
      typedef boost::weak_ptr<IBootstrappedNetworkForAccountFinder> IBootstrappedNetworkForAccountFinderWeakPtr;

      interaction Publication;
      typedef boost::shared_ptr<Publication> PublicationPtr;
      typedef boost::weak_ptr<Publication> PublicationWeakPtr;

      interaction IPublicationForPublicationRepository;
      typedef boost::shared_ptr<IPublicationForPublicationRepository> IPublicationForPublicationRepositoryPtr;
      typedef boost::weak_ptr<IPublicationForPublicationRepository> IPublicationForPublicationRepositoryWeakPtr;

      interaction IPublicationForMessages;
      typedef boost::shared_ptr<IPublicationForMessages> IPublicationForMessagesPtr;
      typedef boost::weak_ptr<IPublicationForMessages> IPublicationForMessagesWeakPtr;

      interaction PublicationMetaData;
      typedef boost::shared_ptr<PublicationMetaData> PublicationMetaDataPtr;
      typedef boost::weak_ptr<PublicationMetaData> PublicationMetaDataWeakPtr;

      interaction IPublicationMetaDataForPublicationRepository;
      typedef boost::shared_ptr<IPublicationMetaDataForPublicationRepository> IPublicationMetaDataForPublicationRepositoryPtr;
      typedef boost::weak_ptr<IPublicationMetaDataForPublicationRepository> IPublicationMetaDataForPublicationRepositoryWeakPtr;

      interaction IPublicationMetaDataForMessages;
      typedef boost::shared_ptr<IPublicationMetaDataForMessages> IPublicationMetaDataForMessagesPtr;
      typedef boost::weak_ptr<IPublicationMetaDataForMessages> IPublicationMetaDataForMessagesWeakPtr;

      interaction PublicationRepository;
      typedef boost::shared_ptr<PublicationRepository> PublicationRepositoryPtr;
      typedef boost::weak_ptr<PublicationRepository> PublicationRepositoryWeakPtr;

      interaction IPublicationRepositoryForAccount;
      typedef boost::shared_ptr<IPublicationRepositoryForAccount> IPublicationRepositoryForAccountPtr;
      typedef boost::weak_ptr<IPublicationRepositoryForAccount> IPublicationRepositoryForAccountWeakPtr;

      class MessageRequester;
      typedef boost::shared_ptr<MessageRequester> MessageRequesterPtr;
      typedef boost::weak_ptr<MessageRequester> MessageRequesterWeakPtr;

      interaction IMessageRequesterAsyncDelegate;
      typedef boost::shared_ptr<IMessageRequesterAsyncDelegate> IMessageRequesterAsyncDelegatePtr;
      typedef boost::weak_ptr<IMessageRequesterAsyncDelegate> IMessageRequesterAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IMessageRequesterAsyncDelegate> IMessageRequesterAsyncDelegateProxy;

      interaction IMessageRequesterForAccount;
      typedef boost::shared_ptr<IMessageRequesterForAccount> IMessageRequesterForAccountPtr;
      typedef boost::weak_ptr<IMessageRequesterForAccount> IMessageRequesterForAccountWeakPtr;

      interaction IMessageRequesterForAccountPeerLocation;
      typedef boost::shared_ptr<IMessageRequesterForAccountPeerLocation> IMessageRequesterForAccountPeerLocationPtr;
      typedef boost::weak_ptr<IMessageRequesterForAccountPeerLocation> IMessageRequesterForAccountPeerLocationWeakPtr;

      class MessageRequesterManager;
      typedef boost::shared_ptr<MessageRequesterManager> MessageRequesterManagerPtr;
      typedef boost::weak_ptr<MessageRequesterManager> MessageRequesterManagerWeakPtr;

      class PeerContactProfile;
      typedef boost::shared_ptr<PeerContactProfile> PeerContactProfilePtr;
      typedef boost::weak_ptr<PeerContactProfile> PeerContactProfileWeakPtr;

      class PeerFiles;
      typedef boost::shared_ptr<PeerFiles> PeerFilesPtr;
      typedef boost::weak_ptr<PeerFiles> PeerFilesWeakPtr;

      class PeerFilePublic;
      typedef boost::shared_ptr<PeerFilePublic> PeerFilePublicPtr;
      typedef boost::weak_ptr<PeerFilePublic> PeerFilePublicWeakPtr;

      class PeerFilePrivate;
      typedef boost::shared_ptr<PeerFilePrivate> PeerFilePrivatePtr;
      typedef boost::weak_ptr<PeerFilePrivate> PeerFilePrivateWeakPtr;

      class PeerSubscription;
      typedef boost::shared_ptr<PeerSubscription> PeerSubscriptionPtr;
      typedef boost::weak_ptr<PeerSubscription> PeerSubscriptionWeakPtr;

      interaction IPeerSubscriptionForAccount;
      typedef boost::shared_ptr<IPeerSubscriptionForAccount> IPeerSubscriptionForAccountPtr;
      typedef boost::weak_ptr<IPeerSubscriptionForAccount> IPeerSubscriptionForAccountWeakPtr;

      class XMLDiff;
      typedef boost::shared_ptr<XMLDiff> XMLDiffPtr;
      typedef boost::weak_ptr<XMLDiff> XMLDiffWeakPtr;
    }
  }
}
