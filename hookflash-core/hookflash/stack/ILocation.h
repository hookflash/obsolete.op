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

#include <hookflash/stack/types.h>
#include <hookflash/stack/message/types.h>

namespace hookflash
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ILocation
    #pragma mark

    interaction ILocation
    {
      enum LocationTypes
      {
        LocationType_Local,
        LocationType_Finder,
        LocationType_Peer,
      };

      enum LocationConnectionStates
      {
        LocationConnectionState_Pending,
        LocationConnectionState_Connected,
        LocationConnectionState_Disconnecting,
        LocationConnectionState_Disconnected,
      };

      static const char *toString(LocationTypes type);
      static const char *toString(LocationConnectionStates state);

      static String toDebugString(ILocationPtr location, bool includeCommaPrefix = true);

      static ILocationPtr getForLocal(IAccountPtr account);
      static ILocationPtr getForFinder(IAccountPtr account);
      static ILocationPtr getForPeer(
                                     IPeerPtr peer,
                                     const char *locationID
                                     );
      static ILocationPtr convert(IMessageSourcePtr messageSource); // if the message source is a location, it will convert

      virtual PUID getID() const = 0;

      virtual LocationTypes getLocationType() const = 0;

      virtual String getPeerURI() const = 0;                        // returns String() if not a peer location
      virtual String getLocationID() const = 0;
      virtual LocationInfoPtr getLocationInfo() const = 0;          // return LocationPtr() if location information is not available at this time

      virtual IPeerPtr getPeer() const = 0;                         // returns IPeerPtr() if there is no peer associated (e.g. location is actually a finder)

      virtual bool isConnected() const = 0;
      virtual LocationConnectionStates getConnectionState() const = 0;

      virtual bool sendMessage(message::MessagePtr message) const = 0;

      virtual void hintNowAvailable() = 0;                          // indicate that this location is likely now online
    };
  }
}
