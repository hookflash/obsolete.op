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

#include <zsLib/types.h>
#include <zsLib/String.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>
#include <zsLib/ProxySubscriptions.h>

#include <cryptopp/secblock.h>

#include <openpeer/services/IICESocket.h>

namespace openpeer
{
  namespace services
  {
    interaction IHTTP;
  }

  namespace stack
  {
    using zsLib::PUID;
    using zsLib::BYTE;
    using zsLib::WORD;
    using zsLib::UINT;
    using zsLib::ULONG;
    using zsLib::Time;
    using zsLib::Duration;
    using zsLib::String;
    using zsLib::RecursiveLock;
    using zsLib::AutoRecursiveLock;
    typedef boost::shared_ptr<AutoRecursiveLock> AutoRecursiveLockPtr;
    using zsLib::IPAddress;
    using zsLib::IMessageQueuePtr;
    using zsLib::XML::ElementPtr;
    using zsLib::XML::DocumentPtr;
    using zsLib::XML::NodePtr;
    using services::IHTTP;

    using services::IICESocket;
    using services::IRSAPrivateKey;
    using services::IRSAPrivateKeyPtr;
    using services::IRSAPublicKey;
    using services::IRSAPublicKeyPtr;

    typedef services::SecureByteBlock SecureByteBlock;
    typedef boost::shared_ptr<SecureByteBlock> SecureByteBlockPtr;
    typedef boost::weak_ptr<SecureByteBlock> SecureByteBlockWeakPtr;

    interaction ILocation;
    typedef boost::shared_ptr<ILocation> ILocationPtr;
    typedef boost::weak_ptr<ILocation> ILocationWeakPtr;

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark Candidate
    #pragma mark

    struct Candidate : public IICESocket::Candidate
    {
      String mClass;
      String mTransport;

      String mAccessToken;
      String mAccessSecretProof;

      Candidate();
      Candidate(const Candidate &candidate);
      Candidate(const IICESocket::Candidate &candidate);
      bool hasData() const;
      String getDebugValueString(bool includeCommaPrefix = true) const;
    };
    typedef boost::shared_ptr<Candidate> CandidatePtr;
    typedef boost::weak_ptr<Candidate> CandidateWeakPtr;
    typedef std::list<Candidate> CandidateList;

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark LocationInfo
    #pragma mark

    struct LocationInfo
    {
      ILocationPtr mLocation;

      String    mDeviceID;
      IPAddress mIPAddress;
      String    mUserAgent;
      String    mOS;
      String    mSystem;
      String    mHost;

      CandidateList mCandidates;

      bool hasData() const;
      String getDebugValueString(bool includeCommaPrefix = true) const;
    };

    typedef boost::shared_ptr<LocationInfo> LocationInfoPtr;
    typedef boost::weak_ptr<LocationInfo> LocationInfoWeakPtr;

    typedef std::list<LocationInfo> LocationInfoList;
    typedef boost::shared_ptr<LocationInfoList> LocationInfoListPtr;
    typedef boost::weak_ptr<LocationInfoList> LocationInfoListWeakPtr;

    typedef std::list<ElementPtr> IdentityBundleElementList;
    typedef boost::shared_ptr<IdentityBundleElementList> IdentityBundleElementListPtr;
    typedef boost::weak_ptr<IdentityBundleElementList> IdentityBundleElementListWeakPtr;

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

    interaction ICache;
    typedef boost::shared_ptr<ICache> ICachePtr;
    typedef boost::weak_ptr<ICache> ICacheWeakPtr;

    interaction ICacheDelegate;
    typedef boost::shared_ptr<ICacheDelegate> ICacheDelegatePtr;
    typedef boost::weak_ptr<ICacheDelegate> ICacheDelegateWeakPtr;

    interaction IDiff;
    typedef boost::shared_ptr<IDiff> IDiffPtr;
    typedef boost::weak_ptr<IDiff> IDiffWeakPtr;

    interaction IHelper;
    typedef boost::shared_ptr<IHelper> IHelperPtr;
    typedef boost::weak_ptr<IHelper> IHelperWeakPtr;

    interaction IMessageIncoming;
    typedef boost::shared_ptr<IMessageIncoming> IMessageIncomingPtr;
    typedef boost::weak_ptr<IMessageIncoming> IMessageIncomingWeakPtr;

    interaction IMessageMonitor;
    typedef boost::shared_ptr<IMessageMonitor> IMessageMonitorPtr;
    typedef boost::weak_ptr<IMessageMonitor> IMessageMonitorWeakPtr;

    interaction IMessageMonitorDelegate;
    typedef boost::shared_ptr<IMessageMonitorDelegate> IMessageMonitorDelegatePtr;
    typedef boost::weak_ptr<IMessageMonitorDelegate> IMessageMonitorDelegateWeakPtr;
    typedef zsLib::Proxy<IMessageMonitorDelegate> IMessageMonitorDelegateProxy;

    interaction IMessageSource;
    typedef boost::shared_ptr<IMessageSource> IMessageSourcePtr;
    typedef boost::weak_ptr<IMessageSource> IMessageSourceWeakPtr;

    interaction ILocation;
    typedef boost::shared_ptr<ILocation> ILocationPtr;
    typedef boost::weak_ptr<ILocation> ILocationWeakPtr;

    typedef std::list<ILocationPtr> LocationList;
    typedef boost::shared_ptr<LocationList> LocationListPtr;
    typedef boost::weak_ptr<LocationList> LocationListWeakPtr;

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

    interaction IPeer;
    typedef boost::shared_ptr<IPeer> IPeerPtr;
    typedef boost::weak_ptr<IPeer> IPeerWeakPtr;

    interaction IPeerSubscription;
    typedef boost::shared_ptr<IPeerSubscription> IPeerSubscriptionPtr;
    typedef boost::weak_ptr<IPeerSubscription> IPeerSubscriptionWeakPtr;

    interaction IPeerSubscriptionDelegate;
    typedef boost::shared_ptr<IPeerSubscriptionDelegate> IPeerSubscriptionDelegatePtr;
    typedef boost::weak_ptr<IPeerSubscriptionDelegate> IPeerSubscriptionDelegateWeakPtr;
    typedef zsLib::Proxy<IPeerSubscriptionDelegate> IPeerSubscriptionDelegateProxy;

    interaction IPublication;
    typedef boost::shared_ptr<IPublication> IPublicationPtr;
    typedef boost::weak_ptr<IPublication> IPublicationWeakPtr;

    interaction IPublicationMetaData;
    typedef boost::shared_ptr<IPublicationMetaData> IPublicationMetaDataPtr;
    typedef boost::weak_ptr<IPublicationMetaData> IPublicationMetaDataWeakPtr;

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

    interaction IStack;
    typedef boost::shared_ptr<IStack> IStackPtr;
    typedef boost::weak_ptr<IStack> IStackWeakPtr;

    interaction IServiceCertificates;
    typedef boost::shared_ptr<IServiceCertificates> IServiceCertificatesPtr;
    typedef boost::weak_ptr<IServiceCertificates> IServiceCertificatesWeakPtr;

    interaction IServiceCertificatesValidateQuery;
    typedef boost::shared_ptr<IServiceCertificatesValidateQuery> IServiceCertificatesValidateQueryPtr;
    typedef boost::weak_ptr<IServiceCertificatesValidateQuery> IServiceCertificatesValidateQueryWeakPtr;

    interaction IServiceCertificatesValidateQueryDelegate;
    typedef boost::shared_ptr<IServiceCertificatesValidateQueryDelegate> IServiceCertificatesValidateQueryDelegatePtr;
    typedef boost::weak_ptr<IServiceCertificatesValidateQueryDelegate> IServiceCertificatesValidateQueryDelegateWeakPtr;
    typedef zsLib::Proxy<IServiceCertificatesValidateQueryDelegate> IServiceCertificatesValidateQueryDelegateProxy;

    interaction IServiceSalt;
    typedef boost::shared_ptr<IServiceSalt> IServiceSaltPtr;
    typedef boost::weak_ptr<IServiceSalt> IServiceSaltWeakPtr;

    interaction IServiceSaltFetchSignedSaltQuery;
    typedef boost::shared_ptr<IServiceSaltFetchSignedSaltQuery> IServiceSaltFetchSignedSaltQueryPtr;
    typedef boost::weak_ptr<IServiceSaltFetchSignedSaltQuery> IServiceSaltFetchSignedSaltQueryWeakPtr;

    interaction IServiceSaltFetchSignedSaltQueryDelegate;
    typedef boost::shared_ptr<IServiceSaltFetchSignedSaltQueryDelegate> IServiceSaltFetchSignedSaltQueryDelegatePtr;
    typedef boost::weak_ptr<IServiceSaltFetchSignedSaltQueryDelegate> IServiceSaltFetchSignedSaltQueryDelegateWeakPtr;
    typedef zsLib::Proxy<IServiceSaltFetchSignedSaltQueryDelegate> IServiceSaltFetchSignedSaltQueryDelegateProxy;

    interaction IServiceIdentity;
    typedef boost::shared_ptr<IServiceIdentity> IServiceIdentityPtr;
    typedef boost::weak_ptr<IServiceIdentity> IServiceIdentityWeakPtr;

    interaction IServiceIdentitySession;
    typedef boost::shared_ptr<IServiceIdentitySession> IServiceIdentitySessionPtr;
    typedef boost::weak_ptr<IServiceIdentitySession> IServiceIdentitySessionWeakPtr;

    interaction IServiceIdentitySessionDelegate;
    typedef boost::shared_ptr<IServiceIdentitySessionDelegate> IServiceIdentitySessionDelegatePtr;
    typedef boost::weak_ptr<IServiceIdentitySessionDelegate> IServiceIdentitySessionDelegateWeakPtr;
    typedef zsLib::Proxy<IServiceIdentitySessionDelegate> IServiceIdentitySessionDelegateProxy;

    interaction IServiceIdentityProofBundleQuery;
    typedef boost::shared_ptr<IServiceIdentityProofBundleQuery> IServiceIdentityProofBundleQueryPtr;
    typedef boost::weak_ptr<IServiceIdentityProofBundleQuery> IServiceIdentityProofBundleQueryWeakPtr;

    interaction IServiceIdentityProofBundleQueryDelegate;
    typedef boost::shared_ptr<IServiceIdentityProofBundleQueryDelegate> IServiceIdentityProofBundleQueryDelegatePtr;
    typedef boost::weak_ptr<IServiceIdentityProofBundleQueryDelegate> IServiceIdentityProofBundleQueryDelegateWeakPtr;
    typedef zsLib::Proxy<IServiceIdentityProofBundleQueryDelegate> IServiceIdentityProofBundleQueryDelegateProxy;

    typedef std::list<IServiceIdentitySessionPtr> ServiceIdentitySessionList;
    typedef boost::shared_ptr<ServiceIdentitySessionList> ServiceIdentitySessionListPtr;
    typedef boost::weak_ptr<ServiceIdentitySessionList> ServiceIdentitySessionListWeakPtr;

    interaction IServiceLockbox;
    typedef boost::shared_ptr<IServiceLockbox> IServiceLockboxPtr;
    typedef boost::weak_ptr<IServiceLockbox> IServiceLockboxWeakPtr;

    interaction IServiceLockboxSession;
    typedef boost::shared_ptr<IServiceLockboxSession> IServiceLockboxSessionPtr;
    typedef boost::weak_ptr<IServiceLockboxSession> IServiceLockboxSessionWeakPtr;

    interaction IServiceLockboxSessionDelegate;
    typedef boost::shared_ptr<IServiceLockboxSessionDelegate> IServiceLockboxSessionDelegatePtr;
    typedef boost::weak_ptr<IServiceLockboxSessionDelegate> IServiceLockboxSessionDelegateWeakPtr;
    typedef zsLib::Proxy<IServiceLockboxSessionDelegate> IServiceLockboxSessionDelegateProxy;
    
    interaction IServiceNamespaceGrantSession;
    typedef boost::shared_ptr<IServiceNamespaceGrantSession> IServiceNamespaceGrantSessionPtr;
    typedef boost::weak_ptr<IServiceNamespaceGrantSession> IServiceNamespaceGrantSessionWeakPtr;

    interaction IServiceNamespaceGrantSessionDelegate;
    typedef boost::shared_ptr<IServiceNamespaceGrantSessionDelegate> IServiceNamespaceGrantSessionDelegatePtr;
    typedef boost::weak_ptr<IServiceNamespaceGrantSessionDelegate> IServiceNamespaceGrantSessionDelegateWeakPtr;
    typedef zsLib::Proxy<IServiceNamespaceGrantSessionDelegate> IServiceNamespaceGrantSessionDelegateProxy;

  }
}
