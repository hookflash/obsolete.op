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

#include <hookflash/provisioning/message/MessageFactoryProvisioning.h>

#include <hookflash/provisioning/message/internal/provisioning_message_LookupProfileResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_CreateAccountResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_AccessAccountResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProfileGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProfilePutResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_SendIDValidationPINResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ValidateIDPINResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProviderLoginURLGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_OAuthLoginWebpageResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProviderAssociateURLGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_OAuthLoginWebpageForAssociationResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PeerProfileLookupResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_MultiPartyAPNSPushResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart1Result.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart2Result.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordPINGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_OAuthPasswordGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordPutResult.h>

#include <hookflash/stack/message/IMessageFactoryManager.h>
#include <hookflash/stack/message/IMessageHelper.h>

#include <zsLib/Log.h>

#define HOOKFLASH_PROVISIONING_MESSAGE_MESSAGE_FACTORY_PROVISIONING_XMLNS "http://www.hookflash.com/provisioning/1.0/message"

namespace hookflash { namespace provisioning { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_provisioning_message) } } }

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      typedef zsLib::String String;
      typedef stack::message::IMessageFactoryManager IMessageFactoryManager;
      typedef stack::message::Message Message;
      typedef stack::message::MessagePtr MessagePtr;
      typedef stack::message::IMessageHelper IMessageHelper;

      namespace internal
      {
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageFactoryProvisioning
      #pragma mark

      //-----------------------------------------------------------------------
      MessageFactoryProvisioningPtr MessageFactoryProvisioning::create()
      {
        MessageFactoryProvisioningPtr pThis(new MessageFactoryProvisioning);
        IMessageFactoryManager::registerFactory(pThis);
        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageFactoryProvisioningPtr MessageFactoryProvisioning::singleton()
      {
        static MessageFactoryProvisioningPtr pThis = create();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageFactoryProvisioning => IMessageFactory
      #pragma mark

      //-----------------------------------------------------------------------
      const char *MessageFactoryProvisioning::xmlns() const
      {
        return HOOKFLASH_PROVISIONING_MESSAGE_MESSAGE_FACTORY_PROVISIONING_XMLNS;
      }

      //-----------------------------------------------------------------------
      Message::Methods MessageFactoryProvisioning::toMethod(const char *inMethod) const
      {
        typedef zsLib::ULONG ULONG;
        String methodStr(inMethod ? inMethod : "");

        for (ULONG loop = (ULONG)(MessageFactoryProvisioning::Method_Invalid+1); loop <= ((ULONG)MessageFactoryProvisioning::Method_Last); ++loop)
        {
          if (methodStr == toString((Message::Methods)loop)) {
            return (Message::Methods)loop;
          }
        }
        return Message::Method_Invalid;
      }

      //-----------------------------------------------------------------------
      const char *MessageFactoryProvisioning::toString(Message::Methods method) const
      {
        switch ((MessageFactoryProvisioning::Methods)method)
        {
          case Method_Invalid:                          return "";

          case Method_LookupProfile:                    return "lookup-profile";

          case Method_CreateAccount:                    return "create-account";
          case Method_AccessAccount:                    return "access-account";

          case Method_ProfileGet:                       return "profile-get";
          case Method_ProfilePut:                       return "profile-put";

          case Method_SendIDValidationPIN:              return "send-id-validation-pin";
          case Method_ValidateIDPIN:                    return "validate-id-pin";

          case Method_ProviderLoginURLGet:              return "provider-login-url-get";
          case Method_OAuthLoginWebpage:                return "oauth-login-webpage";

          case Method_ProviderAssociateURLGet:          return "provider-associate-url-get";
          case Method_OAuthLoginWebpageForAssociation:  return "oauth-login-webpage-for-association";

          case Method_PeerProfileLookup:                return "peer-profile-lookup";
          case Method_MultiPartyAPNSPush:               return "multiparty-apns-push";

          case Method_PasswordGetPart1:                 return "password-get-part1";
          case Method_PasswordGetPart2:                 return "password-get-part2";

          case Method_PasswordPINGet:                   return "password-pin-get";

          case Method_OAuthPasswordGet:                 return "oauth-password-get";
          case Method_PasswordPut:                      return "password-put";
        }
        return "";
      }

      //-----------------------------------------------------------------------
      MessagePtr MessageFactoryProvisioning::create(ElementPtr root)
      {
        if (!root) return MessagePtr();

        Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
        Methods msgMethod = (MessageFactoryProvisioning::Methods)toMethod(IMessageHelper::getAttribute(root, "method"));

        switch (msgType) {
          case Message::MessageType_Invalid:                return MessagePtr();

          case Message::MessageType_Request:
          {
            switch (msgMethod) {
              case Method_Invalid:                          return MessagePtr();

              case Method_LookupProfile:                    return MessagePtr();

              case Method_CreateAccount:                    return MessagePtr();
              case Method_AccessAccount:                    return MessagePtr();

              case Method_ProfileGet:                       return MessagePtr();
              case Method_ProfilePut:                       return MessagePtr();

              case Method_SendIDValidationPIN:              return MessagePtr();
              case Method_ValidateIDPIN:                    return MessagePtr();

              case Method_ProviderLoginURLGet:              return MessagePtr();
              case Method_OAuthLoginWebpage:                return MessagePtr();

              case Method_ProviderAssociateURLGet:          return MessagePtr();
              case Method_OAuthLoginWebpageForAssociation:  return MessagePtr();

              case Method_PeerProfileLookup:                return MessagePtr();
              case Method_MultiPartyAPNSPush:               return MessagePtr();

              case Method_PasswordGetPart1:                 return MessagePtr();
              case Method_PasswordGetPart2:                 return MessagePtr();

              case Method_PasswordPINGet:                   return MessagePtr();

              case Method_OAuthPasswordGet:                 return MessagePtr();
              case Method_PasswordPut:                      return MessagePtr();
            }
            break;
          }
          case Message::MessageType_Result:
          {
            switch (msgMethod) {
              case Method_Invalid:                          return MessagePtr();

              case Method_LookupProfile:                    return internal::LookupProfileResult::create(root);

              case Method_CreateAccount:                    return internal::CreateAccountResult::create(root);
              case Method_AccessAccount:                    return internal::AccessAccountResult::create(root);

              case Method_ProfileGet:                       return internal::ProfileGetResult::create(root);
              case Method_ProfilePut:                       return internal::ProfilePutResult::create(root);

              case Method_SendIDValidationPIN:              return internal::SendIDValidationPINResult::create(root);
              case Method_ValidateIDPIN:                    return internal::ValidateIDPINResult::create(root);

              case Method_ProviderLoginURLGet:              return internal::ProviderLoginURLGetResult::create(root);
              case Method_OAuthLoginWebpage:                return internal::OAuthLoginWebpageResult::create(root);

              case Method_ProviderAssociateURLGet:          return internal::ProviderAssociateURLGetResult::create(root);
              case Method_OAuthLoginWebpageForAssociation:  return internal::OAuthLoginWebpageForAssociationResult::create(root);

              case Method_PeerProfileLookup:                return internal::PeerProfileLookupResult::create(root);
              case Method_MultiPartyAPNSPush:               return internal::MultiPartyAPNSPushResult::create(root);

              case Method_PasswordGetPart1:                 return internal::PasswordGetPart1Result::create(root);
              case Method_PasswordGetPart2:                 return internal::PasswordGetPart2Result::create(root);

              case Method_PasswordPINGet:                   return internal::PasswordPINGetResult::create(root);

              case Method_OAuthPasswordGet:                 return internal::OAuthPasswordGetResult::create(root);
              case Method_PasswordPut:                      return internal::PasswordPutResult::create(root);
            }
            break;
          }
          case Message::MessageType_Notify:                 return MessagePtr();
          case Message::MessageType_Reply:                  return MessagePtr();
        }

        return MessagePtr();
      }
    }
  }
}
