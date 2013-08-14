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

#include <openpeer/services/types.h>
#include <openpeer/services/IHTTP.h>

#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>
#include <zsLib/String.h>

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRUDPChannel
    #pragma mark

    interaction IRUDPChannel
    {
      enum RUDPChannelStates
      {
        RUDPChannelState_Connecting,
        RUDPChannelState_Connected,
        RUDPChannelState_ShuttingDown,
        RUDPChannelState_Shutdown,
      };

      static const char *toString(RUDPChannelStates state);

      enum RUDPChannelShutdownReasons
      {
        RUDPChannelShutdownReason_None                = IHTTP::HTTPStatusCode_None,

        RUDPChannelShutdownReason_OpenFailure         = IHTTP::HTTPStatusCode_NoResponse,
        RUDPChannelShutdownReason_DelegateGone        = IHTTP::HTTPStatusCode_Gone,
        RUDPChannelShutdownReason_Timeout             = IHTTP::HTTPStatusCode_RequestTimeout,
        RUDPChannelShutdownReason_IllegalStreamState  = IHTTP::HTTPStatusCode_InternalServerError,
      };

      static const char *toString(RUDPChannelShutdownReasons reason);

      enum Shutdown
      {
        Shutdown_None =     (0x00),
        Shutdown_Send =     (0x01),
        Shutdown_Receive =  (0x02),
        Shutdown_Both =     (0x03)
      };

      static const char *toString(Shutdown value);

      enum CongestionAlgorithms
      {
        CongestionAlgorithm_None =                         0,
        CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp = 1,
      };

      static const char *toString(CongestionAlgorithms value);

      //-----------------------------------------------------------------------
      // PURPOSE: returns a debug string containing internal object state
      static String toDebugString(IRUDPChannelPtr channel, bool includeCommaPrefix = true);

      virtual PUID getID() const = 0;

      virtual RUDPChannelStates getState(
                                         WORD *outLastErrorCode = NULL,
                                         String *outLastErrorReason = NULL
                                         ) const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This closes the session gracefully.
      virtual void shutdown() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This shutsdown the send/receive state but does not close the
      //          channel session. Use shutdown() to actually shutdown the
      //          channel fully.
      virtual void shutdownDirection(Shutdown state) = 0;

      virtual IPAddress getConnectedRemoteIP() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Return connection information as reported by the remote party
      // NOTE:    Will return an empty string until connected.
      virtual String getRemoteConnectionInfo() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRUDPChannelDelegate
    #pragma mark

    interaction IRUDPChannelDelegate
    {
      typedef services::IRUDPChannelPtr IRUDPChannelPtr;
      typedef IRUDPChannel::RUDPChannelStates RUDPChannelStates;

      virtual void onRDUPChannelStateChanged(
                                             IRUDPChannelPtr session,
                                             RUDPChannelStates state
                                             ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::IRUDPChannelDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IRUDPChannelPtr, IRUDPChannelPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IRUDPChannelDelegate::RUDPChannelStates, RUDPChannelStates)
ZS_DECLARE_PROXY_METHOD_2(onRDUPChannelStateChanged, IRUDPChannelPtr, RUDPChannelStates)
ZS_DECLARE_PROXY_END()
