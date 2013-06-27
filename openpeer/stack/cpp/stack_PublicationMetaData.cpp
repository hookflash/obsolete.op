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

#include <openpeer/stack/internal/stack_PublicationMetaData.h>
#include <openpeer/stack/internal/stack_Publication.h>
#include <openpeer/stack/IPublication.h>
#include <openpeer/stack/ILocation.h>
#include <openpeer/stack/IPeer.h>
#include <openpeer/stack/internal/stack_Location.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static String debugNameValue(
                                   bool &ioFirst,
                                   const String &name,
                                   const String &value,
                                   bool addEquals = true
                                   )
      {
        if (value.isEmpty()) return String();
        if (ioFirst) {
          ioFirst = false;
          return name + "=" + value;
        }
        return String(", ") + name + (addEquals ? "=" : "") + value;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationMetaDataForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationMetaDataPtr IPublicationMetaDataForPublicationRepository::create(
                                                                                  ULONG version,
                                                                                  ULONG baseVersion,
                                                                                  ULONG lineage,
                                                                                  LocationPtr creatorLocation,
                                                                                  const char *name,
                                                                                  const char *mimeType,
                                                                                  Encodings encoding,
                                                                                  const PublishToRelationshipsMap &relationships,
                                                                                  LocationPtr publishedLocation,
                                                                                  Time expires
                                                                                  )
      {
        return IPublicationMetaDataFactory::singleton().creatPublicationMetaData(
                                                                                 version,
                                                                                 baseVersion,
                                                                                 lineage,
                                                                                 creatorLocation,
                                                                                 name,
                                                                                 mimeType,
                                                                                 encoding,
                                                                                 relationships,
                                                                                 publishedLocation,
                                                                                 expires
                                                                                 );
      }

      //-----------------------------------------------------------------------
      PublicationMetaDataPtr IPublicationMetaDataForPublicationRepository::createFrom(IPublicationMetaDataPtr metaData)
      {
        return IPublicationMetaDataFactory::singleton().creatPublicationMetaData(
                                                                                 metaData->getVersion(),
                                                                                 metaData->getBaseVersion(),
                                                                                 metaData->getLineage(),
                                                                                 Location::convert(metaData->getCreatorLocation()),
                                                                                 metaData->getName(),
                                                                                 metaData->getMimeType(),
                                                                                 metaData->getEncoding(),
                                                                                 metaData->getRelationships(),
                                                                                 Location::convert(metaData->getPublishedLocation()),
                                                                                 metaData->getExpires()
                                                                                 );
      }

      //-----------------------------------------------------------------------
      PublicationMetaDataPtr IPublicationMetaDataForPublicationRepository::createForSource(LocationPtr location)
      {
        PublishToRelationshipsMap empty;
        return IPublicationMetaDataFactory::singleton().creatPublicationMetaData(
                                                                                 0,
                                                                                 0,
                                                                                 0,
                                                                                 location,
                                                                                 "",
                                                                                 "",
                                                                                 IPublicationMetaData::Encoding_JSON,
                                                                                 empty,
                                                                                 location,
                                                                                 Time()
                                                                                 );
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationMetaDataForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationMetaDataPtr IPublicationMetaDataForMessages::create(
                                                                     ULONG version,
                                                                     ULONG baseVersion,
                                                                     ULONG lineage,
                                                                     LocationPtr creatorLocation,
                                                                     const char *name,
                                                                     const char *mimeType,
                                                                     Encodings encoding,
                                                                     const PublishToRelationshipsMap &relationships,
                                                                     LocationPtr publishedLocation,
                                                                     Time expires
                                                                     )
      {
        return IPublicationMetaDataFactory::singleton().creatPublicationMetaData(
                                                                                 version,
                                                                                 baseVersion,
                                                                                 lineage,
                                                                                 creatorLocation,
                                                                                 name,
                                                                                 mimeType,
                                                                                 encoding,
                                                                                 relationships,
                                                                                 publishedLocation,
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
                                               LocationPtr creatorLocation,
                                               const char *name,
                                               const char *mimeType,
                                               Encodings encoding,
                                               const PublishToRelationshipsMap &relationships,
                                               LocationPtr publishedLocation,
                                               Time expires
                                               ) :
        mID(zsLib::createPUID()),
        mVersion(version),
        mBaseVersion(baseVersion),
        mLineage(lineage),
        mCreatorLocation(creatorLocation),
        mName(name ? name : ""),
        mMimeType(mimeType ? mimeType : ""),
        mEncoding(encoding),
        mPublishedRelationships(relationships),
        mPublishedLocation(publishedLocation),
        mExpires(expires)
      {
        ZS_LOG_DEBUG(log("created") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::init()
      {
      }

      //-----------------------------------------------------------------------
      PublicationMetaData::~PublicationMetaData()
      {
        if (isNoop()) return;
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      PublicationMetaDataPtr PublicationMetaData::convert(IPublicationMetaDataPtr publication)
      {
        return boost::dynamic_pointer_cast<PublicationMetaData>(publication);
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
      IPublicationPtr PublicationMetaData::toPublication() const
      {
        AutoRecursiveLock lock(mLock);
        return mPublication;
      }

      //-----------------------------------------------------------------------
      ILocationPtr PublicationMetaData::getCreatorLocation() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getCreatorLocation();
        return mCreatorLocation;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getName() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getName();
        return mName;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getMimeType() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getMimeType();
        return mMimeType;
      }

      //-----------------------------------------------------------------------
      ULONG PublicationMetaData::getVersion() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getVersion();
        return mVersion;
      }

      //-----------------------------------------------------------------------
      ULONG PublicationMetaData::getBaseVersion() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getBaseVersion();
        return mBaseVersion;
      }

      //-----------------------------------------------------------------------
      ULONG PublicationMetaData::getLineage() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getLineage();
        return mLineage;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Encodings PublicationMetaData::getEncoding() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getEncoding();
        return mEncoding;
      }

      //-----------------------------------------------------------------------
      Time PublicationMetaData::getExpires() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getExpires();
        return mExpires;
      }

      //-----------------------------------------------------------------------
      ILocationPtr PublicationMetaData::getPublishedLocation() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getPublishedLocation();
        return mPublishedLocation;
      }

      //-----------------------------------------------------------------------
      const IPublicationMetaData::PublishToRelationshipsMap &PublicationMetaData::getRelationships() const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getRelationships();
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
      PublicationMetaDataPtr PublicationMetaData::create(
                                                         ULONG version,
                                                         ULONG baseVersion,
                                                         ULONG lineage,
                                                         LocationPtr creatorLocation,
                                                         const char *name,
                                                         const char *mimeType,
                                                         Encodings encoding,
                                                         const PublishToRelationshipsMap &relationships,
                                                         LocationPtr publishedLocation,
                                                         Time expires
                                                         )
      {
        PublicationMetaDataPtr pThis(new PublicationMetaData(
                                                             version,
                                                             baseVersion,
                                                             lineage,
                                                             creatorLocation,
                                                             name,
                                                             mimeType,
                                                             encoding,
                                                             relationships,
                                                             publishedLocation,
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
      #pragma mark
      #pragma mark PublicationMetaData => IPublicationMetaDataForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      String PublicationMetaData::toDebugString(IPublicationMetaDataPtr metaData, bool includeCommaPrefix)
      {
        if (!metaData) return includeCommaPrefix ? String(", publication meta data=(null)") : String("publication meta data=(null)");
        return PublicationMetaData::convert(metaData)->getDebugValuesString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      PublicationMetaDataPtr PublicationMetaData::toPublicationMetaData() const
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      LocationPtr PublicationMetaData::getCreatorLocation(bool) const
      {
        AutoRecursiveLock lock(mLock);
        return mCreatorLocation;
      }

      //-----------------------------------------------------------------------
      LocationPtr PublicationMetaData::getPublishedLocation(bool) const
      {
        AutoRecursiveLock lock(mLock);
        return mPublishedLocation;
      }

      //-----------------------------------------------------------------------
      bool PublicationMetaData::isMatching(
                                           const IPublicationMetaDataPtr &metaData,
                                           bool ignoreLineage
                                           ) const
      {
        AutoRecursiveLock lock(mLock);

        const char *reason = NULL;
        if (0 != ILocationForPublication::locationCompare(mCreatorLocation, metaData->getCreatorLocation(), reason)) {
          return false;
        }

        if (!ignoreLineage) {
          if (metaData->getLineage() != getLineage()) return false;
        }
        if (metaData->getName() != getName()) return false;
        if (0 != ILocationForPublication::locationCompare(mPublishedLocation, metaData->getPublishedLocation(), reason)) {
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool PublicationMetaData::isLessThan(
                                           const IPublicationMetaDataPtr &metaData,
                                           bool ignoreLineage
                                           ) const
      {
        AutoRecursiveLock lock(mLock);

        const char *reason = "match";

        {
          int compare = ILocationForPublication::locationCompare(mCreatorLocation, metaData->getCreatorLocation(), reason);

          if (compare < 0) {goto result_true;}
          if (compare > 0) {goto result_false;}

          if (!ignoreLineage) {
            if (getLineage() < metaData->getLineage()) {reason = "lineage"; goto result_true;}
            if (getLineage() > metaData->getLineage()) {reason = "lineage"; goto result_false;}
          }
          if (getName() < metaData->getName()) {reason = "name"; goto result_true;}
          if (getName() > metaData->getName()) {reason = "name"; goto result_false;}

          compare = ILocationForPublication::locationCompare(mPublishedLocation, metaData->getPublishedLocation(), reason);
          if (compare < 0) {goto result_true;}
          if (compare > 0) {goto result_false;}
          goto result_false;
        }

      result_true:
        {
          IPublicationPtr publication = metaData->toPublication();
          ZS_LOG_TRACE(log("less than is TRUE") + ", reason=" + reason)
          ZS_LOG_TRACE(log("less than X (TRUE):") + getDebugValuesString())
          ZS_LOG_TRACE(log("less than Y (TRUE):") + (publication ? Publication::convert(publication)->forPublicationMetaData().getDebugValuesString() : PublicationMetaData::convert(metaData)->getDebugValuesString()))
          return true;
        }
      result_false:
        {
          IPublicationPtr publication = metaData->toPublication();
          ZS_LOG_TRACE(log("less than is FALSE") + ", reason=" + reason)
          ZS_LOG_TRACE(log("less than X (FALSE):") + getDebugValuesString())
          ZS_LOG_TRACE(log("less than Y (FALSE):") + (publication ? Publication::convert(publication)->forPublicationMetaData().getDebugValuesString() : PublicationMetaData::convert(metaData)->getDebugValuesString()))
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
      void PublicationMetaData::setCreatorLocation(LocationPtr location)
      {
        AutoRecursiveLock lock(mLock);
        mCreatorLocation = location;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setPublishedLocation(LocationPtr location)
      {
        AutoRecursiveLock lock(mLock);
        mPublishedLocation = location;
      }

      //-----------------------------------------------------------------------
      void PublicationMetaData::setExpires(Time expires)
      {
        AutoRecursiveLock lock(mLock);
        mExpires = expires;
      }

      //-----------------------------------------------------------------------
      String PublicationMetaData::getDebugValuesString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(mLock);
        if (mPublication) return mPublication->forPublicationMetaData().getDebugValuesString();

        LocationPtr creatorLocation = Location::convert(getCreatorLocation());
        LocationPtr publishedLocation = Location::convert(getPublishedLocation());

        bool first = !includeCommaPrefix;

        return debugNameValue(first, "id", Stringize<PUID>(mID).string())
             + debugNameValue(first, "mapped to publication", (mPublication ? "true" : "false"))
             + debugNameValue(first, "name", getName())
             + debugNameValue(first, "version", (0 == mVersion ? String() : Stringize<ULONG>(getVersion()).string()))
             + debugNameValue(first, "base version", (0 == mBaseVersion ? String() : Stringize<ULONG>(getBaseVersion()).string()))
             + debugNameValue(first, "lineage", (0 == mLineage ? String() : Stringize<ULONG>(getLineage()).string()))
             + debugNameValue(first, "creator: ", creatorLocation ? creatorLocation->forPublication().getDebugValueString(false) : String(), false)
             + debugNameValue(first, "published: ", publishedLocation ? publishedLocation->forPublication().getDebugValueString(false) : String(), false)
             + debugNameValue(first, "mime type", getMimeType())
             + debugNameValue(first, "expires", (Time() == mExpires ? String() : Stringize<Time>(getExpires()).string()))
             + debugNameValue(first, "total relationships", (mPublishedRelationships.size() < 1 ? String() : Stringize<size_t>(mPublishedRelationships.size())));
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
        return String("PublicationMetaData [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPublicationMetaData
    #pragma mark

    //-------------------------------------------------------------------------
    String IPublicationMetaData::toDebugString(IPublicationMetaDataPtr metaData, bool includeCommaPrefix)
    {
      return internal::PublicationMetaData::toDebugString(metaData, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    const char *IPublicationMetaData::toString(Encodings encoding)
    {
      switch (encoding) {
        case Encoding_Binary: return "Binary";
        case Encoding_JSON:   return "json";
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
