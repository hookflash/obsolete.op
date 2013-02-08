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

#include <hookflash/stack/internal/stack_PublicationMetaData.h>
#include <hookflash/stack/IPublication.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::ULONG ULONG;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationMetaDataForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationMetaDataForPublicationRepositoryPtr IPublicationMetaDataForPublicationRepository::convert(IPublicationMetaDataPtr metaData)
      {
        return boost::dynamic_pointer_cast<IPublicationMetaDataForPublicationRepository>(metaData);
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataForPublicationRepositoryPtr IPublicationMetaDataForPublicationRepository::create(
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
                                                                                                           )
      {
        return PublicationMetaData::create(
                                           version,
                                           baseVersion,
                                           lineage,
                                           source,
                                           creatorContactID,
                                           creatorLocationID,
                                           name,
                                           mimeType,
                                           encoding,
                                           relationships,
                                           publishedToContactID,
                                           publishedToLocationID,
                                           scope,
                                           lifetime,
                                           expires
                                           );
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataForPublicationRepositoryPtr IPublicationMetaDataForPublicationRepository::createFrom(IPublicationMetaDataPtr metaData)
      {
        return PublicationMetaData::create(
                                           metaData->getVersion(),
                                           metaData->getBaseVersion(),
                                           metaData->getLineage(),
                                           metaData->getSource(),
                                           metaData->getCreatorContactID(),
                                           metaData->getCreatorLocationID(),
                                           metaData->getName(),
                                           metaData->getMimeType(),
                                           metaData->getEncoding(),
                                           metaData->getRelationships(),
                                           metaData->getPublishedToContactID(),
                                           metaData->getPublishedToLocationID(),
                                           metaData->getScope(),
                                           metaData->getLifetime(),
                                           metaData->getExpires()
                                           );
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataForPublicationRepositoryPtr IPublicationMetaDataForPublicationRepository::createFinderSource()
      {
        PublishToRelationshipsMap empty;
        return PublicationMetaData::create(
                                           0,
                                           0,
                                           0,
                                           IPublicationMetaData::Source_Peer,
                                           "",
                                           "",
                                           "",
                                           "",
                                           IPublicationMetaData::Encoding_XML,
                                           empty,
                                           "",
                                           "",
                                           IPublicationMetaData::Scope_Location,
                                           IPublicationMetaData::Lifetime_Permanent,
                                           Time()
                                           );
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataForPublicationRepositoryPtr IPublicationMetaDataForPublicationRepository::createPeerSource(
                                                                                                                     const char *peerContactID,
                                                                                                                     const char *peerLocationID
                                                                                                                     )
      {
        PublishToRelationshipsMap empty;
        return PublicationMetaData::create(
                                           0,
                                           0,
                                           0,
                                           IPublicationMetaData::Source_Peer,
                                           peerContactID,
                                           peerLocationID,
                                           "",
                                           "",
                                           IPublicationMetaData::Encoding_XML,
                                           empty,
                                           peerContactID,
                                           peerLocationID,
                                           IPublicationMetaData::Scope_Location,
                                           IPublicationMetaData::Lifetime_Permanent,
                                           Time()
                                           );
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationMetaDataForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationMetaDataForMessagesPtr IPublicationMetaDataForMessages::convert(IPublicationMetaDataPtr metaData)
      {
        return boost::dynamic_pointer_cast<IPublicationMetaDataForMessages>(metaData);
      }

      //-----------------------------------------------------------------------
      IPublicationMetaDataForMessagesPtr IPublicationMetaDataForMessages::create(
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
                                                                                 )
      {
        return PublicationMetaData::create(
                                           version,
                                           baseVersion,
                                           lineage,
                                           source,
                                           creatorContactID,
                                           creatorLocationID,
                                           name,
                                           mimeType,
                                           encoding,
                                           relationships,
                                           publishedToContactID,
                                           publishedToLocationID,
                                           scope,
                                           lifetime,
                                           expires
                                           );
      }
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationMetaData
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationMetaData::PublicationMetaData(
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
                                               ) :
        mID(zsLib::createPUID()),
        mVersion(version),
        mBaseVersion(baseVersion),
        mLineage(lineage),
        mCreatorContactID(creatorContactID ? creatorContactID : ""),
        mCreatorLocationID(creatorLocationID ? creatorLocationID : ""),
        mName(name ? name : ""),
        mMimeType(mimeType ? mimeType : ""),
        mEncoding(encoding),
        mSource(source),
        mScope(scope),
        mLifetime(lifetime),
        mExpires(expires),
        mPublishedToContactID(publishedToContactID ? publishedToContactID : ""),
        mPublishedToLocationID(publishedToLocationID ? publishedToLocationID : ""),
        mPublishedRelationships(relationships)
      {
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::init()
      {
      }

      //-----------------------------------------------------------------------
      PublicationMetaData::~PublicationMetaData()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      PublicationMetaDataPtr PublicationMetaData::create(
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
                                                         )
      {
        PublicationMetaDataPtr pThis(new PublicationMetaData(
                                                             version,
                                                             baseVersion,
                                                             lineage,
                                                             source,
                                                             creatorContactID,
                                                             creatorLocationID,
                                                             name,
                                                             mimeType,
                                                             encoding,
                                                             relationships,
                                                             publishedToContactID,
                                                             publishedToLocationID,
                                                             scope,
                                                             lifetime,
                                                             expires
                                                             )
                                     );
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationMetaData => IPublicationMetaData
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationPtr PublicationMetaData::getPublication() const
      {
        AutoRecursiveLock lock(mLock);
        return mPublication;
      }

      String PublicationMetaData::getCreatorContactID() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getCreatorContactID();
        return mCreatorContactID;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getCreatorLocationID() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getCreatorLocationID();
        return mCreatorLocationID;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getName() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getName();
        return mName;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getMimeType() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getMimeType();
        return mMimeType;
      }

      //-----------------------------------------------------------------------
      ULONG PublicationMetaData::getVersion() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getVersion();
        return mVersion;
      }

      //-----------------------------------------------------------------------
      ULONG PublicationMetaData::getBaseVersion() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getBaseVersion();
        return mBaseVersion;
      }

      //-----------------------------------------------------------------------
      ULONG PublicationMetaData::getLineage() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getLineage();
        return mLineage;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Encodings PublicationMetaData::getEncoding() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getEncoding();
        return mEncoding;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Sources PublicationMetaData::getSource() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getSource();
        return mSource;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Scopes PublicationMetaData::getScope() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getScope();
        return mScope;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Lifetimes PublicationMetaData::getLifetime() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getLifetime();
        return mLifetime;
      }

      //-----------------------------------------------------------------------
      Time PublicationMetaData::getExpires() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getExpires();
        return mExpires;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getPublishedToContactID() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getPublishedToContactID();
        return mPublishedToContactID;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getPublishedToLocationID() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getPublishedToLocationID();
        return mPublishedToLocationID;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::getRelationships(PublishToRelationshipsMap &outRelationships) const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) {
          mPublication->getRelationships(outRelationships);
          return;
        }
        outRelationships = mPublishedRelationships;
      }

      //-----------------------------------------------------------------------
      const IPublicationMetaData::PublishToRelationshipsMap &PublicationMetaData::getRelationships() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->getRelationships();
        return mPublishedRelationships;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationMetaData => IPublicationMetaDataForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      bool PublicationMetaData::isMatching(
                                   const IPublicationMetaDataForPublicationRepositoryPtr &metaData,
                                   bool ignoreLineage
                                   ) const
      {
        AutoRecursiveLock lock(mLock);
        if (metaData->getSource() != getSource()) return false;
        if (!ignoreLineage) {
          if (metaData->getLineage() != getLineage()) return false;
        }
        if (metaData->getName() != getName()) return false;
        if (metaData->getCreatorContactID() != getCreatorContactID()) return false;
        if (metaData->getCreatorLocationID() != getCreatorLocationID()) return false;
        if (IPublicationMetaData::Source_Peer == getSource()) {
          if (metaData->getPublishedToContactID() != getPublishedToContactID()) return false;
          if (metaData->getPublishedToLocationID() != getPublishedToLocationID()) return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool PublicationMetaData::isLessThan(
                                           const IPublicationMetaDataForPublicationRepositoryPtr &metaData,
                                           bool ignoreLineage
                                           ) const
      {
        AutoRecursiveLock lock(mLock);

        const char *reason = "match";

        {
          if (getSource() < metaData->getSource()) {reason = "source"; goto result_true;}
          if (getSource() > metaData->getSource()) {reason = "source"; goto result_false;}
          if (!ignoreLineage) {
            if (getLineage() < metaData->getLineage()) {reason = "lineage"; goto result_true;}
            if (getLineage() > metaData->getLineage()) {reason = "lineage"; goto result_false;}
          }
          if (getName() < metaData->getName()) {reason = "name"; goto result_true;}
          if (getName() > metaData->getName()) {reason = "name"; goto result_false;}
          if (getCreatorContactID() < metaData->getCreatorContactID()) {reason = "creator contact ID"; goto result_true;}
          if (getCreatorContactID() > metaData->getCreatorContactID()) {reason = "creator contact ID"; goto result_false;}
          if (getCreatorLocationID() < metaData->getCreatorLocationID()) {reason = "creator location ID"; goto result_true;}
          if (getCreatorLocationID() > metaData->getCreatorLocationID()) {reason = "creator location ID"; goto result_false;}
          if (IPublicationMetaData::Source_Peer == getSource()) {
            if (getPublishedToContactID() < metaData->getPublishedToContactID()) {reason = "published to contact ID"; goto result_true;}
            if (getPublishedToContactID() > metaData->getPublishedToContactID()) {reason = "published to contact ID"; goto result_false;}
            if (getPublishedToLocationID() < metaData->getPublishedToLocationID()) {reason = "published to location ID"; goto result_true;}
            if (getPublishedToLocationID() > metaData->getPublishedToLocationID()) {reason = "published to location ID"; goto result_false;}
          }
          goto result_false;
        }

      result_true:
        {
          ZS_LOG_TRACE(log("less than is TRUE") + ", reason=" + reason)
          ZS_LOG_TRACE(log("less than X (TRUE):") + getDebugValuesString())
          ZS_LOG_TRACE(log("less than Y (TRUE):") + metaData->getDebugValuesString())
          return true;
        }
      result_false:
        {
          ZS_LOG_TRACE(log("less than is FALSE") + ", reason=" + reason)
          ZS_LOG_TRACE(log("less than X (FALSE):") + getDebugValuesString())
          ZS_LOG_TRACE(log("less than Y (FALSE):") + metaData->getDebugValuesString())
        }
        return false;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setVersion(ULONG version)
      {
        AutoRecursiveLock lock(mLock);
        mVersion = version;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setBaseVersion(ULONG version)
      {
        AutoRecursiveLock lock(mLock);
        mBaseVersion = version;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setSource(IPublicationMetaData::Sources source)
      {
        AutoRecursiveLock lock(mLock);
        mSource = source;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setCreatorContact(const char *contactID, const char *locationID)
      {
        AutoRecursiveLock lock(mLock);
        mCreatorContactID = contactID;
        mCreatorLocationID = locationID;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setPublishedToContact(const char *contactID, const char *locationID)
      {
        AutoRecursiveLock lock(mLock);
        mPublishedToContactID = contactID;
        mPublishedToLocationID = locationID;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setExpires(Time expires)
      {
        AutoRecursiveLock lock(mLock);
        mExpires = expires;
      }

      //-----------------------------------------------------------------------
      static String debugNameValue(const String &name, const String &value)
      {
        if (value.isEmpty()) return String();
        return String(", ") + name + "=" + value;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getDebugValuesString() const
      {
        AutoRecursiveLock lock(mLock);

        return debugNameValue("id", Stringize<PUID>(mID).string())
             + debugNameValue("mapped to publication", (mPublication ? "true" : "false"))
             + debugNameValue("source", toString(getSource()))
             + debugNameValue("name", getName())
             + debugNameValue("version", (0 == mVersion ? String() : Stringize<ULONG>(getVersion()).string()))
             + debugNameValue("base version", (0 == mBaseVersion ? String() : Stringize<ULONG>(getBaseVersion()).string()))
             + debugNameValue("lineage", (0 == mLineage ? String() : Stringize<ULONG>(getLineage()).string()))
             + debugNameValue("creator contact ID", getCreatorContactID())
             + debugNameValue("creator location ID", getCreatorLocationID())
             + debugNameValue("published to contact ID", getPublishedToContactID())
             + debugNameValue("published to location ID", getPublishedToLocationID())
             + debugNameValue("mime type", getMimeType())
             + debugNameValue("lifetime", toString(getLifetime()))
             + debugNameValue("scope", toString(getScope()))
             + debugNameValue("expires", (Time() == mExpires ? String() : Stringize<Time>(getExpires()).string()))
             + debugNameValue("total relationships", (mPublishedRelationships.size() < 1 ? String() : Stringize<size_t>(mPublishedRelationships.size())));
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PublicationMetaData => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationMetaData::log(const char *message) const
      {
        return String("PublicationMetaData [") + Stringize<PUID>(mID).string() + "] " + message;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPublicationMetaData
    #pragma mark

    const char *IPublicationMetaData::toString(Encodings encoding)
    {
      switch (encoding) {
        case Encoding_Binary: return "Binary";
        case Encoding_XML:    return "XML";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IPublicationMetaData::toString(Sources source)
    {
      switch (source) {
        case Source_Local:    return "Local";
        case Source_Finder:   return "Finder";
        case Source_Peer:     return "Peer";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IPublicationMetaData::toString(Scopes scope)
    {
      switch (scope) {
        case Scope_Location:  return "Location";
        case Scope_Contact:   return "Contact";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IPublicationMetaData::toString(Lifetimes lifetime)
    {
      switch (lifetime) {
        case Lifetime_Session:    return "Session";
        case Lifetime_Permanent:  return "Permanent";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IPublicationMetaData::toString(Permissions permission)
    {
      switch (permission) {
        case Permission_All:      return "All";
        case Permission_None:     return "None";
        case Permission_Some:     return "Some";
        case Permission_Add:      return "Add";
        case Permission_Remove:   return "Remove";
      }
      return "UNDEFINED";
    }
  }
}
