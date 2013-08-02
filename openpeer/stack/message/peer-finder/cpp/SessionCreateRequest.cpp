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

#include <openpeer/stack/message/peer-finder/SessionCreateRequest.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>
#include <openpeer/stack/internal/stack_Location.h>
#include <openpeer/stack/internal/stack_Peer.h>
#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/IPeerFilePrivate.h>
#include <openpeer/stack/IHelper.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>

#define OPENPEER_STACK_MESSAGE_PEER_FINDER_SESSION_CREATE_REQUEST_EXPIRES_TIME_IN_SECONDS ((60*60)*24)

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
        using namespace stack::internal;
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        SessionCreateRequestPtr SessionCreateRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<SessionCreateRequest>(message);
        }

        //---------------------------------------------------------------------
        SessionCreateRequest::SessionCreateRequest()
        {
        }

        //---------------------------------------------------------------------
        SessionCreateRequestPtr SessionCreateRequest::create()
        {
          SessionCreateRequestPtr ret(new SessionCreateRequest);
          return ret;
        }

        //---------------------------------------------------------------------
        bool SessionCreateRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_FinderID:      return (!mFinderID.isEmpty());
            case AttributeType_LocationInfo:  return (mLocationInfo.hasData());
            case AttributeType_PeerFiles:     return mPeerFiles;
            default:                          break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr SessionCreateRequest::encode()
        {
          if (!mPeerFiles) {
            ZS_LOG_ERROR(Detail, "SessionCreateRequest [] peer files was null")
            return DocumentPtr();
          }

          IPeerFilePrivatePtr peerFilePrivate = mPeerFiles->getPeerFilePrivate();
          if (!peerFilePrivate) {
            ZS_LOG_ERROR(Detail, "SessionCreateRequest [] peer file private was null")
            return DocumentPtr();
          }
          IPeerFilePublicPtr peerFilePublic = mPeerFiles->getPeerFilePublic();
          if (!peerFilePublic) {
            ZS_LOG_ERROR(Detail, "SessionCreateRequest [] peer file public was null")
            return DocumentPtr();
          }

          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          ElementPtr sessionProofBundleEl = Element::create("sessionProofBundle");
          ElementPtr sessionProofEl = Element::create("sessionProof");

          if (hasAttribute(AttributeType_FinderID)) {
            sessionProofEl->adoptAsLastChild(IMessageHelper::createElementWithID("finder", mFinderID));
          }

          sessionProofEl->adoptAsLastChild(IMessageHelper::createElementWithText("nonce", IHelper::convertToHex(*IHelper::random(16))));

          Time expires = zsLib::now() + Seconds(OPENPEER_STACK_MESSAGE_PEER_FINDER_SESSION_CREATE_REQUEST_EXPIRES_TIME_IN_SECONDS);

          sessionProofEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("expires", IHelper::timeToString(expires)));

          if (hasAttribute(AttributeType_LocationInfo)) {
            sessionProofEl->adoptAsLastChild(MessageHelper::createElement(mLocationInfo));
          }

          sessionProofEl->adoptAsLastChild(peerFilePublic->saveToElement());

          sessionProofBundleEl->adoptAsLastChild(sessionProofEl);
          peerFilePrivate->signElement(sessionProofEl);

          root->adoptAsLastChild(sessionProofBundleEl);

          return ret;
        }
      }
    }
  }
}
