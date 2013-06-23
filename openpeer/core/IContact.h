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

#include <hookflash/core/types.h>

namespace hookflash
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IContact
    #pragma mark

    interaction IContact
    {
      static String toDebugString(IContactPtr contact, bool includeCommaPrefix = true);

      static IContactPtr createFromPeerFilePublic(
                                                  IAccountPtr account,
                                                  ElementPtr peerFilePublicEl,
                                                  const char *stableIDIfKnown = NULL // (if known)
                                                  );

      static IContactPtr getForSelf(IAccountPtr account);

      virtual PUID getID() const = 0;

      virtual bool isSelf() const = 0;

      virtual String getPeerURI() const = 0;
      virtual String getFindSecret() const = 0;
      virtual String getStableUniqueID() const = 0;                                 // get stable unique ID (if known - can only be known via identity lookup)

      virtual bool hasPeerFilePublic() const = 0;
      virtual ElementPtr savePeerFilePublic() const = 0;

      virtual IAccountPtr getAssociatedAccount() const = 0;

      virtual void hintAboutLocation(const char *contactsLocationID) = 0;           // contact's location ID (as discovered via external method)
    };
  }
}
