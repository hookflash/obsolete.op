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
      #pragma mark IFinderConnectionRelayChannel
      #pragma mark

      interaction IFinderConnectionRelayChannel
      {
        enum SessionStates
        {
          SessionState_Pending,
          SessionState_Connected,
          SessionState_Shutdown,
        };

        static const char *toString(SessionStates state);

        static String toDebugString(IFinderRelayChannelPtr channel, bool includeCommaPrefix = true);

        //---------------------------------------------------------------------
        // PURPOSE: create a TCP connection to a remote finder
        // NOTE:    
        static IFinderConnectionRelayChannelPtr connect(
                                                        IFinderConnectionRelayChannelDelegatePtr delegate,
                                                        const IPAddress &remoteFinderIP,
                                                        const char *localContextID,
                                                        const char *relayAccessToken,
                                                        const char *relayAccessSecretProof,
                                                        ITransportStreamPtr receiveStream,
                                                        ITransportStreamPtr sendStream
                                                        );

        //---------------------------------------------------------------------
        // PURPOSE: get object instance ID
        virtual PUID getID() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: immediately disconnects the channel (no signaling is needed)
        virtual void cancel() = 0;

        //---------------------------------------------------------------------
        // PURPOSE: return the current statte of the connection
        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderConnectionRelayChannelDelegate
      #pragma mark

      interaction IFinderConnectionRelayChannelDelegate
      {
        typedef IFinderConnectionRelayChannel::SessionStates SessionStates;

        //---------------------------------------------------------------------
        // PURPOSE: Notifies the delegate that the state of the connection
        //          has changed.
        virtual void onFinderConnectionRelayChannelStateChanged(
                                                                IFinderConnectionRelayChannelPtr channel,
                                                                SessionStates state
                                                                ) = 0;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::internal::IFinderConnectionRelayChannelDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::IFinderConnectionRelayChannelPtr, IFinderConnectionRelayChannelPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::IFinderConnectionRelayChannel::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onFinderConnectionRelayChannelStateChanged, IFinderConnectionRelayChannelPtr, SessionStates)
ZS_DECLARE_PROXY_END()
