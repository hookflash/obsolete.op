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

#include <hookflash/provisioning/message/internal/provisioning_message_AccessAccountResult.h>
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

      AccessAccountResultPtr AccessAccountResult::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<AccessAccountResult>(message);
      }

      AccessAccountResult::AccessAccountResult()
      {
      }

      bool AccessAccountResult::hasAttribute(AccessAccountResult::AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_AccessKey:                           return !mAccessKey.isEmpty();
          case AttributeType_AccessSecret:                        return !mAccessSecret.isEmpty();

          case AttributeType_LastProfileUpdateTimestamp:          return Time() != mLastProfileUpdateTimestamp;

          case AttributeType_NetworkURI:                          return !mNetworkURI.isEmpty();
          case AttributeType_TURNServer:                          return !mTURNServer.isEmpty();
          case AttributeType_TURNUsername:                        return !mTURNUsername.isEmpty();
          case AttributeType_TURNPassword:                        return !mTURNPassword.isEmpty();
          case AttributeType_STUNServer:                          return !mSTUNServer.isEmpty();

          default:                                                break;
        }
        return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
      }

      namespace internal
      {
        AccessAccountResultPtr AccessAccountResult::create(ElementPtr root)
        {
          AccessAccountResultPtr ret(new message::AccessAccountResult);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          ret->mAccessKey = IMessageHelper::getChildElementText(root, "accessKey");
          ret->mAccessSecret = IMessageHelper::getChildElementText(root, "accessSecret");

          ret->mLastProfileUpdateTimestamp = IMessageHelper::stringToTime(IMessageHelper::getChildElementText(root, "lastProfileUpdateTimestamp"));

          ElementPtr propertiesEl = root->findFirstChildElement("properties");
          if (propertiesEl) {
            ret->mNetworkURI = IMessageHelper::getChildElementText(propertiesEl, "networkURI");
            ret->mTURNServer = IMessageHelper::getChildElementText(propertiesEl, "turnServer");
            ret->mTURNUsername = IMessageHelper::getChildElementText(propertiesEl, "turnUsername");
            ret->mTURNPassword = IMessageHelper::getChildElementText(propertiesEl, "turnPassword");
            ret->mSTUNServer = IMessageHelper::getChildElementText(propertiesEl, "stunServer");
          }

          return ret;
        }

      }
    }
  }
}
