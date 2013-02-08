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

#include <hookflash/provisioning/message/internal/provisioning_message_PasswordPutRequest.h>
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

      PasswordPutRequestPtr PasswordPutRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PasswordPutRequest>(message);
      }

      PasswordPutRequest::PasswordPutRequest()
      {
      }

      bool PasswordPutRequest::hasAttribute(AttributeTypes type) const
      {
        switch ((PasswordPutRequest::AttributeTypes)type)
        {
          case AttributeType_UserID:                      return !mUserID.isEmpty();
          case AttributeType_AccountSalt:                 return !mAccountSalt.isEmpty();
          case AttributeType_AccessKey:                   return !mAccessKey.isEmpty();
          case AttributeType_AccessSecret:                return !mAccessSecret.isEmpty();

          case AttributeType_PasswordNonce:               return !mPasswordNonce.isEmpty();
          case AttributeType_PasswordHash:                return !mPasswordHash.isEmpty();
          case AttributeType_UniqueIDProofInput:          return !mUniqueIDProofInput.isEmpty();

          case AttributeType_PasswordPart:                return !mPasswordPart.isEmpty();
        }
        return false;
      }

      DocumentPtr PasswordPutRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::PasswordPutRequest::encode(*this);
      }

      PasswordPutRequestPtr PasswordPutRequest::create()
      {
        return internal::PasswordPutRequest::create();
      }

      namespace internal
      {
        PasswordPutRequestPtr PasswordPutRequest::create()
        {
          PasswordPutRequestPtr ret(new message::PasswordPutRequest);

          return ret;
        }

        DocumentPtr PasswordPutRequest::encode(message::PasswordPutRequest &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_UserID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("userID", msg.mUserID));
          }

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_AccountSalt)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accountSalt", msg.mAccountSalt));
          }

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_AccessKey)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessKey", msg.mAccessKey));
          }

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_AccessSecret)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessSecret", msg.mAccessSecret));
          }

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_PasswordNonce)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("passwordNonce", msg.mPasswordNonce));
          }

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_PasswordHash)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("passwordHash", msg.mPasswordHash));
          }

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_UniqueIDProofInput)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("uniqueIDProofInput", msg.mUniqueIDProofInput));
          }

          if (msg.hasAttribute(message::PasswordPutRequest::AttributeType_PasswordPart)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("passwordPart", msg.mPasswordPart));
          }

          return ret;
        }

      }
    }
  }
}
