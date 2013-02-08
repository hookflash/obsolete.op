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

#include <zsLib/zsTypes.h>
#include <hookflash/stack/message/hookflashTypes.h>
#include <zsLib/String.h>
#include <zsLib/XML.h>


namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      class Message
      {
      public:
        typedef zsLib::String String;
        typedef zsLib::XML::DocumentPtr DocumentPtr;
        typedef zsLib::XML::ElementPtr ElementPtr;

      public:
        enum MessageTypes
        {
          MessageType_Invalid = -1,

          MessageType_Request,
          MessageType_Result,
          MessageType_Notify,
          MessageType_Reply,

          MessageType_Last = MessageType_Reply,
        };

        static const char *toString(MessageTypes type);
        static MessageTypes toMessageType(const char *type);

        enum Methods
        {
          Method_Invalid = -1,
        };

      public:
        static MessagePtr create(DocumentPtr document);
        static MessagePtr create(ElementPtr root);

        virtual DocumentPtr encode(IPeerFilesPtr peerFile = IPeerFilesPtr());

        virtual MessageTypes messageType() const = 0;

        virtual Methods method() const              {return Method_Invalid;}
        virtual const char *methodAsString() const;

        virtual const String &messageID() const     {return mID;}
        virtual void messageID(const String &id)    {mID = id;}

        virtual bool isRequest() const              {return false;}
        virtual bool isResult() const               {return false;}
        virtual bool isNotify() const               {return false;}
        virtual bool isReply() const                {return false;}

        virtual IMessageFactoryPtr factory() const  {return IMessageFactoryPtr();}

      protected:
        Message();

        String mID;
      };
    }
  }
}
