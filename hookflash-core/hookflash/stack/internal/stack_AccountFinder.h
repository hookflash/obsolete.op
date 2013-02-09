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

#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/internal/types.h>
#include <hookflash/stack/IMessageMonitor.h>
#include <hookflash/services/IRUDPICESocket.h>
#include <hookflash/services/IRUDPICESocketSession.h>
#include <hookflash/services/IRUDPMessaging.h>

#include <hookflash/stack/message/peer-finder/SessionCreateResult.h>
#include <hookflash/stack/message/peer-finder/SessionKeepAliveResult.h>
#include <hookflash/stack/message/peer-finder/SessionDeleteResult.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

#include <map>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using message::peer_finder::SessionCreateResult;
      using message::peer_finder::SessionCreateResultPtr;
      using message::peer_finder::SessionKeepAliveResult;
      using message::peer_finder::SessionKeepAliveResultPtr;
      using message::peer_finder::SessionDeleteResult;
      using message::peer_finder::SessionDeleteResultPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinderFactory
      #pragma mark

      interaction IAccountFinderFactory
      {
        static IAccountFinderFactory &singleton();

        virtual AccountFinderPtr create(
                                        IAccountFinderDelegatePtr delegate,
                                        AccountPtr outer
                                        );
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinderForAccount
      #pragma mark

      interaction IAccountFinderForAccount
      {
        typedef IAccount::AccountStates AccountStates;

        IAccountFinderForAccount &forAccount() {return *this;}
        const IAccountFinderForAccount &forAccount() const {return *this;}

        static AccountFinderPtr create(
                                       IAccountFinderDelegatePtr delegate,
                                       AccountPtr outer
                                       );
        virtual PUID getID() const = 0;
        virtual AccountStates getState() const = 0;

        virtual void shutdown() = 0;

        virtual bool send(MessagePtr message) const = 0;
        virtual IMessageMonitorPtr sendRequest(
                                               IMessageMonitorDelegatePtr delegate,
                                               MessagePtr message,
                                               Duration duration
                                               ) const = 0;

        virtual Finder getCurrentFinder(
                                        String *outServerAgent = NULL,
                                        IPAddress *outIPAddress = NULL
                                        ) const = 0;

        virtual void notifyFinderDNSComplete() = 0;
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

      class AccountFinder : public MessageQueueAssociator,
                            public IAccountFinderForAccount,
                            public IAccountFinderAsyncDelegate,
                            public IRUDPICESocketDelegate,
                            public IRUDPICESocketSessionDelegate,
                            public IRUDPMessagingDelegate,
                            public IMessageMonitorResultDelegate<SessionCreateResult>,
                            public IMessageMonitorResultDelegate<SessionKeepAliveResult>,
                            public IMessageMonitorResultDelegate<SessionDeleteResult>,
                            public ITimerDelegate
      {
      public:
        friend interaction IAccountFinderFactory;

      protected:
        AccountFinder(
                      IMessageQueuePtr queue,
                      IAccountFinderDelegatePtr delegate,
                      AccountPtr outer
                      );

        void init();

      public:
        ~AccountFinder();

        static String toDebugString(AccountFinderPtr finder, bool includeCommaPrefix = true);

      protected:

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IAccountFinderForAccount
        #pragma mark

        static AccountFinderPtr create(
                                       IAccountFinderDelegatePtr delegate,
                                       AccountPtr outer
                                       );

        virtual PUID getID() const {return mID;}

        virtual AccountStates getState() const;

        virtual void shutdown();

        virtual bool send(MessagePtr message) const;
        virtual IMessageMonitorPtr sendRequest(
                                               IMessageMonitorDelegatePtr delegate,
                                               MessagePtr message,
                                               Duration timeout
                                               ) const;

        virtual Finder getCurrentFinder(
                                        String *outServerAgent = NULL,
                                        IPAddress *outIPAddress = NULL
                                        ) const;

        virtual void notifyFinderDNSComplete();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IAccountFinderAsyncDelegate
        #pragma mark

        virtual void onStep();

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
        #pragma mark AccountFinder => IMessageMonitorResultDelegate<SessionCreateResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        SessionCreateResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             SessionCreateResultPtr ignore, // will always be NULL
                                                             MessageResultPtr result
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IMessageMonitorResultDelegate<SessionKeepAliveResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        SessionKeepAliveResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             SessionKeepAliveResultPtr ignore, // will always be NULL
                                                             MessageResultPtr result
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => IMessageMonitorResultDelegate<SessionDeleteResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        SessionDeleteResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             SessionDeleteResultPtr ignore, // will always be NULL
                                                             MessageResultPtr result
                                                             );

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

        bool isPending() const      {return IAccount::AccountState_Pending == mCurrentState;}
        bool isReady() const        {return IAccount::AccountState_Ready == mCurrentState;}
        bool isShuttingDown() const {return IAccount::AccountState_ShuttingDown == mCurrentState;}
        bool isShutdown() const     {return IAccount::AccountState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const;

        IRUDPICESocketPtr getSocket() const;

        void setTimeout(Time expires);

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void cancel();

        void step();
        bool stepSocketSubscription(IRUDPICESocketPtr socket);
        bool stepSocketSession(IRUDPICESocketPtr socket);
        bool stepMessaging();
        bool stepCreateSession();

        void setState(AccountStates state);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountFinder => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;

        AccountStates mCurrentState;

        AccountFinderWeakPtr mThisWeak;
        IAccountFinderDelegatePtr mDelegate;
        AccountWeakPtr mOuter;

        AccountFinderPtr mGracefulShutdownReference;

        IRUDPICESocketSubscriptionPtr mSocketSubscription;
        IRUDPICESocketSessionPtr mSocketSession;
        IRUDPMessagingPtr mMessaging;

        IMessageMonitorPtr mSessionCreateMonitor;
        IMessageMonitorPtr mSessionKeepAliveMonitor;
        IMessageMonitorPtr mSessionDeleteMonitor;

        Finder mFinder;
        IPAddress mFinderIP;
        String mServerAgent;
        Time mSessionCreatedTime;

        TimerPtr mKeepAliveTimer;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinderDelegate
      #pragma mark

      interaction IAccountFinderDelegate
      {
        typedef IAccount::AccountStates AccountStates;

        virtual void onAccountFinderStateChanged(
                                                 AccountFinderPtr finder,
                                                 AccountStates state
                                                 ) = 0;
        virtual void onAccountFinderMessageIncoming(
                                                    AccountFinderPtr finder,
                                                    MessagePtr message
                                                    ) = 0;
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountFinderAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountFinderDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::AccountFinderPtr, AccountFinderPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::IAccountFinderDelegate::AccountStates, AccountStates)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::message::MessagePtr, MessagePtr)
ZS_DECLARE_PROXY_METHOD_2(onAccountFinderStateChanged, AccountFinderPtr, AccountStates)
ZS_DECLARE_PROXY_METHOD_2(onAccountFinderMessageIncoming, AccountFinderPtr, MessagePtr)
ZS_DECLARE_PROXY_END()
