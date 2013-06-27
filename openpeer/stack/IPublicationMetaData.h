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

#include <list>

namespace hookflash
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPublicationMetaData
    #pragma mark

    interaction IPublicationMetaData
    {
      enum Encodings
      {
        Encoding_Binary,
        Encoding_JSON,
      };

      enum Permissions
      {
        Permission_All,
        Permission_None,
        Permission_Some,
        Permission_Add,
        Permission_Remove,
      };

      typedef String PeerURI;

      typedef std::list<PeerURI> PeerURIList;
      typedef std::pair<Permissions, PeerURIList> PermissionAndPeerURIListPair;

      // contact list publication to relationships map
      typedef String DocumentName;
      typedef std::map<DocumentName, PermissionAndPeerURIListPair> PublishToRelationshipsMap;
      typedef PublishToRelationshipsMap SubscribeToRelationshipsMap;

      static const char *toString(Encodings encoding);
      static const char *toString(Permissions permission);

      static String toDebugString(IPublicationMetaDataPtr metaData, bool includeCommaPrefix = true);

      virtual PUID getID() const = 0;

      virtual IPublicationPtr toPublication() const = 0;

      virtual ILocationPtr getCreatorLocation() const = 0;

      virtual String getName() const = 0;
      virtual String getMimeType() const = 0;

      virtual ULONG getVersion() const = 0;
      virtual ULONG getBaseVersion() const = 0;
      virtual ULONG getLineage() const = 0;

      virtual Encodings getEncoding() const = 0;

      virtual Time getExpires() const = 0;

      virtual ILocationPtr getPublishedLocation() const = 0; // where was the docuemnt published

      virtual const PublishToRelationshipsMap &getRelationships() const = 0;
    };
  }
}
