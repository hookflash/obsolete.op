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

#include <hookflash/provisioning/message/hookflashTypes.h>
#include <hookflash/stack/message/IMessageFactory.h>


namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageFactoryStack
      #pragma mark

      class MessageFactoryProvisioning : public stack::message::IMessageFactory
      {
      public:
        typedef stack::message::Message Message;
        typedef stack::message::MessagePtr MessagePtr;

      public:
        enum Methods
        {
          Method_Invalid = Message::Method_Invalid,

          Method_LookupProfile,

          Method_CreateAccount,
          Method_AccessAccount,

          Method_ProfileGet,
          Method_ProfilePut,

          Method_SendIDValidationPIN,
          Method_ValidateIDPIN,

          Method_ProviderLoginURLGet,
          Method_OAuthLoginWebpage,

          Method_ProviderAssociateURLGet,
          Method_OAuthLoginWebpageForAssociation,

          Method_PeerProfileLookup,
          Method_MultiPartyAPNSPush,

          Method_PasswordGetPart1,
          Method_PasswordGetPart2,

          Method_PasswordPINGet,

          Method_OAuthPasswordGet,
          Method_PasswordPut,

          Method_Last = Method_PasswordPut,
        };

      protected:
        static MessageFactoryProvisioningPtr create();

      public:
        static MessageFactoryProvisioningPtr singleton();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryStack => IMessageFactory
        #pragma mark

        virtual const char *xmlns() const;

        virtual Message::Methods toMethod(const char *method) const;
        virtual const char *toString(Message::Methods method) const;

        virtual MessagePtr create(ElementPtr root);
      };
    }
  }
}
