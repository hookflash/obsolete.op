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
#include <hookflash/services/internal/services_RUDPChannel.h>
#include <hookflash/services/IRUDPICESocketSession.h>
#include <hookflash/services/IICESocketSession.h>
#include <hookflash/services/ISTUNRequester.h>

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
      #pragma mark RUDPICESocketSession
      #pragma mark

      class RUDPICESocketSession : public MessageQueueAssociator,
                                   public IRUDPICESocketSession,
                                   public IICESocketSessionDelegate,
                                   public IRUDPChannelDelegateForSessionAndListener
      {
      protected:
        RUDPICESocketSession(
                             IMessageQueuePtr queue,
                             RUDPICESocketPtr parent,
                             IRUDPICESocketSessionDelegatePtr delegate
                             );

        void init(
                  const CandidateList &remoteCandidates,
                  ICEControls control
                  );

      public:
        ~RUDPICESocketSession();

        static RUDPICESocketSessionPtr create(
                                              IMessageQueuePtr queue,
                                              RUDPICESocketPtr parent,
                                              IRUDPICESocketSessionDelegatePtr delegate,
                                              const CandidateList &remoteCandidates,
                                              ICEControls control
                                              );

        //RUDPICESocketSession
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

        //IICESocketSessionDelegate
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

        // IRUDPChannelDelegateForSessionAndListener
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

        typedef WORD ChannelNumber;
        typedef std::map<ChannelNumber, RUDPChannelPtr> SessionMap;
        SessionMap mLocalChannelNumberSessions;   // local channel numbers are the channel numbers we expect to receive from the remote party
        SessionMap mRemoteChannelNumberSessions;  // remote channel numbers are the channel numbers we expect to send to the remote party

        typedef std::list<RUDPChannelPtr> PendingSessionList;
        PendingSessionList mPendingSessions;
      };
    }
  }
}
