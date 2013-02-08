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
#include <hookflash/stack/IPublicationMetaData.h>
#include <zsLib/Proxy.h>

#include <zsLib/XML.h>
#include <list>

namespace hookflash
{
  namespace stack
  {
    interaction IPublication : public IPublicationMetaData
    {
      typedef zsLib::BYTE BYTE;
      typedef zsLib::Time Time;
      typedef zsLib::String String;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      typedef boost::shared_ptr<AutoRecursiveLock> AutoRecursiveLockPtr;

      typedef String ContactID;
      typedef std::list<ContactID> RelationshipList;

      static IPublicationPtr create(
                                    Sources source,
                                    const char *creatorContactID,
                                    const char *creatorLocationID,
                                    const char *name,
                                    const char *mimeType,
                                    const BYTE *data,
                                    size_t sizeInBytes,
                                    const PublishToRelationshipsMap &publishToRelationships,
                                    const char *peerContactID = NULL,
                                    const char *peerLocationID = NULL,
                                    Scopes scope = Scope_Location,
                                    Lifetimes lifetime = Lifetime_Session,
                                    Time expires = Time()
                                    );

      static IPublicationPtr create(
                                    Sources source,
                                    const char *creatorContactID,
                                    const char *creatorLocationID,
                                    const char *name,
                                    const char *mimeType,
                                    DocumentPtr documentToBeAdopted,
                                    const PublishToRelationshipsMap &publishToRelationships,
                                    const char *peerContactID = NULL,
                                    const char *peerLocationID = NULL,
                                    Scopes scope = Scope_Location,
                                    Lifetimes lifetime = Lifetime_Session,
                                    Time expires = Time()
                                    );

      static IPublicationPtr create(
                                    Sources source,
                                    const char *creatorContactID,
                                    const char *creatorLocationID,
                                    const char *name,
                                    const char *mimeType,
                                    const RelationshipList &relationshipsDocument,
                                    const PublishToRelationshipsMap &publishToRelationships,
                                    const char *peerContactID = NULL,
                                    const char *peerLocationID = NULL,
                                    Scopes scope = Scope_Location,
                                    Lifetimes lifetime = Lifetime_Session,
                                    Time expires = Time()
                                    );

      static IPublicationPtr createForLocal(
                                            const char *creatorContactID,
                                            const char *creatorLocationID,
                                            const char *name,
                                            const char *mimeType,
                                            const BYTE *data,
                                            size_t sizeInBytes,
                                            const PublishToRelationshipsMap &publishToRelationships,
                                            Time expires = Time()
                                            );

      static IPublicationPtr createForFinder(
                                             const char *creatorContactID,
                                             const char *creatorLocationID,
                                             const char *name,
                                             const char *mimeType,
                                             const BYTE *data,
                                             size_t sizeInBytes,
                                             const PublishToRelationshipsMap &publishToRelationships,
                                             Scopes scope = Scope_Location,
                                             Lifetimes lifetime = Lifetime_Session,
                                             Time expires = Time()
                                             );

      static IPublicationPtr createForPeer(
                                           const char *creatorContactID,
                                           const char *creatorLocationID,
                                           const char *name,
                                           const char *mimeType,
                                           const BYTE *data,
                                           size_t sizeInBytes,
                                           const char *peerContactID,
                                           const char *peerLocationID,
                                           const PublishToRelationshipsMap &publishToRelationships,
                                           Time expires = Time()
                                           );

      static IPublicationPtr createForLocal(
                                            const char *creatorContactID,
                                            const char *creatorLocationID,
                                            const char *name,
                                            const char *mimeType,
                                            DocumentPtr documentToBeAdopted,
                                            const PublishToRelationshipsMap &publishToRelationships,
                                            Time expires = Time()
                                            );

      static IPublicationPtr createForFinder(
                                             const char *creatorContactID,
                                             const char *creatorLocationID,
                                             const char *name,
                                             const char *mimeType,
                                             DocumentPtr documentToBeAdopted,
                                             const PublishToRelationshipsMap &publishToRelationships,
                                             Scopes scope = Scope_Location,
                                             Lifetimes lifetime = Lifetime_Session,
                                             Time expires = Time()
                                             );

      static IPublicationPtr createForPeer(
                                           const char *creatorContactID,
                                           const char *creatorLocationID,
                                           const char *name,
                                           const char *mimeType,
                                           DocumentPtr documentToBeAdopted,
                                           const PublishToRelationshipsMap &publishToRelationships,
                                           const char *peerContactID,
                                           const char *peerLocationID,
                                           Time expires = Time()
                                           );

      static IPublicationPtr createForLocal(
                                            const char *creatorContactID,
                                            const char *creatorLocationID,
                                            const char *name,
                                            const char *mimeType,
                                            const RelationshipList &relationshipsDocument,
                                            const PublishToRelationshipsMap &publishToRelationships,
                                            Time expires = Time()
                                            );

      static IPublicationPtr createForFinder(
                                             const char *creatorContactID,
                                             const char *creatorLocationID,
                                             const char *name,
                                             const char *mimeType,
                                             const RelationshipList &relationshipsDocument,
                                             const PublishToRelationshipsMap &publishToRelationships,
                                             Scopes scope = Scope_Location,
                                             Lifetimes lifetime = Lifetime_Session,
                                             Time expires = Time()
                                             );

      static IPublicationPtr createForPeer(
                                           const char *creatorContactID,
                                           const char *creatorLocationID,
                                           const char *name,
                                           const char *mimeType,
                                           const RelationshipList &relationshipsDocument,
                                           const PublishToRelationshipsMap &publishToRelationships,
                                           const char *peerContactID,
                                           const char *peerLocationID,
                                           Time expires = Time()
                                           );

      virtual void update(
                          const BYTE *data,
                          size_t sizeInBytes
                          ) = 0;

      virtual void update(DocumentPtr updatedDocumentToBeAdopted) = 0;

      virtual void update(const RelationshipList &relationships) = 0;

      virtual void getRawData(
                              AutoRecursiveLockPtr &outDocumentLock,
                              boost::shared_array<BYTE> &outputBuffer,
                              size_t &outputBufferSizeInBytes
                              ) const = 0;

      virtual DocumentPtr getXML(AutoRecursiveLockPtr &outDocumentLock) const = 0;

      virtual void getAsContactList(RelationshipList &outList) const = 0;
    };
  }
}
