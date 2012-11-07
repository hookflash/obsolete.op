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

#include <hookflash/services/internal/services_ICESocket.h>
#include <hookflash/services/internal/services_ICESocketSession.h>
#include <hookflash/services/internal/services_TURNSocket.h>
#include <hookflash/services/ISTUNRequesterManager.h>
#include <hookflash/services/IHelper.h>
#include <zsLib/Exception.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Numeric.h>
#include <zsLib/Stringize.h>
#include <zsLib/XML.h>
#include <zsLib/zsTypes.h>

#include <cryptopp/osrng.h>

#ifdef _WIN32
#include <Iphlpapi.h>
#endif //_WIN32

#ifndef _WIN32
#include <sys/types.h>
#include <ifaddrs.h>
#endif //_WIN32

#define HOOKFLASH_SERVICES_ICESOCKET_RECYCLE_BUFFER_SIZE  (1 << (sizeof(zsLib::WORD)*8))
#define HOOKFLASH_SERVICES_ICESOCKET_MAX_RECYLCE_BUFFERS  4

#define HOOKFLASH_SERVICES_ICESOCKET_MINIMUM_TURN_KEEP_ALIVE_TIME_IN_SECONDS  HOOKFLASH_SERVICES_IICESOCKET_DEFAULT_HOW_LONG_CANDIDATES_MUST_REMAIN_VALID_IN_SECONDS

#define HOOKFLASH_SERVICES_MAX_REBIND_ATTEMPT_DURATION_IN_SECONDS (10)


namespace hookflash { namespace services { ZS_DECLARE_SUBSYSTEM(hookflash_services) } }


namespace hookflash
{
  namespace services
  {
    using zsLib::Stringize;

    typedef zsLib::String String;
    typedef zsLib::IPAddress IPAddress;
    typedef zsLib::ISocket ISocket;
    typedef zsLib::Socket Socket;
    typedef zsLib::AutoRecursiveLock AutoRecursiveLock;

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      static bool compareLocalIPs(const IPAddress &ip1, const IPAddress &ip2)
      {
        if (ip1.isIPv4()) {
          if (ip2.isIPv4()) {
            return ip1 < ip2;
          }
          return true;
        }

        if (ip2.isIPv4())
          return false;

        return ip1 < ip2;
      }

      //-----------------------------------------------------------------------
      static bool containsIP(const std::list<IPAddress> &ipList, const IPAddress &ip)
      {
        typedef std::list<IPAddress> IPAddressList;

        for (IPAddressList::const_iterator iter = ipList.begin(); iter != ipList.end(); ++iter) {
          if ((*iter) == ip)
            return true;
        }
        return false;
      }

      //-----------------------------------------------------------------------
      static IICESocket::Types normalize(IICESocket::Types transport)
      {
        if (transport == ICESocket::Type_Relayed)
          return ICESocket::Type_Relayed;
        return ICESocket::Type_Local;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      ICESocket::ICESocket(
                           zsLib::IMessageQueuePtr queue,
                           IICESocketDelegatePtr delegate,
                           IDNS::SRVResultPtr srvTURNUDP,
                           IDNS::SRVResultPtr srvTURNTCP,
                           const char *turnServer,
                           const char *turnUsername,
                           const char *turnPassword,
                           bool firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                           IDNS::SRVResultPtr srvSTUN,
                           const char *stunServer,
                           zsLib::WORD port
                           ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(ICESocketState_Pending),
        mBindPort(port),
        mMaxRebindAttemptDuration(Seconds(HOOKFLASH_SERVICES_MAX_REBIND_ATTEMPT_DURATION_IN_SECONDS)),
        mMonitoringWriteReady(true),
        mTURNSRVUDPResult(srvTURNUDP),
        mTURNSRVTCPResult(srvTURNTCP),
        mTURNServer(turnServer ? turnServer : ""),
        mTURNUsername(turnUsername ? turnUsername : ""),
        mTURNPassword(turnPassword ? turnPassword : ""),
        mFirstWORDInAnyPacketWillNotConflictWithTURNChannels(firstWORDInAnyPacketWillNotConflictWithTURNChannels),
        mTURNLastUsed(zsLib::now()),
        mTURNShutdownIfNotUsedBy(zsLib::Seconds(HOOKFLASH_SERVICES_ICESOCKET_MINIMUM_TURN_KEEP_ALIVE_TIME_IN_SECONDS)),
        mSTUNSRVResult(srvSTUN),
        mSTUNServer(stunServer ? stunServer : ""),
        mUsernameFrag(IHelper::randomString(20)),
        mPassword(IHelper::randomString(20))
      {
        mDelegates[0] = IICESocketDelegateProxy::createWeak(queue, delegate);
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      ICESocket::~ICESocket()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      ICESocketPtr ICESocket::create(
                                     zsLib::IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     const char *turnServer,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     const char *stunServer,
                                     zsLib::WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels
                                     )
      {
        ICESocketPtr pThis(new ICESocket(
                                         queue,
                                         delegate,
                                         IDNS::SRVResultPtr(),
                                         IDNS::SRVResultPtr(),
                                         turnServer,
                                         turnServerUsername,
                                         turnServerPassword,
                                         firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                         IDNS::SRVResultPtr(),
                                         stunServer,
                                         port));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ICESocketPtr ICESocket::create(
                                     zsLib::IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     IDNS::SRVResultPtr srvTURNUDP,
                                     IDNS::SRVResultPtr srvTURNTCP,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     IDNS::SRVResultPtr srvSTUN,
                                     zsLib::WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels
                                     )
      {
        ICESocketPtr pThis(new ICESocket(
                                         queue,
                                         delegate,
                                         srvTURNUDP,
                                         srvTURNTCP,
                                         NULL,
                                         turnServerUsername,
                                         turnServerPassword,
                                         firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                         srvSTUN,
                                         NULL,
                                         port));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void ICESocket::init()
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DETAIL(log("init"))

        // before we do anything we must be sure we bind the UDP socket
        if (!bindUDP())
          return;

        step();
      }

      IICESocket::ICESocketStates ICESocket::getState() const
      {
        AutoRecursiveLock lock(mLock);
        return mCurrentState;
      }

      IICESocketSubscriptionPtr ICESocket::subscribe(IICESocketDelegatePtr delegate)
      {
        ZS_LOG_DETAIL(log("subscribing to socket state"))

        AutoRecursiveLock lock(mLock);
        delegate = IICESocketDelegateProxy::createWeak(getAssociatedMessageQueue(), delegate);

        if (ICESocketState_Pending != mCurrentState) {
          try {
            // notify of the state changed since this subcription would not have been notified of previous state changes...
            delegate->onICESocketStateChanged(mThisWeak.lock(), mCurrentState);
          } catch(IICESocketDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        SubscriptionPtr subscription = Subscription::create(mThisWeak.lock());
        ZS_LOG_DETAIL(log("subscription created") + ", subscription ID=" + zsLib::Stringize<PUID>(subscription->mID).string())

        if (isShutdown()) return subscription;

        // remember the subscriber
        mDelegates[subscription->mID] = delegate;
        return subscription;
      }

      //-----------------------------------------------------------------------
      ICESocketPtr ICESocket::convert(IICESocketPtr socket)
      {
        return boost::dynamic_pointer_cast<ICESocket>(socket);
      }

      //-----------------------------------------------------------------------
      void ICESocket::shutdown()
      {
        ZS_LOG_DETAIL(log("shutdown requested"))

        AutoRecursiveLock lock(mLock);
        cancel();
      }

      //-----------------------------------------------------------------------
      void ICESocket::wakeup(zsLib::Duration minimumTimeCandidatesMustRemainValidWhileNotUsed)
      {
        AutoRecursiveLock lock(mLock);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("received request to wake up while ICE socket is shutting down or shutdown"))
          return;
        }

        mTURNLastUsed = zsLib::now();
        mTURNShutdownIfNotUsedBy = (mTURNShutdownIfNotUsedBy > minimumTimeCandidatesMustRemainValidWhileNotUsed ? mTURNShutdownIfNotUsedBy : minimumTimeCandidatesMustRemainValidWhileNotUsed);

        if (ICESocketState_Ready == mCurrentState) {
          // we are already ready, extend the lifetime of how long the socket must remain ready...
          step();
          return;
        }

        // this socket went asleep so must rewake the socket to ensure that all IPs are still valid
        clearReflectedCandidates();
        clearRelayedCandidates();

        if (mTURNSocket) {
          if ((ITURNSocket::TURNSocketState_ShuttingDown == mTURNSocket->getState()) ||
              (ITURNSocket::TURNSocketState_Shutdown == mTURNSocket->getState())) {

            ZS_LOG_DEBUG(log("clearing out old TURN server") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())

            // this TURN socket is toast, so clear it out so a new one can be created later...
            mTURNSocket->shutdown();
            mTURNSocket.reset();
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      void ICESocket::setFoundation(IICESocketPtr foundationSocket)
      {
        AutoRecursiveLock lock(mLock);
        mFoundation = convert(foundationSocket);
      }

      //-----------------------------------------------------------------------
      void ICESocket::getLocalCandidates(CandidateList &outCandidates)
      {
        AutoRecursiveLock lock(mLock);
        makeCandidates();

        outCandidates.clear();
        outCandidates = mLocalCandidates;
      }

      //-----------------------------------------------------------------------
      IICESocketSessionPtr ICESocket::createSessionFromRemoteCandidates(
                                                                        IICESocketSessionDelegatePtr delegate,
                                                                        const CandidateList &remoteCandidates,
                                                                        ICEControls control
                                                                        )
      {
        AutoRecursiveLock lock(mLock);

        ICESocketSessionPtr session = ICESocketSession::create(getAssociatedMessageQueue(), delegate, mThisWeak.lock(), control);
        ZS_THROW_BAD_STATE_IF(!session)

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Basic, log("create session called after socket is being shutdown"))
          // immediately close the session since we are shutting down
          session->close();
          return session;
        }

        // remember the session for later
        mSessions[session->getID()] = session;
        session->updateRemoteCandidates(remoteCandidates);
        return session;
      }

      //-----------------------------------------------------------------------
      void ICESocket::monitorWriteReadyOnAllSessions(bool monitor)
      {
        AutoRecursiveLock lock(mLock);

        mMonitoringWriteReady = monitor;

        if (!mUDPSocket) {
          ZS_LOG_WARNING(Detail, log("cannot start or stop monitoring on ICE socket as UDP socket is not created"))
          return;
        }

        if (monitor) {
          mUDPSocket->monitor(ISocket::Monitor::All);
        } else {
          mUDPSocket->monitor((ISocket::Monitor::Options)(ISocket::Monitor::Read | ISocket::Monitor::Exception));
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onReadReady(zsLib::ISocketPtr socket)
      {
        boost::shared_array<BYTE> buffer;
        IPAddress source;
        ULONG bytesRead = 0;
        AutoRecycleBuffer recycle(*this, buffer);

        // scope: we are going to read the data while within the local but process it outside the lock
        {
          AutoRecursiveLock lock(mLock);
          if (!mUDPSocket) {
            ZS_LOG_WARNING(Detail, log("UDP socket is not ready"))
            return;
          }

          try {
            bool wouldBlock = false;

            getBuffer(buffer);

            bytesRead = mUDPSocket->receiveFrom(source, buffer.get(), HOOKFLASH_SERVICES_ICESOCKET_RECYCLE_BUFFER_SIZE, &wouldBlock);
            if (0 == bytesRead) return;

            ZS_LOG_TRACE(log("packet received") + ", ip=" + source.string())

          } catch(ISocket::Exceptions::Unspecified &error) {
            ZS_LOG_ERROR(Detail, log("receiveFrom error") + ", error=" + Stringize<int>(error.getErrorCode()).string())
            cancel();
            return;
          }
        }

        // this method cannot be called within the scope of a lock because it
        // calls a delegate synchronously
        internalReceivedData(IICESocket::Type_Local, source, buffer.get(), bytesRead);
      }

      //-----------------------------------------------------------------------
      void ICESocket::onWriteReady(zsLib::ISocketPtr socket)
      {
        ZS_LOG_TRACE(log("write ready"))
        AutoRecursiveLock lock(mLock);
        for(ICESocketSessionMap::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter) {
          (*iter).second->notifyLocalWriteReady();
        }

        if (mTURNSocket) {
          ZS_LOG_TRACE(log("notifying TURN socket of write ready") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())
          mTURNSocket->notifyWriteReady();
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onException(zsLib::ISocketPtr socket)
      {
        ZS_LOG_DETAIL(log("exception"))
        AutoRecursiveLock lock(mLock);
        if (socket != mUDPSocket) {
          ZS_LOG_WARNING(Detail, log("notified of exception on socket which is not the bound socket"))
          return;
        }

        mUDPSocket->close();
        mUDPSocket.reset();

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("ICE socket is now closed"))
          return;
        }

        mRebindAttemptStartTime = zsLib::now();

        ZS_LOG_WARNING(Detail, log("attempting to rebind socket immediately") + ", start time=" + Stringize<Time>(mRebindAttemptStartTime).string())

        if (mTURNSocket) {
          if (!mTURNSocket->isRelayingUDP()) {
            ZS_LOG_WARNING(Detail, log("TURN is not relaying UDP so it must be closed"))
            mTURNSocket->shutdown();
            mTURNSocket.reset();

            for (ICESocketSessionMap::iterator iterSession = mSessions.begin(); iterSession != mSessions.end(); )
            {
              ICESocketSessionMap::iterator current = iterSession;
              ++iterSession;

              IICESocketSessionForICESocketPtr &session = (*current).second;
              ZS_LOG_WARNING(Detail, log("forcing socket session to timeout") + ", session ID=" + Stringize<PUID>(session->getID()).string())
              session->timeout();
            }
            mSessions.clear();
            mRoutes.clear();

            // force outselves into the sleep state prematurely
            ZS_LOG_WARNING(Detail, log("forcing abnormal sleep because of backgrounding with TCP TURN"))
            setState(ICESocketState_GoingToSleep);
            setState(ICESocketState_Sleeping);
          }
        }

        // try the bind...
        bindUDP();
        step();
      }

      //-----------------------------------------------------------------------
      void ICESocket::onTURNSocketStateChanged(
                                               ITURNSocketPtr socket,
                                               TURNSocketStates state
                                               )
      {
        AutoRecursiveLock lock(mLock);
        if (socket != mTURNSocket) return;
        ZS_LOG_DETAIL(log("notified that TURN state has changed") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())
        step();
      }

      //-----------------------------------------------------------------------
      void ICESocket::handleTURNSocketReceivedPacket(
                                                     ITURNSocketPtr socket,
                                                     zsLib::IPAddress source,
                                                     const zsLib::BYTE *packet,
                                                     zsLib::ULONG packetLengthInBytes
                                                     )
      {
        // WARNING: This method cannot be called within a lock as it calls delegates synchronously.
        internalReceivedData(IICESocket::Type_Relayed, source, packet, packetLengthInBytes);
      }

      //-----------------------------------------------------------------------
      bool ICESocket::notifyTURNSocketSendPacket(
                                                 ITURNSocketPtr socket,
                                                 zsLib::IPAddress destination,
                                                 const zsLib::BYTE *packet,
                                                 zsLib::ULONG packetLengthInBytes
                                                 )
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_TRACE(log("sending packet for TURN") + ", TURN socket ID=" + Stringize<PUID>(socket->getID()).string() + ", destination=" + destination.string() + ", length=" + Stringize<ULONG>(packetLengthInBytes).string())

        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("unable to send data on behalf of TURN as ICE socket is shutdown") + ", TURN socket ID=" + Stringize<PUID>(socket->getID()).string())
          return false;
        }
        if (socket != mTURNSocket) {
          ZS_LOG_WARNING(Debug, log("unable to send data on behalf of TURN as TURN socket does not match current TURN socket (TURN reconnect reattempt?)") + ", socket ID=" + Stringize<PUID>(socket->getID()).string())
          return false;
        }

        if (!mUDPSocket) {
          ZS_LOG_WARNING(Detail, log("unable to send TURN data as UDP socket is not ready") + ", TURN ID=" + Stringize<PUID>(socket->getID()).string() + ", destination=" + destination.string() + ", packet length=" + Stringize<ULONG>(packetLengthInBytes).string())
          return false;
        }

        try {
          bool wouldBlock = false;
#ifdef HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
          if (true) return true;
#endif //HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
#ifdef HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          if (!destination.isAddressEqual(IPAddress(HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_SERVER_IP))) {
            return true;
          }
#endif //HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          ULONG bytesSent = mUDPSocket->sendTo(destination, packet, packetLengthInBytes, &wouldBlock);
          bool sent = ((!wouldBlock) && (bytesSent == packetLengthInBytes));
          if (!send) {
            ZS_LOG_WARNING(Debug, log("unable to send data on behalf of TURN as UDP socket did not send the data") + ", would block=" + (wouldBlock ? "true" : "false") + ", bytes sent=" + Stringize<ULONG>(bytesSent).string())
          }
          return sent;
        } catch(ISocket::Exceptions::Unspecified &error) {
          ZS_LOG_ERROR(Detail, log("sendTo error") + ", error=" + Stringize<int>(error.getErrorCode()).string())
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void ICESocket::onTURNSocketWriteReady(ITURNSocketPtr socket)
      {
        ZS_LOG_TRACE(log("notified that TURN is write ready") + ", TURN socket ID=" + Stringize<PUID>(socket->getID()).string())
        AutoRecursiveLock lock(mLock);
        for(ICESocketSessionMap::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter) {
          (*iter).second->notifyRelayWriteReady();
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onSTUNDiscoverySendPacket(
                                                ISTUNDiscoveryPtr discovery,
                                                zsLib::IPAddress destination,
                                                boost::shared_array<zsLib::BYTE> packet,
                                                zsLib::ULONG packetLengthInBytes
                                                )
      {
        ZS_LOG_TRACE(log("sending packet for STUN discovery") + ", destination=" + destination.string() + ", length=" + Stringize<ULONG>(packetLengthInBytes).string())

        AutoRecursiveLock lock(mLock);
        if (isShutdown()) {
          ZS_LOG_TRACE(log("cannot send packet as already shutdown"))
          return;
        }
        if (discovery != mSTUNDiscovery) {
          ZS_LOG_TRACE(log("STUN discovery object is not the 'current' object thus ignoring send request"))
          return;
        }

        if (!mUDPSocket) {
          ZS_LOG_WARNING(Detail, log("unable to send STUN discover packet as UDP socket is not ready") + ", discovery ID=" + Stringize<PUID>(discovery->getID()).string() + ", destination=" + destination.string() + ", packet length=" + Stringize<ULONG>(packetLengthInBytes).string())
          return;
        }

        try {
          bool wouldBlock = false;
#ifdef HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
          if (true) return;
#endif //HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
#ifdef HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          if (!destination.isAddressEqual(IPAddress(HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_SERVER_IP))) {
            return;
          }
#endif //HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          mUDPSocket->sendTo(destination, packet.get(), packetLengthInBytes, &wouldBlock);
        } catch(ISocket::Exceptions::Unspecified &error) {
          ZS_LOG_ERROR(Detail, log("sendTo error") + ", error=" + Stringize<int>(error.getErrorCode()).string())
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onSTUNDiscoveryComplete(ISTUNDiscoveryPtr discovery)
      {
        AutoRecursiveLock lock(mLock);
        if (isShutdown()) return;
        if (discovery != mSTUNDiscovery) return;

        ZS_LOG_DETAIL(log("notified STUN discovery finished") + ", reflected ip=" + mSTUNDiscovery->getMappedAddress().string())
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      bool ICESocket::sendTo(
                             IICESocket::Types viaTransport,
                             const zsLib::IPAddress &destination,
                             const zsLib::BYTE *buffer,
                             zsLib::ULONG bufferLengthInBytes,
                             bool isUserData
                             )
      {
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("cannot send packet via ICE socket as it is already shutdown") + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + Stringize<ULONG>(bufferLengthInBytes).string() + ", user data=" + (isUserData ? "true" : "false"))
          return false;
        }

        if (viaTransport == Type_Relayed) {
          if (!mTURNSocket) {
            ZS_LOG_WARNING(Debug, log("cannot send packet via TURN socket as it is not connected") + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + Stringize<ULONG>(bufferLengthInBytes).string() + ", user data=" + (isUserData ? "true" : "false"))
            return false;
          }

          mTURNLastUsed = zsLib::now();
          return mTURNSocket->sendPacket(destination, buffer, bufferLengthInBytes, isUserData);
        }

        if (!mUDPSocket) {
          ZS_LOG_WARNING(Debug, log("cannot send packet as UDP socket is not set") + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + Stringize<ULONG>(bufferLengthInBytes).string() + ", user data=" + (isUserData ? "true" : "false"))
          return false;
        }

        // attempt to send the packet over the UDP buffer
        try {
          bool wouldBlock = false;
#ifdef HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
          if (true) return true;
#endif //HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
#ifdef HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          if (!destination.isAddressEqual(IPAddress(HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_SERVER_IP))) {
            return true;
          }
#endif //HOOKFLASH_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          ULONG bytesSent = mUDPSocket->sendTo(destination, buffer, bufferLengthInBytes, &wouldBlock);
          ZS_LOG_TRACE(log("sending packet") + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + Stringize<ULONG>(bufferLengthInBytes).string() + ", user data=" + (isUserData ? "true" : "false") + ", bytes sent=" + Stringize<ULONG>(bytesSent).string() + ", would block=" + (wouldBlock ? "true" : "false"))
          return ((!wouldBlock) && (bufferLengthInBytes == bytesSent));
        } catch(ISocket::Exceptions::Unspecified &error) {
          ZS_LOG_ERROR(Detail, log("sendTo error") + ", error=" + Stringize<int>(error.getErrorCode()).string())
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void ICESocket::addRoute(IICESocketSessionForICESocketPtr session, const zsLib::IPAddress &source)
      {
        removeRoute(session);
        mRoutes[source] = session;
      }

      //-----------------------------------------------------------------------
      void ICESocket::removeRoute(IICESocketSessionForICESocketPtr inSession)
      {
        for (QuickRouteMap::iterator iter = mRoutes.begin(); iter != mRoutes.end(); ++iter) {
          IICESocketSessionForICESocketPtr &session = (*iter).second;
          if (session == inSession) {
            mRoutes.erase(iter);
            return;
          }
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onICESocketSessionClosed(zsLib::PUID sessionID)
      {
        ZS_LOG_DETAIL(log("notified ICE session closed") + ", session id=" + Stringize<PUID>(sessionID).string())

        AutoRecursiveLock lock(mLock);
        ICESocketSessionMap::iterator found = mSessions.find(sessionID);
        if (found == mSessions.end()) {
          ZS_LOG_WARNING(Detail, log("session is not found (must have already been closed)") + ", session id=" + Stringize<PUID>(sessionID).string())
          return;
        }

        removeRoute((*found).second);
        mSessions.erase(found);

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      void ICESocket::onTimer(TimerPtr timer)
      {
        ZS_LOG_DEBUG(log("timer"))
        AutoRecursiveLock lock(mLock);
        if (timer != mRebindTimer) {
          ZS_LOG_WARNING(Detail, log("received timer notification on obsolete timer") + ", timer ID=" + Stringize<PUID>(timer->getID()).string())
          return;
        }
        bindUDP();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      void ICESocket::getLocalIPs(IPAddressList &outList)
      {
        outList.clear();
        outList = mLocalIPs;
      }

      //-----------------------------------------------------------------------
      IPAddress ICESocket::getReflectedIP()
      {
        IPAddress result;

        if (mTURNSocket) {
          if (ITURNSocket::TURNSocketState_Ready == mTURNSocket->getState()) {
            result = mTURNSocket->getReflectedIP();
          }
        }

        if (result.isAddressEmpty()) {
          if (mSTUNDiscovery) {
            result = mSTUNDiscovery->getMappedAddress();
          }
        }

        if (!result.isAddressEmpty()) {
          if (result.isPortEmpty()) {
            result.setPort(mBindPort);  // just in case TURN TCP gave a port of "0"
          }
        }
        return result;
      }

      //-----------------------------------------------------------------------
      IPAddress ICESocket::getRelayedIP()
      {
        if (mTURNSocket) {
          if (ITURNSocket::TURNSocketState_Ready == mTURNSocket->getState()) {
            return mTURNSocket->getRelayedIP();
          }
        }
        return IPAddress();
      }

      //-----------------------------------------------------------------------
      void ICESocket::internalReceivedData(
                                           IICESocket::Types viaTransport,
                                           const zsLib::IPAddress &source,
                                           const zsLib::BYTE *buffer,
                                           zsLib::ULONG bufferLengthInBytes
                                           )
      {
        // WARNING: DO NOT CALL THIS METHOD WHILE INSIDE A LOCK AS IT COULD
        //          ** DEADLOCK **. This method calls delegates synchronously.
        STUNPacketPtr stun = STUNPacket::parseIfSTUN(buffer, bufferLengthInBytes, STUNPacket::RFC_AllowAll, false, "ICESocket", mID);

        if (stun) {
          ZS_LOG_DEBUG(log("received STUN packet") + ", via=" + toString(viaTransport) + ", ip=" + source.string() + ", class=" + stun->classAsString() + ", method=" + stun->methodAsString())
          ITURNSocketPtr turn;
          if (IICESocket::Type_Relayed != normalize(viaTransport)) {

            // scope: going into a lock to obtain
            {
              AutoRecursiveLock lock(getLock());
              turn = mTURNSocket;
            }

            if (turn) {
              if (turn->handleSTUNPacket(source, stun)) return;
            }
          }

          if (!turn) {
            // if TURN was used, we would already called this routine... (i.e. prevent double lookup)
            if (ISTUNRequesterManager::handleSTUNPacket(source, stun)) return;
          }

          IICESocketSessionForICESocketPtr next;

          if (STUNPacket::Method_Binding == stun->mMethod) {
            if ((STUNPacket::Class_Request != stun->mClass) &&
                (STUNPacket::Class_Indication != stun->mClass)) {
              ZS_LOG_WARNING(Detail, log("ignoring STUN binding which is not a request/indication"))
              return;
            }
          }

          if (STUNPacket::RFC_5766_TURN == stun->guessRFC(STUNPacket::RFC_AllowAll)) {
            ZS_LOG_TRACE(log("ignoring TURN message (likely for cancelled requests)"))
            return;    // ignore any ICE indications
          }

          if (stun->mUsername.isEmpty()) {
            ZS_LOG_WARNING(Detail, log("did not find ICE username on packet thus ignoring STUN packet"))
            return;  // no username is present - this cannot be for us...
          }

          // username is present... but does it have the correct components?
          size_t pos = stun->mUsername.find(":");
          if (String::npos == pos) {
            ZS_LOG_WARNING(Detail, log("did not find \":\" in username on packet thus ignoring STUN packet"))
            return;  // no ":" means that it can't be an ICE requeest
          }

          // split the string at the post
          String localUsernameFrag = stun->mUsername.substr(0, pos); // this would be our local username
          String remoteUsernameFrag = stun->mUsername.substr(pos+1);  // this would be the remote username

          while (true)
          {
            // scope: find the next socket session to test in the list while in a lock
            {
              AutoRecursiveLock lock(getLock());
              if (mSessions.size() < 1) break;  // no sessions to check

              if (!next) {
                next = (*(mSessions.begin())).second;
              } else {
                ICESocketSessionMap::iterator iter = mSessions.find(next->getID());
                if (iter == mSessions.end()) {
                  // should have been found BUT it is possible the list was
                  // changed while outside the lock so start the search from
                  // the beginning again...
                  next = (*(mSessions.begin())).second;
                } else {
                  ++iter;
                  if (iter == mSessions.end()) {
                    // while it is possible that a new session was inserted
                    // while we were processing we don't have ot check it
                    // as this packet could not possibly be for that session.
                    break;
                  }
                  next = (*iter).second;
                }
              }
            }

            if (!next) break;
            if (next->handleSTUNPacket(viaTransport, source, stun, localUsernameFrag, remoteUsernameFrag)) return;
          }

          ZS_LOG_WARNING(Debug, log("did not find session that handles STUN packet"))

          // no STUN outlets left to check so just exit...
          return;
        }

        // this isn't a STUN packet but it might be TURN channel data (but not if came from TURN)
        if (IICESocket::Type_Relayed != normalize(viaTransport)) {
          ITURNSocketPtr turn;

          // scope: going into a lock to obtain
          {
            AutoRecursiveLock lock(getLock());
            turn = mTURNSocket;
          }

          if (turn) {
            if (turn->handleChannelData(source, buffer, bufferLengthInBytes)) return;
          }
        }

        IICESocketSessionForICESocketPtr next;

        // try to find a quick route to the session
        {
          AutoRecursiveLock lock(getLock());
          QuickRouteMap::iterator found = mRoutes.find(source);
          if (found != mRoutes.end()) {
            next = (*found).second;
          }
        }

        if (next) {
          // we found a quick route - but does it actually handle the packet
          // (it is possible for two routes to have same IP in strange firewall
          // configruations thus we might pick the wrong session)
          if (next->handlePacket(viaTransport, source, buffer, bufferLengthInBytes)) return;

          // we chose wrong, so allow the "hunt" method to take over
          next.reset();
        }

        // this could be channel data for one of the sessions, check each session
        while (true)
        {
          // scope: find the next socket session to test in the list while in a lock
          {
            AutoRecursiveLock lock(getLock());
            if (mSessions.size() < 1) break;  // no sessions to check

            if (!next) {
              next = (*(mSessions.begin())).second;
            } else {
              ICESocketSessionMap::iterator iter = mSessions.find(next->getID());
              if (iter == mSessions.end()) {
                next = (*(mSessions.begin())).second;   // start the search over since the previous entry we last searched was not in the map
              } else {
                ++iter;
                if (iter == mSessions.end()) break;
                next = (*iter).second;
              }
            }
          }

          if (!next) break;
          if (next->handlePacket(viaTransport, source, buffer, bufferLengthInBytes)) return;
        }

        ZS_LOG_WARNING(Trace, log("did not find any socket session to handle data packet"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      void ICESocket::cancelSubscription(zsLib::PUID subscriptionID)
      {
        AutoRecursiveLock lock(getLock());
        DelegateMap::iterator found = mDelegates.find(subscriptionID);
        if (found == mDelegates.end()) {
          ZS_LOG_DETAIL(log("subscription not found (already cancelled?)") + ", subscription ID=" + zsLib::Stringize<PUID>(subscriptionID).string())
          return;
        }

        ZS_LOG_DETAIL(log("subscription cancelled") + ", subscription ID=" + zsLib::Stringize<PUID>(subscriptionID).string())
        mDelegates.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      zsLib::String ICESocket::log(const char *message) const
      {
        return String("ICESocket [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void ICESocket::cancel()
      {
        if (isShutdown()) return;
        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        setState(ICESocketState_ShuttingDown);

        if (mRebindTimer) {
          mRebindTimer->cancel();
          mRebindTimer.reset();
        }

        if (mTURNSocket) {
          ZS_LOG_DETAIL(log("shutdown of TURN socket") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())
          mTURNSocket->shutdown();
        }

        if (mGracefulShutdownReference) {
          if (mTURNSocket) {
            if (ITURNSocket::TURNSocketState_Shutdown != mTURNSocket->getState()) {
              ZS_LOG_DETAIL(log("waiting for turn socket to shutdown") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())
              // not completely shutdown yet...
              return;
            }
          }
        }

        // inform that the graceful stutdown has completed...
        setState(ICESocketState_Shutdown);

        ZS_LOG_BASIC(log("shutdown"))

        mGracefulShutdownReference.reset();
        mDelegates.clear();

        if (mSessions.size() > 0) {
          ICESocketSessionMap temp = mSessions;
          mSessions.clear();

          // close down all the ICE sessions immediately
          for(ICESocketSessionMap::iterator iter = temp.begin(); iter != temp.end(); ++iter) {
            (*iter).second->close();
          }
        }
        mRoutes.clear();

        if (mUDPSocket) {
          mUDPSocket->close();
          mUDPSocket.reset();
        }

        mTURNSocket.reset();

        mSTUNSRVResult.reset();
        mSTUNServer.clear();

        if (mSTUNDiscovery) {
          mSTUNDiscovery->cancel();
          mSTUNDiscovery.reset();
        }

        mLocalIPs.clear();
      }

      //-----------------------------------------------------------------------
      void ICESocket::step()
      {
        if ((isShuttingDown()) ||
            (isShutdown())) {
          cancel();
          return;
        }

        // if there are no local IPs then gather those IP addresses now
        if (!gatherLocalIPs())
          return;

        zsLib::Time current = zsLib::now();

        if (mTURNLastUsed + mTURNShutdownIfNotUsedBy < current)
        {
          // the socket can be put to sleep...
          mTURNShutdownIfNotUsedBy = zsLib::Seconds(HOOKFLASH_SERVICES_ICESOCKET_MINIMUM_TURN_KEEP_ALIVE_TIME_IN_SECONDS);  // reset to minimum again...

          if (mTURNSocket) {
            ZS_LOG_DEBUG(log("TURN server can go to sleep") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())

            mTURNSocket->shutdown();

            if (ITURNSocket::TURNSocketState_Shutdown == mTURNSocket->getState()) {
              ZS_LOG_DEBUG(log("TURN server is now shutdown/asleep") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())

              // TURN socket shutdown is complete...
              mTURNSocket.reset();
              mSTUNDiscovery.reset();

              clearReflectedCandidates();
              clearRelayedCandidates();

              setState(ICESocketState_Sleeping);
            } else {
              setState(ICESocketState_GoingToSleep);
            }
          }

          // the socket is going to sleep or is sleeping now...
          return;
        }

        // right now TURN should be activated...
        if (!mTURNSocket)
        {
          if (!mTURNServer.isEmpty()) {
            mTURNSocket = ITURNSocket::create(
                                              getAssociatedMessageQueue(),
                                              mThisWeak.lock(),
                                              mTURNServer,
                                              mTURNUsername,
                                              mTURNPassword,
                                              mFirstWORDInAnyPacketWillNotConflictWithTURNChannels
                                              );
          } else {
            mTURNSocket = ITURNSocket::create(
                                              getAssociatedMessageQueue(),
                                              mThisWeak.lock(),
                                              mTURNSRVUDPResult,
                                              mTURNSRVTCPResult,
                                              mTURNUsername,
                                              mTURNPassword,
                                              mFirstWORDInAnyPacketWillNotConflictWithTURNChannels
                                              );
          }

          if (mTURNSocket) {
            ZS_LOG_DEBUG(log("TURN socket created") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())
          }

          // clear out the STUN discovery if we are performing a TURN connection...
          if (mSTUNDiscovery) {
            mSTUNDiscovery->cancel();
            mSTUNDiscovery.reset();
          }

          clearReflectedCandidates();
          clearRelayedCandidates();
        }

        if (ITURNSocket::TURNSocketState_Shutdown == mTURNSocket->getState()) {
          // TURN socket failed; nothing we can do about it; see if we can activate STUN instead...

          // the reflected IP is now known, do a STUN discovery
          if (!mSTUNDiscovery) {
            ZS_LOG_DETAIL(log("starting STUN discovery"))

            if (mSTUNSRVResult) {
              mSTUNDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), mSTUNSRVResult);
            }
            if (!mSTUNServer.isEmpty()) {
              mSTUNDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), mSTUNServer);
            }

            if (mSTUNDiscovery) {
              ZS_LOG_DETAIL(log("started STUN discovery") + ", STUN discovery ID=" + Stringize<PUID>(mSTUNDiscovery->getID()).string())
            }

            clearReflectedCandidates();
            clearRelayedCandidates();
          }
        }

        // at this point we should have a reflectd IP or TURN failed (or was shutdown intentionally)
        if (mTURNSocket) {
          if (ITURNSocket::TURNSocketState_Ready == mTURNSocket->getState()) {
            ZS_LOG_DEBUG(log("TURN socket reported ready") + ", TURN socket ID=" + Stringize<PUID>(mTURNSocket->getID()).string())
            setState(ICESocketState_Ready);
          }
        }

        if (mSTUNDiscovery) {
          if (mSTUNDiscovery->isComplete()) {
            ZS_LOG_DETAIL(log("STUN discovery is complete") + ", STUN discovery ID=" + Stringize<PUID>(mSTUNDiscovery->getID()).string())
            setState(ICESocketState_Ready);
          }
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::setState(ICESocketStates state)
      {
        if (mCurrentState == state) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        ICESocketPtr pThis = mThisWeak.lock();

        if (pThis) {
          // notify the delegates that the state has changed
          for (DelegateMap::iterator delIter = mDelegates.begin(); delIter != mDelegates.end(); )
          {
            DelegateMap::iterator current = delIter;
            ++delIter;

            IICESocketDelegatePtr delegate = (*current).second;
            try {
              delegate->onICESocketStateChanged(pThis, mCurrentState);
            } catch(IICESocketDelegateProxy::Exceptions::DelegateGone &) {
              mDelegates.erase(current);
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      bool ICESocket::gatherLocalIPs()
      {
        if (!mLocalIPs.empty())
          return true;

#ifdef _WIN32

        // http://tangentsoft.net/wskfaq/examples/ipaddr.html

        // OR
        ZS_LOG_DEBUG(log("--- GATHERING LOCAL IPs: START ---"))

        ULONG size = 0;

        // the 1st call is just to get the table size
        if(GetIpAddrTable(NULL, &size, FALSE) == ERROR_INSUFFICIENT_BUFFER)
        {
          // now that you know the size, allocate a pointer
          MIB_IPADDRTABLE *ipAddr = (MIB_IPADDRTABLE *) new BYTE[size];
          // the 2nd call is to retrieve the info for real
          if(GetIpAddrTable(ipAddr, &size, TRUE) == NO_ERROR)
          {
            // need to loop it to handle multiple interfaces
            for(DWORD i = 0; i < ipAddr->dwNumEntries; i++)
            {
              // this is the IP address
              DWORD dwordIP = ntohl(ipAddr->table[i].dwAddr);
              IPAddress ip(dwordIP);

              if (ip.isAddressEmpty()) continue;
              if (ip.isLoopback()) continue;
              if (ip.isAddrAny()) continue;

              ip.setPort(mBindPort);

              ZS_LOG_DEBUG(log("found local IP") + ", ip=" + ip.string())

              mLocalIPs.push_back(ip);
            }
          }
        }
        ZS_LOG_DEBUG(log("--- GATHERING LOCAL IPs: END ---"))

#else
        ifaddrs *ifAddrStruct = NULL;
        ifaddrs *ifa = NULL;

        getifaddrs(&ifAddrStruct);

        ZS_LOG_DEBUG(log("--- GATHERING LOCAL IPs: START ---"))
        for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
        {
          IPAddress ip;
          if (AF_INET == ifa ->ifa_addr->sa_family) {
            ip = IPAddress(*((sockaddr_in *)ifa->ifa_addr));      // this is an IPv4 address
          } else if (AF_INET6 == ifa->ifa_addr->sa_family) {
#if 0
            // NOT GOING TO SUPPORT JUST YET
            ip = IPAddress(*((sockaddr_in6 *)ifa->ifa_addr));     // this is an IPv6 address
#endif //0
          }

          // do not add these addresses...
          if (ip.isAddressEmpty()) continue;
          if (ip.isLoopback()) continue;
          if (ip.isAddrAny()) continue;

          ip.setPort(mBindPort);

          ZS_LOG_DEBUG(log("found local IP") + ", local IP=" + ip.string())

          mLocalIPs.push_back(ip);
        }
        ZS_LOG_DEBUG(log("--- GATHERING LOCAL IPs: END ---"))

        if (ifAddrStruct) {
          freeifaddrs(ifAddrStruct);
          ifAddrStruct = NULL;
        }
#endif //_WIN32

        if (mLocalIPs.empty()) {
          cancel();
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool ICESocket::bindUDP()
      {
        if (mUDPSocket) {
          if (mRebindTimer) {
            mRebindTimer->cancel();
            mRebindTimer.reset();
          }
          ZS_LOG_WARNING(Detail, log("socket is already bound"))
          return true;
        }

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_ERROR(Detail, log("already shutting down - do not rebind"))
          return false;
        }

        bool failed = false;

        try {
          mUDPSocket = Socket::createUDP();

          IPAddress any = IPAddress::anyV4();
          any.setPort(mBindPort);

          mUDPSocket->bind(any);
          mUDPSocket->setBlocking(false);
          try {
            mUDPSocket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
          } catch(ISocket::Exceptions::UnsupportedSocketOption &) {
          }
          mUDPSocket->setDelegate(mThisWeak.lock());
          IPAddress local = mUDPSocket->getLocalAddress();
          mBindPort = local.getPort();
          ZS_THROW_CUSTOM_PROPERTIES_1_IF(ISocket::Exceptions::Unspecified, 0 == mBindPort, 0)
        } catch(ISocket::Exceptions::Unspecified &error) {
          ZS_LOG_ERROR(Detail, log("bind error") + ", error=" + Stringize<int>(error.getErrorCode()).string())
          failed = true;
        }

        Time tick = zsLib::now();

        if (failed) {
          mUDPSocket->close();
          mUDPSocket.reset();

          if (Time() != mRebindAttemptStartTime) {
            if (mRebindAttemptStartTime + mMaxRebindAttemptDuration < tick) {
              ZS_LOG_ERROR(Detail, log("unable to rebind the socket thus cancelling the whole socket"))
              cancel();
              return false;
            }

            if (!mRebindTimer) {
              mRebindTimer = Timer::create(mThisWeak.lock(), Seconds(1));
            }
          }
          return false;
        }

        if (mRebindTimer) {
          mRebindTimer->cancel();
          mRebindTimer.reset();
        }

        if (!mMonitoringWriteReady) {
          monitorWriteReadyOnAllSessions(false);
        }

        ZS_LOG_DEBUG(log("UDP is now rebinded successfully"))

        return true;
      }

      //-----------------------------------------------------------------------
      bool ICESocket::clearRelayedCandidates(bool checkOnly)
      {
        // clear out the candidate representing TURN since it can't be used anymore
        for (CandidateList::iterator iter = mLocalCandidates.begin(); iter != mLocalCandidates.end(); ++iter) {
          Candidate &candidate = (*iter);
          if (IICESocket::Type_Relayed == candidate.mType) {
            if (!checkOnly) {
              ZS_LOG_DEBUG(log("clearing relay candidate"))
              mLocalCandidates.erase(iter);     // there can only ever be one candidate representing TURN
            }
            return true;
          }
        }
        return false;
      }

      //-----------------------------------------------------------------------
      bool ICESocket::clearReflectedCandidates(bool checkOnly)
      {
        // clear out the candidate representing TURN since it can't be used anymore
        for (CandidateList::iterator iter = mLocalCandidates.begin(); iter != mLocalCandidates.end(); ++iter) {
          Candidate &candidate = (*iter);
          if (IICESocket::Type_ServerReflexive == candidate.mType) {
            if (!checkOnly) {
              ZS_LOG_DEBUG(log("clearing relected candidate"))
              mLocalCandidates.erase(iter);     // there can only ever be one candidate representing TURN
            }
            return true;
          }
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void ICESocket::makeCandidates()
      {
        IPAddressList localIPs;
        getLocalIPs(localIPs);
        localIPs.sort(compareLocalIPs);

        if (mLocalCandidates.size() < 1) {
          ULONG index = 0xFFFF;
          for (IPAddressList::iterator iter = localIPs.begin(); iter != localIPs.end(); ++iter, index -= 0xF) {
            Candidate candidate;
            candidate.mType = IICESocket::Type_Local;
            candidate.mIPAddress = (*iter);
            candidate.mLocalPreference = static_cast<WORD>(index);
            candidate.mPriority = ((1 << 24)*(static_cast<DWORD>(candidate.mType))) + ((1 << 8)*(static_cast<DWORD>(candidate.mLocalPreference))) + (256 - 0);

            candidate.mUsernameFrag = mUsernameFrag;
            candidate.mPassword = mPassword;
            mLocalCandidates.push_back(candidate);
          }
        }

        if (!hasReflectedCandidate()) {
          IPAddress reflectedIP = getReflectedIP();
          if (!reflectedIP.isAddressEmpty()) {
            if (!containsIP(localIPs, reflectedIP)) {
              // and the reflected IP
              Candidate candidate;
              candidate.mType = IICESocket::Type_ServerReflexive;
              candidate.mIPAddress = reflectedIP;
              candidate.mLocalPreference = 0;
              candidate.mPriority = ((1 << 24)*(static_cast<DWORD>(candidate.mType))) + ((1 << 8)*(static_cast<DWORD>(candidate.mLocalPreference))) + (256 - 0);

              candidate.mUsernameFrag = mUsernameFrag;
              candidate.mPassword = mPassword;
              mLocalCandidates.push_back(candidate);
            }
          }
        }

        if (!hasRelayedCandidate()) {
          IPAddress relayedIP = getRelayedIP();
          if (!relayedIP.isAddressEmpty()) {
            // add the relayed IP
            Candidate candidate;
            candidate.mType = IICESocket::Type_Relayed;
            candidate.mIPAddress = relayedIP;
            candidate.mLocalPreference = 0;
            candidate.mPriority = ((1 << 24)*(static_cast<DWORD>(candidate.mType))) + ((1 << 8)*(static_cast<DWORD>(candidate.mLocalPreference))) + (256 - 0);

            candidate.mUsernameFrag = mUsernameFrag;
            candidate.mPassword = mPassword;
            mLocalCandidates.push_back(candidate);
          }
        }

        // repair candidates based on foundation...
        if (mFoundation) {
          CandidateList foundationCandidates;
          mFoundation->getLocalCandidates(foundationCandidates);

          // attempt to match the foundation candidates to the local candidates
          for (CandidateList::iterator localIter = mLocalCandidates.begin(); localIter != mLocalCandidates.end(); ++localIter)
          {
            Candidate &localCandidate = (*localIter);
            for (CandidateList::iterator foundationIter = foundationCandidates.begin(); foundationIter != foundationCandidates.end(); ++foundationIter)
            {
              Candidate &foundationCandidate = (*foundationIter);

              if (localCandidate.mType != foundationCandidate.mType) continue;
              if (!localCandidate.mIPAddress.isAddressEqual(foundationCandidate.mIPAddress)) continue;
              if (localCandidate.mPriority != foundationCandidate.mPriority) continue;
              if (localCandidate.mLocalPreference != foundationCandidate.mLocalPreference) continue;

              if ((localCandidate.mUsernameFrag != foundationCandidate.mUsernameFrag) ||
                  (localCandidate.mPassword != foundationCandidate.mPassword)) {
                // found a close match (thus using it's values - i.e. the username and password from the foundation)
                ZS_LOG_TRACE(log("----------------------------------------------------"))
                ZS_LOG_TRACE(log("REPAIRING CANDIDATE INFORMATION BASED ON FOUNDATION:") + " foundation ID=" + Stringize<PUID>(mFoundation->getID()).string())
                ZS_LOG_TRACE(log("Foundation contains") + foundationCandidate.toDebugString())
                ZS_LOG_TRACE(log("Local contains") + localCandidate.toDebugString())

                localCandidate.mUsernameFrag = foundationCandidate.mUsernameFrag;
                localCandidate.mPassword = foundationCandidate.mPassword;

                ZS_LOG_TRACE(log("Resulting local contains") + localCandidate.toDebugString())
                ZS_LOG_TRACE(log("----------------------------------------------------"))
              }
            }
          }
        }
      }

      //-------------------------------------------------------------------------
      void ICESocket::getBuffer(RecycledPacketBuffer &outBuffer)
      {
        AutoRecursiveLock lock(mLock);
        if (mRecycledBuffers.size() < 1) {
          outBuffer = RecycledPacketBuffer(new BYTE[HOOKFLASH_SERVICES_ICESOCKET_RECYCLE_BUFFER_SIZE]);
          return;
        }

        outBuffer = mRecycledBuffers.front();
        mRecycledBuffers.pop_front();
      }

      //-------------------------------------------------------------------------
      void ICESocket::recycleBuffer(RecycledPacketBuffer &buffer)
      {
        AutoRecursiveLock lock(mLock);
        if (!buffer) return;

        if (mRecycledBuffers.size() >= HOOKFLASH_SERVICES_ICESOCKET_MAX_RECYLCE_BUFFERS) {
          buffer.reset();
          return;
        }
        mRecycledBuffers.push_back(buffer);
      }

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      ICESocket::Subscription::Subscription(ICESocketPtr outer) :
        mOuter(outer),
        mID(zsLib::createPUID())
      {
      }

      ICESocket::Subscription::~Subscription()
      {
        cancel();
      }

      ICESocket::SubscriptionPtr ICESocket::Subscription::create(ICESocketPtr outer)
      {
        SubscriptionPtr pThis(new Subscription(outer));
        return pThis;
      }

      void ICESocket::Subscription::cancel()
      {
        ICESocketPtr outer = mOuter.lock();
        if (!outer) return;

        outer->cancelSubscription(mID);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    const char *IICESocket::toString(ICESocketStates states)
    {
      switch (states) {
        case ICESocketState_Pending:     return "Preparing";
        case ICESocketState_Ready:         return "Ready";
        case ICESocketState_GoingToSleep:  return "Going to sleep";
        case ICESocketState_Sleeping:      return "Sleeping";
        case ICESocketState_ShuttingDown:  return "Shutting down";
        case ICESocketState_Shutdown:      return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IICESocket::toString(Types type)
    {
      switch (type) {
        case Type_Unknown:          return "unknown";
        case Type_Local:            return "local";
        case Type_ServerReflexive:  return "server reflexive";
        case Type_PeerReflexive:    return "peer reflexive";
        case Type_Relayed:          return "relayed";
      }
      return NULL;
    }

    //-------------------------------------------------------------------------
    IICESocketPtr IICESocket::create(
                                     zsLib::IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     const char *turnServer,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     const char *stunServer,
                                     zsLib::WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels
                                     )
    {
      return internal::ICESocket::create(
                                         queue,
                                         delegate,
                                         turnServer,
                                         turnServerUsername,
                                         turnServerPassword,
                                         stunServer,
                                         port,
                                         firstWORDInAnyPacketWillNotConflictWithTURNChannels);
    }

    //-------------------------------------------------------------------------
    IICESocketPtr IICESocket::create(
                                     zsLib::IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     IDNS::SRVResultPtr srvTURNUDP,
                                     IDNS::SRVResultPtr srvTURNTCP,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     IDNS::SRVResultPtr srvSTUN,
                                     zsLib::WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels
                                     )
    {
      return internal::ICESocket::create(
                                         queue,
                                         delegate,
                                         srvTURNUDP,
                                         srvTURNTCP,
                                         turnServerUsername,
                                         turnServerPassword,
                                         srvSTUN,
                                         port,
                                         firstWORDInAnyPacketWillNotConflictWithTURNChannels);
    }

    String IICESocket::Candidate::toDebugString() const
    {
      return String(", type=") + IICESocket::toString(mType) +
                    ", ip=" + mIPAddress.string() +
                    ", priority=" + Stringize<DWORD>(mPriority).string() +
                    ", preference=" + Stringize<WORD>(mLocalPreference).string() +
                    ", usernameFrag=" + mUsernameFrag +
                    ", password=" + mPassword +
                    ", protocol=" + mProtocol;
    }
  }
}
