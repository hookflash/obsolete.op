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

#pragma once

#include <hookflash/stack/message/Message.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      class MessageResult : public Message
      {
      public:
        typedef zsLib::Time Time;
        typedef zsLib::WORD WORD;

        friend class Message;

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
                                       WORD errorCode,
                                       const char *reason
                                       );

        static MessageResultPtr createIfError(ElementPtr root);

        virtual DocumentPtr encode(IPeerFilesPtr peerFile = IPeerFilesPtr());

        virtual MessageTypes messageType() const    {return Message::MessageType_Result;}
        virtual Methods method() const              {return mOriginalMethod;}

        virtual IMessageFactoryPtr factory() const  {return mOriginalFactory;}

        virtual bool isResult() const               {return true;}

        bool hasAttribute(AttributeTypes type) const;

        Time time() const                           {return mTime;}
        void setTime(Time val)                      {mTime = val;}

        WORD errorCode() const                      {return mErrorCode;}
        void setErrorCode(WORD val)                 {mErrorCode = val;}

        String errorReason() const                  {return mErrorReason;}
        void setErrorReason(const char *reason)     {mErrorReason = String(reason ? reason : "");}

        bool hasError() const                       {return (mErrorCode > 0) || (!mErrorReason.isEmpty());}

      protected:
        MessageResult();

        Time mTime;
        WORD mErrorCode;
        String mErrorReason;

      private:
        Methods mOriginalMethod;
        IMessageFactoryPtr mOriginalFactory;
      };
    }
  }
}
