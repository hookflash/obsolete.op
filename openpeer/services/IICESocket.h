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
#include <openpeer/services/IDNS.h>
#include <zsLib/types.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>

#include <list>

#define OPENPEER_SERVICES_IICESOCKET_DEFAULT_HOW_LONG_CANDIDATES_MUST_REMAIN_VALID_IN_SECONDS (10*60)

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IICESocket
    #pragma mark

    interaction IICESocket
    {
      enum ICESocketStates
      {
        ICESocketState_Pending,
        ICESocketState_Ready,
        ICESocketState_GoingToSleep,
        ICESocketState_Sleeping,
        ICESocketState_ShuttingDown,
        ICESocketState_Shutdown,
      };

      static const char *toString(ICESocketStates state);

      enum Types
      {
        Type_Unknown =          1,
        Type_Local =            126,
        Type_ServerReflexive =  100,
        Type_PeerReflexive =    50,
        Type_Relayed =          0,
      };

      static const char *toString(Types type);

      struct Candidate
      {
        Types     mType;
        String    mFoundation;
        IPAddress mIPAddress;
        DWORD     mPriority;
        WORD      mLocalPreference;  // fill with "0" if unknown

        IPAddress mRelatedIP;         // if server reflexive, peer reflexive or relayed, the related base IP

        Candidate(): mType(Type_Unknown), mPriority(0), mLocalPreference(0) {}
        bool hasData() const;
        String toDebugString(bool includeCommaPrefix = true) const;
      };

      typedef std::list<Candidate> CandidateList;
      static void compare(
                          const CandidateList &inOldCandidatesList,
                          const CandidateList &inNewCandidatesList,
                          CandidateList &outAddedCandidates,
                          CandidateList &outRemovedCandidates
                          );

      enum ICEControls
      {
        ICEControl_Controlling,
        ICEControl_Controlled,
      };

      static const char *toString(ICEControls control);
      
      //-----------------------------------------------------------------------
      // PURPOSE: returns a debug string containing internal object state
      static String toDebugString(IICESocketPtr socket, bool includeCommaPrefix = true);

      //-----------------------------------------------------------------------
      // PURPOSE: creates a socket and resolves STUN/TURN servers
      static IICESocketPtr create(
                                  IMessageQueuePtr queue,
                                  IICESocketDelegatePtr delegate,
                                  const char *turnServer,
                                  const char *turnServerUsername,
                                  const char *turnServerPassword,
                                  const char *stunServer,
                                  WORD port = 0,
                                  bool firstWORDInAnyPacketWillNotConflictWithTURNChannels = false,
                                  IICESocketPtr foundationSocket = IICESocketPtr()
                                  );

      //-----------------------------------------------------------------------
      // PURPOSE: creates a socket using existing resolved STUN/TURN SRV
      //          results.
      static IICESocketPtr create(
                                  IMessageQueuePtr queue,
                                  IICESocketDelegatePtr delegate,
                                  IDNS::SRVResultPtr srvTURNUDP,
                                  IDNS::SRVResultPtr srvTURNTCP,
                                  const char *turnServerUsername,
                                  const char *turnServerPassword,
                                  IDNS::SRVResultPtr srvSTUN,
                                  WORD port = 0,
                                  bool firstWORDInAnyPacketWillNotConflictWithTURNChannels = false,
                                  IICESocketPtr foundationSocket = IICESocketPtr()
                                  );

      //-----------------------------------------------------------------------
      // PURPOSE: returns the unique object ID
      virtual PUID getID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Subscribe to the current socket state.
      virtual IICESocketSubscriptionPtr subscribe(IICESocketDelegatePtr delegate) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Gets the current state of the object
      virtual ICESocketStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Gets the ICE username fragment
      virtual String getUsernameFrag() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Gets the ICE password
      virtual String getPassword() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Close the socket and cause all sessions to become closed.
      virtual void shutdown() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Call to wakeup a potentially sleeping socket so that all
      //          local candidates are prepared.
      // NOTE:    Each an every time that local candidates are to be obtained,
      //          this method must be called first to ensure that all services
      //          are ready. For example, TURN is shutdown while not in use
      //          and it must become active otherwise the TURN candidates will
      //          not be available.
      virtual void wakeup(Duration minimumTimeCandidatesMustRemainValidWhileNotUsed = Seconds(OPENPEER_SERVICES_IICESOCKET_DEFAULT_HOW_LONG_CANDIDATES_MUST_REMAIN_VALID_IN_SECONDS)) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Gets a local list of offered candidates
      virtual void getLocalCandidates(CandidateList &outCandidates) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Create a peer to peer connected session when the remote
      //          candidates are already known.
      virtual IICESocketSessionPtr createSessionFromRemoteCandidates(
                                                                     IICESocketSessionDelegatePtr delegate,
                                                                     const char *remoteUsernameFrag,
                                                                     const char *remotePassword,
                                                                     const CandidateList &remoteCandidates,
                                                                     ICEControls control,
                                                                     IICESocketSessionPtr foundation = IICESocketSessionPtr()
                                                                     ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Enable or disable write ready notifications on all sessions
      virtual void monitorWriteReadyOnAllSessions(bool monitor = true) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IICESocketDelegate
    #pragma mark

    interaction IICESocketDelegate
    {
      typedef services::IICESocketPtr IICESocketPtr;
      typedef IICESocket::ICESocketStates ICESocketStates;

      virtual void onICESocketStateChanged(
                                           IICESocketPtr socket,
                                           ICESocketStates state
                                           ) = 0;

      virtual void onICESocketCandidatesChanged(IICESocketPtr socket) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IICESocketSubscription
    #pragma mark

    interaction IICESocketSubscription
    {
      virtual PUID getID() const = 0;

      virtual void cancel() = 0;

      virtual void background() = 0;
    };
    
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::IICESocketDelegate)
ZS_DECLARE_PROXY_METHOD_2(onICESocketStateChanged, openpeer::services::IICESocketPtr, openpeer::services::IICESocketDelegate::ICESocketStates)
ZS_DECLARE_PROXY_METHOD_1(onICESocketCandidatesChanged, openpeer::services::IICESocketPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_SUBSCRIPTIONS_BEGIN(openpeer::services::IICESocketDelegate, openpeer::services::IICESocketSubscription)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_2(onICESocketStateChanged, openpeer::services::IICESocketPtr, openpeer::services::IICESocketDelegate::ICESocketStates)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_1(onICESocketCandidatesChanged, openpeer::services::IICESocketPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_END()
