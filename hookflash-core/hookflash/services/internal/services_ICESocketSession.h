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
#include <hookflash/services/IICESocketSession.h>
#include <hookflash/services/ISTUNRequester.h>
#include <zsLib/types.h>
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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketSessionForICESocket
      #pragma mark

      interaction IICESocketSessionForICESocket
      {
        typedef IICESocketSession::ICEControls ICEControls;
        typedef IICESocketSession::CandidateList CandidateList;

        IICESocketSessionForICESocket &forICESocket() {return *this;}
        const IICESocketSessionForICESocket &forICESocket() const {return *this;}

        static ICESocketSessionPtr create(
                                          IMessageQueuePtr queue,
                                          IICESocketSessionDelegatePtr delegate,
                                          ICESocketPtr socket,
                                          ICEControls control
                                          );

        virtual PUID getID() const = 0;
        virtual void close() = 0;

        virtual void updateRemoteCandidates(const CandidateList &remoteCandidates) = 0;

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

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketSessionAsyncDelegate
      #pragma mark

      interaction IICESocketSessionAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession
      #pragma mark

      class ICESocketSession : public Noop,
                               public MessageQueueAssociator,
                               public IICESocketSession,
                               public IICESocketSessionForICESocket,
                               public IICESocketSessionAsyncDelegate,
                               public IICESocketDelegate,
                               public ISTUNRequesterDelegate,
                               public ITimerDelegate
      {
      public:
        friend interaction IICESocketSessionFactory;

        typedef IICESocketSession::ICEControls ICEControls;
        typedef IICESocketSession::CandidateList CandidateList;

        struct CandidatePair;
        typedef boost::shared_ptr<CandidatePair> CandidatePairPtr;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocketSession::CandidatePair
        #pragma mark

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

        typedef std::list<CandidatePairPtr> CandidatePairList;

      protected:
        ICESocketSession(
                         IMessageQueuePtr queue,
                         IICESocketSessionDelegatePtr delegate,
                         ICESocketPtr socket,
                         ICEControls control
                         );
        
        ICESocketSession(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~ICESocketSession();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocketSession => IICESocketSession
        #pragma mark

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
        #pragma mark
        #pragma mark ICESocketSession => IICESocketSessionForICESocket
        #pragma mark

        static ICESocketSessionPtr create(
                                          IMessageQueuePtr queue,
                                          IICESocketSessionDelegatePtr delegate,
                                          ICESocketPtr socket,
                                          ICEControls control
                                          );

        // (duplicate) virtual PUID getID() const;
        // (duplicate) virtual void close();

        // (duplicate) virtual void updateRemoteCandidates(const CandidateList &remoteCandidates);

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
        #pragma mark
        #pragma mark ICESocketSession => IICESocketSessionAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocketSession => IICESocketDelegate
        #pragma mark

        virtual void onICESocketStateChanged(
                                             IICESocketPtr socket,
                                             ICESocketStates state
                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocketSession => ISTUNRequesterDelegate
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
        #pragma mark ICESocketSession => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocketSession => (internal)
        #pragma mark

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
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ICESocketSession => (data)
        #pragma mark

        mutable RecursiveLock mBogusLock;

        ICESocketSessionWeakPtr mThisWeak;
        ICESocketWeakPtr mICESocketWeak;
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

        CandidatePairList mCandidatePairs;

        CandidateList mRemoteCandidates;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketSessionFactory
      #pragma mark

      interaction IICESocketSessionFactory
      {
        typedef IICESocketSession::ICEControls ICEControls;

        static IICESocketSessionFactory &singleton();

        virtual ICESocketSessionPtr create(
                                           IMessageQueuePtr queue,
                                           IICESocketSessionDelegatePtr delegate,
                                           ICESocketPtr socket,
                                           ICEControls control
                                           );
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::internal::IICESocketSessionAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
