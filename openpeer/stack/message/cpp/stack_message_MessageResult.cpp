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

#include <openpeer/stack/message/MessageResult.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>
#include <openpeer/stack/message/internal/stack_message_MessageFactoryManager.h>
#include <openpeer/stack/message/internal/stack_message_MessageFactoryUnknown.h>

#include <openpeer/services/IHTTP.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>
#include <zsLib/XML.h>

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      using zsLib::Stringize;
      using internal::MessageFactoryUnknown;
      using internal::MessageFactoryUnknownPtr;
      using internal::MessageFactoryManager;

      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      //-----------------------------------------------------------------------
      MessageResult::MessageResult() :
        mTime(zsLib::now()),
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
        if ((!requestOrNotify->isRequest()) &&
            (!requestOrNotify->isNotify())) {
          return MessageResultPtr();
        }
        MessageResultPtr pThis(new MessageResult);
        pThis->mDomain = requestOrNotify->domain();
        pThis->mAppID = requestOrNotify->appID();
        pThis->mID = requestOrNotify->messageID();
        pThis->mOriginalMethod = requestOrNotify->method();
        pThis->mOriginalFactory = requestOrNotify->factory();
        pThis->mErrorCode = errorCode;
        pThis->mErrorReason = String(reason ? String(reason) : String());
        if ((0 != errorCode) &&
            (pThis->mErrorReason.isEmpty())) {
          pThis->mErrorReason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if ((0 == pThis->mErrorCode) &&
            (pThis->mErrorReason.isEmpty())) {
          MessageFactoryUnknownPtr unknownFactory = MessageFactoryUnknown::convert(pThis->mOriginalFactory);
          if (unknownFactory) {
            MessageFactoryManager::ErrorCodes errorCode = MessageFactoryManager::ErrorCode_Unknown;
            switch (unknownFactory->getMethod())
            {
              case MessageFactoryUnknown::Method_Invalid:   break;
              case MessageFactoryUnknown::Method_NotParsed: errorCode = MessageFactoryManager::ErrorCode_NotParsed;
              case MessageFactoryUnknown::Method_Unknown:   errorCode = MessageFactoryManager::ErrorCode_Unknown;
            }
            pThis->mErrorCode = (WORD)errorCode;
            pThis->mErrorReason = MessageFactoryManager::toString(errorCode);
          }
        }
        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageResultPtr MessageResult::create(
                                             ElementPtr root,
                                             WORD errorCode,
                                             const char *reason
                                             )
      {
        MessageResultPtr pThis(new MessageResult);

        pThis->mDomain = IMessageHelper::getAttribute(root, "domain");
        pThis->mAppID = IMessageHelper::getAttribute(root, "appid");
        pThis->mID = IMessageHelper::getAttributeID(root);
        pThis->mErrorCode = errorCode;
        pThis->mErrorReason = String(reason ? String(reason) : String());
        if ((0 != errorCode) &&
            (pThis->mErrorReason.isEmpty())) {
          pThis->mErrorReason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }
        pThis->mTime = IMessageHelper::getAttributeEpoch(root);
        bool found = internal::MessageFactoryManager::getMethod(root, pThis->mOriginalMethod, pThis->mOriginalFactory);
        if (!found) {
          String handler = IMessageHelper::getAttribute(root, "handler");
          String method = IMessageHelper::getAttribute(root, "handler");
          MessageFactoryUnknownPtr unknownFactory = MessageFactoryUnknown::create(handler, method);
          pThis->mOriginalFactory = unknownFactory;
          pThis->mOriginalMethod = (Message::Methods)MessageFactoryUnknown::Method_Unknown;
        }
        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageResultPtr MessageResult::createOnlyIfError(ElementPtr root)
      {
        MessageResultPtr result;

        if (!root) return MessageResultPtr();

        Message::MessageTypes msgType = IMessageHelper::getMessageType(root);
        if (MessageType_Result != msgType) return result;

        WORD errorCode = internal::MessageHelper::getErrorCode(root);
        String reason = internal::MessageHelper::getErrorReason(root);

        if ((0 == errorCode) &&
            (reason.isEmpty())) {
          return MessageResultPtr();
        }

        return create(root, errorCode, reason);
      }

      //-----------------------------------------------------------------------
      DocumentPtr MessageResult::encode()
      {
        return IMessageHelper::createDocumentWithRoot(*this);
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
