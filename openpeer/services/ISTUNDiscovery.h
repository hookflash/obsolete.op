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

#include <zsLib/types.h>
#include <zsLib/Proxy.h>
#include <openpeer/services/IDNS.h>
#include <openpeer/services/STUNPacket.h>
#include <boost/shared_array.hpp>

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISTUNDiscovery
    #pragma mark

    interaction ISTUNDiscovery
    {
      // NOTE: IDNS::setup must have been called before calling this method
      static ISTUNDiscoveryPtr create(
                                      IMessageQueuePtr queue,                   // which message queue to use for this service (should be on the same queue as the requesting object)
                                      ISTUNDiscoveryDelegatePtr delegate,
                                      IDNS::SRVResultPtr service                // which service to use to preform the STUN lookup (only stun/udp is supported)
                                      );

      static ISTUNDiscoveryPtr create(
                                      IMessageQueuePtr queue,                   // which message queue to use for this service (should be on the same queue as the requesting object)
                                      ISTUNDiscoveryDelegatePtr delegate,
                                      const char *srvName                       // will automatically perform a stun/udp lookup on the name passed in
                                      );

      static STUNPacket::RFCs usingRFC();

      virtual PUID getID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Returns true if STUN discovery is completed (or cancelled).
      virtual bool isComplete() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Cancels the STUN discovery process.
      virtual void cancel() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Returns the mapped address as discovered from the server (or
      //          empty if the discovery failed or was cancelled).
      virtual IPAddress getMappedAddress() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Tells the ISTUNDiscovery that it has a packet that it must
      //          handle. This is just a wrapper to call
      //          ISTUNRequesterManager::handleSTUNPacket
      // RETURNS: True if the STUN packet was handled otherwise false.
      static bool handleSTUNPacket(
                                   IPAddress fromIPAddress,
                                   STUNPacketPtr stun
                                   );

      //-----------------------------------------------------------------------
      // PURPOSE: Tells the ISTUNDiscovery that it might have a packet that
      //          it is supposed to handle. This is just a wrapper to
      //          ISTUNRequesterManager::handlePacket
      // RETURNS: True if the packet was handled otherwise false.
      static bool handlePacket(
                               IPAddress fromIPAddress,
                               BYTE *packet,
                               ULONG packetLengthInBytes
                               );
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISTUNDiscoveryDelegate
    #pragma mark

    interaction ISTUNDiscoveryDelegate
    {
      typedef services::ISTUNDiscoveryPtr ISTUNDiscoveryPtr;

      //-----------------------------------------------------------------------
      // PURPOSE: Requests that a STUN packet be sent over the wire.
      virtual void onSTUNDiscoverySendPacket(
                                             ISTUNDiscoveryPtr discovery,
                                             IPAddress destination,
                                             boost::shared_array<BYTE> packet,
                                             ULONG packetLengthInBytes
                                             ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notifies that a STUN discovery is now complete.
      virtual void onSTUNDiscoveryCompleted(ISTUNDiscoveryPtr discovery) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::ISTUNDiscoveryDelegate)
ZS_DECLARE_PROXY_METHOD_4(onSTUNDiscoverySendPacket, openpeer::services::ISTUNDiscoveryPtr, IPAddress, boost::shared_array<BYTE>, ULONG)
ZS_DECLARE_PROXY_METHOD_1(onSTUNDiscoveryCompleted, openpeer::services::ISTUNDiscoveryPtr)
ZS_DECLARE_PROXY_END()
