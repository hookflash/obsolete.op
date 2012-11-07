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
#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/message/IMessageFactory.h>
#include <hookflash/stack/message/internal/stack_message_MessageFactoryManager.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/Log.h>

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace internal
      {
        using zsLib::Stringize;

        typedef zsLib::WORD WORD;
        typedef zsLib::String String;
        typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
        typedef zsLib::XML::ElementPtr ElementPtr;

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
        MessagePtr MessageFactoryManager::create(ElementPtr root)
        {
          MessageFactoryManagerPtr pThis = singleton();
          return pThis->internalCreate(root);
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManager => friend class MessageResult
        #pragma mark

        //---------------------------------------------------------------------
        void MessageFactoryManager::getMethod(
                                              ElementPtr root,
                                              Message::Methods &outMethod,
                                              IMessageFactoryPtr &outFactory
                                              )
        {
          MessageFactoryManagerPtr pThis = singleton();
          pThis->internalGetMethod(root, outMethod, outFactory);
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
          return String("MessageFactoryManager [") + Stringize<PUID>(mID).string() + "] " + message;
        }

        //---------------------------------------------------------------------
        void MessageFactoryManager::internalRegisterFactory(IMessageFactoryPtr factory)
        {
          AutoRecursiveLock lock(mLock);
          mFactories.push_back(factory);
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryManager::internalCreate(ElementPtr root)
        {
          AutoRecursiveLock lock(mLock);

          MessagePtr result;

          try {
            Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
            if (Message::MessageType_Invalid == msgType) return MessagePtr();

            result = MessageResult::createIfError(root);
            if (result) return result;

            String xmlnsStr = IMessageHelper::getAttribute(root, "xmlns");

            for (FactoryList::iterator iter = mFactories.begin(); iter != mFactories.end(); ++iter)
            {
              IMessageFactoryPtr &factory = (*iter);
              const char *xmlnsFactory = factory->xmlns();

              if (xmlnsStr != xmlnsFactory) continue;

              result = factory->create(root);
              if (result) break;
            }

          } catch (zsLib::XML::Exceptions::CheckFailed &) {
          }
          return result;
        }

        //---------------------------------------------------------------------
        void MessageFactoryManager::internalGetMethod(
                                                      ElementPtr root,
                                                      Message::Methods &outMethod,
                                                      IMessageFactoryPtr &outFactory
                                                      )
        {
          AutoRecursiveLock lock(mLock);

          outMethod = Message::Method_Invalid;
          outFactory = IMessageFactoryPtr();

          String xmlnsStr = IMessageHelper::getAttribute(root, "xmlns");
          String methodStr = IMessageHelper::getAttribute(root, "method");

          for (FactoryList::iterator iter = mFactories.begin(); iter != mFactories.end(); ++iter)
          {
            IMessageFactoryPtr &factory = (*iter);
            const char *xmlnsFactory = factory->xmlns();

            if (xmlnsStr != xmlnsFactory) continue;

            Message::Methods method = factory->toMethod(methodStr);
            if (Message::Method_Invalid != method) {
              outMethod = method;
              outFactory = factory;
              return;
            }
          }
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
