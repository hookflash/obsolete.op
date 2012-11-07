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

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/PeerPublishRequest.h>
#include <hookflash/stack/message/PeerGetResult.h>
#include <hookflash/stack/message/MessageFactoryStack.h>

#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IPublication.h>
#include <hookflash/stack/IPublicationRepository.h>
#include <hookflash/stack/internal/stack_Publication.h>
#include <hookflash/stack/internal/stack_PublicationMetaData.h>

#include <hookflash/services/IICESocket.h>

#include <zsLib/Stringize.h>
#include <zsLib/Numeric.h>
#include <zsLib/zsHelpers.h>


namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      using zsLib::Stringize;
      using zsLib::Numeric;

      typedef zsLib::WORD WORD;
      typedef zsLib::UINT UINT;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef zsLib::XML::Exceptions Exceptions;
      typedef zsLib::XML::Document Document;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::Attribute Attribute;
      typedef zsLib::XML::AttributePtr AttributePtr;
      typedef zsLib::XML::Element Element;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::Text Text;
      typedef zsLib::XML::TextPtr TextPtr;
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;
      typedef services::IICESocket::CandidateList CandidateList;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageHelper
      #pragma mark

      //-----------------------------------------------------------------------
      DocumentPtr IMessageHelper::createDocumentWithRoot(const Message &message)
      {
        const char *tagName = Message::toString(message.messageType());

        IMessageFactoryPtr factory = message.factory();

        if (!factory) return DocumentPtr();

        DocumentPtr ret = Document::create();
        ret->setElementNameIsCaseSensative(true);
        ret->setAttributeNameIsCaseSensative(true);

        ElementPtr root = Element::create();
        root->setValue(tagName);
        ret->adoptAsFirstChild(root);

        IMessageHelper::setAttribute(root, "xmlns", factory->xmlns());
        IMessageHelper::setAttributeID(root, message.messageID());
        IMessageHelper::setAttribute(root, "method", factory->toString(message.method()));

        if (Message::MessageType_Result == message.messageType()) {
          const message::MessageResult *msgResult = (dynamic_cast<const message::MessageResult *>(&message));
          if (msgResult->hasAttribute(MessageResult::AttributeType_Time)) {
            IMessageHelper::setAttributeEpoch(root, msgResult->time());
          }
        }

        return ret;
      }

      //-----------------------------------------------------------------------
      Message::MessageTypes IMessageHelper::getMessageType(ElementPtr root)
      {
        if (!root) return Message::MessageType_Invalid;
        return Message::toMessageType(root->getValue());
      }

      //---------------------------------------------------------------------
      String IMessageHelper::getAttributeID(ElementPtr node)
      {
        return IMessageHelper::getAttribute(node, "id");
      }

      //---------------------------------------------------------------------
      void IMessageHelper::setAttributeID(ElementPtr elem, const String &value)
      {
        if (!value.isEmpty())
          IMessageHelper::setAttribute(elem, "id", value);
      }

      //---------------------------------------------------------------------
      String IMessageHelper::timeToString(const Time &value)
      {
        if (Time() == value) return String();

        time_t epoch = zsLib::toEpoch(value);
        return Stringize<time_t>(epoch).string();
      }

      //---------------------------------------------------------------------
      Time IMessageHelper::stringToTime(const String &s)
      {
        if (s.isEmpty()) return Time();

        try {
          time_t epoch = Numeric<time_t>(s);
          return zsLib::toTime(epoch);
        } catch (Numeric<time_t>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Detail, "unable to convert value to time_t, value=" + s)
        }
        return Time();
      }

      //---------------------------------------------------------------------
      Time IMessageHelper::getAttributeEpoch(ElementPtr node)
      {
        return IMessageHelper::stringToTime(IMessageHelper::getAttribute(node, "epoch"));
      }

      //---------------------------------------------------------------------
      void IMessageHelper::setAttributeEpoch(ElementPtr elem, const Time &value)
      {
        if (Time() == value) return;
        setAttribute(elem, "epoch", timeToString(value));
      }

      //-----------------------------------------------------------------------
      String IMessageHelper::getAttribute(
                                          ElementPtr node,
                                          const String &attributeName
                                          )
      {
        if (!node) return String();

        AttributePtr attribute = node->findAttribute(attributeName);
        if (!attribute) return String();

        return attribute->getValue();
      }

      //-----------------------------------------------------------------------
      void IMessageHelper::setAttribute(
                                        ElementPtr elem,
                                        const String &attrName,
                                        const String &value
                                        )
      {
        if (value.isEmpty()) return;

        AttributePtr attr = Attribute::create();
        attr->setName(attrName);
        attr->setValue(value);

        elem->setAttribute(attr);
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElement(const String &elName)
      {
        ElementPtr tmp = Element::create();
        tmp->setValue(elName);
        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithText(
                                                       const String &elName,
                                                       const String &textVal
                                                       )
      {
        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(textVal);

        ElementPtr tmp = Element::create(elName);
        tmp->adoptAsFirstChild(tmpTxt);

        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithTextAndEntityEncode(
                                                                      const String &elName,
                                                                      const String &textVal
                                                                      )
      {
        TextPtr tmpTxt = Text::create();
        tmpTxt->setValueAndEntityEncode(textVal);

        ElementPtr tmp = Element::create(elName);
        tmp->adoptAsFirstChild(tmpTxt);

        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithID(
                                                     const String &elName,
                                                     const String &idValue
                                                     )
      {
        ElementPtr tmp = createElement(elName);
        setAttributeID(tmp, idValue);
        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithAttribute(
                                                            const String &elName,
                                                            const String &attrName,
                                                            const String &attrValue
                                                            )
      {
        ElementPtr tmp = createElement(elName);
        setAttribute(tmp, attrName, attrValue);
        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithIDAndAttribute(
                                                                 const String &elName,
                                                                 const String &idValue,
                                                                 const String &attrName,
                                                                 const String &attrValue
                                                                 )
      {
        ElementPtr tmp = createElementWithID(elName, idValue);
        setAttribute(tmp, attrName, attrValue);
        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithIDAndText(
                                                            const String &elName,
                                                            const String &idValue,
                                                            const String &txtValue
                                                            )
      {
        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(txtValue);

        ElementPtr tmp = createElementWithID(elName, idValue);
        tmp->adoptAsLastChild(tmpTxt);

        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithAttributeAndText(
                                                                   const String &elName,
                                                                   const String &attrName,
                                                                   const String &attrValue,
                                                                   const String &txtValue
                                                                   )
      {
        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(txtValue);

        ElementPtr tmp = createElementWithAttribute(elName, attrName, attrValue);
        tmp->adoptAsLastChild(tmpTxt);

        return tmp;
      }

      //-----------------------------------------------------------------------
      TextPtr IMessageHelper::createText(const String &textVal)
      {
        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(textVal);

        return tmpTxt;
      }

      //-----------------------------------------------------------------------
      String IMessageHelper::getElementText(ElementPtr node)
      {
        if (!node) return String();
        return node->getText();
      }

      //-----------------------------------------------------------------------
      String IMessageHelper::getElementTextAndEntityDecode(ElementPtr node)
      {
        if (!node) return String();
        return node->getTextAndEntityDecode();
      }

      //-----------------------------------------------------------------------
      String IMessageHelper::getChildElementText(
                                                 ElementPtr node,
                                                 const String &childElementName
                                                 )
      {
        if (!node) return String();
        ElementPtr childEl = node->findFirstChildElement(childElementName);
        return getElementText(childEl);
      }

      //-----------------------------------------------------------------------
      void IMessageHelper::setElementText(ElementPtr node, const String &textVal)
      {
        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(textVal);
        node->adoptAsFirstChild(tmpTxt);
      }

      namespace internal
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageHelper
        #pragma mark

        //---------------------------------------------------------------------
        ElementPtr MessageHelper::createElement(const Candidate &candidate)
        {
          typedef zsLib::DWORD DWORD;

          ElementPtr candidateEl = IMessageHelper::createElement("candidate");

          candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("transport", "rudp/udp"));

          ElementPtr ipEl = IMessageHelper::createElementWithText("ip", candidate.mIPAddress.string(false));
          IMessageHelper::setAttribute(ipEl, "format", "ipv4");
          candidateEl->adoptAsLastChild(ipEl);
          candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("port", Stringize<WORD>(candidate.mIPAddress.getPort())));

          if (!candidate.mUsernameFrag.isEmpty())
          {
            candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("usernameFrag", candidate.mUsernameFrag));
          }

          if (!candidate.mPassword.isEmpty())
          {
            candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("password", candidate.mPassword));
          }

          if (candidate.mPriority > 0)
          {
            candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("priority", Stringize<DWORD>(candidate.mPriority)));
          }

          if (!candidate.mProtocol.isEmpty())
          {
            candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("protocol", candidate.mProtocol));
          }

          return candidateEl;
        }

        //---------------------------------------------------------------------
        ElementPtr MessageHelper::createElement(const Location &location)
        {
          ElementPtr locationEl = IMessageHelper::createElementWithID("location", location.mID);
          ElementPtr detailEl = IMessageHelper::createElement("details");
          ElementPtr challengeEl = IMessageHelper::createElement("challenge");

          if (!location.mLocationSalt.isEmpty())
            challengeEl->adoptAsLastChild(IMessageHelper::createElementWithText("locationSalt", location.mLocationSalt));

          if (!location.mLocationFindSecretProof.isEmpty())
            challengeEl->adoptAsLastChild(IMessageHelper::createElementWithAttributeAndText("locationFindSecretProof", "cipher", "sha256/aes256", location.mLocationFindSecretProof));

          if (location.mIPAddress.string(false) != "0.0.0.0")
          {
            ElementPtr ipEl = IMessageHelper::createElementWithText("ip", location.mIPAddress.string(false));
            IMessageHelper::setAttribute(ipEl, "format", "ipv4");
            detailEl->adoptAsLastChild(ipEl);
          }

          if (!location.mDeviceID.isEmpty()) {
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithID("device", location.mDeviceID));
          }

          if (!location.mUserAgent.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndEntityEncode("userAgent", location.mUserAgent));

          if (!location.mOS.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndEntityEncode("os", location.mOS));

          if (!location.mSystem.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndEntityEncode("system", location.mSystem));

          if (!location.mHost.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndEntityEncode("host", location.mHost));

          if (challengeEl->hasChildren())
            locationEl->adoptAsLastChild(challengeEl);

          if (detailEl->hasChildren())
            locationEl->adoptAsLastChild(detailEl);

          if (!location.mContactID.isEmpty())
            locationEl->adoptAsLastChild(IMessageHelper::createElementWithID("contact", location.mContactID));

          if (location.mCandidates.size() > 0)
          {
            ElementPtr candidates = IMessageHelper::createElement("candidates");
            locationEl->adoptAsLastChild(candidates);

            CandidateList::const_iterator it;
            for(it=location.mCandidates.begin(); it!=location.mCandidates.end(); ++it)
            {
              Candidate candidate(*it);
              candidates->adoptAsLastChild(MessageHelper::createElement(candidate));
            }

            locationEl->adoptAsLastChild(candidates);
          }

          if (location.mReasonID > 0 || !location.mReason.isEmpty())
          {
            locationEl->adoptAsLastChild(IMessageHelper::createElementWithIDAndText("reason", Stringize<WORD>(location.mReasonID), location.mReason));
          }

          return locationEl;
        }

        //---------------------------------------------------------------------
        ElementPtr MessageHelper::createElement(
                                                const PublishToRelationshipsMap &relationships,
                                                const char *elementName
                                                )
        {
          ElementPtr rootEl = IMessageHelper::createElement(elementName);

          for (stack::IPublicationMetaData::PublishToRelationshipsMap::const_iterator iter = relationships.begin(); iter != relationships.end(); ++iter)
          {
            String name = (*iter).first;
            const stack::IPublicationMetaData::PermissionAndContactIDListPair &permission = (*iter).second;

            const char *permissionStr = "all";
            switch (permission.first) {
              case stack::IPublicationMetaData::Permission_All:     permissionStr = "all"; break;
              case stack::IPublicationMetaData::Permission_None:    permissionStr = "none"; break;
              case stack::IPublicationMetaData::Permission_Some:    permissionStr = "some"; break;
              case stack::IPublicationMetaData::Permission_Add:     permissionStr = "add"; break;
              case stack::IPublicationMetaData::Permission_Remove:  permissionStr = "remove"; break;
            }

            ElementPtr relationshipsEl = IMessageHelper::createElement("relationships");
            relationshipsEl->setAttribute("name", name);
            relationshipsEl->setAttribute("allow", permissionStr);

            for (stack::IPublicationMetaData::ContactIDList::const_iterator contactIter = permission.second.begin(); contactIter != permission.second.end(); ++contactIter)
            {
              ElementPtr contactEl = IMessageHelper::createElementWithID("contact", (*contactIter));
              relationshipsEl->adoptAsLastChild(contactEl);
            }

            rootEl->adoptAsLastChild(relationshipsEl);
          }

          return rootEl;
        }

        //---------------------------------------------------------------------
        DocumentPtr MessageHelper::createDocument(
                                                  Message &msg,
                                                  IPublicationMetaDataPtr publicationMetaData,
                                                  ULONG *notifyPeerPublishMaxDocumentSizeInBytes,
                                                  IPublicationRepositoryPeerCachePtr peerCache
                                                  )
        {
          typedef zsLib::ULONG ULONG;
          typedef stack::internal::IPublicationForMessages IPublicationForMessages;
          typedef stack::internal::IPublicationForMessagesPtr IPublicationForMessagesPtr;

          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          IPublicationForMessagesPtr publication(IPublicationForMessages::convert(publicationMetaData->getPublication()));

          ULONG fromVersion = 0;
          ULONG toVersion = 0;

          if (publication) {
            fromVersion = publication->getBaseVersion();
            toVersion = publication->getVersion();
          } else {
            fromVersion = publicationMetaData->getBaseVersion();
            toVersion = publicationMetaData->getVersion();
          }

          // do not use the publication for these message types...
          switch (msg.method()) {
            case MessageFactoryStack::Method_PeerPublish:
            {
              if (Message::MessageType_Request == msg.messageType()) {
                message::PeerPublishRequest &request = *(dynamic_cast<message::PeerPublishRequest *>(&msg));
                fromVersion = request.publishedFromVersion();
                toVersion = request.publishedToVersion();
              }
              if (Message::MessageType_Request != msg.messageType()) {
                // only the request can include a publication...
                publication.reset();
              }
              break;
            }
            case MessageFactoryStack::Method_PeerGet:
            {
              if (Message::MessageType_Result == msg.messageType()) {
                message::PeerGetResult &result = *(dynamic_cast<message::PeerGetResult *>(&msg));
                IPublicationMetaDataPtr originalMetaData = result.originalRequestPublicationMetaData();
                fromVersion = originalMetaData->getVersion();
                if (0 != fromVersion) {
                  // remote party already has this version so just return from that version onward...
                  ++fromVersion;
                }
                toVersion = 0;
              }

              if (Message::MessageType_Result != msg.messageType()) {
                // only the result can include a publication
                publication.reset();
              }
              break;
            }
            case MessageFactoryStack::Method_PeerDelete:
            case MessageFactoryStack::Method_PeerSubscribe:
            {
              // these messages requests/results will never include a publication (at least at this time)
              publication.reset();
              break;
            }
            case MessageFactoryStack::Method_PeerPublishNotify:
            {
              if (publication) {
                if (peerCache) {
                  ULONG bogusFillSize = 0;
                  ULONG &maxFillSize = (notifyPeerPublishMaxDocumentSizeInBytes ? *notifyPeerPublishMaxDocumentSizeInBytes : bogusFillSize);
                  if (peerCache->getNextVersionToNotifyAboutAndMarkNotified(publication->convertIPublication(), maxFillSize, fromVersion, toVersion)) {
                    break;
                  }
                }
              }
              publication.reset();
              fromVersion = 0;
              break;
            }
            default:                                  break;
          }

          stack::IPublicationMetaData::PublishToRelationshipsMap relationships;
          publicationMetaData->getRelationships(relationships);

          ElementPtr docEl = IMessageHelper::createElement("document");
          ElementPtr detailsEl = IMessageHelper::createElement("details");
          ElementPtr publishToRelationshipsEl = MessageHelper::createElement(relationships, MessageFactoryStack::Method_PeerSubscribe == (MessageFactoryStack::Methods)msg.method() ? "subscribeToRelationships" : "publishToRelationships");
          ElementPtr dataEl = IMessageHelper::createElement("data");

          ElementPtr contactEl = IMessageHelper::createElement("contact");
          ElementPtr locationEl = IMessageHelper::createElement("location");

          String creatorContactID = publicationMetaData->getCreatorContactID();
          String creatorLocationIDID = publicationMetaData->getCreatorLocationID();

          IMessageHelper::setAttributeID(contactEl, creatorContactID);
          IMessageHelper::setAttributeID(locationEl, creatorLocationIDID);

          NodePtr publishedDocEl;
          if (publication) {
            publishedDocEl = publication->getXMLDiffs(
                                                      fromVersion,
                                                      toVersion
                                                      );

            if (0 == toVersion) {
              // put the version back to something more sensible
              toVersion = publicationMetaData->getVersion();
            }
          }

          ElementPtr nameEl = IMessageHelper::createElementWithText("name", publicationMetaData->getName());
          ElementPtr versionEl = IMessageHelper::createElementWithText("version", Stringize<ULONG>(toVersion));
          ElementPtr baseVersionEl = IMessageHelper::createElementWithText("baseVersion", Stringize<ULONG>(fromVersion));
          ElementPtr lineageEl = IMessageHelper::createElementWithText("lineage", Stringize<ULONG>(publicationMetaData->getLineage()));
          ElementPtr chunkEl = IMessageHelper::createElementWithText("chunk", "1/1");

          const char *scopeStr = "location";
          switch (publicationMetaData->getScope())
          {
            case stack::IPublicationMetaData::Scope_Location: scopeStr = "location"; break;
            case stack::IPublicationMetaData::Scope_Contact:  scopeStr = "contact"; break;
          }

          ElementPtr scopeEl = IMessageHelper::createElementWithText("scope", scopeStr);

          const char *lifetimeStr = "session";
          switch (publicationMetaData->getLifetime())
          {
            case stack::IPublicationMetaData::Lifetime_Session:   lifetimeStr = "session"; break;
            case stack::IPublicationMetaData::Lifetime_Permanent: lifetimeStr = "permanent"; break;
          }

          ElementPtr lifetimeEl = IMessageHelper::createElementWithText("lifetime", lifetimeStr);

          ElementPtr expiresEl;
          if (publicationMetaData->getExpires() != Time()) {

            String expires = boost::posix_time::to_simple_string(publicationMetaData->getExpires());
            expiresEl = IMessageHelper::createElementWithText("expires", expires);
          }

          ElementPtr mimeTypeEl = IMessageHelper::createElementWithText("mime", publicationMetaData->getMimeType());

          stack::IPublication::Encodings encoding = publicationMetaData->getEncoding();
          const char *encodingStr = "binary";
          switch (encoding) {
            case stack::IPublication::Encoding_Binary:  encodingStr = "binary"; break;
            case stack::IPublication::Encoding_XML:     encodingStr = "xml"; break;
          }

          ElementPtr encodingEl = IMessageHelper::createElementWithText("encoding", encodingStr);

          root->adoptAsLastChild(docEl);
          docEl->adoptAsLastChild(detailsEl);
          docEl->adoptAsLastChild(publishToRelationshipsEl);

          detailsEl->adoptAsLastChild(nameEl);
          detailsEl->adoptAsLastChild(versionEl);
          if (0 != fromVersion)
            detailsEl->adoptAsLastChild(baseVersionEl);
          detailsEl->adoptAsLastChild(lineageEl);
          detailsEl->adoptAsLastChild(chunkEl);
          detailsEl->adoptAsLastChild(scopeEl);
          detailsEl->adoptAsLastChild(contactEl);
          detailsEl->adoptAsLastChild(locationEl);
          detailsEl->adoptAsLastChild(lifetimeEl);
          if (expiresEl)
            detailsEl->adoptAsLastChild(expiresEl);
          detailsEl->adoptAsLastChild(mimeTypeEl);
          detailsEl->adoptAsLastChild(encodingEl);

          if (publishedDocEl) {
            dataEl->adoptAsLastChild(publishedDocEl);
            docEl->adoptAsLastChild(dataEl);
          }

          switch (msg.messageType()) {
            case Message::MessageType_Request:
            case Message::MessageType_Notify:   {
              switch (msg.method()) {

                case MessageFactoryStack::Method_PeerPublish:         break;

                case MessageFactoryStack::Method_PeerGet:             {
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  lifetimeEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  publishToRelationshipsEl->orphan();
                  break;
                }
                case MessageFactoryStack::Method_PeerDelete:          {
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  chunkEl->orphan();
                  contactEl->orphan();
                  locationEl->orphan();
                  lifetimeEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  publishToRelationshipsEl->orphan();
                }
                case MessageFactoryStack::Method_PeerSubscribe:       {
                  versionEl->orphan();
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  lineageEl->orphan();
                  chunkEl->orphan();
                  scopeEl->orphan();
                  contactEl->orphan();
                  locationEl->orphan();
                  lifetimeEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  break;
                }
                case MessageFactoryStack::Method_PeerPublishNotify:
                {
                  if (!publication) {
                    if (baseVersionEl)
                      baseVersionEl->orphan();
                  }
                  chunkEl->orphan();
                  publishToRelationshipsEl->orphan();
                  break;
                }
                default:                                  break;
              }
              break;
            }
            case Message::MessageType_Result:
            case Message::MessageType_Reply:    {
              switch (msg.method()) {
                case MessageFactoryStack::Method_PeerPublish:       {
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  locationEl->orphan();
                  contactEl->orphan();
                  dataEl->orphan();
                  break;
                }
                case MessageFactoryStack::Method_PeerGet:           {
                  break;
                }
                case MessageFactoryStack::Method_PeerDelete:        {
                  ZS_THROW_INVALID_USAGE("this method should not be used for delete result")
                  break;
                }
                case MessageFactoryStack::Method_PeerSubscribe:     {
                  versionEl->orphan();
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  lineageEl->orphan();
                  chunkEl->orphan();
                  scopeEl->orphan();
                  contactEl->orphan();
                  locationEl->orphan();
                  lifetimeEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  break;
                }
                case MessageFactoryStack::Method_PeerPublishNotify: {
                  ZS_THROW_INVALID_USAGE("this method should not be used for publication notify result")
                  break;
                }
                default:                                break;
              }
              break;
            }
            case Message::MessageType_Invalid:  break;
          }

          return ret;
        }


/*
<request xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-publish”>

 <document>
  <details>
   <name>/hookflash.com/presence/1.0/bd520f1dbaa13c0cc9b7ff528e83470e/883fa7...9533609131</name>
   <version>12</version>
   <!-- <baseVersion>10</baseVersion> -->
   <lineage>5849943</lineage>
   <chunk>1/12</chunk>
   <scope>location</scope>
   <lifetime>session</lifetime>
   <expires>2002-01-20 23:59:59.000</expires>
   <mime>text/xml</mime>
   <encoding>xml</encoding>
  </details>
  <publishToRelationships>
   <relationships name=”/hookflash.com/authorization-list/1.0/whitelist” allow=”all” />
   <relationships name=”/hookflash.com/authorization-list/1.0/adhoc” allow=”all” />
   <relationships name=”/hookflash.com/shared-groups/1.0/foobar” allow=”all” />
  </publishToRelationships>
  <data>
   ...
  </data>
 </document>

</request>
*/

/*
<result xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-publish” epoch=”13494934”>

 <document>
  <details>
   <name>/hookflash.com/presence/1.0/bd520f1dbaa13c0cc9b7ff528e83470e/883fa7...9533609131</name>
   <version>12</version>
   <lineage>5849943</lineage>
   <chunk>1/12</chunk>
   <scope>location</scope>
   <lifetime>session</lifetime>
   <expires>2002-01-20 23:59:59.000</expires>
   <mime>text/xml</mime>
   <encoding>xml</encoding>
  </details>
  <publishToRelationships>
   <relationships name=”/hookflash.com/authorization-list/1.0/whitelist” allow=”all” />
   <relationships name=”/hookflash.com/authorization-list/1.0/adhoc” allow=”all” />
   <relationships name=”/hookflash.com/shared-groups/1.0/foobar” allow=”all” />
  </publishToRelationships>
 </document>

</result>
*/

        /*
<request xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-get”>

 <document>
  <details>
   <name>/hookflash.com/presence/1.0/bd520f1dbaa13c0cc9b7ff528e83470e/883fa7...9533609131</name>
   <version>12</version>
   <lineage>39239392</lineage>
   <scope>location</scope>
   <contact id=”ea00ede4405c99be9ae45739ebfe57d5” />
   <location id=”524e609f337663bdbf54f7ef47d23ca9” />
   <chunk>1/1</chunk>
  </details>
 </document>

</request>
         */

/*
<result xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-get” epoch=”13494934”>

 <document>
  <details>
   <name>/hookflash.com/presence/1.0/bd520f1dbaa13c0cc9b7ff528e83470e/883fa7...9533609131</name>
   <version>12</version>
   <!-- <baseVersion>10</baseVersion> -->
   <lineage>39239392</lineage>
   <chunk>1/10</chunk>
   <scope>location</scope>
   <contact id=”ea00ede4405c99be9ae45739ebfe57d5” />
   <location id=”524e609f337663bdbf54f7ef47d23ca9” />
   <lifetime>session</lifetime>
   <expires>2002-01-20 23:59:59.000</expires>
   <mime>text/xml</mime>
   <encoding>xml</encoding>
  </details>
  <publishToRelationships>
   <relationships name=”/hookflash.com/authorization-list/1.0/whitelist” allow=”all” />
   <relationships name=”/hookflash.com/authorization-list/1.0/adhoc” allow=”all” />
   <relationships name=”/hookflash.com/shared-groups/1.0/foobar” allow=”all” />
  </publishToRelationships>
  <data>
   ...
  </data>
 </document>

</result>
 */

        /*
<request xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-delete”>

 <document>
  <details>
   <name>/hookflash.com/presence/1.0/bd520f1dbaa13c0cc9b7ff528e83470e/883fa7...9533609131</name>
   <version>12</version>
   <lineage>39239392</lineage>
   <scope>location</scope>
  </details>
 </document>

</request>
         */

        /*
<request xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-subscribe”>

 <document>
  <name>/hookflash.com/presence/1.0/</name>
  <subscribeToRelationships>
   <relationships name=”/hookflash.com/authorization-list/1.0/whitelist” subscribe=”all” />
   <relationships name=”/hookflash.com/authorization-list/1.0/adhoc” subscribe =”add”>
    <contact id=”bd520f1dbaa13c0cc9b7ff528e83470e” />
   </relationships>
   <relationships name=”/hookflash.com/shared-groups/1.0/foobar” subscribe =”all” />
  </subscribeToRelationships>
 </document>

</request>
         */

        /*
<result xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-subscribe” epoch=”13494934”>

 <document>
   <name>/hookflash.com/presence/1.0/</name>
   <subscribeToRelationships>
    <relationships name=”/hookflash.com/authorization-list/1.0/whitelist” subscribe =”all” />
    <relationships name=”/hookflash.com/authorization-list/1.0/adhoc” subscribe =”some”>
     <contact id=”bd520f1dbaa13c0cc9b7ff528e83470e” />
     <contact id=”8d17a88e8d42ffbd138f3895ec45375c” />
    </relationships>
    <relationships name=”/hookflash.com/shared-groups/1.0/foobar” subscribe =”all” />
   </subscribeToRelationships>
 </document>

</result>
         */
        /*
<request xmlns="http://www.hookflash.com/openpeer/1.0/message" id=”abc123” method=”peer-publish-notify”>

 <documents>
  <document>
   <details>
    <name>/hookflash.com/presence/1.0/bd520f1dbaa13c0cc9b7ff528e83470e/883fa7...9533609131</name>
    <version>12</version>
    <lineage>43493943</lineage>
    <scope>location</scope>
    <contact id=”ea00ede4405c99be9ae45739ebfe57d5” />
    <location id=”524e609f337663bdbf54f7ef47d23ca9” />
    <lifetime>session</lifetime>
    <expires>49494393</expires>
    <mime>text/xml</mime>
    <encoding>xml</encoding>
   </details>
  </document>
  <!-- <data> ... </data> -->
 </documents>

</request>
         */

        //---------------------------------------------------------------------
        void MessageHelper::fillFrom(
                                     MessagePtr msg,
                                     ElementPtr rootEl,
                                     IPublicationPtr &outPublication,
                                     IPublicationMetaDataPtr &outPublicationMetaData
                                     )
        {
          typedef zsLib::ULONG ULONG;
          typedef stack::internal::IPublicationForMessages IPublicationForMessages;
          typedef stack::internal::IPublicationForMessagesPtr IPublicationForMessagesPtr;
          typedef stack::internal::IPublicationMetaDataForMessages IPublicationMetaDataForMessages;
          typedef stack::internal::IPublicationMetaDataForMessagesPtr IPublicationMetaDataForMessagesPtr;

          try {
            ElementPtr docEl = rootEl->findFirstChildElementChecked("document");
            ElementPtr detailsEl = docEl->findFirstChildElementChecked("details");

            ElementPtr nameEl = detailsEl->findFirstChildElementChecked("name");
            ElementPtr versionEl = detailsEl->findFirstChildElement("version");
            ElementPtr baseVersionEl = detailsEl->findFirstChildElement("baseVersion");
            ElementPtr lineageEl = detailsEl->findFirstChildElement("lineage");
            ElementPtr scopeEl = detailsEl->findFirstChildElement("scope");
            ElementPtr lifetimeEl = detailsEl->findFirstChildElement("lifetime");
            ElementPtr expiresEl = detailsEl->findFirstChildElement("expires");
            ElementPtr mimeTypeEl = detailsEl->findFirstChildElement("mime");
            ElementPtr encodingEl = detailsEl->findFirstChildElement("encoding");
            String creatorContactID;
            ElementPtr creatorContactEl = detailsEl->findFirstChildElement("contact");
            if (creatorContactEl) {
              creatorContactID = creatorContactEl->getAttributeValue("id");
            }
            String creatorLocationID;
            ElementPtr creatorLocationEl = detailsEl->findFirstChildElement("location");
            if (creatorLocationEl) {
              creatorLocationID = creatorLocationEl->getAttributeValue("id");
            }

            ElementPtr dataEl = docEl->findFirstChildElement("data");

            ULONG version = 0;
            if (versionEl) {
              String versionStr = versionEl->getText();
              try {
                version = Numeric<ULONG>(versionStr);
              } catch(Numeric<ULONG>::ValueOutOfRange &) {
              }
            }

            ULONG baseVersion = 0;
            if (baseVersionEl) {
              String baseVersionStr = baseVersionEl->getText();
              try {
                baseVersion = Numeric<ULONG>(baseVersionStr);
              } catch(Numeric<ULONG>::ValueOutOfRange &) {
              }
            }

            ULONG lineage = 0;
            if (lineageEl) {
              String lineageStr = lineageEl->getText();
              try {
                lineage = Numeric<ULONG>(lineageStr);
              } catch(Numeric<ULONG>::ValueOutOfRange &) {
              }
            }

            IPublicationMetaData::Scopes scope = IPublicationMetaData::Scope_Location;
            if (scopeEl) {
              if (scopeEl->getText() == "contact") {
                scope = IPublicationMetaData::Scope_Contact;
              }
            }

            IPublicationMetaData::Encodings encoding = IPublicationMetaData::Encoding_Binary;
            if (encodingEl) {
              String encodingStr = encodingEl->getText();
              if (encodingStr == "xml") encoding = IPublicationMetaData::Encoding_XML;
              else if (encodingStr == "binary") encoding = IPublicationMetaData::Encoding_Binary;
            }

            IPublicationMetaData::Lifetimes lifetime = IPublicationMetaData::Lifetime_Session;
            if (lifetimeEl) {
              String lifetimeStr = lifetimeEl->getText();
              if (lifetimeStr == "session") lifetime = IPublicationMetaData::Lifetime_Session;
              else if (lifetimeStr == "permanent") lifetime = IPublicationMetaData::Lifetime_Permanent;
            }

            Time expires;
            if (expiresEl) {
              String expiresStr = expiresEl->getText();
              expires = Numeric<Time>(expiresStr);
            }

            String mimeType;
            if (mimeTypeEl) {
              mimeType = mimeTypeEl->getText();
            }

            IPublicationMetaData::PublishToRelationshipsMap relationships;

            ElementPtr publishToRelationshipsEl = docEl->findFirstChildElement(MessageFactoryStack::Method_PeerSubscribe == (MessageFactoryStack::Methods)msg->method() ? "subscribeToRelationships" : "publishToRelationships");
            if (publishToRelationshipsEl) {
              ElementPtr relationshipsEl = publishToRelationshipsEl->findFirstChildElement("relationships");
              while (relationshipsEl)
              {
                String name = relationshipsEl->getAttributeValue("name");
                String allowStr = relationshipsEl->getAttributeValue("allow");

                IPublicationMetaData::ContactIDList contacts;
                ElementPtr contactEl = relationshipsEl->findFirstChildElement("contact");
                while (contactEl)
                {
                  String contactID = contactEl->getAttributeValue("id");
                  if (contactID.size() > 0) {
                    contacts.push_back(contactID);
                  }
                  contactEl = contactEl->findNextSiblingElement("contact");
                }

                IPublicationMetaData::Permissions permission = IPublicationMetaData::Permission_All;
                if (allowStr == "all") permission = IPublicationMetaData::Permission_All;
                else if (allowStr == "none") permission = IPublicationMetaData::Permission_None;
                else if (allowStr == "some") permission = IPublicationMetaData::Permission_Some;
                else if (allowStr == "add") permission = IPublicationMetaData::Permission_Add;
                else if (allowStr == "remove") permission = IPublicationMetaData::Permission_Remove;

                if (name.size() > 0) {
                  relationships[name] = IPublicationMetaData::PermissionAndContactIDListPair(permission, contacts);
                }

                relationshipsEl = relationshipsEl->findNextSiblingElement("relationships");
              }
            }

            bool hasPublication = false;

            switch (msg->messageType()) {
              case Message::MessageType_Request:
              case Message::MessageType_Notify:   {
                switch (msg->method()) {
                  case MessageFactoryStack::Method_PeerPublish:
                  case MessageFactoryStack::Method_PeerPublishNotify: {
                    hasPublication = true;
                    break;
                  }
                  case MessageFactoryStack::Method_PeerGet:
                  case MessageFactoryStack::Method_PeerDelete:
                  case MessageFactoryStack::Method_PeerSubscribe: {
                    hasPublication = false;
                  }
                  default: break;
                }
                break;
              }
              case Message::MessageType_Result:
              case Message::MessageType_Reply:    {
                switch (msg->method()) {
                  case MessageFactoryStack::Method_PeerPublish:
                  case MessageFactoryStack::Method_PeerSubscribe: {
                    hasPublication = false;
                  }
                  case MessageFactoryStack::Method_PeerGet:
                  {
                    hasPublication = true;
                    break;
                  }
                  default: break;
                }
                break;
              }
              case Message::MessageType_Invalid:  break;
            }

            if (!dataEl) {
              hasPublication = false;
            }

            if (hasPublication) {
              IPublicationForMessagesPtr publication = IPublicationForMessages::create(
                                                                                       version,
                                                                                       baseVersion,
                                                                                       lineage,
                                                                                       IPublicationMetaData::Source_Finder,
                                                                                       creatorContactID,
                                                                                       creatorLocationID,
                                                                                       nameEl->getText(),
                                                                                       mimeType,
                                                                                       dataEl,
                                                                                       encoding,
                                                                                       relationships,
                                                                                       "",
                                                                                       "",
                                                                                       scope,
                                                                                       lifetime,
                                                                                       expires
                                                                                       );
              outPublicationMetaData = publication->convertIPublication();
              outPublication = publication->convertIPublication();
            } else {
              IPublicationMetaDataForMessagesPtr metaData = IPublicationMetaDataForMessages::create(
                                                                                                    version,
                                                                                                    baseVersion,
                                                                                                    lineage,
                                                                                                    IPublicationMetaData::Source_Finder,
                                                                                                    creatorContactID,
                                                                                                    creatorLocationID,
                                                                                                    nameEl->getText(),
                                                                                                    mimeType,
                                                                                                    encoding,
                                                                                                    relationships,
                                                                                                    "",
                                                                                                    "",
                                                                                                    scope,
                                                                                                    lifetime,
                                                                                                    expires
                                                                                                    );
              outPublicationMetaData = metaData->convertIPublicationMetaData();
            }
          } catch (Numeric<Time>::ValueOutOfRange &) {
          } catch (CheckFailed &) {
          }
        }

        //---------------------------------------------------------------------
        int MessageHelper::stringToInt(const String &s)
        {
          if (s.isEmpty()) return 0;

          try {
            return Numeric<int>(s);
          } catch (Numeric<int>::ValueOutOfRange &) {
            ZS_LOG_WARNING(Detail, "unable to convert value to int, value=" + s)
          }
          return 0;
        }


        //---------------------------------------------------------------------
        UINT MessageHelper::stringToUint(const String &s)
        {
          if (s.isEmpty()) return 0;

          try {
            return Numeric<UINT>(s);
          } catch (Numeric<UINT>::ValueOutOfRange &) {
            ZS_LOG_WARNING(Detail, "unable to convert value to unsigned int, value=" + s)
          }
          return 0;
        }

        //---------------------------------------------------------------------
        WORD MessageHelper::getErrorCode(ElementPtr root)
        {
          if (!root) return 0;

          ElementPtr errorEl = root->findFirstChildElement("error");
          if (!errorEl) return 0;

          ElementPtr reasonEl = errorEl->findFirstChildElement("reason");
          if (!reasonEl) return 0;

          String ec = IMessageHelper::getAttributeID(reasonEl);
          if (ec.isEmpty()) return 0;

          try {
            return (Numeric<WORD>(ec));
          } catch(Numeric<WORD>::ValueOutOfRange &) {
            ZS_LOG_WARNING(Detail, "unable to convert value to error code, value=" + ec)
          }
          return 0;
        }

        //---------------------------------------------------------------------
        String MessageHelper::getErrorReason(ElementPtr root)
        {
          if (!root) return String();

          ElementPtr errorEl = root->findFirstChildElement("error");
          if (!errorEl) return String();

          ElementPtr reasonEl = errorEl->findFirstChildElement("reason");
          if (!reasonEl) return String();

          return IMessageHelper::getElementText(reasonEl);
        }

        //---------------------------------------------------------------------
        Location MessageHelper::createLocation(ElementPtr elem)
        {
          typedef zsLib::IPAddress IPAddress;

          Location ret;
          if (elem)
          {
            ret.mID = IMessageHelper::getAttributeID(elem);

            ElementPtr challenge = elem->findFirstChildElement("challenge");
            if (challenge)
            {
              ret.mLocationSalt = IMessageHelper::getElementText(challenge->findFirstChildElement("locationSalt"));
              ret.mLocationFindSecretProof = IMessageHelper::getElementText(challenge->findFirstChildElement("locationFindSecretProof"));
            }

            ElementPtr contact = elem->findFirstChildElement("contact");
            if (contact)
            {
              ret.mContactID = IMessageHelper::getAttributeID(contact);
            }

            ElementPtr candidates = elem->findFirstChildElement("candidates");
            if (candidates)
            {
              CandidateList candidateLst;
              ElementPtr candidate = candidates->findFirstChildElement("candidate");
              while (candidate)
              {
                Candidate c = MessageHelper::createCandidate(candidate);
                candidateLst.push_back(c);

                candidate = candidate->getNextSiblingElement();
              }

              if (candidateLst.size() > 0)
                ret.mCandidates = candidateLst;
            }

            ElementPtr reason = elem->findFirstChildElement("reason");
            if (reason)
            {
              try {
                ret.mReasonID = Numeric<WORD>(IMessageHelper::getAttributeID(reason));
              } catch(Numeric<WORD>::ValueOutOfRange &) {
              }
              ret.mReason = IMessageHelper::getElementText(reason);
            }

            if (elem->getValue() == "location")
              elem = elem->findFirstChildElement("details");

            if (elem)
            {
              ElementPtr device = elem->findFirstChildElement("device");
              ElementPtr ip = elem->findFirstChildElement("ip");
              ElementPtr ua = elem->findFirstChildElement("userAgent");
              ElementPtr os = elem->findFirstChildElement("os");
              ElementPtr system = elem->findFirstChildElement("system");
              ElementPtr host = elem->findFirstChildElement("host");
              if (device) {
                ret.mDeviceID = IMessageHelper::getAttribute(device, "id");
              }
              if (ip) {
                IPAddress ipOriginal(IMessageHelper::getElementText(ip), 0);

                ret.mIPAddress.mIPAddress = ipOriginal.mIPAddress;
              }
              if (ua) ret.mUserAgent = IMessageHelper::getElementTextAndEntityDecode(ua);
              if (os) ret.mOS = IMessageHelper::getElementTextAndEntityDecode(os);
              if (system) ret.mSystem = IMessageHelper::getElementTextAndEntityDecode(system);
              if (host) ret.mHost = IMessageHelper::getElementTextAndEntityDecode(host);
            }

          }
          return ret;
        }

        //---------------------------------------------------------------------
        Candidate MessageHelper::createCandidate(ElementPtr elem)
        {
          typedef zsLib::DWORD DWORD;
          typedef zsLib::IPAddress IPAddress;

          Candidate ret;
          if (elem)
          {
            //ElementPtr transport = elem->findFirstChildElement("transport"); hardcoded for now
            ElementPtr ip = elem->findFirstChildElement("ip");
            ElementPtr port = elem->findFirstChildElement("port");
            ElementPtr un = elem->findFirstChildElement("usernameFrag");
            ElementPtr pwd = elem->findFirstChildElement("password");
            ElementPtr priority = elem->findFirstChildElement("priority");
            ElementPtr protocol = elem->findFirstChildElement("protocol");

            if (ip)
            {
              WORD portNo = 0;
              if(port) {
                try {
                  portNo = Numeric<WORD>(IMessageHelper::getElementText(port));
                } catch(Numeric<WORD>::ValueOutOfRange &) {
                }
              }

              IPAddress ipOriginal(IMessageHelper::getElementText(ip), portNo);
              ret.mIPAddress.mIPAddress = ipOriginal.mIPAddress;
              ret.mIPAddress.setPort(portNo);
            }
            if (un) ret.mUsernameFrag = IMessageHelper::getElementText(un);
            if (pwd) ret.mPassword = IMessageHelper::getElementText(pwd);
            if (priority) {
              try {
                ret.mPriority = (DWORD)Numeric<DWORD>(IMessageHelper::getElementText(priority));
              } catch(Numeric<DWORD>::ValueOutOfRange &) {
              }
            }
            else ret.mPriority = 0;
            if (protocol) ret.mProtocol = IMessageHelper::getElementText(protocol);
          }
          return ret;
        }

        //---------------------------------------------------------------------
        Service MessageHelper::createService(ElementPtr elem)
        {
          Service ret;
          if (!elem) return ret;

          ret.mID = IMessageHelper::getAttributeID(elem);
          ret.mType = IMessageHelper::getElementText(elem->findFirstChildElement("type"));
          ret.mURL = IMessageHelper::getElementText(elem->findFirstChildElement("url"));
          ret.mVersion = IMessageHelper::getElementText(elem->findFirstChildElement("version"));
          try {
            ret.mX509Certificate = IMessageHelper::getElementText(elem->findFirstChildElementChecked("KeyInfo")->findFirstChildElementChecked("X509Data")->findFirstChildElementChecked("X509Certificate"));
          } catch(CheckFailed &) {
            ZS_LOG_BASIC("createService XML check failure")
          }

          return ret;
        }

        //---------------------------------------------------------------------
        Finder MessageHelper::createFinder(ElementPtr elem)
        {
          typedef zsLib::FLOAT FLOAT;

          Finder ret;
          if (!elem) return ret;

          ret.mID = IMessageHelper::getAttributeID(elem);
          ret.mTransport = IMessageHelper::getElementText(elem->findFirstChildElement("transport"));
          ret.mSRV = IMessageHelper::getElementText(elem->findFirstChildElement("srv"));

          try
          {
            ret.mX509Certificate = IMessageHelper::getElementText(elem->findFirstChildElementChecked("KeyInfo")->findFirstChildElementChecked("X509Data")->findFirstChildElementChecked("X509Certificate"));
            try {
              ret.mPriority = Numeric<WORD>(IMessageHelper::getElementText(elem->findFirstChildElementChecked("priority")));
            } catch(Numeric<WORD>::ValueOutOfRange &) {
            }
            try {
              ret.mWeight = Numeric<WORD>(IMessageHelper::getElementText(elem->findFirstChildElementChecked("weight")));
            } catch(Numeric<WORD>::ValueOutOfRange &) {
            }
          }
          catch(CheckFailed &) {
            ZS_LOG_BASIC("createFinder XML check failure")
          }

          return ret;
        }
      }
    }
  }
}
