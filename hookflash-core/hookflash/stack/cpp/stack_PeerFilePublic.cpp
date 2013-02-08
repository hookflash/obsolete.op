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

#include <hookflash/stack/internal/stack_PeerFiles.h>
#include <hookflash/stack/internal/stack_PeerFilePublic.h>
#include <hookflash/stack/internal/stack_PeerFilePrivate.h>
#include <hookflash/services/ICanonicalXML.h>
#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/Numeric.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/queue.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/secblock.h>
#include <cryptopp/hex.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Numeric;
      using zsLib::Stringize;

      typedef zsLib::BYTE BYTE;
      typedef zsLib::UINT UINT;
      typedef zsLib::String String;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::Document Document;
      typedef CryptoPP::AutoSeededRandomPool AutoSeededRandomPool;
      typedef CryptoPP::ByteQueue ByteQueue;
      typedef CryptoPP::SecByteBlock SecureByteBlock;
      typedef CryptoPP::SHA256 SHA256;
      typedef CryptoPP::StringSink StringSink;
      typedef CryptoPP::RSASSA_PKCS1v15_SHA_Verifier CryptoPP_Verifier;
      typedef CryptoPP::HexEncoder HexEncoder;
      typedef PeerFilePublic::RSAPublicKeyPtr RSAPublicKeyPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      void convertFromBase64(
                             const String &input,
                             SecureByteBlock &output
                             );

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePublic::PeerFilePublic() :
        mID(zsLib::createPUID())
      {
      }

      //-----------------------------------------------------------------------
      void PeerFilePublic::init()
      {
        if (!mPublicKey) {
          ZS_LOG_DEBUG(log("attempting to load public key from public peer file") + ", contact ID=" + getContactID())
          SecureByteBlock publicKeyBuffer;
          getX509Certificate(publicKeyBuffer);
          if (publicKeyBuffer.SizeInBytes() > 0) {
            // attempt to load this key
            ZS_LOG_DEBUG(log("found public key and attempting to validate..."))
            mPublicKey = RSAPublicKey::load(publicKeyBuffer);
            if (mPublicKey) {
              ZS_LOG_DETAIL(log("validation passed") + ": contact ID=" + getContactID())
            } else {
              ZS_LOG_WARNING(Detail, log("validation **FAILED**") + ": contact ID=" + getContactID())
            }
          }
        } else {
          ZS_LOG_DEBUG(log("validation passed"))
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic => IPeerFilePublic
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePublicPtr PeerFilePublic::loadFromXML(ElementPtr publicPeerRootElement)
      {
        if (!publicPeerRootElement)
          return PeerFilePublicPtr();

        PeerFilePublicPtr pThis(new PeerFilePublic);
        pThis->mThisWeak = pThis;
        pThis->mDocument = Document::create();
        pThis->mDocument->adoptAsLastChild(publicPeerRootElement->clone());
        pThis->init();
        if (!pThis->mPublicKey) {
          ZS_LOG_WARNING(Basic, pThis->log("failed to load public key from public peer file"))
        }
        return pThis;
      }

      //-----------------------------------------------------------------------
      void PeerFilePublic::updateFrom(IPeerFilePublicPtr inAnotherVersion)
      {
        AutoRecursiveLock lock(mLock);

        PeerFilePublicPtr anotherVersion = PeerFilePublic::convert(inAnotherVersion);
        if (!anotherVersion) return;

        AutoRecursiveLock lock2(anotherVersion->mLock);
        if (getPeerFiles()) return;

        String contactIDThis = getContactID();
        String contactIDOther = anotherVersion->getContactID();
        if (contactIDThis != contactIDOther) return;

        if (!anotherVersion->mDocument) return;

        UINT versionThis = getVersionNumber();
        UINT versionOther = anotherVersion->getVersionNumber();

        if (versionThis > versionOther) return;
        if (versionOther > versionThis) {
          mDocument = (anotherVersion->mDocument->clone())->toDocumentChecked();
          return;
        }

        // if this version has more information, do not update
        if (containsSection("A") && (!anotherVersion->containsSection("A"))) return;
        if (containsSection("B") && (!anotherVersion->containsSection("B"))) return;
        if (containsSection("C") && (!anotherVersion->containsSection("C"))) return;

        bool identical = true;
        if (containsSection("A") != anotherVersion->containsSection("A")) identical = false;
        if (containsSection("B") != anotherVersion->containsSection("B")) identical = false;
        if (containsSection("C") != anotherVersion->containsSection("C")) identical = false;

        if (identical) return;
        mDocument = (anotherVersion->mDocument->clone())->toDocumentChecked();
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePublic::saveToXML() const
      {
        AutoRecursiveLock lock(mLock);

        if (!mDocument) return ElementPtr();
        ElementPtr peerRoot = mDocument->getFirstChildElement();
        if (!peerRoot) return ElementPtr();

        return (peerRoot->clone())->toElementChecked();
      }

      //-----------------------------------------------------------------------
      IPeerFilesPtr PeerFilePublic::getPeerFiles() const
      {
        AutoRecursiveLock lock(mLock);
        return mOuter.lock();
      }

      //-----------------------------------------------------------------------
      bool PeerFilePublic::isReadOnly() const
      {
        AutoRecursiveLock lock(mLock);
        return !(mOuter.lock());  // if there is no outer then it must be read only
      }

      //-----------------------------------------------------------------------
      UINT PeerFilePublic::getVersionNumber() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return 0;

        ElementPtr peerRoot = mDocument->getFirstChildElement();
        if (!peerRoot) return 0;
        String version = peerRoot->getAttributeValue("version");

        try {
          return (Numeric<UINT>(version));
        } catch(Numeric<UINT>::ValueOutOfRange &) {
        }
        return 0;
      }

      //-----------------------------------------------------------------------
      bool PeerFilePublic::containsSection(const char *sectionID) const
      {
        AutoRecursiveLock lock(mLock);
        return findSection(sectionID);
      }

      //-----------------------------------------------------------------------
      String PeerFilePublic::getContactID() const
      {
        AutoRecursiveLock lock(mLock);
        ElementPtr sectionElement = findSection("B");
        if (!sectionElement)
          sectionElement = findSection("C");
        if (!sectionElement)
          return calculateContactID();
        ElementPtr contactElement = sectionElement->findFirstChildElement("contact");
        if (!contactElement)
          return calculateContactID();
        String id = contactElement->getAttributeValue("id");
        if (id.isEmpty())
          return calculateContactID();
        return id;
      }

      //-----------------------------------------------------------------------
      String PeerFilePublic::calculateContactID() const
      {
        AutoRecursiveLock lock(mLock);
        ElementPtr sectionAElement = findSection("A");
        if (!sectionAElement) return String();

        String canonicalSectionA = hookflash::services::ICanonicalXML::convert(sectionAElement->getParentElementChecked());
        String contactID;

        // calculate the hash of section A - which is the contact ID
        {
          SecureByteBlock contactIDRaw(32);

          SHA256 contactIDSHA256;
          contactIDSHA256.Update((const BYTE *)canonicalSectionA.c_str(), canonicalSectionA.size());
          contactIDSHA256.Final(contactIDRaw);

          HexEncoder encoder(new StringSink(contactID));
          encoder.Put(contactIDRaw, contactIDRaw.size());
          encoder.MessageEnd();
        }
        return contactID;
      }

      //-----------------------------------------------------------------------
      String PeerFilePublic::getFindSecret() const
      {
        AutoRecursiveLock lock(mLock);
        ElementPtr sectionAElement = findSection("B");
        if (!sectionAElement) return String();

        ElementPtr findSecretElement = sectionAElement->findFirstChildElement("findSecret");
        String findSecret = findSecretElement->getText(true);

        return findSecret;
      }

      //-----------------------------------------------------------------------
      void PeerFilePublic::getURIs(
                                   const char *sectionID,
                                   URIList &outURIs
                                   ) const
      {
        outURIs.clear();

        AutoRecursiveLock lock(mLock);
        ElementPtr sectionElement = findSection(sectionID);
        if (!sectionElement) return;

        ElementPtr urisElement = sectionElement->findFirstChildElement("uris");
        if (!urisElement) return;

        ElementPtr uriElement = urisElement->getFirstChildElement();
        while (uriElement) {
          String name = uriElement->getValue();
          if (name == "uri") {
            String result = uriElement->getText(true);
            if (!result.isEmpty()) {
              outURIs.push_back(result);
            }
          }
          uriElement = uriElement->getNextSiblingElement();
        }
      }

      //-----------------------------------------------------------------------
      void PeerFilePublic::getX509Certificate(SecureByteBlock &outRaw) const
      {
        outRaw.New(0);

        AutoRecursiveLock lock(mLock);
        ElementPtr sectionElement = findSection("A");
        if (!sectionElement) return;

        try {
          ElementPtr signatureElement = sectionElement->getNextSiblingElementChecked();
          ElementPtr nextElement;
          for (; signatureElement; signatureElement = nextElement) {
            nextElement = signatureElement->getNextSiblingElement();
            try {
              if ("Signature" != signatureElement->getValue()) continue;

              String uri = signatureElement->findFirstChildElementChecked("SignedInfo")->findFirstChildElementChecked("Reference")->getAttributeValue("URI");
              uri.trim(" #\t\n\r\v");
              if ("A" != uri) continue;

              // found a match
              ElementPtr x509CertificateElement = signatureElement->findFirstChildElementChecked("KeyInfo")->findFirstChildElementChecked("X509Data")->findFirstChildElementChecked("X509Certificate");

              String x509Certificate = x509CertificateElement->getText();
              convertFromBase64(x509Certificate, outRaw);
              return;
            } catch(zsLib::XML::Exceptions::CheckFailed &) {
            }
          }
        } catch(zsLib::XML::Exceptions::CheckFailed &) {
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic => friend PeerFilePrivate
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePublicPtr PeerFilePublic::createFromPreGenerated(
                                                               PeerFilesPtr peerFiles,
                                                               DocumentPtr document,
                                                               RSAPublicKeyPtr publicKey
                                                               )
      {
        PeerFilePublicPtr pThis(new PeerFilePublic);
        pThis->mThisWeak = pThis;
        pThis->mOuter = peerFiles;
        pThis->mDocument = document;
        pThis->mPublicKey = publicKey;
        pThis->init();
        if (!pThis->mPublicKey) {
          ZS_LOG_ERROR(Basic, pThis->log("failed to load public key from XML"))
          return PeerFilePublicPtr();
        }
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePublicPtr PeerFilePublic::convert(IPeerFilePublicPtr peerFile)
      {
        return boost::dynamic_pointer_cast<PeerFilePublic>(peerFile);
      }

      //-----------------------------------------------------------------------
      String PeerFilePublic::log(const char *message) const
      {
        return String("PeerFilePublic [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePublic::findSection(const char *sectionID) const
      {
        if (!mDocument) return ElementPtr();

        ElementPtr peerRoot = mDocument->getFirstChildElement();
        if (!peerRoot) return ElementPtr();

        ElementPtr sectionBundleElement = peerRoot->getFirstChildElement();
        while (sectionBundleElement) {
          ElementPtr sectionElement = sectionBundleElement->getFirstChildElement();
          if (sectionElement) {
            String id = sectionElement->getAttributeValue("id");
            if (id == sectionID) return sectionElement;
          }
          sectionBundleElement = sectionBundleElement->getNextSiblingElement();
        }
        return ElementPtr();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic::RSAPublicKey
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePublic::RSAPublicKey::RSAPublicKey()
      {
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate::PeerFilePublic => friend PeerFilePublic
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPublicKeyPtr PeerFilePublic::RSAPublicKey::load(const SecureByteBlock &buffer)
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
      void PeerFilePublic::RSAPublicKey::save(SecureByteBlock &outBuffer) const
      {
        ByteQueue byteQueue;
        mPublicKey.Save(byteQueue);

        size_t outputLengthInBytes = (size_t)byteQueue.CurrentSize();
        outBuffer.CleanNew(outputLengthInBytes);

        byteQueue.Get(outBuffer, outputLengthInBytes);
      }

      //-----------------------------------------------------------------------
      bool PeerFilePublic::RSAPublicKey::verify(
                                                const String &inOriginalStrDataSigned,
                                                SecureByteBlock &inSignature
                                                ) const
      {
        CryptoPP_Verifier verifier(mPublicKey);

        try
        {
          bool result = verifier.VerifyMessage((const BYTE *)inOriginalStrDataSigned.c_str(), inOriginalStrDataSigned.length(), inSignature, inSignature.size());
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
    #pragma mark IPeerFilePublic
    #pragma mark

    //-------------------------------------------------------------------------
    IPeerFilePublicPtr IPeerFilePublic::loadFromXML(ElementPtr publicPeerRootElement)
    {
      return internal::PeerFilePublic::loadFromXML(publicPeerRootElement);
    }

  }
}
