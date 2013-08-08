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
#include <zsLib/Proxy.h>
#include <zsLib/ProxySubscriptions.h>
#include <cryptopp/secblock.h>

namespace openpeer
{
  namespace services
  {
    using zsLib::PUID;
    using zsLib::BYTE;
    using zsLib::WORD;
    using zsLib::USHORT;
    using zsLib::UINT;
    using zsLib::ULONG;
    using zsLib::DWORD;
    using zsLib::QWORD;
    using zsLib::String;
    using zsLib::Time;
    using zsLib::Duration;
    using zsLib::Seconds;
    using zsLib::IPAddress;
    using zsLib::Socket;
    using zsLib::SocketPtr;
    using zsLib::ISocketDelegate;
    using zsLib::ISocketDelegatePtr;
    using zsLib::RecursiveLock;
    using zsLib::IMessageQueuePtr;
    using zsLib::Log;
    using zsLib::XML::Element;
    using zsLib::XML::ElementPtr;
    using zsLib::XML::Document;
    using zsLib::XML::DocumentPtr;

    typedef CryptoPP::SecByteBlock SecureByteBlock;
    typedef boost::shared_ptr<SecureByteBlock> SecureByteBlockPtr;
    typedef boost::weak_ptr<SecureByteBlock> SecureByteBlockWeakPtr;

    interaction ICanonicalXML;
    typedef boost::shared_ptr<ICanonicalXML> ICanonicalXMLPtr;
    typedef boost::weak_ptr<ICanonicalXML> ICanonicalXMLWeakPtr;

    interaction IDNS;
    typedef boost::shared_ptr<IDNS> IDNSPtr;
    typedef boost::weak_ptr<IDNS> IDNSWeakPtr;

    interaction IDNSDelegate;
    typedef boost::shared_ptr<IDNSDelegate> IDNSDelegatePtr;
    typedef boost::weak_ptr<IDNSDelegate> IDNSDelegateWeakPtr;
    typedef zsLib::Proxy<IDNSDelegate> IDNSDelegateProxy;

    interaction IDNSQuery;
    typedef boost::shared_ptr<IDNSQuery> IDNSQueryPtr;
    typedef boost::weak_ptr<IDNSQuery> IDNSQueryWeakPtr;

    interaction IICESocket;
    typedef boost::shared_ptr<IICESocket> IICESocketPtr;
    typedef boost::weak_ptr<IICESocket> IICESocketWeakPtr;

    interaction IICESocketDelegate;
    typedef boost::shared_ptr<IICESocketDelegate> IICESocketDelegatePtr;
    typedef boost::weak_ptr<IICESocketDelegate> IICESocketDelegateWeakPtr;
    typedef zsLib::Proxy<IICESocketDelegate> IICESocketDelegateProxy;

    interaction IICESocketSubscription;
    typedef boost::shared_ptr<IICESocketSubscription> IICESocketSubscriptionPtr;
    typedef boost::weak_ptr<IICESocketSubscription> IICESocketSubscriptionWeakPtr;
    typedef zsLib::ProxySubscriptions<IICESocketDelegate, IICESocketSubscription> IICESocketDelegateSubscriptions;

    interaction IICESocketSession;
    typedef boost::shared_ptr<IICESocketSession> IICESocketSessionPtr;
    typedef boost::weak_ptr<IICESocketSession> IICESocketSessionWeakPtr;

    interaction IICESocketSessionDelegate;
    typedef boost::shared_ptr<IICESocketSessionDelegate> IICESocketSessionDelegatePtr;
    typedef boost::weak_ptr<IICESocketSessionDelegate> IICESocketSessionDelegateWeakPtr;
    typedef zsLib::Proxy<IICESocketSessionDelegate> IICESocketSessionDelegateProxy;

    interaction IHTTP;
    typedef boost::shared_ptr<IHTTP> IHTTPPtr;
    typedef boost::weak_ptr<IHTTP> IHTTPWeakPtr;

    interaction IHTTPQuery;
    typedef boost::shared_ptr<IHTTPQuery> IHTTPQueryPtr;
    typedef boost::weak_ptr<IHTTPQuery> IHTTPQueryWeakPtr;

    interaction IHTTPQueryDelegate;
    typedef boost::shared_ptr<IHTTPQueryDelegate> IHTTPQueryDelegatePtr;
    typedef boost::weak_ptr<IHTTPQueryDelegate> IHTTPQueryDelegateWeakPtr;
    typedef zsLib::Proxy<IHTTPQueryDelegate> IHTTPQueryDelegateProxy;

    interaction IMessageLayerSecurityChannel;
    typedef boost::shared_ptr<IMessageLayerSecurityChannel> IMessageLayerSecurityChannelPtr;
    typedef boost::weak_ptr<IMessageLayerSecurityChannel> IMessageLayerSecurityChannelWeakPtr;

    interaction IMessageLayerSecurityChannelDelegate;
    typedef boost::shared_ptr<IMessageLayerSecurityChannelDelegate> IMessageLayerSecurityChannelDelegatePtr;
    typedef boost::weak_ptr<IMessageLayerSecurityChannelDelegate> IMessageLayerSecurityChannelDelegateWeakPtr;
    typedef zsLib::Proxy<IMessageLayerSecurityChannelDelegate> IMessageLayerSecurityChannelDelegateProxy;

    interaction IMessageLayerSecurityChannelSubscription;
    typedef boost::shared_ptr<IMessageLayerSecurityChannelSubscription> IMessageLayerSecurityChannelSubscriptionPtr;
    typedef boost::weak_ptr<IMessageLayerSecurityChannelSubscription> IMessageLayerSecurityChannelSubscriptionWeakPtr;
    typedef zsLib::ProxySubscriptions<IMessageLayerSecurityChannelDelegate, IMessageLayerSecurityChannelSubscription> IMessageLayerSecurityChannelDelegateSubscriptions;

    interaction IRSAPrivateKey;
    typedef boost::shared_ptr<IRSAPrivateKey> IRSAPrivateKeyPtr;
    typedef boost::weak_ptr<IRSAPrivateKey> IRSAPrivateKeyWeakPtr;

    interaction IRSAPublicKey;
    typedef boost::shared_ptr<IRSAPublicKey> IRSAPublicKeyPtr;
    typedef boost::weak_ptr<IRSAPublicKey> IRSAPublicKeyWeakPtr;
    
    interaction IRUDPListener;
    typedef boost::shared_ptr<IRUDPListener> IRUDPListenerPtr;
    typedef boost::weak_ptr<IRUDPListener> IRUDPListenerWeakPtr;

    interaction IRUDPListenerDelegate;
    typedef boost::shared_ptr<IRUDPListenerDelegate> IRUDPListenerDelegatePtr;
    typedef boost::weak_ptr<IRUDPListenerDelegate> IRUDPListenerDelegateWeakPtr;
    typedef zsLib::Proxy<IRUDPListenerDelegate> IRUDPListenerDelegateProxy;

    interaction IRUDPMessaging;
    typedef boost::shared_ptr<IRUDPMessaging> IRUDPMessagingPtr;
    typedef boost::weak_ptr<IRUDPMessaging> IRUDPMessagingWeakPtr;

    interaction IRUDPMessagingDelegate;
    typedef boost::shared_ptr<IRUDPMessagingDelegate> IRUDPMessagingDelegatePtr;
    typedef boost::weak_ptr<IRUDPMessagingDelegate> IRUDPMessagingDelegateWeakPtr;
    typedef zsLib::Proxy<IRUDPMessagingDelegate> IRUDPMessagingDelegateProxy;

    struct RUDPPacket;
    typedef boost::shared_ptr<RUDPPacket> RUDPPacketPtr;
    typedef boost::weak_ptr<RUDPPacket> RUDPPacketWeakPtr;

    interaction IRUDPChannel;
    typedef boost::shared_ptr<IRUDPChannel> IRUDPChannelPtr;
    typedef boost::weak_ptr<IRUDPChannel> IRUDPChannelWeakPtr;

    interaction IRUDPChannelDelegate;
    typedef boost::shared_ptr<IRUDPChannelDelegate> IRUDPChannelDelegatePtr;
    typedef boost::weak_ptr<IRUDPChannelDelegate> IRUDPChannelDelegateWeakPtr;
    typedef zsLib::Proxy<IRUDPChannelDelegate> IRUDPChannelDelegateProxy;

    interaction IRUDPICESocket;
    typedef boost::shared_ptr<IRUDPICESocket> IRUDPICESocketPtr;
    typedef boost::weak_ptr<IRUDPICESocket> IRUDPICESocketWeakPtr;

    interaction IRUDPICESocketSubscription;
    typedef boost::shared_ptr<IRUDPICESocketSubscription> IRUDPICESocketSubscriptionPtr;
    typedef boost::weak_ptr<IRUDPICESocketSubscription> IRUDPICESocketSubscriptionWeakPtr;

    interaction IRUDPICESocketDelegate;
    typedef boost::shared_ptr<IRUDPICESocketDelegate> IRUDPICESocketDelegatePtr;
    typedef boost::weak_ptr<IRUDPICESocketDelegate> IRUDPICESocketDelegateWeakPtr;
    typedef zsLib::Proxy<IRUDPICESocketDelegate> IRUDPICESocketDelegateProxy;
    typedef zsLib::ProxySubscriptions<IRUDPICESocketDelegate, IRUDPICESocketSubscription> IRUDPICESocketDelegateSubscriptions;

    interaction IRUDPICESocketSession;
    typedef boost::shared_ptr<IRUDPICESocketSession> IRUDPICESocketSessionPtr;
    typedef boost::weak_ptr<IRUDPICESocketSession> IRUDPICESocketSessionWeakPtr;

    interaction IRUDPICESocketSessionDelegate;
    typedef boost::shared_ptr<IRUDPICESocketSessionDelegate> IRUDPICESocketSessionDelegatePtr;
    typedef boost::weak_ptr<IRUDPICESocketSessionDelegate> IRUDPICESocketSessionDelegateWeakPtr;
    typedef zsLib::Proxy<IRUDPICESocketSessionDelegate> IRUDPICESocketSessionDelegateProxy;

    interaction ISTUNDiscovery;
    typedef boost::shared_ptr<ISTUNDiscovery> ISTUNDiscoveryPtr;
    typedef boost::weak_ptr<ISTUNDiscovery> ISTUNDiscoveryWeakPtr;

    interaction ISTUNDiscoveryDelegate;
    typedef boost::shared_ptr<ISTUNDiscoveryDelegate> ISTUNDiscoveryDelegatePtr;
    typedef boost::weak_ptr<ISTUNDiscoveryDelegate> ISTUNDiscoveryDelegateWeakPtr;
    typedef zsLib::Proxy<ISTUNDiscoveryDelegate> ISTUNDiscoveryDelegateProxy;

    interaction ISTUNRequester;
    typedef boost::shared_ptr<ISTUNRequester> ISTUNRequesterPtr;
    typedef boost::weak_ptr<ISTUNRequester> ISTUNRequesterWeakPtr;

    interaction ISTUNRequesterDelegate;
    typedef boost::shared_ptr<ISTUNRequesterDelegate> ISTUNRequesterDelegatePtr;
    typedef boost::weak_ptr<ISTUNRequesterDelegate> ISTUNRequesterDelegateWeakPtr;
    typedef zsLib::Proxy<ISTUNRequesterDelegate> ISTUNRequesterDelegateProxy;

    struct STUNPacket;
    typedef boost::shared_ptr<STUNPacket> STUNPacketPtr;
    typedef boost::weak_ptr<STUNPacket> STUNPacketWeakPtr;

    interaction ITCPMessaging;
    typedef boost::shared_ptr<ITCPMessaging> ITCPMessagingPtr;
    typedef boost::weak_ptr<ITCPMessaging> ITCPMessagingWeakPtr;

    interaction ITCPMessagingDelegate;
    typedef boost::shared_ptr<ITCPMessagingDelegate> ITCPMessagingDelegatePtr;
    typedef boost::weak_ptr<ITCPMessagingDelegate> ITCPMessagingDelegateWeakPtr;

    interaction ITCPMessagingSubscription;
    typedef boost::shared_ptr<ITCPMessagingSubscription> ITCPMessagingSubscriptionPtr;
    typedef boost::weak_ptr<ITCPMessagingSubscription> ITCPMessagingSubscriptionWeakPtr;
    typedef zsLib::ProxySubscriptions<ITCPMessagingDelegate, ITCPMessagingSubscription> ITCPMessagingDelegateSubscriptions;

    interaction ITransportStream;
    typedef boost::shared_ptr<ITransportStream> ITransportStreamPtr;
    typedef boost::weak_ptr<ITransportStream> ITransportStreamWeakPtr;

    interaction ITransportStreamWriter;
    typedef boost::shared_ptr<ITransportStreamWriter> ITransportStreamWriterPtr;
    typedef boost::weak_ptr<ITransportStreamWriter> ITransportStreamWriterWeakPtr;

    interaction ITransportStreamWriterDelegate;
    typedef boost::shared_ptr<ITransportStreamWriterDelegate> ITransportStreamWriterDelegatePtr;
    typedef boost::weak_ptr<ITransportStreamWriterDelegate> ITransportStreamWriterDelegateWeakPtr;

    interaction ITransportStreamWriterSubscription;
    typedef boost::shared_ptr<ITransportStreamWriterSubscription> ITransportStreamWriterSubscriptionPtr;
    typedef boost::weak_ptr<ITransportStreamWriterSubscription> ITransportStreamWriterSubscriptionWeakPtr;
    typedef zsLib::ProxySubscriptions<ITransportStreamWriterDelegate, ITransportStreamWriterSubscription> ITransportStreamWriterDelegateSubscriptions;

    interaction ITransportStreamReader;
    typedef boost::shared_ptr<ITransportStreamReader> ITransportStreamReaderPtr;
    typedef boost::weak_ptr<ITransportStreamReader> ITransportStreamReaderWeakPtr;

    interaction ITransportStreamReaderDelegate;
    typedef boost::shared_ptr<ITransportStreamReaderDelegate> ITransportStreamReaderDelegatePtr;
    typedef boost::weak_ptr<ITransportStreamReaderDelegate> ITransportStreamReaderDelegateWeakPtr;

    interaction ITransportStreamReaderSubscription;
    typedef boost::shared_ptr<ITransportStreamReaderSubscription> ITransportStreamReaderSubscriptionPtr;
    typedef boost::weak_ptr<ITransportStreamReaderSubscription> ITransportStreamReaderSubscriptionWeakPtr;
    typedef zsLib::ProxySubscriptions<ITransportStreamReaderDelegate, ITransportStreamReaderSubscription> ITransportStreamReaderDelegateSubscriptions;

    interaction ITURNSocket;
    typedef boost::shared_ptr<ITURNSocket> ITURNSocketPtr;
    typedef boost::weak_ptr<ITURNSocket> ITURNSocketWeakPtr;

    interaction ITURNSocketDelegate;
    typedef boost::shared_ptr<ITURNSocketDelegate> ITURNSocketDelegatePtr;
    typedef boost::weak_ptr<ITURNSocketDelegate> ITURNSocketDelegateWeakPtr;
    typedef zsLib::Proxy<ITURNSocketDelegate> ITURNSocketDelegateProxy;

    interaction IWakeDelegate;
    typedef boost::shared_ptr<IWakeDelegate> IWakeDelegatePtr;
    typedef boost::weak_ptr<IWakeDelegate> IWakeDelegateWeakPtr;
    typedef zsLib::Proxy<IWakeDelegate> IWakeDelegateProxy;
  }
}
