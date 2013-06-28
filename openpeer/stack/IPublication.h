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
#include <openpeer/stack/IPublicationMetaData.h>

#include <list>

namespace openpeer
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPublication
    #pragma mark

    interaction IPublication : public IPublicationMetaData
    {
      typedef String PeerURI;
      typedef std::list<PeerURI> RelationshipList;
      typedef boost::shared_ptr<RelationshipList> RelationshipListPtr;
      typedef boost::weak_ptr<RelationshipList> RelationshipListWeakPtr;

      static String toDebugString(IPublicationPtr publication, bool includeCommaPrefix = true);

      static IPublicationPtr create(
                                    ILocationPtr creatorLocation,
                                    const char *name,
                                    const char *mimeType,
                                    const SecureByteBlock &buffer,
                                    const PublishToRelationshipsMap &publishToRelationships,
                                    ILocationPtr documentLocation = ILocationPtr(),
                                    Time expires = Time()
                                    );

      static IPublicationPtr create(
                                    ILocationPtr creatorLocation,
                                    const char *name,
                                    const char *mimeType,
                                    DocumentPtr documentToBeAdopted,
                                    const PublishToRelationshipsMap &publishToRelationships,
                                    ILocationPtr documentLocation = ILocationPtr(),
                                    Time expires = Time()
                                    );

      static IPublicationPtr create(
                                    ILocationPtr creatorLocation,
                                    const char *name,
                                    const char *mimeType,
                                    const RelationshipList &relationshipsDocument,
                                    const PublishToRelationshipsMap &publishToRelationships,
                                    ILocationPtr documentLocation = ILocationPtr(),
                                    Time expires = Time()
                                    );

      virtual void update(const SecureByteBlock &buffer) = 0;

      virtual void update(DocumentPtr updatedDocumentToBeAdopted) = 0;

      virtual void update(const RelationshipList &relationships) = 0;

      virtual SecureByteBlockPtr getRawData(AutoRecursiveLockPtr &outDocumentLock) const = 0;

      virtual DocumentPtr getJSON(AutoRecursiveLockPtr &outDocumentLock) const = 0;

      virtual RelationshipListPtr getAsContactList() const = 0;
    };
  }
}
