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

#include <hookflash/stack/message/peer-contact/PeerContactIdentityAssociateRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/IPeerFilePublic.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#define HOOKFLASH_STACK_MESSAGE_PEER_CONTACT_ASSOCIATE_EXPIRES_TIME_IN_SECONDS ((60*60)*24)

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_contact
      {
        using zsLib::Seconds;
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        PeerContactIdentityAssociateRequestPtr PeerContactIdentityAssociateRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<PeerContactIdentityAssociateRequest>(message);
        }

        //---------------------------------------------------------------------
        PeerContactIdentityAssociateRequest::PeerContactIdentityAssociateRequest()
        {
        }

        //---------------------------------------------------------------------
        PeerContactIdentityAssociateRequestPtr PeerContactIdentityAssociateRequest::create()
        {
          PeerContactIdentityAssociateRequestPtr ret(new PeerContactIdentityAssociateRequest);
          return ret;
        }

        //---------------------------------------------------------------------
        bool PeerContactIdentityAssociateRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_ContactAccessToken:          return !mContactAccessToken.isEmpty();
            case AttributeType_ContactAccessSecret:         return !mContactAccessSecret.isEmpty();
            case AttributeType_Identities:                  return (mIdentities.size() > 0);
            case AttributeType_PeerFiles:                   return mPeerFiles;
            default:                                        break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr PeerContactIdentityAssociateRequest::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          String clientNonce = IHelper::randomString(32);
          String expires = IMessageHelper::timeToString(zsLib::now() + Seconds(HOOKFLASH_STACK_MESSAGE_PEER_CONTACT_ASSOCIATE_EXPIRES_TIME_IN_SECONDS));

          String finalAccessProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKey(mContactAccessSecret), "peer-contact-identity-associate:" + clientNonce + ":" + expires + ":" + mContactAccessToken));

          root->adoptAsLastChild(IMessageHelper::createElementWithText("clientNonce", clientNonce));
          if (hasAttribute(AttributeType_ContactAccessToken)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("contactAccessToken", mContactAccessToken));
          }
          if (hasAttribute(AttributeType_ContactAccessSecret)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("contactAccessSecretProof", finalAccessProof));
          }
          root->adoptAsLastChild(IMessageHelper::createElementWithNumber("contactAccessSecretProofExpires", expires));

          ElementPtr identitiesEl = Element::create("identities");

          IPeerFilePrivatePtr peerFilePrivate;
          IPeerFilePublicPtr peerFilePublic;
          if (mPeerFiles) {
            peerFilePrivate = mPeerFiles->getPeerFilePrivate();
            peerFilePublic = mPeerFiles->getPeerFilePublic();
          }

          for (IdentityInfoList::iterator iter = mIdentities.begin(); iter != mIdentities.end(); ++iter)
          {
            IdentityInfo &actualInfo = (*iter);
            if (actualInfo.hasData()) {
              IdentityInfo info;
              info.mDisposition = actualInfo.mDisposition;
              info.mURI = actualInfo.mURI;
              info.mProvider = actualInfo.mProvider;
              info.mReloginAccessKey = actualInfo.mReloginAccessKey;
              info.mReloginAccessKeyEncrypted = actualInfo.mReloginAccessKeyEncrypted;
              info.mSecretSalt = actualInfo.mSecretSalt;

              if ((info.mReloginAccessKey.hasData()) &&
                  (info.mSecretSalt.hasData()) &&
                  (info.mURI) &&
                  (peerFilePrivate) &&
                  (peerFilePublic)) {

                SecureByteBlockPtr peerSalt = peerFilePrivate->getSalt();
                if (peerSalt) {
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 1
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 2
//                  String peerSaltAsBase64 = IHelper::convertToBase64(*peerSalt);
//                  SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKey((const char *)((const BYTE *)(*peerFilePrivate->getPassword(true)))), "relogin:" + IHelper::convertToHex(*IHelper::hash(info.mURI)), IHelper::HashAlgorthm_SHA256);
//                  SecureByteBlockPtr iv = IHelper::hash(info.mSecretSalt + ":" + peerSaltAsBase64, IHelper::HashAlgorthm_MD5);
//
//                  info.mReloginAccessKeyEncrypted = IHelper::convertToBase64(*IHelper::encrypt(*key, *iv, info.mReloginAccessKey));
                  info.mReloginAccessKeyEncrypted = info.mReloginAccessKey;
                }
              }

              info.mSecretSalt.clear();
              info.mReloginAccessKey.clear();

              identitiesEl->adoptAsLastChild(MessageHelper::createElement(info));
            }
          }

          if (identitiesEl->hasChildren()) {
            root->adoptAsLastChild(identitiesEl);
          }
          return ret;
        }
      }
    }
  }
}
