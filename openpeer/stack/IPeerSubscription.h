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

#include <openpeer/stack/types.h>
#include <openpeer/stack/message/types.h>
#include <openpeer/stack/IPeer.h>
#include <openpeer/stack/ILocation.h>

namespace hookflash
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPeerSubscription
    #pragma mark

    interaction IPeerSubscription
    {
      static String toDebugString(IPeerSubscriptionPtr subscription, bool includeCommaPrefix = true);

      static IPeerSubscriptionPtr subscribeAll(
                                               IAccountPtr account,
                                               IPeerSubscriptionDelegatePtr delegate
                                               );

      static IPeerSubscriptionPtr subscribe(
                                            IPeerPtr peer,
                                            IPeerSubscriptionDelegatePtr delegate
                                            );

      virtual PUID getID() const = 0;

      virtual IPeerPtr getSubscribedToPeer() const = 0;   // return IPeerPtr() if the subscription is for all peers, not a single peer

      virtual bool isShutdown() const = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPeerSubscriptionDelegate
    #pragma mark

    interaction IPeerSubscriptionDelegate
    {
      typedef ILocation::LocationConnectionStates LocationConnectionStates;
      typedef IPeer::PeerFindStates PeerFindStates;

      virtual void onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription) = 0;

      virtual void onPeerSubscriptionFindStateChanged(
                                                      IPeerSubscriptionPtr subscription,
                                                      IPeerPtr peer,
                                                      PeerFindStates state
                                                      ) = 0;

      virtual void onPeerSubscriptionLocationConnectionStateChanged(
                                                                    IPeerSubscriptionPtr subscription,
                                                                    ILocationPtr location,
                                                                    LocationConnectionStates state
                                                                    ) = 0;

      virtual void onPeerSubscriptionMessageIncoming(
                                                     IPeerSubscriptionPtr subscription,
                                                     IMessageIncomingPtr message
                                                     ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IPeerSubscriptionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPeerSubscriptionPtr, IPeerSubscriptionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPeerPtr, IPeerPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::ILocationPtr, ILocationPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IMessageIncomingPtr, IMessageIncomingPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPeerSubscriptionDelegate::LocationConnectionStates, LocationConnectionStates)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IPeerSubscriptionDelegate::PeerFindStates, PeerFindStates)
ZS_DECLARE_PROXY_METHOD_1(onPeerSubscriptionShutdown, IPeerSubscriptionPtr)
ZS_DECLARE_PROXY_METHOD_3(onPeerSubscriptionFindStateChanged, IPeerSubscriptionPtr, IPeerPtr, PeerFindStates)
ZS_DECLARE_PROXY_METHOD_3(onPeerSubscriptionLocationConnectionStateChanged, IPeerSubscriptionPtr, ILocationPtr, LocationConnectionStates)
ZS_DECLARE_PROXY_METHOD_2(onPeerSubscriptionMessageIncoming, IPeerSubscriptionPtr, IMessageIncomingPtr)
ZS_DECLARE_PROXY_END()
