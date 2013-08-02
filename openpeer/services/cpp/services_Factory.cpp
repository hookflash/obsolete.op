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

#include <openpeer/services/internal/services_Factory.h>

#include <zsLib/Log.h>

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helper)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Factory
      #pragma mark

      //-----------------------------------------------------------------------
      void Factory::override(FactoryPtr override)
      {
        singleton()->mOverride = override;
      }

      //-----------------------------------------------------------------------
      FactoryPtr &Factory::singleton()
      {
        static FactoryPtr global = Factory::create();
        if (global->mOverride) return global->mOverride;
        return global;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Factory => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      FactoryPtr Factory::create()
      {
        return FactoryPtr(new Factory);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IDNSFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IDNSFactory &IDNSFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      IDNSQueryPtr IDNSFactory::lookupA(
                                        IDNSDelegatePtr delegate,
                                        const char *name
                                        )
      {
        return DNS::lookupA(delegate, name);
      }

      //-----------------------------------------------------------------------
      IDNSQueryPtr IDNSFactory::lookupAAAA(
                                           IDNSDelegatePtr delegate,
                                           const char *name
                                           )
      {
        return DNS::lookupAAAA(delegate, name);
      }

      //-----------------------------------------------------------------------
      IDNSQueryPtr IDNSFactory::lookupAorAAAA(
                                              IDNSDelegatePtr delegate,
                                              const char *name
                                              )
      {
        return DNS::lookupAorAAAA(delegate, name);
      }

      //-----------------------------------------------------------------------
      IDNSQueryPtr IDNSFactory::lookupSRV(
                                          IDNSDelegatePtr delegate,
                                          const char *name,
                                          const char *service,
                                          const char *protocol,
                                          WORD defaultPort,
                                          WORD defaultPriority,
                                          WORD defaultWeight,
                                          SRVLookupTypes lookupType
                                          )
      {
        return DNS::lookupSRV(delegate, name, service, protocol, defaultPort, defaultPriority, defaultWeight, lookupType);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IHTTPFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IHTTPFactory &IHTTPFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      IHTTPQueryPtr IHTTPFactory::get(
                                      IHTTPQueryDelegatePtr delegate,
                                      const char *userAgent,
                                      const char *url,
                                      Duration timeout
                                      )
      {
        return HTTP::get(delegate, userAgent, url, timeout);
      }

      //-----------------------------------------------------------------------
      IHTTPQueryPtr IHTTPFactory::post(
                                       IHTTPQueryDelegatePtr delegate,
                                       const char *userAgent,
                                       const char *url,
                                       const BYTE *postData,
                                       ULONG postDataLengthInBytes,
                                       const char *postDataMimeType,
                                       Duration timeout
                                       )
      {
        return HTTP::post(delegate, userAgent, url, postData, postDataLengthInBytes, postDataMimeType, timeout);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IICESocketFactory &IICESocketFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      ICESocketPtr IICESocketFactory::create(
                                             IMessageQueuePtr queue,
                                             IICESocketDelegatePtr delegate,
                                             const char *turnServer,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             const char *stunServer,
                                             WORD port,
                                             bool firstWORDInAnyPacketWillNotConflictWithTURNChannels
                                             )
      {
        return internal::ICESocket::create(queue, delegate, turnServer, turnServerUsername, turnServerPassword, stunServer, port, firstWORDInAnyPacketWillNotConflictWithTURNChannels);
      }

      //-----------------------------------------------------------------------
      ICESocketPtr IICESocketFactory::create(
                                             IMessageQueuePtr queue,
                                             IICESocketDelegatePtr delegate,
                                             IDNS::SRVResultPtr srvTURNUDP,
                                             IDNS::SRVResultPtr srvTURNTCP,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             IDNS::SRVResultPtr srvSTUN,
                                             WORD port,
                                             bool firstWORDInAnyPacketWillNotConflictWithTURNChannels
                                             )
      {
        return internal::ICESocket::create(queue, delegate, srvTURNUDP, srvTURNTCP, turnServerUsername, turnServerPassword, srvSTUN, port, firstWORDInAnyPacketWillNotConflictWithTURNChannels);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketSessionFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IICESocketSessionFactory &IICESocketSessionFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      ICESocketSessionPtr IICESocketSessionFactory::create(
                                                           IMessageQueuePtr queue,
                                                           IICESocketSessionDelegatePtr delegate,
                                                           ICESocketPtr socket,
                                                           ICEControls control
                                                           )
      {
        return internal::ICESocketSession::create(queue, delegate, socket, control);
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageLayerSecurityChannelFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IMessageLayerSecurityChannelFactory &IMessageLayerSecurityChannelFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannelPtr IMessageLayerSecurityChannelFactory::create(
                                                                                 IMessageLayerSecurityChannelDelegatePtr delegate,
                                                                                 ITransportStreamPtr receiveStreamEncoded,
                                                                                 ITransportStreamPtr receiveStreamDecoded,
                                                                                 ITransportStreamPtr sendStreamDecoded,
                                                                                 ITransportStreamPtr sendStreamEncoded,
                                                                                 const char *contextID
                                                                                 )
      {
        return MessageLayerSecurityChannel::create(delegate, receiveStreamEncoded, receiveStreamDecoded, sendStreamDecoded, sendStreamEncoded, contextID);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPrivateKeyFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IRSAPrivateKeyFactory &IRSAPrivateKeyFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr IRSAPrivateKeyFactory::generate(
                                                       RSAPublicKeyPtr &outPublicKey,
                                                       ULONG keySizeInBites
                                                       )
      {
        return RSAPrivateKey::generate(outPublicKey, keySizeInBites);
      }

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr IRSAPrivateKeyFactory::loadPrivateKey(const SecureByteBlock &buffer)
      {
        return RSAPrivateKey::load(buffer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPublicKeyFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IRSAPublicKeyFactory &IRSAPublicKeyFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RSAPublicKeyPtr IRSAPublicKeyFactory::loadPublicKey(const SecureByteBlock &buffer)
      {
        return RSAPublicKey::load(buffer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IRUDPChannelFactory &IRUDPChannelFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RUDPChannelPtr IRUDPChannelFactory::createForRUDPICESocketSessionIncoming(
                                                                                IMessageQueuePtr queue,
                                                                                IRUDPChannelDelegateForSessionAndListenerPtr master,
                                                                                const IPAddress &remoteIP,
                                                                                WORD incomingChannelNumber,
                                                                                const char *localUserFrag,
                                                                                const char *remoteUserFrag,
                                                                                const char *localPassword,
                                                                                const char *remotePassword,
                                                                                STUNPacketPtr channelOpenPacket,
                                                                                STUNPacketPtr &outResponse
                                                                                )
      {
        return RUDPChannel::createForRUDPICESocketSessionIncoming(queue, master, remoteIP, incomingChannelNumber, localUserFrag, remoteUserFrag, localPassword, remotePassword, channelOpenPacket, outResponse);
      }

      //-----------------------------------------------------------------------
      RUDPChannelPtr IRUDPChannelFactory::createForRUDPICESocketSessionOutgoing(
                                                                                IMessageQueuePtr queue,
                                                                                IRUDPChannelDelegateForSessionAndListenerPtr master,
                                                                                IRUDPChannelDelegatePtr delegate,
                                                                                const IPAddress &remoteIP,
                                                                                WORD incomingChannelNumber,
                                                                                const char *localUserFrag,
                                                                                const char *remoteUserFrag,
                                                                                const char *localPassword,
                                                                                const char *remotePassword,
                                                                                const char *connectionInfo
                                                                                )
      {
        return RUDPChannel::createForRUDPICESocketSessionOutgoing(queue, master, delegate, remoteIP, incomingChannelNumber, localUserFrag, remoteUserFrag, localPassword, remotePassword, connectionInfo);
      }

      //-----------------------------------------------------------------------
      RUDPChannelPtr IRUDPChannelFactory::createForListener(
                                                            IMessageQueuePtr queue,
                                                            IRUDPChannelDelegateForSessionAndListenerPtr master,
                                                            const IPAddress &remoteIP,
                                                            WORD incomingChannelNumber,
                                                            STUNPacketPtr channelOpenPacket,
                                                            STUNPacketPtr &outResponse
                                                            )
      {
        return RUDPChannel::createForListener(queue, master, remoteIP, incomingChannelNumber, channelOpenPacket, outResponse);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelStreamFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IRUDPChannelStreamFactory &IRUDPChannelStreamFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RUDPChannelStreamPtr IRUDPChannelStreamFactory::create(
                                                             IMessageQueuePtr queue,
                                                             IRUDPChannelStreamDelegatePtr delegate,
                                                             QWORD nextSequenceNumberToUseForSending,
                                                             QWORD nextSequenberNumberExpectingToReceive,
                                                             WORD sendingChannelNumber,
                                                             WORD receivingChannelNumber,
                                                             DWORD minimumNegotiatedRTTInMilliseconds
                                                             )
      {
        return RUDPChannelStream::create(queue, delegate, nextSequenceNumberToUseForSending, nextSequenberNumberExpectingToReceive, sendingChannelNumber, receivingChannelNumber, minimumNegotiatedRTTInMilliseconds);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPICESocketFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IRUDPICESocketFactory &IRUDPICESocketFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RUDPICESocketPtr IRUDPICESocketFactory::create(
                                                     IMessageQueuePtr queue,
                                                     IRUDPICESocketDelegatePtr delegate,
                                                     const char *turnServer,
                                                     const char *turnServerUsername,
                                                     const char *turnServerPassword,
                                                     const char *stunServer,
                                                     WORD port
                                                     )
      {
        return RUDPICESocket::create(queue, delegate, turnServer, turnServerUsername, turnServerPassword, stunServer, port);
      }

      //-----------------------------------------------------------------------
      RUDPICESocketPtr IRUDPICESocketFactory::create(
                                                     IMessageQueuePtr queue,
                                                     IRUDPICESocketDelegatePtr delegate,
                                                     IDNS::SRVResultPtr srvTURNUDP,
                                                     IDNS::SRVResultPtr srvTURNTCP,
                                                     const char *turnServerUsername,
                                                     const char *turnServerPassword,
                                                     IDNS::SRVResultPtr srvSTUN,
                                                     WORD port
                                                     )
      {
        return RUDPICESocket::create(queue, delegate, srvTURNUDP, srvTURNTCP, turnServerUsername, turnServerPassword, srvSTUN, port);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPICESocketSessionFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IRUDPICESocketSessionFactory &IRUDPICESocketSessionFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RUDPICESocketSessionPtr IRUDPICESocketSessionFactory::create(
                                                                   IMessageQueuePtr queue,
                                                                   RUDPICESocketPtr parent,
                                                                   IRUDPICESocketSessionDelegatePtr delegate,
                                                                   const CandidateList &remoteCandidates,
                                                                   ICEControls control
                                                                   )
      {
        return RUDPICESocketSession::create(queue, parent, delegate, remoteCandidates, control);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPListenerFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IRUDPListenerFactory &IRUDPListenerFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RUDPListenerPtr IRUDPListenerFactory::create(
                                                   IMessageQueuePtr queue,
                                                   IRUDPListenerDelegatePtr delegate,
                                                   WORD port,
                                                   const char *realm
                                                   )
      {
        return RUDPListener::create(queue, delegate, port, realm);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPListenerFactory
      #pragma mark

      //-------------------------------------------------------------------------
      IRUDPMessagingFactory &IRUDPMessagingFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      RUDPMessagingPtr IRUDPMessagingFactory::acceptChannel(
                                                            IMessageQueuePtr queue,
                                                            IRUDPListenerPtr listener,
                                                            IRUDPMessagingDelegatePtr delegate,
                                                            ULONG maxMessageSizeInBytes
                                                            )
      {
        return RUDPMessaging::acceptChannel(queue, listener, delegate, maxMessageSizeInBytes);
      }

      //-----------------------------------------------------------------------
      RUDPMessagingPtr IRUDPMessagingFactory::acceptChannel(
                                                            IMessageQueuePtr queue,
                                                            IRUDPICESocketSessionPtr session,
                                                            IRUDPMessagingDelegatePtr delegate,
                                                            ULONG maxMessageSizeInBytes
                                                            )
      {
        return RUDPMessaging::acceptChannel(queue, session, delegate, maxMessageSizeInBytes);
      }

      //-----------------------------------------------------------------------
      RUDPMessagingPtr IRUDPMessagingFactory::openChannel(
                                                          IMessageQueuePtr queue,
                                                          IRUDPICESocketSessionPtr session,
                                                          IRUDPMessagingDelegatePtr delegate,
                                                          const char *connectionInfo,
                                                          ULONG maxMessageSizeInBytes
                                                          )
      {
        return RUDPMessaging::openChannel(queue, session, delegate, connectionInfo, maxMessageSizeInBytes);
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISTUNDiscoveryFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ISTUNDiscoveryFactory &ISTUNDiscoveryFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      STUNDiscoveryPtr ISTUNDiscoveryFactory::create(
                                                     IMessageQueuePtr queue,
                                                     ISTUNDiscoveryDelegatePtr delegate,
                                                     IDNS::SRVResultPtr service
                                                     )
      {
        return STUNDiscovery::create(queue, delegate, service);
      }

      //-----------------------------------------------------------------------
      STUNDiscoveryPtr ISTUNDiscoveryFactory::create(
                                                     IMessageQueuePtr queue,
                                                     ISTUNDiscoveryDelegatePtr delegate,
                                                     const char *srvName
                                                     )
      {
        return STUNDiscovery::create(queue, delegate, srvName);
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISTUNRequesterFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ISTUNRequesterFactory &ISTUNRequesterFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      STUNRequesterPtr ISTUNRequesterFactory::create(
                                                     IMessageQueuePtr queue,
                                                     ISTUNRequesterDelegatePtr delegate,
                                                     IPAddress serverIP,
                                                     STUNPacketPtr stun,
                                                     STUNPacket::RFCs usingRFC,
                                                     Duration maxTimeout
                                                     )
      {
        return STUNRequester::create(queue, delegate, serverIP, stun, usingRFC, maxTimeout);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISTUNRequesterFactory
      #pragma mark
      
      //-----------------------------------------------------------------------
      ISTUNRequesterManagerFactory &ISTUNRequesterManagerFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      STUNRequesterManagerPtr ISTUNRequesterManagerFactory::createSTUNRequesterManager()
      {
        return STUNRequesterManager::create();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ITCPMessagingFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ITCPMessagingFactory &ITCPMessagingFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      TCPMessagingPtr ITCPMessagingFactory::accept(
                                                   ITCPMessagingDelegatePtr delegate,
                                                   ITransportStreamPtr receiveStream,
                                                   ITransportStreamPtr sendStream,
                                                   bool framesHaveChannelNumber,
                                                   SocketPtr socket,
                                                   ULONG maxMessageSizeInBytes
                                                   )
      {
        return internal::TCPMessaging::accept(delegate, receiveStream, sendStream, framesHaveChannelNumber, socket, maxMessageSizeInBytes);
      }

      //-----------------------------------------------------------------------
      TCPMessagingPtr ITCPMessagingFactory::connect(
                                                    ITCPMessagingDelegatePtr delegate,
                                                    ITransportStreamPtr receiveStream,
                                                    ITransportStreamPtr sendStream,
                                                    bool framesHaveChannelNumber,
                                                    IPAddress remoteIP,
                                                    ULONG maxMessageSizeInBytes
                                                    )
      {
        return internal::TCPMessaging::connect(delegate, receiveStream, sendStream, framesHaveChannelNumber, remoteIP, maxMessageSizeInBytes);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ITransportStreamFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ITransportStreamFactory &ITransportStreamFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      TransportStreamPtr ITransportStreamFactory::create(
                                                         ITransportStreamWriterDelegatePtr writerDelegate,
                                                         ITransportStreamReaderDelegatePtr readerDelegate
                                                         )
      {
        return internal::TransportStream::create(writerDelegate, readerDelegate);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ITURNSocketFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ITURNSocketFactory &ITURNSocketFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      TURNSocketPtr ITURNSocketFactory::create(
                                         IMessageQueuePtr queue,
                                         ITURNSocketDelegatePtr delegate,
                                         const char *turnServer,
                                         const char *turnServerUsername,
                                         const char *turnServerPassword,
                                         bool useChannelBinding,
                                         WORD limitChannelToRangeStart,
                                         WORD limitChannelRoRangeEnd
                                         )
      {
        return TURNSocket::create(queue, delegate, turnServer, turnServerUsername, turnServerPassword, useChannelBinding, limitChannelToRangeStart, limitChannelRoRangeEnd);
      }

      //-----------------------------------------------------------------------
      TURNSocketPtr ITURNSocketFactory::create(
                                         IMessageQueuePtr queue,
                                         ITURNSocketDelegatePtr delegate,
                                         IDNS::SRVResultPtr srvTURNUDP,
                                         IDNS::SRVResultPtr srvTURNTCP,
                                         const char *turnServerUsername,
                                         const char *turnServerPassword,
                                         bool useChannelBinding,
                                         WORD limitChannelToRangeStart,
                                         WORD limitChannelRoRangeEnd
                                         )
      {
        return TURNSocket::create(queue, delegate, srvTURNUDP, srvTURNTCP, turnServerUsername, turnServerPassword, useChannelBinding, limitChannelToRangeStart, limitChannelRoRangeEnd);
      }
    }
  }
}
