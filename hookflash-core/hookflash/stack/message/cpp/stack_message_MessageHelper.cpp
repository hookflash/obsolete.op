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

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/message/IMessageFactory.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/message/MessageReply.h>

#include <hookflash/stack/message/peer-common/MessageFactoryPeerCommon.h>
#include <hookflash/stack/message/peer-common/PeerPublishRequest.h>
#include <hookflash/stack/message/peer-common/PeerGetResult.h>

#include <hookflash/stack/IPublicationRepository.h>
#include <hookflash/stack/internal/stack_Location.h>
#include <hookflash/stack/internal/stack_Peer.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/internal/stack_Publication.h>
#include <hookflash/stack/internal/stack_PublicationMetaData.h>
#include <hookflash/stack/IRSAPublicKey.h>

#include <zsLib/Numeric.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      using zsLib::DWORD;
      using zsLib::Stringize;
      using zsLib::Numeric;

      using namespace stack::internal;

      using peer_common::MessageFactoryPeerCommon;
      using peer_common::PeerPublishRequest;
      using peer_common::PeerGetResult;

      typedef stack::IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;

      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

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

        ElementPtr root = Element::create(tagName);
        ret->adoptAsFirstChild(root);

        String domain = message.domain();

        if (!domain.isEmpty()) {
          IMessageHelper::setAttribute(root, "domain", domain);
        }
        IMessageHelper::setAttribute(root, "handler", factory->getHandler());
        IMessageHelper::setAttributeID(root, message.messageID());
        IMessageHelper::setAttribute(root, "method", factory->toString(message.method()));

        if (message.isResult()) {
          const message::MessageResult *msgResult = (dynamic_cast<const message::MessageResult *>(&message));
          if (msgResult->hasAttribute(MessageResult::AttributeType_Time)) {
            IMessageHelper::setAttributeEpoch(root, msgResult->time());
          }
        }
        if (message.isReply()) {
          const message::MessageReply *msgReply = (dynamic_cast<const message::MessageReply *>(&message));
          if (msgReply->hasAttribute(MessageReply::AttributeType_Time)) {
            IMessageHelper::setAttributeEpoch(root, msgReply->time());
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
        elem->setAttribute("epoch", timeToString(value), false);
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
        ElementPtr tmp = Element::create(elName);

        if (textVal.isEmpty()) return tmp;

        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(textVal, Text::Format_JSONStringEncoded);

        tmp->adoptAsFirstChild(tmpTxt);

        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithNumber(
                                                         const String &elName,
                                                         const String &numberAsStringValue
                                                         )
      {
        ElementPtr tmp = Element::create(elName);

        if (numberAsStringValue.isEmpty()) return tmp;

        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(numberAsStringValue, Text::Format_JSONNumberEncoded);
        tmp->adoptAsFirstChild(tmpTxt);

        return tmp;
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithTime(
                                                       const String &elName,
                                                       Time time
                                                       )
      {
        return createElementWithNumber(elName, timeToString(time));
      }

      //-----------------------------------------------------------------------
      ElementPtr IMessageHelper::createElementWithTextAndJSONEncode(
                                                                      const String &elName,
                                                                      const String &textVal
                                                                      )
      {
        ElementPtr tmp = Element::create(elName);
        if (textVal.isEmpty()) return tmp;

        TextPtr tmpTxt = Text::create();
        tmpTxt->setValueAndJSONEncode(textVal);
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

        if (idValue.isEmpty()) return tmp;

        setAttributeID(tmp, idValue);
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
      String IMessageHelper::getElementTextAndDecode(ElementPtr node)
      {
        if (!node) return String();
        return node->getTextDecoded();
      }

      //-----------------------------------------------------------------------
      void IMessageHelper::fill(
                                Message &message,
                                ElementPtr root,
                                IMessageSourcePtr source
                                )
      {
        String id = IMessageHelper::getAttribute(root, "id");
        String domain = IMessageHelper::getAttribute(root, "domain");

        if (!id.isEmpty()) {
          message.messageID(id);
        }
        if (!domain.isEmpty()) {
          message.domain(domain);
        }
        if (message.isResult()) {
          Time time = IMessageHelper::getAttributeEpoch(root);
          message::MessageResult *result = (dynamic_cast<message::MessageResult *>(&message));
          result->time(time);
        }
        if (message.isReply()) {
          Time time = IMessageHelper::getAttributeEpoch(root);
          message::MessageReply *reply = (dynamic_cast<message::MessageReply *>(&message));
          reply->time(time);
        }
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
        ElementPtr MessageHelper::createElement(
                                                const Candidate &candidate,
                                                const SecureByteBlock *encryptionKey
                                                )
        {
          ElementPtr candidateEl = IMessageHelper::createElement("candidate");

          candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("transport", "rudp/udp"));

          ElementPtr ipEl = IMessageHelper::createElementWithText("ip", candidate.mIPAddress.string(false));
          candidateEl->adoptAsLastChild(ipEl);
          candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("port", Stringize<WORD>(candidate.mIPAddress.getPort())));

          if (!candidate.mUsernameFrag.isEmpty())
          {
            candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("usernameFrag", candidate.mUsernameFrag));
          }

          if (!candidate.mPassword.isEmpty())
          {
            if (encryptionKey) {
              String encryptedPassword = IHelper::convertToBase64(*IHelper::encrypt(*encryptionKey, *IHelper::hash(candidate.mUsernameFrag, IHelper::HashAlgorthm_MD5), candidate.mPassword));
              candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("passwordEncrypted", encryptedPassword));
            } else {
              candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("password", candidate.mPassword));
            }
          }

          if (candidate.mPriority > 0)
          {
            candidateEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("priority", Stringize<DWORD>(candidate.mPriority)));
          }

          if (!candidate.mProtocol.isEmpty())
          {
            candidateEl->adoptAsLastChild(IMessageHelper::createElementWithText("protocol", candidate.mProtocol));
          }

          return candidateEl;
        }

        //---------------------------------------------------------------------
        ElementPtr MessageHelper::createElement(
                                                const LocationInfo &locationInfo,
                                                const SecureByteBlock *encryptionKey
                                                )
        {
          if (!locationInfo.mLocation) {
            ZS_LOG_WARNING(Detail, "MessageHelper [] missing location object in location info")
            return ElementPtr();
          }

          LocationPtr location = Location::convert(locationInfo.mLocation);

          ElementPtr locationEl = IMessageHelper::createElementWithID("location", location->forMessages().getLocationID());
          ElementPtr detailEl = IMessageHelper::createElement("details");

          if (!locationInfo.mIPAddress.isAddressEmpty())
          {
            ElementPtr ipEl = IMessageHelper::createElementWithText("ip", locationInfo.mIPAddress.string(false));
            detailEl->adoptAsLastChild(ipEl);
          }

          if (!locationInfo.mDeviceID.isEmpty()) {
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithID("device", locationInfo.mDeviceID));
          }

          if (!locationInfo.mUserAgent.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("userAgent", locationInfo.mUserAgent));

          if (!locationInfo.mOS.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("os", locationInfo.mOS));

          if (!locationInfo.mSystem.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("system", locationInfo.mSystem));

          if (!locationInfo.mHost.isEmpty())
            detailEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("host", locationInfo.mHost));

          PeerPtr peer = location->forMessages().getPeer();
          if (peer) {
            locationEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("contact", peer->forMessages().getPeerURI()));
          }

          if (detailEl->hasChildren()) {
            locationEl->adoptAsLastChild(detailEl);
          }

          if (locationInfo.mCandidates.size() > 0)
          {
            ElementPtr candidates = IMessageHelper::createElement("candidates");
            locationEl->adoptAsLastChild(candidates);

            CandidateList::const_iterator it;
            for(it=locationInfo.mCandidates.begin(); it!=locationInfo.mCandidates.end(); ++it)
            {
              Candidate candidate(*it);
              candidates->adoptAsLastChild(MessageHelper::createElement(candidate, encryptionKey));
            }

            locationEl->adoptAsLastChild(candidates);
          }

          return locationEl;
        }

        //---------------------------------------------------------------------
        ElementPtr MessageHelper::createElement(
                                                const IdentityInfo &identity,
                                                bool forcePriorityWeightOutput
                                                )
        {
          ElementPtr identityEl = Element::create("identity");
          if (IdentityInfo::Disposition_NA != identity.mDisposition) {
            identityEl->setAttribute("disposition", IdentityInfo::toString(identity.mDisposition));
          }

          if (!identity.mAccessToken.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("accessToken", identity.mAccessToken));
          }
          if (!identity.mAccessSecret.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("accessSecret", identity.mAccessSecret));
          }
          if (Time() != identity.mAccessSecretExpires) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("accessSecretExpires", IMessageHelper::timeToString(identity.mAccessSecretExpires)));
          }
          if (!identity.mAccessSecretProof.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("accessSecretProof", identity.mAccessSecretProof));
          }
          if (Time() != identity.mAccessSecretProofExpires) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("accessSecretProofExpires", IMessageHelper::timeToString(identity.mAccessSecretProofExpires)));
          }

          if (!identity.mBase.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("base", identity.mBase));
          }
          if (!identity.mURI.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("uri", identity.mURI));
          }
          if (!identity.mURIEncrypted.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("uriEncrypted", identity.mURIEncrypted));
          }
          if (!identity.mHash.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("hash", identity.mHash));
          }
          if (!identity.mProvider.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("provider", identity.mProvider));
          }

          if (!identity.mContactUserID.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("contactUserID", identity.mContactUserID));
          }
          if (!identity.mContact.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("contact", identity.mContact));
          }
          if (!identity.mContactFindSecret.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("contactFindSecret", identity.mContactFindSecret));
          }
          if (!identity.mPrivatePeerFileSalt.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("privatePeerFileSalt", identity.mPrivatePeerFileSalt));
          }
          if (!identity.mPrivatePeerFileSecretEncrypted.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("privatePeerFileSecretEncrypted", identity.mPrivatePeerFileSecretEncrypted));
          }

          if (Time() != identity.mLastReset) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("lastRest", IMessageHelper::timeToString(identity.mLastReset)));
          }
          if (!identity.mReloginAccessKey.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("reloginAccessKey", identity.mReloginAccessKey));
          }
          if (!identity.mReloginAccessKeyEncrypted.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("reloginAccessKeyEncrypted", identity.mReloginAccessKeyEncrypted));
          }

          if (!identity.mSecretSalt.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("secretSalt", identity.mSecretSalt));
          }
          if (!identity.mSecretEncrypted.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("secretEncrypted", identity.mSecretEncrypted));
          }
          if (!identity.mSecretDecryptionKeyEncrypted.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("secretDecryptionKeyEncrypted", identity.mSecretDecryptionKeyEncrypted));
          }

          if ((0 != identity.mPriority) ||
              (forcePriorityWeightOutput)) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("priority", Stringize<WORD>(identity.mPriority).string()));
          }
          if ((0 != identity.mWeight) ||
              (forcePriorityWeightOutput)) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("weight", Stringize<WORD>(identity.mWeight).string()));
          }

          if (Time() != identity.mUpdated) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("updated", IMessageHelper::timeToString(identity.mUpdated)));
          }
          if (Time() != identity.mExpires) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("expires", IMessageHelper::timeToString(identity.mExpires)));
          }

          if (!identity.mName.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("name", identity.mName));
          }
          if (!identity.mProfile.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("profile", identity.mProfile));
          }
          if (!identity.mVProfile.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("vprofile", identity.mVProfile));
          }

          if (!identity.mProfile.isEmpty()) {
            identityEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("profile", identity.mProfile));
          }

          if (identity.mAvatars.size() > 0) {
            ElementPtr avatarsEl = Element::create("avatars");
            for (IdentityInfo::AvatarList::const_iterator iter = identity.mAvatars.begin(); iter != identity.mAvatars.end(); ++iter)
            {
              const IdentityInfo::Avatar &avatar = (*iter);
              ElementPtr avatarEl = Element::create("avatar");

              if (!avatar.mName.isEmpty()) {
                avatarEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("name", avatar.mName));
              }
              if (!avatar.mURL.isEmpty()) {
                avatarEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("name", avatar.mURL));
              }
              if (0 != avatar.mWidth) {
                avatarEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("width", Stringize<int>(avatar.mWidth).string()));
              }
              if (0 != avatar.mHeight) {
                avatarEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("height", Stringize<int>(avatar.mHeight).string()));
              }

              if (avatarEl->hasChildren()) {
                avatarsEl->adoptAsLastChild(avatarEl);
              }
            }
            if (avatarsEl->hasChildren()) {
              identityEl->adoptAsLastChild(avatarsEl);
            }
          }

          return identityEl;
        }

        //---------------------------------------------------------------------
        ElementPtr MessageHelper::createElement(
                                                const PublishToRelationshipsMap &relationships,
                                                const char *elementName
                                                )
        {
          ElementPtr rootEl = IMessageHelper::createElement(elementName);

          for (PublishToRelationshipsMap::const_iterator iter = relationships.begin(); iter != relationships.end(); ++iter)
          {
            String name = (*iter).first;
            const stack::IPublicationMetaData::PermissionAndPeerURIListPair &permission = (*iter).second;

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

            for (stack::IPublicationMetaData::PeerURIList::const_iterator contactIter = permission.second.begin(); contactIter != permission.second.end(); ++contactIter)
            {
              ElementPtr contactEl = IMessageHelper::createElementWithTextAndJSONEncode("contact", (*contactIter));
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
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          PublicationPtr publication = Publication::convert(publicationMetaData->toPublication());

          ULONG fromVersion = 0;
          ULONG toVersion = 0;

          if (publication) {
            fromVersion = publication->forMessages().getBaseVersion();
            toVersion = publication->forMessages().getVersion();
          } else {
            fromVersion = publicationMetaData->getBaseVersion();
            toVersion = publicationMetaData->getVersion();
          }

          // do not use the publication for these message types...
          switch ((MessageFactoryPeerCommon::Methods)msg.method()) {
            case MessageFactoryPeerCommon::Method_PeerPublish:
            {
              if (Message::MessageType_Request == msg.messageType()) {
                PeerPublishRequest &request = *(dynamic_cast<PeerPublishRequest *>(&msg));
                fromVersion = request.publishedFromVersion();
                toVersion = request.publishedToVersion();
              }
              if (Message::MessageType_Request != msg.messageType()) {
                // only the request can include a publication...
                publication.reset();
              }
              break;
            }
            case MessageFactoryPeerCommon::Method_PeerGet:
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
            case MessageFactoryPeerCommon::Method_PeerDelete:
            case MessageFactoryPeerCommon::Method_PeerSubscribe:
            {
              // these messages requests/results will never include a publication (at least at this time)
              publication.reset();
              break;
            }
            case MessageFactoryPeerCommon::Method_PeerPublishNotify:
            {
              if (publication) {
                if (peerCache) {
                  ULONG bogusFillSize = 0;
                  ULONG &maxFillSize = (notifyPeerPublishMaxDocumentSizeInBytes ? *notifyPeerPublishMaxDocumentSizeInBytes : bogusFillSize);
                  if (peerCache->getNextVersionToNotifyAboutAndMarkNotified(publication, maxFillSize, fromVersion, toVersion)) {
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

          // make a copy of the relationships
          PublishToRelationshipsMap relationships = publicationMetaData->getRelationships();

          ElementPtr docEl = IMessageHelper::createElement("document");
          ElementPtr detailsEl = IMessageHelper::createElement("details");
          ElementPtr publishToRelationshipsEl = MessageHelper::createElement(relationships, MessageFactoryPeerCommon::Method_PeerSubscribe == (MessageFactoryPeerCommon::Methods)msg.method() ? "subscribeToRelationships" : "publishToRelationships");
          ElementPtr dataEl = IMessageHelper::createElement("data");

          String creatorPeerURI;
          String creatorLocationID;

          LocationPtr creatorLocation = Location::convert(publicationMetaData->getCreatorLocation());
          if (creatorLocation ) {
            creatorLocationID = creatorLocation->forMessages().getLocationID();
            creatorPeerURI = creatorLocation->forMessages().getPeerURI();
          }

          ElementPtr contactEl = IMessageHelper::createElementWithTextAndJSONEncode("contact", creatorPeerURI);
          ElementPtr locationEl = IMessageHelper::createElementWithTextAndJSONEncode("location", creatorLocationID);

          NodePtr publishedDocEl;
          if (publication) {
            publishedDocEl = publication->forMessages().getDiffs(
                                                                 fromVersion,
                                                                 toVersion
                                                                 );

            if (0 == toVersion) {
              // put the version back to something more sensible
              toVersion = publicationMetaData->getVersion();
            }
          }

          ElementPtr nameEl = IMessageHelper::createElementWithText("name", publicationMetaData->getName());
          ElementPtr versionEl = IMessageHelper::createElementWithNumber("version", Stringize<ULONG>(toVersion));
          ElementPtr baseVersionEl = IMessageHelper::createElementWithNumber("baseVersion", Stringize<ULONG>(fromVersion));
          ElementPtr lineageEl = IMessageHelper::createElementWithNumber("lineage", Stringize<ULONG>(publicationMetaData->getLineage()));
          ElementPtr chunkEl = IMessageHelper::createElementWithText("chunk", "1/1");

          ElementPtr expiresEl;
          if (publicationMetaData->getExpires() != Time()) {
            expiresEl = IMessageHelper::createElementWithNumber("expires", timeToString(publicationMetaData->getExpires()));
          }

          ElementPtr mimeTypeEl = IMessageHelper::createElementWithText("mime", publicationMetaData->getMimeType());

          stack::IPublication::Encodings encoding = publicationMetaData->getEncoding();
          const char *encodingStr = "binary";
          switch (encoding) {
            case stack::IPublication::Encoding_Binary:  encodingStr = "binary"; break;
            case stack::IPublication::Encoding_JSON:    encodingStr = "json"; break;
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
          detailsEl->adoptAsLastChild(contactEl);
          detailsEl->adoptAsLastChild(locationEl);
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
              switch ((MessageFactoryPeerCommon::Methods)msg.method()) {

                case MessageFactoryPeerCommon::Method_PeerPublish:         break;

                case MessageFactoryPeerCommon::Method_PeerGet:             {
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  publishToRelationshipsEl->orphan();
                  break;
                }
                case MessageFactoryPeerCommon::Method_PeerDelete:          {
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  chunkEl->orphan();
                  contactEl->orphan();
                  locationEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  publishToRelationshipsEl->orphan();
                }
                case MessageFactoryPeerCommon::Method_PeerSubscribe:       {
                  versionEl->orphan();
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  lineageEl->orphan();
                  chunkEl->orphan();
                  contactEl->orphan();
                  locationEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  break;
                }
                case MessageFactoryPeerCommon::Method_PeerPublishNotify:
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
              switch ((MessageFactoryPeerCommon::Methods)msg.method()) {
                case MessageFactoryPeerCommon::Method_PeerPublish:       {
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  locationEl->orphan();
                  contactEl->orphan();
                  dataEl->orphan();
                  break;
                }
                case MessageFactoryPeerCommon::Method_PeerGet:           {
                  break;
                }
                case MessageFactoryPeerCommon::Method_PeerDelete:        {
                  ZS_THROW_INVALID_USAGE("this method should not be used for delete result")
                  break;
                }
                case MessageFactoryPeerCommon::Method_PeerSubscribe:     {
                  versionEl->orphan();
                  if (baseVersionEl)
                    baseVersionEl->orphan();
                  lineageEl->orphan();
                  chunkEl->orphan();
                  contactEl->orphan();
                  locationEl->orphan();
                  if (expiresEl)
                    expiresEl->orphan();
                  mimeTypeEl->orphan();
                  encodingEl->orphan();
                  dataEl->orphan();
                  break;
                }
                case MessageFactoryPeerCommon::Method_PeerPublishNotify: {
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
   <contact>peer://domain.com/ea00ede4405c99be9ae45739ebfe57d5<contact/>
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
   <contact>peer://domain.com/ea00ede4405c99be9ae45739ebfe57d5<contact/>
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
    <contact>peer://domain.com/bd520f1dbaa13c0cc9b7ff528e83470e</contact>
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
     <contact>peer://domain.com/bd520f1dbaa13c0cc9b7ff528e83470e</contact>
     <contact>peer://domain.com/8d17a88e8d42ffbd138f3895ec45375c</contact>
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
    <contact>peer://domain.com/ea00ede4405c99be9ae45739ebfe57d5</contact>
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
                                     IMessageSourcePtr messageSource,
                                     MessagePtr msg,
                                     ElementPtr rootEl,
                                     IPublicationPtr &outPublication,
                                     IPublicationMetaDataPtr &outPublicationMetaData
                                     )
        {
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

            String contact;
            ElementPtr contactEl = detailsEl->findFirstChildElement("contact");
            if (contactEl) {
              contact = contactEl->getTextDecoded();
            }

            String locationID;
            ElementPtr locationEl = detailsEl->findFirstChildElement("location");
            if (locationEl) {
              locationID = locationEl->getTextDecoded();
            }

            LocationPtr location = ILocationForMessages::create(messageSource, contact, locationID);

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

            IPublicationMetaData::Encodings encoding = IPublicationMetaData::Encoding_Binary;
            if (encodingEl) {
              String encodingStr = encodingEl->getText();
              if (encodingStr == "json") encoding = IPublicationMetaData::Encoding_JSON;
              else if (encodingStr == "binary") encoding = IPublicationMetaData::Encoding_Binary;
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

            ElementPtr publishToRelationshipsEl = docEl->findFirstChildElement(MessageFactoryPeerCommon::Method_PeerSubscribe == (MessageFactoryPeerCommon::Methods)msg->method() ? "subscribeToRelationships" : "publishToRelationships");
            if (publishToRelationshipsEl) {
              ElementPtr relationshipsEl = publishToRelationshipsEl->findFirstChildElement("relationships");
              while (relationshipsEl)
              {
                String name = relationshipsEl->getAttributeValue("name");
                String allowStr = relationshipsEl->getAttributeValue("allow");

                IPublicationMetaData::PeerURIList contacts;
                ElementPtr contactEl = relationshipsEl->findFirstChildElement("contact");
                while (contactEl)
                {
                  String contact = contactEl->getTextDecoded();
                  if (contact.size() > 0) {
                    contacts.push_back(contact);
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
                  relationships[name] = IPublicationMetaData::PermissionAndPeerURIListPair(permission, contacts);
                }

                relationshipsEl = relationshipsEl->findNextSiblingElement("relationships");
              }
            }

            bool hasPublication = false;

            switch (msg->messageType()) {
              case Message::MessageType_Request:
              case Message::MessageType_Notify:   {
                switch ((MessageFactoryPeerCommon::Methods)msg->method()) {
                  case MessageFactoryPeerCommon::Method_PeerPublish:
                  case MessageFactoryPeerCommon::Method_PeerPublishNotify: {
                    hasPublication = true;
                    break;
                  }
                  case MessageFactoryPeerCommon::Method_PeerGet:
                  case MessageFactoryPeerCommon::Method_PeerDelete:
                  case MessageFactoryPeerCommon::Method_PeerSubscribe: {
                    hasPublication = false;
                  }
                  default: break;
                }
                break;
              }
              case Message::MessageType_Result:
              case Message::MessageType_Reply:    {
                switch ((MessageFactoryPeerCommon::Methods)msg->method()) {
                  case MessageFactoryPeerCommon::Method_PeerPublish:
                  case MessageFactoryPeerCommon::Method_PeerSubscribe: {
                    hasPublication = false;
                  }
                  case MessageFactoryPeerCommon::Method_PeerGet:
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
              PublicationPtr publication = IPublicationForMessages::create(
                                                                           version,
                                                                           baseVersion,
                                                                           lineage,
                                                                           location,
                                                                           nameEl->getText(),
                                                                           mimeType,
                                                                           dataEl,
                                                                           encoding,
                                                                           relationships,
                                                                           location,
                                                                           expires
                                                                           );
              outPublicationMetaData = publication->forMessages().toPublicationMetaData();
              outPublication = publication;
            } else {
              PublicationMetaDataPtr metaData = IPublicationMetaDataForMessages::create(
                                                                                        version,
                                                                                        baseVersion,
                                                                                        lineage,
                                                                                        location,
                                                                                        nameEl->getText(),
                                                                                        mimeType,
                                                                                        encoding,
                                                                                        relationships,
                                                                                        location,
                                                                                        expires
                                                                                        );
              outPublicationMetaData = metaData;
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
        LocationInfo MessageHelper::createLocation(
                                                   ElementPtr elem,
                                                   IMessageSourcePtr messageSource,
                                                   const SecureByteBlock *encryptionKey
                                                   )
        {
          LocationInfo ret;
          if (!elem) return ret;

          String id = IMessageHelper::getAttributeID(elem);

          ElementPtr contact = elem->findFirstChildElement("contact");
          if (contact)
          {
            String peerURI = IMessageHelper::getElementTextAndDecode(contact);
            ret.mLocation = ILocationForMessages::create(messageSource, peerURI, id);
          }

          ElementPtr candidates = elem->findFirstChildElement("candidates");
          if (candidates)
          {
            CandidateList candidateLst;
            ElementPtr candidate = candidates->findFirstChildElement("candidate");
            while (candidate)
            {
              Candidate c = MessageHelper::createCandidate(candidate, encryptionKey);
              candidateLst.push_back(c);

              candidate = candidate->getNextSiblingElement();
            }

            if (candidateLst.size() > 0)
              ret.mCandidates = candidateLst;
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
            if (ua) ret.mUserAgent = IMessageHelper::getElementTextAndDecode(ua);
            if (os) ret.mOS = IMessageHelper::getElementTextAndDecode(os);
            if (system) ret.mSystem = IMessageHelper::getElementTextAndDecode(system);
            if (host) ret.mHost = IMessageHelper::getElementTextAndDecode(host);
          }
          return ret;
        }

        //---------------------------------------------------------------------
        Candidate MessageHelper::createCandidate(
                                                 ElementPtr elem,
                                                 const SecureByteBlock *encryptionKey
                                                 )
        {
          Candidate ret;
          if (!elem) return ret;

          ElementPtr ip = elem->findFirstChildElement("ip");
          ElementPtr port = elem->findFirstChildElement("port");
          ElementPtr un = elem->findFirstChildElement("usernameFrag");
          ElementPtr pwd = elem->findFirstChildElement("password");
          ElementPtr epwd = elem->findFirstChildElement("passwordEncrypted");
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
          if (epwd) {
            if (encryptionKey) {
              ret.mPassword = IHelper::convertToString(*IHelper::decrypt(*encryptionKey, *IHelper::hash(ret.mUsernameFrag, IHelper::HashAlgorthm_MD5), *IHelper::convertFromBase64(IMessageHelper::getElementText(pwd))));
            } else {
              ret.mPassword = IMessageHelper::getElementText(epwd);
            }
          }
          if (priority) {
            try {
              ret.mPriority = (DWORD)Numeric<DWORD>(IMessageHelper::getElementText(priority));
            } catch(Numeric<DWORD>::ValueOutOfRange &) {
            }
          }
          else ret.mPriority = 0;
          if (protocol) ret.mProtocol = IMessageHelper::getElementText(protocol);

          return ret;
        }

        //---------------------------------------------------------------------
        Finder MessageHelper::createFinder(ElementPtr elem)
        {
          Finder ret;
          if (!elem) return ret;

          ret.mID = IMessageHelper::getAttributeID(elem);
          ret.mTransport = IMessageHelper::getElementText(elem->findFirstChildElement("transport"));
          ret.mSRV = IMessageHelper::getElementText(elem->findFirstChildElement("srv"));
          ret.mRegion = IMessageHelper::getElementText(elem->findFirstChildElement("region"));
          ret.mCreated = stringToTime(IMessageHelper::getElementText(elem->findFirstChildElement("created")));
          ret.mExpires = stringToTime(IMessageHelper::getElementText(elem->findFirstChildElement("expires")));

          try
          {
            ret.mPublicKey = IRSAPublicKey::load(*IHelper::convertFromBase64(IMessageHelper::getElementText(elem->findFirstChildElementChecked("key")->findFirstChildElementChecked("X509Data"))));
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

        //---------------------------------------------------------------------
        Service MessageHelper::createService(ElementPtr serviceEl)
        {
          Service service;

          if (!serviceEl) return service;

          service.mID = IMessageHelper::getAttributeID(serviceEl);
          service.mType = IMessageHelper::getElementText(serviceEl->findFirstChildElement("type"));
          service.mVersion = IMessageHelper::getElementText(serviceEl->findFirstChildElement("version"));

          ElementPtr methodsEl = serviceEl->findFirstChildElement("methods");
          if (methodsEl) {
            ElementPtr methodEl = methodsEl->findFirstChildElement("method");
            while (methodEl) {
              Service::Method method;
              method.mName = IMessageHelper::getElementText(methodEl->findFirstChildElement("name"));
              method.mURI = IMessageHelper::getElementText(methodEl->findFirstChildElement("uri"));
              method.mUsername = IMessageHelper::getElementText(methodEl->findFirstChildElement("username"));
              method.mPassword = IMessageHelper::getElementText(methodEl->findFirstChildElement("password"));

              if (method.hasData()) {
                service.mMethods[method.mName] = method;
              }

              methodEl = methodEl->findNextSiblingElement("method");
            }
          }
          return service;
        }

        //---------------------------------------------------------------------
        IdentityInfo MessageHelper::createIdentity(ElementPtr elem)
        {
          IdentityInfo info;

          if (!elem) return info;

          info.mDisposition = IdentityInfo::toDisposition(elem->getAttributeValue("disposition"));

          info.mAccessToken = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("accessToken"));
          info.mAccessSecret = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("accessSecret"));
          info.mAccessSecretExpires = IMessageHelper::stringToTime(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("accessSecretExpires")));
          info.mAccessSecretProof = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("accessSecretProof"));
          info.mAccessSecretProofExpires = IMessageHelper::stringToTime(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("accessSecretProofExpires")));

          info.mBase = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("base"));
          info.mURI = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("uri"));
          info.mURIEncrypted = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("uriEncrypted"));
          info.mHash = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("hash"));
          info.mProvider = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("provider"));

          info.mContact = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("contact"));
          info.mContactFindSecret = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("contactFindSecret"));
          info.mPrivatePeerFileSalt = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("privatePeerFileSalt"));
          info.mPrivatePeerFileSecretEncrypted = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("privatePeerFileSecretEncrypted"));

          info.mLastReset = IMessageHelper::stringToTime(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("lastReset")));
          info.mReloginAccessKeyEncrypted = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("reloginAccessKeyEncrypted"));

          info.mSecretSalt = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("secretSalt"));
          info.mSecretEncrypted = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("secretEncrypted"));
          info.mSecretDecryptionKeyEncrypted = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("secretDecryptionKeyEncrypted"));

          try {
            info.mPriority = Numeric<WORD>(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("priority")));
          } catch(Numeric<WORD>::ValueOutOfRange &) {
          }
          try {
            info.mWeight = Numeric<WORD>(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("weight")));
          } catch(Numeric<WORD>::ValueOutOfRange &) {
          }

          info.mUpdated = IMessageHelper::stringToTime(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("updated")));
          info.mExpires = IMessageHelper::stringToTime(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("expires")));

          info.mName = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("name"));
          info.mProfile = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("profile"));
          info.mVProfile = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("vprofile"));

          ElementPtr avatarsEl = elem->findFirstChildElement("avatars");
          if (avatarsEl) {
            ElementPtr avatarEl = elem->findFirstChildElement("avatar");
            while (avatarEl) {
              IdentityInfo::Avatar avatar;
              avatar.mName = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("name"));
              avatar.mURL = IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("url"));
              try {
                avatar.mWidth = Numeric<int>(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("width")));
              } catch(Numeric<int>::ValueOutOfRange &) {
              }
              try {
                avatar.mHeight = Numeric<int>(IMessageHelper::getElementTextAndDecode(elem->findFirstChildElement("height")));
              } catch(Numeric<int>::ValueOutOfRange &) {
              }

              if (avatar.hasData()) {
                info.mAvatars.push_back(avatar);
              }
              avatarEl = elem->findNextSiblingElement("avatar");
            }
          }

          return info;
        }
      }
    }
  }
}

