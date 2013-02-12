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

#include <hookflash/stack/internal/stack_RSAPublicKey.h>
#include <hookflash/stack/internal/stack_RSAPrivateKey.h>
#include <zsLib/Log.h>
#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>


#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef CryptoPP::ByteQueue ByteQueue;
      typedef CryptoPP::AutoSeededRandomPool AutoSeededRandomPool;
      typedef CryptoPP::RSASSA_PKCS1v15_SHA_Verifier Verifier;

      typedef CryptoPP::RSAES_OAEP_SHA_Decryptor Decryptor;
      typedef CryptoPP::RSAES_OAEP_SHA_Encryptor Encryptor;

      using CryptoPP::PK_EncryptorFilter;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRSAPublicKeyForRSAPrivateKey
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPublicKeyPtr IRSAPublicKeyForRSAPrivateKey::load(const SecureByteBlock &buffer)
      {
        return IRSAPublicKeyFactory::singleton().loadPublicKey(buffer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RSAPublicKey
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPublicKey::RSAPublicKey() :
        mID(zsLib::createPUID())
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      RSAPublicKey::~RSAPublicKey()
      {
        if(isNoop()) return;
        
        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      RSAPublicKeyPtr RSAPublicKey::convert(IRSAPublicKeyPtr publicKey)
      {
        return boost::dynamic_pointer_cast<RSAPublicKey>(publicKey);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RSAPublicKey => IRSAPublicKey
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPublicKeyPtr RSAPublicKey::generate(RSAPrivateKeyPtr &outPrivatekey)
      {
        RSAPublicKeyPtr result;
        outPrivatekey = IRSAPrivateKeyForRSAPublicKey::generate(result);
        return result;
      }

      //-----------------------------------------------------------------------
      RSAPublicKeyPtr RSAPublicKey::load(const SecureByteBlock &buffer)
      {
        AutoSeededRandomPool rng;

        ByteQueue byteQueue;
        byteQueue.LazyPut(buffer.BytePtr(), buffer.SizeInBytes());
        byteQueue.FinalizeLazyPut();

        RSAPublicKeyPtr pThis(new RSAPublicKey());

        try
        {
          pThis->mPublicKey.Load(byteQueue);
          if (!pThis->mPublicKey.Validate(rng, 3)) {
            ZS_LOG_ERROR(Basic, "Failed to load an existing public key")
            return RSAPublicKeyPtr();
          }
        } catch (CryptoPP::Exception &e) {
          ZS_LOG_WARNING(Detail, String("Cryptography library threw an exception, reason=") + e.what())
          return RSAPublicKeyPtr();
        }

        return pThis;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr RSAPublicKey::save() const
      {
        SecureByteBlockPtr output(new SecureByteBlock);

        ByteQueue byteQueue;
        mPublicKey.Save(byteQueue);

        size_t outputLengthInBytes = (size_t)byteQueue.CurrentSize();
        output->CleanNew(outputLengthInBytes);

        byteQueue.Get(*output, outputLengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      bool RSAPublicKey::verify(
                                const SecureByteBlock &inOriginalBufferSigned,
                                const SecureByteBlock &inSignature
                                ) const
      {
        return verify(inOriginalBufferSigned, inOriginalBufferSigned.size(), inSignature);
      }

      //-----------------------------------------------------------------------
      bool RSAPublicKey::verify(
                                const String &inOriginalStringSigned,
                                const SecureByteBlock &inSignature
                                ) const
      {
        return verify((const BYTE *)inOriginalStringSigned.c_str(), inOriginalStringSigned.length(), inSignature);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr RSAPublicKey::encrypt(const SecureByteBlock &buffer) const
      {
        AutoSeededRandomPool rng;
        Encryptor encryptor(mPublicKey);

        SecureByteBlockPtr output(new SecureByteBlock);

        ByteQueue queue;
        queue.Put(buffer, buffer.SizeInBytes());

        ByteQueue *outputQueue = new ByteQueue;

        PK_EncryptorFilter filter(rng, encryptor, outputQueue);
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
      #pragma mark RSAPublicKey => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String RSAPublicKey::log(const char *message) const
      {
        return String("RSAPublicKey [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      bool RSAPublicKey::verify(
                                const BYTE *inBuffer,
                                size_t inBufferLengthInBytes,
                                const SecureByteBlock &inSignature
                                ) const
      {
        Verifier verifier(mPublicKey);

        try
        {
          bool result = verifier.VerifyMessage(inBuffer, inBufferLengthInBytes, inSignature, inSignature.size());
          if (!result) {
            ZS_LOG_WARNING(Detail, "Signature value did not pass")
            return false;
          }
        } catch (CryptoPP::Exception &e) {
          ZS_LOG_WARNING(Detail, String("Cryptography library threw an exception, reason=") + e.what())
          return false;
        }
        return true;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRSAPublicKey
    #pragma mark

    //-------------------------------------------------------------------------
    IRSAPublicKeyPtr IRSAPublicKey::generate(IRSAPrivateKeyPtr &outPrivateKey)
    {
      internal::RSAPrivateKeyPtr privateKey;
      IRSAPublicKeyPtr publicKey = internal::RSAPublicKey::generate(privateKey);
      outPrivateKey = privateKey;
      return publicKey;
    }

    //-------------------------------------------------------------------------
    IRSAPublicKeyPtr IRSAPublicKey::load(const SecureByteBlock &buffer)
    {
      return internal::IRSAPublicKeyFactory::singleton().loadPublicKey(buffer);
    }

  }
}
