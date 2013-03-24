/*

 Copyright (c) 2013, SMB Phone Inc.
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

#include <hookflash/stack/message/peer-contact/MessageFactoryPeerContact.h>
#include <hookflash/stack/message/peer-contact/PublicPeerFilesGetResult.h>
#include <hookflash/stack/message/peer-contact/PeerContactLoginResult.h>
#include <hookflash/stack/message/peer-contact/PrivatePeerFileGetResult.h>
#include <hookflash/stack/message/peer-contact/PrivatePeerFileSetResult.h>
#include <hookflash/stack/message/peer-contact/PeerContactIdentityAssociateResult.h>
#include <hookflash/stack/message/peer-contact/PeerContactServicesGetResult.h>
#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/IMessageFactoryManager.h>

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <hookflash/stack/IHelper.h>

#define HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_PEER_CONTACT_HANDLER "peer-contact"


namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_contact
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryIdentity
        #pragma mark

        //---------------------------------------------------------------------
        MessageFactoryPeerContactPtr MessageFactoryPeerContact::create()
        {
          MessageFactoryPeerContactPtr pThis(new MessageFactoryPeerContact);
          IMessageFactoryManager::registerFactory(pThis);
          return pThis;
        }

        //---------------------------------------------------------------------
        MessageFactoryPeerContactPtr MessageFactoryPeerContact::singleton()
        {
          static MessageFactoryPeerContactPtr pThis = create();
          return pThis;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryPeerContact => IMessageFactory
        #pragma mark

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerContact::getHandler() const
        {
          return HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_PEER_CONTACT_HANDLER;
        }

        //---------------------------------------------------------------------
        Message::Methods MessageFactoryPeerContact::toMethod(const char *inMethod) const
        {
          typedef zsLib::ULONG ULONG;
          String methodStr(inMethod ? inMethod : "");

          for (ULONG loop = (ULONG)(MessageFactoryPeerContact::Method_Invalid+1); loop <= ((ULONG)MessageFactoryPeerContact::Method_Last); ++loop)
          {
            if (methodStr == toString((Message::Methods)loop)) {
              return (Message::Methods)loop;
            }
          }
          return Message::Method_Invalid;
        }

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerContact::toString(Message::Methods method) const
        {
          switch ((MessageFactoryPeerContact::Methods)method)
          {
            case Method_Invalid:                          return "";

            case Method_PublicPeerFilesGet:               return "public-peer-files-get";
            case Method_PeerContactLogin:                 return "peer-contact-login";
            case Method_PrivatePeerFileGet:               return "private-peer-file-get";
            case Method_PrivatePeerFileSet:               return "private-peer-file-set";
            case Method_PeerContactIdentityAssociate:     return "peer-contact-identity-associate";
            case Method_PeerContactServicesGet:           return "peer-contact-services-get";
          }
          return "";
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryPeerContact::create(
                                                     ElementPtr root,
                                                     IMessageSourcePtr messageSource
                                                     )
        {
          if (!root) return MessagePtr();

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          Methods msgMethod = (MessageFactoryPeerContact::Methods)toMethod(IMessageHelper::getAttribute(root, "method"));

          switch (msgType) {
            case Message::MessageType_Invalid:                return MessagePtr();

            case Message::MessageType_Request:                return MessagePtr();
            case Message::MessageType_Result:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_PublicPeerFilesGet:               return PublicPeerFilesGetResult::create(root, messageSource);
                case Method_PeerContactLogin:                 return PeerContactLoginResult::create(root, messageSource);
                case Method_PrivatePeerFileGet:               return PrivatePeerFileGetResult::create(root, messageSource);
                case Method_PrivatePeerFileSet:               return PrivatePeerFileSetResult::create(root, messageSource);
                case Method_PeerContactIdentityAssociate:     return PeerContactIdentityAssociateResult::create(root, messageSource);
                case Method_PeerContactServicesGet:           return PeerContactServicesGetResult::create(root, messageSource);
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
}
