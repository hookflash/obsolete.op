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

#include <openpeer/core/types.h>

namespace openpeer
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ICall
    #pragma mark

    interaction ICall
    {
      enum CallStates
      {
        CallState_None,       // call has no state yet
        CallState_Preparing,  // call is negotiating in the background - do not present this call to a user yet...
        CallState_Incoming,   // call is incoming from a remote party
        CallState_Placed,     // call has been placed to the remote party
        CallState_Early,      // call is outgoing to a remote party and is receiving early media (media before being answered)
        CallState_Ringing,    // call is incoming from a remote party and is ringing
        CallState_Ringback,   // call is outgoing to a remote party and remote party is ringing
        CallState_Open,       // call is open
        CallState_Active,     // call is open, and participant is actively communicating
        CallState_Inactive,   // call is open, and participant is inactive
        CallState_Hold,       // call is open but on hold
        CallState_Closing,    // call is hanging up
        CallState_Closed,     // call has ended
      };

      enum CallClosedReasons
      {
        CallClosedReason_None                     = 0,

        CallClosedReason_User                     = 200,

        CallClosedReason_RequestTimeout           = 408,
        CallClosedReason_TemporarilyUnavailable   = 480,
        CallClosedReason_Busy                     = 486,
        CallClosedReason_RequestTerminated        = 487,
        CallClosedReason_NotAcceptableHere        = 488,

        CallClosedReason_ServerInternalError      = 500,

        CallClosedReason_Decline                  = 603,
      };

      static const char *toString(CallStates state);
      static const char *toString(CallClosedReasons reason);

      static String toDebugString(ICallPtr call, bool includeCommaPrefix = true);

      static ICallPtr placeCall(
                                IConversationThreadPtr conversationThread,
                                IContactPtr toContact,
                                bool includeAudio,
                                bool includeVideo
                                );

      virtual PUID getID() const = 0;

      virtual String getCallID() const = 0;

      virtual IConversationThreadPtr getConversationThread() const = 0;

      virtual IContactPtr getCaller() const = 0;
      virtual IContactPtr getCallee() const = 0;

      virtual bool hasAudio() const = 0;
      virtual bool hasVideo() const = 0;

      virtual CallStates getState() const = 0;  // state is always relative to "this" location, be is caller or callee
      virtual CallClosedReasons getClosedReason() const = 0;

      virtual Time getcreationTime() const = 0;
      virtual Time getRingTime() const = 0;
      virtual Time getAnswerTime() const = 0;
      virtual Time getClosedTime() const = 0;

      virtual void ring() = 0;          // tell the caller that the call is ringing
      virtual void answer() = 0;        // answer the call
      virtual void hold(bool hold) = 0; // place the call on hold (or remove from hold)
      virtual void hangup(CallClosedReasons reason = CallClosedReason_User) = 0;        // end the call
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ICallDelegate
    #pragma mark

    interaction ICallDelegate
    {
      typedef ICall::CallStates CallStates;

      virtual void onCallStateChanged(
                                      ICallPtr call,
                                      CallStates state
                                      ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::ICallDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::core::ICallPtr, ICallPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::core::ICall::CallStates, CallStates)
ZS_DECLARE_PROXY_METHOD_2(onCallStateChanged, ICallPtr, CallStates)
ZS_DECLARE_PROXY_END()
