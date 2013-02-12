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
#include <hookflash/services/internal/services_IRUDPChannelStream.h>
#include <hookflash/services/IRUDPChannel.h>
#include <hookflash/services/ISTUNRequester.h>

#include <zsLib/Timer.h>

#include <map>

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
      #pragma mark IRUDPChannelForRUDPICESocketSession
      #pragma mark

      interaction IRUDPChannelForRUDPICESocketSession
      {
        IRUDPChannelForRUDPICESocketSession &forSession() {return *this;}
        const IRUDPChannelForRUDPICESocketSession &forSession() const {return *this;}

        static RUDPChannelPtr createForRUDPICESocketSessionIncoming(
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
                                                                    );

        static RUDPChannelPtr createForRUDPICESocketSessionOutgoing(
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
                                                                    );

        virtual PUID getID() const = 0;

        virtual void setDelegate(IRUDPChannelDelegatePtr delegate) = 0;

        virtual bool handleSTUN(
                                STUNPacketPtr stun,
                                STUNPacketPtr &outResponse,
                                const String &localUsernameFrag,
                                const String &remoteUsernameFrag
                                ) = 0;

        virtual void handleRUDP(
                                RUDPPacketPtr rudp,
                                const BYTE *buffer,
                                ULONG bufferLengthInBytes
                                ) = 0;

        virtual void notifyWriteReady() = 0;
        virtual WORD getIncomingChannelNumber() const = 0;
        virtual WORD getOutgoingChannelNumber() const = 0;

        virtual void issueConnectIfNotIssued() = 0;

        virtual void shutdown() = 0;
        virtual void shutdownFromTimeout() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelForRUDPListener
      #pragma mark

      interaction IRUDPChannelForRUDPListener
      {
        IRUDPChannelForRUDPListener &forListener() {return *this;}
        const IRUDPChannelForRUDPListener &forListener() const {return *this;}

        static RUDPChannelPtr createForListener(
                                                IMessageQueuePtr queue,
                                                IRUDPChannelDelegateForSessionAndListenerPtr master,
                                                const IPAddress &remoteIP,
                                                WORD incomingChannelNumber,
                                                STUNPacketPtr channelOpenPacket,
                                                STUNPacketPtr &outResponse
                                                );

        virtual void setDelegate(IRUDPChannelDelegatePtr delegate) = 0;

        virtual bool handleSTUN(
                                STUNPacketPtr stun,
                                STUNPacketPtr &outResponse,
                                const String &localUsernameFrag,
                                const String &remoteUsernameFrag
                                ) = 0;

        virtual void handleRUDP(
                                RUDPPacketPtr rudp,
                                const BYTE *buffer,
                                ULONG bufferLengthInBytes
                                ) = 0;

        virtual void notifyWriteReady() = 0;

        virtual void shutdown() = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelAsyncDelegate
      #pragma mark

      interaction IRUDPChannelAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannel
      #pragma mark

      class RUDPChannel : public Noop,
                          public MessageQueueAssociator,
                          public IRUDPChannel,
                          public IRUDPChannelForRUDPICESocketSession,
                          public IRUDPChannelForRUDPListener,
                          public IRUDPChannelAsyncDelegate,
                          public IRUDPChannelStreamDelegate,
                          public ISTUNRequesterDelegate,
                          public ITimerDelegate
      {
      public:
        friend interaction IRUDPChannelFactory;

        typedef PUID ACKRequestID;
        typedef std::map<ACKRequestID, ISTUNRequesterPtr> ACKRequestMap;

        typedef std::pair<boost::shared_array<BYTE>, ULONG> PendingSendBuffer;
        typedef std::list<PendingSendBuffer> PendingSendBufferList;

      protected:
        RUDPChannel(
                    IMessageQueuePtr queue,
                    IRUDPChannelDelegateForSessionAndListenerPtr master,
                    const IPAddress &remoteIP,
                    const char *localUserFrag,
                    const char *remoteUserFrag,
                    const char *localPassword,
                    const char *remotePassword,
                    DWORD minimumRTT,
                    DWORD lifetime,
                    WORD incomingChannelNumber,
                    QWORD localSequenceNumber,
                    const char *localChannelInfo,
                    WORD outgoingChannelNumber = 0,
                    QWORD remoteSequenceNumber = 0,
                    const char *remoteChannelInfo = NULL
                    );
        RUDPChannel(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~RUDPChannel();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => IRUDPChannel
        #pragma mark

        virtual PUID getID() const {return mID;}

        virtual RUDPChannelStates getState() const;
        virtual RUDPChannelShutdownReasons getShutdownReason() const;

        virtual void shutdown();

        virtual void shutdownDirection(Shutdown state);

        virtual bool send(
                          const BYTE *buffer,
                          ULONG bufferLengthInBytes
                          );

        virtual ULONG getReceiveSizeAvailableInBytes();
        virtual ULONG receive(
                                     BYTE *outBuffer,
                                     ULONG bufferLengthInBytes
                                     );

        virtual IPAddress getConnectedRemoteIP();

        virtual String getRemoteConnectionInfo();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => IRUDPChannelForRUDPICESocketSession
        #pragma mark

        static RUDPChannelPtr createForRUDPICESocketSessionIncoming(
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
                                                                    );

        static RUDPChannelPtr createForRUDPICESocketSessionOutgoing(
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
                                                                    );

        // (duplicate) virtual PUID getID() const;

        virtual void setDelegate(IRUDPChannelDelegatePtr delegate);

        virtual bool handleSTUN(
                                STUNPacketPtr stun,
                                STUNPacketPtr &outResponse,
                                const String &localUsernameFrag,
                                const String &remoteUsernameFrag
                                );

        virtual void handleRUDP(
                                RUDPPacketPtr rudp,
                                const BYTE *buffer,
                                ULONG bufferLengthInBytes
                                );

        virtual void notifyWriteReady();
        virtual WORD getIncomingChannelNumber() const;
        virtual WORD getOutgoingChannelNumber() const;

        virtual void issueConnectIfNotIssued();

        // (duplicate) virtual void shutdown();

        virtual void shutdownFromTimeout();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => IRUDPChannelForRUDPListener
        #pragma mark

        static RUDPChannelPtr createForListener(
                                                IMessageQueuePtr queue,
                                                IRUDPChannelDelegateForSessionAndListenerPtr master,
                                                const IPAddress &remoteIP,
                                                WORD incomingChannelNumber,
                                                STUNPacketPtr channelOpenPacket,
                                                STUNPacketPtr &outResponse
                                                );

        // (duplicate) virtual void setDelegate(IRUDPChannelDelegatePtr delegate);

        // (duplicate) virtual bool handleSTUN(
        //                                     STUNPacketPtr stun,
        //                                     STUNPacketPtr &outResponse,
        //                                     const String &localUsernameFrag,
        //                                     const String &remoteUsernameFrag
        //                                     );

        // (duplicate) virtual void handleRUDP(
        //                                     RUDPPacketPtr rudp,
        //                                     const BYTE *buffer,
        //                                     ULONG bufferLengthInBytes
        //                                     );

        // (duplicate) virtual void notifyWriteReady();

        // (duplicate) virtual void shutdown();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => IRUDPChannelAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => IRUDPChannelStreamDelegate
        #pragma mark

        virtual void onRUDPChannelStreamStateChanged(
                                                     IRUDPChannelStreamPtr stream,
                                                     RUDPChannelStreamStates state
                                                     );

        virtual void onRUDPChannelStreamReadReady(IRUDPChannelStreamPtr stream);
        virtual void onRUDPChannelStreamWriteReady(IRUDPChannelStreamPtr stream);

        virtual bool notifyRUDPChannelStreamSendPacket(
                                                       IRUDPChannelStreamPtr stream,
                                                       const BYTE *packet,
                                                       ULONG packetLengthInBytes
                                                       );

        virtual void onRUDPChannelStreamSendExternalACKNow(
                                                           IRUDPChannelStreamPtr stream,
                                                           bool guarenteeDelivery,
                                                           PUID guarenteeDeliveryRequestID = 0
                                                           );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => ISTUNRequesterDelegate
        #pragma mark

        virtual void onSTUNRequesterSendPacket(
                                               ISTUNRequesterPtr requester,
                                               IPAddress destination,
                                               boost::shared_array<BYTE> packet,
                                               ULONG packetLengthInBytes
                                               );

        virtual bool handleSTUNRequesterResponse(
                                                 ISTUNRequesterPtr requester,
                                                 IPAddress fromIPAddress,
                                                 STUNPacketPtr response
                                                 );

        virtual void onSTUNRequesterTimedOut(ISTUNRequesterPtr requester);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => (internal)
        #pragma mark

        String log(const char *message) const;
        void fix(STUNPacketPtr stun) const;

        bool isShuttingDown() {return RUDPChannelState_ShuttingDown == mCurrentState;}
        bool isShutdown() {return RUDPChannelState_Shutdown == mCurrentState;}

        void cancel(bool waitForAllDataToSend);
        void step();

        void setState(RUDPChannelStates state);
        void setShutdownReason(RUDPChannelShutdownReasons reason);

        bool isValidIntegrity(STUNPacketPtr stun);
        void fillCredentials(STUNPacketPtr &outSTUN);
        void fillACK(STUNPacketPtr &outSTUN);

        bool handleStaleNonce(
                              ISTUNRequesterPtr &originalRequestVariable,
                              STUNPacketPtr response
                              );

        void sendPendingNow();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannel => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        RUDPChannelWeakPtr mThisWeak;
        RUDPChannelPtr mGracefulShutdownReference;
        PUID mID;

        bool mIncoming;

        RUDPChannelStates mCurrentState;
        RUDPChannelShutdownReasons mShutdownReason;

        IRUDPChannelDelegatePtr mDelegate;
        IRUDPChannelDelegateForSessionAndListenerPtr mMasterDelegate;

        bool mInformedReadReady;
        bool mInformedWriteReady;

        IRUDPChannelStreamPtr mStream;
        ISTUNRequesterPtr mOpenRequest;
        ISTUNRequesterPtr mShutdownRequest;
        bool mSTUNRequestPreviouslyTimedOut;    // if true then no need issue a "close" STUN request if a STUN request has previously timed out

        TimerPtr mTimer;

        IRUDPChannelStream::Shutdown mShutdownDirection;

        IPAddress mRemoteIP;

        String mLocalUsernameFrag;
        String mRemoteUsernameFrag;
        String mLocalPassword;
        String mRemotePassword;

        String mRealm;
        String mNonce;

        WORD mIncomingChannelNumber;
        WORD mOutgoingChannelNumber;

        QWORD mLocalSequenceNumber;
        QWORD mRemoteSequenceNumber;

        DWORD mMinimumRTT;
        DWORD mLifetime;

        String mLocalChannelInfo;
        String mRemoteChannelInfo;

        Time mLastSentData;
        Time mLastReceivedData;

        ACKRequestMap mOutstandingACKs;

        PendingSendBufferList mPendingBuffers;
      };

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelDelegateForSessionAndListener
      #pragma mark

      interaction IRUDPChannelDelegateForSessionAndListener
      {
        typedef IRUDPChannel::RUDPChannelStates RUDPChannelStates;

        virtual void onRUDPChannelStateChanged(
                                               RUDPChannelPtr channel,
                                               RUDPChannelStates state
                                               ) = 0;

        //---------------------------------------------------------------------
        // PURPOSE: Send a packet over the socket interface to the remote party.
        virtual bool notifyRUDPChannelSendPacket(
                                                 RUDPChannelPtr channel,
                                                 const IPAddress &remoteIP,
                                                 const BYTE *packet,
                                                 ULONG packetLengthInBytes
                                                 ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelFactory
      #pragma mark

      interaction IRUDPChannelFactory
      {
        static IRUDPChannelFactory &singleton();

        virtual RUDPChannelPtr createForRUDPICESocketSessionIncoming(
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
                                                                     );

        virtual RUDPChannelPtr createForRUDPICESocketSessionOutgoing(
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
                                                                     );

        virtual RUDPChannelPtr createForListener(
                                                 IMessageQueuePtr queue,
                                                 IRUDPChannelDelegateForSessionAndListenerPtr master,
                                                 const IPAddress &remoteIP,
                                                 WORD incomingChannelNumber,
                                                 STUNPacketPtr channelOpenPacket,
                                                 STUNPacketPtr &outResponse
                                                 );
      };
      
    }
  }
}


ZS_DECLARE_PROXY_BEGIN(hookflash::services::internal::IRUDPChannelDelegateForSessionAndListener)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::internal::RUDPChannelPtr, RUDPChannelPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::internal::IRUDPChannelDelegateForSessionAndListener::RUDPChannelStates, RUDPChannelStates)
ZS_DECLARE_PROXY_METHOD_2(onRUDPChannelStateChanged, RUDPChannelPtr, RUDPChannelStates)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_4(notifyRUDPChannelSendPacket, bool, RUDPChannelPtr, const IPAddress &, const BYTE *, ULONG)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::services::internal::IRUDPChannelAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
