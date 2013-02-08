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

#include <hookflash/services/hookflashTypes.h>

#include <zsLib/Proxy.h>

namespace hookflash
{
  namespace services
  {
    interaction IHTTP
    {
      typedef zsLib::BYTE BYTE;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::String String;
      typedef zsLib::Duration Duration;
      typedef zsLib::Seconds Seconds;

      static IHTTPQueryPtr get(
                               IHTTPQueryDelegatePtr delegate,
                               const char *userAgent,
                               const char *url,
                               Duration timeout = Duration()
                               );

      static IHTTPQueryPtr post(
                                IHTTPQueryDelegatePtr delegate,
                                const char *userAgent,
                                const char *url,
                                const char *postData,
                                const char *postDataMimeType = NULL,
                                Duration timeout = Duration()
                                );

      static IHTTPQueryPtr post(
                                IHTTPQueryDelegatePtr delegate,
                                const char *userAgent,
                                const char *url,
                                const BYTE *postData,
                                ULONG postDataLengthInBytes,
                                const char *postDataMimeType = NULL,
                                Duration timeout = Duration()
                                );
    };

    interaction IHTTPQuery
    {
      typedef zsLib::PUID PUID;
      typedef zsLib::BYTE BYTE;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::String String;

      virtual PUID getID() const = 0;

      virtual void cancel() = 0;

      virtual bool isComplete() const = 0;
      virtual bool wasSuccessful() const = 0;
      virtual long getResponseCode() const = 0;

      virtual ULONG getHeaderReadSizeAvailableInBtytes() const = 0;
      virtual ULONG readHeader(
                               BYTE *outResultData,
                               ULONG bytesToRead
                               ) = 0;

      virtual ULONG readHeaderAsString(String &outHeader) = 0;

      virtual ULONG getReadDataAvailableInBytes() const = 0;

      virtual ULONG readData(
                             BYTE *outResultData,
                             ULONG bytesToRead
                             ) = 0;

      virtual ULONG readDataAsString(String &outResultData) = 0;
    };

    interaction IHTTPQueryDelegate
    {
      virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query) = 0;
      virtual void onHTTPComplete(IHTTPQueryPtr query) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::IHTTPQueryDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::services::IHTTPQueryPtr, IHTTPQueryPtr)
ZS_DECLARE_PROXY_METHOD_1(onHTTPReadDataAvailable, IHTTPQueryPtr)
ZS_DECLARE_PROXY_METHOD_1(onHTTPComplete, IHTTPQueryPtr)
ZS_DECLARE_PROXY_END()
