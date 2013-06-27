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
#include <openpeer/services/STUNPacket.h>

namespace hookflash
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISTUNRequesterManager
    #pragma mark

    interaction ISTUNRequesterManager
    {
      //-----------------------------------------------------------------------
      // PURPOSE: If this is a not a multiplexed situation where multiple
      //          stun services could be running on the same port then caller
      //          can send in the packet to be parsed by the routine.
      // RETURNS: Returns the STUNRequester object that handled the STUN packet
      //          (or NULL if none were able to handle the STUN packet since
      //          the packet did not belong to any known requests).
      static ISTUNRequesterPtr handlePacket(
                                            IPAddress fromIPAddress,
                                            const BYTE *packet,
                                            ULONG packetLengthInBytes,
                                            STUNPacket::RFCs allowedRFCs
                                            );

      //-----------------------------------------------------------------------
      // PURPOSE: If the STUN packet was a response to any of the STUNRequester
      //          objects, this obejct will deliver the response to the correct
      //          requester object.
      // RETURNS: Returns which STUNRequester object handled the STUN packet
      //          (or NULL if none were able to handle the STUN packet since
      //          the packet did not belong to any known requests).
      static ISTUNRequesterPtr handleSTUNPacket(
                                                IPAddress fromIPAddress,
                                                STUNPacketPtr stun
                                                );
    };
  }
}
