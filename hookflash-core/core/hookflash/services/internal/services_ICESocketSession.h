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

#pragma once

#include <hookflash/services/internal/hookflashTypes.h>
#include <hookflash/services/IICESocketSession.h>
#include <hookflash/services/ISTUNRequester.h>
#include <zsLib/zsTypes.h>
#include <zsLib/Timer.h>
#include <zsLib/MessageQueueAssociator.h>

#include <list>
#include <utility>

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      interaction IICESocketSessionAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      interaction IICESocketSessionForICESocket
      {
        typedef zsLib::PUID PUID;
        typedef zsLib::BYTE BYTE;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::String String;
        typedef zsLib::IPAddress IPAddress;

        virtual PUID getID() const = 0;
        virtual void close() = 0;
        virtual void timeout() = 0;

        virtual bool handleSTUNPacket(
                                      IICESocket::Types viaTransport,
                                      const IPAddress &source,
                                      STUNPacketPtr stun,
                                      const String &localUsernameFrag,
                                      const String &remoteUsernameFrag
                                      ) = 0;
        virtual bool handlePacket(
                                  IICESocket::Types viaTransport,
                                  const IPAddress &source,
                                  const BYTE *packet,
                                  ULONG packetLengthInBytes
                                  ) = 0;

        virtual void notifyLocalWriteReady() = 0;
        virtual void notifyRelayWriteReady() = 0;
      };

      class ICESocketSession : public zsLib::MessageQueueAssociator,
                               public IICESocketSession,
                               public IICESocketSessionForICESocket,
                               public IICESocketSessionAsyncDelegate,
                               public IICESocketDelegate,
                               public ISTUNRequesterDelegate,
                               public zsLib::ITimerDelegate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::BYTE BYTE;
        typedef zsLib::QWORD QWORD;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::Time Time;
        typedef zsLib::String String;
        typedef zsLib::IPAddress IPAddress;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::TimerPtr TimerPtr;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;

        struct CandidatePair;
        typedef boost::shared_ptr<CandidatePair> CandidatePairPtr;

        struct CandidatePair
        {
          static CandidatePairPtr create();
          CandidatePairPtr clone() const;

          Candidate mLocal;
          Candidate mRemote;

          bool mReceivedRequest;
          bool mReceivedResponse;
          bool mFailed;

          ISTUNRequesterPtr mRequester;
        };

      protected:
        ICESocketSession(
                         IMessageQueuePtr queue,
                         IICESocketSessionDelegatePtr delegate,
                         ICESocketPtr socket,
                         ICEControls control
                         );

        void init();

      public:
        ~ICESocketSession();

        static ICESocketSessionPtr create(
                                          IMessageQueuePtr queue,
                                          IICESocketSessionDelegatePtr delegate,
                                          ICESocketPtr socket,
                                          ICEControls control
                                          );

        //---------------------------------------------------------------------
        // IICESocketSession
        virtual PUID getID() const {return mID;}

        virtual IICESocketPtr getSocket();

        virtual ICESocketSessionStates getState() const;
        virtual ICESocketSessionShutdownReasons getShutdownReason() const;

        virtual void close();

        virtual void getLocalCandidates(CandidateList &outCandidates);
        virtual void updateRemoteCandidates(const CandidateList &remoteCandidates);

        virtual void setKeepAliveProperties(
                                            Duration sendKeepAliveIndications,
                                            Duration expectSTUNOrDataWithinWithinOrSendAliveCheck = Duration(),
                                            Duration keepAliveSTUNRequestTimeout = Duration(),
                                            Duration backgroundingTimeout = Duration()
                                            );

        virtual bool sendPacket(
                                const BYTE *packet,
                                ULONG packetLengthInBytes
                                );

        virtual ICEControls getConnectedControlState();

        virtual IPAddress getConnectedRemoteIP();

        virtual bool getNominatedCandidateInformation(
                                                      Candidate &outLocal,
                                                      Candidate &outRemote
                                                      );

        //---------------------------------------------------------------------
        // IICESocketSessionForICESocket

        //        virtual PUID getID() const;  // already handled, same meaning...
        //        virtual void close();               // already handled, same meaning...
        virtual void timeout();

        virtual bool handleSTUNPacket(
                                      IICESocket::Types viaTransport,
                                      const IPAddress &source,
                                      STUNPacketPtr stun,
                                      const String &localUsernameFrag,
                                      const String &remoteUsernameFrag
                                      );
        virtual bool handlePacket(
                                  IICESocket::Types viaTransport,
                                  const IPAddress &source,
                                  const BYTE *packet,
                                  ULONG packetLengthInBytes
                                  );

        virtual void notifyLocalWriteReady();
        virtual void notifyRelayWriteReady();

        //---------------------------------------------------------------------
        // IICESocketSessionAsyncDelegate
        virtual void onStep();

        //---------------------------------------------------------------------
        // IICESocketDelegate
        virtual void onICESocketStateChanged(
                                             IICESocketPtr socket,
                                             ICESocketStates state
                                             );

        //---------------------------------------------------------------------
        // ISTUNRequesterDelegate
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
        // ITimerDelegate
        virtual void onTimer(TimerPtr timer);

      protected:
        RecursiveLock &getLock() const;

        String log(const char *message) const;
        void fix(STUNPacketPtr stun) const;

        bool isShutdown() const {return ICESocketSessionState_Shutdown == mCurrentState;}

        void cancel();
        void step();
        void setState(ICESocketSessionStates state);
        void setShutdownReason(ICESocketSessionShutdownReasons reason);

        void switchRole(ICEControls newRole);

        bool sendTo(
                    IICESocket::Types viaTransport,
                    const IPAddress &destination,
                    const BYTE *buffer,
                    ULONG bufferLengthInBytes,
                    bool isUserData
                    );

      protected:
        mutable RecursiveLock mBogusLock;

        ICESocketSessionWeakPtr mThisWeak;
        IICESocketForICESocketSessionWeakPtr mICESocketWeak;
        PUID mID;

        ICESocketSessionStates mCurrentState;
        ICESocketSessionShutdownReasons mShutdownReason;

        IICESocketSessionDelegatePtr mDelegate;
        bool mInformedWriteReady;

        IICESocketSubscriptionPtr mSocketSubscription;

        TimerPtr mActivateTimer;
        TimerPtr mKeepAliveTimer;
        TimerPtr mExpectingDataTimer;
        TimerPtr mStepTimer;

        ICEControls mControl;
        QWORD mConflictResolver;
        Time mStartedSearchAt;

        ISTUNRequesterPtr mNominateRequester;
        CandidatePairPtr mNominated;
        Time mLastSentData;
        Time mLastActivity;

        ISTUNRequesterPtr mAliveCheckRequester;
        Time mLastReceivedDataOrSTUN;
        Duration mKeepAliveDuration;
        Duration mExpectSTUNOrDataWithinDuration;
        Duration mKeepAliveSTUNRequestTimeout;
        Duration mBackgroundingTimeout;

        typedef std::list<CandidatePairPtr> CandidatePairList;
        CandidatePairList mCandidatePairs;

        CandidateList mRemoteCandidates;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::internal::IICESocketSessionAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
