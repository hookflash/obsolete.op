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

#include <hookflash/stack/internal/stack_Publication.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/IXMLDiff.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    using zsLib::Stringize;

    typedef zsLib::BYTE BYTE;
    typedef zsLib::ULONG ULONG;
    typedef zsLib::CSTR CSTR;
    typedef zsLib::String String;
    typedef zsLib::Time Time;
    typedef zsLib::XML::NodePtr NodePtr;
    typedef zsLib::XML::Document Document;
    typedef zsLib::XML::DocumentPtr DocumentPtr;
    typedef zsLib::XML::Element Element;
    typedef zsLib::XML::ElementPtr ElementPtr;
    typedef zsLib::XML::Text Text;
    typedef zsLib::XML::TextPtr TextPtr;

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      static DocumentPtr createDocumentFromRelationships(const IPublication::RelationshipList &relationships)
      {
        typedef IPublication::RelationshipList RelationshipList;

        DocumentPtr doc = Document::create();
        ElementPtr contactsEl = Element::create();
        contactsEl->setValue("contacts");

        for (RelationshipList::const_iterator iter = relationships.begin(); iter != relationships.end(); ++iter)
        {
          const String &id = (*iter);

          ElementPtr contactEl = Element::create();
          contactEl->setValue("contact");
          contactEl->setAttribute("id", id);

          contactsEl->adoptAsLastChild(contactEl);
        }

        doc->adoptAsLastChild(contactsEl);

        return doc;
      }

      //-----------------------------------------------------------------------
      static String toString(NodePtr node)
      {
        NodePtr previousParent;
        NodePtr previousSibling;
        ElementPtr element;

        DocumentPtr doc;

        if (!node) {
          return "<!-- NULL -->";
        }

        if (node->isElement()) {
          previousSibling = node->getPreviousSibling();
          previousParent = node->getParent();

          element = node->clone()->toElement();
          ZS_THROW_BAD_STATE_IF(!element)

          doc = Document::create();
          doc->adoptAsLastChild(element);
        } else if (node->isDocument()) {
          doc = node->toDocument();
        } else {
          return "<!-- UNSUPPORTED -->";
        }
        if (!doc) {
          return "<!-- UNSUPPORTED -->";
        }

        boost::shared_array<char> output;
        ULONG length = 0;
        output = doc->write(&length);

        // to avoid cloning but the element back into it's original place
        if (element) {
          if (previousSibling) {
            previousSibling->adoptAsNextSibling(element);
          } else if (previousParent) {
            previousParent->adoptAsFirstChild(element);
          } else {
            element->orphan();
          }
        }

        return (CSTR)output.get();
      }

      //-----------------------------------------------------------------------
      class Publication_UniqueLineage;
      typedef boost::shared_ptr<Publication_UniqueLineage> Publication_UniqueLineagePtr;
      typedef boost::weak_ptr<Publication_UniqueLineage> Publication_UniqueLineageWeakPtr;

      class Publication_UniqueLineage
      {
      public:
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::AutoRecursiveLock AutoRecursiveLock;

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
      IPublicationForPublicationRepositoryPtr IPublicationForPublicationRepository::convert(IPublicationPtr publication)
      {
        return boost::dynamic_pointer_cast<IPublicationForPublicationRepository>(publication);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPublicationForMessages
      #pragma mark

      IPublicationForMessagesPtr IPublicationForMessages::convert(IPublicationPtr publication)
      {
        return boost::dynamic_pointer_cast<Publication>(publication);
      }

      //-----------------------------------------------------------------------
      IPublicationForMessagesPtr IPublicationForMessages::create(
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
                                                                       )
      {
        return Publication::create(
                                   version,
                                   baseVersion,
                                   lineage,
                                   source,
                                   creatorContactID,
                                   creatorLocationID,
                                   name,
                                   mimeType,
                                   dataEl,
                                   encoding,
                                   publishToRelationships,
                                   peerContactID,
                                   peerLocationID,
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
      #pragma mark Publication
      #pragma mark

      //-----------------------------------------------------------------------
      Publication::Publication(
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
                               ) :
        mID(zsLib::createPUID()),
        mSource(source),
        mContactID(creatorContactID ? creatorContactID : ""),
        mLocationID(creatorLocationID ? creatorLocationID : ""),
        mName(name ? name : ""),
        mMimeType(mimeType ? mimeType : ""),
        mPublishedRelationships(publishToRelationships),
        mPublishedToContactID(peerContactID ? peerContactID : ""),
        mPublishedToLocationID(peerLocationID ? peerLocationID : ""),
        mScope(scope),
        mLifetime(lifetime),
        mExpires(expires),
        mDataLengthInBytes(0),
        mVersion(1),
        mBaseVersion(0),
        mLineage(Publication_UniqueLineage::singleton()->getUniqueLineage())
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
      PublicationPtr Publication::convert(IPublicationForPublicationRepositoryPtr publication)
      {
        return boost::dynamic_pointer_cast<Publication>(publication);
      }

      //-----------------------------------------------------------------------
      Publication::~Publication()
      {
        mThisWeak.reset();
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
      IPublicationPtr Publication::getPublication() const
      {
        return mThisWeak.lock();
      }

      String Publication::getCreatorContactID() const
      {
        AutoRecursiveLock lock(mLock);
        return mContactID;
      }

      //-----------------------------------------------------------------------
      String Publication::getCreatorLocationID() const
      {
        AutoRecursiveLock lock(mLock);
        return mLocationID;
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
          return Encoding_XML;
        }
        return Encoding_Binary;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Sources Publication::getSource() const
      {
        AutoRecursiveLock lock(mLock);
        return mSource;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Scopes Publication::getScope() const
      {
        AutoRecursiveLock lock(mLock);
        return mScope;
      }

      //-----------------------------------------------------------------------
      IPublicationMetaData::Lifetimes Publication::getLifetime() const
      {
        AutoRecursiveLock lock(mLock);
        return mLifetime;
      }

      //-----------------------------------------------------------------------
      Time Publication::getExpires() const
      {
        AutoRecursiveLock lock(mLock);
        return mExpires;
      }

      //-----------------------------------------------------------------------
      String Publication::getPublishedToContactID() const
      {
        AutoRecursiveLock lock(mLock);
        return mPublishedToContactID;
      }

      //-----------------------------------------------------------------------
      String Publication::getPublishedToLocationID() const
      {
        AutoRecursiveLock lock(mLock);
        return mPublishedToLocationID;
      }

      //-----------------------------------------------------------------------
      void Publication::getRelationships(PublishToRelationshipsMap &outRelationships) const
      {
        AutoRecursiveLock lock(mLock);
        outRelationships = mPublishedRelationships;
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
      PublicationPtr Publication::create(
                                         Sources source,
                                         const char *creatorContactID,
                                         const char *creatorLocationID,
                                         const char *name,
                                         const char *mimeType,
                                         const BYTE *data,
                                         size_t sizeInBytes,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         const char *peerContactID,
                                         const char *peerLocationID,
                                         Scopes scope,
                                         Lifetimes lifetime,
                                         Time expires
                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!data)

        PublicationPtr pThis(new Publication(source, creatorContactID, creatorLocationID, name, mimeType, publishToRelationships, peerContactID, peerLocationID, scope, lifetime, expires));
        pThis->mThisWeak = pThis;
        pThis->mData = boost::shared_array<BYTE>(new BYTE[sizeInBytes ? sizeInBytes : 1]);
        memcpy(pThis->mData.get(), data, sizeInBytes);
        pThis->mDataLengthInBytes = sizeInBytes;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      PublicationPtr Publication::create(
                                         Sources source,
                                         const char *creatorContactID,
                                         const char *creatorLocationID,
                                         const char *name,
                                         const char *mimeType,
                                         DocumentPtr documentToBeAdopted,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         const char *peerContactID,
                                         const char *peerLocationID,
                                         Scopes scope,
                                         Lifetimes lifetime,
                                         Time expires
                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!documentToBeAdopted)

        PublicationPtr pThis(new Publication(source, creatorContactID, creatorLocationID, name, mimeType, publishToRelationships, peerContactID, peerLocationID, scope, lifetime, expires));
        pThis->mThisWeak = pThis;
        pThis->mDocument = documentToBeAdopted;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      PublicationPtr Publication::create(
                                         Sources source,
                                         const char *creatorContactID,
                                         const char *creatorLocationID,
                                         const char *name,
                                         const char *mimeType,
                                         const RelationshipList &relationshipsDocument,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         const char *peerContactID,
                                         const char *peerLocationID,
                                         Scopes scope,
                                         Lifetimes lifetime,
                                         Time expires
                                         )
      {
        DocumentPtr doc = createDocumentFromRelationships(relationshipsDocument);
        return Publication::create(source, creatorContactID, creatorLocationID, name, mimeType, doc, publishToRelationships, peerContactID, peerLocationID, scope, lifetime, expires);
      }

      //-----------------------------------------------------------------------
      PublicationPtr Publication::create(
                                         ULONG version,
                                         ULONG baseVersion,
                                         ULONG lineage,
                                         Sources source,
                                         const char *creatorContactID,
                                         const char *creatorLocationID,
                                         const char *name,
                                         const char *mimeType,
                                         ElementPtr dataEl,
                                         IPublicationMetaData::Encodings encoding,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         const char *peerContactID,
                                         const char *peerLocationID,
                                         Scopes scope,
                                         Lifetimes lifetime,
                                         Time expires
                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!dataEl)

        PublicationPtr pThis(new Publication(source, creatorContactID, creatorLocationID, name, mimeType, publishToRelationships, peerContactID, peerLocationID, scope, lifetime, expires));
        pThis->mThisWeak = pThis;
        pThis->mVersion = version;
        pThis->mBaseVersion = baseVersion;
        pThis->mLineage = lineage;
        switch (encoding) {
          case IPublicationMetaData::Encoding_Binary: {
            String base64 = dataEl->getValue();
            SecureByteBlock output;
            IHelper::convertFromBase64(base64, output);
            size_t size = output.size();
            BYTE *buffer = output.data();
            boost::shared_array<BYTE> temp(new BYTE[size+1]);
            memset(temp.get(), 0, sizeof(BYTE)*(size+1));
            memcpy(temp.get(), buffer, size);
            pThis->mData = temp;
            pThis->mDataLengthInBytes = size;
            break;
          }
          case IPublicationMetaData::Encoding_XML: {
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
      void Publication::update(
                               const BYTE *data,
                               size_t sizeInBytes
                               )
      {
        AutoRecursiveLock lock(mLock);

        mData = boost::shared_array<BYTE>(new BYTE[sizeInBytes ? sizeInBytes : 1]);
        memcpy(mData.get(), data, sizeInBytes);
        mDataLengthInBytes = sizeInBytes;

        mXMLDiffDocuments.clear();
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
          ZS_LOG_BASIC(log("Updating from XML:") + "\n" + internal::toString(updatedDocumentToBeAdopted) + "\n")
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
        }

        ++mVersion;

        mData.reset();
        mDataLengthInBytes = 0;

        ElementPtr xdsElem = updatedDocumentToBeAdopted->findFirstChildElement("xds"); // root for XML diffs elements
        if (!xdsElem) {
          mXMLDiffDocuments.clear();

          mDocument = updatedDocumentToBeAdopted;
          return;
        }

        if (mXMLDiffDocuments.end() == mXMLDiffDocuments.find(mVersion-1)) {
          if (mVersion > 2) {
            ZS_LOG_WARNING(Detail, log("diff document does not contain version in a row"))
          }
          // if diffs are not in a row then erase the diffs...
          mXMLDiffDocuments.clear();
        }

        // this is a difference document
        IXMLDiff::process(mDocument, updatedDocumentToBeAdopted);

        mXMLDiffDocuments[mVersion] = updatedDocumentToBeAdopted;

        ZS_LOG_DEBUG(log("updating document complete") + getDebugValuesString())

        if (ZS_IS_LOGGING(Trace)) {
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("..............................................................................."))
          ZS_LOG_DEBUG(log("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"))
          ZS_LOG_BASIC(log("FINAL XML:") + "\n" + internal::toString(mDocument) + "\n")
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
      void Publication::getRawData(
                                   AutoRecursiveLockPtr &outDocumentLock,
                                   boost::shared_array<BYTE> &outputBuffer,
                                   size_t &outputBufferSizeInBytes
                                   ) const
      {
        outDocumentLock = AutoRecursiveLockPtr(new AutoRecursiveLock(mLock));
        outputBuffer = mData;
        outputBufferSizeInBytes = mDataLengthInBytes;
      }

      //-----------------------------------------------------------------------
      DocumentPtr Publication::getXML(AutoRecursiveLockPtr &outDocumentLock) const
      {
        outDocumentLock = AutoRecursiveLockPtr(new AutoRecursiveLock(mLock));
        return mDocument;
      }

      //-----------------------------------------------------------------------
      void Publication::getAsContactList(RelationshipList &outList) const
      {
        AutoRecursiveLock lock(mLock);

        ZS_LOG_TRACE(log("getting publication as contact list") + getDebugValuesString())

        outList.clear();
        if (!mDocument) {
          ZS_LOG_WARNING(Detail, log("publication document is empty") + getDebugValuesString())
          return;
        }

        ElementPtr contactsElem = mDocument->findFirstChildElement("contacts");
        if (!contactsElem) {
          ZS_LOG_WARNING(Debug, log("unable to find contact root element") + getDebugValuesString())
          return;
        }

        ElementPtr contactElem = contactsElem->findFirstChildElement("contact");
        while (contactElem) {
          String id = contactElem->getAttributeValue("id");
          if (!id.isEmpty()) {
            ZS_LOG_TRACE(log("found contact") + ", contact ID=" + id)
            outList.push_back(id);
          }
          contactElem = contactElem->findNextSiblingElement("contact");
        }

        ZS_LOG_TRACE(log("end of getting as contact list") + ", total=" + Stringize<size_t>(outList.size()).string())
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication => IPublicationForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      IPublicationMetaDataPtr Publication::convertIPublicationMetaData() const
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      IPublicationPtr Publication::convertIPublication() const
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      bool Publication::isMatching(
                                   const IPublicationMetaDataForPublicationRepositoryPtr &metaData,
                                   bool ignoreLineage
                                   ) const
      {
        AutoRecursiveLock lock(mLock);
        if (metaData->getSource() != mSource) return false;
        if (!ignoreLineage) {
          if (metaData->getLineage() != mLineage) return false;
        }
        if (metaData->getName() != mName) return false;
        if (metaData->getCreatorContactID() != mContactID) return false;
        if (metaData->getCreatorLocationID() != mLocationID) return false;
        if (IPublicationMetaData::Source_Peer == mSource) {
          if (metaData->getPublishedToContactID() != mPublishedToContactID) return false;
          if (metaData->getPublishedToLocationID() != mPublishedToLocationID) return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool Publication::isLessThan(
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
      void Publication::setSource(IPublicationMetaData::Sources source)
      {
        AutoRecursiveLock lock(mLock);
        mSource = source;
      }

      //-----------------------------------------------------------------------
      void Publication::setCreatorContact(const char *contactID, const char *locationID)
      {
        AutoRecursiveLock lock(mLock);
        mContactID = contactID;
        mLocationID = locationID;

        ZS_LOG_TRACE(log("updated internal publication creator contact information") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      void Publication::setPublishedToContact(const char *contactID, const char *locationID)
      {
        AutoRecursiveLock lock(mLock);
        mPublishedToContactID = contactID;
        mPublishedToLocationID = locationID;

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
                                                     IPublicationForPublicationRepositoryPtr fetchedPublication,
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
          mDataLengthInBytes = publication->mDataLengthInBytes;
          mDocument.reset();
        } else {
          ElementPtr xdsEl = publication->mDocument->findFirstChildElement("xds");
          if (xdsEl) {
            if (publication->mBaseVersion != mVersion + 1) {
              if (NULL != noThrowVersionMismatched) {
                *noThrowVersionMismatched = true;
                return;
              }
              ZS_THROW_CUSTOM(Exceptions::VersionMismatch, "remote party sent diff based on wrong document" + getDebugValuesString())
              return;
            }
            IXMLDiff::process(mDocument, publication->mDocument);
          } else {
            mDocument = publication->mDocument;
          }
        }

        mContactID = publication->mContactID;
        mLocationID = publication->mLocationID;

        mMimeType = publication->mMimeType;
        mVersion = publication->mVersion;
        mBaseVersion = 0;
        mLineage = publication->mLineage;

        mScope = publication->mScope;
        mSource = publication->mSource;
        mLifetime = publication->mLifetime;
        mExpires = publication->mExpires;

        mPublishedToContactID = publication->mPublishedToContactID;
        mPublishedToLocationID = publication->mPublishedToLocationID;

        mPublishedRelationships = publication->mPublishedRelationships;

        mXMLDiffDocuments.clear();

        logDocument();
        ZS_LOG_DEBUG(log("updating from fetched publication complete") + getDebugValuesString())
      }

      //-----------------------------------------------------------------------
      static String debugNameValue(const String &name, const String &value)
      {
        if (value.isEmpty()) return String();
        return String(", ") + name + "=" + value;
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
            outOutputSizeInBytes = mDataLengthInBytes;
            return;
          }

          ZS_LOG_TRACE(log("document is binary data thus returning base64 bit encoded size (which required calculating)") +  + getDebugValuesString())
          ULONG fromVersion = 0;
          NodePtr node = getXMLDiffs(fromVersion, mVersion);
          DocumentPtr doc = Document::create();
          doc->adoptAsFirstChild(node);
          outOutputSizeInBytes = doc->getOutputSize();
          return;
        }

        for (ULONG from = fromVersionNumber; from <= toVersionNumber; ++from) {
          XMLDiffDocumentMap::const_iterator found = mXMLDiffDocuments.find(from);
          if (found == mXMLDiffDocuments.end()) {
            ZS_LOG_TRACE(log("diff is not available (thus returning entire document size)") + ", from=" + Stringize<ULONG>(fromVersionNumber).string() + ", current=" + Stringize<ULONG>(from).string() + getDebugValuesString())
            getEntirePublicationOutputSize(outOutputSizeInBytes);
            return;
          }

          ZS_LOG_TRACE(log("returning size of latest version diff's document") + ", from=" + Stringize<ULONG>(fromVersionNumber).string() + ", current=" + Stringize<ULONG>(from).string() +  + getDebugValuesString())
          const DocumentPtr &doc = (*found).second;
          outOutputSizeInBytes += doc->getOutputSize();
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
        outOutputSizeInBytes = mDocument->getOutputSize();
      }

      //-----------------------------------------------------------------------
      String Publication::getDebugValuesString() const
      {
        AutoRecursiveLock lock(mLock);

        return debugNameValue("id", Stringize<PUID>(mID).string())
        + debugNameValue("source", toString(mSource))
        + debugNameValue("name", mName)
        + debugNameValue("version", (0 == mVersion ? String() : Stringize<ULONG>(mVersion).string()))
        + debugNameValue("base version", (0 == mBaseVersion ? String() : Stringize<ULONG>(mBaseVersion).string()))
        + debugNameValue("lineage", (0 == mLineage ? String() : Stringize<ULONG>(mLineage).string()))
        + debugNameValue("creator contact ID", getCreatorContactID())
        + debugNameValue("creator location ID", getCreatorLocationID())
        + debugNameValue("published to contact ID", getPublishedToContactID())
        + debugNameValue("published to location ID", getPublishedToLocationID())
        + debugNameValue("mime type", mMimeType)
        + debugNameValue("lifetime", toString(mLifetime))
        + debugNameValue("scope", toString(mScope))
        + debugNameValue("expires", (Time() == mExpires ? String() : Stringize<Time>(mExpires).string()))
        + debugNameValue("data length", (0 == mDataLengthInBytes ? String() : Stringize<size_t>(mDataLengthInBytes).string()))
        + debugNameValue("diffs total", (mXMLDiffDocuments.size() < 1 ? String() : Stringize<size_t>(mXMLDiffDocuments.size())))
        + debugNameValue("total relationships", (mPublishedRelationships.size() < 1 ? String() : Stringize<size_t>(mPublishedRelationships.size())));
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Publication => IPublicationForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      NodePtr Publication::getXMLDiffs(
                                       ULONG &ioFromVersion,
                                       ULONG toVersion
                                       ) const
      {
        AutoRecursiveLock lock(mLock);

        ZS_LOG_DEBUG(log("requested XML diffs") + ", from version=" +Stringize<ULONG>(ioFromVersion).string() + ", to version=" + Stringize<ULONG>(toVersion).string())

        if ((0 == toVersion) ||
            (toVersion >= mVersion)) {
          toVersion = mVersion;
        }

        if (!mDocument) {
          if (!mData) {
            ZS_LOG_WARNING(Detail, log("XML for publishing has no data available to return") + getDebugValuesString())
            return NodePtr();
          }

          ZS_LOG_WARNING(Detail, log("returning data as base 64 encoded") + getDebugValuesString())

          String data = IHelper::convertToBase64(mData.get(), mDataLengthInBytes);
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
        XMLDiffDocumentMap::const_iterator foundFrom = mXMLDiffDocuments.find(ioFromVersion);
        XMLDiffDocumentMap::const_iterator foundTo = mXMLDiffDocuments.find(toVersion);

        if ((foundFrom == mXMLDiffDocuments.end()) ||
            (foundTo == mXMLDiffDocuments.end())) {

          ZS_LOG_WARNING(Detail, log("unable to find XML differences for requested publication (thus returning whole document)"))
          ioFromVersion = 0;
          return getXMLDiffs(ioFromVersion, toVersion);
        }

        DocumentPtr cloned;

        ++foundTo;

        VersionNumber currentVersion = ioFromVersion;

        ElementPtr xdsOutputEl;

        // we have the diffs, process them into one document
        for (XMLDiffDocumentMap::const_iterator iter = foundFrom; iter != foundTo; ++iter, ++currentVersion) {
          const VersionNumber &versionNumber = (*iter).first;

          ZS_LOG_TRACE(log("processing diff") + ", version=" + Stringize<VersionNumber>(versionNumber).string())

          if (currentVersion != versionNumber) {
            ZS_LOG_ERROR(Detail, log("XML differences has a version number hole") + ", expecting=" + Stringize<VersionNumber>(currentVersion).string() + ", found=" + Stringize<VersionNumber>(versionNumber).string())
            ioFromVersion = 0;
            return getXMLDiffs(ioFromVersion, toVersion);
          }

          DocumentPtr doc = (*iter).second;

          try {
            if (!cloned) {
              cloned = doc->clone()->toDocument();
              xdsOutputEl = cloned->findFirstChildElementChecked("xds");
            } else {
              xdsOutputEl = cloned->findFirstChildElementChecked("xds");

              // we need to process all elements and insert them into the other...
              ElementPtr xdsEl = doc->findFirstChildElementChecked("xds");

              ElementPtr xdEl = xdsEl->findFirstChildElement("xd");
              while (xdEl) {
                xdsOutputEl->adoptAsLastChild(xdEl->clone());
                xdEl = xdEl->findNextSiblingElement("xd");
              }
            }
          } catch (zsLib::XML::Exceptions::CheckFailed &) {
            ZS_LOG_ERROR(Detail, log("XML diff document is corrupted (recovering by returning entire document)") + ", version=" + Stringize<VersionNumber>(versionNumber).string())
            ioFromVersion = 0;
            return getXMLDiffs(ioFromVersion, toVersion);
          }
        }

        ZS_THROW_INVALID_ASSUMPTION_IF(!cloned)
        ZS_THROW_INVALID_ASSUMPTION_IF(!xdsOutputEl)

        ZS_LOG_DEBUG(log("returning orphaned clone of XML differences document"))
        xdsOutputEl->orphan();
        return xdsOutputEl;
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
            ZS_LOG_BASIC(log("publication contains XML:") + "\n" + internal::toString(mDocument) + "\n")
          } else if (mData) {
            ZS_LOG_BASIC(log("publication contains binary data") + ", length=" + Stringize<size_t>(mDataLengthInBytes).string())
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

    IPublicationPtr IPublication::create(
                                         Sources source,
                                         const char *creatorContactID,
                                         const char *creatorLocationID,
                                         const char *name,
                                         const char *mimeType,
                                         const BYTE *data,
                                         size_t sizeInBytes,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         const char *peerContactID,
                                         const char *peerLocationID,
                                         Scopes scope,
                                         Lifetimes lifetime,
                                         Time expires
                                         )
    {
      return internal::Publication::create(source, creatorContactID, creatorLocationID, name, mimeType, data, sizeInBytes, publishToRelationships, peerContactID, peerLocationID, scope, lifetime, expires);
    }

    IPublicationPtr IPublication::create(
                                         Sources source,
                                         const char *creatorContactID,
                                         const char *creatorLocationID,
                                         const char *name,
                                         const char *mimeType,
                                         DocumentPtr documentToBeAdopted,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         const char *peerContactID,
                                         const char *peerLocationID,
                                         Scopes scope,
                                         Lifetimes lifetime,
                                         Time expires
                                         )
    {
      return internal::Publication::create(source, creatorContactID, creatorLocationID, name, mimeType, documentToBeAdopted, publishToRelationships, peerContactID, peerLocationID, scope, lifetime, expires);
    }

    IPublicationPtr IPublication::create(
                                         Sources source,
                                         const char *creatorContactID,
                                         const char *creatorLocationID,
                                         const char *name,
                                         const char *mimeType,
                                         const RelationshipList &relationshipsDocument,
                                         const PublishToRelationshipsMap &publishToRelationships,
                                         const char *peerContactID,
                                         const char *peerLocationID,
                                         Scopes scope,
                                         Lifetimes lifetime,
                                         Time expires
                                         )
    {
      return internal::Publication::create(source, creatorContactID, creatorLocationID, name, mimeType, relationshipsDocument, publishToRelationships, peerContactID, peerLocationID, scope, lifetime, expires);
    }

    IPublicationPtr IPublication::createForLocal(
                                                 const char *creatorContactID,
                                                 const char *creatorLocationID,
                                                 const char *name,
                                                 const char *mimeType,
                                                 const BYTE *data,
                                                 size_t sizeInBytes,
                                                 const PublishToRelationshipsMap &publishToRelationships,
                                                 Time expires
                                                 )
    {
      return IPublication::create(Source_Local, creatorContactID, creatorLocationID, name, mimeType, data, sizeInBytes, publishToRelationships, NULL, NULL, Scope_Location, Lifetime_Session, expires);
    }

    IPublicationPtr IPublication::createForFinder(
                                                  const char *creatorContactID,
                                                  const char *creatorLocationID,
                                                  const char *name,
                                                  const char *mimeType,
                                                  const BYTE *data,
                                                  size_t sizeInBytes,
                                                  const PublishToRelationshipsMap &publishToRelationships,
                                                  Scopes scope,
                                                  Lifetimes lifetime,
                                                  Time expires
                                                  )
    {
      return IPublication::create(Source_Finder, creatorContactID, creatorLocationID, name, mimeType, data, sizeInBytes, publishToRelationships, NULL, NULL, scope, lifetime, expires);
    }

    IPublicationPtr IPublication::createForPeer(
                                                const char *creatorContactID,
                                                const char *creatorLocationID,
                                                const char *name,
                                                const char *mimeType,
                                                const BYTE *data,
                                                size_t sizeInBytes,
                                                const char *peerContactID,
                                                const char *peerLocationID,
                                                const PublishToRelationshipsMap &publishToRelationships,
                                                Time expires
                                                )
    {
      return IPublication::create(Source_Peer, creatorContactID, creatorLocationID, name, mimeType, data, sizeInBytes, publishToRelationships, peerContactID, peerLocationID, Scope_Location, Lifetime_Session, expires);
    }

    IPublicationPtr IPublication::createForLocal(
                                                 const char *creatorContactID,
                                                 const char *creatorLocationID,
                                                 const char *name,
                                                 const char *mimeType,
                                                 DocumentPtr documentToBeAdopted,
                                                 const PublishToRelationshipsMap &publishToRelationships,
                                                 Time expires
                                                 )
    {
      return IPublication::create(Source_Local, creatorContactID, creatorLocationID, name, mimeType, documentToBeAdopted, publishToRelationships, NULL, NULL, Scope_Location, Lifetime_Session, expires);
    }

    IPublicationPtr IPublication::createForFinder(
                                                  const char *creatorContactID,
                                                  const char *creatorLocationID,
                                                  const char *name,
                                                  const char *mimeType,
                                                  DocumentPtr documentToBeAdopted,
                                                  const PublishToRelationshipsMap &publishToRelationships,
                                                  Scopes scope,
                                                  Lifetimes lifetime,
                                                  Time expires
                                                  )
    {
      return IPublication::create(Source_Finder, creatorContactID, creatorLocationID, name, mimeType, documentToBeAdopted, publishToRelationships, NULL, NULL, scope, lifetime, expires);
    }

    IPublicationPtr IPublication::createForPeer(
                                                const char *creatorContactID,
                                                const char *creatorLocationID,
                                                const char *name,
                                                const char *mimeType,
                                                DocumentPtr documentToBeAdopted,
                                                const PublishToRelationshipsMap &publishToRelationships,
                                                const char *peerContactID,
                                                const char *peerLocationID,
                                                Time expires
                                                )
    {
      return IPublication::create(Source_Peer, creatorContactID, creatorLocationID, name, mimeType, documentToBeAdopted, publishToRelationships, peerContactID, peerLocationID, Scope_Location, Lifetime_Session, expires);
    }

    IPublicationPtr IPublication::createForLocal(
                                                 const char *creatorContactID,
                                                 const char *creatorLocationID,
                                                 const char *name,
                                                 const char *mimeType,
                                                 const RelationshipList &relationshipsDocument,
                                                 const PublishToRelationshipsMap &publishToRelationships,
                                                 Time expires
                                                 )
    {
      return IPublication::create(Source_Local, creatorContactID, creatorLocationID, name, mimeType, relationshipsDocument, publishToRelationships, NULL, NULL, Scope_Location, Lifetime_Session, expires);
    }

    IPublicationPtr IPublication::createForFinder(
                                                  const char *creatorContactID,
                                                  const char *creatorLocationID,
                                                  const char *name,
                                                  const char *mimeType,
                                                  const RelationshipList &relationshipsDocument,
                                                  const PublishToRelationshipsMap &publishToRelationships,
                                                  Scopes scope,
                                                  Lifetimes lifetime,
                                                  Time expires
                                                  )
    {
      return IPublication::create(Source_Finder, creatorContactID, creatorLocationID, name, mimeType, relationshipsDocument, publishToRelationships, NULL, NULL, scope, lifetime, expires);
    }

    IPublicationPtr IPublication::createForPeer(
                                                const char *creatorContactID,
                                                const char *creatorLocationID,
                                                const char *name,
                                                const char *mimeType,
                                                const RelationshipList &relationshipsDocument,
                                                const PublishToRelationshipsMap &publishToRelationships,
                                                const char *peerContactID,
                                                const char *peerLocationID,
                                                Time expires
                                                )
    {
      return IPublication::create(Source_Peer, creatorContactID, creatorLocationID, name, mimeType, relationshipsDocument, publishToRelationships, peerContactID, peerLocationID, Scope_Location, Lifetime_Session, expires);
    }
  }
}
