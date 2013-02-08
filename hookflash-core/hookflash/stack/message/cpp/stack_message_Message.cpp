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
#include <hookflash/stack/message/IMessageFactory.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/internal/stack_message_MessageFactoryManager.h>

#include <zsLib/Log.h>

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      //-----------------------------------------------------------------------
      const char *Message::toString(MessageTypes type)
      {
        switch (type)
        {
          case MessageType_Invalid:     return "";
          case MessageType_Request:     return "request";
          case MessageType_Result:      return "result";
          case MessageType_Notify:      return "notify";
          case MessageType_Reply:       return "reply";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      Message::MessageTypes Message::toMessageType(const char *inType)
      {
        typedef zsLib::ULONG ULONG;
        String type(inType ? inType : "");

        for (ULONG loop = (ULONG)(Message::MessageType_Invalid+1); loop <= ((ULONG)Message::MessageType_Last); ++loop)
        {
          if (type == toString((Message::MessageTypes)loop)) {
            return (Message::MessageTypes)loop;
          }
        }
        return Message::MessageType_Invalid;
      }

      //-----------------------------------------------------------------------
      Message::Message()
      {
        mID = hookflash::services::IHelper::randomString(32);
      }

      //-----------------------------------------------------------------------
      MessagePtr Message::create(DocumentPtr document)
      {
        MessagePtr result;

        try
        {
          ElementPtr root = document->getFirstChildElementChecked();
          return Message::create(root);
        } catch(zsLib::XML::Exceptions::CheckFailed &) {
        }

        return result;
      }

      //-----------------------------------------------------------------------
      MessagePtr Message::create(ElementPtr root)
      {
        return internal::MessageFactoryManager::create(root);
      }

      //-----------------------------------------------------------------------
      const char *Message::methodAsString() const
      {
        IMessageFactoryPtr originalFactory = factory();
        if (!originalFactory) return "UNDEFINED";

        return originalFactory->toString(method());
      }

      //-----------------------------------------------------------------------
      DocumentPtr Message::encode(IPeerFilesPtr peerFile)
      {
        ZS_THROW_NOT_IMPLEMENTED("The encoder for this method is not implemented")
        return DocumentPtr();
      }
    }
  }
}
