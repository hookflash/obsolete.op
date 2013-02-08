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

#pragma once

#include <hookflash/provisioning/message/MessageFactoryProvisioning.h>
#include <hookflash/provisioning/IAccount.h>
#include <hookflash/stack/message/MessageResult.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      namespace internal { class PasswordGetPart1Result;}

      class PasswordGetPart1Result : public stack::message::MessageResult
      {
      public:
        typedef stack::message::MessagePtr MessagePtr;
        typedef stack::message::IMessageFactoryPtr IMessageFactoryPtr;

        friend class internal::PasswordGetPart1Result;

        enum AttributeTypes
        {
          AttributeType_AccountSalt = AttributeType_Last + 1,

          AttributeType_PasswordNonce,
          AttributeType_EncryptedPasswordPart1,
          AttributeType_EncryptedPasswordHash,
          AttributeType_EncryptedEncryptionKeyPart1,
          AttributeType_ValidationPassProofPart1,
        };

      public:
        static PasswordGetPart1ResultPtr convert(MessagePtr message);

        virtual Methods method() const                      {return (Message::Methods)MessageFactoryProvisioning::Method_PasswordGetPart1;}

        virtual IMessageFactoryPtr factory() const          {return MessageFactoryProvisioning::singleton();}

        bool hasAttribute(AttributeTypes type) const;

        const String &accountSalt() const                   {return mAccountSalt;}
        void accountSalt(const String &val)                 {mAccountSalt = val;}

        const String &passwordNonce() const                 {return mPasswordNonce;}
        void passwordNonce(const String &val)               {mPasswordNonce = val;}

        const String &encryptedPasswordPart1() const        {return mEncryptedPasswordPart1;}
        void encryptedPasswordPart1(const String &val)      {mEncryptedPasswordPart1 = val;}

        const String &encryptedPasswordHash() const         {return mEncryptedPasswordHash;}
        void encryptedPasswordHash(const String &val)       {mEncryptedPasswordHash = val;}

        const String &encryptedEncryptionKeyPart1() const   {return mEncryptedEncryptionKeyPart1;}
        void encryptedEncryptionKeyPart1(const String &val) {mEncryptedEncryptionKeyPart1 = val;}

        const String &validationPassProofPart1() const      {return mValidationPassProofPart1;}
        void validationPassProofPart1(const String &val)    {mValidationPassProofPart1 = val;}

      protected:
        PasswordGetPart1Result();

        String mAccountSalt;

        String mPasswordNonce;
        String mEncryptedPasswordPart1;
        String mEncryptedPasswordHash;
        String mEncryptedEncryptionKeyPart1;
        String mValidationPassProofPart1;
      };
    }
  }
}
