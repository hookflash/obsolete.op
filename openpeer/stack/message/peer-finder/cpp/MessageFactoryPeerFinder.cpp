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

#include <hookflash/stack/message/peer-finder/MessageFactoryPeerFinder.h>
#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/IMessageFactoryManager.h>

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <hookflash/stack/message/peer-finder/SessionCreateResult.h>
#include <hookflash/stack/message/peer-finder/SessionDeleteResult.h>
#include <hookflash/stack/message/peer-finder/SessionKeepAliveResult.h>

#include <hookflash/stack/message/peer-finder/PeerLocationFindRequest.h>
#include <hookflash/stack/message/peer-finder/PeerLocationFindResult.h>
#include <hookflash/stack/message/peer-finder/PeerLocationFindReply.h>

#define HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_PEER_FINDER_HANDLER "peer-finder"

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_finder
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryPeerFinder
        #pragma mark

        //---------------------------------------------------------------------
        MessageFactoryPeerFinderPtr MessageFactoryPeerFinder::create()
        {
          MessageFactoryPeerFinderPtr pThis(new MessageFactoryPeerFinder);
          IMessageFactoryManager::registerFactory(pThis);
          return pThis;
        }

        //---------------------------------------------------------------------
        MessageFactoryPeerFinderPtr MessageFactoryPeerFinder::singleton()
        {
          static MessageFactoryPeerFinderPtr pThis = create();
          return pThis;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryPeerFinder => IMessageFactory
        #pragma mark

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerFinder::getHandler() const
        {
          return HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_PEER_FINDER_HANDLER;
        }

        //---------------------------------------------------------------------
        Message::Methods MessageFactoryPeerFinder::toMethod(const char *inMethod) const
        {
          typedef zsLib::ULONG ULONG;
          String methodStr(inMethod ? inMethod : "");

          for (ULONG loop = (ULONG)(MessageFactoryPeerFinder::Method_Invalid+1); loop <= ((ULONG)MessageFactoryPeerFinder::Method_Last); ++loop)
          {
            if (methodStr == toString((Message::Methods)loop)) {
              return (Message::Methods)loop;
            }
          }
          return Message::Method_Invalid;
        }

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerFinder::toString(Message::Methods method) const
        {
          switch ((MessageFactoryPeerFinder::Methods)method)
          {
            case Method_Invalid:                  return "";

            case Method_SessionKeepAlive:         return "session-keep-alive";
            case Method_SessionCreate:            return "session-create";
            case Method_SessionDelete:            return "session-delete";

            case Method_PeerLocationFind:         return "peer-location-find";
          }
          return "";
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryPeerFinder::create(
                                                    ElementPtr root,
                                                    IMessageSourcePtr messageSource
                                                    )
        {
          if (!root) return MessagePtr();

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          Methods msgMethod = (MessageFactoryPeerFinder::Methods)toMethod(IMessageHelper::getAttribute(root, "method"));

          switch (msgType) {
            case Message::MessageType_Invalid:                return MessagePtr();

            case Message::MessageType_Request:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_SessionKeepAlive:                 return MessagePtr();
                case Method_SessionCreate:                    return MessagePtr();
                case Method_SessionDelete:                    return MessagePtr();

                case Method_PeerLocationFind:                 return PeerLocationFindRequest::create(root, messageSource);
              }
              break;
            }
            case Message::MessageType_Result:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_SessionKeepAlive:                 return SessionKeepAliveResult::create(root, messageSource);
                case Method_SessionCreate:                    return SessionCreateResult::create(root, messageSource);
                case Method_SessionDelete:                    return SessionDeleteResult::create(root, messageSource);

                case Method_PeerLocationFind:                 return PeerLocationFindResult::create(root, messageSource);
              }
              break;
            }
            case Message::MessageType_Notify:                 return MessagePtr();
            case Message::MessageType_Reply:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_SessionKeepAlive:                 return MessagePtr();
                case Method_SessionCreate:                    return MessagePtr();
                case Method_SessionDelete:                    return MessagePtr();

                case Method_PeerLocationFind:                 return PeerLocationFindReply::create(root, messageSource);
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
