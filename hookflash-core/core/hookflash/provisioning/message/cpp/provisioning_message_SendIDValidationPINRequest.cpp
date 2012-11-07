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

#include <hookflash/provisioning/message/internal/provisioning_message_SendIDValidationPINRequest.h>
#include <hookflash/stack/message/IMessageHelper.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef stack::message::IMessageHelper IMessageHelper;

      SendIDValidationPINRequestPtr SendIDValidationPINRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<SendIDValidationPINRequest>(message);
      }

      SendIDValidationPINRequest::SendIDValidationPINRequest() :
        mIdentityType(provisioning::IAccount::IdentityType_None)
      {
      }

      bool SendIDValidationPINRequest::hasAttribute(AttributeTypes type) const
      {
        switch ((SendIDValidationPINRequest::AttributeTypes)type)
        {
          case AttributeType_UserID:                              return !mUserID.isEmpty();
          case AttributeType_AccessKey:                           return !mAccessKey.isEmpty();
          case AttributeType_AccessSecret:                        return !mAccessSecret.isEmpty();

          case AttributeType_IdentityType:                        return provisioning::IAccount::IdentityType_None != mIdentityType;
          case AttributeType_IdentityUniqueID:                    return !mIdentityUniqueID.isEmpty();
          case AttributeType_IdentityValidationID:                return !mIdentityValidationID.isEmpty();
        }
        return false;
      }

      DocumentPtr SendIDValidationPINRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::SendIDValidationPINRequest::encode(*this);
      }

      SendIDValidationPINRequestPtr SendIDValidationPINRequest::create()
      {
        return internal::SendIDValidationPINRequest::create();
      }

      namespace internal
      {
        SendIDValidationPINRequestPtr SendIDValidationPINRequest::create()
        {
          SendIDValidationPINRequestPtr ret(new message::SendIDValidationPINRequest);

          return ret;
        }

        DocumentPtr SendIDValidationPINRequest::encode(message::SendIDValidationPINRequest &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          if (msg.hasAttribute(message::SendIDValidationPINRequest::AttributeType_UserID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("userID", msg.mUserID));
          }

          if (msg.hasAttribute(message::SendIDValidationPINRequest::AttributeType_AccessKey)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessKey", msg.mAccessKey));
          }

          if (msg.hasAttribute(message::SendIDValidationPINRequest::AttributeType_AccessSecret)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessSecret", msg.mAccessSecret));
          }

          if (msg.hasAttribute(message::SendIDValidationPINRequest::AttributeType_IdentityType)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("identityType", provisioning::IAccount::toCodeString(msg.mIdentityType)));
          }

          if (msg.hasAttribute(message::SendIDValidationPINRequest::AttributeType_IdentityUniqueID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("identityUniqueID", msg.mIdentityUniqueID));
          }

          if (msg.hasAttribute(message::SendIDValidationPINRequest::AttributeType_IdentityValidationID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("identityValidationID", msg.mIdentityValidationID));
          }

          return ret;
        }

      }
    }
  }
}
