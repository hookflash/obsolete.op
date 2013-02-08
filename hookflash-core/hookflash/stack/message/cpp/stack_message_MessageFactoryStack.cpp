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

#include <hookflash/stack/message/Message.h>
#include <hookflash/services/IHelper.h>
#include <hookflash/stack/message/MessageFactoryStack.h>
#include <hookflash/stack/message/IMessageFactoryManager.h>

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <hookflash/stack/message/internal/stack_message_PeerPublishRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerGetResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerDeleteRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerDeleteResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerSubscribeRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerSubscribeResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishNotifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishNotifyResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionCreateRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionCreateResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionDeleteRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionDeleteResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerKeepAliveRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerKeepAliveResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindReply.h>

#include <hookflash/stack/message/internal/stack_message_PeerToPeerPeerIdentifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToPeerPeerIdentifyResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperServicesGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperServicesGetResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperFindersGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperFindersGetResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToSaltSignedSaltGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToSaltSignedSaltGetResult.h>


#include <zsLib/Log.h>

#define HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_STACK_XMLNS "http://www.hookflash.com/openpeer/1.0/message"

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace internal
      {
      }

      typedef zsLib::String String;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageFactoryStack
      #pragma mark

      //-----------------------------------------------------------------------
      MessageFactoryStackPtr MessageFactoryStack::create()
      {
        MessageFactoryStackPtr pThis(new MessageFactoryStack);
        IMessageFactoryManager::registerFactory(pThis);
        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageFactoryStackPtr MessageFactoryStack::singleton()
      {
        static MessageFactoryStackPtr pThis = create();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageFactoryStack => IMessageFactory
      #pragma mark

      //-----------------------------------------------------------------------
      const char *MessageFactoryStack::xmlns() const
      {
        return HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_STACK_XMLNS;
      }

      //-----------------------------------------------------------------------
      Message::Methods MessageFactoryStack::toMethod(const char *inMethod) const
      {
        typedef zsLib::ULONG ULONG;
        String methodStr(inMethod ? inMethod : "");

        for (ULONG loop = (ULONG)(MessageFactoryStack::Method_Invalid+1); loop <= ((ULONG)MessageFactoryStack::Method_Last); ++loop)
        {
          if (methodStr == toString((Message::Methods)loop)) {
            return (Message::Methods)loop;
          }
        }
        return Message::Method_Invalid;
      }

      //-----------------------------------------------------------------------
      const char *MessageFactoryStack::toString(Message::Methods method) const
      {
        switch ((MessageFactoryStack::Methods)method)
        {
          case Method_Invalid:                        return "";

          case Method_PeerKeepAlive:                  return "session-keep-alive";

          case Method_PeerPublish:                    return "peer-publish";
          case Method_PeerGet:                        return "peer-get";
          case Method_PeerDelete:                     return "peer-delete";
          case Method_PeerSubscribe:                  return "peer-subscribe";
          case Method_PeerPublishNotify:              return "peer-publish-notify";

          case Method_PeerToBootstrapper_ServicesGet: return "services-get";
          case Method_PeerToBootstrapper_FindersGet:  return "finders-get";

          case Method_PeerToSalt_SignedSaltGet:       return "signed-salt-get";

          case Method_PeerToFinder_SessionCreate:     return "session-create";
          case Method_PeerToFinder_SessionDelete:     return "session-delete";
          case Method_PeerToFinder_PeerLocationFind:  return "peer-location-find";

          case Method_PeerToPeer_PeerIdentify:        return "p2p-peer-identify";
        }
        return "";
      }

      //-----------------------------------------------------------------------
      MessagePtr MessageFactoryStack::create(ElementPtr root)
      {
        if (!root) return MessagePtr();

        Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
        Methods msgMethod = (MessageFactoryStack::Methods)toMethod(IMessageHelper::getAttribute(root, "method"));

        switch (msgType) {
          case Message::MessageType_Invalid:                return MessagePtr();

          case Message::MessageType_Request:
          {
            switch (msgMethod) {
              case Method_Invalid:                          return MessagePtr();

              case Method_PeerKeepAlive:                    return internal::PeerKeepAliveRequest::create(root);

              case Method_PeerPublish:                      return internal::PeerPublishRequest::create(root);
              case Method_PeerGet:                          return internal::PeerGetRequest::create(root);
              case Method_PeerDelete:                       return internal::PeerDeleteRequest::create(root);
              case Method_PeerSubscribe:                    return internal::PeerSubscribeRequest::create(root);
              case Method_PeerPublishNotify:                return MessagePtr();

              case Method_PeerToBootstrapper_ServicesGet:   return MessagePtr();
              case Method_PeerToBootstrapper_FindersGet:    return MessagePtr();

              case Method_PeerToSalt_SignedSaltGet:         return MessagePtr();

              case Method_PeerToFinder_SessionCreate:       return MessagePtr();
              case Method_PeerToFinder_SessionDelete:       return MessagePtr();
              case Method_PeerToFinder_PeerLocationFind:    return internal::PeerToFinderPeerLocationFindRequest::create(root);

              case Method_PeerToPeer_PeerIdentify:          return internal::PeerToPeerPeerIdentifyRequest::create(root);
            }
            break;
          }
          case Message::MessageType_Result:
          {
            switch (msgMethod) {
              case Method_Invalid:                          return MessagePtr();

              case Method_PeerKeepAlive:                    return internal::PeerKeepAliveResult::create(root);

              case Method_PeerPublish:                      return internal::PeerPublishResult::create(root);
              case Method_PeerGet:                          return internal::PeerGetResult::create(root);
              case Method_PeerDelete:                       return internal::PeerDeleteResult::create(root);
              case Method_PeerSubscribe:                    return internal::PeerSubscribeResult::create(root);
              case Method_PeerPublishNotify:                return internal::PeerPublishNotifyResult::create(root);

              case Method_PeerToBootstrapper_ServicesGet:   return internal::PeerToBootstrapperServicesGetResult::create(root);
              case Method_PeerToBootstrapper_FindersGet:    return internal::PeerToBootstrapperFindersGetResult::create(root);

              case Method_PeerToSalt_SignedSaltGet:         return internal::PeerToSaltSignedSaltGetResult::create(root);

              case Method_PeerToFinder_SessionCreate:       return internal::PeerToFinderSessionCreateResult::create(root);
              case Method_PeerToFinder_SessionDelete:       return internal::PeerToFinderSessionDeleteResult::create(root);
              case Method_PeerToFinder_PeerLocationFind:    return internal::PeerToFinderPeerLocationFindResult::create(root);

              case Method_PeerToPeer_PeerIdentify:          return internal::PeerToPeerPeerIdentifyResult::create(root);
            }
            break;
          }
          case Message::MessageType_Notify:
          {
            switch (msgMethod) {
              case Method_Invalid:                          return MessagePtr();

              case Method_PeerKeepAlive:                    return MessagePtr();

              case Method_PeerPublish:                      return MessagePtr();
              case Method_PeerGet:                          return MessagePtr();
              case Method_PeerDelete:                       return MessagePtr();
              case Method_PeerSubscribe:                    return MessagePtr();
              case Method_PeerPublishNotify:                return internal::PeerPublishNotifyRequest::create(root);

              case Method_PeerToBootstrapper_ServicesGet:   return MessagePtr();
              case Method_PeerToBootstrapper_FindersGet:    return MessagePtr();

              case Method_PeerToSalt_SignedSaltGet:         return MessagePtr();

              case Method_PeerToFinder_SessionCreate:       return MessagePtr();
              case Method_PeerToFinder_SessionDelete:       return MessagePtr();
              case Method_PeerToFinder_PeerLocationFind:    return MessagePtr();

              case Method_PeerToPeer_PeerIdentify:          return MessagePtr();
            }
            break;
          }
          case Message::MessageType_Reply:
          {
            switch (msgMethod) {
              case Method_Invalid:                          return MessagePtr();

              case Method_PeerKeepAlive:                    return MessagePtr();

              case Method_PeerPublish:                      return MessagePtr();
              case Method_PeerGet:                          return MessagePtr();
              case Method_PeerDelete:                       return MessagePtr();
              case Method_PeerSubscribe:                    return MessagePtr();
              case Method_PeerPublishNotify:                return MessagePtr();

              case Method_PeerToBootstrapper_ServicesGet:   return MessagePtr();
              case Method_PeerToBootstrapper_FindersGet:    return MessagePtr();

              case Method_PeerToSalt_SignedSaltGet:         return MessagePtr();

              case Method_PeerToFinder_SessionCreate:       return MessagePtr();
              case Method_PeerToFinder_SessionDelete:       return MessagePtr();
              case Method_PeerToFinder_PeerLocationFind:    return internal::PeerToFinderPeerLocationFindReply::create(root);

              case Method_PeerToPeer_PeerIdentify:          return MessagePtr();
            }
            break;
          }
        }

        return MessagePtr();
      }
    }
  }
}
