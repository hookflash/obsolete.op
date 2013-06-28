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
#include <openpeer/services/IICESocket.h>
#include <openpeer/services/IICESocketSession.h>
#include <zsLib/types.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRUDPICESocketSession
    #pragma mark

    interaction IRUDPICESocketSession
    {
      typedef IICESocket::Candidate Candidate;
      typedef IICESocket::CandidateList CandidateList;
      typedef IICESocket::Types Types;
      typedef IICESocket::ICEControls ICEControls;

      enum RUDPICESocketSessionStates
      {
        RUDPICESocketSessionState_Pending,
        RUDPICESocketSessionState_Prepared,
        RUDPICESocketSessionState_Searching,
        RUDPICESocketSessionState_Ready,
        RUDPICESocketSessionState_ShuttingDown,
        RUDPICESocketSessionState_Shutdown,
      };

      static const char *toString(RUDPICESocketSessionStates state);

      enum RUDPICESocketSessionShutdownReasons
      {
        RUDPICESocketSessionShutdownReason_None                   = IICESocketSession::ICESocketSessionShutdownReason_None,

        RUDPICESocketSessionShutdownReason_Closed                 = IICESocketSession::ICESocketSessionShutdownReason_Closed,

        RUDPICESocketSessionShutdownReason_Timeout                = IICESocketSession::ICESocketSessionShutdownReason_Timeout,
        RUDPICESocketSessionShutdownReason_BackgroundingTimeout   = IICESocketSession::ICESocketSessionShutdownReason_BackgroundingTimeout,
        RUDPICESocketSessionShutdownReason_CandidateSearchFailed  = IICESocketSession::ICESocketSessionShutdownReason_CandidateSearchFailed,

        RUDPICESocketSessionShutdownReason_DelegateGone           = IICESocketSession::ICESocketSessionShutdownReason_DelegateGone,
        RUDPICESocketSessionShutdownReason_SocketGone             = IICESocketSession::ICESocketSessionShutdownReason_SocketGone,
      };

      static const char *toString(RUDPICESocketSessionShutdownReasons reason);

      virtual PUID getID() const = 0;

      virtual IRUDPICESocketPtr getSocket() = 0;

      virtual RUDPICESocketSessionStates getState() const = 0;
      virtual RUDPICESocketSessionShutdownReasons getShutdownReason() const = 0;

      virtual void shutdown() = 0;

      virtual void getLocalCandidates(CandidateList &outCandidates) = 0;
      virtual void updateRemoteCandidates(const CandidateList &remoteCandidates) = 0;

      virtual void setKeepAliveProperties(
                                          Duration sendKeepAliveIndications,
                                          Duration expectSTUNOrDataWithinWithinOrSendAliveCheck = Duration(),
                                          Duration keepAliveSTUNRequestTimeout = Duration(),
                                          Duration backgroundingTimeout = Duration()
                                          ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Although each ICE session starts off as being in a particular
      //          controlling state, the state can change due to an unintended
      //          conflict between which side is actually controlling. This
      //          yields the current (or final) controlling state of the
      //          connection.
      virtual ICEControls getConnectedControlState() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Once the connection is established, the remote IP of the
      //          current destination address will be known.
      virtual IPAddress getConnectedRemoteIP() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: When a connection is established, the nominated connection
      //          information will become known at that time.
      // RETURNS: true if a connected pair is nominated currently, otherwise
      //          false. If false the information in the out results is not
      //          valid or usable data.
      virtual bool getNominatedCandidateInformation(
                                                    Candidate &outLocal,
                                                    Candidate &outRemote
                                                    ) = 0;

      // NOTE: Will return NULL if no channel can be open at this time.
      virtual IRUDPChannelPtr openChannel(
                                          IRUDPChannelDelegatePtr delegate,
                                          const char *connectionInfo
                                          ) = 0;

      // NOTE: Will return NULL if no channel can be accepted at this time.
      virtual IRUDPChannelPtr acceptChannel(IRUDPChannelDelegatePtr delegate) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRUDPICESocketSessionDelegate
    #pragma mark

    interaction IRUDPICESocketSessionDelegate
    {
      typedef services::IRUDPICESocketSessionPtr IRUDPICESocketSessionPtr;
      typedef IRUDPICESocketSession::RUDPICESocketSessionStates RUDPICESocketSessionStates;

      virtual void onRUDPICESocketSessionStateChanged(
                                                      IRUDPICESocketSessionPtr session,
                                                      RUDPICESocketSessionStates state
                                                      ) = 0;

      virtual void onRUDPICESocketSessionChannelWaiting(IRUDPICESocketSessionPtr session) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::IRUDPICESocketSessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IRUDPICESocketSessionPtr, IRUDPICESocketSessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IRUDPICESocketSessionDelegate::RUDPICESocketSessionStates, RUDPICESocketSessionStates)
ZS_DECLARE_PROXY_METHOD_2(onRUDPICESocketSessionStateChanged, IRUDPICESocketSessionPtr, RUDPICESocketSessionStates)
ZS_DECLARE_PROXY_METHOD_1(onRUDPICESocketSessionChannelWaiting, IRUDPICESocketSessionPtr)
ZS_DECLARE_PROXY_END()
