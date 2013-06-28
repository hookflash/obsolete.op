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

#pragma once

#include <openpeer/stack/message/internal/types.h>
#include <openpeer/stack/message/IMessageFactoryManager.h>
#include <openpeer/stack/message/Message.h>

#include <openpeer/services/IHTTP.h>

#include <list>

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace internal
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageFactoryManager
        #pragma mark

        class MessageFactoryManager : public IMessageFactoryManager
        {
        public:
          typedef zsLib::PUID PUID;
          typedef zsLib::String String;
          typedef zsLib::RecursiveLock RecursiveLock;
          typedef zsLib::XML::ElementPtr ElementPtr;

          friend class stack::message::Message;
          friend class stack::message::MessageResult;

          enum ErrorCodes
          {
            ErrorCode_NotParsed        = IHTTP::HTTPStatusCode_BadRequest,
            ErrorCode_Unknown          = IHTTP::HTTPStatusCode_NotImplemented,
          };

          static const char *toString(ErrorCodes error);

        protected:
          MessageFactoryManager();
          void init();

          static MessageFactoryManagerPtr create();

        public:
          ~MessageFactoryManager();

          static MessageFactoryManagerPtr singleton();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MessageFactoryManager => IMessageFactoryManager
          #pragma mark

          static void registerFactory(IMessageFactoryPtr factory);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MessageFactoryManager => friend class Message
          #pragma mark

          static MessagePtr create(
                                   ElementPtr root,
                                   IMessageSourcePtr messageSource
                                   );

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MessageFactoryManager => friend class MessageResult
          #pragma mark

          static bool getMethod(
                                ElementPtr root,
                                Message::Methods &outMethod,
                                IMessageFactoryPtr &outFactory
                                );

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MessageFactoryManager => (internal)
          #pragma mark

          String log(const char *message);

          void internalRegisterFactory(IMessageFactoryPtr factory);
          MessagePtr internalCreate(
                                    ElementPtr root,
                                    IMessageSourcePtr messageSource
                                    );

          bool internalGetMethod(
                                 ElementPtr root,
                                 Message::Methods &outMethod,
                                 IMessageFactoryPtr &outFactory
                                 );

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark MessageFactoryManager => (data)
          #pragma mark

          mutable RecursiveLock mLock;
          PUID mID;
          MessageFactoryManagerWeakPtr mThisWeak;

          typedef std::list<IMessageFactoryPtr> FactoryList;
          FactoryList mFactories;
        };
      }
    }
  }
}
