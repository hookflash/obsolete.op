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

#include <hookflash/services/types.h>

namespace CryptoPP
{
  class AutoSeededRandomPool;
  class ByteQueue;
  class HexEncoder;
  class HexDecoder;
};

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      using zsLib::Noop;
      using zsLib::AutoRecursiveLock;
      using zsLib::CSTR;
      using zsLib::PTRNUMBER;
      using zsLib::Milliseconds;
      using zsLib::Timer;
      using zsLib::TimerPtr;
      using zsLib::ITimerDelegate;
      using zsLib::ITimerDelegatePtr;
      using zsLib::ISocket;
      using zsLib::ISocketPtr;
      using zsLib::ISocketDelegate;
      using zsLib::Socket;
      using zsLib::SocketPtr;
      using zsLib::SocketWeakPtr;
      using zsLib::ThreadPtr;
      using zsLib::Event;
      using zsLib::EventPtr;
      using zsLib::MessageQueueAssociator;
      using zsLib::Subsystem;
      using zsLib::LogPtr;
      using zsLib::ILogDelegate;
      using zsLib::MessageQueueThread;
      using zsLib::MessageQueueThreadPtr;
      using zsLib::XML::AttributePtr;
      using zsLib::XML::Document;
      using zsLib::XML::DocumentPtr;
      using zsLib::XML::Node;
      using zsLib::XML::NodePtr;
      using zsLib::XML::Text;
      using zsLib::XML::TextPtr;
      using zsLib::XML::CommentPtr;
      using zsLib::XML::UnknownPtr;
      using zsLib::XML::DeclarationPtr;
      using zsLib::XML::Generator;
      using zsLib::XML::GeneratorPtr;
      using zsLib::XML::WalkSink;

      using CryptoPP::AutoSeededRandomPool;
      using CryptoPP::ByteQueue;
      using CryptoPP::HexEncoder;
      using CryptoPP::HexDecoder;

      interaction DNS;
      typedef boost::shared_ptr<DNS> DNSPtr;
      typedef boost::weak_ptr<DNS> DNSWeakPtr;

      class DNSMonitor;
      typedef boost::shared_ptr<DNSMonitor> DNSMonitorPtr;
      typedef boost::weak_ptr<DNSMonitor> DNSMonitorWeakPtr;

      class DNSQuery;
      typedef boost::shared_ptr<DNSQuery> DNSQueryPtr;
      typedef boost::weak_ptr<DNSQuery> DNSQueryWeakPtr;

      class Factory;
      typedef boost::shared_ptr<Factory> FactoryPtr;
      typedef boost::weak_ptr<Factory> FactoryWeakPtr;

      class ICESocket;
      typedef boost::shared_ptr<ICESocket> ICESocketPtr;
      typedef boost::weak_ptr<ICESocket> ICESocketWeakPtr;

      interaction IICESocketForICESocketSession;
      typedef boost::shared_ptr<IICESocketForICESocketSession> IICESocketForICESocketSessionPtr;
      typedef boost::weak_ptr<IICESocketForICESocketSession> IICESocketForICESocketSessionWeakPtr;
      typedef zsLib::Proxy<IICESocketForICESocketSession> IICESocketForICESocketSessionProxy;

      class ICESocketSession;
      typedef boost::shared_ptr<ICESocketSession> ICESocketSessionPtr;
      typedef boost::weak_ptr<ICESocketSession> ICESocketSessionWeakPtr;

      interaction IICESocketSessionAsyncDelegate;
      typedef boost::shared_ptr<IICESocketSessionAsyncDelegate> IICESocketSessionAsyncDelegatePtr;
      typedef boost::weak_ptr<IICESocketSessionAsyncDelegate> IICESocketSessionAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IICESocketSessionAsyncDelegate> IICESocketSessionAsyncDelegateProxy;

      interaction HTTP;
      typedef boost::shared_ptr<HTTP> HTTPPtr;
      typedef boost::weak_ptr<HTTP> HTTPWeakPtr;

      class RUDPChannel;
      typedef boost::shared_ptr<RUDPChannel> RUDPChannelPtr;
      typedef boost::weak_ptr<RUDPChannel> RUDPChannelWeakPtr;

      interaction IRUDPChannelDelegateForSessionAndListener;
      typedef boost::shared_ptr<IRUDPChannelDelegateForSessionAndListener> IRUDPChannelDelegateForSessionAndListenerPtr;
      typedef boost::weak_ptr<IRUDPChannelDelegateForSessionAndListener> IRUDPChannelDelegateForSessionAndListenerWeakPtr;
      typedef zsLib::Proxy<IRUDPChannelDelegateForSessionAndListener> IRUDPChannelDelegateForSessionAndListenerProxy;

      interaction IRUDPChannelAsyncDelegate;
      typedef boost::shared_ptr<IRUDPChannelAsyncDelegate> IRUDPChannelAsyncDelegatePtr;
      typedef boost::weak_ptr<IRUDPChannelAsyncDelegate> IRUDPChannelAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<IRUDPChannelAsyncDelegate> IRUDPChannelAsyncDelegateProxy;

      interaction IRUDPChannelStream;
      typedef boost::shared_ptr<IRUDPChannelStream> IRUDPChannelStreamPtr;
      typedef boost::weak_ptr<IRUDPChannelStream> IRUDPChannelStreamWeakPtr;

      interaction IRUDPChannelStreamDelegate;
      typedef boost::shared_ptr<IRUDPChannelStreamDelegate> IRUDPChannelStreamDelegatePtr;
      typedef boost::weak_ptr<IRUDPChannelStreamDelegate> IRUDPChannelStreamDelegateWeakPtr;
      typedef zsLib::Proxy<IRUDPChannelStreamDelegate> IRUDPChannelStreamDelegateProxy;

      interaction IRUDPChannelStreamAsync;
      typedef boost::shared_ptr<IRUDPChannelStreamAsync> IRUDPChannelStreamAsyncPtr;
      typedef boost::weak_ptr<IRUDPChannelStreamAsync> IRUDPChannelStreamAsyncWeakPtr;
      typedef zsLib::Proxy<IRUDPChannelStreamAsync> IRUDPChannelStreamAsyncProxy;

      class RUDPChannelStream;
      typedef boost::shared_ptr<RUDPChannelStream> RUDPChannelStreamPtr;
      typedef boost::weak_ptr<RUDPChannelStream> RUDPChannelStreamWeakPtr;

      class RUDPICESocket;
      typedef boost::shared_ptr<RUDPICESocket> RUDPICESocketPtr;
      typedef boost::weak_ptr<RUDPICESocket> RUDPICESocketWeakPtr;

      interaction IRUDPICESocketForRUDPICESocketSession;
      typedef boost::shared_ptr<IRUDPICESocketForRUDPICESocketSession> IRUDPICESocketForRUDPICESocketSessionPtr;
      typedef boost::weak_ptr<IRUDPICESocketForRUDPICESocketSession> IRUDPICESocketForRUDPICESocketSessionWeakPtr;
      typedef zsLib::Proxy<IRUDPICESocketForRUDPICESocketSession> IRUDPICESocketForRUDPICESocketSessionProxy;

      class RUDPICESocketSession;
      typedef boost::shared_ptr<RUDPICESocketSession> RUDPICESocketSessionPtr;
      typedef boost::weak_ptr<RUDPICESocketSession> RUDPICESocketSessionWeakPtr;

      class RUDPListener;
      typedef boost::shared_ptr<RUDPListener> RUDPListenerPtr;
      typedef boost::weak_ptr<RUDPListener> RUDPListenerWeakPtr;

      class RUDPMessaging;
      typedef boost::shared_ptr<RUDPMessaging> RUDPMessagingPtr;
      typedef boost::weak_ptr<RUDPMessaging> RUDPMessagingWeakPtr;

      class STUNDiscovery;
      typedef boost::shared_ptr<STUNDiscovery> STUNDiscoveryPtr;
      typedef boost::weak_ptr<STUNDiscovery> STUNDiscoveryWeakPtr;

      class STUNRequester;
      typedef boost::shared_ptr<STUNRequester> STUNRequesterPtr;
      typedef boost::weak_ptr<STUNRequester> STUNRequesterWeakPtr;

      class STUNRequesterManager;
      typedef boost::shared_ptr<STUNRequesterManager> STUNRequesterManagerPtr;
      typedef boost::weak_ptr<STUNRequesterManager> STUNRequesterManagerWeakPtr;

      interaction ITURNSocketAsyncDelegate;
      typedef boost::shared_ptr<ITURNSocketAsyncDelegate> ITURNSocketAsyncDelegatePtr;
      typedef boost::weak_ptr<ITURNSocketAsyncDelegate> ITURNSocketAsyncDelegateWeakPtr;
      typedef zsLib::Proxy<ITURNSocketAsyncDelegate> ITURNSocketAsyncDelegateProxy;

      class TURNSocket;
      typedef boost::shared_ptr<TURNSocket> TURNSocketPtr;
      typedef boost::weak_ptr<TURNSocket> TURNSocketWeakPtr;
    }
  }
}
