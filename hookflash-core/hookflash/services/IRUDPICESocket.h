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

#include <hookflash/services/hookflashTypes.h>
#include <hookflash/services/IICESocket.h>
#include <zsLib/Proxy.h>

#define HOOKFLASH_SERVICES_IRUDPSOCKET_DEFAULT_HOW_LONG_CANDIDATES_MUST_REMAIN_VALID_IN_SECONDS (10*60)

namespace hookflash
{
  namespace services
  {
    interaction IRUDPICESocket
    {
      typedef zsLib::PUID PUID;
      typedef zsLib::WORD WORD;
      typedef zsLib::Duration Duration;
      typedef zsLib::Seconds Seconds;
      typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
      typedef IICESocket::Types Types;
      typedef IICESocket::Candidate Candidate;
      typedef IICESocket::CandidateList CandidateList;
      typedef IICESocket::ICEControls ICEControls;

      enum RUDPICESocketStates
      {
        RUDPICESocketState_Pending =        IICESocket::ICESocketState_Pending,
        RUDPICESocketState_Ready =          IICESocket::ICESocketState_Ready,
        RUDPICESocketState_GoingToSleep =   IICESocket::ICESocketState_GoingToSleep,
        RUDPICESocketState_Sleeping =       IICESocket::ICESocketState_Sleeping,
        RUDPICESocketState_ShuttingDown =   IICESocket::ICESocketState_ShuttingDown,
        RUDPICESocketState_Shutdown =       IICESocket::ICESocketState_Shutdown,
      };

      static const char *toString(RUDPICESocketStates state);

      static IRUDPICESocketPtr create(
                                      IMessageQueuePtr queue,
                                      IRUDPICESocketDelegatePtr delegate,
                                      const char *turnServer,
                                      const char *turnServerUsername,
                                      const char *turnServerPassword,
                                      const char *stunServer,
                                      WORD port = 0
                                      );

      static IRUDPICESocketPtr create(
                                      IMessageQueuePtr queue,
                                      IRUDPICESocketDelegatePtr delegate,
                                      IDNS::SRVResultPtr srvTURNUDP,
                                      IDNS::SRVResultPtr srvTURNTCP,
                                      const char *turnServerUsername,
                                      const char *turnServerPassword,
                                      IDNS::SRVResultPtr srvSTUN,
                                      WORD port = 0
                                      );

      virtual PUID getID() const = 0;

      virtual RUDPICESocketStates getState() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Subscribe to the current socket state.
      virtual IRUDPICESocketSubscriptionPtr subscribe(IRUDPICESocketDelegatePtr delegate) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Close the socket and cause all sessions to become closed.
      virtual void shutdown() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: see IICESocket::prepareLocalCandidates for information
      virtual void wakeup(Duration minimumTimeCandidatesMustRemainValidWhileNotUsed = Seconds(HOOKFLASH_SERVICES_IRUDPSOCKET_DEFAULT_HOW_LONG_CANDIDATES_MUST_REMAIN_VALID_IN_SECONDS)) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Gets a local list of offered candidates
      virtual void getLocalCandidates(CandidateList &outCandidates) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Create a peer to peer connected session when the remote
      //          candidates are already known.
      virtual IRUDPICESocketSessionPtr createSessionFromRemoteCandidates(
                                                                         IRUDPICESocketSessionDelegatePtr delegate,
                                                                         const CandidateList &remoteCandidates,
                                                                         ICEControls control
                                                                         ) = 0;
    };

    interaction IRUDPICESocketSubscription
    {
      virtual void cancel() = 0;
    };

    interaction IRUDPICESocketDelegate
    {
      typedef services::IRUDPICESocketPtr IRUDPICESocketPtr;
      typedef IRUDPICESocket::RUDPICESocketStates RUDPICESocketStates;

      virtual void onRUDPICESocketStateChanged(
                                               IRUDPICESocketPtr socket,
                                               RUDPICESocketStates state
                                               ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::IRUDPICESocketDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IRUDPICESocketPtr, IRUDPICESocketPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IRUDPICESocketDelegate::RUDPICESocketStates, RUDPICESocketStates)
ZS_DECLARE_PROXY_METHOD_2(onRUDPICESocketStateChanged, IRUDPICESocketPtr, RUDPICESocketStates)
ZS_DECLARE_PROXY_END()
