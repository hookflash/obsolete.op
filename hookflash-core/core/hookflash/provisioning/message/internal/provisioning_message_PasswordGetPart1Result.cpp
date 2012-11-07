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

#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart1Result.h>
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

      PasswordGetPart1ResultPtr PasswordGetPart1Result::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PasswordGetPart1Result>(message);
      }

      PasswordGetPart1Result::PasswordGetPart1Result()
      {
      }

      bool PasswordGetPart1Result::hasAttribute(PasswordGetPart1Result::AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_AccountSalt:                         return !mAccountSalt.isEmpty();

          case AttributeType_PasswordNonce:                       return !mPasswordNonce.isEmpty();
          case AttributeType_EncryptedPasswordPart1:              return !mEncryptedPasswordPart1.isEmpty();
          case AttributeType_EncryptedPasswordHash:               return !mEncryptedPasswordHash.isEmpty();
          case AttributeType_EncryptedEncryptionKeyPart1:         return !mEncryptedEncryptionKeyPart1.isEmpty();
          case AttributeType_ValidationPassProofPart1:            return !mValidationPassProofPart1.isEmpty();

          default:                                                break;
        }
        return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
      }

      namespace internal
      {
        PasswordGetPart1ResultPtr PasswordGetPart1Result::create(ElementPtr root)
        {
          PasswordGetPart1ResultPtr ret(new message::PasswordGetPart1Result);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          ret->mAccountSalt = IMessageHelper::getChildElementText(root, "accountSalt");

          ret->mPasswordNonce = IMessageHelper::getChildElementText(root, "passwordNonce");
          ret->mEncryptedPasswordPart1 = IMessageHelper::getChildElementText(root, "encryptedPasswordPart1");
          ret->mEncryptedPasswordHash = IMessageHelper::getChildElementText(root, "encryptedPasswordHash");
          ret->mEncryptedEncryptionKeyPart1 = IMessageHelper::getChildElementText(root, "encryptedEncryptionKeyPart1");
          ret->mValidationPassProofPart1 = IMessageHelper::getChildElementText(root, "validationPassProofPart1");

          return ret;
        }

      }
    }
  }
}
