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

#include <openpeer/services/internal/types.h>
#include <openpeer/services/IRSAPublicKey.h>

#include <cryptopp/rsa.h>
#include <cryptopp/secblock.h>

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPublicKeyForRSAPrivateKey
      #pragma mark

      interaction IRSAPublicKeyForRSAPrivateKey
      {
        IRSAPublicKeyForRSAPrivateKey &forPrivateKey() {return *this;}
        const IRSAPublicKeyForRSAPrivateKey &forPrivateKey() const {return *this;}

        static RSAPublicKeyPtr load(const SecureByteBlock &buffer);
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RSAPublicKey
      #pragma mark

      class RSAPublicKey : public Noop,
                           public IRSAPublicKey,
                           public IRSAPublicKeyForRSAPrivateKey
      {
      public:
        friend interaction IRSAPublicKeyFactory;
        friend interaction IRSAPublicKey;

        typedef CryptoPP::RSA::PublicKey PublicKey;

      protected:
        RSAPublicKey();
        
        RSAPublicKey(Noop) : Noop(true) {};

      public:
        ~RSAPublicKey();

        static RSAPublicKeyPtr convert(IRSAPublicKeyPtr publicKey);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RSAPublicKey => IRSAPublicKey
        #pragma mark

        static RSAPublicKeyPtr generate(RSAPrivateKeyPtr &outPrivatekey);

        static RSAPublicKeyPtr load(const SecureByteBlock &buffer);

        virtual SecureByteBlockPtr save() const;

        virtual String getFingerprint() const;

        virtual bool verify(
                            const SecureByteBlock &inOriginalBufferSigned,
                            const SecureByteBlock &inSignature
                            ) const;

        virtual bool verify(
                            const String &inOriginalStringSigned,
                            const SecureByteBlock &inSignature
                            ) const;

        virtual bool verifySignature(ElementPtr signedEl) const;

        virtual SecureByteBlockPtr encrypt(const SecureByteBlock &buffer) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RSAPublicKey => (internal)
        #pragma mark

        String log(const char *message) const;

        bool verify(
                    const BYTE *inBuffer,
                    size_t inBufferLengthInBytes,
                    const SecureByteBlock &inSignature
                    ) const;

      private:
        //-------------------------------------------------------------------
        #pragma mark
        #pragma mark RSAPrivateKey => (data)
        #pragma mark

        PUID mID;
        PublicKey mPublicKey;
        String mFingerprint;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPublicKeyFactory
      #pragma mark

      interaction IRSAPublicKeyFactory
      {
        static IRSAPublicKeyFactory &singleton();

        virtual RSAPublicKeyPtr loadPublicKey(const SecureByteBlock &buffer);
      };
    }
  }
}
