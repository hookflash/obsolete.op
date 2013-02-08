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
#include <hookflash/stack/message/MessageResult.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      namespace internal { class OAuthPasswordGetResult;}

      class OAuthPasswordGetResult : public stack::message::MessageResult
      {
      public:
        typedef stack::message::MessagePtr MessagePtr;
        typedef stack::message::IMessageFactoryPtr IMessageFactoryPtr;

        friend class internal::OAuthPasswordGetResult;

        enum AttributeTypes
        {
          AttributeType_AccountSalt = AttributeType_Last + 1,

          AttributeType_PasswordNonce,
          AttributeType_PasswordPart,
          AttributeType_PasswordHash,
        };

      public:
        static OAuthPasswordGetResultPtr convert(MessagePtr message);

        virtual Methods method() const              {return (Message::Methods)MessageFactoryProvisioning::Method_OAuthPasswordGet;}

        virtual IMessageFactoryPtr factory() const  {return MessageFactoryProvisioning::singleton();}

        bool hasAttribute(AttributeTypes type) const;

        const String &accountSalt() const           {return mAccountSalt;}
        void accountSalt(const String &val)         {mAccountSalt = val;}

        const String &passwordNonce() const         {return mPasswordNonce;}
        void passwordNonce(const String &val)       {mPasswordNonce = val;}

        const String &passwordPart() const          {return mPasswordPart;}
        void passwordPart(const String &val)        {mPasswordPart = val;}

        const String &passwordHash() const          {return mPasswordHash;}
        void passwordHash(const String &val)        {mPasswordHash = val;}

      protected:
        OAuthPasswordGetResult();

        String mAccountSalt;

        String mPasswordNonce;
        String mPasswordPart;
        String mPasswordHash;
      };
    }
  }
}
