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

#include <openpeer/stack/message/internal/stack_message_MessageFactoryManager.h>
#include <openpeer/stack/message/internal/stack_message_MessageFactoryUnknown.h>

#include <openpeer/stack/message/MessageRequest.h>
#include <openpeer/stack/message/MessageResult.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>
#include <openpeer/stack/message/IMessageFactory.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>

namespace openpeer { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(openpeer_stack_message) } } }

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace internal
      {
        typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManagerGlobal
        #pragma mark

        class MessageFactoryManagerGlobal
        {
        public:
          MessageFactoryManagerGlobal();

        protected:
        };

        static MessageFactoryManagerGlobal gMessageFactoryManagerGlobal;

        MessageFactoryManagerGlobal::MessageFactoryManagerGlobal()
        {
          static MessageFactoryManagerGlobal *pMessageFactoryManagerGlobal = &gMessageFactoryManagerGlobal;
          (void)pMessageFactoryManagerGlobal;

          MessageFactoryManager::singleton();
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManager
        #pragma mark

        //---------------------------------------------------------------------
        const char *MessageFactoryManager::toString(ErrorCodes error)
        {
          return IHTTP::toString(IHTTP::toStatusCode(error));
        }

        //---------------------------------------------------------------------
        MessageFactoryManager::MessageFactoryManager()
        {
        }

        //---------------------------------------------------------------------
        MessageFactoryManagerPtr MessageFactoryManager::create()
        {
          MessageFactoryManagerPtr pThis(new MessageFactoryManager);
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        void MessageFactoryManager::init()
        {
        }

        //---------------------------------------------------------------------
        MessageFactoryManager::~MessageFactoryManager()
        {
        }

        //---------------------------------------------------------------------
        MessageFactoryManagerPtr MessageFactoryManager::singleton()
        {
          static MessageFactoryManagerPtr singleton = create();
          return singleton;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManager => IMessageFactoryManager
        #pragma mark

        //---------------------------------------------------------------------
        void MessageFactoryManager::registerFactory(IMessageFactoryPtr factory)
        {
          MessageFactoryManagerPtr pThis = singleton();
          pThis->internalRegisterFactory(factory);
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManager => friend class Message
        #pragma mark

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryManager::create(
                                                 ElementPtr root,
                                                 IMessageSourcePtr messageSource
                                                 )
        {
          MessageFactoryManagerPtr pThis = singleton();
          return pThis->internalCreate(root, messageSource);
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManager => friend class MessageResult
        #pragma mark

        //---------------------------------------------------------------------
        bool MessageFactoryManager::getMethod(
                                              ElementPtr root,
                                              Message::Methods &outMethod,
                                              IMessageFactoryPtr &outFactory
                                              )
        {
          MessageFactoryManagerPtr pThis = singleton();
          return pThis->internalGetMethod(root, outMethod, outFactory);
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManager => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        String MessageFactoryManager::log(const char *message)
        {
          return String("MessageFactoryManager [") + string(mID) + "] " + message;
        }

        //---------------------------------------------------------------------
        void MessageFactoryManager::internalRegisterFactory(IMessageFactoryPtr factory)
        {
          AutoRecursiveLock lock(mLock);
          mFactories.push_back(factory);
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryManager::internalCreate(
                                                         ElementPtr root,
                                                         IMessageSourcePtr messageSource
                                                         )
        {
          AutoRecursiveLock lock(mLock);

          if (!root) {
            ZS_LOG_WARNING(Detail, log("cannot create message from null element"))
            return MessagePtr();
          }

          MessagePtr message;
          String handlerStr = IMessageHelper::getAttribute(root, "handler");

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          if (Message::MessageType_Invalid == msgType) return MessagePtr();

          bool foundHandler = false;

          try {
            message = MessageResult::createOnlyIfError(root);
            if (message) return message;

            for (FactoryList::iterator iter = mFactories.begin(); iter != mFactories.end(); ++iter)
            {
              IMessageFactoryPtr &factory = (*iter);
              const char *handlerFactory = factory->getHandler();

              if (handlerStr != handlerFactory) continue;

              message = factory->create(root, messageSource);
              if (message) break;

              foundHandler = true;
            }
          } catch (CheckFailed &) {
          }

          if (!message) {
            switch (msgType) {
              case Message::MessageType_Invalid:  break;
              case Message::MessageType_Request:
              case Message::MessageType_Notify:
              {
                ZS_LOG_WARNING(Detail, log("message request or notify was not understood so generating generic message"))
                MessageFactoryUnknownPtr unknown = MessageFactoryUnknown::create(handlerStr, IMessageHelper::getAttribute(root, "method"), (foundHandler ? MessageFactoryUnknown::Method_NotParsed : MessageFactoryUnknown::Method_Unknown));
                message = unknown->create(root, messageSource);
                break;
              }
              case Message::MessageType_Result:
              {
                ZS_LOG_WARNING(Detail, log("message result received but was not understood, converting to an error message"))
                ErrorCodes error = (foundHandler ? ErrorCode_NotParsed : ErrorCode_Unknown);
                message = MessageResult::create(root, error, toString(error));
                break;
              }
            }
          }

          if (message) {
            message->creationElement(root);
            message->creationSource(messageSource);
            return message;
          }

          ZS_LOG_WARNING(Detail, log("cannot generate message for unknown reply"))
          return MessagePtr();
        }

        //---------------------------------------------------------------------
        bool MessageFactoryManager::internalGetMethod(
                                                      ElementPtr root,
                                                      Message::Methods &outMethod,
                                                      IMessageFactoryPtr &outFactory
                                                      )
        {
          AutoRecursiveLock lock(mLock);

          outMethod = Message::Method_Invalid;
          outFactory = IMessageFactoryPtr();

          String handlerStr = IMessageHelper::getAttribute(root, "handler");
          String methodStr = IMessageHelper::getAttribute(root, "method");

          for (FactoryList::iterator iter = mFactories.begin(); iter != mFactories.end(); ++iter)
          {
            IMessageFactoryPtr &factory = (*iter);
            const char *handlerFactory = factory->getHandler();

            if (handlerStr != handlerFactory) continue;

            Message::Methods method = factory->toMethod(methodStr);
            if (Message::Method_Invalid != method) {
              outMethod = method;
              outFactory = factory;
              return true;
            }
          }
          return false;
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageFactoryManager
      #pragma mark

      //-----------------------------------------------------------------------
      void IMessageFactoryManager::registerFactory(IMessageFactoryPtr factory)
      {
        internal::MessageFactoryManager::registerFactory(factory);
      }
    }
  }
}
