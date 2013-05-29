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
#include <hookflash/core/types.h>
#include <hookflash/services/types.h>

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      using zsLib::Noop;
      using zsLib::BYTE;
      using zsLib::CSTR;
      using zsLib::INT;
      using zsLib::UINT;
      using zsLib::DWORD;
      using zsLib::AutoLock;
      using zsLib::AutoRecursiveLock;
      using zsLib::Lock;
      using zsLib::RecursiveLock;
      using zsLib::Log;
      using zsLib::MessageQueue;
      using zsLib::IMessageQueuePtr;
      using zsLib::MessageQueuePtr;
      using zsLib::MessageQueueAssociator;
      using zsLib::IMessageQueueNotify;
      using zsLib::IMessageQueueMessagePtr;
      using zsLib::IMessageQueueThread;
      using zsLib::MessageQueueThread;
      using zsLib::IMessageQueueThreadPtr;
      using zsLib::MessageQueueThreadPtr;
      using zsLib::Timer;
      using zsLib::TimerPtr;
      using zsLib::ITimerDelegate;
      using zsLib::Seconds;
      using zsLib::Socket;

      using zsLib::XML::AttributePtr;
      using zsLib::XML::Document;
      using zsLib::XML::DocumentPtr;
      using zsLib::XML::Generator;
      using zsLib::XML::GeneratorPtr;

      using stack::AutoRecursiveLockPtr;
      using stack::IBootstrappedNetwork;
      using stack::IBootstrappedNetworkPtr;
      using stack::IBootstrappedNetworkDelegate;
      using stack::ILocation;
      using stack::ILocationPtr;
      using stack::LocationList;
      using stack::LocationListPtr;
      using stack::IMessageIncomingPtr;
      using stack::IMessageMonitor;
      using stack::IMessageMonitorPtr;
      using stack::IPeer;
      using stack::IPeerPtr;
      using stack::IPeerFiles;
      using stack::IPeerFilesPtr;
      using stack::IPeerFilePrivatePtr;
      using stack::IPeerFilePublic;
      using stack::IPeerFilePublicPtr;
      using stack::IPeerSubscription;
      using stack::IPeerSubscriptionPtr;
      using stack::IPeerSubscriptionDelegate;
      using stack::ILocation;
      using stack::IPublication;
      using stack::IPublicationPtr;
      using stack::IPublicationMetaData;
      using stack::IPublicationMetaDataPtr;
      using stack::IPublicationFetcherPtr;
      using stack::IPublicationPublisherPtr;
      using stack::IPublicationPublisherDelegateProxy;
      using stack::IPublicationRepository;
      using stack::IPublicationRepositoryPtr;
      using stack::IPublicationSubscription;
      using stack::IPublicationSubscriptionPtr;
      using stack::IPublicationSubscriptionDelegate;
      using stack::IServiceIdentity;
      using stack::IServiceIdentityPtr;
      using stack::IServiceIdentitySession;
      using stack::IServiceIdentitySessionPtr;
      using stack::IServiceIdentitySessionDelegate;
      using stack::ServiceIdentitySessionList;
      using stack::ServiceIdentitySessionListPtr;
      using stack::IServiceLockbox;
      using stack::IServiceLockboxPtr;
      using stack::IServiceLockboxSession;
      using stack::IServiceLockboxSessionPtr;
      using stack::IServiceLockboxSessionDelegate;

      using services::IICESocket;
      using services::IICESocketPtr;
      using services::IICESocketDelegate;
      using services::IICESocketSubscriptionPtr;
      using services::IICESocketSession;
      using services::IICESocketSessionPtr;
      using services::IHTTP;

      class Account;
      typedef boost::shared_ptr<Account> AccountPtr;
      typedef boost::weak_ptr<Account> AccountWeakPtr;

      interaction IAccountAsyncDelegate;
      typedef boost::shared_ptr<IAccountAsyncDelegate> IAccountAsyncDelegatePtr;
      typedef boost::weak_ptr<IAccountAsyncDelegate> IAccountAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IAccountAsyncDelegate> IAccountAsyncDelegateProxy;

      class Cache;
      typedef boost::shared_ptr<Cache> CachePtr;
      typedef boost::weak_ptr<Cache> CacheWeakPtr;

      class Call;
      typedef boost::shared_ptr<Call> CallPtr;
      typedef boost::weak_ptr<Call> CallWeakPtr;

      interaction ICallAsync;
      typedef boost::shared_ptr<ICallAsync> ICallAsyncPtr;
      typedef boost::weak_ptr<ICallAsync> ICallAsyncWeakPtr;
      typedef zsLib::Proxy<ICallAsync> ICallAsyncProxy;

      class CallTransport;
      typedef boost::shared_ptr<CallTransport> CallTransportPtr;
      typedef boost::weak_ptr<CallTransport> CallTransportWeakPtr;

      interaction ICallTransport;
      typedef boost::shared_ptr<ICallTransport> ICallTransportPtr;
      typedef boost::weak_ptr<ICallTransport> ICallTransportWeakPtr;

      interaction ICallTransportDelegate;
      typedef boost::shared_ptr<ICallTransportDelegate> ICallTransportDelegatePtr;
      typedef boost::weak_ptr<ICallTransportDelegate> ICallTransportDelegateWeakPtr;
      typedef zsLib::Proxy<ICallTransportDelegate> ICallTransportDelegateProxy;

      interaction ICallTransportAsync;
      typedef boost::shared_ptr<ICallTransportAsync> ICallTransportAsyncPtr;
      typedef boost::weak_ptr<ICallTransportAsync> ICallTransportAsyncWeakPtr;
      typedef zsLib::Proxy<ICallTransportAsync> ICallTransportAsyncProxy;

      class Contact;
      typedef boost::shared_ptr<Contact> ContactPtr;
      typedef boost::weak_ptr<Contact> ContactWeakPtr;

      class ContactPeerFilePublicLookup;
      typedef boost::shared_ptr<ContactPeerFilePublicLookup> ContactPeerFilePublicLookupPtr;
      typedef boost::weak_ptr<ContactPeerFilePublicLookup> ContactPeerFilePublicLookupWeakPtr;

      class ConversationThread;
      typedef boost::shared_ptr<ConversationThread> ConversationThreadPtr;
      typedef boost::weak_ptr<ConversationThread> ConversationThreadWeakPtr;

      interaction IConversationThreadAsync;
      typedef boost::shared_ptr<IConversationThreadAsync> IConversationThreadAsyncPtr;
      typedef boost::weak_ptr<IConversationThreadAsync> IConversationThreadAsyncWeakPtr;
      typedef zsLib::Proxy<IConversationThreadAsync> IConversationThreadAsyncProxy;

      interaction IConversationThreadHostSlaveBase;
      typedef boost::shared_ptr<IConversationThreadHostSlaveBase> IConversationThreadHostSlaveBasePtr;
      typedef boost::weak_ptr<IConversationThreadHostSlaveBase> IConversationThreadHostSlaveBaseWeakPtr;

      class ConversationThreadHost;
      typedef boost::shared_ptr<ConversationThreadHost> ConversationThreadHostPtr;
      typedef boost::weak_ptr<ConversationThreadHost> ConversationThreadHostWeakPtr;

      interaction IConversationThreadHostAsync;
      typedef boost::shared_ptr<IConversationThreadHostAsync> IConversationThreadHostAsyncPtr;
      typedef boost::weak_ptr<ConversationThreadHost> IConversationThreadHostAsyncWeakPtr;
      typedef zsLib::Proxy<IConversationThreadHostAsync> IConversationThreadHostAsyncProxy;

      class ConversationThreadSlave;
      typedef boost::shared_ptr<ConversationThreadSlave> ConversationThreadSlavePtr;
      typedef boost::weak_ptr<ConversationThreadSlave> ConversationThreadSlaveWeakPtr;

      interaction IConversationThreadSlaveAsync;
      typedef boost::shared_ptr<IConversationThreadSlaveAsync> IConversationThreadSlaveAsyncPtr;
      typedef boost::weak_ptr<IConversationThreadSlaveAsync> IConversationThreadSlaveAsyncWeakPtr;
      typedef zsLib::Proxy<IConversationThreadSlaveAsync> IConversationThreadSlaveAsyncProxy;

      interaction IConversationThreadDocumentFetcher;
      typedef boost::shared_ptr<IConversationThreadDocumentFetcher> IConversationThreadDocumentFetcherPtr;
      typedef boost::weak_ptr<IConversationThreadDocumentFetcher> IConversationThreadDocumentFetcherWeakPtr;

      interaction IConversationThreadDocumentFetcherDelegate;
      typedef boost::shared_ptr<IConversationThreadDocumentFetcherDelegate> IConversationThreadDocumentFetcherDelegatePtr;
      typedef boost::weak_ptr<IConversationThreadDocumentFetcherDelegate> IConversationThreadDocumentFetcherDelegateWeakPtr;
      typedef zsLib::Proxy<IConversationThreadDocumentFetcherDelegate> IConversationThreadDocumentFetcherDelegateProxy;

      interaction ConversationThreadDocumentFetcher;
      typedef boost::shared_ptr<ConversationThreadDocumentFetcher> ConversationThreadDocumentFetcherPtr;
      typedef boost::weak_ptr<ConversationThreadDocumentFetcher> ConversationThreadDocumentFetcherWeakPtr;

      interaction Factory;
      typedef boost::shared_ptr<Factory> FactoryPtr;
      typedef boost::weak_ptr<Factory> FactoryWeakPtr;

      class Identity;
      typedef boost::shared_ptr<Identity> IdentityPtr;
      typedef boost::weak_ptr<Identity> IdentityWeakPtr;

      class IdentityLookup;
      typedef boost::shared_ptr<IdentityLookup> IdentityLookupPtr;
      typedef boost::weak_ptr<IdentityLookup> IdentityLookupWeakPtr;

      class MediaEngine;
      typedef boost::shared_ptr<MediaEngine> MediaEnginePtr;
      typedef boost::weak_ptr<MediaEngine> MediaEngineWeakPtr;

      interaction IShutdownCheckAgainDelegate;
      typedef boost::shared_ptr<IShutdownCheckAgainDelegate> IShutdownCheckAgainDelegatePtr;
      typedef boost::weak_ptr<IShutdownCheckAgainDelegate> IShutdownCheckAgainDelegateWeakPtr;
      typedef zsLib::Proxy<IShutdownCheckAgainDelegate> IShutdownCheckAgainDelegateProxy;

      class Stack;
      typedef boost::shared_ptr<Stack> StackPtr;
      typedef boost::weak_ptr<Stack> StackWeakPtr;

      class VideoViewPort;
      typedef boost::shared_ptr<VideoViewPort> VideoViewPortPtr;
      typedef boost::weak_ptr<VideoViewPort> VideoViewPortWeakPtr;
    }
  }
}
