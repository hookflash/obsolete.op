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

#include <openpeer/stack/internal/stack_Helper.h>

#include <openpeer/stack/message/IMessageHelper.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

using namespace zsLib::XML;

namespace openpeer
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
      ElementPtr Helper::getSignatureInfo(
                                          ElementPtr signedEl,
                                          ElementPtr *outSignatureEl,
                                          String *outPeerURI,
                                          String *outKeyID,
                                          String *outKeyDomain,
                                          String *outService,
                                          String *outFullPublicKey,
                                          String *outFingerprint
                                          )
      {
        ElementPtr signatureEl;
        signedEl = services::IHelper::getSignatureInfo(signedEl, &signatureEl, outFullPublicKey, outFingerprint);

        if (outSignatureEl) {
          *outSignatureEl = signatureEl;
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
          if (outFullPublicKey) {
            *outFullPublicKey = IMessageHelper::getElementTextAndDecode(keyEl->findFirstChildElement("x509Data"));
          }
          if (outFingerprint) {
            *outFingerprint = IMessageHelper::getElementTextAndDecode(keyEl->findFirstChildElement("fingerprint"));
          }
        }

        if (outSignatureEl) {
          *outSignatureEl = signatureEl;
        }
        return signedEl;
      }

      //-----------------------------------------------------------------------
      void Helper::convert(
                           const CandidateList &input,
                           IICESocket::CandidateList &output
                           )
      {
        for (CandidateList::const_iterator iter = input.begin(); iter != input.end(); ++iter)
        {
          output.push_back(*iter);
        }
      }

      //-----------------------------------------------------------------------
      void Helper::convert(
                           const IICESocket::CandidateList &input,
                           CandidateList &output
                           )
      {
        for (IICESocket::CandidateList::const_iterator iter = input.begin(); iter != input.end(); ++iter)
        {
          output.push_back(*iter);
        }
      }

      //-----------------------------------------------------------------------
      String Helper::splitEncrypt(
                                  SecureByteBlock &key,
                                  SecureByteBlock &value
                                  )
      {
        SecureByteBlockPtr iv = services::IHelper::random(services::IHelper::getHashDigestSize(services::IHelper::HashAlgorthm_MD5));
        String base64 = services::IHelper::convertToBase64(*services::IHelper::encrypt(key, *iv, value));
        return services::IHelper::convertToHex(*iv) + ":" + base64;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::splitDecrypt(
                                              SecureByteBlock &key,
                                              const char *hexIVAndBase64EncodedData
                                              )
      {
        if (!hexIVAndBase64EncodedData) return SecureByteBlockPtr();

        services::IHelper::SplitMap split;
        services::IHelper::split(hexIVAndBase64EncodedData, split, ':');

        if (split.size() != 2) {
          ZS_LOG_WARNING(Detail, String("failed to decode data (does not have salt:base64 pattern") + ", value=" + hexIVAndBase64EncodedData)
          return SecureByteBlockPtr();
        }

        const String &salt = split[0];
        const String &value = split[1];

        SecureByteBlockPtr iv = services::IHelper::convertFromHex(salt);

        SecureByteBlockPtr dataToConvert = services::IHelper::convertFromBase64(value);
        if (!dataToConvert) {
          ZS_LOG_WARNING(Detail, String("failed to decode data from base64") + ", value=" + value)
          return SecureByteBlockPtr();
        }

       return services::IHelper::decrypt(key, *iv, *dataToConvert);
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
    ElementPtr IHelper::getSignatureInfo(
                                         ElementPtr signedEl,
                                         ElementPtr *outSignatureEl,
                                         String *outPeerURI,
                                         String *outKeyID,
                                         String *outKeyDomain,
                                         String *outService,
                                         String *outFullPublicKey,
                                         String *outFingerprint
                                         )
    {
      return internal::Helper::getSignatureInfo(signedEl, outSignatureEl, outPeerURI, outKeyID, outKeyDomain, outService, outFullPublicKey, outFingerprint);
    }

    //-------------------------------------------------------------------------
    void IHelper::convert(
                          const CandidateList &input,
                          IICESocket::CandidateList &output
                          )
    {
      return internal::Helper::convert(input, output);
    }

    //-------------------------------------------------------------------------
    void IHelper::convert(
                          const IICESocket::CandidateList &input,
                          CandidateList &output
                          )
    {
      return internal::Helper::convert(input, output);
    }
    
    //-------------------------------------------------------------------------
    String IHelper::splitEncrypt(
                                 SecureByteBlock &key,
                                 SecureByteBlock &value
                                 )
    {
      return internal::Helper::splitEncrypt(key, value);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::splitDecrypt(
                                           SecureByteBlock &key,
                                           const char *hexIVAndBase64EncodedData
                                           )
    {
      return internal::Helper::splitDecrypt(key, hexIVAndBase64EncodedData);
    }

  }
}
