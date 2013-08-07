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

#include <openpeer/services/internal/services_ICESocket.h>
#include <openpeer/services/internal/services_ICESocketSession.h>
#include <openpeer/services/internal/services_TURNSocket.h>
#include <openpeer/services/internal/services_Helper.h>

#include <openpeer/services/ISTUNRequesterManager.h>
#include <openpeer/services/IHTTP.h>

#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/Numeric.h>
#include <zsLib/Stringize.h>
#include <zsLib/XML.h>
#include <zsLib/types.h>

#include <cryptopp/osrng.h>
#include <cryptopp/crc.h>

#ifdef _ANDROID
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif // _ANDROID

#ifndef _WIN32
#ifndef _ANDROID
#include <sys/types.h>
#include <ifaddrs.h>
#endif //_WIN32
#endif //_ANDROID

#define OPENPEER_SERVICES_ICESOCKET_RECYCLE_BUFFER_SIZE  (1 << (sizeof(WORD)*8))
#define OPENPEER_SERVICES_ICESOCKET_MAX_RECYLCE_BUFFERS  4

#define OPENPEER_SERVICES_ICESOCKET_MINIMUM_TURN_KEEP_ALIVE_TIME_IN_SECONDS  OPENPEER_SERVICES_IICESOCKET_DEFAULT_HOW_LONG_CANDIDATES_MUST_REMAIN_VALID_IN_SECONDS

#define OPENPEER_SERVICES_MAX_REBIND_ATTEMPT_DURATION_IN_SECONDS (10)

#define OPENPEER_SERVICES_REBIND_TIMER_WHEN_NO_SOCKETS_IN_SECONDS (1)
#define OPENPEER_SERVICES_REBIND_TIMER_WHEN_HAS_SOCKETS_IN_SECONDS (30)


namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }


namespace openpeer
{
  namespace services
  {
    using zsLib::IPv6PortPair;
    using zsLib::string;
    typedef CryptoPP::CRC32 CRC32;

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

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
      static bool compare(const IICESocket::Candidate &candidate1, const IICESocket::Candidate &candidate2)
      {
        if (candidate1.mType != candidate2.mType) return false;
        if (candidate1.mPriority != candidate2.mPriority) return false;
        if (candidate1.mIPAddress != candidate2.mIPAddress) return false;
        if (candidate1.mFoundation != candidate2.mFoundation) return false;

        return true;
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
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      ICESocket::ICESocket(
                           IMessageQueuePtr queue,
                           IICESocketDelegatePtr delegate,
                           IDNS::SRVResultPtr srvTURNUDP,
                           IDNS::SRVResultPtr srvTURNTCP,
                           const char *turnServer,
                           const char *turnUsername,
                           const char *turnPassword,
                           bool firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                           IDNS::SRVResultPtr srvSTUN,
                           const char *stunServer,
                           WORD port,
                           IICESocketPtr foundationSocket
                           ) :
        MessageQueueAssociator(queue),

        mCurrentState(ICESocketState_Pending),

        mFoundation(ICESocket::convert(foundationSocket)),

        mBindPort(port),
        mUsernameFrag(IHelper::randomString(20)),
        mPassword(IHelper::randomString(20)),

        mNextLocalPreference(0xFFFF),

        mRebindAttemptStartTime(zsLib::now()),

        mMonitoringWriteReady(true),

        mTURNSRVUDPResult(srvTURNUDP),
        mTURNSRVTCPResult(srvTURNTCP),
        mTURNServer(turnServer ? turnServer : ""),
        mTURNUsername(turnUsername ? turnUsername : ""),
        mTURNPassword(turnPassword ? turnPassword : ""),
        mFirstWORDInAnyPacketWillNotConflictWithTURNChannels(firstWORDInAnyPacketWillNotConflictWithTURNChannels),
        mTURNLastUsed(zsLib::now()),
        mTURNShutdownIfNotUsedBy(Seconds(OPENPEER_SERVICES_ICESOCKET_MINIMUM_TURN_KEEP_ALIVE_TIME_IN_SECONDS)),

        mSTUNSRVResult(srvSTUN),
        mSTUNServer(stunServer ? stunServer : ""),

        mLastCandidateCRC(0)
      {
        mDefaultSubscription = mSubscriptions.subscribe(delegate, queue);
        ZS_LOG_BASIC(log("created"))

        // calculate the empty list CRC value
        CRC32 crc;
        crc.Final((BYTE *)(&mLastCandidateCRC));
      }

      //-----------------------------------------------------------------------
      void ICESocket::init()
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DETAIL(log("init"))

        step();
      }
      
      //-----------------------------------------------------------------------
      ICESocket::~ICESocket()
      {
        if (isNoop()) return;

        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      ICESocketPtr ICESocket::convert(IICESocketPtr socket)
      {
        return boost::dynamic_pointer_cast<ICESocket>(socket);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket => IICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      String ICESocket::toDebugString(IICESocketPtr socket, bool includeCommaPrefix)
      {
        if (!socket) return String(includeCommaPrefix ? ", ice socket=(null)" : "ice socket=(null)");

        ICESocketPtr pThis = ICESocket::convert(socket);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ICESocketPtr ICESocket::create(
                                     IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     const char *turnServer,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     const char *stunServer,
                                     WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                     IICESocketPtr foundationSocket
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
                                         port,
                                         foundationSocket));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ICESocketPtr ICESocket::create(
                                     IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     IDNS::SRVResultPtr srvTURNUDP,
                                     IDNS::SRVResultPtr srvTURNTCP,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     IDNS::SRVResultPtr srvSTUN,
                                     WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                     IICESocketPtr foundationSocket
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
                                         port,
                                         foundationSocket));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IICESocketSubscriptionPtr ICESocket::subscribe(IICESocketDelegatePtr originalDelegate)
      {
        ZS_LOG_DETAIL(log("subscribing to socket state"))

        AutoRecursiveLock lock(getLock());
        if (!originalDelegate) return mDefaultSubscription;

        IICESocketSubscriptionPtr subscription = mSubscriptions.subscribe(originalDelegate);

        IICESocketDelegatePtr delegate = mSubscriptions.delegate(subscription);

        if (delegate) {
          ICESocketPtr pThis = mThisWeak.lock();

          if (ICESocketState_Pending != mCurrentState) {
            delegate->onICESocketStateChanged(pThis, mCurrentState);
          }
          if (mNotifiedCandidateChanged) {
            delegate->onICESocketCandidatesChanged(pThis);
          }
        }

        if (isShutdown()) {
          mSubscriptions.clear();
        }
        
        return subscription;
      }
      
      //-----------------------------------------------------------------------
      IICESocket::ICESocketStates ICESocket::getState(
                                                      WORD *outLastErrorCode,
                                                      String *outLastErrorReason
                                                      ) const
      {
        AutoRecursiveLock lock(mLock);
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      String ICESocket::getUsernameFrag() const
      {
        AutoRecursiveLock lock(mLock);
        return mUsernameFrag;
      }

      //-----------------------------------------------------------------------
      String ICESocket::getPassword() const
      {
        AutoRecursiveLock lock(mLock);
        return mPassword;
      }

      //-----------------------------------------------------------------------
      void ICESocket::shutdown()
      {
        ZS_LOG_DETAIL(log("shutdown requested"))

        AutoRecursiveLock lock(mLock);
        cancel();
      }

      //-----------------------------------------------------------------------
      void ICESocket::wakeup(Duration minimumTimeCandidatesMustRemainValidWhileNotUsed)
      {
        AutoRecursiveLock lock(mLock);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("received request to wake up while ICE socket is shutting down or shutdown"))
          return;
        }

        mTURNLastUsed = zsLib::now();
        mTURNShutdownIfNotUsedBy = (mTURNShutdownIfNotUsedBy > minimumTimeCandidatesMustRemainValidWhileNotUsed ? mTURNShutdownIfNotUsedBy : minimumTimeCandidatesMustRemainValidWhileNotUsed);

        step();
      }

      //-----------------------------------------------------------------------
      void ICESocket::getLocalCandidates(CandidateList &outCandidates)
      {
        AutoRecursiveLock lock(mLock);

        outCandidates.clear();

        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); ++iter)
        {
          LocalSocketPtr &localSocket = (*iter).second;

          if (!localSocket->mLocal.mIPAddress.isEmpty()) {
            outCandidates.push_back(localSocket->mLocal);
          }
        }
        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); ++iter)
        {
          LocalSocketPtr &localSocket = (*iter).second;

          if (!localSocket->mReflexive.mIPAddress.isEmpty()) {
            outCandidates.push_back(localSocket->mReflexive);
          }
        }
        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); ++iter)
        {
          LocalSocketPtr &localSocket = (*iter).second;

          if (!localSocket->mRelay.mIPAddress.isEmpty()) {
            outCandidates.push_back(localSocket->mRelay);
          }
        }
      }

      //-----------------------------------------------------------------------
      IICESocketSessionPtr ICESocket::createSessionFromRemoteCandidates(
                                                                        IICESocketSessionDelegatePtr delegate,
                                                                        const char *remoteUsernameFrag,
                                                                        const char *remotePassword,
                                                                        const CandidateList &remoteCandidates,
                                                                        ICEControls control
                                                                        )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!remoteUsernameFrag)
        ZS_THROW_INVALID_ARGUMENT_IF(!remotePassword)

        AutoRecursiveLock lock(mLock);

        ICESocketSessionPtr session = IICESocketSessionForICESocket::create(getAssociatedMessageQueue(), delegate, mThisWeak.lock(), remoteUsernameFrag, remotePassword, control);
        ZS_THROW_BAD_STATE_IF(!session)

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Basic, log("create session called after socket is being shutdown"))
          // immediately close the session since we are shutting down
          session->forICESocket().close();
          return session;
        }

        // remember the session for later
        mSessions[session->forICESocket().getID()] = session;
        session->forICESocket().updateRemoteCandidates(remoteCandidates);
        return session;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket => IICESocketForICESocketSession
      #pragma mark

      //-----------------------------------------------------------------------
      bool ICESocket::sendTo(
                             const IPAddress &viaLocalIP,
                             IICESocket::Types viaTransport,
                             const IPAddress &destination,
                             const BYTE *buffer,
                             ULONG bufferLengthInBytes,
                             bool isUserData
                             )
      {
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("cannot send packet via ICE socket as it is already shutdown") + ", via local IP" + string(viaLocalIP) + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + string(bufferLengthInBytes) + ", user data=" + (isUserData ? "true" : "false"))
          return false;
        }

        SocketPtr socket;
        ITURNSocketPtr turnSocket;

        // get socket or turn socket value
        {
          AutoRecursiveLock lock(getLock());

          SocketLocalIPMap::iterator found = mSocketLocalIPs.find(viaLocalIP);
          if (found == mSocketLocalIPs.end()) {
            ZS_LOG_WARNING(Detail, log("did not find local IP to use"))
            return false;
          }

          LocalSocketPtr &localSocket = (*found).second;
          if (viaTransport == Type_Relayed) {
            turnSocket = localSocket->mTURNSocket;
          } else {
            socket = localSocket->mSocket;
          }
        }

        if (viaTransport == Type_Relayed) {
          if (!turnSocket) {
            ZS_LOG_WARNING(Debug, log("cannot send packet via TURN socket as it is not connected") + ", via local IP" + string(viaLocalIP) + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + string(bufferLengthInBytes) + ", user data=" + (isUserData ? "true" : "false"))
            return false;
          }

          mTURNLastUsed = zsLib::now();
          return turnSocket->sendPacket(destination, buffer, bufferLengthInBytes, isUserData);
        }

        if (!socket) {
          ZS_LOG_WARNING(Debug, log("cannot send packet as UDP socket is not set") + ", via local IP" + string(viaLocalIP) + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + string(bufferLengthInBytes) + ", user data=" + (isUserData ? "true" : "false"))
          return false;
        }

        // attempt to send the packet over the UDP buffer
        try {
          bool wouldBlock = false;
#ifdef OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
          if (true) return true;
#endif //OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
#ifdef OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          if (!destination.isAddressEqual(IPAddress(OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_SERVER_IP))) {
            return true;
          }
#endif //OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          ULONG bytesSent = socket->sendTo(destination, buffer, bufferLengthInBytes, &wouldBlock);
          ZS_LOG_TRACE(log("sending packet") + ", via local IP" + string(viaLocalIP) + ", via=" + toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + string(bufferLengthInBytes) + ", user data=" + (isUserData ? "true" : "false") + ", bytes sent=" + string(bytesSent) + ", would block=" + (wouldBlock ? "true" : "false"))
          return ((!wouldBlock) && (bufferLengthInBytes == bytesSent));
        } catch(ISocket::Exceptions::Unspecified &error) {
          ZS_LOG_ERROR(Detail, log("sendTo error") + ", error=" + string(error.getErrorCode()))
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void ICESocket::addRoute(ICESocketSessionPtr session, const IPAddress &source)
      {
        removeRoute(session);
        mRoutes[source] = session;
      }

      //-----------------------------------------------------------------------
      void ICESocket::removeRoute(ICESocketSessionPtr inSession)
      {
        for (QuickRouteMap::iterator iter = mRoutes.begin(); iter != mRoutes.end(); ++iter) {
          ICESocketSessionPtr &session = (*iter).second;
          if (session == inSession) {
            mRoutes.erase(iter);
            return;
          }
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onICESocketSessionClosed(PUID sessionID)
      {
        ZS_LOG_DETAIL(log("notified ICE session closed") + ", session id=" + string(sessionID))

        AutoRecursiveLock lock(mLock);
        ICESocketSessionMap::iterator found = mSessions.find(sessionID);
        if (found == mSessions.end()) {
          ZS_LOG_WARNING(Detail, log("session is not found (must have already been closed)") + ", session id=" + string(sessionID))
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
      #pragma mark
      #pragma mark ICESocket => ISocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocket::monitorWriteReadyOnAllSessions(bool monitor)
      {
        AutoRecursiveLock lock(mLock);

        mMonitoringWriteReady = monitor;

        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); )
        {
          LocalSocketPtr &localSocket = (*iter).second;

          if (monitor) {
            localSocket->mSocket->monitor(ISocket::Monitor::All);
          } else {
            localSocket->mSocket->monitor((ISocket::Monitor::Options)(ISocket::Monitor::Read | ISocket::Monitor::Exception));
          }
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onReadReady(ISocketPtr socket)
      {
        boost::shared_array<BYTE> buffer;
        IPAddress viaLocalIP;
        IPAddress source;
        ULONG bytesRead = 0;
        AutoRecycleBuffer recycle(*this, buffer);

        // scope: we are going to read the data while within the local but process it outside the lock
        {
          AutoRecursiveLock lock(mLock);

          SocketMap::iterator found = mSockets.find(socket);
          if (found == mSockets.end()) {
            ZS_LOG_WARNING(Detail, log("UDP socket is not ready"))
            return;
          }

          LocalSocketPtr &localSocket = (*found).second;
          viaLocalIP = localSocket->mLocal.mIPAddress;

          try {
            bool wouldBlock = false;

            getBuffer(buffer);

            bytesRead = localSocket->mSocket->receiveFrom(source, buffer.get(), OPENPEER_SERVICES_ICESOCKET_RECYCLE_BUFFER_SIZE, &wouldBlock);
            if (0 == bytesRead) return;

            ZS_LOG_TRACE(log("packet received") + ", ip=" + source.string())

          } catch(ISocket::Exceptions::Unspecified &error) {
            ZS_LOG_ERROR(Detail, log("receiveFrom error") + ", error=" + string(error.getErrorCode()))
            cancel();
            return;
          }
        }

        // this method cannot be called within the scope of a lock because it
        // calls a delegate synchronously
        internalReceivedData(viaLocalIP, IICESocket::Type_Local, source, buffer.get(), bytesRead);
      }

      //-----------------------------------------------------------------------
      void ICESocket::onWriteReady(ISocketPtr socket)
      {
        ZS_LOG_TRACE(log("write ready"))
        AutoRecursiveLock lock(mLock);

        SocketMap::iterator found = mSockets.find(socket);
        if (found == mSockets.end()) {
          ZS_LOG_WARNING(Detail, log("UDP socket is not ready"))
          return;
        }

        LocalSocketPtr &localSocket = (*found).second;

        for(ICESocketSessionMap::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter) {
          (*iter).second->forICESocket().notifyLocalWriteReady(localSocket->mLocal.mIPAddress);
        }

        if (localSocket->mTURNSocket) {
          ZS_LOG_TRACE(log("notifying TURN socket of write ready") + ", TURN socket ID=" + string(localSocket->mTURNSocket->getID()))
          localSocket->mTURNSocket->notifyWriteReady();
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onException(ISocketPtr socket)
      {
        ZS_LOG_DETAIL(log("on exception"))
        AutoRecursiveLock lock(mLock);

        {
          SocketMap::iterator found = mSockets.find(socket);
          if (found == mSockets.end()) {
            ZS_LOG_WARNING(Detail, log("notified of exception on socket which is not the bound socket"))
            return;
          }

          LocalSocketPtr &localSocket = (*found).second;

          if (localSocket->mTURNSocket) {
            clearTURN(localSocket->mTURNSocket);
            localSocket->mTURNSocket->shutdown();
            localSocket->mTURNSocket.reset();
          }
          if (localSocket->mSTUNDiscovery) {
            clearSTUN(localSocket->mSTUNDiscovery);
            localSocket->mSTUNDiscovery->cancel();
            localSocket->mSTUNDiscovery.reset();
          }

          localSocket->mSocket->close();
          localSocket->mSocket.reset();

          mSockets.erase(found);
        }

        // attempt to rebind immediately
        get(mRebindCheckNow) = true;
        mRebindAttemptStartTime = zsLib::now();
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket => ITURNSocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocket::onTURNSocketStateChanged(
                                               ITURNSocketPtr socket,
                                               TURNSocketStates state
                                               )
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DEBUG(log("turn socket state changed"))
        step();
      }

      //-----------------------------------------------------------------------
      void ICESocket::handleTURNSocketReceivedPacket(
                                                     ITURNSocketPtr socket,
                                                     IPAddress source,
                                                     const BYTE *packet,
                                                     ULONG packetLengthInBytes
                                                     )
      {
        // WARNING: This method cannot be called within a lock as it calls delegates synchronously.
        IPAddress viaLocalIP;
        {
          AutoRecursiveLock lock(getLock());
          SocketTURNMap::iterator found = mSocketTURNs.find(socket);
          if (found == mSocketTURNs.end()) {
            ZS_LOG_WARNING(Detail, log("TURN not associated with any local socket"))
            return;
          }
          LocalSocketPtr &localSocket = (*found).second;
          viaLocalIP = localSocket->mLocal.mIPAddress;
        }

        internalReceivedData(viaLocalIP, IICESocket::Type_Relayed, source, packet, packetLengthInBytes);
      }

      //-----------------------------------------------------------------------
      bool ICESocket::notifyTURNSocketSendPacket(
                                                 ITURNSocketPtr socket,
                                                 IPAddress destination,
                                                 const BYTE *packet,
                                                 ULONG packetLengthInBytes
                                                 )
      {
        AutoRecursiveLock lock(mLock);

        ZS_LOG_TRACE(log("sending packet for TURN") + ", TURN socket ID=" + string(socket->getID()) + ", destination=" + destination.string() + ", length=" + string(packetLengthInBytes))

        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("unable to send data on behalf of TURN as ICE socket is shutdown") + ", TURN socket ID=" + string(socket->getID()))
          return false;
        }

        SocketTURNMap::iterator found = mSocketTURNs.find(socket);
        if (found == mSocketTURNs.end()) {
          ZS_LOG_WARNING(Debug, log("unable to send data on behalf of TURN as TURN socket does not match current TURN socket (TURN reconnect reattempt?)") + ", socket ID=" + string(socket->getID()))
          return false;
        }
        LocalSocketPtr &localSocket = (*found).second;

        try {
          bool wouldBlock = false;
#ifdef OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
          if (true) return true;
#endif //OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
#ifdef OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          if (!destination.isAddressEqual(IPAddress(OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_SERVER_IP))) {
            return true;
          }
#endif //OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          ULONG bytesSent = localSocket->mSocket->sendTo(destination, packet, packetLengthInBytes, &wouldBlock);
          bool sent = ((!wouldBlock) && (bytesSent == packetLengthInBytes));
          if (!sent) {
            ZS_LOG_WARNING(Debug, log("unable to send data on behalf of TURN as UDP socket did not send the data") + ", would block=" + (wouldBlock ? "true" : "false") + ", bytes sent=" + string(bytesSent))
          }
          return sent;
        } catch(ISocket::Exceptions::Unspecified &error) {
          ZS_LOG_ERROR(Detail, log("sendTo error") + ", error=" + string(error.getErrorCode()))
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void ICESocket::onTURNSocketWriteReady(ITURNSocketPtr socket)
      {
        ZS_LOG_TRACE(log("notified that TURN is write ready") + ", TURN socket ID=" + string(socket->getID()))

        AutoRecursiveLock lock(mLock);

        SocketTURNMap::iterator found = mSocketTURNs.find(socket);
        if (found == mSocketTURNs.end()) {
          ZS_LOG_WARNING(Debug, log("cannot notify socket write ready as TURN socket does not match current TURN socket (TURN reconnect reattempt?)") + ", socket ID=" + string(socket->getID()))
          return;
        }

        LocalSocketPtr &localSocket = (*found).second;

        for(ICESocketSessionMap::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter) {
          (*iter).second->forICESocket().notifyRelayWriteReady(localSocket->mLocal.mIPAddress);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket => ISTUNDiscoveryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocket::onSTUNDiscoverySendPacket(
                                                ISTUNDiscoveryPtr discovery,
                                                IPAddress destination,
                                                boost::shared_array<BYTE> packet,
                                                ULONG packetLengthInBytes
                                                )
      {
        ZS_LOG_TRACE(log("sending packet for STUN discovery") + ", destination=" + destination.string() + ", length=" + string(packetLengthInBytes))

        AutoRecursiveLock lock(mLock);
        if (isShutdown()) {
          ZS_LOG_TRACE(log("cannot send packet as already shutdown"))
          return;
        }

        SocketSTUNMap::iterator found = mSocketSTUNs.find(discovery);
        if (found == mSocketSTUNs.end()) {
          ZS_LOG_WARNING(Debug, log("cannot send STUN packet as STUN socket does not match current STUN socket") + ", socket ID=" + string(discovery->getID()))
          return;
        }

        LocalSocketPtr &localSocket = (*found).second;

        try {
          bool wouldBlock = false;
#ifdef OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
          if (true) return;
#endif //OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_TCP
#ifdef OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          if (!destination.isAddressEqual(IPAddress(OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_SERVER_IP))) {
            return;
          }
#endif //OPENPEER_SERVICES_TURNSOCKET_DEBUGGING_FORCE_USE_TURN_WITH_UDP
          localSocket->mSocket->sendTo(destination, packet.get(), packetLengthInBytes, &wouldBlock);
        } catch(ISocket::Exceptions::Unspecified &error) {
          ZS_LOG_ERROR(Detail, log("sendTo error") + ", error=" + string(error.getErrorCode()))
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::onSTUNDiscoveryCompleted(ISTUNDiscoveryPtr discovery)
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DETAIL(log("notified STUN discovery finished") + ", id=" + string(discovery->getID()) + ", reflected ip=" + discovery->getMappedAddress().string())
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocket::onTimer(TimerPtr timer)
      {
        ZS_LOG_DEBUG(log("on timer"))

        AutoRecursiveLock lock(mLock);
        if (timer != mRebindTimer) {
          ZS_LOG_WARNING(Detail, log("received timer notification on obsolete timer") + ", timer ID=" + string(timer->getID()))
          return;
        }
        get(mRebindCheckNow) = true;

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      String ICESocket::log(const char *message) const
      {
        return String("ICESocket [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ICESocket::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return
        Helper::getDebugValue("ice socket id", string(mID), firstTime) +
        Helper::getDebugValue("graceful shutdown", mGracefulShutdownReference ? String("true") : String(), firstTime) +

        Helper::getDebugValue("subscriptions", mSubscriptions.size() > 0 ? string(mSubscriptions.size()) : String(), firstTime) +
        Helper::getDebugValue("default subscription", mDefaultSubscription ? String("true") : String(), firstTime) +

        Helper::getDebugValue("state", IICESocket::toString(mCurrentState), firstTime) +
        Helper::getDebugValue("last error", 0 != mLastError ? string(mLastError) : String(), firstTime) +
        Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +

        Helper::getDebugValue("foundation", mFoundation ? String("true") : String(), firstTime) +

        Helper::getDebugValue("bind port", 0 != mBindPort ? string(mBindPort) : String(), firstTime) +
        Helper::getDebugValue("usernameFrag", mUsernameFrag, firstTime) +
        Helper::getDebugValue("password", mPassword, firstTime) +

        Helper::getDebugValue("next local preference", 0 != mNextLocalPreference ? string(mNextLocalPreference) : String(), firstTime) +
        Helper::getDebugValue("socket local IPs", mSocketLocalIPs.size() > 0 ? string(mSocketLocalIPs.size()) : String(), firstTime) +
        Helper::getDebugValue("turn sockets", mSocketTURNs.size() > 0 ? string(mSocketTURNs.size()) : String(), firstTime) +
        Helper::getDebugValue("stun sockets", mSocketSTUNs.size() > 0 ? string(mSocketSTUNs.size()) : String(), firstTime) +
        Helper::getDebugValue("sockets", mSockets.size() > 0 ? string(mSockets.size()) : String(), firstTime) +

        Helper::getDebugValue("rebind timer", mRebindTimer ? String("true") : String(), firstTime) +
        Helper::getDebugValue("rebind attempt start time", Time() != mRebindAttemptStartTime ? IHelper::timeToString(mRebindAttemptStartTime) : String(), firstTime) +
        Helper::getDebugValue("rebind check now", mRebindCheckNow ? String("true") : String(), firstTime) +

        Helper::getDebugValue("monitoring write ready", mMonitoringWriteReady ? String("true") : String(), firstTime) +

        Helper::getDebugValue("turn srv udp result", mTURNSRVUDPResult ? String("true") : String(), firstTime) +
        Helper::getDebugValue("turn srv tcp result", mTURNSRVTCPResult ? String("true") : String(), firstTime) +
        Helper::getDebugValue("turn server", mTURNServer, firstTime) +
        Helper::getDebugValue("turn username", mTURNUsername, firstTime) +
        Helper::getDebugValue("turn password", mTURNPassword, firstTime) +
        Helper::getDebugValue("turn first WORD safe", mFirstWORDInAnyPacketWillNotConflictWithTURNChannels ? String("true") : String(), firstTime) +
        Helper::getDebugValue("turn last used", Time() != mTURNLastUsed ? IHelper::timeToString(mTURNLastUsed) : String(), firstTime) +
        Helper::getDebugValue("turn stutdown duration (s)", Duration() != mTURNShutdownIfNotUsedBy ? string(mTURNShutdownIfNotUsedBy.seconds()) : String(), firstTime) +

        Helper::getDebugValue("stun srv udp result", mSTUNSRVResult ? String("true") : String(), firstTime) +
        Helper::getDebugValue("stun server", mSTUNServer, firstTime) +

        Helper::getDebugValue("sessions", mSessions.size() > 0 ? string(mSessions.size()) : String(), firstTime) +

        Helper::getDebugValue("routes", mRoutes.size() > 0 ? string(mRoutes.size()) : String(), firstTime) +

        Helper::getDebugValue("recyle buffers", mRecycledBuffers.size() > 0 ? string(mRecycledBuffers.size()) : String(), firstTime) +

        Helper::getDebugValue("notified candidates changed", mNotifiedCandidateChanged ? String("true") : String(), firstTime) +
        Helper::getDebugValue("candidate crc", 0 != mLastCandidateCRC ? string(mLastCandidateCRC) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void ICESocket::cancel()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already cancelled"))
          return;
        }

        ZS_LOG_DEBUG(log("cancel called"))

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        setState(ICESocketState_ShuttingDown);

        if (mRebindTimer) {
          mRebindTimer->cancel();
          mRebindTimer.reset();
        }

        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); )
        {
          SocketMap::iterator current = iter;
          ++iter;

          LocalSocketPtr &localSocket = (*current).second;

          if (localSocket->mSTUNDiscovery) {
            clearSTUN(localSocket->mSTUNDiscovery);
            localSocket->mSTUNDiscovery->cancel();
            localSocket->mSTUNDiscovery.reset();
          }

          if (localSocket->mTURNSocket) {
            localSocket->mTURNSocket->shutdown();
            if (ITURNSocket::TURNSocketState_Shutdown != localSocket->mTURNSocket->getState()) {
              ZS_LOG_DEBUG(log("turn socket still pending shutdown") + ", base IP=" + string(localSocket->mLocal.mIPAddress) + ", turn socket id=" + string(localSocket->mTURNSocket->getID()))
              continue;
            }
            clearTURN(localSocket->mTURNSocket);
            localSocket->mTURNSocket.reset();
          }

          if (localSocket->mSocket) {
            localSocket->mSocket->close();
            localSocket->mSocket.reset();
          }

          SocketLocalIPMap::iterator found = mSocketLocalIPs.find(localSocket->mLocal.mIPAddress);
          if (found != mSocketLocalIPs.end()) {
            mSocketLocalIPs.erase(found);
          }

          mSockets.erase(current);
        }

        if (mGracefulShutdownReference) {
          if (mSockets.size() > 0) {
            ZS_LOG_DEBUG(log("waiting for sockets to shutdown") + ", total=" + string(mSockets.size()))
            return;
          }
        }

        // inform that the graceful stutdown has completed...
        setState(ICESocketState_Shutdown);

        ZS_LOG_BASIC(log("shutdown"))

        mGracefulShutdownReference.reset();

        mSubscriptions.clear();
        mDefaultSubscription.reset();

        mFoundation.reset();

        if (mSessions.size() > 0) {
          ICESocketSessionMap temp = mSessions;
          mSessions.clear();

          // close down all the ICE sessions immediately
          for(ICESocketSessionMap::iterator iter = temp.begin(); iter != temp.end(); ++iter) {
            (*iter).second->forICESocket().close();
          }
        }

        mRoutes.clear();

        mSTUNSRVResult.reset();
        mSTUNServer.clear();
      }

      //-----------------------------------------------------------------------
      void ICESocket::setState(ICESocketStates state)
      {
        if (mCurrentState == state) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        ICESocketPtr pThis = mThisWeak.lock();

        if (pThis) {
          mSubscriptions.delegate()->onICESocketStateChanged(pThis, mCurrentState);
        }
      }

      //-----------------------------------------------------------------------
      void ICESocket::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());
        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        get(mLastError) = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + ", code=" + string(mLastError) + ", reason=" + mLastErrorReason + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      void ICESocket::step()
      {
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("step redirected to shutdown"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!stepBind()) goto post_candidate_check;
        if (!stepSTUN()) goto post_candidate_check;
        if (!stepTURN()) goto post_candidate_check;

        setState(IICESocket::ICESocketState_Ready);

      post_candidate_check:
        {
          if ((isShuttingDown()) ||
              (isShutdown())) {
            ZS_LOG_DEBUG(log("step shutdown thus redirected to shutdown"))
            cancel();
            return;
          }

          stepCandidates();
        }
      }

      //-----------------------------------------------------------------------
      bool ICESocket::stepBind()
      {
        if (mSockets.size() > 0) {
          if (!mRebindCheckNow) {
            ZS_LOG_DEBUG(log("already bound thus nothing to do"))
            return true;
          }
          ZS_LOG_DEBUG(log("rechecking binding now"))
          get(mRebindCheckNow) = false;
        }

        ZS_LOG_DEBUG(log("step bind") + ", total sockets=" + string(mSockets.size()))

        IPAddressList localIPs;
        if (!getLocalIPs(localIPs)) {
          ZS_LOG_WARNING(Detail, log("failed to obtain any local IPs"))
          return (mSockets.size() > 0);
        }

        bool hadNone = (mSockets.size() < 1);


        for (IPAddressList::iterator iter = localIPs.begin(); iter != localIPs.end(); ++iter)
        {
          IPAddress &ip = (*iter);

          IPAddress bindIP(ip);
          bindIP.setPort(mBindPort);

          SocketLocalIPMap::iterator found = mSocketLocalIPs.find(bindIP);
          if (found != mSocketLocalIPs.end()) {
            ZS_LOG_DEBUG(log("already bound") + ", ip=" + string(ip))
            continue;
          }

          SocketPtr socket;

          ZS_LOG_DEBUG(log("attempting to bind to IP") + ", ip=" + string(bindIP))

          try {
            SocketPtr socket = Socket::createUDP();

            socket->bind(ip);
            socket->setBlocking(false);
            try {
#ifndef __QNX__
              socket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
#endif //ndef __QNX__
            } catch(ISocket::Exceptions::UnsupportedSocketOption &) {
            }

            IPAddress local = socket->getLocalAddress();

            socket->setDelegate(mThisWeak.lock());

            mBindPort = local.getPort();
            bindIP.setPort(mBindPort);
            ZS_THROW_CUSTOM_PROPERTIES_1_IF(ISocket::Exceptions::Unspecified, 0 == mBindPort, 0)
          } catch(ISocket::Exceptions::Unspecified &error) {
            ZS_LOG_ERROR(Detail, log("bind error") + ", error=" + string(error.getErrorCode()))
            socket.reset();
          }

          if (!socket) {
            ZS_LOG_WARNING(Debug, log("bind failure"))
            continue;
          }

          ZS_LOG_DEBUG(log("bind successful"))

          LocalSocketPtr localSocket(new LocalSocket(mNextLocalPreference, mUsernameFrag, mPassword));

          mNextLocalPreference -= 0xF;
          if (mNextLocalPreference > 0xFFFF) {
            ZS_LOG_WARNING(Basic, log("unexpected local preference wrap around"))
            --mNextLocalPreference;
            mNextLocalPreference = (mNextLocalPreference | 0xFFFF);
          }

          localSocket->mSocket = socket;
          localSocket->mLocal.mIPAddress = bindIP;

          String usernameFrag = (mFoundation ? mFoundation->getUsernameFrag() : mUsernameFrag);

          // algorithm to ensure that two candidates with same foundation IP / type end up with same foundation value
          localSocket->mLocal.mFoundation = IHelper::convertToHex(*IHelper::hash("foundation:" + usernameFrag + ":" + string(bindIP) + ":" + toString(localSocket->mLocal.mType), IHelper::HashAlgorthm_MD5));;

          localSocket->mReflexive.mRelatedIP = bindIP;
          localSocket->mRelay.mRelatedIP = bindIP;

          mSocketLocalIPs[bindIP] = localSocket;
          mSockets[socket] = localSocket;
        }

        if ((hadNone) &&
            (mSockets.size() > 0)) {
          mRebindTimer->cancel();
          mRebindTimer.reset();
        }

        if (!mRebindTimer) {
          mRebindTimer = Timer::create(mThisWeak.lock(), Seconds(mSockets.size() > 0 ? OPENPEER_SERVICES_REBIND_TIMER_WHEN_HAS_SOCKETS_IN_SECONDS : OPENPEER_SERVICES_REBIND_TIMER_WHEN_NO_SOCKETS_IN_SECONDS));
        }

        if (!mMonitoringWriteReady) {
          monitorWriteReadyOnAllSessions(false);
        }

        if (mSockets.size() < 1) {

          Time tick = zsLib::now();

          if (mRebindAttemptStartTime + Seconds(OPENPEER_SERVICES_MAX_REBIND_ATTEMPT_DURATION_IN_SECONDS) < tick) {
            ZS_LOG_ERROR(Detail, log("unable to bind IP thus cancelling") + ", bind port=" + string(mBindPort))
            setError(IHTTP::HTTPStatusCode_RequestTimeout, "unable to bind to local UDP port");
            cancel();
            return false;
          }

          ZS_LOG_WARNING(Detail, log("unable to bind to local UDP port but will try again") + string(mBindPort))
        }

        ZS_LOG_DEBUG(log("UDP port is bound"))
        return true;
      }

      //-----------------------------------------------------------------------
      bool ICESocket::stepSTUN()
      {
        ZS_LOG_DEBUG(log("step STUN"))

        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); ++iter)
        {
          LocalSocketPtr &localSocket = (*iter).second;

          if (!localSocket->mSTUNDiscovery) {
            localSocket->mReflexive.mIPAddress.clear();
            localSocket->mReflexive.mFoundation.clear();

            ZS_LOG_DEBUG(log("performing STUN discovery using IP") + ", base IP=" + string(localSocket->mLocal.mIPAddress))
            if (mSTUNSRVResult) {
              localSocket->mSTUNDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), mSTUNSRVResult);
            }
            if (!mSTUNServer.isEmpty()) {
              localSocket->mSTUNDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), mSTUNServer);
            }
            mSocketSTUNs[localSocket->mSTUNDiscovery] = localSocket;
          }

          if (!localSocket->mSTUNDiscovery) {
            ZS_LOG_DEBUG(log("unable to perform STUN discovery for IP") + ", base IP=" + string(localSocket->mLocal.mIPAddress))
            continue;
          }

          if (!localSocket->mSTUNDiscovery->isComplete()) {
            ZS_LOG_TRACE(log("stun discovery not complete yet") + ", base IP=" + string(localSocket->mLocal.mIPAddress))
            continue;
          }

          if (!localSocket->mReflexive.mIPAddress.isAddressEmpty()) {
            ZS_LOG_TRACE(log("stun discovery already complete yet") + ", base IP=" + string(localSocket->mLocal.mIPAddress) + ", previously discovered=" + string(localSocket->mReflexive.mIPAddress))
            continue;
          }

          localSocket->mReflexive.mIPAddress = localSocket->mSTUNDiscovery->getMappedAddress();
          String usernameFrag = (mFoundation ? mFoundation->getUsernameFrag() : mUsernameFrag);
          localSocket->mReflexive.mFoundation = IHelper::convertToHex(*IHelper::hash("foundation:" + usernameFrag + ":" + string(localSocket->mReflexive.mIPAddress) + ":" + toString(localSocket->mReflexive.mType), IHelper::HashAlgorthm_MD5));;

          ZS_LOG_DEBUG(log("stun discovery complete") + ", base IP=" + string(localSocket->mLocal.mIPAddress) + ", discovered=" + string(localSocket->mReflexive.mIPAddress))
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool ICESocket::stepTURN()
      {
        Time tick = zsLib::now();

        bool shouldSleep = false;

        if (mTURNLastUsed + mTURNShutdownIfNotUsedBy < tick)
        {
          // the socket can be put to sleep...
          mTURNShutdownIfNotUsedBy = Seconds(OPENPEER_SERVICES_ICESOCKET_MINIMUM_TURN_KEEP_ALIVE_TIME_IN_SECONDS);  // reset to minimum again...
          shouldSleep = true;
        }

        ZS_LOG_DEBUG(log("step TURN") + ", should sleep=" + (shouldSleep ? "true" : "false"))

        bool allConnected = true;
        bool allSleeping = true;

        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); ++iter)
        {
          LocalSocketPtr &localSocket = (*iter).second;

          if (shouldSleep) {
            if (!localSocket->mTURNSocket) {
              ZS_LOG_TRACE(log("no TURN server present for IP") + ", base IP=" + string(localSocket->mLocal.mIPAddress))
              continue;
            }
            localSocket->mTURNSocket->shutdown();
            if (ITURNSocket::TURNSocketState_ShuttingDown == localSocket->mTURNSocket->getState()) {
              ZS_LOG_TRACE(log("TURN still shutting down") + ", base IP=" + string(localSocket->mLocal.mIPAddress))
              allSleeping = false;
              continue;
            }
            ZS_LOG_DEBUG(log("TURN shutting down") + ", base IP=" + string(localSocket->mLocal.mIPAddress))

            clearTURN(localSocket->mTURNSocket);
            localSocket->mTURNSocket.reset();
          } else {
            if (localSocket->mTURNSocket) {
              ITURNSocket::TURNSocketStates state = localSocket->mTURNSocket->getState();
              switch (state) {
                case ITURNSocket::TURNSocketState_Pending:    {
                  allConnected = false;
                  break;
                }
                case ITURNSocket::TURNSocketState_Ready:      {
                  if (localSocket->mRelay.mIPAddress.isAddressEmpty()) {
                    localSocket->mRelay.mIPAddress = localSocket->mTURNSocket->getRelayedIP();

                    String usernameFrag = (mFoundation ? mFoundation->getUsernameFrag() : mUsernameFrag);
                    localSocket->mRelay.mFoundation = IHelper::convertToHex(*IHelper::hash("foundation:" + usernameFrag + ":" + string(localSocket->mRelay.mIPAddress) + ":" + toString(localSocket->mRelay.mType), IHelper::HashAlgorthm_MD5));;

                    ZS_LOG_DEBUG(log("TURN relay ready") + ", base IP=" + string(localSocket->mLocal.mIPAddress) + ", discovered=" + string(localSocket->mRelay.mIPAddress))
                  }
                  break;
                }
                case ITURNSocket::TURNSocketState_ShuttingDown: {
                  allConnected = false;
                  break;
                }
                case ITURNSocket::TURNSocketState_Shutdown:   {
                  allConnected = false;
                  clearTURN(localSocket->mTURNSocket);
                  localSocket->mTURNSocket.reset();
                  break;
                }
              }
            }

            if (!localSocket->mTURNSocket) {
              if (localSocket->mSTUNDiscovery) {
                if (localSocket->mSTUNDiscovery->getMappedAddress().isEmpty()) {
                  ZS_LOG_TRACE(log("cannot create TURN as STUN discovery not complete") + ", base IP=" + string(localSocket->mLocal.mIPAddress))
                  allConnected = false;
                  continue;
                }
              }

              bool foundDuplicate = false;

              // check to see if TURN should be created (must not be another socket with TURN with the same reflexive address)
              for (SocketMap::iterator checkIter = mSockets.begin(); checkIter != mSockets.end(); ++checkIter)
              {
                LocalSocketPtr &checkSocket = (*checkIter).second;
                if (checkSocket == localSocket) {
                  ZS_LOG_TRACE(log("turn check - no need to compare against same socket"))
                  continue;
                }
                if (checkSocket->mReflexive.mIPAddress != localSocket->mReflexive.mIPAddress) {
                  ZS_LOG_TRACE(log("turn check - mapped address does not match thus still allowed to create TURN"))
                  continue;
                }
                if (!checkSocket->mTURNSocket) {
                  ZS_LOG_TRACE(log("turn check - TURN socket does not exist on duplication reflexive socket thus safe to create TURN"))
                  continue;
                }

                ZS_LOG_TRACE(log("turn check - TURN socket already exists on duplication reflexive socket thus not safe to create TURN"))
                foundDuplicate = true;
                break;
              }

              if (foundDuplicate) continue;
            }

            if (!localSocket->mTURNSocket) {
              localSocket->mRelay.mIPAddress.clear();
              localSocket->mRelay.mFoundation.clear();

              allConnected = false;

              if (!mTURNServer.isEmpty()) {
                localSocket->mTURNSocket = ITURNSocket::create(
                                                               getAssociatedMessageQueue(),
                                                               mThisWeak.lock(),
                                                               mTURNServer,
                                                               mTURNUsername,
                                                               mTURNPassword,
                                                               mFirstWORDInAnyPacketWillNotConflictWithTURNChannels
                                                               );
              } else {
                localSocket->mTURNSocket = ITURNSocket::create(
                                                               getAssociatedMessageQueue(),
                                                               mThisWeak.lock(),
                                                               mTURNSRVUDPResult,
                                                               mTURNSRVTCPResult,
                                                               mTURNUsername,
                                                               mTURNPassword,
                                                               mFirstWORDInAnyPacketWillNotConflictWithTURNChannels
                                                               );
              }

              mSocketTURNs[localSocket->mTURNSocket] = localSocket;

              ZS_LOG_DEBUG(log("TURN socket created") + ", base IP=" + string(localSocket->mLocal.mIPAddress) + ", TURN socket ID=" + string(localSocket->mTURNSocket->getID()))
            }
          }
        }

        if (shouldSleep) {
          if (allSleeping) {
            setState(IICESocket::ICESocketState_Sleeping);
          } else {
            setState(IICESocket::ICESocketState_GoingToSleep);
          }
          return false;
        }

        return allConnected;
      }

      //-----------------------------------------------------------------------
      bool ICESocket::stepCandidates()
      {
        DWORD crcValue = 0;

        CRC32 crc;
        for (SocketMap::iterator iter = mSockets.begin(); iter != mSockets.end(); ++iter)
        {
          LocalSocketPtr &localSocket = (*iter).second;

          if (!localSocket->mLocal.mIPAddress.isEmpty()) {
            crc.Update((const BYTE *)(":local:"), strlen(":local:"));

            IPv6PortPair &portPair = localSocket->mLocal.mIPAddress;
            crc.Update((const BYTE *)(&portPair), sizeof(IPv6PortPair));
          }

          if (!localSocket->mReflexive.mIPAddress.isEmpty()) {
            crc.Update((const BYTE *)(":reflexive:"), strlen(":reflexive:"));

            IPv6PortPair &portPair = localSocket->mReflexive.mIPAddress;
            crc.Update((const BYTE *)(&portPair), sizeof(IPv6PortPair));
          }
          if (!localSocket->mRelay.mIPAddress.isEmpty()) {
            crc.Update((const BYTE *)(":relay:"), strlen(":relay:"));

            IPv6PortPair &portPair = localSocket->mRelay.mIPAddress;
            crc.Update((const BYTE *)(&portPair), sizeof(IPv6PortPair));
          }
        }
        crc.Final((BYTE *)(&crcValue));

        if (mLastCandidateCRC == crcValue) {
          ZS_LOG_DEBUG(log("candidate list has not changed"))
          return true;
        }

        mLastCandidateCRC = crcValue;

        mSubscriptions.delegate()->onICESocketCandidatesChanged(mThisWeak.lock());
        get(mNotifiedCandidateChanged) = true;
        return true;
      }

      //-----------------------------------------------------------------------
      bool ICESocket::getLocalIPs(IPAddressList &outIPs)
      {
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

              outIPs.push_back(ip);
            }
          }
        }
        ZS_LOG_DEBUG(log("--- GATHERING LOCAL IPs: END ---"))
#elif _ANDROID
        int fd;
        struct ifreq ifr;

        fd = socket(AF_INET, SOCK_DGRAM, 0);

        /* I want to get an IPv4 IP address */
        ifr.ifr_addr.sa_family = AF_INET;

        /* I want IP address attached to "eth0" */
        strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

        ioctl(fd, SIOCGIFADDR, &ifr);

        close(fd);
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

          outIPs.push_back(ip);
        }
        ZS_LOG_DEBUG(log("--- GATHERING LOCAL IPs: END ---"))

        if (ifAddrStruct) {
          freeifaddrs(ifAddrStruct);
          ifAddrStruct = NULL;
        }
#endif //_WIN32

        outIPs.sort(compareLocalIPs);

        if (outIPs.empty()) {
          ZS_LOG_DEBUG(log("failed to read any local IPs"))
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void ICESocket::clearTURN(ITURNSocketPtr turn)
      {
        if (!turn) return;
        SocketTURNMap::iterator found = mSocketTURNs.find(turn);
        if (found == mSocketTURNs.end()) return;

        mSocketTURNs.erase(found);
      }

      //-----------------------------------------------------------------------
      void ICESocket::clearSTUN(ISTUNDiscoveryPtr stun)
      {
        if (!stun) return;
        SocketSTUNMap::iterator found = mSocketSTUNs.find(stun);
        if (found == mSocketSTUNs.end()) return;

        mSocketSTUNs.erase(found);
      }

      //-----------------------------------------------------------------------
      void ICESocket::internalReceivedData(
                                           const IPAddress &viaLocalIP,
                                           IICESocket::Types viaTransport,
                                           const IPAddress &source,
                                           const BYTE *buffer,
                                           ULONG bufferLengthInBytes
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
              SocketLocalIPMap::iterator found = mSocketLocalIPs.find(viaLocalIP);
              if (found != mSocketLocalIPs.end()) {
                LocalSocketPtr &localSocket = (*found).second;
                turn = localSocket->mTURNSocket;
              }
            }

            if (turn) {
              if (turn->handleSTUNPacket(source, stun)) return;
            }
          }

          if (!turn) {
            // if TURN was used, we would already called this routine... (i.e. prevent double lookup)
            if (ISTUNRequesterManager::handleSTUNPacket(source, stun)) return;
          }

          ICESocketSessionPtr next;

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
                ICESocketSessionMap::iterator iter = mSessions.find(next->forICESocket().getID());
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
            if (next->forICESocket().handleSTUNPacket(viaLocalIP, viaTransport, source, stun, localUsernameFrag, remoteUsernameFrag)) return;
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
            SocketLocalIPMap::iterator found = mSocketLocalIPs.find(viaLocalIP);
            if (found != mSocketLocalIPs.end()) {
              LocalSocketPtr &localSocket = (*found).second;
              turn = localSocket->mTURNSocket;
            }
          }

          if (turn) {
            if (turn->handleChannelData(source, buffer, bufferLengthInBytes)) return;
          }
        }

        ICESocketSessionPtr next;

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
          if (next->forICESocket().handlePacket(viaLocalIP, viaTransport, source, buffer, bufferLengthInBytes)) return;

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
              ICESocketSessionMap::iterator iter = mSessions.find(next->forICESocket().getID());
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
          if (next->forICESocket().handlePacket(viaLocalIP, viaTransport, source, buffer, bufferLengthInBytes)) return;
        }

        ZS_LOG_WARNING(Trace, log("did not find any socket session to handle data packet"))
      }

      //-----------------------------------------------------------------------
      void ICESocket::getBuffer(RecycledPacketBuffer &outBuffer)
      {
        AutoRecursiveLock lock(mLock);
        if (mRecycledBuffers.size() < 1) {
          outBuffer = RecycledPacketBuffer(new BYTE[OPENPEER_SERVICES_ICESOCKET_RECYCLE_BUFFER_SIZE]);
          return;
        }

        outBuffer = mRecycledBuffers.front();
        mRecycledBuffers.pop_front();
      }

      //-----------------------------------------------------------------------
      void ICESocket::recycleBuffer(RecycledPacketBuffer &buffer)
      {
        AutoRecursiveLock lock(mLock);
        if (!buffer) return;

        if (mRecycledBuffers.size() >= OPENPEER_SERVICES_ICESOCKET_MAX_RECYLCE_BUFFERS) {
          buffer.reset();
          return;
        }
        mRecycledBuffers.push_back(buffer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket::LocalSocket
      #pragma mark

      //-----------------------------------------------------------------------
      ICESocket::LocalSocket::LocalSocket(
                                          ULONG nextLocalPreference,
                                          const String &usernameFrag,
                                          const String &password
                                          )
      {
        mLocal.mLocalPreference = nextLocalPreference;
        mLocal.mType = ICESocket::Type_Local;
        mLocal.mPriority = ((1 << 24)*(static_cast<DWORD>(mLocal.mType))) + ((1 << 8)*(static_cast<DWORD>(mLocal.mLocalPreference))) + (256 - 0);

        mReflexive.mLocalPreference = nextLocalPreference;
        mReflexive.mType = ICESocket::Type_ServerReflexive;
        mReflexive.mPriority = ((1 << 24)*(static_cast<DWORD>(mReflexive.mType))) + ((1 << 8)*(static_cast<DWORD>(mReflexive.mLocalPreference))) + (256 - 0);

        mRelay.mLocalPreference = nextLocalPreference;
        mRelay.mType = ICESocket::Type_Relayed;
        mRelay.mPriority = ((1 << 24)*(static_cast<DWORD>(mRelay.mType))) + ((1 << 8)*(static_cast<DWORD>(mRelay.mLocalPreference))) + (256 - 0);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IICESocket
    #pragma mark

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
    void IICESocket::compare(
                             const CandidateList &inOldCandidatesList,
                             const CandidateList &inNewCandidatesList,
                             CandidateList &outAddedCandidates,
                             CandidateList &outRemovedCandidates
                             )
    {
      outAddedCandidates.clear();
      outRemovedCandidates.clear();

      // check new list to see which candidates are not part of the old list
      for (CandidateList::const_iterator outerIter = inNewCandidatesList.begin(); outerIter != inNewCandidatesList.end(); ++outerIter)
      {
        const Candidate &newCandidate = (*outerIter);

        bool found = false;

        for (CandidateList::const_iterator innerIter = inOldCandidatesList.begin(); innerIter != inOldCandidatesList.end(); ++innerIter)
        {
          const Candidate &oldCandidate = (*outerIter);

          if (!internal::compare(newCandidate, oldCandidate)) continue;

          found = true;
          break;
        }

        if (!found) {
          outAddedCandidates.push_back(newCandidate);
        }
      }

      // check old list to see which candidates are not part of the new list
      for (CandidateList::const_iterator outerIter = inOldCandidatesList.begin(); outerIter != inOldCandidatesList.end(); ++outerIter)
      {
        const Candidate &oldCandidate = (*outerIter);

        bool found = false;

        for (CandidateList::const_iterator innerIter = inNewCandidatesList.begin(); innerIter != inNewCandidatesList.end(); ++innerIter)
        {
          const Candidate &newCandidate = (*outerIter);

          if (!internal::compare(newCandidate, oldCandidate)) continue;

          found = true;
          break;
        }

        if (!found) {
          outRemovedCandidates.push_back(oldCandidate);
        }
      }
    }

    //-------------------------------------------------------------------------
    String IICESocket::toDebugString(IICESocketPtr socket, bool includeCommaPrefix)
    {
      return internal::ICESocket::toDebugString(socket, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IICESocketPtr IICESocket::create(
                                     IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     const char *turnServer,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     const char *stunServer,
                                     WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                     IICESocketPtr foundationSocket
                                     )
    {
      return internal::IICESocketFactory::singleton().create(
                                                             queue,
                                                             delegate,
                                                             turnServer,
                                                             turnServerUsername,
                                                             turnServerPassword,
                                                             stunServer,
                                                             port,
                                                             firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                                             foundationSocket);
    }

    //-------------------------------------------------------------------------
    IICESocketPtr IICESocket::create(
                                     IMessageQueuePtr queue,
                                     IICESocketDelegatePtr delegate,
                                     IDNS::SRVResultPtr srvTURNUDP,
                                     IDNS::SRVResultPtr srvTURNTCP,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     IDNS::SRVResultPtr srvSTUN,
                                     WORD port,
                                     bool firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                     IICESocketPtr foundationSocket
                                     )
    {
      return internal::IICESocketFactory::singleton().create(
                                                             queue,
                                                             delegate,
                                                             srvTURNUDP,
                                                             srvTURNTCP,
                                                             turnServerUsername,
                                                             turnServerPassword,
                                                             srvSTUN,
                                                             port,
                                                             firstWORDInAnyPacketWillNotConflictWithTURNChannels,
                                                             foundationSocket);
    }

    //-------------------------------------------------------------------------
    String IICESocket::Candidate::toDebugString(bool includeCommaPrefix) const
    {
      bool firstTime = !includeCommaPrefix;
      return internal::Helper::getDebugValue("type", IICESocket::toString(mType), firstTime) +
             internal::Helper::getDebugValue("foundation", mFoundation, firstTime) +
             internal::Helper::getDebugValue("ip", mIPAddress.string(), firstTime) +
             internal::Helper::getDebugValue("priority", 0 != mPriority ? string(mPriority) : String(), firstTime) +
             internal::Helper::getDebugValue("preference", 0 != mLocalPreference ? string(mLocalPreference) : String(), firstTime) +
             internal::Helper::getDebugValue("usernameFrag", mUsernameFrag, firstTime) +
             internal::Helper::getDebugValue("password", mPassword, firstTime) +
             internal::Helper::getDebugValue("protocol", mProtocol, firstTime);
    }
  }
}
