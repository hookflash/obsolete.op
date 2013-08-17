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

#include <openpeer/stack/message/identity/IdentityLookupUpdateRequest.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>
#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePrivate.h>
#include <openpeer/stack/IPeerFilePublic.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#define OPENPEER_STACK_MESSAGE_IDENTITY_ACCESS_LOCKBOX_UPDATE_EXPIRES_TIME_IN_SECONDS ((60*60)*24)
#define OPENPEER_STACK_MESSAGE_IDENTITY_ACCESS_CONTACT_PROOF_EXPIRES_IN_HOURS ((24)*365*2)

namespace openpeer { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(openpeer_stack_message) } } }

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      using services::IHelper;

      namespace identity
      {
        using zsLib::Seconds;
        using zsLib::Hours;
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        IdentityLookupUpdateRequestPtr IdentityLookupUpdateRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<IdentityLookupUpdateRequest>(message);
        }

        //---------------------------------------------------------------------
        IdentityLookupUpdateRequest::IdentityLookupUpdateRequest()
        {
        }

        //---------------------------------------------------------------------
        IdentityLookupUpdateRequestPtr IdentityLookupUpdateRequest::create()
        {
          IdentityLookupUpdateRequestPtr ret(new IdentityLookupUpdateRequest);
          return ret;
        }

        //---------------------------------------------------------------------
        bool IdentityLookupUpdateRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_IdentityInfo:      return mIdentityInfo.hasData();
            default:                              break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr IdentityLookupUpdateRequest::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          IPeerFilePrivatePtr peerFilePrivate;
          IPeerFilePublicPtr peerFilePublic;

          if (mPeerFiles) {
            peerFilePrivate = mPeerFiles->getPeerFilePrivate();
            if (!peerFilePrivate) {
              ZS_LOG_ERROR(Detail, "IdentityLookupUpdateRequest [] peer file private was null")
              return DocumentPtr();
            }
            peerFilePublic = mPeerFiles->getPeerFilePublic();
            if (!peerFilePublic) {
              ZS_LOG_ERROR(Detail, "IdentityLookupUpdateRequest [] peer file public was null")
              return DocumentPtr();
            }
          }

          String clientNonce = IHelper::randomString(32);

          LockboxInfo lockboxInfo;

          lockboxInfo.mDomain = mLockboxInfo.mDomain;
          lockboxInfo.mAccessToken = mLockboxInfo.mAccessToken;
          if (mLockboxInfo.mAccessSecret.hasData()) {
            lockboxInfo.mAccessSecretProofExpires = zsLib::now() + Seconds(OPENPEER_STACK_MESSAGE_IDENTITY_ACCESS_LOCKBOX_UPDATE_EXPIRES_TIME_IN_SECONDS);
            lockboxInfo.mAccessSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(mLockboxInfo.mAccessSecret), "lockbox-access-validate:" + clientNonce + ":" + IHelper::timeToString(lockboxInfo.mAccessSecretProofExpires) + ":" + lockboxInfo.mAccessToken + ":identity-lookup-update"));
          }

          IdentityInfo identityInfo;

          identityInfo.mURI = mIdentityInfo.mURI;
          identityInfo.mProvider = mIdentityInfo.mProvider;

          identityInfo.mAccessToken = mIdentityInfo.mAccessToken;
          if (mIdentityInfo.mAccessSecret.hasData()) {
            identityInfo.mAccessSecretProofExpires = zsLib::now() + Seconds(OPENPEER_STACK_MESSAGE_IDENTITY_ACCESS_LOCKBOX_UPDATE_EXPIRES_TIME_IN_SECONDS);
            identityInfo.mAccessSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(mIdentityInfo.mAccessSecret), "identity-access-validate:" + identityInfo.mURI + ":" + clientNonce + ":" + IHelper::timeToString(identityInfo.mAccessSecretProofExpires) + ":" + identityInfo.mAccessToken + ":identity-lookup-update"));
          }

          identityInfo.mStableID = mIdentityInfo.mStableID;
          identityInfo.mPeerFilePublic = mIdentityInfo.mPeerFilePublic;
          if (!identityInfo.mPeerFilePublic) {
            identityInfo.mPeerFilePublic = peerFilePublic;
          }

          identityInfo.mPriority = mIdentityInfo.mPriority;
          identityInfo.mWeight = mIdentityInfo.mWeight;

          // all needed to create contact proof bundle
          if ((identityInfo.mURI.hasData()) &&
              (identityInfo.mStableID.hasData()) &&
              (identityInfo.mPeerFilePublic) &&
              (peerFilePrivate)) {

            ElementPtr contactProofEl = Element::create("contactProof");

            contactProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("stableID", identityInfo.mStableID));
            contactProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("contact", identityInfo.mPeerFilePublic->getPeerURI()));
            contactProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("uri", identityInfo.mURI));
            Time created = zsLib::now();
            Time expires = created + Hours(OPENPEER_STACK_MESSAGE_IDENTITY_ACCESS_CONTACT_PROOF_EXPIRES_IN_HOURS);

            contactProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("created", IHelper::timeToString(created)));
            contactProofEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("expires", IHelper::timeToString(expires)));

            identityInfo.mContactProofBundle = Element::create("contactProofBundle");
            identityInfo.mContactProofBundle->adoptAsLastChild(contactProofEl);

            peerFilePrivate->signElement(contactProofEl);
          }


          root->adoptAsLastChild(IMessageHelper::createElementWithText("nonce", clientNonce));

          if (lockboxInfo.hasData()) {
            root->adoptAsLastChild(MessageHelper::createElement(lockboxInfo));
          }

          if (identityInfo.hasData()) {
            ElementPtr identityEl = MessageHelper::createElement(identityInfo);
            root->adoptAsLastChild(identityEl);
          }

          return ret;
        }
      }
    }
  }
}
