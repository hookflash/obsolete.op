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

#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/IPeer.h>
#include <hookflash/services/IHelper.h>
#include <hookflash/stack/message/IMessageHelper.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>

#include <zsLib/RegEx.h>

#include <cryptopp/modes.h>
#include <cryptopp/hex.h>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>
#include <cryptopp/sha.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

using namespace zsLib::XML;

namespace hookflash
{
  namespace stack
  {
    using CryptoPP::CFB_Mode;
    using CryptoPP::HMAC;

    using CryptoPP::HexEncoder;
    using CryptoPP::HexDecoder;
    using CryptoPP::StringSink;
    using CryptoPP::ByteQueue;
    using CryptoPP::Base64Encoder;
    using CryptoPP::Base64Decoder;
    using CryptoPP::AES;
    using CryptoPP::Weak::MD5;
    using CryptoPP::SHA256;
    using CryptoPP::SHA1;
    using message::IMessageHelper;

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Helper
      #pragma mark

      //-----------------------------------------------------------------------
      String Helper::getDebugValue(const char *name, const String &value, bool &firstTime)
      {
        if (value.isEmpty()) return String();
        if (firstTime) {
          firstTime = false;
          return String(name) + "=" + value;
        }
        return String(", ") + name + "=" + value;
      }

      //-----------------------------------------------------------------------
      RecursiveLock &Helper::getGlobalLock()
      {
        return services::IHelper::getGlobalLock();
      }

      //-----------------------------------------------------------------------
      String Helper::randomString(UINT lengthInChars)
      {
        return services::IHelper::randomString(lengthInChars);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::random(UINT lengthInBytes)
      {
        return services::IHelper::random(lengthInBytes);
      }

      //-----------------------------------------------------------------------
      int Helper::compare(
                          const SecureByteBlock &left,
                          const SecureByteBlock &right
                          )
      {
        if (left.SizeInBytes() < right.SizeInBytes()) {
          return -1;
        }
        if (right.SizeInBytes() < left.SizeInBytes()) {
          return 1;
        }
        return memcmp(left, right, left.SizeInBytes());
      }

      //-----------------------------------------------------------------------
      String Helper::convertToString(const SecureByteBlock &buffer)
      {
        if (buffer.size() < 1) return String();

        // check if buffer ia already NUL terminated
        if ('\0' == (char)((buffer.BytePtr())[(sizeof(char)*buffer.size())-sizeof(char)])) {
          return (const char *)(buffer.BytePtr());  // return buffer cast as const char *
        }

        SecureByteBlock outputFinal;
        outputFinal.CleanNew(buffer.SizeInBytes()+sizeof(char));

        memcpy(outputFinal, buffer, buffer.SizeInBytes());

        return (const char *)((const BYTE *)(outputFinal));
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::convertToBuffer(
                                                 const char *input,
                                                 bool appendNUL
                                                 )
      {
        SecureByteBlockPtr output(new SecureByteBlock);
        output->CleanNew(((sizeof(char)*strlen(input)) + (appendNUL ? sizeof(char) : 0)));

        memcpy(*output, input, sizeof(char)*(strlen(input)));
        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::convertToBuffer(
                                                 const BYTE *buffer,
                                                 ULONG bufferLengthInBytes,
                                                 bool appendNULIfMissing
                                                 )
      {
        SecureByteBlockPtr output(new SecureByteBlock);

        ULONG finalLength = bufferLengthInBytes;
        if (bufferLengthInBytes) {
          if (0 != finalLength) {
            if (0 != buffer[bufferLengthInBytes-1]) {
              finalLength += sizeof(char);
            }
          } else {
            finalLength += sizeof(char);
          }
        }

        if (0 == finalLength) {
          return output;
        }

        output->CleanNew(finalLength);

        if (0 != bufferLengthInBytes) {
          memcpy(*output, buffer, finalLength);
        }
        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::makeBufferStringSafe(const SecureByteBlock &input)
      {
        return convertToBuffer(input.BytePtr(), input.SizeInBytes(), true);
      }

      //-----------------------------------------------------------------------
      String Helper::convertToBase64(
                                      const BYTE *buffer,
                                      ULONG bufferLengthInBytes
                                      )
      {
        String result;
        Base64Encoder encoder(new StringSink(result), false);
        encoder.Put(buffer, bufferLengthInBytes);
        encoder.MessageEnd();
        return result;
      }

      //-----------------------------------------------------------------------
      String Helper::convertToBase64(const String &input)
      {
        if (input.isEmpty()) return String();
        return IHelper::convertToBase64((const BYTE *)(input.c_str()), input.length());
      }

      //-----------------------------------------------------------------------
      String Helper::convertToBase64(const SecureByteBlock &input)
      {
        if (input.size() < 1) return String();
        return IHelper::convertToBase64(input, input.size());
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::convertFromBase64(const String &input)
      {
        SecureByteBlockPtr output(new SecureByteBlock);

        ByteQueue queue;
        queue.Put((BYTE *)input.c_str(), input.size());

        ByteQueue *outputQueue = new ByteQueue;
        Base64Decoder decoder(outputQueue);
        queue.CopyTo(decoder);
        decoder.MessageEnd();

        size_t outputLengthInBytes = (size_t)outputQueue->CurrentSize();
        output->CleanNew(outputLengthInBytes);

        outputQueue->Get(*output, outputLengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      String Helper::convertToHex(
                                  const BYTE *buffer,
                                  ULONG bufferLengthInBytes,
                                  bool outputUpperCase
                                  )
      {
        String result;

        HexEncoder encoder(new StringSink(result), outputUpperCase);
        encoder.Put(buffer, bufferLengthInBytes);
        encoder.MessageEnd();

        return result;
      }

      //-----------------------------------------------------------------------
      String Helper::convertToHex(
                                  SecureByteBlock &input,
                                  bool outputUpperCase
                                  )
      {
        return convertToHex(input, input.size(), outputUpperCase);
      }

      //-------------------------------------------------------------------------
      SecureByteBlockPtr Helper::convertFromHex(const String &input)
      {
        SecureByteBlockPtr output(new SecureByteBlock);
        ByteQueue queue;
        queue.Put((BYTE *)input.c_str(), input.size());

        ByteQueue *outputQueue = new ByteQueue;
        HexDecoder decoder(outputQueue);
        queue.CopyTo(decoder);
        decoder.MessageEnd();

        size_t outputLengthInBytes = outputQueue->CurrentSize();
        output->CleanNew(outputLengthInBytes);

        outputQueue->Get(*output, outputLengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::encrypt(
                                         const SecureByteBlock &key, // key length of 32 = AES/256
                                         const SecureByteBlock &iv,
                                         const SecureByteBlock &buffer,
                                         EncryptionAlgorthms algorithm
                                         )
      {
        return encrypt(key, iv, buffer, buffer.size(), algorithm);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::encrypt(
                                         const SecureByteBlock &key, // key length of 32 = AES/256
                                         const SecureByteBlock &iv,
                                         const char *value,
                                         EncryptionAlgorthms algorithm
                                         )
      {
        return encrypt(key, iv, (const BYTE *)value, strlen(value)*sizeof(char), algorithm);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::encrypt(
                                         const SecureByteBlock &key, // key length of 32 = AES/256
                                         const SecureByteBlock &iv,
                                         const BYTE *buffer,
                                         size_t bufferLengthInBytes,
                                         EncryptionAlgorthms algorithm
                                         )
      {
        SecureByteBlockPtr output(new SecureByteBlock(bufferLengthInBytes));
        CFB_Mode<AES>::Encryption cfbEncryption(key, key.size(), iv);
        cfbEncryption.ProcessData(*output, buffer, bufferLengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::decrypt(
                                         const SecureByteBlock &key,
                                         const SecureByteBlock &iv,
                                         const SecureByteBlock &buffer,
                                         EncryptionAlgorthms algorithm
                                         )
      {
        SecureByteBlockPtr output(new SecureByteBlock(buffer.size()));
        CFB_Mode<AES>::Decryption cfbDecryption(key, key.size(), iv);
        cfbDecryption.ProcessData(*output, buffer, buffer.size());
        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::hash(
                                      const char *value,
                                      HashAlgorthms algorithm
                                      )
      {
        SecureByteBlockPtr output;

        switch (algorithm) {
          case HashAlgorthm_MD5:      {
            MD5 hasher;
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update((const BYTE *)(value), strlen(value));
            hasher.Final(*output);
            break;
          }
          case HashAlgorthm_SHA1:     {
            SHA1 hasher;
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update((const BYTE *)(value), strlen(value));
            hasher.Final(*output);
            break;
          }
          case HashAlgorthm_SHA256:   {
            SHA256 hasher;
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update((const BYTE *)(value), strlen(value));
            hasher.Final(*output);
            break;
          }
        }

        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::hmacKey(
                                         const char *password,
                                         HashAlgorthms algorithm
                                         )
      {
        SecureByteBlockPtr key;
        size_t length = 0;

        switch (algorithm) {
          case HashAlgorthm_MD5:      length = 16; break;
          case HashAlgorthm_SHA1:     length = 20; break;
          case HashAlgorthm_SHA256:   length = 32; break;
        }

        if (strlen(password)*sizeof(char) > length) {
          key = hash(password, algorithm);
        } else {
          key = SecureByteBlockPtr(new SecureByteBlock);
          key->CleanNew(length);
          memcpy(*key, password, strlen(password)*sizeof(char));
        }

        return key;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::hmac(
                                      const SecureByteBlock &key,
                                      const String &value,
                                      HashAlgorthms algorithm
                                      )
      {
        return hmac(key, (const BYTE *)(value.c_str()), value.length(), algorithm);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::hmac(
                                      const SecureByteBlock &key,
                                      const SecureByteBlock &buffer,
                                      HashAlgorthms algorithm
                                      )
      {
        return hmac(key, buffer, buffer.size(), algorithm);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::hmac(
                                      const SecureByteBlock &key,
                                      const BYTE *buffer,
                                      size_t bufferLengthInBytes,
                                      HashAlgorthms algorithm
                                      )
      {
        SecureByteBlockPtr output;

        switch (algorithm) {
          case HashAlgorthm_MD5:      {
            HMAC<MD5> hasher(key, key.size());
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update(buffer, bufferLengthInBytes);
            hasher.Final(*output);
            break;
          }
          case HashAlgorthm_SHA1:     {
            HMAC<SHA1> hasher(key, key.size());
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update(buffer, bufferLengthInBytes);
            hasher.Final(*output);
            break;
          }
          case HashAlgorthm_SHA256:   {
            HMAC<SHA256> hasher(key, key.size());
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update(buffer, bufferLengthInBytes);
            hasher.Final(*output);
            break;
          }
        }

        return output;
      }

      //-----------------------------------------------------------------------
      ElementPtr Helper::getSignatureInfo(
                                          ElementPtr signedEl,
                                          ElementPtr *outSignatureEl,
                                          String *outPeerURI,
                                          String *outKeyID,
                                          String *outKeyDomain,
                                          String *outService
                                          )
      {
        if (!signedEl) {
          ZS_LOG_WARNING(Detail, "requested to get signature info on a null element")
          return ElementPtr();
        }

        ElementPtr signatureEl = signedEl->findNextSiblingElement("signature");
        if (!signatureEl) {
          // if this element does not have a signed next sibling then it can't be the signed elemnt thus assume it's the bundle passed in instead
          signedEl = signedEl->getFirstChildElement();
          while (signedEl) {
            if ("signature" != signedEl->getValue()) {
              break;
            }
            signedEl = signedEl->getNextSiblingElement();
          }

          if (!signedEl) {
            ZS_LOG_DETAIL("no signed element was found (is okay if signing element for first time)")
            return ElementPtr();
          }

          signatureEl = signedEl->findNextSiblingElement("signature");
        }

        String id = signedEl->getAttributeValue("id");
        if (id.length() < 1) {
          ZS_LOG_WARNING(Detail, "ID is missing on signed element")
          return ElementPtr();
        }

        id = "#" + id;

        while (signatureEl) {
          ElementPtr referenceEl = signatureEl->findFirstChildElementChecked("reference");
          if (referenceEl) {
            String referenceID = referenceEl->getTextDecoded();
            if (referenceID == id) {
              ZS_LOG_TRACE("found the signature reference, reference id=" + id)
              break;
            }
          }

          signatureEl = signatureEl->findNextSiblingElement("signature");
        }

        if (!signatureEl) {
          ZS_LOG_WARNING(Detail, "could not find signature element")
          return ElementPtr();
        }

        ElementPtr keyEl = signatureEl->findFirstChildElement("key");
        if (keyEl) {
          if (outPeerURI) {
            *outPeerURI = IMessageHelper::getElementTextAndDecode(keyEl->findFirstChildElement("uri"));
          }
          if (outKeyID) {
            *outKeyID = IMessageHelper::getAttributeID(keyEl);
          }
          if (outKeyDomain) {
            *outKeyDomain = IMessageHelper::getElementTextAndDecode(keyEl->findFirstChildElement("domain"));
          }
          if (outService) {
            *outService = IMessageHelper::getElementTextAndDecode(keyEl->findFirstChildElement("service"));
          }
        }

        if (outSignatureEl) {
          *outSignatureEl = signatureEl;
        }
        return signedEl;
      }

      //-----------------------------------------------------------------------
      bool Helper::isValidDomain(const char *inDomain)
      {
        String domain(inDomain ? String(inDomain) : String());
        zsLib::RegEx regex("^([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}$");
        if (!regex.hasMatch(inDomain)) {
          ZS_LOG_WARNING(Detail, "Helper [] domain name is not valid, domain=" + domain)
          return false;
        }
        ZS_LOG_TRACE("Helper [] valid domain, domain=" + domain)
        return true;
      }

      //-----------------------------------------------------------------------
      void Helper::split(
                         const String &input,
                         SplitMap &outResult,
                         char splitChar
                         )
      {
        if (0 == input.size()) return;

        size_t start = input.find(splitChar);
        size_t end = String::npos;

        Index index = 0;
        if (String::npos == start) {
          outResult[index] = input;
          return;
        }

        if (0 != start) {
          // special case where start is not a /
          outResult[index] = input.substr(0, start);
          ++index;
        }

        do {
          end = input.find(splitChar, start+1);

          if (end == String::npos) {
            // there is no more splits left so copy from start / to end
            outResult[index] = input.substr(start+1);
            ++index;
            break;
          }

          // take the mid-point of the string
          if (end != start+1) {
            outResult[index] = input.substr(start+1, end-(start+1));
            ++index;
          }

          // the next starting point will be the current end point
          start = end;
        } while (true);
      }

      //-----------------------------------------------------------------------
      const zsLib::String &Helper::get(
                                        const SplitMap &inResult,
                                        Index index
                                        )
      {
        static String empty;
        SplitMap::const_iterator found = inResult.find(index);
        if (found == inResult.end()) return empty;
        return (*found).second;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IHelper
    #pragma mark

    //-------------------------------------------------------------------------
    RecursiveLock &IHelper::getGlobalLock()
    {
      return internal::Helper::getGlobalLock();
    }

    //-------------------------------------------------------------------------
    String IHelper::randomString(UINT lengthInChars)
    {
      return internal::Helper::randomString(lengthInChars);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::random(UINT lengthInBytes)
    {
      return internal::Helper::random(lengthInBytes);
    }

    //-------------------------------------------------------------------------
    int IHelper::compare(
                         const SecureByteBlock &left,
                         const SecureByteBlock &right
                         )
    {
      return internal::Helper::compare(left, right);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToString(const SecureByteBlock &buffer)
    {
      return internal::Helper::convertToString(buffer);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(
                                                const char *input,
                                                bool appendNUL
                                                )
    {
      return internal::Helper::convertToBuffer(input, appendNUL);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(
                                                const std::string &input,
                                                bool appendNUL
                                                )
    {
      return internal::Helper::convertToBuffer(input.c_str(), appendNUL);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(
                                                const BYTE *buffer,
                                                ULONG bufferLengthInBytes,
                                                bool appendNULIfMissing
                                                )
    {
      return internal::Helper::convertToBuffer(buffer, bufferLengthInBytes, appendNULIfMissing);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::makeBufferStringSafe(const SecureByteBlock &input)
    {
      return internal::Helper::makeBufferStringSafe(input);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(
                                    const BYTE *buffer,
                                    ULONG bufferLengthInBytes
                                    )
    {
      return internal::Helper::convertToBase64(buffer, bufferLengthInBytes);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(const String &input)
    {
      return internal::Helper::convertToBase64(input);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(const SecureByteBlock &input)
    {
      return internal::Helper::convertToBase64(input);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertFromBase64(const String &input)
    {
      return internal::Helper::convertFromBase64(input);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertStringFromBase64(const String &input)
    {
      return internal::Helper::convertStringFromBase64(input);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToHex(
                                 const BYTE *buffer,
                                 ULONG bufferLengthInBytes,
                                 bool outputUpperCase
                                 )
    {
      return internal::Helper::convertToHex(buffer, bufferLengthInBytes, outputUpperCase);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToHex(
                                 SecureByteBlock &input,
                                 bool outputUpperCase
                                 )
    {
      return internal::Helper::convertToHex(input, outputUpperCase);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertFromHex(const String &input)
    {
      return internal::Helper::convertFromHex(input);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::encrypt(
                                        const SecureByteBlock &key,
                                        const SecureByteBlock &iv,
                                        const SecureByteBlock &buffer,
                                        EncryptionAlgorthms algorithm
                                        )
    {
      return internal::Helper::encrypt(key, iv, buffer, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::encrypt(
                                        const SecureByteBlock &key, // key length of 32 = AES/256
                                        const SecureByteBlock &iv,
                                        const char *value,
                                        EncryptionAlgorthms algorithm
                                        )
    {
      return internal::Helper::encrypt(key, iv, value, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::encrypt(
                                        const SecureByteBlock &key, // key length of 32 = AES/256
                                        const SecureByteBlock &iv,
                                        const std::string &value,
                                        EncryptionAlgorthms algorithm
                                        )
    {
      return internal::Helper::encrypt(key, iv, value.c_str(), algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::encrypt(
                                        const SecureByteBlock &key, // key length of 32 = AES/256
                                        const SecureByteBlock &iv,
                                        const BYTE *buffer,
                                        size_t bufferLengthInBytes,
                                        EncryptionAlgorthms algorithm
                                        )
    {
      return internal::Helper::encrypt(key, iv, buffer, bufferLengthInBytes, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::decrypt(
                                        const SecureByteBlock &key,
                                        const SecureByteBlock &iv,
                                        const SecureByteBlock &buffer,
                                        EncryptionAlgorthms algorithm
                                        )
    {
      return internal::Helper::decrypt(key, iv, buffer, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hash(
                                     const char *buffer,
                                     HashAlgorthms algorithm
                                     )
    {
      return internal::Helper::hash(buffer, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hash(
                                     const std::string &buffer,
                                     HashAlgorthms algorithm
                                     )
    {
      return internal::Helper::hash(buffer.c_str(), algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmacKey(
                                        const char *password,
                                        HashAlgorthms algorithm
                                        )
    {
      return internal::Helper::hmacKey(password, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmacKey(
                                        const std::string &password,
                                        HashAlgorthms algorithm
                                        )
    {
      return internal::Helper::hmacKey(password.c_str(), algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmac(
                                     const SecureByteBlock &key,
                                     const char *value,
                                     HashAlgorthms algorithm
                                     )
    {
      return internal::Helper::hmac(key, value, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmac(
                                     const SecureByteBlock &key,
                                     const std::string &value,
                                     HashAlgorthms algorithm
                                     )
    {
      return internal::Helper::hmac(key, value.c_str(), algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmac(
                                     const SecureByteBlock &key,
                                     const SecureByteBlock &buffer,
                                     HashAlgorthms algorithm
                                     )
    {
      return internal::Helper::hmac(key, buffer, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmac(
                                     const SecureByteBlock &key,
                                     const BYTE *buffer,
                                     size_t bufferLengthInBytes,
                                     HashAlgorthms algorithm
                                     )
    {
      return internal::Helper::hmac(key, buffer, bufferLengthInBytes, algorithm);
    }

    //-------------------------------------------------------------------------
    ElementPtr IHelper::getSignatureInfo(
                                         ElementPtr signedEl,
                                         ElementPtr *outSignatureEl,
                                         String *outPeerURI,
                                         String *outKeyID,
                                         String *outKeyDomain,
                                         String *outService
                                         )
    {
      return internal::Helper::getSignatureInfo(signedEl, outSignatureEl, outPeerURI, outKeyID, outKeyDomain, outService);
    }

    //-------------------------------------------------------------------------
    bool IHelper::isValidDomain(const char *domain)
    {
      return internal::Helper::isValidDomain(domain);
    }

    //-------------------------------------------------------------------------
    void IHelper::split(
                        const String &input,
                        SplitMap &outResult,
                        char splitChar
                        )
    {
      return internal::Helper::split(input, outResult, splitChar);
    }

    //-------------------------------------------------------------------------
    const String &IHelper::get(
                               const SplitMap &inResult,
                               Index index
                               )
    {
      return internal::Helper::get(inResult, index);
    }
  }
}
