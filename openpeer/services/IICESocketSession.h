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
#include <zsLib/types.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>

namespace hookflash
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IICESocketSession
    #pragma mark

    interaction IICESocketSession
    {
      typedef IICESocket::Candidate Candidate;
      typedef IICESocket::CandidateList CandidateList;
      typedef IICESocket::Types Types;
      typedef IICESocket::ICEControls ICEControls;

      enum ICESocketSessionStates
      {
        ICESocketSessionState_Pending,
        ICESocketSessionState_Prepared,
        ICESocketSessionState_Searching,
        ICESocketSessionState_Nominating,
        ICESocketSessionState_Nominated,
        ICESocketSessionState_Shutdown,
      };

      static const char *toString(ICESocketSessionStates state);

      enum ICESocketSessionShutdownReasons
      {
        ICESocketSessionShutdownReason_None,

        ICESocketSessionShutdownReason_Closed,

        ICESocketSessionShutdownReason_Timeout,
        ICESocketSessionShutdownReason_BackgroundingTimeout,
        ICESocketSessionShutdownReason_CandidateSearchFailed,

        ICESocketSessionShutdownReason_DelegateGone,
        ICESocketSessionShutdownReason_SocketGone,
      };

      static const char *toString(ICESocketSessionShutdownReasons reason);

      virtual PUID getID() const = 0;

      virtual IICESocketPtr getSocket() = 0;

      virtual ICESocketSessionStates getState() const = 0;
      virtual ICESocketSessionShutdownReasons getShutdownReason() const = 0;

      virtual void close() = 0;

      virtual void getLocalCandidates(CandidateList &outCandidates) = 0;
      virtual void updateRemoteCandidates(const CandidateList &remoteCandidates) = 0;

      virtual void setKeepAliveProperties(
                                          Duration sendKeepAliveIndications,
                                          Duration expectSTUNOrDataWithinWithinOrSendAliveCheck = Duration(),
                                          Duration keepAliveSTUNRequestTimeout = Duration(),
                                          Duration backgroundingTimeout = Duration()
                                          ) = 0;

      virtual bool sendPacket(
                              const BYTE *packet,
                              ULONG packetLengthInBytes
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
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IICESocketSessionDelegate
    #pragma mark

    interaction IICESocketSessionDelegate
    {
      typedef services::IICESocketSessionPtr IICESocketSessionPtr;
      typedef services::STUNPacketPtr STUNPacketPtr;
      typedef IICESocketSession::ICESocketSessionStates ICESocketSessionStates;

      virtual void onICESocketSessionStateChanged(
                                                  IICESocketSessionPtr session,
                                                  ICESocketSessionStates state
                                                  ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Pushes a received packet to the delegate to be processed
      //          immediately upon receipt.
      virtual void handleICESocketSessionReceivedPacket(
                                                        IICESocketSessionPtr session,
                                                        const BYTE *buffer,
                                                        ULONG bufferLengthInBytes
                                                        ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Allows the delegate to handle an incoming STUN packet that
      //          was not meant for the ICE socket.
      virtual bool handleICESocketSessionReceivedSTUNPacket(
                                                            IICESocketSessionPtr session,
                                                            STUNPacketPtr stun,
                                                            const String &localUsernameFrag,
                                                            const String &remoteUsernameFrag
                                                            ) = 0;

      virtual void onICESocketSessionWriteReady(IICESocketSessionPtr session) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::IICESocketSessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IICESocketSessionPtr, IICESocketSessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::STUNPacketPtr, STUNPacketPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IICESocketSessionDelegate::ICESocketSessionStates, ICESocketSessionStates)
ZS_DECLARE_PROXY_METHOD_2(onICESocketSessionStateChanged, IICESocketSessionPtr, hookflash::services::IICESocketSessionDelegate::ICESocketSessionStates)
ZS_DECLARE_PROXY_METHOD_SYNC_3(handleICESocketSessionReceivedPacket, IICESocketSessionPtr, const BYTE *, ULONG)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_4(handleICESocketSessionReceivedSTUNPacket, bool, IICESocketSessionPtr, STUNPacketPtr, const String &, const String &)
ZS_DECLARE_PROXY_METHOD_1(onICESocketSessionWriteReady, hookflash::services::IICESocketSessionPtr)
ZS_DECLARE_PROXY_END()
