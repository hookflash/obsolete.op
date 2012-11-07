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

#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/message/IMessageFactory.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/internal/stack_message_MessageFactoryManager.h>

#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      using zsLib::Stringize;

      typedef zsLib::WORD WORD;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef internal::MessageHelper MessageHelper;

      //-----------------------------------------------------------------------
      MessageResult::MessageResult() :
        mErrorCode(0),
        mOriginalMethod(Method_Invalid)
      {
      }

      //-----------------------------------------------------------------------
      MessageResultPtr MessageResult::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<MessageResult>(message);
      }

      //-----------------------------------------------------------------------
      MessageResultPtr MessageResult::create(
                                             MessagePtr requestOrNotify,
                                             WORD errorCode,
                                             const char *reason
                                             )
      {
        MessageResultPtr pThis(new MessageResult);
        pThis->mID = requestOrNotify->messageID();
        pThis->mOriginalMethod = requestOrNotify->method();
        pThis->mTime = zsLib::now();
        pThis->mErrorCode = errorCode,
        pThis->mErrorReason = String(reason ? reason : "");
        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageResultPtr MessageResult::createIfError(ElementPtr root)
      {
        MessageResultPtr result;

        try {
          if (!root) return result;

          Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
          if (MessageType_Result != msgType) return result;

          WORD errorCode = internal::MessageHelper::getErrorCode(root);
          String reason = internal::MessageHelper::getErrorReason(root);

          if ((0 == errorCode) &&
              (reason.isEmpty())) {
            return result;
          }

          result = MessageResultPtr(new MessageResult);

          result->mID = IMessageHelper::getAttributeID(root);
          result->setErrorCode(errorCode);
          result->setErrorReason(reason);
          result->mTime = IMessageHelper::getAttributeEpoch(root);
          internal::MessageFactoryManager::getMethod(root, result->mOriginalMethod, result->mOriginalFactory);
        } catch (zsLib::XML::Exceptions::CheckFailed &) {
        }

        return result;
      }

      //-----------------------------------------------------------------------
      DocumentPtr MessageResult::encode(IPeerFilesPtr peerFile)
      {
        DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
        ElementPtr root = ret->getFirstChildElement();

        // <error>
        // <reason id=”301”>Moved Permanently</reason>
        // </error>

        ElementPtr reason = IMessageHelper::createElementWithText("reason", mErrorReason);
        if (0 != mErrorCode) {
          IMessageHelper::setAttributeID(reason, Stringize<WORD>(mErrorCode).string());
        }

        ElementPtr error = IMessageHelper::createElement("error");
        error->adoptAsLastChild(reason);

        root->adoptAsLastChild(error);
        return ret;
      }

      //-----------------------------------------------------------------------
      bool MessageResult::hasAttribute(AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_Time:
            return (Time() != mTime);
          case AttributeType_ErrorCode:
            return (mErrorCode > 0);
          case AttributeType_ErrorReason:
            return (!mErrorReason.isEmpty());
          default:
            break;
        }
        return false;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
