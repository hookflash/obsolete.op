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

#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/IMessageRequester.h>
#include <hookflash/services/IRUDPICESocket.h>
#include <hookflash/services/IRUDPICESocketSession.h>
#include <hookflash/services/IRUDPMessaging.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>
#include <zsLib/Timer.h>

#include <map>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinder
      #pragma mark

      interaction IAccountFinder
      {
        typedef zsLib::PUID PUID;
        typedef zsLib::String String;
        typedef zsLib::Duration Duration;
        typedef zsLib::XML::DocumentPtr DocumentPtr;

        enum AccountFinderStates
        {
          AccountFinderState_Pending,
          AccountFinderState_Ready,
          AccountFinderState_ShuttingDown,
          AccountFinderState_Shutdown,
        };

        static const char *toString(AccountFinderStates state);

        virtual PUID getID() const = 0;
        virtual AccountFinderStates getState() const = 0;

        virtual void shutdown() = 0;

        virtual String getLocationID() const = 0;

        virtual bool requestSendMessage(
                                        DocumentPtr message,
                                        bool sendOnlyIfReady = true
                                        ) = 0;

        virtual IMessageRequesterPtr sendRequest(
                                                 IMessageRequesterDelegatePtr delegate,
                                                 message::MessagePtr requestMessage,
                                                 Duration timeout,
                                                 bool onlyIfReady = true
                                                 ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinderAsyncDelegate
      #pragma mark

      interaction IAccountFinderAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountFinder
      #pragma mark

      class AccountFinder : public zsLib::MessageQueueAssociator,
                            public IAccountFinder,
                            public IAccountFinderAsyncDelegate,
                            public IBootstrappedNetworkDelegate,
                            public services::IRUDPICESocketDelegate,
                            public services::IRUDPICESocketSessionDelegate,
                            public services::IRUDPMessagingDelegate,
                            public IMessageRequesterDelegate,
                            public zsLib::ITimerDelegate
      {
      protected:
        typedef zsLib::Duration Duration;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef zsLib::TimerPtr TimerPtr;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef services::IDNS IDNS;
        typedef services::IICESocket::CandidateList CandidateList;
        typedef services::IICESocket::Candidate Candidate;
        typedef services::IRUDPICESocketSubscriptionPtr IRUDPICESocketSubscriptionPtr;

      protected:
        AccountFinder(
                      IMessageQueuePtr queue,
                      IAccountFinderDelegatePtr delegate,
                      IAccountForAccountFinderPtr outer
                      );

        void init();

      public:
        ~AccountFinder();

        static AccountFinderPtr create(
                                       IMessageQueuePtr queue,
                                       IAccountFinderDelegatePtr delegate,
                                       IAccountForAccountFinderPtr outer
                                       );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IAccountFinder
        #pragma mark

        virtual PUID getID() const {return mID;}
        virtual AccountFinderStates getState() const;
        virtual void shutdown();
        virtual String getLocationID() const;
        virtual bool requestSendMessage(
                                        DocumentPtr message,
                                        bool sendOnlyIfReady = true
                                        );

        // (duplicate) virtual IMessageRequesterPtr sendRequest(
        //                                                      IMessageRequesterDelegatePtr delegate,
        //                                                      message::MessagePtr requestMessage,
        //                                                      Duration timeout,
        //                                                      bool onlyIfReady = true
        //                                                      );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IAccountFinderAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IBootstrappedNetworkDelegate
        #pragma mark

        virtual void onBootstrappedNetworkStateChanged(
                                                       IBootstrappedNetworkPtr bootstrapper,
                                                       BootstrappedNetworkStates state
                                                       );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IRUDPICESocketDelegate
        #pragma mark

        virtual void onRUDPICESocketStateChanged(
                                                 IRUDPICESocketPtr socket,
                                                 RUDPICESocketStates state
                                                 );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IRUDPICESocketSessionDelegate
        #pragma mark

        virtual void onRUDPICESocketSessionStateChanged(
                                                        IRUDPICESocketSessionPtr session,
                                                        RUDPICESocketSessionStates state
                                                        );

        virtual void onRUDPICESocketSessionChannelWaiting(IRUDPICESocketSessionPtr session);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IRUDPMessagingDelegate
        #pragma mark

        virtual void onRUDPMessagingStateChanged(
                                                 IRUDPMessagingPtr session,
                                                 RUDPMessagingStates state
                                                 );

        virtual void onRUDPMessagingReadReady(IRUDPMessagingPtr session);
        virtual void onRUDPMessagingWriteReady(IRUDPMessagingPtr session);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IMessageRequesterDelegate
        #pragma mark

        virtual bool handleMessageRequesterMessageReceived(
                                                           IMessageRequesterPtr requester,
                                                           message::MessagePtr message
                                                           );

        virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => (internal)
        #pragma mark

        bool isPending() const {return AccountFinderState_Pending == mCurrentState;}
        bool isReady() const {return AccountFinderState_Ready == mCurrentState;}
        bool isShuttingDown() const {return AccountFinderState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return AccountFinderState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const;
        IAccountForAccountFinderPtr getOuter() const;

        IRUDPICESocketPtr getSocket() const;
        IBootstrappedNetworkForAccountFinderPtr getBootstrapper() const;
        IPeerFilesPtr getPeerFiles() const;

        String log(const char *message) const;

        IMessageRequesterPtr sendRequest(
                                         IMessageRequesterDelegatePtr delegate,
                                         message::MessagePtr requestMessage,
                                         Duration timeout,
                                         bool onlyIfReady = true
                                         );

        void cancel();
        void step();
        void setState(AccountFinderStates state);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;

        AccountFinderStates mCurrentState;

        AccountFinderWeakPtr mThisWeak;
        IAccountFinderDelegatePtr mDelegate;
        IAccountForAccountFinderWeakPtr mOuter;

        AccountFinderPtr mGracefulShutdownReference;

        IBootstrappedNetworkSubscriptionPtr mBootstrapperSubscription;

        IRUDPICESocketSubscriptionPtr mSocketSubscription;
        IRUDPICESocketSessionPtr mSocketSession;
        IRUDPMessagingPtr mMessaging;

        String mLocationID;
        SecureByteBlock mLocationSalt;
        IMessageRequesterPtr mSessionCreateRequester;
        IMessageRequesterPtr mSessionKeepAliveRequester;
        IMessageRequesterPtr mSessionDeleteRequester;

        TimerPtr mKeepAliveTimer;
      };

      interaction IAccountFinderDelegate
      {
        typedef IAccountFinder::AccountFinderStates AccountFinderStates;

        virtual void onAccountFinderStateChanged(
                                                 IAccountFinderPtr finder,
                                                 AccountFinderStates state
                                                 ) = 0;
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountFinderAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountFinderDelegate)
ZS_DECLARE_PROXY_METHOD_2(onAccountFinderStateChanged, hookflash::stack::internal::IAccountFinderPtr, hookflash::stack::internal::IAccountFinder::AccountFinderStates)
ZS_DECLARE_PROXY_END()
