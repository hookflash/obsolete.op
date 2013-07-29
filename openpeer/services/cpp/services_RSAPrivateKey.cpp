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

#include <openpeer/services/internal/services_RSAPrivateKey.h>
#include <openpeer/services/internal/services_RSAPublicKey.h>
#include <openpeer/services/IHelper.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>


namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      using zsLib::Stringize;

      using CryptoPP::ByteQueue;
      using CryptoPP::AutoSeededRandomPool;
      typedef CryptoPP::RSA::PublicKey PublicKey;
      typedef CryptoPP::RSASSA_PKCS1v15_SHA_Signer Signer;

      typedef CryptoPP::RSAES_OAEP_SHA_Decryptor Decryptor;
      typedef CryptoPP::RSAES_OAEP_SHA_Encryptor Encryptor;

      using CryptoPP::PK_DecryptorFilter;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPrivateKeyForRSAPublicKey
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr IRSAPrivateKeyForRSAPublicKey::generate(RSAPublicKeyPtr &outPublicKey)
      {
        return IRSAPrivateKeyFactory::singleton().generate(outPublicKey);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RSAPrivateKey
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPrivateKey::RSAPrivateKey() :
        mID(zsLib::createPUID())
      {
        ZS_LOG_DEBUG("created")
      }

      //-----------------------------------------------------------------------
      RSAPrivateKey::~RSAPrivateKey()
      {
        if(isNoop()) return;
        
        ZS_LOG_DEBUG("destoyed")
      }

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr RSAPrivateKey::convert(IRSAPrivateKeyPtr privateKey)
      {
        return boost::dynamic_pointer_cast<RSAPrivateKey>(privateKey);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RSAPrivateKey => IRSAPrivateKey
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr RSAPrivateKey::generate(
                                               RSAPublicKeyPtr &outPublicKey,
                                               ULONG keySizeInBites
                                               )
      {
        AutoSeededRandomPool rng;
        SecureByteBlock publicKeyBuffer;

        RSAPrivateKeyPtr pThis(new RSAPrivateKey);

        pThis->mPrivateKey.GenerateRandomWithKeySize(rng, keySizeInBites);
        if (!pThis->mPrivateKey.Validate(rng, 3)) {
          ZS_LOG_ERROR(Basic, "failed to generate a new private key")
          return RSAPrivateKeyPtr();
        }

        PublicKey rsaPublic(pThis->mPrivateKey);
        if (!rsaPublic.Validate(rng, 3)) {
          ZS_LOG_ERROR(Basic, "Failed to generate a public key for the new private key")
          return RSAPrivateKeyPtr();
        }

        ByteQueue byteQueue;
        rsaPublic.Save(byteQueue);

        size_t outputLengthInBytes = (size_t)byteQueue.CurrentSize();
        publicKeyBuffer.CleanNew(outputLengthInBytes);

        byteQueue.Get(publicKeyBuffer, outputLengthInBytes);

        outPublicKey = IRSAPublicKeyForRSAPrivateKey::load(publicKeyBuffer);
        return pThis;
      }

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr RSAPrivateKey::load(const SecureByteBlock &buffer)
      {
        AutoSeededRandomPool rng;

        ByteQueue byteQueue;
        byteQueue.LazyPut(buffer.BytePtr(), buffer.SizeInBytes());
        byteQueue.FinalizeLazyPut();

        RSAPrivateKeyPtr pThis(new RSAPrivateKey);

        try {
          pThis->mPrivateKey.Load(byteQueue);
          if (!pThis->mPrivateKey.Validate(rng, 3)) {
            ZS_LOG_ERROR(Basic, "Failed to load an existing private key")
            return RSAPrivateKeyPtr();
          }
        } catch (CryptoPP::Exception &e) {
          ZS_LOG_ERROR(Basic, String("cryptography library threw an exception, reason=") + e.what())
          return RSAPrivateKeyPtr();
        }

        return pThis;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr RSAPrivateKey::save() const
      {
        SecureByteBlockPtr output(new SecureByteBlock);
        ByteQueue byteQueue;
        mPrivateKey.Save(byteQueue);

        size_t outputLengthInBytes = (size_t)byteQueue.CurrentSize();
        output->CleanNew(outputLengthInBytes);

        byteQueue.Get(*output, outputLengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr RSAPrivateKey::sign(const SecureByteBlock &inBufferToSign) const
      {
        return sign(inBufferToSign, inBufferToSign.size());
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr RSAPrivateKey::sign(const String &inStrDataToSign) const
      {
        return sign((const BYTE *)(inStrDataToSign.c_str()), inStrDataToSign.length());
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr RSAPrivateKey::decrypt(const SecureByteBlock &buffer) const
      {
        AutoSeededRandomPool rng;
        Decryptor decryptor(mPrivateKey);

        SecureByteBlockPtr output(new SecureByteBlock);

        ByteQueue queue;
        queue.Put(buffer, buffer.SizeInBytes());

        ByteQueue *outputQueue = new ByteQueue;

        PK_DecryptorFilter filter(rng, decryptor, outputQueue);
        queue.CopyTo(filter);
        filter.MessageEnd();

        size_t outputLengthInBytes = (size_t)outputQueue->CurrentSize();
        output->CleanNew(outputLengthInBytes);

        outputQueue->Get(*output, outputLengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RSAPrivateKey => IRSAPrivateKey
      #pragma mark

      //-----------------------------------------------------------------------
      String RSAPrivateKey::log(const char *message) const
      {
        return String("RSAPrivateKey [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr RSAPrivateKey::sign(
                                             const BYTE *inBuffer,
                                             size_t inBufferSizeInBytes
                                             ) const
      {
        SecureByteBlockPtr output(new SecureByteBlock);

        AutoSeededRandomPool rng;

        Signer signer(mPrivateKey);

        size_t length = signer.MaxSignatureLength();

        output->CleanNew(length);

        signer.SignMessage(rng, inBuffer, inBufferSizeInBytes, *output);

        return output;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRSAPrivateKey
    #pragma mark

    //-------------------------------------------------------------------------
    IRSAPrivateKeyPtr IRSAPrivateKey::generate(
                                               IRSAPublicKeyPtr &outPublicKey,
                                               ULONG keySizeInBites
                                               )
    {
      internal::RSAPublicKeyPtr publicKey;
      IRSAPrivateKeyPtr result = internal::IRSAPrivateKeyFactory::singleton().generate(publicKey, keySizeInBites);
      outPublicKey = publicKey;
      return result;
    }

    //-------------------------------------------------------------------------
    IRSAPrivateKeyPtr IRSAPrivateKey::load(const SecureByteBlock &buffer)
    {
      return internal::IRSAPrivateKeyFactory::singleton().loadPrivateKey(buffer);
    }
  }
}
