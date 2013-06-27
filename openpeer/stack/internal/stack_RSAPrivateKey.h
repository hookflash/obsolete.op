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

#include <openpeer/stack/internal/types.h>
#include <openpeer/stack/IRSAPrivateKey.h>

#include <cryptopp/rsa.h>
#include <cryptopp/secblock.h>

#define HOOKFLASH_STACK_RSA_PRIVATE_KEY_GENERATION_SIZE (2048)

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPrivateKeyForRSAPublicKey
      #pragma mark

      interaction IRSAPrivateKeyForRSAPublicKey
      {
        IRSAPrivateKeyForRSAPublicKey &forRSAPublicKey() {return *this;}
        const IRSAPrivateKeyForRSAPublicKey &forRSAPublicKey() const {return *this;}

        static RSAPrivateKeyPtr generate(RSAPublicKeyPtr &outPublicKey);
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RSAPrivateKey
      #pragma mark

      class RSAPrivateKey : public Noop,
                            public IRSAPrivateKey
      {
      public:
        friend interaction IRSAPrivateKeyFactory;

        typedef CryptoPP::RSA::PrivateKey PrivateKey;

      protected:
        RSAPrivateKey();
        
        RSAPrivateKey(Noop) : Noop(true) {};

      public:
        ~RSAPrivateKey();

        static RSAPrivateKeyPtr convert(IRSAPrivateKeyPtr privateKey);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RSAPrivateKey => IRSAPrivateKey
        #pragma mark

        static RSAPrivateKeyPtr generate(RSAPublicKeyPtr &outPublicKey);

        static RSAPrivateKeyPtr load(const SecureByteBlock &buffer);

        virtual SecureByteBlockPtr save() const;

        virtual SecureByteBlockPtr sign(const SecureByteBlock &inBufferToSign) const;

        virtual SecureByteBlockPtr sign(const String &stringToSign) const;

        virtual SecureByteBlockPtr decrypt(const SecureByteBlock &buffer) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RSAPrivateKey => (internal)
        #pragma mark

        String log(const char *message) const;

        virtual SecureByteBlockPtr sign(
                                        const BYTE *inBuffer,
                                        size_t inBufferSizeInBytes
                                        ) const;

      private:
        //-------------------------------------------------------------------
        #pragma mark
        #pragma mark RSAPrivateKey => (data)
        #pragma mark

        PUID mID;

        PrivateKey mPrivateKey;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPrivateKeyFactory
      #pragma mark

      interaction IRSAPrivateKeyFactory
      {
        static IRSAPrivateKeyFactory &singleton();

        virtual RSAPrivateKeyPtr generate(RSAPublicKeyPtr &outPublicKey);

        virtual RSAPrivateKeyPtr loadPrivateKey(const SecureByteBlock &buffer);
      };
      
    }
  }
}
