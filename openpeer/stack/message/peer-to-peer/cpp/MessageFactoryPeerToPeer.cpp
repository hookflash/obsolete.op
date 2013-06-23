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

#include <hookflash/stack/message/peer-to-peer/MessageFactoryPeerToPeer.h>
#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/IMessageFactoryManager.h>

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <hookflash/stack/message/peer-to-peer/PeerKeepAliveRequest.h>
#include <hookflash/stack/message/peer-to-peer/PeerKeepAliveResult.h>

#include <hookflash/stack/message/peer-to-peer/PeerIdentifyRequest.h>
#include <hookflash/stack/message/peer-to-peer/PeerIdentifyResult.h>

#include <hookflash/stack/IHelper.h>

#include <zsLib/Log.h>

#define HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_PEER_TO_PEER_HANDLER "p2p"

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_to_peer
      {

        typedef zsLib::String String;

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryPeerToPeer
        #pragma mark

        //---------------------------------------------------------------------
        MessageFactoryPeerToPeerPtr MessageFactoryPeerToPeer::create()
        {
          MessageFactoryPeerToPeerPtr pThis(new MessageFactoryPeerToPeer);
          IMessageFactoryManager::registerFactory(pThis);
          return pThis;
        }

        //---------------------------------------------------------------------
        MessageFactoryPeerToPeerPtr MessageFactoryPeerToPeer::singleton()
        {
          static MessageFactoryPeerToPeerPtr pThis = create();
          return pThis;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryPeerToPeer => IMessageFactory
        #pragma mark

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerToPeer::getHandler() const
        {
          return HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_PEER_TO_PEER_HANDLER;
        }

        //---------------------------------------------------------------------
        Message::Methods MessageFactoryPeerToPeer::toMethod(const char *inMethod) const
        {
          typedef zsLib::ULONG ULONG;
          String methodStr(inMethod ? inMethod : "");

          for (ULONG loop = (ULONG)(MessageFactoryPeerToPeer::Method_Invalid+1); loop <= ((ULONG)MessageFactoryPeerToPeer::Method_Last); ++loop)
          {
            if (methodStr == toString((Message::Methods)loop)) {
              return (Message::Methods)loop;
            }
          }
          return Message::Method_Invalid;
        }

        //---------------------------------------------------------------------
        const char *MessageFactoryPeerToPeer::toString(Message::Methods method) const
        {
          switch ((MessageFactoryPeerToPeer::Methods)method)
          {
            case Method_Invalid:              return "";

            case Method_PeerKeepAlive:        return "peer-keep-alive";
            case Method_PeerIdentify:         return "peer-identify";
          }
          return "";
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryPeerToPeer::create(
                                                    ElementPtr root,
                                                    IMessageSourcePtr messageSource
                                                    )
        {
          if (!root) return MessagePtr();

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          Methods msgMethod = (MessageFactoryPeerToPeer::Methods)toMethod(IMessageHelper::getAttribute(root, "method"));

          switch (msgType) {
            case Message::MessageType_Invalid:                return MessagePtr();

            case Message::MessageType_Request:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_PeerKeepAlive:                    return PeerKeepAliveRequest::create(root, messageSource);
                case Method_PeerIdentify:                     return PeerIdentifyRequest::create(root, messageSource);
              }
              break;
            }
            case Message::MessageType_Result:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_PeerKeepAlive:                    return PeerKeepAliveResult::create(root, messageSource);
                case Method_PeerIdentify:                     return PeerIdentifyResult::create(root, messageSource);
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
