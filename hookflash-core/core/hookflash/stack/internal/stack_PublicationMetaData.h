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

#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/IPublicationMetaData.h>

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
      #pragma mark IPublicationMetaDataForPublicationRepository
      #pragma mark

      interaction IPublicationMetaDataForPublicationRepository
      {
        typedef zsLib::ULONG ULONG;
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef IPublicationMetaData::Sources Sources;
        typedef IPublicationMetaData::Encodings Encodings;
        typedef IPublicationMetaData::Scopes Scopes;
        typedef IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;
        typedef IPublicationMetaData::Lifetimes Lifetimes;

        static IPublicationMetaDataForPublicationRepositoryPtr convert(IPublicationMetaDataPtr metaData);

        virtual IPublicationMetaDataPtr convertIPublicationMetaData() const = 0;

        static IPublicationMetaDataForPublicationRepositoryPtr create(
                                                                      ULONG version,
                                                                      ULONG baseVersion,
                                                                      ULONG lineage,
                                                                      Sources source,
                                                                      const char *creatorContactID,
                                                                      const char *creatorLocationID,
                                                                      const char *name,
                                                                      const char *mimeType,
                                                                      Encodings encoding,
                                                                      const PublishToRelationshipsMap &relationships,
                                                                      const char *publishedToContactID = NULL,
                                                                      const char *publishedToLocationID = NULL,
                                                                      Scopes scope = IPublicationMetaData::Scope_Location,
                                                                      Lifetimes lifetime = IPublicationMetaData::Lifetime_Session,
                                                                      Time expires = Time()
                                                                      );

        static IPublicationMetaDataForPublicationRepositoryPtr createFrom(IPublicationMetaDataPtr metaData);

        static IPublicationMetaDataForPublicationRepositoryPtr createFinderSource();

        static IPublicationMetaDataForPublicationRepositoryPtr createPeerSource(
                                                                                const char *peerContactID,
                                                                                const char *peerLocationID
                                                                                );

        virtual String getCreatorContactID() const = 0;
        virtual String getCreatorLocationID() const = 0;

        virtual String getName() const = 0;

        virtual ULONG getVersion() const = 0;
        virtual ULONG getBaseVersion() const = 0;
        virtual ULONG getLineage() const = 0;

        virtual Sources getSource() const = 0;

        virtual Time getExpires() const = 0;

        virtual String getPublishedToContactID() const = 0;
        virtual String getPublishedToLocationID() const = 0;

        virtual void getRelationships(PublishToRelationshipsMap &outRelationships) const = 0;
        virtual const PublishToRelationshipsMap &getRelationships() const = 0;

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

        virtual String getDebugValuesString() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationMetaDataForMessages
      #pragma mark

      interaction IPublicationMetaDataForMessages
      {
        typedef zsLib::ULONG ULONG;
        typedef zsLib::Time Time;
        typedef IPublicationMetaData::Sources Sources;
        typedef IPublicationMetaData::Encodings Encodings;
        typedef IPublicationMetaData::Scopes Scopes;
        typedef IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;
        typedef IPublicationMetaData::Lifetimes Lifetimes;

        static IPublicationMetaDataForMessagesPtr convert(IPublicationMetaDataPtr metaData);

        static IPublicationMetaDataForMessagesPtr create(
                                                         ULONG version,
                                                         ULONG baseVersion,
                                                         ULONG lineage,
                                                         Sources source,
                                                         const char *creatorContactID,
                                                         const char *creatorLocationID,
                                                         const char *name,
                                                         const char *mimeType,
                                                         Encodings encoding,
                                                         const PublishToRelationshipsMap &relationships,
                                                         const char *publishedToContactID = NULL,
                                                         const char *publishedToLocationID = NULL,
                                                         Scopes scope = IPublicationMetaData::Scope_Location,
                                                         Lifetimes lifetime = IPublicationMetaData::Lifetime_Session,
                                                         Time expires = Time()
                                                         );

        virtual IPublicationMetaDataPtr convertIPublicationMetaData() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationMetaData
      #pragma mark

      class PublicationMetaData : public IPublicationMetaData,
                                  public IPublicationMetaDataForPublicationRepository,
                                  public IPublicationMetaDataForMessages
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef IPublicationMetaData::Sources Sources;
        typedef IPublicationMetaData::Encodings Encodings;
        typedef IPublicationMetaData::Scopes Scopes;
        typedef IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;
        typedef IPublicationMetaData::Lifetimes Lifetimes;

        friend interaction IPublicationMetaDataForPublicationRepository;
        friend interaction IPublicationMetaDataForMessages;

      protected:
        PublicationMetaData(
                            ULONG version,
                            ULONG baseVersion,
                            ULONG lineage,
                            Sources source,
                            const char *creatorContactID,
                            const char *creatorLocationID,
                            const char *name,
                            const char *mimeType,
                            Encodings encoding,
                            const PublishToRelationshipsMap &relationships,
                            const char *publishedToContactID,
                            const char *publishedToLocationID,
                            Scopes scope,
                            Lifetimes lifetime,
                            Time expires
                            );

        void init();

      public:
        ~PublicationMetaData();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationMetaData => IPublicationMetaData
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
        #pragma mark PublicationMetaData => IPublicationMetaDataForPublicationRepository
        #pragma mark

        virtual IPublicationMetaDataPtr convertIPublicationMetaData() const {return mThisWeak.lock();}

        static PublicationMetaDataPtr create(
                                             ULONG version,
                                             ULONG baseVersion,
                                             ULONG lineage,
                                             Sources source,
                                             const char *creatorContactID,
                                             const char *creatorLocationID,
                                             const char *name,
                                             const char *mimeType,
                                             Encodings encoding,
                                             const PublishToRelationshipsMap &relationships,
                                             const char *publishedToContactID,
                                             const char *publishedToLocationID,
                                             Scopes scope,
                                             Lifetimes lifetime,
                                             Time expires
                                             );

        // (duplicate) virtual String getCreatorContactID() const;
        // (duplicate) virtual String getCreatorLocationID() const;

        // (duplicate) virtual String getName() const;

        // (duplicate) virtual ULONG getVersion() const;
        // (duplicate) virtual ULONG getBaseVersion() const;
        // (duplicate) virtual ULONG getLineage() const;

        // (duplicate) virtual Sources getSource() const;

        // (duplicate) virtual Time getExpires() const;

        // (duplicate) virtual String getPublishedToContactID() const;
        // (duplicate) virtual String getPublishedToLocationID() const;

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

        virtual String getDebugValuesString() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationMetaData => IPublicationMetaDataForPublicationRepository
        #pragma mark

        // (duplicate) virtual IPublicationMetaDataPtr convertIPublicationMetaData() const {return mThisWeak.lock();}

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationMetaData => (internal)
        #pragma mark

        String log(const char *message) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PublicationMetaData => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        PublicationMetaDataWeakPtr mThisWeak;

        IPublicationPtr mPublication;

        String mCreatorContactID;
        String mCreatorLocationID;

        String mName;
        String mMimeType;

        ULONG mVersion;
        ULONG mBaseVersion;
        ULONG mLineage;

        Encodings mEncoding;

        Sources mSource;
        Scopes mScope;
        Lifetimes mLifetime;

        Time mExpires;

        String mPublishedToContactID;
        String mPublishedToLocationID;

        PublishToRelationshipsMap mPublishedRelationships;
      };
    }
  }
}
