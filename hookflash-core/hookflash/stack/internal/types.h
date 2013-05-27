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

#include <hookflash/stack/types.h>
#include <hookflash/stack/message/types.h>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Noop;
      using zsLib::CSTR;
      using zsLib::Seconds;
      using zsLib::Hours;
      using zsLib::Timer;
      using zsLib::TimerPtr;
      using zsLib::ITimerDelegate;
      using zsLib::MessageQueueAssociator;
      using zsLib::XML::Element;
      using zsLib::XML::Document;
      using zsLib::XML::DocumentPtr;
      using zsLib::XML::NodePtr;
      using zsLib::XML::Text;
      using zsLib::XML::TextPtr;
      using zsLib::XML::Element;
      using zsLib::XML::AttributePtr;
      using zsLib::XML::Generator;
      using zsLib::XML::GeneratorPtr;

      using services::IDNS;
      using services::IICESocket;
      using services::IRUDPICESocket;
      using services::IRUDPICESocketPtr;
      using services::IRUDPICESocketDelegate;
      using services::IRUDPICESocketSession;
      using services::IRUDPICESocketSessionDelegate;
      using services::IRUDPICESocketSubscriptionPtr;
      using services::IRUDPMessaging;
      using services::IRUDPMessagingDelegate;
      using services::IDNSDelegate;
      using services::IDNSQuery;
      using services::IDNSQueryPtr;
      using services::IHTTP;
      using services::IHTTPQueryPtr;
      using services::IHTTPQueryDelegate;

      using namespace message;

      class Account;
      typedef boost::shared_ptr<Account> AccountPtr;
      typedef boost::weak_ptr<Account> AccountWeakPtr;

      interaction IAccountAsyncDelegate;
      typedef boost::shared_ptr<IAccountAsyncDelegate> IAccountAsyncDelegatePtr;
      typedef boost::weak_ptr<IAccountAsyncDelegate> IAccountAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountAsyncDelegate> IAccountAsyncDelegateProxy;

      class AccountFinder;
      typedef boost::shared_ptr<AccountFinder> AccountFinderPtr;
      typedef boost::weak_ptr<AccountFinder> AccountFinderWeakPtr;

      interaction IAccountFinderDelegate;
      typedef boost::shared_ptr<IAccountFinderDelegate> IAccountFinderDelegatePtr;
      typedef boost::weak_ptr<IAccountFinderDelegate> IAccountFinderDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountFinderDelegate> IAccountFinderDelegateProxy;

      interaction IAccountFinderAsyncDelegate;
      typedef boost::shared_ptr<IAccountFinderAsyncDelegate> IAccountFinderAsyncDelegatePtr;
      typedef boost::weak_ptr<IAccountFinderAsyncDelegate> IAccountFinderAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountFinderAsyncDelegate> IAccountFinderAsyncDelegateProxy;

      class AccountPeerLocation;
      typedef boost::shared_ptr<AccountPeerLocation> AccountPeerLocationPtr;
      typedef boost::weak_ptr<AccountPeerLocation> AccountPeerLocationWeakPtr;

      interaction IAccountPeerLocationDelegate;
      typedef boost::shared_ptr<IAccountPeerLocationDelegate> IAccountPeerLocationDelegatePtr;
      typedef boost::weak_ptr<IAccountPeerLocationDelegate> IAccountPeerLocationDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountPeerLocationDelegate> IAccountPeerLocationDelegateProxy;

      interaction IAccountPeerLocationAsyncDelegate;
      typedef boost::shared_ptr<IAccountPeerLocationAsyncDelegate> IAccountPeerLocationAsyncDelegatePtr;
      typedef boost::weak_ptr<IAccountPeerLocationAsyncDelegate> IAccountPeerLocationAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountPeerLocationAsyncDelegate> IAccountPeerLocationAsyncDelegateProxy;

      class BootstrappedNetwork;
      typedef boost::shared_ptr<BootstrappedNetwork> BootstrappedNetworkPtr;
      typedef boost::weak_ptr<BootstrappedNetwork> BootstrappedNetworkWeakPtr;

      interaction IBootstrappedNetworkAsyncDelegate;
      typedef boost::shared_ptr<IBootstrappedNetworkAsyncDelegate> IBootstrappedNetworkAsyncDelegatePtr;
      typedef boost::weak_ptr<IBootstrappedNetworkAsyncDelegate> IBootstrappedNetworkAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IBootstrappedNetworkAsyncDelegate> IBootstrappedNetworkAsyncDelegateProxy;

      class BootstrappedNetworkManager;
      typedef boost::shared_ptr<BootstrappedNetworkManager> BootstrappedNetworkManagerPtr;
      typedef boost::weak_ptr<BootstrappedNetworkManager> BootstrappedNetworkManagerWeakPtr;

      class ServiceCertificatesValidateQuery;
      typedef boost::shared_ptr<ServiceCertificatesValidateQuery> ServiceCertificatesValidateQueryPtr;
      typedef boost::weak_ptr<ServiceCertificatesValidateQuery> ServiceCertificatesValidateQueryWeakPtr;

      class Diff;
      typedef boost::shared_ptr<Diff> DiffPtr;
      typedef boost::weak_ptr<Diff> DiffWeakPtr;

      class Factory;
      typedef boost::shared_ptr<Factory> FactoryPtr;
      typedef boost::weak_ptr<Factory> FactoryWeakPtr;

      class Helper;
      typedef boost::shared_ptr<Helper> HelperPtr;
      typedef boost::weak_ptr<Helper> HelperWeakPtr;

      class MessageIncoming;
      typedef boost::shared_ptr<MessageIncoming> MessageIncomingPtr;
      typedef boost::weak_ptr<MessageIncoming> MessageIncomingWeakPtr;

      class Location;
      typedef boost::shared_ptr<Location> LocationPtr;
      typedef boost::weak_ptr<Location> LocationWeakPtr;

      class Peer;
      typedef boost::shared_ptr<Peer> PeerPtr;
      typedef boost::weak_ptr<Peer> PeerWeakPtr;

      interaction Publication;
      typedef boost::shared_ptr<Publication> PublicationPtr;
      typedef boost::weak_ptr<Publication> PublicationWeakPtr;

      interaction PublicationMetaData;
      typedef boost::shared_ptr<PublicationMetaData> PublicationMetaDataPtr;
      typedef boost::weak_ptr<PublicationMetaData> PublicationMetaDataWeakPtr;

      interaction IPublicationMetaDataForPublicationRepository;
      typedef boost::shared_ptr<IPublicationMetaDataForPublicationRepository> IPublicationMetaDataForPublicationRepositoryPtr;
      typedef boost::weak_ptr<IPublicationMetaDataForPublicationRepository> IPublicationMetaDataForPublicationRepositoryWeakPtr;

      interaction PublicationRepository;
      typedef boost::shared_ptr<PublicationRepository> PublicationRepositoryPtr;
      typedef boost::weak_ptr<PublicationRepository> PublicationRepositoryWeakPtr;

      class MessageMonitor;
      typedef boost::shared_ptr<MessageMonitor> MessageMonitorPtr;
      typedef boost::weak_ptr<MessageMonitor> MessageMonitorWeakPtr;

      interaction IMessageMonitorAsyncDelegate;
      typedef boost::shared_ptr<IMessageMonitorAsyncDelegate> IMessageMonitorAsyncDelegatePtr;
      typedef boost::weak_ptr<IMessageMonitorAsyncDelegate> IMessageMonitorAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IMessageMonitorAsyncDelegate> IMessageMonitorAsyncDelegateProxy;

      class MessageMonitorManager;
      typedef boost::shared_ptr<MessageMonitorManager> MessageMonitorManagerPtr;
      typedef boost::weak_ptr<MessageMonitorManager> MessageMonitorManagerWeakPtr;

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

      class ServiceIdentitySession;
      typedef boost::shared_ptr<ServiceIdentitySession> ServiceIdentitySessionPtr;
      typedef boost::weak_ptr<ServiceIdentitySession> ServiceIdentitySessionWeakPtr;

      class IServiceIdentitySessionAsyncDelegate;
      typedef boost::shared_ptr<IServiceIdentitySessionAsyncDelegate> IServiceIdentitySessionAsyncDelegatePtr;
      typedef boost::weak_ptr<IServiceIdentitySessionAsyncDelegate> IServiceIdentitySessionAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IServiceIdentitySessionAsyncDelegate> IServiceIdentitySessionAsyncDelegateProxy;

      class ServiceLockboxSession;
      typedef boost::shared_ptr<ServiceLockboxSession> ServiceLockboxSessionPtr;
      typedef boost::weak_ptr<ServiceLockboxSession> ServiceLockboxSessionWeakPtr;

      class IServiceLockboxSessionAsyncDelegate;
      typedef boost::shared_ptr<IServiceLockboxSessionAsyncDelegate> IServiceLockboxSessionAsyncDelegatePtr;
      typedef boost::weak_ptr<IServiceLockboxSessionAsyncDelegate> IServiceLockboxSessionAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IServiceLockboxSessionAsyncDelegate> IServiceLockboxSessionAsyncDelegateProxy;

      class ServiceSaltFetchSignedSaltQuery;
      typedef boost::shared_ptr<ServiceSaltFetchSignedSaltQuery> ServiceSaltFetchSignedSaltQueryPtr;
      typedef boost::weak_ptr<ServiceSaltFetchSignedSaltQuery> ServiceSaltFetchSignedSaltQueryWeakPtr;

      class Stack;
      typedef boost::shared_ptr<Stack> StackPtr;
      typedef boost::weak_ptr<Stack> StackWeakPtr;

      class RSAPrivateKey;
      typedef boost::shared_ptr<RSAPrivateKey> RSAPrivateKeyPtr;
      typedef boost::weak_ptr<RSAPrivateKey> RSAPrivateKeyWeakPtr;

      class RSAPublicKey;
      typedef boost::shared_ptr<RSAPublicKey> RSAPublicKeyPtr;
      typedef boost::weak_ptr<RSAPublicKey> RSAPublicKeyWeakPtr;
    }
  }
}
