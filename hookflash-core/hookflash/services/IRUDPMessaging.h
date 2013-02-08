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

#include <hookflash/services/types.h>
#include <hookflash/services/IRUDPChannel.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>
#include <zsLib/String.h>
#include <boost/shared_array.hpp>

#define HOOKFLASH_SERVICES_IRDUPMESSAGING_MAX_MESSAGE_SIZE_IN_BYTES (0xFFFFF)

namespace hookflash
{
  namespace services
  {
    interaction IRUDPMessaging
    {
      typedef boost::shared_array<BYTE> MessageBuffer;
      typedef IRUDPChannel::Shutdown Shutdown;

      enum RUDPMessagingStates
      {
        RUDPMessagingState_Connecting =   IRUDPChannel::RUDPChannelState_Connecting,
        RUDPMessagingState_Connected =    IRUDPChannel::RUDPChannelState_Connected,
        RUDPMessagingState_ShuttingDown = IRUDPChannel::RUDPChannelState_ShuttingDown,
        RUDPMessagingState_Shutdown =     IRUDPChannel::RUDPChannelState_Shutdown,
      };

      static const char *toString(RUDPMessagingStates state);

      enum RUDPMessagingShutdownReasons
      {
        RUDPMessagingShutdownReason_None                        = IRUDPChannel::RUDPChannelShutdownReason_None,

        RUDPMessagingShutdownReason_Closed                      = IRUDPChannel::RUDPChannelShutdownReason_Closed,

        RUDPMessagingShutdownReason_OpenFailure                 = IRUDPChannel::RUDPChannelShutdownReason_OpenFailure,
        RUDPMessagingShutdownReason_DelegateGone                = IRUDPChannel::RUDPChannelShutdownReason_DelegateGone,
        RUDPMessagingShutdownReason_Timeout                     = IRUDPChannel::RUDPChannelShutdownReason_Timeout,
        RUDPMessagingShutdownReason_IllegalStreamState          = IRUDPChannel::RUDPChannelShutdownReason_IllegalStreamState,
      };

      static const char *toString(RUDPMessagingShutdownReasons reason);

      //-----------------------------------------------------------------------
      // PURPOSE: Creates a messaging object from an RUDP listener by accepting
      //          the channel and processing it.
      // NOTE:    Will return NULL if the accept failed to open (i.e. the
      //          accepted channel closed before the accept was called).
      static IRUDPMessagingPtr acceptChannel(
                                             IMessageQueuePtr queue,
                                             IRUDPListenerPtr listener,
                                             IRUDPMessagingDelegatePtr delegate,
                                             ULONG maxMessageSizeInBytes = HOOKFLASH_SERVICES_IRDUPMESSAGING_MAX_MESSAGE_SIZE_IN_BYTES
                                             );

      //-----------------------------------------------------------------------
      // PURPOSE: Creates a messaging object from an RUDP socket session
      //          by accepting the channel and processing it.
      // NOTE:    Will return NULL if the accept failed to open (i.e. the
      //          accepted channel closed before the accept was called).
      static IRUDPMessagingPtr acceptChannel(
                                             IMessageQueuePtr queue,
                                             IRUDPICESocketSessionPtr session,
                                             IRUDPMessagingDelegatePtr delegate,
                                             ULONG maxMessageSizeInBytes = HOOKFLASH_SERVICES_IRDUPMESSAGING_MAX_MESSAGE_SIZE_IN_BYTES
                                             );

      //-----------------------------------------------------------------------
      // PURPOSE: Creates a messaging object from an RUDP socket session
      //          by issuing a openChannel and then processing the results.
      // NOTE:    Will return NULL if the accept failed to open (i.e. the
      //          RUDP session was closed before this was called).
      static IRUDPMessagingPtr openChannel(
                                           IMessageQueuePtr queue,
                                           IRUDPICESocketSessionPtr session,
                                           IRUDPMessagingDelegatePtr delegate,
                                           const char *connectionInfo,
                                           ULONG maxMessageSizeInBytes = HOOKFLASH_SERVICES_IRDUPMESSAGING_MAX_MESSAGE_SIZE_IN_BYTES
                                           );

      virtual PUID getID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Get the current state of the messaging
      virtual RUDPMessagingStates getState() const = 0;

      virtual RUDPMessagingShutdownReasons getShutdownReason() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This closes the session gracefully.
      virtual void shutdown() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This shutsdown the send/receive state but does not close the
      //          channel session. Use shutdown() to actually shutdown the
      //          channel fully.
      virtual void shutdownDirection(Shutdown state) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Send a message over the channel.
      virtual bool send(
                        const BYTE *message,
                        ULONG messsageLengthInBytes
                        ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Set the maximum size of a message expecting to receive
      virtual void setMaxMessageSizeInBytes(ULONG maxMessageSizeInBytes) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Allocate a buffer large enough to hold the next message
      //          available.
      // NOTE:    Will return NULL if no buffer available.
      virtual MessageBuffer getBufferLargeEnoughForNextMessage() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains how big the next message in the channel will be.
      // NOTE:    Will return "0" if there is no message available yet.
      virtual ULONG getNextReceivedMessageSizeInBytes() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains the next message to process from the channel.
      // WARNING: The buffer must be as large as returned by
      //          "getNextReceivedMessageSizeInBytes()"
      virtual ULONG receive(BYTE *outBuffer) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Get the IP address of the connected remote party.
      // NOTE:    IP address will be empty until the session is connected.
      virtual IPAddress getConnectedRemoteIP() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Return connection information as reported by the remote party
      // NOTE:    Will return an empty string until connected.
      virtual String getRemoteConnectionInfo() = 0;
    };

    interaction IRUDPMessagingDelegate
    {
      typedef services::IRUDPMessagingPtr IRUDPMessagingPtr;
      typedef IRUDPMessaging::RUDPMessagingStates RUDPMessagingStates;

      virtual void onRUDPMessagingStateChanged(
                                               IRUDPMessagingPtr messaging,
                                               RUDPMessagingStates state
                                               ) = 0;

      virtual void onRUDPMessagingReadReady(IRUDPMessagingPtr messaging) = 0;
      virtual void onRUDPMessagingWriteReady(IRUDPMessagingPtr messaging) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::IRUDPMessagingDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IRUDPMessagingPtr, IRUDPMessagingPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IRUDPMessagingDelegate::RUDPMessagingStates, RUDPMessagingStates)
ZS_DECLARE_PROXY_METHOD_2(onRUDPMessagingStateChanged, IRUDPMessagingPtr, RUDPMessagingStates)
ZS_DECLARE_PROXY_METHOD_1(onRUDPMessagingReadReady, IRUDPMessagingPtr)
ZS_DECLARE_PROXY_METHOD_1(onRUDPMessagingWriteReady, IRUDPMessagingPtr)
ZS_DECLARE_PROXY_END()
