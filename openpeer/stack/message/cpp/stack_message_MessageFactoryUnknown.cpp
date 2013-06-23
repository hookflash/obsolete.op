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

#include <hookflash/stack/message/internal/stack_message_MessageFactoryUnknown.h>
#include <hookflash/stack/message/internal/stack_message_MessageRequestUnknown.h>
#include <hookflash/stack/message/internal/stack_message_MessageNotifyUnknown.h>

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>


namespace hookflash
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
        #pragma mark MessageFactoryUnknown
        #pragma mark

        //---------------------------------------------------------------------
        MessageFactoryUnknown::MessageFactoryUnknown(
                                                     const char *handler,
                                                     const char *method,
                                                     Methods methodCode
                                                     ) :
          mHandler(handler ? String(handler) : String()),
          mMethod(method ? String(method) : String()),
          mMethodCode(methodCode)
        {
        }

        //---------------------------------------------------------------------
        MessageFactoryUnknownPtr MessageFactoryUnknown::convert(IMessageFactoryPtr factory)
        {
          return boost::dynamic_pointer_cast<MessageFactoryUnknown>(factory);
        }

        //---------------------------------------------------------------------
        MessageFactoryUnknownPtr MessageFactoryUnknown::create(
                                                               const char *handler,
                                                               const char *method,
                                                               Methods methodCode
                                                               )
        {
          MessageFactoryUnknownPtr pThis(new MessageFactoryUnknown(handler, method, methodCode));
          pThis->mThisWeak = pThis;
          return pThis;
        }

        //---------------------------------------------------------------------
        Message::Methods MessageFactoryUnknown::toMethod(const char *method) const
        {
          if (mMethod == method) return (Message::Methods)mMethodCode;
          return Message::Method_Invalid;
        }

        //---------------------------------------------------------------------
        const char *MessageFactoryUnknown::toString(Message::Methods method) const
        {
          return mMethod;
        }

        //---------------------------------------------------------------------
        MessagePtr MessageFactoryUnknown::create(
                                                 ElementPtr root,
                                                 IMessageSourcePtr messageSource
                                                 )
        {
          if (!root) return MessagePtr();

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          switch (msgType) {
            case Message::MessageType_Request:  return MessageRequestUnknown::create(mThisWeak.lock(), (Message::MessageTypes)Method_Unknown, root, messageSource);
            case Message::MessageType_Notify:   return MessageNotifyUnknown::create(mThisWeak.lock(), (Message::MessageTypes)Method_Unknown, root, messageSource);
            default:                            break;
          }
          return MessagePtr();
        }


      }
    }
  }
}
