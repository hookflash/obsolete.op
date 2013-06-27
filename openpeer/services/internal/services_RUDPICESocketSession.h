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
#include <openpeer/services/internal/services_RUDPChannel.h>
#include <openpeer/services/IRUDPICESocketSession.h>
#include <openpeer/services/IICESocketSession.h>
#include <openpeer/services/ISTUNRequester.h>

#define HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_START (0x6000)                    // the actual range is 0x4000 -> 0x7FFF but to prevent collision with TURN, RUDP this is a recommended range to use
#define HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_END   (0x7FFF)

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
      #pragma mark IRUDPICESocketSessionForRUDPICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      interaction IRUDPICESocketSessionForRUDPICESocket
      {
        typedef IICESocket::CandidateList CandidateList;
        typedef IICESocket::ICEControls ICEControls;

        IRUDPICESocketSessionForRUDPICESocket &forSocket() {return *this;}
        const IRUDPICESocketSessionForRUDPICESocket &forSocket() const {return *this;}

        static RUDPICESocketSessionPtr create(
                                              IMessageQueuePtr queue,
                                              RUDPICESocketPtr parent,
                                              IRUDPICESocketSessionDelegatePtr delegate,
                                              const CandidateList &remoteCandidates,
                                              ICEControls control
                                              );

        virtual PUID getID() const = 0;

        virtual void shutdown() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocketSession
      #pragma mark

      class RUDPICESocketSession : public Noop,
                                   public MessageQueueAssociator,
                                   public IRUDPICESocketSession,
                                   public IRUDPICESocketSessionForRUDPICESocket,
                                   public IICESocketSessionDelegate,
                                   public IRUDPChannelDelegateForSessionAndListener
      {
      public:
        friend interaction IRUDPICESocketSessionFactory;

        typedef IICESocket::CandidateList CandidateList;
        typedef IICESocket::ICEControls ICEControls;

        typedef WORD ChannelNumber;
        typedef std::map<ChannelNumber, RUDPChannelPtr> SessionMap;

        typedef std::list<RUDPChannelPtr> PendingSessionList;

      protected:
        RUDPICESocketSession(
                             IMessageQueuePtr queue,
                             RUDPICESocketPtr parent,
                             IRUDPICESocketSessionDelegatePtr delegate
                             );
        
        RUDPICESocketSession(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init(
                  const CandidateList &remoteCandidates,
                  ICEControls control
                  );

      public:
        ~RUDPICESocketSession();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocketSession => RUDPICESocketSession
        #pragma mark

        virtual PUID getID() const {return mID;}

        virtual IRUDPICESocketPtr getSocket();

        virtual RUDPICESocketSessionStates getState() const;
        virtual RUDPICESocketSessionShutdownReasons getShutdownReason() const;

        virtual void shutdown();

        virtual void getLocalCandidates(CandidateList &outCandidates);
        virtual void updateRemoteCandidates(const CandidateList &remoteCandidates);

        virtual void setKeepAliveProperties(
                                            Duration sendKeepAliveIndications,
                                            Duration expectSTUNOrDataWithinWithinOrSendAliveCheck = Duration(),
                                            Duration keepAliveSTUNRequestTimeout = Duration(),
                                            Duration backgroundingTimeout = Duration()
                                            );

        virtual ICEControls getConnectedControlState();

        virtual IPAddress getConnectedRemoteIP();

        virtual bool getNominatedCandidateInformation(
                                                      Candidate &outLocal,
                                                      Candidate &outRemote
                                                      );

        virtual IRUDPChannelPtr openChannel(
                                            IRUDPChannelDelegatePtr delegate,
                                            const char *connectionInfo
                                            );

        virtual IRUDPChannelPtr acceptChannel(IRUDPChannelDelegatePtr delegate);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocketSession => IRUDPICESocketSessionForRUDPICESocket
        #pragma mark

        static RUDPICESocketSessionPtr create(
                                              IMessageQueuePtr queue,
                                              RUDPICESocketPtr parent,
                                              IRUDPICESocketSessionDelegatePtr delegate,
                                              const CandidateList &remoteCandidates,
                                              ICEControls control
                                              );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocketSession => IICESocketSessionDelegate
        #pragma mark

        virtual void onICESocketSessionStateChanged(
                                                    IICESocketSessionPtr session,
                                                    ICESocketSessionStates state
                                                    );

        virtual void handleICESocketSessionReceivedPacket(
                                                          IICESocketSessionPtr session,
                                                          const BYTE *buffer,
                                                          ULONG bufferLengthInBytes
                                                          );

        virtual bool handleICESocketSessionReceivedSTUNPacket(
                                                              IICESocketSessionPtr session,
                                                              STUNPacketPtr stun,
                                                              const String &localUsernameFrag,
                                                              const String &remoteUsernameFrag
                                                              );

        virtual void onICESocketSessionWriteReady(IICESocketSessionPtr session);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocketSession => IRUDPChannelDelegateForSessionAndListener
        #pragma mark

        virtual void onRUDPChannelStateChanged(
                                               RUDPChannelPtr channel,
                                               RUDPChannelStates state
                                               );

        virtual bool notifyRUDPChannelSendPacket(
                                                 RUDPChannelPtr channel,
                                                 const IPAddress &remoteIP,
                                                 const BYTE *packet,
                                                 ULONG packetLengthInBytes
                                                 );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocketSession => (internal)
        #pragma mark

        RecursiveLock &getLock() const;

        String log(const char *message) const;
        void fix(STUNPacketPtr stun) const;

        IICESocketSessionPtr getICESession() const;

        bool isReady() {return RUDPICESocketSessionState_Ready == mCurrentState;}
        bool isShuttingDown() const {return RUDPICESocketSessionState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return RUDPICESocketSessionState_Shutdown == mCurrentState;}

        void cancel();
        void step();

        void setState(RUDPICESocketSessionStates state);
        void setShutdownReason(RUDPICESocketSessionShutdownReasons reason);

        bool handleUnknownChannel(
                                  STUNPacketPtr &stun,
                                  STUNPacketPtr &outResponse
                                  );

        void issueChannelConnectIfPossible();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocketSession => (data)
        #pragma mark

        mutable RecursiveLock mBogusLock;

        RUDPICESocketSessionWeakPtr mThisWeak;
        RUDPICESocketWeakPtr mOuter;

        RUDPICESocketSessionPtr mGracefulShutdownReference;

        PUID mID;
        RUDPICESocketSessionStates mCurrentState;
        RUDPICESocketSessionShutdownReasons mShutdownReason;

        IRUDPICESocketSessionDelegatePtr mDelegate;

        IICESocketSessionPtr mICESession;

        String mLocalUsernameFrag;
        String mRemoteUsernameFrag;

        SessionMap mLocalChannelNumberSessions;   // local channel numbers are the channel numbers we expect to receive from the remote party
        SessionMap mRemoteChannelNumberSessions;  // remote channel numbers are the channel numbers we expect to send to the remote party

        PendingSessionList mPendingSessions;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPICESocketSessionFactory
      #pragma mark

      interaction IRUDPICESocketSessionFactory
      {
        typedef IICESocket::CandidateList CandidateList;
        typedef IICESocket::ICEControls ICEControls;

        static IRUDPICESocketSessionFactory &singleton();

        virtual RUDPICESocketSessionPtr create(
                                               IMessageQueuePtr queue,
                                               RUDPICESocketPtr parent,
                                               IRUDPICESocketSessionDelegatePtr delegate,
                                               const CandidateList &remoteCandidates,
                                               ICEControls control
                                               );
      };
      
    }
  }
}
