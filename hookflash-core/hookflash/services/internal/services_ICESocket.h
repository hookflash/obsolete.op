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

#include <hookflash/services/internal/types.h>
#include <hookflash/services/IICESocket.h>
#include <hookflash/services/IDNS.h>
#include <hookflash/services/ITURNSocket.h>
#include <hookflash/services/ISTUNDiscovery.h>
#include <zsLib/types.h>
#include <zsLib/IPAddress.h>
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Socket.h>
#include <zsLib/XML.h>
#include <zsLib/Timer.h>

#include <list>

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      interaction IICESocketForICESocketSession
      {
        virtual IICESocketPtr getSocket() const = 0;

        virtual RecursiveLock &getLock() const = 0;

        virtual bool sendTo(
                            IICESocket::Types viaTransport,
                            const IPAddress &destination,
                            const BYTE *buffer,
                            ULONG bufferLengthInBytes,
                            bool isUserData
                            ) = 0;

        virtual void addRoute(IICESocketSessionForICESocketPtr session, const IPAddress &source) = 0;
        virtual void removeRoute(IICESocketSessionForICESocketPtr session) = 0;

        virtual void onICESocketSessionClosed(PUID sessionID) = 0;
      };

      class ICESocket : public MessageQueueAssociator,
                        public IICESocket,
                        public ISocketDelegate,
                        public ITURNSocketDelegate,
                        public ISTUNDiscoveryDelegate,
                        public IICESocketForICESocketSession,
                        public ITimerDelegate
      {
        typedef boost::shared_array<BYTE> RecycledPacketBuffer;
        typedef std::list<RecycledPacketBuffer> RecycledPacketBufferList;

      protected:
        class Subscription;
        typedef boost::shared_ptr<Subscription> SubscriptionPtr;
        typedef boost::weak_ptr<Subscription> SubscriptionWeakPtr;

        friend class Subscription;

      protected:
        ICESocket(
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
                  WORD port
                  );

        void init();

        static ICESocketPtr convert(IICESocketPtr socket);

      public:
        ~ICESocket();

        // IICESocket
        static ICESocketPtr create(
                                   IMessageQueuePtr queue,
                                   IICESocketDelegatePtr delegate,
                                   const char *turnServer,
                                   const char *turnServerUsername,
                                   const char *turnServerPassword,
                                   const char *stunServer,
                                   WORD port = 0,
                                   bool firstWORDInAnyPacketWillNotConflictWithTURNChannels = false
                                   );

        static ICESocketPtr create(
                                   IMessageQueuePtr queue,
                                   IICESocketDelegatePtr delegate,
                                   IDNS::SRVResultPtr srvTURNUDP,
                                   IDNS::SRVResultPtr srvTURNTCP,
                                   const char *turnServerUsername,
                                   const char *turnServerPassword,
                                   IDNS::SRVResultPtr srvSTUN,
                                   WORD port = 0,
                                   bool firstWORDInAnyPacketWillNotConflictWithTURNChannels = false
                                   );

        virtual PUID getID() const {return mID;}

        virtual ICESocketStates getState() const;

        virtual IICESocketSubscriptionPtr subscribe(IICESocketDelegatePtr delegate);

        virtual void shutdown();

        virtual void wakeup(Duration minimumTimeCandidatesMustRemainValidWhileNotUsed = Seconds(60*10));

        virtual void setFoundation(IICESocketPtr foundationSocket);
        virtual void getLocalCandidates(CandidateList &outCandidates);

        virtual IICESocketSessionPtr createSessionFromRemoteCandidates(
                                                                       IICESocketSessionDelegatePtr delegate,
                                                                       const CandidateList &remoteCandidates,
                                                                       ICEControls control
                                                                       );

        virtual void monitorWriteReadyOnAllSessions(bool monitor = true);

        // ISocketDelegate
        virtual void onReadReady(ISocketPtr socket);
        virtual void onWriteReady(ISocketPtr socket);
        virtual void onException(ISocketPtr socket);

        // ITURNSocketDelegate
        virtual void onTURNSocketStateChanged(
                                              ITURNSocketPtr socket,
                                              TURNSocketStates state
                                              );

        virtual void handleTURNSocketReceivedPacket(
                                                    ITURNSocketPtr socket,
                                                    IPAddress source,
                                                    const BYTE *packet,
                                                    ULONG packetLengthInBytes
                                                    );

        virtual bool notifyTURNSocketSendPacket(
                                                ITURNSocketPtr socket,
                                                IPAddress destination,
                                                const BYTE *packet,
                                                ULONG packetLengthInBytes
                                                );

        virtual void onTURNSocketWriteReady(ITURNSocketPtr socket);

        // ISTUNDiscoveryDelegate
        virtual void onSTUNDiscoverySendPacket(
                                               ISTUNDiscoveryPtr discovery,
                                               IPAddress destination,
                                               boost::shared_array<BYTE> packet,
                                               ULONG packetLengthInBytes
                                               );

        virtual void onSTUNDiscoveryCompleted(ISTUNDiscoveryPtr discovery);

        //IICESocketForICESocketSession
        virtual IICESocketPtr getSocket() const {return mThisWeak.lock();}
        virtual RecursiveLock &getLock() const {return mLock;}

        virtual bool sendTo(
                            IICESocket::Types viaTransport,
                            const IPAddress &destination,
                            const BYTE *buffer,
                            ULONG bufferLengthInBytes,
                            bool isUserData
                            );

        virtual void addRoute(IICESocketSessionForICESocketPtr session, const IPAddress &source);
        virtual void removeRoute(IICESocketSessionForICESocketPtr session);

        virtual void onICESocketSessionClosed(PUID sessionID);

        // ITimerDelegate
        virtual void onTimer(TimerPtr timer);

      protected:
        typedef std::list<IPAddress> IPAddressList;

        // helpers for friend classes
        void getLocalIPs(IPAddressList &outList);
        IPAddress getReflectedIP();
        IPAddress getRelayedIP();

        //---------------------------------------------------------------------
        // NOTE:  Do NOT call this method while in a lock because it must
        //        deliver data to delegates synchronously.
        void internalReceivedData(
                                  IICESocket::Types viaTransport,
                                  const IPAddress &source,
                                  const BYTE *buffer,
                                  ULONG bufferLengthInBytes
                                  );

      protected:
        void cancelSubscription(PUID subscriptionID);

      protected:
        String log(const char *message) const;

        bool isShuttingDown() const {return ICESocketState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return ICESocketState_Shutdown == mCurrentState;}

        void cancel();
        void step();

        void setState(ICESocketStates state);

        bool gatherLocalIPs();

        bool bindUDP();

        bool hasReflectedCandidate() {return clearReflectedCandidates(true);}
        bool hasRelayedCandidate() {return clearRelayedCandidates(true);}

        bool clearReflectedCandidates(bool checkOnly = false);
        bool clearRelayedCandidates(bool checkOnly = false);
        void makeCandidates();

        void getBuffer(RecycledPacketBuffer &outBuffer);
        void recycleBuffer(RecycledPacketBuffer &buffer);

        class Subscription : public IICESocketSubscription
        {
          Subscription(ICESocketPtr outer);

        public:
          ~Subscription();

          static SubscriptionPtr create(ICESocketPtr outer);

          virtual void cancel();

          PUID mID;
          ICESocketWeakPtr mOuter;
        };

        class AutoRecycleBuffer
        {
        public:
          AutoRecycleBuffer(ICESocket &outer, RecycledPacketBuffer &buffer) : mOuter(outer), mBuffer(buffer) {}
          ~AutoRecycleBuffer() {mOuter.recycleBuffer(mBuffer);}
        private:
          ICESocket &mOuter;
          RecycledPacketBuffer &mBuffer;
        };

      protected:
        mutable RecursiveLock mLock;
        ICESocketWeakPtr mThisWeak;
        ICESocketPtr mGracefulShutdownReference;

        PUID mID;
        ICESocketStates mCurrentState;

        ICESocketPtr mFoundation;

        typedef std::map<PUID, IICESocketDelegatePtr> DelegateMap;
        DelegateMap mDelegates;

        WORD         mBindPort;
        SocketPtr    mUDPSocket;

        TimerPtr mRebindTimer;
        Time mRebindAttemptStartTime;
        Duration mMaxRebindAttemptDuration;

        bool mMonitoringWriteReady;

        ITURNSocketPtr      mTURNSocket;
        IDNS::SRVResultPtr  mTURNSRVUDPResult;
        IDNS::SRVResultPtr  mTURNSRVTCPResult;
        String              mTURNServer;
        String              mTURNUsername;
        String              mTURNPassword;
        bool                mFirstWORDInAnyPacketWillNotConflictWithTURNChannels;
        Time                mTURNLastUsed;                    // when was the TURN server last used to transport any data
        Duration            mTURNShutdownIfNotUsedBy;         // when will TURN be shutdown if it is not used by this time

        IDNS::SRVResultPtr  mSTUNSRVResult;
        String              mSTUNServer;
        ISTUNDiscoveryPtr   mSTUNDiscovery;

        IPAddressList       mLocalIPs;
        CandidateList       mLocalCandidates;

        String              mUsernameFrag;
        String              mPassword;

        typedef std::map<PUID, IICESocketSessionForICESocketPtr> ICESocketSessionMap;
        ICESocketSessionMap mSessions;

        typedef std::map<IPAddress, IICESocketSessionForICESocketPtr> QuickRouteMap;
        QuickRouteMap mRoutes;

        RecycledPacketBufferList mRecycledBuffers;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::internal::IICESocketForICESocketSession)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getSocket, hookflash::services::IICESocketPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getLock, RecursiveLock &)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_5(sendTo, bool, hookflash::services::IICESocket::Types, const IPAddress &, const BYTE *, ULONG, bool)
ZS_DECLARE_PROXY_METHOD_1(onICESocketSessionClosed, PUID)
ZS_DECLARE_PROXY_METHOD_SYNC_2(addRoute, hookflash::services::internal::IICESocketSessionForICESocketPtr, const IPAddress &)
ZS_DECLARE_PROXY_METHOD_SYNC_1(removeRoute, hookflash::services::internal::IICESocketSessionForICESocketPtr)
ZS_DECLARE_PROXY_END()
