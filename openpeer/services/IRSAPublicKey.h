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

#include <openpeer/stack/types.h>

#define OPENPEER_SERVICES_JSON_SIGNATURE_ALGORITHM "http://meta.openpeer.org/2012/12/14/jsonsig#rsa-sha1"

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRSAPublicKey
    #pragma mark

    interaction IRSAPublicKey
    {
      static IRSAPublicKeyPtr generate(IRSAPrivateKeyPtr &outPrivateKey);

      static IRSAPublicKeyPtr load(const SecureByteBlock &buffer);

      virtual SecureByteBlockPtr save() const = 0;

      virtual String getFingerprint() const = 0;

      virtual bool verify(
                          const SecureByteBlock &inOriginalBufferSigned,
                          const SecureByteBlock &inSignature
                          ) const = 0;

      virtual bool verify(
                          const String &inOriginalStringSigned,
                          const SecureByteBlock &inSignature
                          ) const = 0;

      virtual bool verifySignature(ElementPtr signedEl) const = 0;

      virtual SecureByteBlockPtr encrypt(const SecureByteBlock &buffer) const = 0;
    };
  }
}
