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

#include <openpeer/services/internal/services_Helper.h>
#include <openpeer/services/IDNS.h>
#include <cryptopp/osrng.h>

#include <zsLib/Stringize.h>
#include <zsLib/Numeric.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/RegEx.h>

#include <iostream>
#include <fstream>
#ifndef _WIN32
#include <pthread.h>
#endif //ndef _WIN32

#include <boost/shared_array.hpp>

#include <cryptopp/modes.h>
#include <cryptopp/hex.h>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>
#include <cryptopp/sha.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>


namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }


namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      using zsLib::Numeric;
      using zsLib::QWORD;

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

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static String getElementTextAndDecode(ElementPtr node)
      {
        if (!node) return String();
        return node->getTextDecoded();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark GlobalLock
      #pragma mark

      //-----------------------------------------------------------------------
      class GlobalLock
      {
      public:
        GlobalLock() {}
        ~GlobalLock() {}

        //---------------------------------------------------------------------
        static GlobalLock &singleton()
        {
          static GlobalLock lock;
          return lock;
        }

        //---------------------------------------------------------------------
        RecursiveLock &getLock() const
        {
          return mLock;
        }

      private:
        mutable RecursiveLock mLock;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark GlobalLockInit
      #pragma mark

      //-----------------------------------------------------------------------
      class GlobalLockInit
      {
      public:
        //---------------------------------------------------------------------
        GlobalLockInit()
        {
          singleton();
        }

        //---------------------------------------------------------------------
        RecursiveLock &singleton()
        {
          return (GlobalLock::singleton()).getLock();
        }
      };

      static GlobalLockInit gGlobalLockInit;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceThread
      #pragma mark

      //-----------------------------------------------------------------------
      class ServiceThread;
      typedef boost::shared_ptr<ServiceThread> ServiceThreadPtr;
      typedef boost::weak_ptr<ServiceThread> ServiceThreadWeakPtr;

      class ServiceThread
      {
        //---------------------------------------------------------------------
        ServiceThread() {}

        //---------------------------------------------------------------------
        void init()
        {
          mThread = MessageQueueThread::createBasic("org.openpeer.services.serviceThread");
        }

      public:
        //---------------------------------------------------------------------
        ~ServiceThread()
        {
          if (!mThread) return;
          mThread->waitForShutdown();
        }

        //---------------------------------------------------------------------
        static ServiceThreadPtr create()
        {
          ServiceThreadPtr pThis(new ServiceThread);
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        static ServiceThreadPtr singleton()
        {
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static ServiceThreadPtr thread = ServiceThread::create();
          return thread;
        }

        //---------------------------------------------------------------------
        MessageQueueThreadPtr getThread() const
        {
          return mThread;
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceThread => (data)
        #pragma mark

        ServiceThreadWeakPtr mThisWeak;

        MessageQueueThreadPtr mThread;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Helper
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Helper::getGlobalLock()
      {
        return gGlobalLockInit.singleton();
      }

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

      //---------------------------------------------------------------------
      String Helper::timeToString(const Time &value)
      {
        if (Time() == value) return String();

        time_t epoch = zsLib::toEpoch(value);
        return string(epoch);
      }

      //---------------------------------------------------------------------
      Time Helper::stringToTime(const String &str)
      {
        if (str.isEmpty()) return Time();

        try {
          time_t timestamp = Numeric<time_t>(str);
          return zsLib::toTime(timestamp);
        } catch (Numeric<time_t>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Detail, "unable to convert value to time_t, value=" + str)
          try {
            QWORD timestamp = Numeric<QWORD>(str);
            ZS_LOG_WARNING(Debug, "date exceeds maximum time_t, value=" + string(timestamp))
            return Time(boost::date_time::max_date_time);
          } catch (Numeric<QWORD>::ValueOutOfRange &) {
            ZS_LOG_WARNING(Detail, "even QWORD failed to convert value to max_date_time, value=" + str)
          }
        }
        return Time();
      }

      //-----------------------------------------------------------------------
      String Helper::randomString(UINT lengthInChars)
      {
        static const char *randomCharArray = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static size_t randomSize = strlen(randomCharArray);

        BYTE staticBuffer[256];
        char staticOutputBuffer[sizeof(staticBuffer)+1];

        boost::shared_array<BYTE> allocatedBuffer;
        boost::shared_array<char> allocatedOutputBuffer;

        BYTE *buffer = &(staticBuffer[0]);
        char *output = &(staticOutputBuffer[0]);
        if (lengthInChars > sizeof(staticBuffer)) {
          // use the allocated buffer instead
          allocatedBuffer = boost::shared_array<BYTE>(new BYTE[lengthInChars]);
          allocatedOutputBuffer = boost::shared_array<char>(new char[lengthInChars+1]);
          buffer = allocatedBuffer.get();
          output = allocatedOutputBuffer.get();
        }

        AutoSeededRandomPool rng;
        rng.GenerateBlock(&(buffer[0]), lengthInChars);

        memset(&(output[0]), 0, sizeof(char)*(lengthInChars+1));

        for (UINT loop = 0; loop < lengthInChars; ++loop) {
          output[loop] = randomCharArray[((buffer[loop])%randomSize)];
        }
        return String((CSTR)(&(output[0])));
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::random(UINT lengthInBytes)
      {
        SecureByteBlockPtr output(new SecureByteBlock);
        AutoSeededRandomPool rng;
        output->CleanNew(lengthInBytes);
        rng.GenerateBlock(*output, lengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      ULONG Helper::random(ULONG minValue, ULONG maxValue)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(minValue > maxValue)
        if (minValue == maxValue) return minValue;

        ULONG range = maxValue - minValue;

        ULONG value = 0;
        
        AutoSeededRandomPool rng;
        rng.GenerateBlock((BYTE *) &value, sizeof(ULONG));

        value = minValue + (value % range);

        return value;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Helper::getServiceQueue()
      {
        ServiceThreadPtr thread = ServiceThread::singleton();
        return thread->getThread();
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
        if (0 == left.SizeInBytes()) {
          return 0;
        }
        return memcmp(left, right, left.SizeInBytes());
      }

      //-------------------------------------------------------------------------
      SecureByteBlockPtr Helper::clone(SecureByteBlockPtr pBuffer)
      {
        if (!pBuffer) return SecureByteBlockPtr();
        return Helper::clone(*pBuffer);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::clone(const SecureByteBlock &buffer)
      {
        SecureByteBlockPtr pBuffer(new SecureByteBlock);
        SecureByteBlock::size_type size = buffer.SizeInBytes();
        if (size < 1) return pBuffer;
        pBuffer->CleanNew(size);

        memcpy(pBuffer->BytePtr(), buffer.BytePtr(), size);
        return pBuffer;
      }

      //-----------------------------------------------------------------------
      String Helper::convertToString(const SecureByteBlock &buffer)
      {
        if (buffer.size() < 1) return String();
        return (const char *)(buffer.BytePtr());  // return buffer cast as const char *
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::convertToBuffer(const char *input)
      {
        if (NULL == input) return SecureByteBlockPtr();

        SecureByteBlockPtr output(new SecureByteBlock);
        size_t len = strlen(input);
        if (len < 1) return output;

        output->CleanNew(sizeof(char)*len);

        memcpy(*output, input, sizeof(char)*len);
        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::convertToBuffer(
                                                 const BYTE *buffer,
                                                 ULONG bufferLengthInBytes
                                                 )
      {
        SecureByteBlockPtr output(new SecureByteBlock);

        if (bufferLengthInBytes < 1) return output;

        output->CleanNew(bufferLengthInBytes);

        memcpy(*output, buffer, bufferLengthInBytes);

        return output;
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

        if (outputLengthInBytes < 1) return output;

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

        SecureByteBlock::size_type outputLengthInBytes = (SecureByteBlock::size_type)outputQueue->CurrentSize();
        if (outputLengthInBytes < 1) return output;

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
      size_t Helper::getHashDigestSize(HashAlgorthms algorithm)
      {
        switch (algorithm) {
          case HashAlgorthm_MD5:      {
            MD5 hasher;
            return hasher.DigestSize();
          }
          case HashAlgorthm_SHA1:     {
            SHA1 hasher;
            return hasher.DigestSize();
          }
          case HashAlgorthm_SHA256:   {
            SHA256 hasher;
            return hasher.DigestSize();
          }
        }

        return 0;
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
      SecureByteBlockPtr Helper::hash(
                                      const SecureByteBlock &buffer,
                                      HashAlgorthms algorithm
                                      )
      {
        SecureByteBlockPtr output;

        switch (algorithm) {
          case HashAlgorthm_MD5:      {
            MD5 hasher;
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update(buffer.BytePtr(), buffer.SizeInBytes());
            hasher.Final(*output);
            break;
          }
          case HashAlgorthm_SHA1:     {
            SHA1 hasher;
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update(buffer.BytePtr(), buffer.SizeInBytes());
            hasher.Final(*output);
            break;
          }
          case HashAlgorthm_SHA256:   {
            SHA256 hasher;
            output = SecureByteBlockPtr(new SecureByteBlock(hasher.DigestSize()));
            hasher.Update(buffer.BytePtr(), buffer.SizeInBytes());
            hasher.Final(*output);
            break;
          }
        }

        return output;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::hmacKeyFromPassphrase(const char *passphrase)
      {
        return convertToBuffer(passphrase);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::hmacKeyFromPassphrase(const std::string &passphrase)
      {
        return convertToBuffer(passphrase.c_str());
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
      void Helper::splitKey(
                            const SecureByteBlock &key,
                            SecureByteBlockPtr &part1,
                            SecureByteBlockPtr &part2
                            )
      {
        if (key.size() < 1) return;

        SecureByteBlockPtr hash = IHelper::hash(key);
        ZS_THROW_BAD_STATE_IF(!hash)

        SecureByteBlockPtr randomData = IHelper::random(key.SizeInBytes() + hash->SizeInBytes());

        SecureByteBlockPtr final(new SecureByteBlock);
        final->CleanNew(key.SizeInBytes() + hash->SizeInBytes());

        BYTE *dest = final->BytePtr();
        const BYTE *source = key.BytePtr();
        const BYTE *random = randomData->BytePtr();

        SecureByteBlock::size_type length1 = key.SizeInBytes();
        for (; length1 > 0; --length1, ++dest, ++source, ++random)
        {
          *dest = (*source) ^ (*random);
        }

        SecureByteBlock::size_type length2 = hash->SizeInBytes();
        source = hash->BytePtr();
        for (; length2 > 0; --length2, ++dest, ++source, ++random)
        {
          *dest = (*source) ^ (*random);
        }

        // set the output split key into the base 64 values
        part1 = randomData;
        part2 = final;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::combineKey(
                                            const SecureByteBlockPtr &part1,
                                            const SecureByteBlockPtr &part2
                                            )
      {
        if ((!part1) || (!part2)) {
          ZS_LOG_WARNING(Detail, String("value missing") + ", part1=" + (part1 ? "true" : "(null)") + ", part2=" + (part2 ? "true" : "(null)"))
          return SecureByteBlockPtr();
        }

        SecureByteBlockPtr extracted = IHelper::hash("empty");

        if (part1->SizeInBytes() != part2->SizeInBytes()) {
          ZS_LOG_WARNING(Detail, String("illegal size") + ", part1 size=" + string(part1->SizeInBytes()) + ", part2 size=" + string(part2->SizeInBytes()))
          return SecureByteBlockPtr();
        }
        if (part1->SizeInBytes() <= extracted->SizeInBytes()) {
          ZS_LOG_WARNING(Detail, String("illegal hash size") + ", part size=" + string(part1->SizeInBytes()) + ", hash size=" + string(extracted->SizeInBytes()))
          return SecureByteBlockPtr();
        }

        SecureByteBlockPtr buffer(new SecureByteBlock);
        buffer->CleanNew(part1->SizeInBytes() - extracted->SizeInBytes());

        BYTE *dest = buffer->BytePtr();
        const BYTE *src1 = part1->BytePtr();
        const BYTE *src2 = part2->BytePtr();

        SecureByteBlock::size_type length1 = part1->SizeInBytes() - extracted->SizeInBytes();
        for (; 0 != length1; --length1, ++dest, ++src1, ++src2)
        {
          *dest = (*src1) ^ (*src2);
        }

        SecureByteBlock::size_type length2 = extracted->SizeInBytes();
        dest = extracted->BytePtr();
        for (; 0 != length2; --length2, ++dest, ++src1, ++src2)
        {
          *dest = (*src1) ^ (*src2);
        }

        SecureByteBlockPtr hash = IHelper::hash(*buffer);

        if (0 != IHelper::compare(*extracted, *hash)) {
          ZS_LOG_WARNING(Detail, String("extracted hash does not match calculated hash") + ", extracted=" + IHelper::convertToBase64(*extracted) + ", calculated=" + IHelper::convertToBase64(*hash))
          return SecureByteBlockPtr();
        }

        return buffer;
      }

      //-----------------------------------------------------------------------
      ElementPtr Helper::getSignatureInfo(
                                          ElementPtr signedEl,
                                          ElementPtr *outSignatureEl,
                                          String *outFullPublicKey,
                                          String *outFingerprint
                                          )
      {
        if (!signedEl) {
          ZS_LOG_WARNING(Detail, "services::Helper [] requested to get signature info on a null element")
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
            ZS_LOG_DETAIL("services::Helper [] no signed element was found (is okay if signing element for first time)")
            return ElementPtr();
          }

          signatureEl = signedEl->findNextSiblingElement("signature");
        }

        String id = signedEl->getAttributeValue("id");
        if (id.length() < 1) {
          ZS_LOG_WARNING(Detail, "services::Helper [] ID is missing on signed element")
          return ElementPtr();
        }

        id = "#" + id;

        while (signatureEl) {
          ElementPtr referenceEl = signatureEl->findFirstChildElementChecked("reference");
          if (referenceEl) {
            String referenceID = referenceEl->getTextDecoded();
            if (referenceID == id) {
              ZS_LOG_TRACE("services::Helper [] found the signature reference, reference id=" + id)
              break;
            }
          }

          signatureEl = signatureEl->findNextSiblingElement("signature");
        }

        if (!signatureEl) {
          ZS_LOG_WARNING(Detail, "services::Helper [] could not find signature element")
          return ElementPtr();
        }

        ElementPtr keyEl = signatureEl->findFirstChildElement("key");
        if (keyEl) {
          if (outFullPublicKey) {
            *outFullPublicKey = getElementTextAndDecode(keyEl->findFirstChildElement("x509Data"));
          }
          if (outFingerprint) {
            *outFingerprint = getElementTextAndDecode(keyEl->findFirstChildElement("fingerprint"));
          }
        }

        if (outSignatureEl) {
          *outSignatureEl = signatureEl;
        }
        return signedEl;
      }

      //-----------------------------------------------------------------------
      ElementPtr Helper::cloneAsCanonicalJSON(ElementPtr element)
      {
        if (!element) return element;

        class Walker : public WalkSink
        {
        public:
          Walker() {}

          virtual bool onElementEnter(ElementPtr inElement)
          {
            typedef std::list<NodePtr> ChildrenList;
            typedef std::list<AttributePtr> ChildrenAttributeList;

            // sort the elements and "other" node types
            {
              ChildrenList children;

              while (inElement->hasChildren()) {
                NodePtr child = inElement->getFirstChild();
                child->orphan();
                children.push_back(child);
              }

              NodePtr lastInsert;
              bool insertedElement = false;
              while (children.size() > 0)
              {
                NodePtr currentNode = children.front();
                children.pop_front();

                ElementPtr currentEl = (currentNode->isElement() ? currentNode->toElement() : ElementPtr());

                if (!insertedElement) {
                  inElement->adoptAsLastChild(currentNode);
                  lastInsert = currentNode;
                  insertedElement = true;
                  continue;
                }

                if (!currentEl) {
                  lastInsert->adoptAsNextSibling(currentNode);
                  lastInsert = currentNode;
                  continue;
                }

                insertedElement = true; // will have to be true now as this child is an element

                String currentName = currentEl->getValue();

                // I know this isn't optmized as it's insertion sort but this was not meant to canonicalize the entire text of a book, but rather tiny snippets of JSON...

                bool inserted = false;

                ElementPtr childEl = inElement->getFirstChildElement();
                while (childEl) {
                  String childName = childEl->getValue();

                  if (currentName < childName) {
                    // must insert before this child
                    childEl->adoptAsPreviousSibling(currentEl);
                    lastInsert = currentEl;
                    inserted = true;
                    break;
                  }

                  childEl = childEl->getNextSiblingElement();
                }

                if (inserted)
                  continue;

                inElement->adoptAsLastChild(currentEl);
                lastInsert = currentEl;
              }

              ZS_THROW_BAD_STATE_IF(children.size() > 0)
            }

            // sort the attributes
            {
              ChildrenAttributeList children;

              while (inElement->getFirstAttribute()) {
                AttributePtr child = inElement->getFirstAttribute();
                child->orphan();
                children.push_back(child);
              }

              while (children.size() > 0)
              {
                AttributePtr current = children.front();
                children.pop_front();

                // I know this isn't optimized as it's insertion sort but this was not meant to canonicalize the entire text of a book, but rather tiny snippets of JSON...

                AttributePtr child = inElement->getFirstAttribute();
                if (!child) {
                  inElement->setAttribute(current);
                  continue;
                }

                bool inserted = false;
                String currentName = current->getName();

                while (child) {
                  String childName = child->getName();

                  if (currentName < childName) {
                    // must insert before this child
                    child->adoptAsPreviousSibling(current);
                    inserted = true;
                    break;
                  }

                  NodePtr nextNode = child->getNextSibling();
                  if (!nextNode) break;
                  child = nextNode->toAttribute();
                }

                if (inserted)
                  continue;

                // there *MUST* be a last attribute or crash
                inElement->getLastAttributeChecked()->adoptAsNextSibling(current);
              }

              ZS_THROW_BAD_STATE_IF(children.size() > 0)
            }

            return false;
          }

        private:
        };

        if (ZS_IS_LOGGING(Trace)) {
          ZS_LOG_BASIC("vvvvvvvvvvvv -- PRE-SORT  -- vvvvvvvvvvvv")
          {
            GeneratorPtr generator = Generator::createJSONGenerator();
            boost::shared_array<char> output = generator->write(element);
            ZS_LOG_BASIC( ((CSTR)output.get()) )
          }
          ZS_LOG_BASIC("^^^^^^^^^^^^ -- PRE-SORT  -- ^^^^^^^^^^^^")
        }
        ElementPtr convertEl = element->clone()->toElement();

        Node::FilterList filter;
        filter.push_back(Node::NodeType::Element);
        Walker walker;
        convertEl->walk(walker, &filter);

        if (ZS_IS_LOGGING(Trace)) {
          // let's output some logging...
          ZS_LOG_BASIC("vvvvvvvvvvvv -- POST-SORT -- vvvvvvvvvvvv")
          {
            GeneratorPtr generator = Generator::createJSONGenerator();
            boost::shared_array<char> output = generator->write(convertEl);
            ZS_LOG_BASIC( ((CSTR)output.get()) )
          }
          ZS_LOG_BASIC("^^^^^^^^^^^^ -- POST-SORT -- ^^^^^^^^^^^^")
        }

        return convertEl;
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

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark services::IHelper
    #pragma mark

    //-------------------------------------------------------------------------
    RecursiveLock &IHelper::getGlobalLock()
    {
      return internal::Helper::getGlobalLock();
    }

    //-------------------------------------------------------------------------
    static String timeToString(const Time &value)
    {
      return internal::Helper::timeToString(value);
    }

    //-------------------------------------------------------------------------
    IMessageQueuePtr IHelper::getServiceQueue()
    {
      return internal::Helper::getServiceQueue();
    }

    //-------------------------------------------------------------------------
    Time IHelper::stringToTime(const String &str)
    {
      return internal::Helper::stringToTime(str);
    }

    //-------------------------------------------------------------------------
    String IHelper::timeToString(const Time &value)
    {
      return internal::Helper::timeToString(value);
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
    ULONG IHelper::random(ULONG minValue, ULONG maxValue)
    {
      return internal::Helper::random(minValue, maxValue);
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
    SecureByteBlockPtr IHelper::clone(SecureByteBlockPtr pBuffer)
    {
      return internal::Helper::clone(pBuffer);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::clone(const SecureByteBlock &buffer)
    {
      return internal::Helper::clone(buffer);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToString(const SecureByteBlock &buffer)
    {
      return internal::Helper::convertToString(buffer);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(const char *input)
    {
      return internal::Helper::convertToBuffer(input);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(const std::string &input)
    {
      return internal::Helper::convertToBuffer(input.c_str());
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(
                                                const BYTE *buffer,
                                                ULONG bufferLengthInBytes
                                                )
    {
      return internal::Helper::convertToBuffer(buffer, bufferLengthInBytes);
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
    size_t IHelper::getHashDigestSize(HashAlgorthms algorithm)
    {
      return internal::Helper::getHashDigestSize(algorithm);
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
    SecureByteBlockPtr IHelper::hash(
                                     const SecureByteBlock &buffer,
                                     HashAlgorthms algorithm
                                     )
    {
      return internal::Helper::hash(buffer, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmacKeyFromPassphrase(const char *passphrase)
    {
      return internal::Helper::hmacKeyFromPassphrase(passphrase);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::hmacKeyFromPassphrase(const std::string &passphrase)
    {
      return internal::Helper::hmacKeyFromPassphrase(passphrase);
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
    void IHelper::splitKey(
                           const SecureByteBlock &key,
                           SecureByteBlockPtr &part1,
                           SecureByteBlockPtr &part2
                           )
    {
      return internal::Helper::splitKey(key, part1, part2);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::combineKey(
                                           const SecureByteBlockPtr &part1,
                                           const SecureByteBlockPtr &part2
                                           )
    {
      return internal::Helper::combineKey(part1, part2);
    }

    //-------------------------------------------------------------------------
    ElementPtr IHelper::getSignatureInfo(
                                         ElementPtr signedEl,
                                         ElementPtr *outSignatureEl,
                                         String *outFullPublicKey,
                                         String *outFingerprint
                                         )
    {
      return internal::Helper::getSignatureInfo(signedEl, outSignatureEl, outFullPublicKey, outFingerprint);
    }

    //-------------------------------------------------------------------------
    ElementPtr IHelper::cloneAsCanonicalJSON(ElementPtr element)
    {
      return internal::Helper::cloneAsCanonicalJSON(element);
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
      internal::Helper::split(input, outResult, splitChar);
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
