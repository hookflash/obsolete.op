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

#include <openpeer/stack/message/peer-common/MessageFactoryPeerCommon.h>
#include <openpeer/stack/message/Message.h>
#include <openpeer/stack/message/IMessageFactoryManager.h>

#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>

#include <openpeer/stack/message/peer-common/PeerPublishRequest.h>
#include <openpeer/stack/message/peer-common/PeerPublishResult.h>
#include <openpeer/stack/message/peer-common/PeerGetRequest.h>
#include <openpeer/stack/message/peer-common/PeerGetResult.h>
#include <openpeer/stack/message/peer-common/PeerDeleteRequest.h>
#include <openpeer/stack/message/peer-common/PeerDeleteResult.h>
#include <openpeer/stack/message/peer-common/PeerSubscribeRequest.h>
#include <openpeer/stack/message/peer-common/PeerSubscribeResult.h>
#include <openpeer/stack/message/peer-common/PeerPublishNotifyRequest.h>
#include <openpeer/stack/message/peer-common/PeerPublishNotifyResult.h>

#include <openpeer/stack/IHelper.h>

#define OPENPEER_STACK_MESSAGE_MESSAGE_FACTORY_PEER_COMMON_HANDLER "peer-common"

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace peer_common
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryPeerCommon
        #pragma mark

        //---------------------------------------------------------------------
        MessageFactoryPeerCommonPtr MessageFactoryPeerCommon::create()
        {
          MessageFactoryPeerCommonPtr pThis(new MessageFactoryPeerCommon);
          IMessageFactoryManager::registerFactory(pThis);
          return pThis;
        }

        //---------------------------------------------------------------------
        MessageFactoryPeerCommonPtr MessageFactoryPeerCommon::singleton()
        {
          static MessageFactoryPeerCommonPtr pThis = create();
          return pThis;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryPeerCommon => IMessageFactory
        #pragma mark

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerCommon::getHandler() const
        {
          return OPENPEER_STACK_MESSAGE_MESSAGE_FACTORY_PEER_COMMON_HANDLER;
        }

        //---------------------------------------------------------------------
        Message::Methods MessageFactoryPeerCommon::toMethod(const char *inMethod) const
        {
          typedef zsLib::ULONG ULONG;
          String methodStr(inMethod ? inMethod : "");

          for (ULONG loop = (ULONG)(MessageFactoryPeerCommon::Method_Invalid+1); loop <= ((ULONG)MessageFactoryPeerCommon::Method_Last); ++loop)
          {
            if (methodStr == toString((Message::Methods)loop)) {
              return (Message::Methods)loop;
            }
          }
          return Message::Method_Invalid;
        }

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerCommon::toString(Message::Methods method) const
        {
          switch ((MessageFactoryPeerCommon::Methods)method)
          {
            case Method_Invalid:                        return "";

            case Method_PeerPublish:                    return "peer-publish";
            case Method_PeerGet:                        return "peer-get";
            case Method_PeerDelete:                     return "peer-delete";
            case Method_PeerSubscribe:                  return "peer-subscribe";
            case Method_PeerPublishNotify:              return "peer-publish-notify";
          }
          return "";
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryPeerCommon::create(
                                                    ElementPtr root,
                                                    IMessageSourcePtr messageSource
                                                    )
        {
          if (!root) return MessagePtr();

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          Methods msgMethod = (MessageFactoryPeerCommon::Methods)toMethod(IMessageHelper::getAttribute(root, "method"));

          switch (msgType) {
            case Message::MessageType_Invalid:                return MessagePtr();

            case Message::MessageType_Request:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_PeerPublish:                      return PeerPublishRequest::create(root, messageSource);
                case Method_PeerGet:                          return PeerGetRequest::create(root, messageSource);
                case Method_PeerDelete:                       return PeerDeleteRequest::create(root, messageSource);
                case Method_PeerSubscribe:                    return PeerSubscribeRequest::create(root, messageSource);
                case Method_PeerPublishNotify:                return MessagePtr();
              }
              break;
            }
            case Message::MessageType_Result:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_PeerPublish:                      return PeerPublishResult::create(root, messageSource);
                case Method_PeerGet:                          return PeerGetResult::create(root, messageSource);
                case Method_PeerDelete:                       return PeerDeleteResult::create(root, messageSource);
                case Method_PeerSubscribe:                    return PeerSubscribeResult::create(root, messageSource);
                case Method_PeerPublishNotify:                return PeerPublishNotifyResult::create(root, messageSource);
              }
              break;
            }
            case Message::MessageType_Notify:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_PeerPublish:                      return MessagePtr();
                case Method_PeerGet:                          return MessagePtr();
                case Method_PeerDelete:                       return MessagePtr();
                case Method_PeerSubscribe:                    return MessagePtr();
                case Method_PeerPublishNotify:                return PeerPublishNotifyRequest::create(root, messageSource);
              }
              break;
            }
          }

          return MessagePtr();
        }
      }
    }
  }
}
