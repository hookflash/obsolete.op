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

#include <hookflash/provisioning/message/internal/provisioning_message_OAuthLoginWebpageResult.h>
#include <hookflash/stack/message/IMessageHelper.h>

#include <zsLib/Numeric.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      using zsLib::Numeric;

      typedef zsLib::String String;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef stack::message::IMessageHelper IMessageHelper;

      OAuthLoginWebpageResultPtr OAuthLoginWebpageResult::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<OAuthLoginWebpageResult>(message);
      }

      OAuthLoginWebpageResult::OAuthLoginWebpageResult() :
        mCreated(-1),
        mProviderType(provisioning::IAccount::IdentityType_None)
      {
      }

      bool OAuthLoginWebpageResult::hasAttribute(OAuthLoginWebpageResult::AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_Created:                             return mCreated >= 0;

          case AttributeType_UserID:                              return !mUserID.isEmpty();
          case AttributeType_AccountSalt:                         return !mAccountSalt.isEmpty();
          case AttributeType_AccessKey:                           return !mAccessKey.isEmpty();
          case AttributeType_AccessSecret:                        return !mAccessSecret.isEmpty();

          case AttributeType_LastProfileUpdateTimestamp:          return Time() != mLastProfileUpdateTimestamp;

          case AttributeType_Name:                                return !mName.isEmpty();

          case AttributeType_NetworkURI:                          return !mNetworkURI.isEmpty();
          case AttributeType_TURNServer:                          return !mTURNServer.isEmpty();
          case AttributeType_TURNUsername:                        return !mTURNUsername.isEmpty();
          case AttributeType_TURNPassword:                        return !mTURNPassword.isEmpty();
          case AttributeType_STUNServer:                          return !mSTUNServer.isEmpty();

          case AttributeType_ProviderType:                        return provisioning::IAccount::IdentityType_None != mProviderType;
          case AttributeType_ProviderUniqueID:                    return !mProviderUniqueID.isEmpty();
          case AttributeType_ProviderOAuthAccessToken:            return !mProviderOAuthAccessToken.isEmpty();
          case AttributeType_ProviderEncryptedOAuthAccessSecret:  return !mProviderEncryptedOAuthAccessSecret.isEmpty();

          default:                                          break;
        }
        return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
      }

      namespace internal
      {
        OAuthLoginWebpageResultPtr OAuthLoginWebpageResult::create(ElementPtr root)
        {
          OAuthLoginWebpageResultPtr ret(new message::OAuthLoginWebpageResult);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          String createdStr = IMessageHelper::getChildElementText(root, "created");
          if (!createdStr.isEmpty()) {
            try {
              bool val = Numeric<bool>(createdStr);
              ret->mCreated = (val ? 1 : 0);
            } catch (Numeric<bool>::ValueOutOfRange &) {
              ret->mCreated = -1;
            }
          }
          ret->mUserID = IMessageHelper::getChildElementText(root, "userID");
          ret->mAccountSalt = IMessageHelper::getChildElementText(root, "accountSalt");
          ret->mAccessKey = IMessageHelper::getChildElementText(root, "accessKey");
          ret->mAccessSecret = IMessageHelper::getChildElementText(root, "accessSecret");

          ret->mLastProfileUpdateTimestamp = IMessageHelper::stringToTime(IMessageHelper::getChildElementText(root, "lastProfileUpdateTimestamp"));

          ret->mName = IMessageHelper::getChildElementText(root, "name");

          ElementPtr propertiesEl = root->findFirstChildElement("properties");
          if (propertiesEl) {
            ret->mNetworkURI = IMessageHelper::getChildElementText(propertiesEl, "networkURI");
            ret->mTURNServer = IMessageHelper::getChildElementText(propertiesEl, "turnServer");
            ret->mTURNUsername = IMessageHelper::getChildElementText(propertiesEl, "turnUsername");
            ret->mTURNPassword = IMessageHelper::getChildElementText(propertiesEl, "turnPassword");
            ret->mSTUNServer = IMessageHelper::getChildElementText(propertiesEl, "stunServer");
          }

          ret->mProviderType = provisioning::IAccount::toIdentity(IMessageHelper::getChildElementText(root, "providerType"));
          ret->mProviderUniqueID = IMessageHelper::getChildElementText(root, "providerUniqueID");
          ret->mProviderOAuthAccessToken = IMessageHelper::getChildElementText(root, "providerOAuthAccessToken");
          ret->mProviderEncryptedOAuthAccessSecret = IMessageHelper::getChildElementText(root, "providerEncryptedOAuthAccessSecret");

          return ret;
        }

      }
    }
  }
}
