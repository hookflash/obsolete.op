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

#include <hookflash/stack/message/bootstrapper/MessageFactoryBootstrapper.h>
#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/IMessageFactoryManager.h>

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/bootstrapper/ServicesGetResult.h>

#include <hookflash/stack/IHelper.h>

#define HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_BOOTSTRAPPER_HANDLER "bootstrapper"


namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace bootstrapper
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryBootstrapper
        #pragma mark

        //---------------------------------------------------------------------
        MessageFactoryBootstrapperPtr MessageFactoryBootstrapper::create()
        {
          MessageFactoryBootstrapperPtr pThis(new MessageFactoryBootstrapper);
          IMessageFactoryManager::registerFactory(pThis);
          return pThis;
        }

        //---------------------------------------------------------------------
        MessageFactoryBootstrapperPtr MessageFactoryBootstrapper::singleton()
        {
          static MessageFactoryBootstrapperPtr pThis = create();
          return pThis;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryBootstrapper => IMessageFactory
        #pragma mark

        //---------------------------------------------------------------------
        const char *MessageFactoryBootstrapper::getHandler() const
        {
          return HOOKFLASH_STACK_MESSAGE_MESSAGE_FACTORY_BOOTSTRAPPER_HANDLER;
        }

        //---------------------------------------------------------------------
        Message::Methods MessageFactoryBootstrapper::toMethod(const char *inMethod) const
        {
          typedef zsLib::ULONG ULONG;
          String methodStr(inMethod ? inMethod : "");

          for (ULONG loop = (ULONG)(MessageFactoryBootstrapper::Method_Invalid+1); loop <= ((ULONG)MessageFactoryBootstrapper::Method_Last); ++loop)
          {
            if (methodStr == toString((Message::Methods)loop)) {
              return (Message::Methods)loop;
            }
          }
          return Message::Method_Invalid;
        }

        //---------------------------------------------------------------------
        const char *MessageFactoryBootstrapper::toString(Message::Methods method) const
        {
          switch ((MessageFactoryBootstrapper::Methods)method)
          {
            case Method_Invalid:            return "";

            case Method_ServicesGet:        return "services-get";
          }
          return "";
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryBootstrapper::create(
                                                      ElementPtr root,
                                                      IMessageSourcePtr messageSource
                                                      )
        {
          if (!root) return MessagePtr();

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          Methods msgMethod = (MessageFactoryBootstrapper::Methods)toMethod(IMessageHelper::getAttribute(root, "method"));

          switch (msgType) {
            case Message::MessageType_Invalid:                return MessagePtr();

            case Message::MessageType_Request:                return MessagePtr();
            case Message::MessageType_Result:
            {
              switch (msgMethod) {
                case Method_Invalid:                          return MessagePtr();

                case Method_ServicesGet:                      return ServicesGetResult::create(root, messageSource);
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
