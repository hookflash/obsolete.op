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

#include <hookflash/stack/message/peer-to-peer/PeerIdentifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_Location.h>
#include <hookflash/stack/internal/stack_Peer.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>
#include <zsLib/XML.h>

#define HOOKFLASH_STACK_MESSAGE_PEER_IDENTIFY_REQUEST_LIFETIME_IN_SECONDS ((60*60)*24)

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_to_peer
      {
        using zsLib::Stringize;
        using zsLib::Seconds;
        using namespace stack::internal;
        using namespace message::internal;

        typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

        //---------------------------------------------------------------------
        PeerIdentifyRequestPtr PeerIdentifyRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<PeerIdentifyRequest>(message);
        }

        //---------------------------------------------------------------------
        PeerIdentifyRequest::PeerIdentifyRequest()
        {
        }

        //---------------------------------------------------------------------
        PeerIdentifyRequestPtr PeerIdentifyRequest::create()
        {
          PeerIdentifyRequestPtr ret(new PeerIdentifyRequest);
          return ret;
        }

        //---------------------------------------------------------------------
        PeerIdentifyRequestPtr PeerIdentifyRequest::create(
                                                           ElementPtr root,
                                                           IMessageSourcePtr messageSource
                                                           )
        {
          PeerIdentifyRequestPtr ret(new PeerIdentifyRequest);

          try
          {
            ret->mID = IMessageHelper::getAttributeID(root);

            ElementPtr peerIdentityProofBundleEl = root->findFirstChildElementChecked("peerIdentityProofBundle");
            ElementPtr peerIdentityProofEl = peerIdentityProofBundleEl->findFirstChildElementChecked("peerIdentityProof");

            ElementPtr peerEl = peerIdentityProofEl->findFirstChildElementChecked("peer");
            ret->mPeerFilePublic = IPeerFilePublic::loadFromElement(peerEl);
            if (!ret->mPeerFilePublic) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] missing remote peer information")
              return PeerIdentifyRequestPtr();
            }

            LocationPtr locationSource = ILocationForMessages::convert(messageSource);
            if (!locationSource) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] message source could not be identified")
              return PeerIdentifyRequestPtr();
            }

            AccountPtr account = locationSource->forMessages().getAccount();
            if (!account) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] account gone so peer identify request cannot be verified")
              return PeerIdentifyRequestPtr();
            }

            ILocationPtr localLocation = ILocation::getForLocal(account);
            if (!account) {
              ZS_LOG_ERROR(Detail, "PeerIdentifyRequest [] local location is null")
              return PeerIdentifyRequestPtr();
            }

            Time expires = IMessageHelper::stringToTime(peerIdentityProofEl->findFirstChildElementChecked("expires")->getText());
            if (zsLib::now() > expires) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] request expired, expires=" + IMessageHelper::timeToString(expires) + ", now=" + IMessageHelper::timeToString(zsLib::now()))
              return PeerIdentifyRequestPtr();
            }

            ret->mFindSecret = IMessageHelper::getElementTextAndDecode(peerIdentityProofEl->findFirstChildElement("findSecret"));
            ret->mLocationInfo = MessageHelper::createLocation(peerIdentityProofEl->findFirstChildElement("location"), messageSource);

            PeerPtr remotePeer = IPeerForMessages::create(account, ret->mPeerFilePublic);
            if (!remotePeer) {
              ZS_LOG_ERROR(Detail, "PeerIdentifyRequest [] remote peer object could not be created")
              return PeerIdentifyRequestPtr();
            }

            LocationPtr location = Location::convert(ret->mLocationInfo.mLocation);

            if (!location) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] remote location object could not be created")
              return PeerIdentifyRequestPtr();
            }

            if (location->forMessages().getPeerURI() != remotePeer->forMessages().getPeerURI()) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] location peer does not match public peer file" +  ILocation::toDebugString(location) + IPeer::toDebugString(remotePeer))
              return PeerIdentifyRequestPtr();
            }

            PeerPtr signaturePeer = IPeerForMessages::getFromSignature(account, peerIdentityProofEl);
            if (!signaturePeer) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] signature peer is null")
              return PeerIdentifyRequestPtr();
            }

            if (signaturePeer->forMessages().getID() == remotePeer->forMessages().getID()) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] signature peer does not match identity peer, signature peer: " + IPeer::toDebugString(signaturePeer, false) + ", request peer: " + IPeer::toDebugString(remotePeer, false))
              return PeerIdentifyRequestPtr();
            }

            if (!signaturePeer->forMessages().verifySignature(peerIdentityProofEl)) {
              ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] signature does not validate")
              return PeerIdentifyRequestPtr();
            }

          } catch (CheckFailed &) {
            ZS_LOG_WARNING(Detail, "PeerIdentifyRequest [] failed to obtain information")
            return PeerIdentifyRequestPtr();
          }
          return ret;
        }

        //---------------------------------------------------------------------
        bool PeerIdentifyRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_LocationInfo:    return mLocationInfo.hasData();
            case AttributeType_FindSecret:      return (!mFindSecret.isEmpty());
            case AttributeType_PeerFilePublic:  return mPeerFilePublic;
            case AttributeType_PeerFiles:       return mPeerFiles;
            default:                            break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr PeerIdentifyRequest::encode()
        {
          if (!mPeerFiles) {
            ZS_LOG_ERROR(Detail, "PeerIdentifyRequest [] peer files was null")
            return DocumentPtr();
          }

          IPeerFilePrivatePtr peerFilePrivate = mPeerFiles->getPeerFilePrivate();
          if (!peerFilePrivate) {
            ZS_LOG_ERROR(Detail, "PeerIdentifyRequest [] peer file private was null")
            return DocumentPtr();
          }
          IPeerFilePublicPtr peerFilePublic = mPeerFiles->getPeerFilePublic();
          if (!peerFilePublic) {
            ZS_LOG_ERROR(Detail, "PeerIdentifyRequest [] peer file public was null")
            return DocumentPtr();
          }

          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          ElementPtr peerIdentityProofBundleEl = Element::create("peerIdentityProofBundle");
          ElementPtr peerIdentityProofEl = Element::create("peerIdentityProof");

          peerIdentityProofEl->adoptAsLastChild(IMessageHelper::createElementWithText("clientNonce", IHelper::randomString(32)));

          Time expires = zsLib::now() + Duration(Seconds(HOOKFLASH_STACK_MESSAGE_PEER_IDENTIFY_REQUEST_LIFETIME_IN_SECONDS));
          peerIdentityProofEl->adoptAsLastChild(IMessageHelper::createElementWithText("expires", IMessageHelper::timeToString(expires)));

          if (hasAttribute(AttributeType_FindSecret)) {
            peerIdentityProofEl->adoptAsLastChild(IMessageHelper::createElementWithText("findSecret", mFindSecret));
          }

          if (hasAttribute(AttributeType_LocationInfo)) {
            peerIdentityProofEl->adoptAsLastChild(MessageHelper::createElement(mLocationInfo));
          }

          peerIdentityProofEl->adoptAsLastChild(peerFilePublic->saveToElement());

          peerIdentityProofBundleEl->adoptAsLastChild(peerIdentityProofEl);
          peerFilePrivate->signElement(peerIdentityProofEl);
          root->adoptAsLastChild(peerIdentityProofBundleEl);
          return ret;
        }


      }
    }
  }
}
