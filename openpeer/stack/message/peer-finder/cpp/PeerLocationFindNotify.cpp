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

#include <openpeer/stack/message/peer-finder/PeerLocationFindNotify.h>
#include <openpeer/stack/message/peer-finder/PeerLocationFindRequest.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>
#include <openpeer/stack/internal/stack_Location.h>
#include <openpeer/stack/internal/stack_Peer.h>
#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/IPeerFilePrivate.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace openpeer { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(openpeer_stack_message) } } }

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace peer_finder
      {
        typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

        using message::internal::MessageHelper;

        using namespace stack::internal;

        //---------------------------------------------------------------------
        PeerLocationFindNotifyPtr PeerLocationFindNotify::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<PeerLocationFindNotify>(message);
        }

        //---------------------------------------------------------------------
        PeerLocationFindNotify::PeerLocationFindNotify()
        {
        }

        //---------------------------------------------------------------------
        PeerLocationFindNotifyPtr PeerLocationFindNotify::create(
                                                                 ElementPtr root,
                                                                 IMessageSourcePtr messageSource
                                                                 )
        {
          PeerLocationFindNotifyPtr ret(new PeerLocationFindNotify);
          IMessageHelper::fill(*ret, root, messageSource);

          try {

            ElementPtr findProofEl = root->findFirstChildElementChecked("findProofBundle")->findFirstChildElementChecked("findProof");

            ret->mRequestfindProofBundleDigestValue = IMessageHelper::getElementTextAndDecode(findProofEl->findFirstChildElementChecked("requestFindProofBundleDigestValue"));

            ret->mContext = IMessageHelper::getElementTextAndDecode(findProofEl->findFirstChildElementChecked("context"));
            ret->mPeerSecret = IMessageHelper::getElementTextAndDecode(findProofEl->findFirstChildElementChecked("peerSecret"));

            ret->mICEUsernameFrag = IMessageHelper::getElementTextAndDecode(findProofEl->findFirstChildElementChecked("iceUsernameFrag"));
            ret->mICEPassword = IMessageHelper::getElementTextAndDecode(findProofEl->findFirstChildElementChecked("icePassword"));

            ElementPtr locationEl = findProofEl->findFirstChildElement("location");
            if (locationEl) {
              ret->mLocationInfo = internal::MessageHelper::createLocation(locationEl, messageSource, ret->mPeerSecret.hasData() ? ret->mPeerSecret.c_str() : NULL);
            }

            if (!ret->mLocationInfo.mLocation) {
              ZS_LOG_ERROR(Detail, "PeerLocationFindNotify [] missing location information in find request")
              return PeerLocationFindNotifyPtr();
            }

            PeerPtr peer = Location::convert(ret->mLocationInfo.mLocation)->forMessages().getPeer();

            if (!peer) {
              ZS_LOG_WARNING(Detail, "PeerLocationFindNotify [] expected element is missing")
              return PeerLocationFindNotifyPtr();
            }

            if (!peer->forMessages().verifySignature(findProofEl)) {
              ZS_LOG_WARNING(Detail, "PeerLocationFindNotify [] could not validate signature of find proof request")
              return PeerLocationFindNotifyPtr();
            }

#define WARNING_REMOVE_ROUTES 1
#define WARNING_REMOVE_ROUTES 2

            ElementPtr routes = root->findFirstChildElement("routes");
            if (routes)
            {
              RouteList routeLst;
              ElementPtr route = routes->findFirstChildElement("route");
              while (route)
              {
                String id = IMessageHelper::getAttributeID(route);
                routeLst.push_back(id);

                route = route->getNextSiblingElement();
              }

              if (routeLst.size() > 0)
                ret->mRoutes = routeLst;
            }
          } catch(CheckFailed &) {
            ZS_LOG_WARNING(Detail, "PeerLocationFindNotify [] expected element is missing")
            return PeerLocationFindNotifyPtr();
          }

          return ret;
        }

        //---------------------------------------------------------------------
        PeerLocationFindNotifyPtr PeerLocationFindNotify::create(PeerLocationFindRequestPtr request)
        {
          PeerLocationFindNotifyPtr ret(new PeerLocationFindNotify);

          ret->mDomain = request->domain();
          ret->mID = request->messageID();

          if (request->hasAttribute(PeerLocationFindRequest::AttributeType_PeerSecret)) {
            ret->mPeerSecret = request->peerSecret();
          }
          if (request->hasAttribute(PeerLocationFindRequest::AttributeType_RequestfindProofBundleDigestValue)) {
            ret->mRequestfindProofBundleDigestValue = request->mRequestfindProofBundleDigestValue;
          }
          if (request->hasAttribute(PeerLocationFindRequest::AttributeType_Routes)) {
            ret->mRoutes = request->mRoutes;
          }

          return ret;
        }

        //---------------------------------------------------------------------
        bool PeerLocationFindNotify::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_RequestfindProofBundleDigestValue: return mRequestfindProofBundleDigestValue.hasData();
            case AttributeType_Context:                           return mContext.hasData();
            case AttributeType_PeerSecret:                        return mPeerSecret.hasData();
            case AttributeType_LocationInfo:                      return mLocationInfo.hasData();
            case AttributeType_Routes:                            return mRoutes.size() > 0;
            case AttributeType_PeerFiles:                         return mPeerFiles;
            default:
              break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr PeerLocationFindNotify::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          if (!mPeerFiles) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindNotify [] peer files was null")
            return DocumentPtr();
          }

          IPeerFilePrivatePtr peerFilePrivate = mPeerFiles->getPeerFilePrivate();
          if (!peerFilePrivate) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindNotify [] peer file private was null")
            return DocumentPtr();
          }
          IPeerFilePublicPtr peerFilePublic = mPeerFiles->getPeerFilePublic();
          if (!peerFilePublic) {
            ZS_LOG_ERROR(Detail, "PeerLocationFindNotify [] peer file public was null")
            return DocumentPtr();
          }

          ElementPtr findProofBundleEl = Element::create("findProofBundle");
          ElementPtr findProofEl = Element::create("findProof");

          if (hasAttribute(AttributeType_RequestfindProofBundleDigestValue))
          {
            findProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("requestFindProofBundleDigestValue", mRequestfindProofBundleDigestValue));
          }

          if (hasAttribute(AttributeType_Context)) {
            findProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("context", mContext));
          }

          if (hasAttribute(AttributeType_PeerSecret)) {
            findProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("peerSecret", mPeerSecret));
          }

          if (mICEUsernameFrag.hasData()) {
            findProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("iceUsernameFrag", mICEUsernameFrag));
          }

          if (mICEPassword.hasData()) {
            findProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("icePassword", mICEPassword));
          }

          if (hasAttribute(AttributeType_LocationInfo)) {
            findProofEl->adoptAsLastChild(MessageHelper::createElement(mLocationInfo, mPeerSecret.hasData() ? mPeerSecret.c_str() : NULL));
          }

          findProofBundleEl->adoptAsLastChild(findProofEl);
          peerFilePrivate->signElement(findProofEl);
          root->adoptAsLastChild(findProofBundleEl);

#define WARNING_REMOVE_ROUTES 1
#define WARNING_REMOVE_ROUTES 2

          if (hasAttribute(AttributeType_Routes))
          {
            ElementPtr routesEl = IMessageHelper::createElement("routes");
            root->adoptAsLastChild(routesEl);

            for (RouteList::const_iterator it = mRoutes.begin(); it != mRoutes.end(); ++it)
            {
              const String &routeID = (*it);
              routesEl->adoptAsLastChild(IMessageHelper::createElementWithID("route", routeID));
            }
          }

          return ret;
        }

      }
    }
  }
}
