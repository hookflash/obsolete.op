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

#include <openpeer/services/internal/types.h>
#include <openpeer/services/IICESocket.h>
#include <openpeer/services/IDNS.h>
#include <openpeer/services/ITURNSocket.h>
#include <openpeer/services/ISTUNDiscovery.h>
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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketForICESocketSession
      #pragma mark

      interaction IICESocketForICESocketSession
      {
        IICESocketForICESocketSession &forICESocketSession() {return *this;}
        const IICESocketForICESocketSession &forICESocketSession() const {return *this;}

        virtual IICESocketPtr getSocket() const = 0;

        virtual RecursiveLock &getLock() const = 0;

        virtual bool sendTo(
                            IICESocket::Types viaTransport,
                            const IPAddress &destination,
                            const BYTE *buffer,
                            ULONG bufferLengthInBytes,
                            bool isUserData
                            ) = 0;

        virtual void addRoute(ICESocketSessionPtr session, const IPAddress &source) = 0;
        virtual void removeRoute(ICESocketSessionPtr session) = 0;

        virtual void onICESocketSessionClosed(PUID sessionID) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocket
      #pragma mark

      class ICESocket : public Noop,
                        public MessageQueueAssociator,
                        public IICESocket,
                        public ISocketDelegate,
                        public ITURNSocketDelegate,
                        public ISTUNDiscoveryDelegate,
                        public IICESocketForICESocketSession,
                        public ITimerDelegate
      {
      public:
        friend interaction IICESocketFactory;

        class Subscription;
        typedef boost::shared_ptr<Subscription> SubscriptionPtr;
        typedef boost::weak_ptr<Subscription> SubscriptionWeakPtr;

        friend class Subscription;

        typedef boost::shared_array<BYTE> RecycledPacketBuffer;
        typedef std::list<RecycledPacketBuffer> RecycledPacketBufferList;

        typedef std::list<IPAddress> IPAddressList;

        typedef std::map<PUID, IICESocketDelegatePtr> DelegateMap;

        typedef std::map<PUID, ICESocketSessionPtr> ICESocketSessionMap;

        typedef std::map<IPAddress, ICESocketSessionPtr> QuickRouteMap;

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
        ICESocket(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {}

        void init();

      public:
        ~ICESocket();

        static ICESocketPtr convert(IICESocketPtr socket);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => IICESocket
        #pragma mark

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

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => IICESocketForICESocketSession
        #pragma mark

        virtual IICESocketPtr getSocket() const {return mThisWeak.lock();}
        virtual RecursiveLock &getLock() const {return mLock;}

        virtual bool sendTo(
                            IICESocket::Types viaTransport,
                            const IPAddress &destination,
                            const BYTE *buffer,
                            ULONG bufferLengthInBytes,
                            bool isUserData
                            );

        virtual void addRoute(ICESocketSessionPtr session, const IPAddress &source);
        virtual void removeRoute(ICESocketSessionPtr session);

        virtual void onICESocketSessionClosed(PUID sessionID);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => ISocketDelegate
        #pragma mark

        virtual void onReadReady(ISocketPtr socket);
        virtual void onWriteReady(ISocketPtr socket);
        virtual void onException(ISocketPtr socket);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => ITURNSocketDelegate
        #pragma mark

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

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => ISTUNDiscoveryDelegate
        #pragma mark

        virtual void onSTUNDiscoverySendPacket(
                                               ISTUNDiscoveryPtr discovery,
                                               IPAddress destination,
                                               boost::shared_array<BYTE> packet,
                                               ULONG packetLengthInBytes
                                               );

        virtual void onSTUNDiscoveryCompleted(ISTUNDiscoveryPtr discovery);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => friend Subscription
        #pragma mark

        void cancelSubscription(Subscription &subscription);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket => (internal)
        #pragma mark

        String log(const char *message) const;

        bool isShuttingDown() const {return ICESocketState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return ICESocketState_Shutdown == mCurrentState;}

        void cancel();
        void step();

        void setState(ICESocketStates state);

        bool bindUDP();

        bool gatherLocalIPs();

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

        bool hasReflectedCandidate() {return clearReflectedCandidates(true);}
        bool hasRelayedCandidate() {return clearRelayedCandidates(true);}

        bool clearReflectedCandidates(bool checkOnly = false);
        bool clearRelayedCandidates(bool checkOnly = false);
        void makeCandidates();

        void getBuffer(RecycledPacketBuffer &outBuffer);
        void recycleBuffer(RecycledPacketBuffer &buffer);

      public:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket::Subscription
        #pragma mark

        class Subscription : public IICESocketSubscription
        {
          Subscription(ICESocketPtr outer);

        public:
          ~Subscription();

          static SubscriptionPtr create(ICESocketPtr outer);

          virtual PUID getID() const {return mID;}

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
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocket (internal)
        #pragma mark

        mutable RecursiveLock mLock;
        ICESocketWeakPtr mThisWeak;
        ICESocketPtr mGracefulShutdownReference;

        PUID mID;
        ICESocketStates mCurrentState;

        ICESocketPtr mFoundation;

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

        ICESocketSessionMap mSessions;

        QuickRouteMap mRoutes;

        RecycledPacketBufferList mRecycledBuffers;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketFactory
      #pragma mark

      interaction IICESocketFactory
      {
        static IICESocketFactory &singleton();

        virtual ICESocketPtr create(
                                    IMessageQueuePtr queue,
                                    IICESocketDelegatePtr delegate,
                                    const char *turnServer,
                                    const char *turnServerUsername,
                                    const char *turnServerPassword,
                                    const char *stunServer,
                                    WORD port = 0,
                                    bool firstWORDInAnyPacketWillNotConflictWithTURNChannels = false
                                    );

        virtual ICESocketPtr create(
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
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::internal::IICESocketForICESocketSession)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getSocket, hookflash::services::IICESocketPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getLock, RecursiveLock &)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_5(sendTo, bool, hookflash::services::IICESocket::Types, const IPAddress &, const BYTE *, ULONG, bool)
ZS_DECLARE_PROXY_METHOD_1(onICESocketSessionClosed, PUID)
ZS_DECLARE_PROXY_METHOD_SYNC_2(addRoute, hookflash::services::internal::ICESocketSessionPtr, const IPAddress &)
ZS_DECLARE_PROXY_METHOD_SYNC_1(removeRoute, hookflash::services::internal::ICESocketSessionPtr)
ZS_DECLARE_PROXY_END()
