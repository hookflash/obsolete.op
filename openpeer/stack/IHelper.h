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
#include <openpeer/stack/message/types.h>


namespace openpeer
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IHelper
    #pragma mark

    interaction IHelper
    {
      enum EncryptionAlgorthms
      {
        EncryptionAlgorthm_AES,
      };

      enum HashAlgorthms
      {
        HashAlgorthm_MD5,
        HashAlgorthm_SHA1,
        HashAlgorthm_SHA256,
      };

      typedef size_t Index;
      typedef std::map<Index, String> SplitMap;

      static RecursiveLock &getGlobalLock();

      static String randomString(UINT lengthInChars);

      static SecureByteBlockPtr random(UINT lengthInBytes);

      static ULONG random(ULONG minValue, ULONG maxValue);

      static int compare(
                         const SecureByteBlock &left,
                         const SecureByteBlock &right
                         );

      static SecureByteBlockPtr clone(SecureByteBlockPtr pBuffer);
      static SecureByteBlockPtr clone(const SecureByteBlock &buffer);

      static String convertToString(const SecureByteBlock &buffer);
      static SecureByteBlockPtr convertToBuffer(
                                                const char *input,
                                                bool appendNUL = true
                                                );
      static SecureByteBlockPtr convertToBuffer(
                                                const std::string &input,
                                                bool appendNUL = true
                                                );
      static SecureByteBlockPtr convertToBuffer(
                                                const BYTE *buffer,
                                                ULONG bufferLengthInBytes,
                                                bool appendNULIfMissing = false
                                                );
      static SecureByteBlockPtr makeBufferStringSafe(const SecureByteBlock &input);

      static String convertToBase64(
                                    const BYTE *buffer,
                                    ULONG bufferLengthInBytes
                                    );

      static String convertToBase64(const SecureByteBlock &input);

      static String convertToBase64(const String &input);

      static SecureByteBlockPtr convertFromBase64(const String &input);

      static String convertStringFromBase64(const String &input);

      static String convertToHex(
                                 const BYTE *buffer,
                                 ULONG bufferLengthInBytes,
                                 bool outputUpperCase = false
                                 );

      static String convertToHex(
                                 SecureByteBlock &input,
                                 bool outputUpperCase = false
                                 );

      static SecureByteBlockPtr convertFromHex(const String &input);

      static SecureByteBlockPtr encrypt(
                                        const SecureByteBlock &key, // key length of 32 = AES/256
                                        const SecureByteBlock &iv,  // 16 bytes for AES
                                        const SecureByteBlock &value,
                                        EncryptionAlgorthms algorithm = EncryptionAlgorthm_AES
                                        );

      static SecureByteBlockPtr encrypt(
                                        const SecureByteBlock &key, // key length of 32 = AES/256
                                        const SecureByteBlock &iv,  // 16 bytes for AES
                                        const char *value,
                                        EncryptionAlgorthms algorithm = EncryptionAlgorthm_AES
                                        );

      static SecureByteBlockPtr encrypt(
                                        const SecureByteBlock &key, // key length of 32 = AES/256
                                        const SecureByteBlock &iv,  // 16 bytes for AES
                                        const std::string &value,
                                        EncryptionAlgorthms algorithm = EncryptionAlgorthm_AES
                                        );

      static SecureByteBlockPtr encrypt(
                                        const SecureByteBlock &key, // key length of 32 = AES/256
                                        const SecureByteBlock &iv,  // 16 bytes for AES
                                        const BYTE *buffer,
                                        size_t bufferLengthInBytes,
                                        EncryptionAlgorthms algorithm = EncryptionAlgorthm_AES
                                        );

      static SecureByteBlockPtr decrypt(
                                        const SecureByteBlock &key,
                                        const SecureByteBlock &iv,
                                        const SecureByteBlock &value,
                                        EncryptionAlgorthms algorithm = EncryptionAlgorthm_AES
                                        );

      static size_t getHashDigestSize(HashAlgorthms algorithm); // returns hash algorithm's digest output size in bytes

      static SecureByteBlockPtr hash(
                                     const char *value,
                                     HashAlgorthms algorithm = HashAlgorthm_SHA1
                                     );
      static SecureByteBlockPtr hash(
                                     const std::string &value,
                                     HashAlgorthms algorithm = HashAlgorthm_SHA1
                                     );
      static SecureByteBlockPtr hash(
                                     const SecureByteBlock &value,
                                     HashAlgorthms algorithm = HashAlgorthm_SHA1
                                     );

      static SecureByteBlockPtr hmacKeyFromPassphrase(const char *passphrase);
      static SecureByteBlockPtr hmacKeyFromPassphrase(const std::string &passphrase);

      static SecureByteBlockPtr hmac(
                                     const SecureByteBlock &key,
                                     const char *value,
                                     HashAlgorthms algorithm = HashAlgorthm_SHA1
                                     );
      static SecureByteBlockPtr hmac(
                                     const SecureByteBlock &key,
                                     const std::string &value,
                                     HashAlgorthms algorithm = HashAlgorthm_SHA1
                                     );

      static SecureByteBlockPtr hmac(
                                     const SecureByteBlock &key,
                                     const SecureByteBlock &buffer,
                                     HashAlgorthms algorithm = HashAlgorthm_SHA1
                                     );

      static SecureByteBlockPtr hmac(
                                     const SecureByteBlock &key,
                                     const BYTE *buffer,
                                     size_t bufferLengthInBytes,
                                     HashAlgorthms algorithm = HashAlgorthm_SHA1
                                     );

      static void splitKey(
                           const SecureByteBlock &key,
                           SecureByteBlockPtr &part1,
                           SecureByteBlockPtr &part2
                           );
      static SecureByteBlockPtr combineKey(
                                           const SecureByteBlockPtr &part1,
                                           const SecureByteBlockPtr &part2
                                           );

      // RETURNS: returns the actual signed element, rather than the bundle element (if bundle was passed in) or NULL if no signature was found
      static ElementPtr getSignatureInfo(
                                         ElementPtr signedEl,
                                         ElementPtr *outSignatureEl = NULL,
                                         String *outPeerURI = NULL,
                                         String *outKeyID = NULL,
                                         String *outKeyDomain = NULL,
                                         String *outService = NULL,
                                         String *outFullPublicKey = NULL,
                                         String *outFingerprint = NULL
                                         );

      static ElementPtr cloneAsCanonicalJSON(ElementPtr element);

      static bool isValidDomain(const char *domain);

      static void split(
                        const String &input,
                        SplitMap &outResult,
                        char splitChar = '/'
                        );

      static const String &get(
                               const SplitMap &inResult,
                               Index index
                               );
    };
  }
}
