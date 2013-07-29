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

#include <openpeer/stack/message/peer-finder/PeerLocationFindRequest.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>
#include <openpeer/stack/internal/stack_Location.h>
#include <openpeer/stack/internal/stack_Peer.h>
#include <openpeer/stack/internal/stack_Account.h>

#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/IPeerFilePrivate.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>


#define OPENPEER_STACK_MESSAGE_PEER_LOCATION_FIND_REQUEST_LIFETIME_IN_SECONDS ((60*60)*24)

namespace openpeer { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(openpeer_stack_message) } } }

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      using services::IHelper;

      namespace peer_finder
      {
        using zsLib::Stringize;
        using zsLib::Seconds;
        typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;
        using namespace stack::internal;

        //---------------------------------------------------------------------
        PeerLocationFindRequestPtr PeerLocationFindRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<PeerLocationFindRequest>(message);
        }

        //---------------------------------------------------------------------
        PeerLocationFindRequest::PeerLocationFindRequest()
        {
        }

        //---------------------------------------------------------------------
        PeerLocationFindRequestPtr PeerLocationFindRequest::create(
                                                                   ElementPtr root,
                                                                   IMessageSourcePtr messageSource
                                                                   )
        {
          PeerLocationFindRequestPtr ret(new PeerLocationFindRequest);
          IMessageHelper::fill(*ret, root, messageSource);

          LocationPtr messageLocation = ILocationForMessages::convert(messageSource);
          if (!messageLocation) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] message source was not a known location")
            return PeerLocationFindRequestPtr();
          }

          AccountPtr account = messageLocation->forMessages().getAccount();
          if (!account) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] account object is gone")
            return PeerLocationFindRequestPtr();
          }

          IPeerFilesPtr peerFiles = account->forMessages().getPeerFiles();
          if (!peerFiles) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] peer files not found in account")
            return PeerLocationFindRequestPtr();
          }

          IPeerFilePrivatePtr peerFilePrivate = peerFiles->getPeerFilePrivate();
          if (!peerFilePrivate) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] peer file private was null")
            return PeerLocationFindRequestPtr();
          }
          IPeerFilePublicPtr peerFilePublic = peerFiles->getPeerFilePublic();
          if (!peerFilePublic) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] peer file public was null")
            return PeerLocationFindRequestPtr();
          }

          LocationPtr localLocation = ILocationForMessages::getForLocal(account);
          if (!localLocation) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] could not obtain local location")
            return PeerLocationFindRequestPtr();
          }

          try {
            ElementPtr findProofEl = root->findFirstChildElementChecked("findProofBundle")->findFirstChildElementChecked("findProof");

            String clientNonce = findProofEl->findFirstChildElementChecked("nonce")->getText();

            String peerURI = findProofEl->findFirstChildElementChecked("find")->getText();

            if (peerURI != localLocation->forMessages().getPeerURI()) {
              ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] find was not intended for this peer, find peer URI=" + peerURI + ILocation::toDebugString(localLocation))
              return PeerLocationFindRequestPtr();
            }

            String findSecretProof = findProofEl->findFirstChildElementChecked("findSecretProof")->getText();
            Time expires = IHelper::stringToTime(findProofEl->findFirstChildElementChecked("findSecretProofExpires")->getText());
            String peerSecretEncrypted = findProofEl->findFirstChildElementChecked("peerSecretEncrypted")->getText();

            String findSecret = peerFilePublic->getFindSecret();
            String calculatedFindSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(findSecret), "proof:" + clientNonce + ":" + IHelper::timeToString(expires)));

            if (calculatedFindSecretProof != findSecretProof) {
              ZS_LOG_WARNING(Detail, "PeerLocationFindRequest [] calculated find secret proof did not match request, calculated=" + calculatedFindSecretProof + ", request=" + findSecretProof)
              return PeerLocationFindRequestPtr();
            }

            if (zsLib::now() > expires) {
              ZS_LOG_WARNING(Detail, "PeerLocationFindRequest [] request expired, expires=" + IHelper::timeToString(expires) + ", now=" + IHelper::timeToString(zsLib::now()))
              return PeerLocationFindRequestPtr();
            }

            ret->mPeerSecret = peerFilePrivate->decrypt(*IHelper::convertFromBase64(peerSecretEncrypted));
            if (!ret->mPeerSecret) {
              ZS_LOG_WARNING(Detail, "PeerLocationFindRequest [] peer secret failed to decrypt")
              return PeerLocationFindRequestPtr();
            }
            ZS_LOG_TRACE(String("decrypted peer secret") + ", secret=" + IHelper::convertToHex(*ret->mPeerSecret))

            ElementPtr locationEl = findProofEl->findFirstChildElement("location");
            if (locationEl) {
              ret->mLocationInfo = internal::MessageHelper::createLocation(locationEl, messageSource, (ret->mPeerSecret).get());
            }

            LocationPtr location = Location::convert(ret->mLocationInfo.mLocation);

            if (!location) {
              ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] missing location information in find request")
              return PeerLocationFindRequestPtr();
            }

            if (!location->forMessages().getPeer()) {
              ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] missing location peer information in find request")
              return PeerLocationFindRequestPtr();
            }

            PeerPtr peer = IPeerForMessages::getFromSignature(account, findProofEl);
            if (!peer) {
              ZS_LOG_WARNING(Detail, "PeerLocationFindRequest [] peer object failed to create from signature")
              return PeerLocationFindRequestPtr();
            }

            if (location->forMessages().getPeerURI() != peer->forMessages().getPeerURI()) {
              ZS_LOG_WARNING(Detail, "PeerLocationFindRequest [] location peer is not same as signature peer")
              return PeerLocationFindRequestPtr();
            }

            ElementPtr signatureEl;
            IHelper::getSignatureInfo(findProofEl, &signatureEl);

            if (signatureEl) {
              ret->mRequestfindProofBundleDigestValue = signatureEl->findFirstChildElementChecked("digestValue")->getTextDecoded();
            }

            if (peer->forMessages().getPeerFilePublic()) {
              // know the peer file public so this should verify the signature
              if (!peer->forMessages().verifySignature(findProofEl)) {
                ZS_LOG_WARNING(Detail, "PeerLocationFindRequest [] signatuer on request did not verify")
                return PeerLocationFindRequestPtr();
              }
            }

            ElementPtr routesEl = root->findFirstChildElement("routes");
            if (routesEl) {
              RouteList routeLst;
              ElementPtr routeEl = routesEl->findFirstChildElement("route");
              while (routeEl)
              {
                String id = IMessageHelper::getAttributeID(routeEl);
                routeLst.push_back(id);

                routeEl = routeEl->getNextSiblingElement();
              }

              if (routeLst.size() > 0)
                ret->mRoutes = routeLst;
            }

            ElementPtr excludeEl = root->findFirstChildElement("exclude");
            if (excludeEl) {
              ElementPtr locationsEl = excludeEl->findFirstChildElement("locations");
              if (locationsEl) {
                ExcludedLocationList excludeList;
                ElementPtr locationEl = locationsEl->findFirstChildElement("location");
                while (locationEl)
                {
                  String id = IMessageHelper::getAttributeID(locationEl);
                  excludeList.push_back(id);

                  locationEl = locationEl->findNextSiblingElement("location");
                }

                if (excludeList.size() > 0)
                  ret->mExcludedLocations = excludeList;
              }
            }
          } catch(CheckFailed &) {
            ZS_LOG_WARNING(Detail, "PeerLocationFindRequest [] expected element is missing")
          }

          return ret;
        }

        //---------------------------------------------------------------------
        bool PeerLocationFindRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_RequestfindProofBundleDigestValue: return mRequestfindProofBundleDigestValue.hasData();
            case AttributeType_FindPeer:                          return mFindPeer;
            case AttributeType_PeerSecret:                        return mPeerSecret;
            case AttributeType_LocationInfo:                      return mLocationInfo.hasData();
            case AttributeType_ExcludedLocations:                 return (mExcludedLocations.size() > 0);
            case AttributeType_Routes:                            return (mRoutes.size() != 0);
            case AttributeType_PeerFiles:                         return mPeerFiles;
            default:                                              break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr PeerLocationFindRequest::encode()
        {
          if (!mPeerFiles) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] peer files was null")
            return DocumentPtr();
          }

          IPeerFilePrivatePtr peerFilePrivate = mPeerFiles->getPeerFilePrivate();
          if (!peerFilePrivate) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] peer file private was null")
            return DocumentPtr();
          }
          IPeerFilePublicPtr peerFilePublic = mPeerFiles->getPeerFilePublic();
          if (!peerFilePublic) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindRequest [] peer file public was null")
            return DocumentPtr();
          }

          String clientNonce = IHelper::convertToHex(*IHelper::random(16));

          Time expires = zsLib::now() + Duration(Seconds(OPENPEER_STACK_MESSAGE_PEER_LOCATION_FIND_REQUEST_LIFETIME_IN_SECONDS));

          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          ElementPtr findProofBundleEl = Element::create("findProofBundle");
          ElementPtr findProofEl = Element::create("findProof");

          findProofEl->adoptAsLastChild(IMessageHelper::createElementWithText("nonce", clientNonce));

          if (mFindPeer) {
            findProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("find", mFindPeer->getPeerURI()));
          }

          if (mFindPeer) {
            IPeerFilePublicPtr remotePeerFilePublic = mFindPeer->getPeerFilePublic();
            if (remotePeerFilePublic) {
              String findSecret = remotePeerFilePublic->getFindSecret();
              if (findSecret.length() > 0) {
                String findSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(findSecret), "proof:" + clientNonce + ":" + IHelper::timeToString(expires)));
                findProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("findSecretProof", findSecretProof));
                findProofEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("findSecretProofExpires", IHelper::timeToString(expires)));
              }

              if (hasAttribute(AttributeType_PeerSecret)) {

                ZS_LOG_TRACE(String("encrypting peer secret") + ", secret=" + IHelper::convertToHex(*mPeerSecret))

                String peerSecret = IHelper::convertToBase64(*remotePeerFilePublic->encrypt(*mPeerSecret));
                findProofEl->adoptAsLastChild(IMessageHelper::createElementWithText("peerSecretEncrypted", peerSecret));
              }
            }
          }

          if (hasAttribute(AttributeType_LocationInfo))
          {
            ElementPtr locationEl = internal::MessageHelper::createElement(mLocationInfo, mPeerSecret.get());
            findProofEl->adoptAsLastChild(locationEl);
          }

          findProofBundleEl->adoptAsLastChild(findProofEl);
          peerFilePrivate->signElement(findProofEl);
          root->adoptAsLastChild(findProofBundleEl);

          ElementPtr signatureEl;
          IHelper::getSignatureInfo(findProofEl, &signatureEl);

          if (signatureEl) {
            mRequestfindProofBundleDigestValue = signatureEl->findFirstChildElementChecked("digestValue")->getTextDecoded();
          }

          if (hasAttribute(AttributeType_ExcludedLocations))
          {
            ElementPtr excludeEl = IMessageHelper::createElement("exclude");
            ElementPtr locationsEl = IMessageHelper::createElement("locations");
            excludeEl->adoptAsLastChild(locationsEl);
            root->adoptAsLastChild(excludeEl);

            for (ExcludedLocationList::const_iterator it = mExcludedLocations.begin(); it != mExcludedLocations.end(); ++it)
            {
              const String &location = (*it);
              locationsEl->adoptAsLastChild(IMessageHelper::createElementWithID("location", location));
            }
          }

          if (hasAttribute(AttributeType_Routes))
          {
            ElementPtr routesEl = IMessageHelper::createElement("routes");
            root->adoptAsLastChild(routesEl);

            for(RouteList::const_iterator it = mRoutes.begin(); it != mRoutes.end(); ++it)
            {
              const String &route = (*it);
              routesEl->adoptAsLastChild(IMessageHelper::createElementWithID("route", route));
            }
          }

          return ret;
        }

        //---------------------------------------------------------------------
        PeerLocationFindRequestPtr PeerLocationFindRequest::create()
        {
          PeerLocationFindRequestPtr ret(new PeerLocationFindRequest);
          return ret;
        }

      }
    }
  }
}
