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

#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/internal/stack_PublicationMetaData.h>
#include <hookflash/stack/IPublication.h>

#include <zsLib/Exception.h>

#include <boost/shared_array.hpp>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationForPublicationRepository
      #pragma mark

      interaction IPublicationForPublicationRepository : public IPublicationMetaDataForPublicationRepository
      {
        struct Exceptions
        {
          ZS_DECLARE_CUSTOM_EXCEPTION(VersionMismatch)
        };

        typedef zsLib::PUID PUID;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef zsLib::XML::DocumentPtr DocumentPtr;
        typedef IPublication::Sources Sources;
        typedef IPublication::RelationshipList RelationshipList;
        typedef IPublication::PublishToRelationshipsMap PublishToRelationshipsMap;
        typedef IPublication::AutoRecursiveLockPtr AutoRecursiveLockPtr;

        static IPublicationForPublicationRepositoryPtr convert(IPublicationPtr publication);

        virtual PUID getID() const = 0;
        virtual IPublicationMetaDataPtr convertIPublicationMetaData() const = 0;
        virtual IPublicationPtr convertIPublication() const = 0;

        virtual String getCreatorContactID() const = 0;
        virtual String getCreatorLocationID() const = 0;

        virtual String getName() const = 0;

        virtual ULONG getVersion() const = 0;
        virtual ULONG getLineage() const = 0;
        virtual ULONG getBaseVersion() const = 0;

        virtual Sources getSource() const = 0;

        virtual Time getExpires() const = 0;

        virtual String getPublishedToContactID() const = 0;
        virtual String getPublishedToLocationID() const = 0;

        virtual void getRelationships(PublishToRelationshipsMap &outRelationships) const = 0;
        virtual const PublishToRelationshipsMap &getRelationships() const = 0;

        virtual DocumentPtr getXML(AutoRecursiveLockPtr &outDocumentLock) const = 0;

        virtual void getAsContactList(RelationshipList &outList) const = 0;

        virtual bool isMatching(
                                const IPublicationMetaDataForPublicationRepositoryPtr &metaData,
                                bool ignoreLineage = false
                                ) const = 0;

        virtual bool isLessThan(
                                const IPublicationMetaDataForPublicationRepositoryPtr &metaData,
                                bool ignoreLineage = false
                                ) const = 0;

        virtual void setVersion(ULONG version) = 0;
        virtual void setBaseVersion(ULONG version) = 0;

        virtual void setSource(IPublicationMetaData::Sources source) = 0;
        virtual void setCreatorContact(const char *contactID, const char *locationID) = 0;
        virtual void setPublishedToContact(const char *contactID, const char *locationID) = 0;

        virtual void setExpires(Time expires) = 0;

        virtual Time getCacheExpires() const = 0;
        virtual void setCacheExpires(Time expires) = 0;

        virtual void updateFromFetchedPublication(
                                                  IPublicationForPublicationRepositoryPtr fetchedPublication,
                                                  bool *noThrowVersionMismatched = NULL
                                                  ) throw (Exceptions::VersionMismatch) = 0;

        virtual void getDiffVersionsOutputSize(
                                               ULONG fromVersionNumber,
                                               ULONG toVersionNumber,
                                               ULONG &outOutputSizeInBytes,
                                               bool rawSizeOkay = true
                                               ) const = 0;

        virtual void getEntirePublicationOutputSize(
                                                    ULONG &outOutputSizeInBytes,
                                                    bool rawSizeOkay = true
                                                    ) const = 0;

        virtual String getDebugValuesString() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationForMessages
      #pragma mark

      interaction IPublicationForMessages : public IPublicationMetaDataForMessages
      {
        typedef zsLib::ULONG ULONG;
        typedef zsLib::Time Time;
        typedef zsLib::XML::NodePtr NodePtr;
        typedef zsLib::XML::ElementPtr ElementPtr;
        typedef IPublicationMetaData::Sources Sources;
        typedef IPublicationMetaData::Encodings Encodings;
        typedef IPublicationMetaData::Scopes Scopes;
        typedef IPublicationMetaData::Lifetimes Lifetimes;
        typedef IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;

        static IPublicationForMessagesPtr convert(IPublicationPtr publication);

        static IPublicationForMessagesPtr create(
                                                    ULONG version,
                                                    ULONG baseVersion,
                                                    ULONG lineage,
                                                    Sources source,
                                                    const char *creatorContactID,
                                                    const char *creatorLocationID,
                                                    const char *name,
                                                    const char *mimeType,
                                                    ElementPtr dataEl,
                                                    Encodings encoding,
                                                    const PublishToRelationshipsMap &publishToRelationships,
                                                    const char *peerContactID = NULL,
                                                    const char *peerLocationID = NULL,
                                                    Scopes scope = IPublicationMetaData::Scope_Location,
                                                    Lifetimes lifetime = IPublicationMetaData::Lifetime_Session,
                                                    Time expires = Time()
                                                    );

        virtual IPublicationMetaDataPtr convertIPublicationMetaData() const = 0;
        virtual IPublicationPtr convertIPublication() const = 0;

        virtual NodePtr getXMLDiffs(
                                    ULONG &ioFromVersion,
                                    ULONG toVersion
                                    ) const = 0;

        virtual ULONG getVersion() const = 0;
        virtual ULONG getBaseVersion() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication
      #pragma mark

      class Publication : public IPublication,
                          public IPublicationForPublicationRepository,
                          public IPublicationForMessages
      {
      public:
        typedef zsLib::BYTE BYTE;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::Time Time;
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
        typedef zsLib::XML::NodePtr NodePtr;
        typedef zsLib::XML::ElementPtr ElementPtr;
        typedef zsLib::XML::DocumentPtr DocumentPtr;
        typedef IPublication::AutoRecursiveLockPtr AutoRecursiveLockPtr;
        typedef IPublication::Sources Sources;
        typedef IPublication::Encodings Encodings;
        typedef IPublication::Scopes Scopes;
        typedef IPublication::Lifetimes Lifetimes;
        typedef IPublication::RelationshipList RelationshipList;
        typedef IPublication::PublishToRelationshipsMap PublishToRelationshipsMap;

        friend interaction IPublication;
        friend interaction IPublicationForPublicationRepository;
        friend interaction IPublicationForMessages;

      protected:
        Publication(
                    Sources source,
                    const char *creatorContactID,
                    const char *creatorLocationID,
                    const char *name,
                    const char *mimeType,
                    const PublishToRelationshipsMap &publishToRelationships,
                    const char *peerContactID,
                    const char *peerLocationID,
                    Scopes scope,
                    Lifetimes lifetime,
                    Time expires
                    );

        void init();

      public:
        ~Publication();

        static PublicationPtr convert(IPublicationPtr publication);
        static PublicationPtr convert(IPublicationForPublicationRepositoryPtr publication);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Publication => IPublicationMetaData
        #pragma mark

        virtual IPublicationPtr getPublication() const;

        virtual String getCreatorContactID() const;
        virtual String getCreatorLocationID() const;

        virtual String getName() const;
        virtual String getMimeType() const;

        virtual ULONG getVersion() const;
        virtual ULONG getBaseVersion() const;
        virtual ULONG getLineage() const;

        virtual Encodings getEncoding() const;

        virtual Sources getSource() const;
        virtual Scopes getScope() const;
        virtual Lifetimes getLifetime() const;

        virtual Time getExpires() const;

        virtual String getPublishedToContactID() const;
        virtual String getPublishedToLocationID() const;

        virtual void getRelationships(PublishToRelationshipsMap &outRelationships) const;
        virtual const PublishToRelationshipsMap &getRelationships() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Publication => IPublication
        #pragma mark

        static PublicationPtr create(
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

        static PublicationPtr create(
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

        static PublicationPtr create(
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

        virtual void update(
                            const BYTE *data,
                            size_t sizeInBytes
                            );

        virtual void update(DocumentPtr updatedDocumentToBeAdopted);

        virtual void update(const RelationshipList &relationships);

        virtual void getRawData(
                                AutoRecursiveLockPtr &outDocumentLock,
                                boost::shared_array<BYTE> &outputBuffer,
                                size_t &outputBufferSizeInBytes
                                ) const;

        virtual DocumentPtr getXML(AutoRecursiveLockPtr &outDocumentLock) const;

        virtual void getAsContactList(RelationshipList &outList) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Publication => IPublicationForPublicationRepository
        #pragma mark

        virtual PUID getID() const {return mID;}
        virtual IPublicationMetaDataPtr convertIPublicationMetaData() const;
        virtual IPublicationPtr convertIPublication() const;

        // (duplicate) virtual String getCreatorContactID() const;
        // (duplciate) virtual String getCreatorLocationID() const;

        // (duplicate) virtual String getName() const;

        // (duplicate) virtual ULONG getVersion() const;

        // (duplicate) virtual Sources getSource() const;
        // (duplicate) virtual ULONG getLineage() const;
        // (duplicate) virtual ULONG getBaseVersion() const;

        // (duplicate) virtual Time getExpires() const;

        // (duplicate) virtual String getPublishedToContactID() const;
        // (duplicate) virtual String getPublishedToLocationID() const;

        // (duplicate) virtual DocumentPtr getXML(AutoRecursiveLockPtr &outDocumentLock) const;

        // (duplicate) virtual void getAsContactList(RelationshipList &outList) const;

        // (duplicate) virtual void getRelationships(PublishToRelationshipsMap &outRelationships) const;
        // (duplicate) virtual const PublishToRelationshipsMap &getRelationships() const;

        virtual bool isMatching(
                                const IPublicationMetaDataForPublicationRepositoryPtr &metaData,
                                bool ignoreLineage = false
                                ) const;

        virtual bool isLessThan(
                                const IPublicationMetaDataForPublicationRepositoryPtr &metaData,
                                bool ignoreLineage = false
                                ) const;

        virtual void setVersion(ULONG version);
        virtual void setBaseVersion(ULONG version);

        virtual void setSource(IPublicationMetaData::Sources source);
        virtual void setCreatorContact(const char *contactID, const char *locationID);
        virtual void setPublishedToContact(const char *contactID, const char *locationID);

        virtual void setExpires(Time expires);

        virtual Time getCacheExpires() const;
        virtual void setCacheExpires(Time expires);

        virtual void updateFromFetchedPublication(
                                                  IPublicationForPublicationRepositoryPtr fetchedPublication,
                                                  bool *noThrowVersionMismatched = NULL
                                                  ) throw (Exceptions::VersionMismatch);

        virtual void getDiffVersionsOutputSize(
                                               ULONG fromVersionNumber,
                                               ULONG toVersionNumber,
                                               ULONG &outOutputSizeInBytes,
                                               bool rawSizeOkay = true
                                               ) const;

        virtual void getEntirePublicationOutputSize(
                                                    ULONG &outOutputSizeInBytes,
                                                    bool rawSizeOkay = true
                                                    ) const;

        virtual String getDebugValuesString() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Publication => IPublicationForMessages
        #pragma mark

        static PublicationPtr create(
                                     ULONG version,
                                     ULONG baseVersion,
                                     ULONG lineage,
                                     Sources source,
                                     const char *creatorContactID,
                                     const char *creatorLocationID,
                                     const char *name,
                                     const char *mimeType,
                                     ElementPtr dataEl,
                                     Encodings encoding,
                                     const PublishToRelationshipsMap &publishToRelationships,
                                     const char *peerContactID,
                                     const char *peerLocationID,
                                     Scopes scope,
                                     Lifetimes lifetime,
                                     Time expires
                                     );

        // (duplicate) virtual IPublicationMetaDataPtr convertIPublicationMetaData() const;
        // (duplicate) virtual IPublicationPtr convertIPublication() const;

        virtual NodePtr getXMLDiffs(
                                    ULONG &ioFromVersion,
                                    ULONG toVersion
                                    ) const;

        // (duplicate) virtual ULONG getVersion() const;
        // (duplicate) virtual ULONG getBaseVersion() const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Publication => (internal)
        #pragma mark

        String log(const char *message) const;
        void logDocument() const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Publication => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        PublicationWeakPtr mThisWeak;

        String mContactID;
        String mLocationID;

        String mName;
        String mMimeType;

        boost::shared_array<BYTE> mData;
        size_t mDataLengthInBytes;

        DocumentPtr mDocument;

        ULONG mVersion;
        ULONG mBaseVersion;
        ULONG mLineage;

        Scopes mScope;
        Sources mSource;
        Lifetimes mLifetime;

        Time mExpires;
        Time mCacheExpires;

        String mPublishedToContactID;
        String mPublishedToLocationID;

        PublishToRelationshipsMap mPublishedRelationships;

        typedef ULONG VersionNumber;
        typedef DocumentPtr XMLDiffDocument;
        typedef std::map<VersionNumber, XMLDiffDocument> XMLDiffDocumentMap;

        XMLDiffDocumentMap mXMLDiffDocuments;
      };
    }
  }
}
