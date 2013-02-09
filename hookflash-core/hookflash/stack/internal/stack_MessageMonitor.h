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

#include <hookflash/stack/IMessageMonitor.h>
#include <hookflash/stack/internal/types.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

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
      #pragma mark IMessageMonitorFactory
      #pragma mark

      interaction IMessageMonitorFactory
      {
        static IMessageMonitorFactory &singleton();

        virtual MessageMonitorPtr monitor(
                                          IMessageMonitorDelegatePtr delegate,
                                          message::MessagePtr requestMessage,
                                          Duration timeout
                                          );

        virtual MessageMonitorPtr monitorAndSendToLocation(
                                                           IMessageMonitorDelegatePtr delegate,
                                                           ILocationPtr peerLocation,
                                                           message::MessagePtr message,
                                                           Duration timeout
                                                           );

        virtual MessageMonitorPtr monitorAndSendToService(
                                                          IMessageMonitorDelegatePtr delegate,
                                                          IBootstrappedNetworkPtr bootstrappedNetwork,
                                                          const char *serviceType,
                                                          const char *serviceMethodName,
                                                          message::MessagePtr message,
                                                          Duration timeout
                                                          );
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorForAccountFinder
      #pragma mark

      interaction IMessageMonitorForAccountFinder
      {
        IMessageMonitorForAccountFinder &forAccountFinder() {return *this;}
        const IMessageMonitorForAccountFinder &forAccountFinder() const {return *this;}

        virtual void notifyMessageSendFailed() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMontiorForAccountPeerLocation
      #pragma mark

      interaction IMessageMonitorForAccountPeerLocation
      {
        IMessageMonitorForAccountPeerLocation &forAccountPeerLocation() {return *this;}
        const IMessageMonitorForAccountPeerLocation &forAccountPeerLocation() const {return *this;}

        virtual void notifyMessageSendFailed() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorForMessageMonitorManager
      #pragma mark

      interaction IMessageMonitorForMessageMonitorManager
      {
        IMessageMonitorForMessageMonitorManager &forMessageMonitorManager() {return *this;}
        const IMessageMonitorForMessageMonitorManager &forMessageMonitorManager() const {return *this;}

        virtual bool handleMessage(message::MessagePtr message) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorAsyncDelegate
      #pragma mark

      interaction IMessageMonitorAsyncDelegate
      {
        virtual void onHandleMessage(
                                     IMessageMonitorDelegatePtr delegate,
                                     MessagePtr message
                                     ) = 0;

        virtual void onAutoHandleFailureResult(MessageResultPtr result) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark
      #pragma mark

      class MessageMonitor : public MessageQueueAssociator,
                             public IMessageMonitor,
                             public IMessageMonitorAsyncDelegate,
                             public IMessageMonitorForAccountFinder,
                             public IMessageMonitorForAccountPeerLocation,
                             public IMessageMonitorForMessageMonitorManager,
                             public ITimerDelegate
      {
      public:
        friend interaction IMessageMonitorFactory;
        friend interaction IMessageMonitor;

      protected:
        MessageMonitor(IMessageQueuePtr queue);

        void init(Duration timeout);

      public:
        ~MessageMonitor();

        static MessageMonitorPtr convert(IMessageMonitorPtr monitor);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageMonitor => IMessageMonitor
        #pragma mark

        static String toDebugString(IMessageMonitorPtr monitor, bool includeCommaPrefix = true);

        static MessageMonitorPtr monitor(
                                         IMessageMonitorDelegatePtr delegate,
                                         message::MessagePtr requestMessage,
                                         Duration timeout
                                         );

        static MessageMonitorPtr monitorAndSendToLocation(
                                                          IMessageMonitorDelegatePtr delegate,
                                                          ILocationPtr peerLocation,
                                                          message::MessagePtr message,
                                                          Duration timeout
                                                          );

        static MessageMonitorPtr monitorAndSendToService(
                                                         IMessageMonitorDelegatePtr delegate,
                                                         IBootstrappedNetworkPtr bootstrappedNetwork,
                                                         const char *serviceType,
                                                         const char *serviceMethodName,
                                                         message::MessagePtr message,
                                                         Duration timeout
                                                         );

        static bool handleMessageReceived(message::MessagePtr message);

        virtual PUID getID() const {return mID;}

        virtual bool isComplete() const;
        virtual bool wasHandled() const;

        virtual void cancel();

        virtual String getMonitoredMessageID();
        virtual message::MessagePtr getMonitoredMessage();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageMonitor => IMessageMonitorAsyncDelegate
        #pragma mark

        virtual void onHandleMessage(
                                     IMessageMonitorDelegatePtr delegate,
                                     message::MessagePtr message
                                     );

        virtual void onAutoHandleFailureResult(MessageResultPtr result);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageMonitor => IMessageMonitorForAccountFinder
        #pragma mark

        virtual void notifyMessageSendFailed();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageMonitor => IMessageMonitorForAccountPeerLocation
        #pragma mark

        // (duplicate) virtual void notifyMessageSendFailed();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageMonitor => IMessageMonitorForMessageMonitorManager
        #pragma mark

        virtual bool handleMessage(message::MessagePtr message);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageMonitor => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageMonitor => (internal)
        #pragma mark

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        RecursiveLock &getLock() const;

      protected:
        PUID mID;
        mutable RecursiveLock mBogusLock;
        MessageMonitorWeakPtr mThisWeak;

        IMessageMonitorDelegatePtr mDelegate;

        bool mWasHandled;
        bool mTimeoutFired;
        ULONG mPendingHandled;

        TimerPtr mTimer;
        String mMessageID;
        message::MessagePtr mOriginalMessage;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IMessageMonitorAsyncDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IMessageMonitorDelegatePtr, IMessageMonitorDelegatePtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::message::MessagePtr, MessagePtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::message::MessageResultPtr, MessageResultPtr)
ZS_DECLARE_PROXY_METHOD_2(onHandleMessage, IMessageMonitorDelegatePtr, MessagePtr)
ZS_DECLARE_PROXY_METHOD_1(onAutoHandleFailureResult, MessageResultPtr)
ZS_DECLARE_PROXY_END()
