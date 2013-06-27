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

#include <openpeer/stack/message/Message.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      class MessageResult : public Message
      {
      public:
        friend class Message;

        typedef WORD ErrorCodeType;

        enum AttributeTypes
        {
          AttributeType_Time,
          AttributeType_ErrorCode,
          AttributeType_ErrorReason,

          AttributeType_Last = AttributeType_ErrorReason,
        };

      public:
        static MessageResultPtr convert(MessagePtr message);

        static MessageResultPtr create(
                                       MessagePtr requestOrNotify,
                                       WORD errorCode = 0,
                                       const char *reason = NULL
                                       );

        static MessageResultPtr create(
                                       ElementPtr root,
                                       WORD errorCode = 0,
                                       const char *reason = NULL
                                       );

        static MessageResultPtr createOnlyIfError(ElementPtr root);

        virtual DocumentPtr encode();

        virtual MessageTypes messageType() const    {return Message::MessageType_Result;}
        virtual Methods method() const              {return mOriginalMethod;}

        virtual IMessageFactoryPtr factory() const  {return mOriginalFactory;}

        virtual bool isResult() const               {return true;}

        bool hasAttribute(AttributeTypes type) const;

        Time time() const                           {return mTime;}
        void time(Time val)                         {mTime = val;}

        ErrorCodeType errorCode() const             {return mErrorCode;}
        void errorCode(WORD val)                    {mErrorCode = val;}

        String errorReason() const                  {return mErrorReason;}
        void errorReason(const char *reason)        {mErrorReason = String(reason ? reason : "");}

        bool hasError() const                       {return (mErrorCode > 0) || (!mErrorReason.isEmpty());}

      protected:
        MessageResult();

        Time mTime;
        ErrorCodeType mErrorCode;
        String mErrorReason;

      private:
        Methods mOriginalMethod;
        IMessageFactoryPtr mOriginalFactory;
      };
    }
  }
}
