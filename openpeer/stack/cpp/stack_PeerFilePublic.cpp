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

#include <hookflash/stack/internal/stack_PeerFiles.h>
#include <hookflash/stack/internal/stack_PeerFilePublic.h>
#include <hookflash/stack/internal/stack_PeerFilePrivate.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/IPeer.h>
#include <hookflash/stack/IRSAPublicKey.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#include <cryptopp/sha.h>


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      typedef CryptoPP::SHA256 SHA256;


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerFilePublicForPeerFiles
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerFilePublicForPeerFilePrivate
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePublicPtr IPeerFilePublicForPeerFilePrivate::createFromPublicKey(
                                                                               PeerFilesPtr peerFiles,
                                                                               DocumentPtr publicDoc,
                                                                               IRSAPublicKeyPtr publicKey,
                                                                               const String &peerURI
                                                                               )
      {
        return IPeerFilePublicFactory::singleton().createFromPublicKey(peerFiles, publicDoc, publicKey, peerURI);
      }

      //-----------------------------------------------------------------------
      PeerFilePublicPtr IPeerFilePublicForPeerFilePrivate::loadFromElement(
                                                                           PeerFilesPtr peerFiles,
                                                                           DocumentPtr publicDoc
                                                                           )
      {
        return IPeerFilePublicFactory::singleton().loadFromElement(peerFiles, publicDoc);
      }

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
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void PeerFilePublic::init()
      {
      }

      //-----------------------------------------------------------------------
      PeerFilePublic::~PeerFilePublic()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      PeerFilePublicPtr PeerFilePublic::convert(IPeerFilePublicPtr peerFilePublic)
      {
        return boost::dynamic_pointer_cast<PeerFilePublic>(peerFilePublic);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic => IPeerFilePublic
      #pragma mark

      //-----------------------------------------------------------------------
      String PeerFilePublic::toDebugString(IPeerFilePublicPtr peerFilePublic, bool includePrefixComma)
      {
        if (!peerFilePublic) return includePrefixComma ? String(", peer file public=(null)") : String("peer file public=(null)");
        return PeerFilePublic::convert(peerFilePublic)->getDebugValueString(includePrefixComma);
      }

      //-----------------------------------------------------------------------
      PeerFilePublicPtr PeerFilePublic::loadFromElement(ElementPtr publicPeerRootElement)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!publicPeerRootElement)

        PeerFilePublicPtr pThis(new PeerFilePublic);
        pThis->mThisWeak = pThis;
        pThis->mDocument = Document::create();
        pThis->mDocument->adoptAsFirstChild(publicPeerRootElement->clone());
        pThis->init();

        if (!pThis->load()) {
          ZS_LOG_WARNING(Detail, pThis->log("failed to load public peer file"))
          return PeerFilePublicPtr();
        }

        return pThis;
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePublic::saveToElement() const
      {
        if (!mDocument) return ElementPtr();
        ElementPtr rootEl = mDocument->getFirstChildElement();
        if (!rootEl) return ElementPtr();
        return rootEl->clone()->toElement();
      }

      //-----------------------------------------------------------------------
      String PeerFilePublic::getPeerURI() const
      {
        return mPeerURI;
      }

      //-----------------------------------------------------------------------
      Time PeerFilePublic::getCreated() const
      {
        ElementPtr sectionAEl = findSection("A");
        if (!sectionAEl) {
          ZS_LOG_WARNING(Detail, log("failed to find section A in public peer file, peer URI=") + mPeerURI)
          return Time();
        }
        try {
          return message::IMessageHelper::stringToTime(sectionAEl->findFirstChildElementChecked("created")->getTextDecoded());
        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("failed to find created, peer URI=") + mPeerURI)
        }
        return Time();
      }

      //-----------------------------------------------------------------------
      Time PeerFilePublic::getExpires() const
      {
        ElementPtr sectionAEl = findSection("A");
        if (!sectionAEl) {
          ZS_LOG_WARNING(Detail, log("failed to find section A in public peer file, peer URI=") + mPeerURI)
          return Time();
        }
        try {
          return message::IMessageHelper::stringToTime(sectionAEl->findFirstChildElementChecked("expires")->getTextDecoded());
        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("failed to find expires, peer URI=") + mPeerURI)
        }
        return Time();
      }

      //-----------------------------------------------------------------------
      String PeerFilePublic::getFindSecret() const
      {
        ElementPtr sectionBEl = findSection("B");
        if (!sectionBEl) {
          ZS_LOG_WARNING(Detail, log("failed to find section B in public peer file, peer URI=") + mPeerURI)
          return String();
        }

        try {
          return sectionBEl->findFirstChildElementChecked("findSecret")->getTextDecoded();
        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("failed to obtain find secret, peer URI=") + mPeerURI)
        }
        return String();
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePublic::getSignedSaltBundle() const
      {
        ElementPtr sectionAEl = findSection("A");
        if (!sectionAEl) {
          ZS_LOG_WARNING(Detail, log("failed to find section A in public peer file, peer URI=") + mPeerURI)
          return ElementPtr();
        }

        try {
          return sectionAEl->findFirstChildElementChecked("saltBundle")->clone()->toElement();
        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("failed to obtain salt bundle, peer URI=") + mPeerURI)
        }
        return ElementPtr();
      }

      //-----------------------------------------------------------------------
      IdentityBundleElementListPtr PeerFilePublic::getIdentityBundles() const
      {
        IdentityBundleElementListPtr result(new IdentityBundleElementList);

        ElementPtr sectionCEl = findSection("C");
        if (!sectionCEl) {
          ZS_LOG_WARNING(Detail, log("failed to find section C in public peer file, peer URI=") + mPeerURI)
          return result;
        }

        try {
          ElementPtr identitiesEl = sectionCEl->findFirstChildElement("identities");
          if (!identitiesEl) return result;

          ElementPtr identityBundleEl = identitiesEl->findFirstChildElement("identityBundle");
          while (identityBundleEl) {
            result->push_back(identityBundleEl->clone()->toElement());
            identityBundleEl = identityBundleEl->findNextSiblingElement("identityBundle");
          }
        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("failed to obtain salt bundle, peer URI=") + mPeerURI)
        }
        return result;
      }

      //-----------------------------------------------------------------------
      IPeerFilesPtr PeerFilePublic::getAssociatedPeerFiles() const
      {
        return mOuter.lock();
      }

      //-----------------------------------------------------------------------
      IPeerFilePrivatePtr PeerFilePublic::getAssociatedPeerFilePrivate() const
      {
        IPeerFilesPtr peerFiles = mOuter.lock();
        if (!peerFiles) return IPeerFilePrivatePtr();
        return peerFiles->getPeerFilePrivate();
      }

      //-----------------------------------------------------------------------
      IRSAPublicKeyPtr PeerFilePublic::getPublicKey() const
      {
        return mPublicKey;
      }

      //-----------------------------------------------------------------------
      bool PeerFilePublic::verifySignature(ElementPtr signedEl) const
      {
        ZS_THROW_BAD_STATE_IF(!mPublicKey)
        ZS_THROW_INVALID_ARGUMENT_IF(!signedEl)

        ElementPtr signatureEl;
        signedEl = IHelper::getSignatureInfo(signedEl, &signatureEl);

        if (!signedEl) {
          ZS_LOG_WARNING(Detail, log("signature validation failed because no signed element found"))
          return false;
        }

        // found the signature reference, now check if the peer URIs match - they must...
        try {
          String foundURI = signatureEl->findFirstChildElementChecked("key")->findFirstChildElementChecked("uri")->getTextDecoded();
          if (foundURI != mPeerURI) {
            ZS_LOG_WARNING(Detail, log("signature validation failed since was not signed by this peer file, signature's URI=") + foundURI + ", peer file URI=" + mPeerURI)
            return false;
          }

          String algorithm = signatureEl->findFirstChildElementChecked("algorithm")->getTextDecoded();
          if (algorithm != HOOKFLASH_STACK_PEER_FILE_SIGNATURE_ALGORITHM) {
            ZS_LOG_WARNING(Detail, log("signature validation algorithm is not understood, algorithm=") + algorithm)
            return false;
          }

          String signatureDigestAsString = signatureEl->findFirstChildElementChecked("digestValue")->getTextDecoded();

          ElementPtr canonicalSigned = Helper::cloneAsCanonicalJSON(signedEl);

          GeneratorPtr generator = Generator::createJSONGenerator();
          boost::shared_array<char> signedElAsJSON = generator->write(canonicalSigned);

          SecureByteBlockPtr actualDigest = IHelper::hash((const char *)(signedElAsJSON.get()), IHelper::HashAlgorthm_SHA1);

          if (0 != IHelper::compare(*actualDigest, *IHelper::convertFromBase64(signatureDigestAsString))) {
            ZS_LOG_WARNING(Detail, log("digest values did not match, signature digest=") + signatureDigestAsString + ", actual digest=" + IHelper::convertToBase64(*actualDigest))
            return false;
          }

          SecureByteBlockPtr signatureDigestSigned = IHelper::convertFromBase64(signatureEl->findFirstChildElementChecked("digestSigned")->getTextDecoded());

          if (!mPublicKey->verify(*actualDigest, *signatureDigestSigned)) {
            ZS_LOG_WARNING(Detail, log("signature failed to validate") + ", peer URI=" + mPeerURI)
            return false;
          }

        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("signature missing element"))
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr PeerFilePublic::encrypt(const SecureByteBlock &buffer) const
      {
        return mPublicKey->encrypt(buffer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic => IPeerFiles
      #pragma mark

      //-----------------------------------------------------------------------
      String PeerFilePublic::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("peer file public id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("peer uri", mPeerURI, firstTime) +
               Helper::getDebugValue("created", IMessageHelper::timeToString(getCreated()), firstTime) +
               Helper::getDebugValue("expires", IMessageHelper::timeToString(getExpires()), firstTime) +
               Helper::getDebugValue("find secret", getFindSecret(), firstTime);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic => IPeerFilePublicForPeerFilePrivate
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePublicPtr PeerFilePublic::createFromPublicKey(
                                                            PeerFilesPtr peerFiles,
                                                            DocumentPtr publicDoc,
                                                            IRSAPublicKeyPtr publicKey,
                                                            const String &peerURI
                                                            )
      {
        PeerFilePublicPtr pThis(new PeerFilePublic);
        pThis->mThisWeak = pThis;
        pThis->mOuter = peerFiles;
        pThis->mDocument = publicDoc;
        pThis->mPublicKey = publicKey;
        pThis->mPeerURI = peerURI;
        pThis->init();

        return pThis;
      }

      //-----------------------------------------------------------------------
      PeerFilePublicPtr PeerFilePublic::loadFromElement(
                                                        PeerFilesPtr peerFiles,
                                                        DocumentPtr publicDoc
                                                        )
      {
        PeerFilePublicPtr pThis(new PeerFilePublic);
        pThis->mThisWeak = pThis;
        pThis->mOuter = peerFiles;
        pThis->mDocument = publicDoc;
        pThis->init();
        if (!pThis->load()) {
          ZS_LOG_ERROR(Detail, pThis->log("public peer file failed to parse"))
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
      String PeerFilePublic::log(const char *message) const
      {
        return String("PeerFilePublic [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      bool PeerFilePublic::load()
      {
        ElementPtr sectionAEl = findSection("A");
        if (!sectionAEl) {
          ZS_LOG_ERROR(Detail, log("unable to find section A in the public peer file"))
          return false;
        }

        try {
          String cipher = sectionAEl->findFirstChildElementChecked("cipher")->getTextDecoded();
          if (HOOKFLASH_STACK_PEER_FILE_CIPHER != cipher) {
            ZS_LOG_WARNING(Detail, log("cipher suite is not understood, cipher suite=") + cipher + ", expecting=" + HOOKFLASH_STACK_PEER_FILE_CIPHER)
            return false;
          }

          GeneratorPtr generator = Generator::createJSONGenerator();

          String contactID;
          {
            ULONG length = 0;
            ElementPtr bundleAEl = sectionAEl->getParentElementChecked();
            ElementPtr canonicalBundleAEl = Helper::cloneAsCanonicalJSON(bundleAEl);
            boost::shared_array<char> sectionAsString = generator->write(canonicalBundleAEl, &length);

            SHA256 sha256;
            SecureByteBlock bundleHash(sha256.DigestSize());

            sha256.Update((const BYTE *)"contact:", strlen("contact:"));
            sha256.Update((const BYTE *)(sectionAsString.get()), length);
            sha256.Final(bundleHash);

            contactID = IHelper::convertToHex(bundleHash);
          }

          ElementPtr domainEl;
          try {
            ElementPtr saltSignatureEl;
            if (!IHelper::getSignatureInfo(sectionAEl->findFirstChildElementChecked("saltBundle")->findFirstChildElementChecked("salt"), &saltSignatureEl)) {
              ZS_LOG_ERROR(Basic, log("failed to find salt signature"))
            }
            domainEl = saltSignatureEl->findFirstChildElementChecked("key")->findFirstChildElementChecked("domain");
          } catch (CheckFailed &) {
            ZS_LOG_ERROR(Basic, log("failed to obtain signature domain from signed salt"))
            return false;
          }
          String domain = domainEl->getTextDecoded();
          if (domain.length() < 1) {
            ZS_LOG_ERROR(Basic, log("domain from signed salt was empty"))
            return false;
          }

          mPeerURI = IPeer::joinURI(domain, contactID);
          if (!mPeerURI) {
            ZS_LOG_ERROR(Basic, log("failed to generate a proper peer URI"))
            return false;
          }

          ElementPtr signatureEl;
          if (!IHelper::getSignatureInfo(sectionAEl, &signatureEl)) {
            ZS_LOG_ERROR(Basic, log("failed to obtain signature domain from signed salt"))
            return false;
          }

          SecureByteBlockPtr x509Certificate = IHelper::convertFromBase64(signatureEl->findFirstChildElementChecked("key")->findFirstChildElementChecked("x509Data")->getTextDecoded());

          mPublicKey = IRSAPublicKey::load(*x509Certificate);
          if (!mPublicKey) {
            ZS_LOG_ERROR(Detail, log("failed to load public key, peer URI=") + mPeerURI)
            return false;
          }

          {
            ULONG length = 0;
            ElementPtr canonicalSectionAEl = Helper::cloneAsCanonicalJSON(sectionAEl);
            boost::shared_array<char> sectionAAsString = generator->write(canonicalSectionAEl, &length);

            SecureByteBlockPtr sectionHash = IHelper::hash((const char *)(sectionAAsString.get()), IHelper::HashAlgorthm_SHA1);
            String algorithm = signatureEl->findFirstChildElementChecked("algorithm")->getTextDecoded();
            if (HOOKFLASH_STACK_PEER_FILE_SIGNATURE_ALGORITHM != algorithm) {
              ZS_LOG_WARNING(Detail, log("signature algorithm was not understood, peer URI=") + mPeerURI + ", algorithm=" + algorithm + ", expecting=" + HOOKFLASH_STACK_PEER_FILE_SIGNATURE_ALGORITHM)
              return false;
            }

            SecureByteBlockPtr digestValue = IHelper::convertFromBase64(signatureEl->findFirstChildElementChecked("digestValue")->getTextDecoded());

            if (0 != IHelper::compare(*sectionHash, *digestValue)) {
              ZS_LOG_ERROR(Detail, log("digest value does not match on section A signature on public peer file, peer URI=") + mPeerURI +  ", calculated digest=" + IHelper::convertToBase64(*sectionHash) + ", signature digest=" + IHelper::convertToBase64(*digestValue))
              return false;
            }

            SecureByteBlockPtr digestSigned =  IHelper::convertFromBase64(signatureEl->findFirstChildElementChecked("digestSigned")->getTextDecoded());
            if (!mPublicKey->verify(*sectionHash, *digestSigned)) {
              ZS_LOG_ERROR(Detail, log("signature on section A of public peer file failed to validate, peer URI=") + mPeerURI)
              return false;
            }
          }
        } catch(CheckFailed &) {
          ZS_LOG_ERROR(Detail, log("unable to parse the public peer file"))
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePublic::findSection(const char *sectionID) const
      {
        if (!mDocument) return ElementPtr();

        ElementPtr peerRootEl = mDocument->getFirstChildElement();
        if (!peerRootEl) return ElementPtr();

        ElementPtr sectionBundleEl = peerRootEl->getFirstChildElement();
        while (sectionBundleEl) {
          ElementPtr sectionEl = sectionBundleEl->getFirstChildElement();
          if (sectionEl) {
            String id = sectionEl->getAttributeValue("id");
            if (id == sectionID) return sectionEl;
          }
          sectionBundleEl = sectionBundleEl->getNextSiblingElement();
        }
        return ElementPtr();
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
    String IPeerFilePublic::toDebugString(IPeerFilePublicPtr peerFilePublic, bool includeCommaPrefix)
    {
      return internal::PeerFilePublic::toDebugString(peerFilePublic, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IPeerFilePublicPtr IPeerFilePublic::loadFromElement(ElementPtr publicPeerRootElement)
    {
      return internal::IPeerFilePublicFactory::singleton().loadFromElement(publicPeerRootElement);
    }
  }
}
