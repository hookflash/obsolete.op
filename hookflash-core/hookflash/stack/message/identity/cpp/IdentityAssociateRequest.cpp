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

#include <hookflash/stack/message/identity/IdentityAssociateRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/IPeerFilePublic.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#define HOOKFLASH_STACK_MESSAGE_IDENTITY_ASSOCIATE_EXPIRES_TIME_IN_SECONDS ((60*60)*24)

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace identity
      {
        using zsLib::Seconds;
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        IdentityAssociateRequestPtr IdentityAssociateRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<IdentityAssociateRequest>(message);
        }

        //---------------------------------------------------------------------
        IdentityAssociateRequest::IdentityAssociateRequest()
        {
        }

        //---------------------------------------------------------------------
        IdentityAssociateRequestPtr IdentityAssociateRequest::create()
        {
          IdentityAssociateRequestPtr ret(new IdentityAssociateRequest);
          return ret;
        }

        //---------------------------------------------------------------------
        bool IdentityAssociateRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_IdentityInfo:      return mIdentityInfo.hasData();
            case AttributeType_PeerFiles:         return mPeerFiles;
            default:                              break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr IdentityAssociateRequest::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          String clientNonce = IHelper::randomString(32);
          String expires = IMessageHelper::timeToString(zsLib::now() + Seconds(HOOKFLASH_STACK_MESSAGE_IDENTITY_ASSOCIATE_EXPIRES_TIME_IN_SECONDS));

          IdentityInfo info;
          info.mURI = mIdentityInfo.mURI;
          info.mProvider = mIdentityInfo.mProvider;
          info.mAccessToken = mIdentityInfo.mAccessToken;
          info.mAccessSecret = mIdentityInfo.mAccessSecret;

          info.mContactUserID = mIdentityInfo.mContactUserID;

          info.mSecret = mIdentityInfo.mSecret;
          info.mSecretSalt = mIdentityInfo.mSecretSalt;
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 1
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 2
          info.mPriority = 1;//mIdentityInfo.mPriority;
          info.mWeight = 3;//mIdentityInfo.mWeight;

          if (info.mAccessSecret.hasData()) {
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 3
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 4
            //info.mAccessSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKey(info.mAccessSecret), "identity-associate:" + info.mURI + ":" + clientNonce + ":" + expires + ":" + info.mAccessToken));
            info.mAccessSecretProof = info.mAccessSecret;
            info.mAccessSecret.clear();
          }
          info.mAccessSecretProofExpires = IMessageHelper::stringToTime(expires);

          IPeerFilePrivatePtr peerFilePrivate;
          IPeerFilePublicPtr peerFilePublic;

          if (mPeerFiles) {
            peerFilePrivate = mPeerFiles->getPeerFilePrivate();
            peerFilePublic = mPeerFiles->getPeerFilePublic();
            if ((!peerFilePrivate) ||
                (!peerFilePublic)) {
              mPeerFiles.reset();
            }
          }

          if (mPeerFiles) {
            info.mContact = peerFilePublic->getPeerURI();
            info.mContactFindSecret = peerFilePublic->getFindSecret();

            SecureByteBlockPtr peerSalt = peerFilePrivate->getSalt();
            String peerSaltAsBase64 = IHelper::convertToBase64(*peerSalt);
            info.mPrivatePeerFileSalt = peerSaltAsBase64;

            if ((info.mSecret.hasData()) &&
                (info.mSecretSalt.hasData()) &&
                (peerSalt)) {
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 5
#define MUST_REMOVE_SECURITY_HACK_ONLY_FOR_BB10_RELEASE_PURPOSES 6
//              SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKey(info.mSecret), "private-peer-file-secret:" + peerSaltAsBase64 + ":" + info.mSecretSalt);
//              SecureByteBlockPtr iv = IHelper::hash(peerSaltAsBase64 + ":" + info.mSecretSalt, IHelper::HashAlgorthm_MD5);
//
//              info.mPrivatePeerFileSecretEncrypted = IHelper::convertToBase64(*IHelper::encrypt(*key, *iv, *peerFilePrivate->getPassword(false)));
              
              info.mPrivatePeerFileSecretEncrypted = IHelper::convertToString((SecureByteBlock)*peerFilePrivate->getPassword(false).get());
            }
          }

          info.mSecret.clear();

          ElementPtr identityEl = MessageHelper::createElement(info);

          if (info.hasData()) {
            if (mPeerFiles) {
              ElementPtr identityAssociateProofBundleEl = Element::create("identityAssociateProofBundle");
              ElementPtr identityAssociateProofEl = Element::create("identityAssociateProof");
              identityAssociateProofBundleEl->adoptAsLastChild(identityAssociateProofEl);
              
              identityAssociateProofEl->adoptAsLastChild(IMessageHelper::createElementWithText("clientNonce", clientNonce));
              
              identityAssociateProofEl->adoptAsLastChild(identityEl);

              ElementPtr peerEl = peerFilePublic->saveToElement();
              identityAssociateProofEl->adoptAsLastChild(peerEl);

              root->adoptAsLastChild(identityAssociateProofBundleEl);

              peerFilePrivate->signElement(identityAssociateProofEl);
            } else {
              root->adoptAsLastChild(identityEl);
            }
          }

          return ret;
        }
      }
    }
  }
}
