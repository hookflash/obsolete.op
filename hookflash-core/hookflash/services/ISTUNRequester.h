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
#include <hookflash/services/STUNPacket.h>
#include <zsLib/Proxy.h>

#include <boost/shared_array.hpp>

namespace hookflash
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISTUNRequester
    #pragma mark

    interaction ISTUNRequester
    {
      static ISTUNRequesterPtr create(
                                      IMessageQueuePtr queue,
                                      ISTUNRequesterDelegatePtr delegate,
                                      IPAddress serverIP,
                                      STUNPacketPtr stun,
                                      STUNPacket::RFCs usingRFC,
                                      Duration maxTimeout = Duration()
                                      );

      //-----------------------------------------------------------------------
      // PURPOSE: This causes a packet (which might be STUN) to be handled
      //          by the STUN requester. This is really a wrapper to
      //          ISTUNRequesterManager::handlePacket.
      static bool handlePacket(
                               IPAddress fromIPAddress,
                               const BYTE *packet,
                               ULONG packetLengthInBytes,
                               STUNPacket::RFCs allowedRFCs
                               );

      //-----------------------------------------------------------------------
      // PURPOSE: This causes a STUN packet to be handled
      //          by the STUN requester. This is really a wrapper to
      //          ISTUNRequesterManager::handleSTUNPacket.
      static bool handleSTUNPacket(
                                   IPAddress fromIPAddress,
                                   STUNPacketPtr stun
                                   );

      virtual PUID getID() const = 0;

      virtual bool isComplete() const = 0;

      virtual void cancel() = 0;

      virtual void retryRequestNow() = 0;

      virtual IPAddress getServerIP() const = 0;
      virtual STUNPacketPtr getRequest() const = 0;

      virtual Duration getMaxTimeout() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISTUNRequesterDelegate
    #pragma mark

    interaction ISTUNRequesterDelegate
    {
      typedef services::ISTUNRequesterPtr ISTUNRequesterPtr;
      typedef services::STUNPacketPtr STUNPacketPtr;

      //-----------------------------------------------------------------------
      // PURPOSE: Requests that a STUN packet be sent over the wire.
      virtual void onSTUNRequesterSendPacket(
                                             ISTUNRequesterPtr requester,
                                             IPAddress destination,
                                             boost::shared_array<BYTE> packet,
                                             ULONG packetLengthInBytes
                                             ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notifies that we believe we have a STUN packet response
      //          for the request but not absolutely positive
      // RETURNS: Return true only if the response is a valid response to the
      //          request otherwise return false.
      virtual bool handleSTUNRequesterResponse(
                                               ISTUNRequesterPtr requester,
                                               IPAddress fromIPAddress,
                                               STUNPacketPtr response
                                               ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notifies that a STUN discovery is now complete.
      virtual void onSTUNRequesterTimedOut(ISTUNRequesterPtr requester) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::ISTUNRequesterDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::ISTUNRequesterPtr, ISTUNRequesterPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::STUNPacketPtr, STUNPacketPtr)
ZS_DECLARE_PROXY_METHOD_4(onSTUNRequesterSendPacket, ISTUNRequesterPtr, IPAddress, boost::shared_array<BYTE>, ULONG)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_3(handleSTUNRequesterResponse, bool, ISTUNRequesterPtr, IPAddress, STUNPacketPtr)
ZS_DECLARE_PROXY_METHOD_1(onSTUNRequesterTimedOut, ISTUNRequesterPtr)
ZS_DECLARE_PROXY_END()
