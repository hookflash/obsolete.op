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
    interaction IConnectionSubscription
    {
      typedef std::list<IPeerLocationPtr> PeerLocations;

      enum ConnectionStates
      {
        ConnectionState_Pending,
        ConnectionState_Connected,
        ConnectionState_Disconnecting,
        ConnectionState_Disconnected,
      };

      static const char *toString(ConnectionStates state);

      virtual bool isShutdown() const = 0;

      virtual ConnectionStates getFinderConnectionState() const = 0;

      virtual ConnectionStates getPeerLocationConnectionState(
                                                              const char *contactID,
                                                              const char *locationID
                                                              ) const = 0;

      virtual void getPeerLocations(
                                    const char *contactID,
                                    PeerLocations &outLocations,
                                    bool includeOnlyConnectedLocations
                                    ) const = 0;

      virtual bool sendFinderMessage(message::MessagePtr message) = 0;

      virtual bool sendPeerMessage(
                                   const char *contactID,
                                   const char *locationID,
                                   message::MessagePtr message
                                   ) = 0;

      virtual void cancel() = 0;
    };

    interaction IConnectionSubscriptionDelegate
    {
      typedef IConnectionSubscription::ConnectionStates ConnectionStates;

      virtual void onConnectionSubscriptionShutdown(IConnectionSubscriptionPtr subscription) = 0;

      virtual void onConnectionSubscriptionFinderConnectionStateChanged(
                                                                        IConnectionSubscriptionPtr subscription,
                                                                        ConnectionStates state
                                                                        ) = 0;

      virtual void onConnectionSubscriptionPeerLocationConnectionStateChanged(
                                                                              IConnectionSubscriptionPtr subscription,
                                                                              IPeerLocationPtr location,
                                                                              ConnectionStates state
                                                                              ) = 0;

      virtual void onConnectionSubscriptionIncomingMessage(
                                                           IConnectionSubscriptionPtr subscription,
                                                           IConnectionSubscriptionMessagePtr message
                                                           ) = 0;
    };

    interaction IConnectionSubscriptionMessage
    {
      enum Sources
      {
        Source_Finder,
        Source_Peer,
      };

      static const char *toString(Sources source);

      virtual IConnectionSubscriptionPtr getConnectionSubscription() const = 0;

      virtual Sources getSource() const = 0;

      virtual zsLib::String getPeerContactID() const = 0;
      virtual zsLib::String getPeerLocationID() const = 0;
      virtual message::MessagePtr getMessage() const = 0;

      virtual bool sendResponse(message::MessagePtr message) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IConnectionSubscriptionDelegate)
ZS_DECLARE_PROXY_METHOD_1(onConnectionSubscriptionShutdown, hookflash::stack::IConnectionSubscriptionPtr)
ZS_DECLARE_PROXY_METHOD_2(onConnectionSubscriptionFinderConnectionStateChanged, hookflash::stack::IConnectionSubscriptionPtr, hookflash::stack::IConnectionSubscriptionDelegate::ConnectionStates)
ZS_DECLARE_PROXY_METHOD_3(onConnectionSubscriptionPeerLocationConnectionStateChanged, hookflash::stack::IConnectionSubscriptionPtr, hookflash::stack::IPeerLocationPtr, hookflash::stack::IConnectionSubscriptionDelegate::ConnectionStates)
ZS_DECLARE_PROXY_METHOD_2(onConnectionSubscriptionIncomingMessage, hookflash::stack::IConnectionSubscriptionPtr, hookflash::stack::IConnectionSubscriptionMessagePtr)
ZS_DECLARE_PROXY_END()
