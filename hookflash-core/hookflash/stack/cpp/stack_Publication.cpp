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

#include <hookflash/stack/internal/stack_Publication.h>
#include <hookflash/stack/internal/stack_PublicationMetaData.h>
#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/IPeer.h>
#include <hookflash/stack/internal/stack_Diff.h>
#include <hookflash/stack/internal/stack_Location.h>

#include <zsLib/XML.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    using zsLib::Stringize;
    using message::IMessageHelper;

    typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static DocumentPtr createDocumentFromRelationships(const IPublication::RelationshipList &relationships)
      {
        typedef IPublication::RelationshipList RelationshipList;

        DocumentPtr doc = Document::create();
        ElementPtr contactsEl = Element::create();
        contactsEl->setValue("contacts");

        for (RelationshipList::const_iterator iter = relationships.begin(); iter != relationships.end(); ++iter)
        {
          const String contact = (*iter);

          ElementPtr contactEl = IMessageHelper::createElementWithTextAndJSONEncode("contact", contact);
          contactsEl->adoptAsLastChild(contactEl);
        }

        doc->adoptAsLastChild(contactsEl);

        return doc;
      }

      //-----------------------------------------------------------------------
      static String toString(NodePtr node)
      {
        if (!node) return "(null)";

        GeneratorPtr generator = Generator::createJSONGenerator();

        boost::shared_array<char> output;
        ULONG length = 0;
        output = generator->write(node, &length);

        return (CSTR)output.get();
      }

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
      class Publication_UniqueLineage;
      typedef boost::shared_ptr<Publication_UniqueLineage> Publication_UniqueLineagePtr;
      typedef boost::weak_ptr<Publication_UniqueLineage> Publication_UniqueLineageWeakPtr;

      class Publication_UniqueLineage
      {
      protected:
        Publication_UniqueLineage() :
          mUnique(0)
        {
        }

        static Publication_UniqueLineagePtr create()
        {
          Publication_UniqueLineagePtr pThis(new Publication_UniqueLineage);
          return pThis;
        }

      public:
        ~Publication_UniqueLineage() {}

        static Publication_UniqueLineagePtr singleton()
        {
          static Publication_UniqueLineagePtr singleton = create();
          return singleton;
        }

        ULONG getUniqueLineage()
        {
          AutoRecursiveLock lock(mLock);
          ULONG proposed = (ULONG)(time(NULL));
          if (proposed <= mUnique) {
            proposed = mUnique + 1;
          }
          mUnique = proposed;
          return mUnique;
        }

      private:
        mutable RecursiveLock mLock;
        ULONG mUnique;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationPtr IPublicationForMessages::create(
                                                     ULONG version,
                                                     ULONG baseVersion,
                                                     ULONG lineage,
                                                     LocationPtr creatorLocation,
                                                     const char *name,
                                                     const char *mimeType,
                                                     ElementPtr dataEl,
                                                     Encodings encoding,
                                                     const PublishToRelationshipsMap &publishToRelationships,
                                                     LocationPtr publishedLocation,
                                                     Time expires
                                                     )
      {
        return IPublicationFactory::singleton().create(
                                                       version,
                                                       baseVersion,
                                                       lineage,
                                                       creatorLocation,
                                                       name,
                                                       mimeType,
                                                       dataEl,
                                                       encoding,
                                                       publishToRelationships,
                                                       publishedLocation,
                                                       expires
                                                       );
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication
      #pragma mark

      //-----------------------------------------------------------------------
      Publication::Publication(
                               LocationPtr creatorLocation,
                               const char *name,
                               const char *mimeType,
                               const PublishToRelationshipsMap &publishToRelationships,
                               LocationPtr publishedLocation,
                               Time expires
                               ) :
        PublicationMetaData(
                            1,
                            0,
                            Publication_UniqueLineage::singleton()->getUniqueLineage(),
                            creatorLocation,
                            name,
                            mimeType,
                            Encoding_JSON,  // this will have to be ignored
                            publishToRelationships,
                            publishedLocation,
                            expires
                            )
      {
      }

      //-----------------------------------------------------------------------
      void Publication::init()
      {
        ZS_LOG_DEBUG(log("created") + getDebugValuesString())
        logDocument();
      }

      //-----------------------------------------------------------------------
      PublicationPtr Publication::convert(IPublicationPtr publication)
      {
        return boost::dynamic_pointer_cast<Publication>(publication);
      }

      //-----------------------------------------------------------------------
      Publication::~Publication()
      {
        if (isNoop()) return;
        mThisWeakPublication.reset();
        ZS_LOG_DEBUG(log("destroyed") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication => IPublicationMetaData
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationPtr Publication::toPublication() const
      {
        return mThisWeakPublication.lock();
      }

      //-----------------------------------------------------------------------
      ILocationPtr Publication::getCreatorLocation() const
      {
        AutoRecursiveLock lock(mLock);
        return mCreatorLocation;
      }

      //-----------------------------------------------------------------------
      String Publication::getName() const
      {
        AutoRecursiveLock lock(mLock);
        return mName;
      }

      //-----------------------------------------------------------------------
      String Publication::getMimeType() const
      {
        AutoRecursiveLock lock(mLock);
        return mMimeType;
      }

      //-----------------------------------------------------------------------
      ULONG Publication::getVersion() const
      {
        AutoRecursiveLock lock(mLock);
        return mVersion;
      }

      //-----------------------------------------------------------------------
      ULONG Publication::getBaseVersion() const
      {
        AutoRecursiveLock lock(mLock);
        return mBaseVersion;
      }

      //-----------------------------------------------------------------------
      ULONG Publication::getLineage() const
      {
        AutoRecursiveLock lock(mLock);
        return mLineage;
      }

      //-----------------------------------------------------------------------
      IPublication::Encodings Publication::getEncoding() const
      {
        AutoRecursiveLock lock(mLock);
        if (mDocument) {
          return Encoding_JSON;
        }
        return Encoding_Binary;
      }

      //-----------------------------------------------------------------------
      Time Publication::getExpires() const
      {
        AutoRecursiveLock lock(mLock);
        return mExpires;
      }

      //-----------------------------------------------------------------------
      ILocationPtr Publication::getPublishedLocation() const
      {
        AutoRecursiveLock lock(mLock);
        return mPublishedLocation;
      }

      //-----------------------------------------------------------------------
      const IPublicationMetaData::PublishToRelationshipsMap &Publication::getRelationships() const
      {
        AutoRecursiveLock lock(mLock);
        return mPublishedRelationships;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication => IPublication
      #pragma mark

      //-----------------------------------------------------------------------
      String Publication::toDebugString(IPublicationPtr publication, bool includeCommaPrefix)
      {
        if (!publication) return includeCommaPrefix ? String(", publication=(null)") : String("publication=(null)");
        return Publication::convert(publication)->getDebugValuesString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      PublicationPtr Publication::create(
                                         LocationPtr creatorLocation,
                                         const char *name,
                                         const char *mimeType,
                                         const SecureByteBlock &data,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         LocationPtr publishedLocation,
                                         Time expires
                                         )
      {
        PublicationPtr pThis(new Publication(creatorLocation, name, mimeType, publishToRelationships, publishedLocation, expires));
        pThis->mThisWeak = pThis;
        pThis->mThisWeakPublication = pThis;
        pThis->mPublication = pThis;
        pThis->mData = SecureByteBlockPtr(new SecureByteBlock(data.SizeInBytes()));
        memcpy(*(pThis->mData), data, data.SizeInBytes());
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      PublicationPtr Publication::create(
                                         LocationPtr creatorLocation,
                                         const char *name,
                                         const char *mimeType,
                                         DocumentPtr documentToBeAdopted,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         LocationPtr publishedLocation,
                                         Time expires
                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!documentToBeAdopted)

        PublicationPtr pThis(new Publication(creatorLocation, name, mimeType, publishToRelationships, publishedLocation, expires));
        pThis->mThisWeak = pThis;
        pThis->mThisWeakPublication = pThis;
        pThis->mPublication = pThis;
        pThis->mDocument = documentToBeAdopted;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      PublicationPtr Publication::create(
                                         LocationPtr creatorLocation,
                                         const char *name,
                                         const char *mimeType,
                                         const RelationshipList &relationshipsDocument,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         LocationPtr publishedLocation,
                                         Time expires
                                         )
      {
        DocumentPtr doc = createDocumentFromRelationships(relationshipsDocument);
        return IPublicationFactory::singleton().create(creatorLocation, name, mimeType, doc, publishToRelationships, publishedLocation, expires);
      }

      //-----------------------------------------------------------------------
      void Publication::update(const SecureByteBlock &data)
      {
        AutoRecursiveLock lock(mLock);

        mData = SecureByteBlockPtr(new SecureByteBlock(data.SizeInBytes()));
        memcpy(*mData, data, data.SizeInBytes());

        mDiffDocuments.clear();
        mDocument.reset();

        ++mVersion;
      }

      //-----------------------------------------------------------------------
      void Publication::update(DocumentPtr updatedDocumentToBeAdopted)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!updatedDocumentToBeAdopted)

        AutoRecursiveLock lock(mLock);
        ZS_LOG_DETAIL(log("updating document") + getDebugValuesString())
        if (ZS_IS_LOGGING(Trace)) {
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_BASIC(log("Updating from JSON:") + "\n" + internal::toString(updatedDocumentToBeAdopted) + "\n")
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
        }

        ++mVersion;

        mData.reset();

        ElementPtr diffElem = updatedDocumentToBeAdopted->findFirstChildElement(HOOKFLASH_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME); // root for diffs elements
        if (!diffElem) {
          mDiffDocuments.clear();

          mDocument = updatedDocumentToBeAdopted;
          return;
        }

        if (mDiffDocuments.end() == mDiffDocuments.find(mVersion-1)) {
          if (mVersion > 2) {
            ZS_LOG_WARNING(Detail, log("diff document does not contain version in a row"))
          }
          // if diffs are not in a row then erase the diffs...
          mDiffDocuments.clear();
        }

        // this is a difference document
        IDiff::process(mDocument, updatedDocumentToBeAdopted);

        mDiffDocuments[mVersion] = updatedDocumentToBeAdopted;

        ZS_LOG_DEBUG(log("updating document complete") + getDebugValuesString())

        if (ZS_IS_LOGGING(Trace)) {
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"))
          ZS_LOG_BASIC(log("FINAL JSON:") + "\n" + internal::toString(mDocument) + "\n")
          ZS_LOG_DEBUG(log("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
        }
      }

      //-----------------------------------------------------------------------
      void Publication::update(const RelationshipList &relationships)
      {
        DocumentPtr document = createDocumentFromRelationships(relationships);
        update(document);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Publication::getRawData(AutoRecursiveLockPtr &outDocumentLock) const
      {
        outDocumentLock = AutoRecursiveLockPtr(new AutoRecursiveLock(mLock));
        return mData;
      }

      //-----------------------------------------------------------------------
      DocumentPtr Publication::getJSON(AutoRecursiveLockPtr &outDocumentLock) const
      {
        outDocumentLock = AutoRecursiveLockPtr(new AutoRecursiveLock(mLock));
        return mDocument;
      }

      //-----------------------------------------------------------------------
      IPublication::RelationshipListPtr Publication::getAsContactList() const
      {
        AutoRecursiveLock lock(mLock);

        ZS_LOG_TRACE(log("getting publication as contact list") + getDebugValuesString())

        RelationshipListPtr result = RelationshipListPtr(new RelationshipList);

        RelationshipList &outList = (*result);

        if (!mDocument) {
          ZS_LOG_WARNING(Detail, log("publication document is empty") + getDebugValuesString())
          return result;
        }

        ElementPtr contactsEl = mDocument->findFirstChildElement("contacts");
        if (!contactsEl) {
          ZS_LOG_WARNING(Debug, log("unable to find contact root element") + getDebugValuesString())
          return result;
        }

        ElementPtr contactEl = contactsEl->findFirstChildElement("contact");
        while (contactEl) {
          String contact = contactEl->getTextDecoded();
          if (!contact.isEmpty()) {
            ZS_LOG_TRACE(log("found contact") + ", contact URI=" + contact)
            outList.push_back(contact);
          }
          contactEl = contactEl->findNextSiblingElement("contact");
        }

        ZS_LOG_TRACE(log("end of getting as contact list") + ", total=" + Stringize<size_t>(outList.size()).string())
        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication => IPublicationForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationMetaDataPtr Publication::toPublicationMetaData() const
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      LocationPtr Publication::getCreatorLocation(bool) const
      {
        AutoRecursiveLock lock(mLock);
        return mCreatorLocation;
      }

      //-----------------------------------------------------------------------
      LocationPtr Publication::getPublishedLocation(bool) const
      {
        AutoRecursiveLock lock(mLock);
        return mPublishedLocation;
      }

      //-----------------------------------------------------------------------
      bool Publication::isMatching(
                                   const IPublicationMetaDataPtr &metaData,
                                   bool ignoreLineage
                                   ) const
      {
        AutoRecursiveLock lock(mLock);
        return PublicationMetaData::isMatching(metaData, ignoreLineage);
      }

      //-----------------------------------------------------------------------
      bool Publication::isLessThan(
                                   const IPublicationMetaDataPtr &metaData,
                                   bool ignoreLineage
                                   ) const
      {
        AutoRecursiveLock lock(mLock);
        return PublicationMetaData::isLessThan(metaData, ignoreLineage);
      }

      //-----------------------------------------------------------------------
      void Publication::setVersion(ULONG version)
      {
        AutoRecursiveLock lock(mLock);
        mVersion = version;
      }

      //-----------------------------------------------------------------------
      void Publication::setBaseVersion(ULONG version)
      {
        AutoRecursiveLock lock(mLock);
        mBaseVersion = version;
      }

      //-----------------------------------------------------------------------
      void Publication::setCreatorLocation(LocationPtr location)
      {
        AutoRecursiveLock lock(mLock);
        mCreatorLocation = location;

        ZS_LOG_TRACE(log("updated internal publication creator information") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void Publication::setPublishedLocation(LocationPtr location)
      {
        AutoRecursiveLock lock(mLock);
        mPublishedLocation = location;

        ZS_LOG_TRACE(log("updated internal publication published to contact information") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void Publication::setExpires(Time expires)
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_TRACE(log("updating expires time") + ", was=" + Stringize<Time>(mExpires).string() + ", now=" + Stringize<Time>(expires).string())

        mExpires = expires;
      }

      //-----------------------------------------------------------------------
      Time Publication::getCacheExpires() const
      {
        AutoRecursiveLock lock(mLock);
        return mCacheExpires;
      }

      //-----------------------------------------------------------------------
      void Publication::setCacheExpires(Time expires)
      {
        AutoRecursiveLock lock(mLock);
        mCacheExpires = expires;
      }

      //-----------------------------------------------------------------------
      void Publication::updateFromFetchedPublication(
                                                     PublicationPtr fetchedPublication,
                                                     bool *noThrowVersionMismatched
                                                     ) throw (Exceptions::VersionMismatch)
      {
        if (NULL != noThrowVersionMismatched)
          *noThrowVersionMismatched = false;

        AutoRecursiveLock lock(mLock);

        ZS_LOG_DETAIL(log("updating from fetched publication") + getDebugValuesString())

        PublicationPtr publication = convert(fetchedPublication);

        if (publication->mData) {
          mData = publication->mData;
          mDocument.reset();
        } else {
          ElementPtr diffEl = publication->mDocument->findFirstChildElement(HOOKFLASH_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME);
          if (diffEl) {
            if (publication->mBaseVersion != mVersion + 1) {
              if (NULL != noThrowVersionMismatched) {
                *noThrowVersionMismatched = true;
                return;
              }
              ZS_THROW_CUSTOM(Exceptions::VersionMismatch, "remote party sent diff based on wrong document" + getDebugValuesString())
              return;
            }
            IDiff::process(mDocument, publication->mDocument);
          } else {
            mDocument = publication->mDocument;
          }
        }

        mCreatorLocation = publication->mCreatorLocation;

        mMimeType = publication->mMimeType;
        mVersion = publication->mVersion;
        mBaseVersion = 0;
        mLineage = publication->mLineage;

        mExpires = publication->mExpires;

        mPublishedLocation = publication->mPublishedLocation;

        mPublishedRelationships = publication->mPublishedRelationships;

        mDiffDocuments.clear();

        logDocument();

        ZS_LOG_DEBUG(log("updating from fetched publication complete") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void Publication::getDiffVersionsOutputSize(
                                                  ULONG fromVersionNumber,
                                                  ULONG toVersionNumber,
                                                  ULONG &outOutputSizeInBytes,
                                                  bool rawSizeOkay
                                                  ) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(fromVersionNumber > toVersionNumber)

        AutoRecursiveLock lock(mLock);
        outOutputSizeInBytes = 0;

        if (!mDocument) {
          if (!mData) {
            ZS_LOG_WARNING(Detail, log("no data available to return") + getDebugValuesString())
            return;
          }

          if (rawSizeOkay) {
            ZS_LOG_TRACE(log("document is binary data thus returning non-encoded raw size (no base64 calculating required)") +  + getDebugValuesString())
            outOutputSizeInBytes = mData->SizeInBytes();
            return;
          }

          ZS_LOG_TRACE(log("document is binary data thus returning base64 bit encoded size (which required calculating)") +  + getDebugValuesString())

          ULONG fromVersion = 0;
          NodePtr node = getDiffs(fromVersion, mVersion);

          GeneratorPtr generator =  Generator::createJSONGenerator();
          outOutputSizeInBytes = generator->getOutputSize(node);
          return;
        }

        for (ULONG from = fromVersionNumber; from <= toVersionNumber; ++from) {
          DiffDocumentMap::const_iterator found = mDiffDocuments.find(from);
          if (found == mDiffDocuments.end()) {
            ZS_LOG_TRACE(log("diff is not available (thus returning entire document size)") + ", from=" + Stringize<ULONG>(fromVersionNumber).string() + ", current=" + Stringize<ULONG>(from).string() + getDebugValuesString())
            getEntirePublicationOutputSize(outOutputSizeInBytes);
            return;
          }

          ZS_LOG_TRACE(log("returning size of latest version diff's document") + ", from=" + Stringize<ULONG>(fromVersionNumber).string() + ", current=" + Stringize<ULONG>(from).string() +  + getDebugValuesString())
          const DocumentPtr &doc = (*found).second;
          GeneratorPtr generator = Generator::createJSONGenerator();
          outOutputSizeInBytes += generator->getOutputSize(doc);
        }
      }

      //-----------------------------------------------------------------------
      void Publication::getEntirePublicationOutputSize(
                                                       ULONG &outOutputSizeInBytes,
                                                       bool rawSizeOkay
                                                       ) const
      {
        outOutputSizeInBytes = 0;

        AutoRecursiveLock lock(mLock);

        if (!mDocument) {
          getDiffVersionsOutputSize(0, 0, outOutputSizeInBytes);
          return;
        }
        GeneratorPtr generator = Generator::createJSONGenerator();
        outOutputSizeInBytes = generator->getOutputSize(mDocument);
      }

      //-----------------------------------------------------------------------
      String Publication::getDebugValuesString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(mLock);

        LocationPtr creatorLocation = Location::convert(getCreatorLocation());
        LocationPtr publishedLocation = Location::convert(getPublishedLocation());

        bool first = !includeCommaPrefix;

        return debugNameValue(first, "id", Stringize<PUID>(mID).string())
             + debugNameValue(first, "name", mName)
             + debugNameValue(first, "version", (0 == mVersion ? String() : Stringize<ULONG>(mVersion).string()))
             + debugNameValue(first, "base version", (0 == mBaseVersion ? String() : Stringize<ULONG>(mBaseVersion).string()))
             + debugNameValue(first, "lineage", (0 == mLineage ? String() : Stringize<ULONG>(mLineage).string()))
             + debugNameValue(first, "creator: ", creatorLocation ? creatorLocation->forPublication().getDebugValueString() : String(), false)
             + debugNameValue(first, "published: ", publishedLocation ? publishedLocation->forPublication().getDebugValueString() : String(), false)
             + debugNameValue(first, "mime type", mMimeType)
             + debugNameValue(first, "expires", (Time() == mExpires ? String() : Stringize<Time>(mExpires).string()))
             + debugNameValue(first, "data length", mData ? (0 == mData->SizeInBytes() ? String() : Stringize<size_t>(mData->SizeInBytes()).string()) : String())
             + debugNameValue(first, "diffs total", (mDiffDocuments.size() < 1 ? String() : Stringize<size_t>(mDiffDocuments.size())))
             + debugNameValue(first, "total relationships", (mPublishedRelationships.size() < 1 ? String() : Stringize<size_t>(mPublishedRelationships.size())));
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication => IPublicationForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      PublicationPtr Publication::create(
                                         ULONG version,
                                         ULONG baseVersion,
                                         ULONG lineage,
                                         LocationPtr creatorLocation,
                                         const char *name,
                                         const char *mimeType,
                                         ElementPtr dataEl,
                                         IPublicationMetaData::Encodings encoding,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         LocationPtr publishedLocation,
                                         Time expires
                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!dataEl)

        PublicationPtr pThis(new Publication(creatorLocation, name, mimeType, publishToRelationships, publishedLocation, expires));
        pThis->mThisWeak = pThis;
        pThis->mThisWeakPublication = pThis;
        pThis->mPublication = pThis;
        pThis->mVersion = version;
        pThis->mBaseVersion = baseVersion;
        pThis->mLineage = lineage;
        switch (encoding) {
          case IPublicationMetaData::Encoding_Binary: {
            String base64String = dataEl->getTextDecoded();
            pThis->mData = IHelper::convertFromBase64(base64String);
            break;
          }
          case IPublicationMetaData::Encoding_JSON: {
            DocumentPtr doc = Document::create();
            NodePtr node = dataEl->getFirstChild();
            while (node) {
              NodePtr next = node->getNextSibling();
              node->orphan();
              doc->adoptAsLastChild(node);
              node = next;
            }
            pThis->mDocument = doc;
            break;
          }
        }
        pThis->init();
        return pThis;
      }
      
      //-----------------------------------------------------------------------
      NodePtr Publication::getDiffs(
                                    ULONG &ioFromVersion,
                                    ULONG toVersion
                                    ) const
      {
        AutoRecursiveLock lock(mLock);

        ZS_LOG_DEBUG(log("requested JSON diffs") + ", from version=" +Stringize<ULONG>(ioFromVersion).string() + ", to version=" + Stringize<ULONG>(toVersion).string())

        if ((0 == toVersion) ||
            (toVersion >= mVersion)) {
          toVersion = mVersion;
        }

        if (!mDocument) {
          if (!mData) {
            ZS_LOG_WARNING(Detail, log("JSON for publishing has no data available to return") + getDebugValuesString())
            return NodePtr();
          }

          ZS_LOG_WARNING(Detail, log("returning data as base 64 encoded") + getDebugValuesString())

          String data = IHelper::convertToBase64(*mData);
          TextPtr text = Text::create();
          text->setValue(data);
          return text;
        }

        if (0 == ioFromVersion) {
          ZS_LOG_DEBUG(log("first time publishing or fetching document thus returning entire document") + getDebugValuesString())

          ElementPtr firstEl = mDocument->getFirstChildElement();
          if (!firstEl) return firstEl;
          return firstEl->clone();
        }

        ZS_LOG_DEBUG(log("publishing or fetching differences from last version published") + ", from base version=" + Stringize<ULONG>(toVersion).string() + getDebugValuesString() + ", to version=" + Stringize<ULONG>(toVersion).string())

        // see if we have diffs from the last published point
        DiffDocumentMap::const_iterator foundFrom = mDiffDocuments.find(ioFromVersion);
        DiffDocumentMap::const_iterator foundTo = mDiffDocuments.find(toVersion);

        if ((foundFrom == mDiffDocuments.end()) ||
            (foundTo == mDiffDocuments.end())) {

          ZS_LOG_WARNING(Detail, log("unable to find JSON differences for requested publication (thus returning whole document)"))
          ioFromVersion = 0;
          return getDiffs(ioFromVersion, toVersion);
        }

        DocumentPtr cloned;

        ++foundTo;

        VersionNumber currentVersion = ioFromVersion;

        ElementPtr diffOutputEl;

        // we have the diffs, process them into one document
        for (DiffDocumentMap::const_iterator iter = foundFrom; iter != foundTo; ++iter, ++currentVersion) {
          const VersionNumber &versionNumber = (*iter).first;

          ZS_LOG_TRACE(log("processing diff") + ", version=" + Stringize<VersionNumber>(versionNumber).string())

          if (currentVersion != versionNumber) {
            ZS_LOG_ERROR(Detail, log("JSON differences has a version number hole") + ", expecting=" + Stringize<VersionNumber>(currentVersion).string() + ", found=" + Stringize<VersionNumber>(versionNumber).string())
            ioFromVersion = 0;
            return getDiffs(ioFromVersion, toVersion);
          }

          DocumentPtr doc = (*iter).second;

          try {
            if (!cloned) {
              cloned = doc->clone()->toDocument();
              diffOutputEl = cloned->findFirstChildElementChecked(HOOKFLASH_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME);
            } else {
              diffOutputEl = cloned->findFirstChildElementChecked(HOOKFLASH_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME);

              // we need to process all elements and insert them into the other...
              ElementPtr diffEl = doc->findFirstChildElementChecked(HOOKFLASH_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME);

              ElementPtr itemEl = diffEl->findFirstChildElement(HOOKFLASH_STACK_DIFF_DOCUMENT_ITEM_ELEMENT_NAME);
              while (itemEl) {
                diffOutputEl->adoptAsLastChild(itemEl->clone());
                itemEl = itemEl->findNextSiblingElement(HOOKFLASH_STACK_DIFF_DOCUMENT_ITEM_ELEMENT_NAME);
              }
            }
          } catch (CheckFailed &) {
            ZS_LOG_ERROR(Detail, log("JSON diff document is corrupted (recovering by returning entire document)") + ", version=" + Stringize<VersionNumber>(versionNumber).string())
            ioFromVersion = 0;
            return getDiffs(ioFromVersion, toVersion);
          }
        }

        ZS_THROW_INVALID_ASSUMPTION_IF(!cloned)
        ZS_THROW_INVALID_ASSUMPTION_IF(!diffOutputEl)

        ZS_LOG_DEBUG(log("returning orphaned clone of differences document"))
        diffOutputEl->orphan();
        return diffOutputEl;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Publication::log(const char *message) const
      {
        return String("Publication [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void Publication::logDocument() const
      {
        if (ZS_IS_LOGGING(Trace)) {
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          if (mDocument) {
            ZS_LOG_BASIC(log("publication contains JSON:") + "\n" + internal::toString(mDocument) + "\n")
          } else if (mData) {
            ZS_LOG_BASIC(log("publication contains binary data") + ", length=" + Stringize<size_t>(mData ? mData->SizeInBytes() : 0).string())
          } else {
            ZS_LOG_BASIC(log("publication is NULL"))
          }
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
        }
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPublication
    #pragma mark

    //-------------------------------------------------------------------------
    String IPublication::toDebugString(IPublicationPtr publication, bool includeCommaPrefix)
    {
      return internal::Publication::toDebugString(publication, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IPublicationPtr IPublication::create(
                                         ILocationPtr creatorLocation,
                                         const char *name,
                                         const char *mimeType,
                                         const SecureByteBlock &data,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         ILocationPtr publishedLocation,
                                         Time expires
                                         )
    {
      return internal::IPublicationFactory::singleton().create(internal::Location::convert(creatorLocation), name, mimeType, data, publishToRelationships, internal::Location::convert(publishedLocation), expires);
    }

    //-------------------------------------------------------------------------
    IPublicationPtr IPublication::create(
                                         ILocationPtr creatorLocation,
                                         const char *name,
                                         const char *mimeType,
                                         DocumentPtr documentToBeAdopted,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         ILocationPtr publishedLocation,
                                         Time expires
                                         )
    {
      return internal::IPublicationFactory::singleton().create(internal::Location::convert(creatorLocation), name, mimeType, documentToBeAdopted, publishToRelationships, internal::Location::convert(publishedLocation), expires);
    }

    //-------------------------------------------------------------------------
    IPublicationPtr IPublication::create(
                                         ILocationPtr creatorLocation,
                                         const char *name,
                                         const char *mimeType,
                                         const RelationshipList &relationshipsDocument,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         ILocationPtr publishedLocation,
                                         Time expires
                                         )
    {
      return internal::Publication::create(internal::Location::convert(creatorLocation), name, mimeType, relationshipsDocument, publishToRelationships, internal::Location::convert(publishedLocation), expires);
    }
  }
}
