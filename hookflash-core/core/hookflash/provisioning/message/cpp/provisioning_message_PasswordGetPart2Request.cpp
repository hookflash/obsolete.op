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

#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart2Request.h>
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

      PasswordGetPart2RequestPtr PasswordGetPart2Request::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PasswordGetPart2Request>(message);
      }

      PasswordGetPart2Request::PasswordGetPart2Request()
      {
      }

      bool PasswordGetPart2Request::hasAttribute(AttributeTypes type) const
      {
        switch ((PasswordGetPart2Request::AttributeTypes)type)
        {
          case AttributeType_UserID:                      return !mUserID.isEmpty();
          case AttributeType_ValidationKey1:              return !mValidationKey1.isEmpty();
          case AttributeType_ValidationKey2:              return !mValidationKey2.isEmpty();
          case AttributeType_PINDigits:                   return !mPINDigits.isEmpty();

          case AttributeType_EncryptedEncryptionKeyPart1: return !mEncryptedEncryptionKeyPart1.isEmpty();
          case AttributeType_ValidationPassProofPart1:    return !mValidationPassProofPart1.isEmpty();
        }
        return false;
      }

      DocumentPtr PasswordGetPart2Request::encode(IPeerFilesPtr peerFile)
      {
        return internal::PasswordGetPart2Request::encode(*this);
      }

      PasswordGetPart2RequestPtr PasswordGetPart2Request::create()
      {
        return internal::PasswordGetPart2Request::create();
      }

      namespace internal
      {
        PasswordGetPart2RequestPtr PasswordGetPart2Request::create()
        {
          PasswordGetPart2RequestPtr ret(new message::PasswordGetPart2Request);

          return ret;
        }

        DocumentPtr PasswordGetPart2Request::encode(message::PasswordGetPart2Request &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          if (msg.hasAttribute(message::PasswordGetPart2Request::AttributeType_UserID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("userID", msg.mUserID));
          }

          if (msg.hasAttribute(message::PasswordGetPart2Request::AttributeType_ValidationKey1)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("validationKey1", msg.mValidationKey1));
          }

          if (msg.hasAttribute(message::PasswordGetPart2Request::AttributeType_ValidationKey2)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("validationKey2", msg.mValidationKey2));
          }

          if (msg.hasAttribute(message::PasswordGetPart2Request::AttributeType_PINDigits)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("pinDigits", msg.mPINDigits));
          }

          if (msg.hasAttribute(message::PasswordGetPart2Request::AttributeType_EncryptedEncryptionKeyPart1)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("encryptedEncryptionKeyPart1", msg.mEncryptedEncryptionKeyPart1));
          }

          if (msg.hasAttribute(message::PasswordGetPart2Request::AttributeType_ValidationPassProofPart1)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("validationPassProofPart1", msg.mValidationPassProofPart1));
          }

          return ret;
        }

      }
    }
  }
}
