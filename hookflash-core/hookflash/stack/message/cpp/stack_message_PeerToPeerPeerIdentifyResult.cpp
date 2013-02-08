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

#include <hookflash/stack/message/internal/stack_message_PeerToPeerPeerIdentifyResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerToPeerPeerIdentifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/zsHelpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;

      PeerToPeerPeerIdentifyResultPtr PeerToPeerPeerIdentifyResult::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerToPeerPeerIdentifyResult>(message);
      }

      PeerToPeerPeerIdentifyResult::PeerToPeerPeerIdentifyResult()
      {
      }

      bool PeerToPeerPeerIdentifyResult::hasAttribute(PeerToPeerPeerIdentifyResult::AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_ContactID:
            return !mContactID.isEmpty();
          case AttributeType_LocationID:
            return !mLocationID.isEmpty();
          case AttributeType_UserAgent:
            return !mUserAgent.isEmpty();
          default:
            break;
        }
        return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
      }

      DocumentPtr PeerToPeerPeerIdentifyResult::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerToPeerPeerIdentifyResult::encode(*this);
      }

      PeerToPeerPeerIdentifyResultPtr PeerToPeerPeerIdentifyResult::create(PeerToPeerPeerIdentifyRequestPtr request)
      {
        return internal::PeerToPeerPeerIdentifyResult::create(request);
      }


      namespace internal
      {
        PeerToPeerPeerIdentifyResultPtr PeerToPeerPeerIdentifyResult::create(ElementPtr root)
        {
          PeerToPeerPeerIdentifyResultPtr ret(new message::PeerToPeerPeerIdentifyResult);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          ret->mContactID = IMessageHelper::getAttributeID(root->findFirstChildElement("contact"));
          ret->mLocationID = IMessageHelper::getAttributeID(root->findFirstChildElement("location"));
          ret->mUserAgent = IMessageHelper::getElementTextAndEntityDecode(root->findFirstChildElement("userAgent"));

          return ret;
        }

        PeerToPeerPeerIdentifyResultPtr PeerToPeerPeerIdentifyResult::create(PeerToPeerPeerIdentifyRequestPtr request)
        {
          PeerToPeerPeerIdentifyResultPtr ret(new message::PeerToPeerPeerIdentifyResult);

          ret->mID = request->messageID();
          ret->mTime = zsLib::now();

          return ret;
        }

        DocumentPtr PeerToPeerPeerIdentifyResult::encode(message::PeerToPeerPeerIdentifyResult &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          if (msg.hasAttribute(message::PeerToPeerPeerIdentifyResult::AttributeType_ContactID))
          {
            root->adoptAsLastChild(IMessageHelper::createElementWithID("contact", msg.mContactID));
          }

          if (msg.hasAttribute(message::PeerToPeerPeerIdentifyResult::AttributeType_LocationID))
          {
            root->adoptAsLastChild(IMessageHelper::createElementWithID("location", msg.mLocationID));
          }

          if (msg.hasAttribute(message::PeerToPeerPeerIdentifyResult::AttributeType_UserAgent))
          {
            root->adoptAsLastChild(IMessageHelper::createElementWithTextAndEntityEncode("userAgent", msg.mUserAgent));
          }

          return ret;
        }
      }
    }
  }
}
