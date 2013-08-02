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

#include <openpeer/stack/internal/types.h>

namespace openpeer
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
      #pragma mark IFinderConnection
      #pragma mark

      interaction IFinderConnection
      {
        enum SessionStates
        {
          SessionState_Pending,
          SessionState_Connected,
          SessionState_Shutdown,
        };

        static const char *toString(SessionStates state);

        static String toDebugString(IFinderConnectionPtr connection, bool includeCommaPrefix = true);

        //---------------------------------------------------------------------
        // PURPOSE: create a TCP connection to a remote finder
        // NOTE:    
        static IFinderConnectionPtr connect(
                                            IFinderConnectionDelegatePtr delegate,
                                            const IPAddress &remoteFinderIP,
                                            ITransportStreamPtr receiveStream,
                                            ITransportStreamPtr sendStream
                                            );

        //---------------------------------------------------------------------
        // PURPOSE: get object instance ID
        virtual PUID getID() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: subscribe to the relay channel events
        // NOTE:    If IFinderRelayChannelDelegatePtr() is specified, the
        //          default subscription used during object creation will be
        //          returned (thus allowing the default delegate subscription
        //          to be cancelled if needed).
        virtual IFinderConnectionSubscriptionPtr subscribe(IFinderConnectionDelegatePtr delegate) = 0;

        //---------------------------------------------------------------------
        // PURPOSE: immediately disconnects the channel (no signaling is needed)
        virtual void cancel() = 0;

        //---------------------------------------------------------------------
        // PURPOSE: return the current statte of the connection
        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: accepts and returns the latest incoming relay channel
        virtual IFinderRelayChannelPtr accept(
                                              IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                              AccountPtr account,
                                              ITransportStreamPtr receiveStream,
                                              ITransportStreamPtr sendStream
                                              ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderConnectionDelegate
      #pragma mark

      interaction IFinderConnectionDelegate
      {
        typedef IFinderConnection::SessionStates SessionStates;

        //---------------------------------------------------------------------
        // PURPOSE: Notifies the delegate that the state of the connection
        //          has changed.
        virtual void onFinderConnectionStateChanged(
                                                    IFinderConnectionPtr connection,
                                                    SessionStates state
                                                    ) = 0;

        virtual void onFinderConnectionIncomingRelayChannel(IFinderConnectionPtr connection) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderConnectionSubscription
      #pragma mark

      interaction IFinderConnectionSubscription
      {
        //---------------------------------------------------------------------
        // PURPOSE: get process unique ID for subscription object
        virtual PUID getID() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: Cancel a subscription to relay channel manager events
        virtual void cancel() = 0;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::internal::IFinderConnectionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::IFinderConnectionPtr, IFinderConnectionPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::IFinderConnection::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onFinderConnectionStateChanged, IFinderConnectionPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onFinderConnectionIncomingRelayChannel, IFinderConnectionPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_SUBSCRIPTIONS_BEGIN(openpeer::stack::internal::IFinderConnectionDelegate, openpeer::stack::internal::IFinderConnectionSubscription)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::stack::internal::IFinderConnectionPtr, IFinderConnectionPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::stack::internal::IFinderConnection::SessionStates, SessionStates)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_2(onFinderConnectionStateChanged, IFinderConnectionPtr, SessionStates)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_1(onFinderConnectionIncomingRelayChannel, IFinderConnectionPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_END()
