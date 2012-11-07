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

#include <zsLib/zsTypes.h>
#include <zsLib/String.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>

#include <cryptopp/secblock.h>

#include <hookflash/services/IICESocket.h>

namespace hookflash
{
  namespace stack
  {
    typedef CryptoPP::SecByteBlock SecureByteBlock;

    struct Location
    {
      typedef zsLib::WORD WORD;
      typedef zsLib::String String;
      typedef zsLib::IPAddress IPAddress;
      typedef hookflash::services::IICESocket::CandidateList CandidateList;

      String    mID;
      String    mDeviceID;
      IPAddress mIPAddress;
      String    mUserAgent;
      String    mOS;
      String    mSystem;
      String    mHost;

      String    mLocationSalt;
      String    mLocationFindSecretProof;

      String    mContactID;

      WORD         mReasonID;
      String       mReason;

      CandidateList mCandidates;

      Location() : mReasonID(0) {}

      bool hasData() const
      {
        return (!mID.isEmpty() || ! mDeviceID.isEmpty() || !mUserAgent.isEmpty() || !mOS.isEmpty() || !mSystem.isEmpty() || !mHost.isEmpty() ||
                !mContactID.isEmpty() || mCandidates.size() > 0 || mReasonID > 0 || !mReason.isEmpty());
      }
    };
    typedef std::list<Location> LocationList;

    interaction IAccount;
    typedef boost::shared_ptr<IAccount> IAccountPtr;
    typedef boost::weak_ptr<IAccount> IAccountWeakPtr;

    interaction IAccountDelegate;
    typedef boost::shared_ptr<IAccountDelegate> IAccountDelegatePtr;
    typedef boost::weak_ptr<IAccountDelegate> IAccountDelegateWeakPtr;
    typedef zsLib::Proxy<IAccountDelegate> IAccountDelegateProxy;

    interaction IBootstrappedNetwork;
    typedef boost::shared_ptr<IBootstrappedNetwork> IBootstrappedNetworkPtr;
    typedef boost::weak_ptr<IBootstrappedNetwork> IBootstrappedNetworkWeakPtr;

    interaction IBootstrappedNetworkDelegate;
    typedef boost::shared_ptr<IBootstrappedNetworkDelegate> IBootstrappedNetworkDelegatePtr;
    typedef boost::weak_ptr<IBootstrappedNetworkDelegate> IBootstrappedNetworkDelegateWeakPtr;
    typedef zsLib::Proxy<IBootstrappedNetworkDelegate> IBootstrappedNetworkDelegateProxy;

    interaction IBootstrappedNetworkSubscription;
    typedef boost::shared_ptr<IBootstrappedNetworkSubscription> IBootstrappedNetworkSubscriptionPtr;
    typedef boost::weak_ptr<IBootstrappedNetworkSubscription> IBootstrappedNetworkSubscriptionWeakPtr;

    interaction IBootstrappedNetworkFetchSignedSaltQuery;
    typedef boost::shared_ptr<IBootstrappedNetworkFetchSignedSaltQuery> IBootstrappedNetworkFetchSignedSaltQueryPtr;
    typedef boost::weak_ptr<IBootstrappedNetworkFetchSignedSaltQuery> IBootstrappedNetworkFetchSignedSaltQueryWeakPtr;

    interaction IBootstrappedNetworkFetchSignedSaltQueryDelegate;
    typedef boost::shared_ptr<IBootstrappedNetworkFetchSignedSaltQueryDelegate> IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr;
    typedef boost::weak_ptr<IBootstrappedNetworkFetchSignedSaltQueryDelegate> IBootstrappedNetworkFetchSignedSaltQueryDelegateWeakPtr;
    typedef zsLib::Proxy<IBootstrappedNetworkFetchSignedSaltQueryDelegate> IBootstrappedNetworkFetchSignedSaltQueryDelegateProxy;

    interaction IConnectionSubscription;
    typedef boost::shared_ptr<IConnectionSubscription> IConnectionSubscriptionPtr;
    typedef boost::weak_ptr<IConnectionSubscription> IConnectionSubscriptionWeakPtr;

    interaction IConnectionSubscriptionDelegate;
    typedef boost::shared_ptr<IConnectionSubscriptionDelegate> IConnectionSubscriptionDelegatePtr;
    typedef boost::weak_ptr<IConnectionSubscriptionDelegate> IConnectionSubscriptionDelegateWeakPtr;
    typedef zsLib::Proxy<IConnectionSubscriptionDelegate> IConnectionSubscriptionDelegateProxy;

    interaction IConnectionSubscriptionMessage;
    typedef boost::shared_ptr<IConnectionSubscriptionMessage> IConnectionSubscriptionMessagePtr;
    typedef boost::weak_ptr<IConnectionSubscriptionMessage> IConnectionSubscriptionMessageWeakPtr;

    interaction IHelper;
    typedef boost::shared_ptr<IHelper> IHelperPtr;
    typedef boost::weak_ptr<IHelper> IHelperWeakPtr;

    interaction IMessageRequester;
    typedef boost::shared_ptr<IMessageRequester> IMessageRequesterPtr;
    typedef boost::weak_ptr<IMessageRequester> IMessageRequesterWeakPtr;

    interaction IMessageRequesterDelegate;
    typedef boost::shared_ptr<IMessageRequesterDelegate> IMessageRequesterDelegatePtr;
    typedef boost::weak_ptr<IMessageRequesterDelegate> IMessageRequesterDelegateWeakPtr;
    typedef zsLib::Proxy<IMessageRequesterDelegate> IMessageRequesterDelegateProxy;

    interaction IPeerContactProfile;
    typedef boost::shared_ptr<IPeerContactProfile> IPeerContactProfilePtr;
    typedef boost::weak_ptr<IPeerContactProfile> IPeerContactProfileWeakPtr;

    interaction IPeerFiles;
    typedef boost::shared_ptr<IPeerFiles> IPeerFilesPtr;
    typedef boost::weak_ptr<IPeerFiles> IPeerFilesWeakPtr;

    interaction IPeerFilePublic;
    typedef boost::shared_ptr<IPeerFilePublic> IPeerFilePublicPtr;
    typedef boost::weak_ptr<IPeerFilePublic> IPeerFilePublicWeakPtr;

    interaction IPeerFilePrivate;
    typedef boost::shared_ptr<IPeerFilePrivate> IPeerFilePrivatePtr;
    typedef boost::weak_ptr<IPeerFilePrivate> IPeerFilePrivateWeakPtr;

    interaction IPeerLocation;
    typedef boost::shared_ptr<IPeerLocation> IPeerLocationPtr;
    typedef boost::weak_ptr<IPeerLocation> IPeerLocationWeakPtr;

    interaction IPeerSubscription;
    typedef boost::shared_ptr<IPeerSubscription> IPeerSubscriptionPtr;
    typedef boost::weak_ptr<IPeerSubscription> IPeerSubscriptionWeakPtr;

    interaction IPeerSubscriptionMessage;
    typedef boost::shared_ptr<IPeerSubscriptionMessage> IPeerSubscriptionMessagePtr;
    typedef boost::weak_ptr<IPeerSubscriptionMessage> IPeerSubscriptionMessageWeakPtr;

    interaction IPeerSubscriptionDelegate;
    typedef boost::shared_ptr<IPeerSubscriptionDelegate> IPeerSubscriptionDelegatePtr;
    typedef boost::weak_ptr<IPeerSubscriptionDelegate> IPeerSubscriptionDelegateWeakPtr;
    typedef zsLib::Proxy<IPeerSubscriptionDelegate> IPeerSubscriptionDelegateProxy;

    interaction IPublicationMetaData;
    typedef boost::shared_ptr<IPublicationMetaData> IPublicationMetaDataPtr;
    typedef boost::weak_ptr<IPublicationMetaData> IPublicationMetaDataWeakPtr;

    interaction IPublication;
    typedef boost::shared_ptr<IPublication> IPublicationPtr;
    typedef boost::weak_ptr<IPublication> IPublicationWeakPtr;

    interaction IPublicationPublisher;
    typedef boost::shared_ptr<IPublicationPublisher> IPublicationPublisherPtr;
    typedef boost::weak_ptr<IPublicationPublisher> IPublicationPublisherWeakPtr;

    interaction IPublicationPublisherDelegate;
    typedef boost::shared_ptr<IPublicationPublisherDelegate> IPublicationPublisherDelegatePtr;
    typedef boost::weak_ptr<IPublicationPublisherDelegate> IPublicationPublisherDelegateWeakPtr;
    typedef zsLib::Proxy<IPublicationPublisherDelegate> IPublicationPublisherDelegateProxy;

    interaction IPublicationFetcher;
    typedef boost::shared_ptr<IPublicationFetcher> IPublicationFetcherPtr;
    typedef boost::weak_ptr<IPublicationFetcher> IPublicationFetcherWeakPtr;

    interaction IPublicationFetcherDelegate;
    typedef boost::shared_ptr<IPublicationFetcherDelegate> IPublicationFetcherDelegatePtr;
    typedef boost::weak_ptr<IPublicationFetcherDelegate> IPublicationFetcherDelegateWeakPtr;
    typedef zsLib::Proxy<IPublicationFetcherDelegate> IPublicationFetcherDelegateProxy;

    interaction IPublicationRemover;
    typedef boost::shared_ptr<IPublicationRemover> IPublicationRemoverPtr;
    typedef boost::weak_ptr<IPublicationRemover> IPublicationRemoverWeakPtr;

    interaction IPublicationRemoverDelegate;
    typedef boost::shared_ptr<IPublicationRemoverDelegate> IPublicationRemoverDelegatePtr;
    typedef boost::weak_ptr<IPublicationRemoverDelegate> IPublicationRemoverDelegateWeakPtr;
    typedef zsLib::Proxy<IPublicationRemoverDelegate> IPublicationRemoverDelegateProxy;

    interaction IPublicationRepository;
    typedef boost::shared_ptr<IPublicationRepository> IPublicationRepositoryPtr;
    typedef boost::weak_ptr<IPublicationRepository> IPublicationRepositoryWeakPtr;

    interaction IPublicationRepositoryPeerCache;
    typedef boost::shared_ptr<IPublicationRepositoryPeerCache> IPublicationRepositoryPeerCachePtr;
    typedef boost::weak_ptr<IPublicationRepositoryPeerCache> IPublicationRepositoryPeerCacheWeakPtr;

    interaction IPublicationSubscription;
    typedef boost::shared_ptr<IPublicationSubscription> IPublicationSubscriptionPtr;
    typedef boost::weak_ptr<IPublicationSubscription> IPublicationSubscriptionWeakPtr;

    interaction IPublicationSubscriptionDelegate;
    typedef boost::shared_ptr<IPublicationSubscriptionDelegate> IPublicationSubscriptionDelegatePtr;
    typedef boost::weak_ptr<IPublicationSubscriptionDelegate> IPublicationSubscriptionDelegateWeakPtr;
    typedef zsLib::Proxy<IPublicationSubscriptionDelegate> IPublicationSubscriptionDelegateProxy;

    interaction IXMLDiff;
    typedef boost::shared_ptr<IXMLDiff> IXMLDiffPtr;
    typedef boost::weak_ptr<IXMLDiff> IXMLDiffWeakPtr;
  }
}
