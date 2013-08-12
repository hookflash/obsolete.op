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
#include <openpeer/services/IRUDPICESocket.h>
#include <openpeer/services/IICESocket.h>

#include <map>

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
      #pragma mark IRUDPICESocketForRUDPICESocketSession
      #pragma mark

      interaction IRUDPICESocketForRUDPICESocketSession
      {
        typedef IICESocket::CandidateList CandidateList;
        typedef IICESocket::ICEControls ICEControls;

        IRUDPICESocketForRUDPICESocketSession &forSession() {return *this;}
        const IRUDPICESocketForRUDPICESocketSession &forSession() const {return *this;}

        virtual IRUDPICESocketSessionPtr createSessionFromRemoteCandidates(
                                                                           IRUDPICESocketSessionDelegatePtr delegate,
                                                                           const char *remoteUsernameFrag,
                                                                           const char *remotePassword,
                                                                           const CandidateList &remoteCandidates,
                                                                           ICEControls control
                                                                           ) = 0;

        virtual IICESocketPtr getICESocket() const = 0;
        virtual IRUDPICESocketPtr getRUDPICESocket() const = 0;

        virtual RecursiveLock &getLock() const = 0;

        virtual void onRUDPICESessionClosed(PUID sessionID) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket
      #pragma mark

      class RUDPICESocket : public Noop,
                            public MessageQueueAssociator,
                            public IRUDPICESocket,
                            public IICESocketDelegate,
                            public IRUDPICESocketForRUDPICESocketSession
      {
      public:
        friend interaction IRUDPICESocketFactory;
        friend interaction IRUDPICESocket;

        typedef IICESocket::CandidateList CandidateList;
        typedef IICESocket::ICEControls ICEControls;

        typedef std::map<PUID, RUDPICESocketSessionPtr> SessionMap;

      protected:
        RUDPICESocket(
                      IMessageQueuePtr queue,
                      IRUDPICESocketDelegatePtr delegate
                      );
        RUDPICESocket(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init(
                  const char *turnServer,
                  const char *turnServerUsername,
                  const char *turnServerPassword,
                  const char *stunServer,
                  WORD port = 0
                  );
        void init(
                  IDNS::SRVResultPtr srvTURNUDP,
                  IDNS::SRVResultPtr srvTURNTCP,
                  const char *turnServerUsername,
                  const char *turnServerPassword,
                  IDNS::SRVResultPtr srvSTUN,
                  WORD port = 0
                  );

      public:
        ~RUDPICESocket();

        static RUDPICESocketPtr convert(IRUDPICESocketPtr socket);
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocket => IRUDPICESocket
        #pragma mark

        static String toDebugString(IRUDPICESocketPtr socket, bool includeCommaPrefix = true);

        static RUDPICESocketPtr create(
                                       IMessageQueuePtr queue,
                                       IRUDPICESocketDelegatePtr delegate,
                                       const char *turnServer,
                                       const char *turnServerUsername,
                                       const char *turnServerPassword,
                                       const char *stunServer,
                                       WORD port = 0
                                       );

        static RUDPICESocketPtr create(
                                       IMessageQueuePtr queue,
                                       IRUDPICESocketDelegatePtr delegate,
                                       IDNS::SRVResultPtr srvTURNUDP,
                                       IDNS::SRVResultPtr srvTURNTCP,
                                       const char *turnServerUsername,
                                       const char *turnServerPassword,
                                       IDNS::SRVResultPtr srvSTUN,
                                       WORD port = 0
                                       );

        virtual PUID getID() const {return mID;}

        virtual RUDPICESocketStates getState(
                                             WORD *outLastErrorCode = NULL,
                                             String *outLastErrorReason = NULL
                                             ) const;

        virtual IRUDPICESocketSubscriptionPtr subscribe(IRUDPICESocketDelegatePtr delegate);

        virtual String getUsernameFrag() const;

        virtual String getPassword() const;

        virtual void shutdown();

        virtual void wakeup(Duration minimumTimeCandidatesMustRemainValidWhileNotUsed);

        virtual void getLocalCandidates(CandidateList &outCandidates);

        virtual IRUDPICESocketSessionPtr createSessionFromRemoteCandidates(
                                                                           IRUDPICESocketSessionDelegatePtr delegate,
                                                                           const char *remoteUsernameFrag,
                                                                           const char *remotePassword,
                                                                           const CandidateList &remoteCandidates,
                                                                           ICEControls control
                                                                           );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocket => IRUDPICESocketForRUDPICESocketSession
        #pragma mark

        virtual RecursiveLock &getLock() const {return mLock;}

        virtual IICESocketPtr getICESocket() const;
        virtual IRUDPICESocketPtr getRUDPICESocket() const;

        virtual void onRUDPICESessionClosed(PUID sessionID);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocket => IICESocketDelegate
        #pragma mark

        virtual void onICESocketStateChanged(
                                             IICESocketPtr socket,
                                             ICESocketStates state
                                             );

        virtual void onICESocketCandidatesChanged(IICESocketPtr socket);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocket => (internal)
        #pragma mark

        String log(const char *message) const;
        bool isShuttingDown();
        bool isShutdown();

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void cancel();
        void setState(RUDPICESocketStates state);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPICESocket => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        AutoPUID mID;
        RUDPICESocketWeakPtr mThisWeak;
        RUDPICESocketPtr mGracefulShutdownReference;

        RUDPICESocketStates mCurrentState;

        IRUDPICESocketDelegateSubscriptions mSubscriptions;
        IRUDPICESocketSubscriptionPtr mDefaultSubscription;

        AutoBool mNotifiedCandidateChanged;

        IICESocketPtr mICESocket;

        SessionMap mSessions;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPICESocketFactory
      #pragma mark

      interaction IRUDPICESocketFactory
      {
        static IRUDPICESocketFactory &singleton();

        virtual RUDPICESocketPtr create(
                                        IMessageQueuePtr queue,
                                        IRUDPICESocketDelegatePtr delegate,
                                        const char *turnServer,
                                        const char *turnServerUsername,
                                        const char *turnServerPassword,
                                        const char *stunServer,
                                        WORD port = 0
                                        );

        virtual RUDPICESocketPtr create(
                                        IMessageQueuePtr queue,
                                        IRUDPICESocketDelegatePtr delegate,
                                        IDNS::SRVResultPtr srvTURNUDP,
                                        IDNS::SRVResultPtr srvTURNTCP,
                                        const char *turnServerUsername,
                                        const char *turnServerPassword,
                                        IDNS::SRVResultPtr srvSTUN,
                                        WORD port = 0
                                        );
      };
      
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::internal::IRUDPICESocketForRUDPICESocketSession)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::RecursiveLock, RecursiveLock)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::PUID, PUID)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IRUDPICESocketSessionPtr, IRUDPICESocketSessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IRUDPICESocketSessionDelegatePtr, IRUDPICESocketSessionDelegatePtr)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_5(createSessionFromRemoteCandidates, IRUDPICESocketSessionPtr, IRUDPICESocketSessionDelegatePtr, const char *, const char *, const CandidateList &, ICEControls)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getLock, RecursiveLock &)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getICESocket, openpeer::services::IICESocketPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_CONST_RETURN_0(getRUDPICESocket, openpeer::services::IRUDPICESocketPtr)
ZS_DECLARE_PROXY_METHOD_1(onRUDPICESessionClosed, PUID)
ZS_DECLARE_PROXY_END()
