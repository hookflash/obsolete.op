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

#include <hookflash/stack/hookflashTypes.h>
#include <hookflash/stack/message/hookflashTypes.h>

#include <zsLib/Proxy.h>
#include <zsLib/String.h>

namespace hookflash
{
  namespace stack
  {
    interaction IPeerSubscription
    {
      typedef zsLib::PUID PUID;
      typedef zsLib::String String;

      enum PeerSubscriptionFindStates
      {
        PeerSubscriptionFindState_Idle,
        PeerSubscriptionFindState_Finding,
        PeerSubscriptionFindState_Completed,
      };

      typedef stack::LocationList LocationList;
      typedef std::list<IPeerLocationPtr> PeerLocationList;

      static const char *toString(PeerSubscriptionFindStates state);

      virtual PUID getID() const = 0;

      virtual bool isShutdown() = 0;

      virtual String getContactID() = 0;

      virtual PeerSubscriptionFindStates getFindState() const = 0;

      virtual void getPeerLocations(
                                    LocationList &outLocations,
                                    bool includeOnlyConnectedLocations
                                    ) = 0;
      virtual void getPeerLocations(
                                    PeerLocationList &outPeerLocations,
                                    bool includeOnlyConnectedLocations
                                    ) = 0;

      virtual bool sendPeerMesage(
                                  const char *locationID,
                                  message::MessagePtr message
                                  ) = 0;

      virtual void cancel() = 0;
    };

    interaction IPeerSubscriptionMessage
    {
      typedef zsLib::String String;

      virtual IPeerSubscriptionPtr getPeerSubscription() const = 0;

      virtual String getContactID() const = 0;
      virtual String getLocationID() const = 0;
      virtual message::MessagePtr getMessage() const = 0;

      virtual bool sendResponse(message::MessagePtr message) = 0;
    };

    interaction IPeerSubscriptionDelegate
    {
      typedef IPeerSubscription::PeerSubscriptionFindStates PeerSubscriptionFindStates;

      virtual void onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription) = 0;

      virtual void onPeerSubscriptionFindStateChanged(
                                                      IPeerSubscriptionPtr subscription,
                                                      PeerSubscriptionFindStates state
                                                      ) = 0;

      virtual void onPeerSubscriptionLocationsChanged(IPeerSubscriptionPtr subscription) = 0;

      virtual void onPeerSubscriptionMessage(
                                             IPeerSubscriptionPtr subscription,
                                             IPeerSubscriptionMessagePtr incomingMessage
                                             ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IPeerSubscriptionDelegate)
ZS_DECLARE_PROXY_METHOD_1(onPeerSubscriptionShutdown, hookflash::stack::IPeerSubscriptionPtr)
ZS_DECLARE_PROXY_METHOD_2(onPeerSubscriptionFindStateChanged, hookflash::stack::IPeerSubscriptionPtr, hookflash::stack::IPeerSubscriptionDelegate::PeerSubscriptionFindStates)
ZS_DECLARE_PROXY_METHOD_1(onPeerSubscriptionLocationsChanged, hookflash::stack::IPeerSubscriptionPtr)
ZS_DECLARE_PROXY_METHOD_2(onPeerSubscriptionMessage, hookflash::stack::IPeerSubscriptionPtr, hookflash::stack::IPeerSubscriptionMessagePtr)
ZS_DECLARE_PROXY_END()
