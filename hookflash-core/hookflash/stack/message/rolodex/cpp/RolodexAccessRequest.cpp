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

#include <hookflash/stack/message/rolodex/RolodexAccessRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/internal/stack_Stack.h>
#include <hookflash/stack/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#define HOOKFLASH_STACK_MESSAGE_ROLODEX_ACCESS_REQUEST_EXPIRES_TIME_IN_SECONDS ((60*60)*24)

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace rolodex
      {
        using zsLib::Seconds;
        using internal::MessageHelper;
        using stack::internal::IStackForInternal;

        //---------------------------------------------------------------------
        RolodexAccessRequestPtr RolodexAccessRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<RolodexAccessRequest>(message);
        }

        //---------------------------------------------------------------------
        RolodexAccessRequest::RolodexAccessRequest()
        {
        }

        //---------------------------------------------------------------------
        RolodexAccessRequestPtr RolodexAccessRequest::create()
        {
          RolodexAccessRequestPtr ret(new RolodexAccessRequest);
          return ret;
        }

        //---------------------------------------------------------------------
        bool RolodexAccessRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_IdentityInfo:      return mIdentityInfo.hasData();
            case AttributeType_RolodexInfo:       return mRolodexInfo.hasData();
            case AttributeType_GrantInfo:         return mGrantInfo.hasData();
            default:                              break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr RolodexAccessRequest::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr rootEl = ret->getFirstChildElement();

          String clientNonce = IHelper::randomString(32);
          IdentityInfo identityInfo;

          identityInfo.mURI = mIdentityInfo.mURI;
          identityInfo.mProvider = mIdentityInfo.mProvider;

          identityInfo.mAccessToken = mIdentityInfo.mAccessToken;
          if (mIdentityInfo.mAccessSecret.hasData()) {
            identityInfo.mAccessSecretProofExpires = zsLib::now() + Seconds(HOOKFLASH_STACK_MESSAGE_ROLODEX_ACCESS_REQUEST_EXPIRES_TIME_IN_SECONDS);
            identityInfo.mAccessSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKey(mIdentityInfo.mAccessSecret), "identity-access-validate:" + identityInfo.mURI + ":" + clientNonce + ":" + IMessageHelper::timeToString(identityInfo.mAccessSecretProofExpires) + ":" + identityInfo.mAccessToken + ":lockbox-access"));
          }

          rootEl->adoptAsLastChild(IMessageHelper::createElementWithText("clientNonce", clientNonce));
          if (identityInfo.hasData()) {
            rootEl->adoptAsLastChild(MessageHelper::createElement(identityInfo));
          }

          RolodexInfo rolodexInfo;
          rolodexInfo.mServerToken = mRolodexInfo.mServerToken;
          rolodexInfo.mVersion = mRolodexInfo.mVersion;
          rolodexInfo.mRefreshFlag = mRolodexInfo.mRefreshFlag;

          if (rolodexInfo.hasData()) {
            rootEl->adoptAsLastChild(MessageHelper::createElement(rolodexInfo));
          }

          GrantInfo grantInfo;
          grantInfo.mID = mGrantInfo.mID;
          grantInfo.mDomain = mGrantInfo.mDomain;

          if (grantInfo.mSecret.hasData()) {
            grantInfo.mExpires = zsLib::now() + Seconds(HOOKFLASH_STACK_MESSAGE_ROLODEX_ACCESS_REQUEST_EXPIRES_TIME_IN_SECONDS);
            grantInfo.mSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hash(grantInfo.mSecret), "namespace-grant-validate:" + grantInfo.mID + ":" + clientNonce + ":" + IMessageHelper::timeToString(grantInfo.mExpires) + ":lockbox-access"));
          }

          if (grantInfo.hasData()) {
            rootEl->adoptAsLastChild(MessageHelper::createElement(grantInfo));
          }

          return ret;
        }
      }
    }
  }
}
