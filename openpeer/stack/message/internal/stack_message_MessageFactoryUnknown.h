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
#include <openpeer/stack/message/IMessageFactory.h>

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

        class MessageFactoryUnknown : public IMessageFactory
        {
        public:
          enum Methods
          {
            Method_Invalid = Message::Method_Invalid,

            Method_NotParsed,
            Method_Unknown,

            Method_Last = Method_Unknown,
          };

        protected:
          MessageFactoryUnknown(
                                const char *handler,
                                const char *method,
                                Methods methodCode
                                );

        public:
          static MessageFactoryUnknownPtr convert(IMessageFactoryPtr factory);

          static MessageFactoryUnknownPtr create(
                                                 const char *handler,
                                                 const char *method,
                                                 Methods methodCode = Method_Unknown
                                                 );

          virtual const char *getHandler() const
          {
            return mHandler;
          }

          virtual Message::Methods toMethod(const char *method) const;
          virtual const char *toString(Message::Methods method) const;

          virtual MessagePtr create(
                                    ElementPtr root,
                                    IMessageSourcePtr messageSource
                                    );

          virtual Methods getMethod() const
          {
            return mMethodCode;
          }

        protected:
          MessageFactoryUnknownWeakPtr mThisWeak;

          String mHandler;
          String mMethod;
          Methods mMethodCode;
        };

      }
    }
  }
}
