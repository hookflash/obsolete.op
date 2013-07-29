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

#include <openpeer/stack/internal/stack_PeerFiles.h>
#include <openpeer/stack/internal/stack_PeerFilePublic.h>
#include <openpeer/stack/internal/stack_PeerFilePrivate.h>
#include <openpeer/stack/IPeer.h>
#include <openpeer/stack/message/IMessageHelper.h>

#include <openpeer/services/IHelper.h>
#include <openpeer/services/IRSAPrivateKey.h>
#include <openpeer/services/IRSAPublicKey.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <cryptopp/sha.h>

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      using services::IHelper;

      using CryptoPP::SHA256;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerFilePrivateForPeerFiles
      #pragma mark

      //-----------------------------------------------------------------------
      bool IPeerFilePrivateForPeerFiles::generate(
                                                  PeerFilesPtr peerFiles,
                                                  PeerFilePrivatePtr &outPeerFilePrivate,
                                                  PeerFilePublicPtr &outPeerFilePublic,
                                                  const char *password,
                                                  ElementPtr signedSalt
                                                  )
      {
        return IPeerFilePrivateFactory::singleton().generate(peerFiles, outPeerFilePrivate, outPeerFilePublic, password, signedSalt);
      }

      //-----------------------------------------------------------------------
      bool IPeerFilePrivateForPeerFiles::loadFromElement(
                                                         PeerFilesPtr peerFiles,
                                                         PeerFilePrivatePtr &outPeerFilePrivate,
                                                         PeerFilePublicPtr &outPeerFilePublic,
                                                         const char *password,
                                                         ElementPtr peerFileRootElement
                                                         )
      {
        return IPeerFilePrivateFactory::singleton().loadFromElement(peerFiles, outPeerFilePrivate, outPeerFilePublic, password, peerFileRootElement);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePrivate::PeerFilePrivate(PeerFilesPtr peerFiles) :
        mID(zsLib::createPUID()),
        mOuter(peerFiles)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void PeerFilePrivate::init()
      {
      }

      //-----------------------------------------------------------------------
      PeerFilePrivate::~PeerFilePrivate()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      PeerFilePrivatePtr PeerFilePrivate::convert(IPeerFilePrivatePtr peerFilePrivate)
      {
        return boost::dynamic_pointer_cast<PeerFilePrivate>(peerFilePrivate);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate => IPeerFilePrivate
      #pragma mark

      //-----------------------------------------------------------------------
      IPeerFilesPtr PeerFilePrivate::getAssociatedPeerFiles() const
      {
        return mOuter.lock();
      }

      //-----------------------------------------------------------------------
      IPeerFilePublicPtr PeerFilePrivate::getAssociatedPeerFilePublic() const
      {
        IPeerFilesPtr outer = getAssociatedPeerFiles();
        if (!outer) return IPeerFilePublicPtr();
        return outer->getPeerFilePublic();
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePrivate::saveToElement() const
      {
        if (!mDocument) return ElementPtr();

        ElementPtr peerRoot = mDocument->getFirstChildElement();
        if (!peerRoot) return ElementPtr();

        return (peerRoot->clone())->toElementChecked();
      }

      //-----------------------------------------------------------------------
      IRSAPrivateKeyPtr PeerFilePrivate::getPrivateKey() const
      {
        return mPrivateKey;
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePrivate::getPrivateData() const
      {
        if (!mPrivateDataDoc) return ElementPtr();
        ElementPtr childEl = mPrivateDataDoc->getFirstChildElement();
        if (!childEl) return ElementPtr();
        return childEl->clone()->toElement();
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr PeerFilePrivate::getPassword(bool appendNUL) const
      {
        return IHelper::convertToBuffer((const char *) ((const BYTE *)*mPassword), appendNUL);
      }

      //-----------------------------------------------------------------------
      String PeerFilePrivate::getSecretProof() const
      {
        ElementPtr sectionEl = findSection("A");
        if (sectionEl) return String();

        ElementPtr proofEl = sectionEl->findFirstChildElement("secretProof");
        if (!proofEl) return String();

        return proofEl->getTextDecoded();
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr PeerFilePrivate::getSalt() const
      {
        ElementPtr sectionAEl = findSection("A");

        if (!sectionAEl) {
          ZS_LOG_ERROR(Detail, log("could not find section 'A'") + IPeerFilePrivate::toDebugString(mThisWeak.lock()))
          return SecureByteBlockPtr();
        }

        try {
          return IHelper::convertFromBase64(sectionAEl->findFirstChildElementChecked("salt")->getTextDecoded());
        } catch(CheckFailed &) {
        }
        ZS_LOG_ERROR(Detail, log("could not find salt in private peer file") + IPeerFilePrivate::toDebugString(mThisWeak.lock()))
        return SecureByteBlockPtr();
      }

      //-----------------------------------------------------------------------
      void PeerFilePrivate::signElement(
                                        ElementPtr elementToSign,
                                        SignatureTypes signatureType,
                                        IRSAPublicKeyPtr publicKey
                                        ) const
      {
        ZS_THROW_BAD_STATE_IF(!mPrivateKey)
        ZS_THROW_INVALID_ARGUMENT_IF(!elementToSign)

        if (mPeerURI.length() < 1) {
          if (SignatureType_PeerURI == signatureType) {
            // cannot sign with reference when peer URI is not set
            signatureType = SignatureType_FullPublicKey;
          }
        }

        ElementPtr signatureEl;

        bool randomized = false;
        String id = elementToSign->getAttributeValue("id");
        if (id.length() < 1) {
          id = IHelper::convertToHex(*IHelper::random(16));
          elementToSign->setAttribute("id", id);
          randomized = true;
        }

        String referenceID = "#" + id;

        if (!randomized) {
          ElementPtr altSignatureEl = elementToSign->findNextSiblingElement("signature");
          while (altSignatureEl) {
            ElementPtr referenceEl = altSignatureEl->findFirstChildElement("reference");
            if (referenceEl) {
              String value = referenceEl->getTextDecoded();
              if (value == referenceID) {
                signatureEl = altSignatureEl;
              }
            }
          }

          if (signatureEl) {
            // clean out the current signature entirely since it will be replaced
            NodePtr child = signatureEl->getFirstChild();
            while (child) {
              child->orphan();
              child = signatureEl->getFirstChild();
            }
          }
        }

        GeneratorPtr generator = Generator::createJSONGenerator();

        ElementPtr canonicalJSONEl = IHelper::cloneAsCanonicalJSON(elementToSign);
        boost::shared_array<char> elementAsJSON = generator->write(canonicalJSONEl);

        SecureByteBlockPtr elementHash = IHelper::hash(elementAsJSON.get(), IHelper::HashAlgorthm_SHA1);

        if (!signatureEl)
          signatureEl = Element::create("signature");
        signatureEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("reference", referenceID));
        signatureEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("algorithm", OPENPEER_STACK_PEER_FILE_SIGNATURE_ALGORITHM));
        signatureEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("digestValue", IHelper::convertToBase64(*elementHash)));
        signatureEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("digestSigned", IHelper::convertToBase64(*mPrivateKey->sign(*elementHash))));

        ElementPtr keyEl = Element::create("key");


        switch (signatureType) {
          case SignatureType_FullPublicKey:
          case SignatureType_Fingerprint:
          {
            if (!publicKey) {
              IPeerFilePublicPtr peerFilePublic = getAssociatedPeerFilePublic();
              ZS_THROW_BAD_STATE_IF(!peerFilePublic)

              publicKey = peerFilePublic->getPublicKey();
              ZS_THROW_BAD_STATE_IF(!publicKey)
            }
            break;
          }
          case SignatureType_PeerURI:       break;
        }

        switch (signatureType) {
          case SignatureType_FullPublicKey:
          {
            ElementPtr x509DataEl = message::IMessageHelper::createElementWithText("x509Data", IHelper::convertToBase64(*publicKey->save()));
            keyEl->adoptAsLastChild(x509DataEl);
            break;
          }
          case SignatureType_PeerURI:
          {
            ElementPtr uriEl = message::IMessageHelper::createElementWithText("uri", mPeerURI);
            keyEl->adoptAsLastChild(uriEl);
            break;
          }
          case SignatureType_Fingerprint:
          {
            ElementPtr fingerprintEl = message::IMessageHelper::createElementWithText("fingerprint", publicKey->getFingerprint());
            keyEl->adoptAsLastChild(fingerprintEl);
            break;
          }
        }

        signatureEl->adoptAsLastChild(keyEl);

        if (!signatureEl->getParent()) {
          // this signature is not part of the tree structure right now, add it...
          elementToSign->adoptAsNextSibling(signatureEl);
        }
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr PeerFilePrivate::decrypt(const SecureByteBlock &buffer) const
      {
        return mPrivateKey->decrypt(buffer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate => IPeerFilePrivateForPeerFiles
      #pragma mark

      //-----------------------------------------------------------------------
      bool PeerFilePrivate::generate(
                                     PeerFilesPtr peerFiles,
                                     PeerFilePrivatePtr &outPeerFilePrivate,
                                     PeerFilePublicPtr &outPeerFilePublic,
                                     const char *password,
                                     ElementPtr signedSaltBundleEl
                                     )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFiles)
        ZS_THROW_INVALID_ARGUMENT_IF(!password)
        ZS_THROW_INVALID_ARGUMENT_IF(!signedSaltBundleEl)

        PeerFilePrivatePtr pThis(new PeerFilePrivate(peerFiles));
        pThis->mThisWeak = pThis;
        pThis->init();

        IRSAPublicKeyPtr publicKey;
        pThis->mPrivateKey = IRSAPrivateKey::generate(publicKey);

        if ((!pThis->mPrivateKey) ||
            (!publicKey)) {
          ZS_LOG_ERROR(Basic, pThis->log("failed to generate public/privat key pair"))
          return false;
        }

        pThis->mPassword = IHelper::convertToBuffer(password);
        DocumentPtr publicDoc;

        if (!pThis->generateDocuments(publicKey, signedSaltBundleEl, publicDoc)) {
          ZS_LOG_ERROR(Basic, pThis->log("failed to generate public/private key document"))
        }

        outPeerFilePublic = IPeerFilePublicForPeerFilePrivate::createFromPublicKey(peerFiles, publicDoc, publicKey, pThis->mPeerURI);
        outPeerFilePrivate = pThis;

        return peerFiles;
      }

      //-----------------------------------------------------------------------
      bool PeerFilePrivate::loadFromElement(
                                            PeerFilesPtr peerFiles,
                                            PeerFilePrivatePtr &outPeerFilePrivate,
                                            PeerFilePublicPtr &outPeerFilePublic,
                                            const char *password,
                                            ElementPtr peerFileRootElement
                                            )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFiles)
        ZS_THROW_INVALID_ARGUMENT_IF(!password)
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFileRootElement)

        PeerFilePrivatePtr pThis(new PeerFilePrivate(peerFiles));
        pThis->mThisWeak = pThis;
        pThis->init();

        pThis->mPassword = IHelper::convertToBuffer(password);
        DocumentPtr publicDoc;

        bool loaded = pThis->loadFromElement(peerFileRootElement, publicDoc);
        if (!loaded) {
          ZS_LOG_ERROR(Basic, pThis->log("failed to load private peer file"))
          return PeerFilesPtr();
        }

        PeerFilePublicPtr peerFilePublic = IPeerFilePublicForPeerFilePrivate::loadFromElement(peerFiles, publicDoc);
        if (!peerFilePublic) {
          ZS_LOG_ERROR(Basic, pThis->log("failed to load public peer file"))
          return false;
        }

        if (!pThis->verifySignatures(peerFilePublic)) {
          ZS_LOG_ERROR(Basic, pThis->log("signatures did not validate in private peer file")  + IPeerFilePublic::toDebugString(peerFilePublic))
          return false;
        }

        outPeerFilePublic = peerFilePublic;
        outPeerFilePrivate = pThis;

        return peerFiles;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String PeerFilePrivate::log(const char *message) const
      {
        return String("PeerFilePrivate [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr PeerFilePrivate::getKey(
                                                 const char *phrase,
                                                 const char *saltInBase64
                                                 )
      {
        return IHelper::hmac(*IHelper::hmacKeyFromPassphrase((const char *)((const BYTE *)(*mPassword))), String(phrase) + ":" + saltInBase64, IHelper::HashAlgorthm_SHA256);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr PeerFilePrivate::getIV(
                                                const char *phrase,
                                                const char *saltInBase64
                                                )
      {
        return IHelper::hash(String(phrase) + ":" + saltInBase64, IHelper::HashAlgorthm_MD5);
      }

      //-----------------------------------------------------------------------
      String PeerFilePrivate::encrypt(
                                      const char *phrase,
                                      const char *saltAsBase64,
                                      SecureByteBlock &buffer
                                      )
      {
        SecureByteBlockPtr key = getKey(phrase, saltAsBase64);
        SecureByteBlockPtr iv = getIV(phrase, saltAsBase64);
        return IHelper::convertToBase64(*IHelper::encrypt(*key, *iv, buffer));
      }

      //-----------------------------------------------------------------------
      String PeerFilePrivate::encrypt(
                                      const char *phrase,
                                      const char *saltAsBase64,
                                      const char *value
                                      )
      {
        SecureByteBlockPtr key = getKey(phrase, saltAsBase64);
        SecureByteBlockPtr iv = getIV(phrase, saltAsBase64);
        return IHelper::convertToBase64(*IHelper::encrypt(*key, *iv, value));
      }

      //-----------------------------------------------------------------------
      String PeerFilePrivate::decryptString(
                                            const char *phrase,
                                            const String &saltAsBase64,
                                            const String &encryptedStringInBase64
                                            )
      {
        SecureByteBlockPtr key = getKey(phrase, saltAsBase64);
        SecureByteBlockPtr iv = getIV(phrase, saltAsBase64);
        SecureByteBlockPtr output = IHelper::decrypt(*key, *iv, *IHelper::convertFromBase64(encryptedStringInBase64));
        return IHelper::convertToString(*output);
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr PeerFilePrivate::decryptBuffer(
                                                        const char *phrase,
                                                        const String &saltAsBase64,
                                                        const String &encryptedStringInBase64
                                                        )
      {
        SecureByteBlockPtr key = getKey(phrase, saltAsBase64);
        SecureByteBlockPtr iv = getIV(phrase, saltAsBase64);
        return IHelper::decrypt(*key, *iv, *IHelper::convertFromBase64(encryptedStringInBase64));
      }

      //-----------------------------------------------------------------------
      bool PeerFilePrivate::generateDocuments(
                                              IRSAPublicKeyPtr publicKey,
                                              ElementPtr signedSaltBundleEl,
                                              DocumentPtr &outPublicPeerDocument
                                              )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!signedSaltBundleEl)

        String contactID;
        String domain;

        // generate public peer file
        {
          DocumentPtr document = Document::create();

          ElementPtr peerEl = Element::create("peer");
          peerEl->setAttribute("version", "1");

          // public peer section "A"
          {
            ElementPtr sectionBundleEl = Element::create("sectionBundle");

            ElementPtr sectionEl = message::IMessageHelper::createElementWithID("section", "A");
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("algorithm", OPENPEER_STACK_PEER_FILE_CIPHER));

            Time created = zsLib::now();
            Time expires = created + Duration(Hours(OPENPEER_STACK_PEER_FILE_PRIVATE_KEY_EXPIRY_IN_HOURS));

            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithNumber("created", Stringize<time_t>(zsLib::toEpoch(created))));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithNumber("expires", Stringize<time_t>(zsLib::toEpoch(expires))));

            sectionEl->adoptAsLastChild(signedSaltBundleEl->clone());

            sectionBundleEl->adoptAsLastChild(sectionEl);
            peerEl->adoptAsLastChild(sectionBundleEl);

            signElement(sectionEl, SignatureType_FullPublicKey, publicKey);

            ElementPtr domainEl;
            try {
              ElementPtr saltSignatureEl;
              if (!IHelper::getSignatureInfo(signedSaltBundleEl->findFirstChildElementChecked("salt"), &saltSignatureEl)) {
                ZS_LOG_ERROR(Basic, log("failed to find salt signature"))
              }
              domainEl = saltSignatureEl->findFirstChildElementChecked("key")->findFirstChildElementChecked("domain");
            } catch (CheckFailed &) {
              ZS_LOG_ERROR(Basic, log("failed to obtain signature domain from signed salt"))
              return false;
            }
            domain = domainEl->getTextDecoded();
            if (domain.length() < 1) {
              ZS_LOG_ERROR(Basic, log("domain from signed salt was empty"))
              return false;
            }

            // calculate the contact ID/domain

            GeneratorPtr generator = Generator::createJSONGenerator();

            ULONG length = 0;
            ElementPtr canoncialSectionBundleEl = IHelper::cloneAsCanonicalJSON(sectionBundleEl);
            boost::shared_array<char> sectionAsJSON = generator->write(canoncialSectionBundleEl, &length);

            SHA256 sha256;
            SecureByteBlock bundleHash(sha256.DigestSize());

            sha256.Update((const BYTE *)"contact:", strlen("contact:"));
            sha256.Update((const BYTE *)sectionAsJSON.get(), length);
            sha256.Final(bundleHash);

            contactID = IHelper::convertToHex(bundleHash);
          }

          mPeerURI = IPeer::joinURI(domain, contactID);

          ZS_THROW_BAD_STATE_IF(domain.length() < 1)
          ZS_THROW_BAD_STATE_IF(contactID.length() < 1)
          ZS_THROW_BAD_STATE_IF(mPeerURI.length() < 1)

          // public section "B"
          {
            ElementPtr sectionBundleEl = Element::create("sectionBundle");

            ElementPtr sectionEl = message::IMessageHelper::createElementWithID("section", "B");
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithTextAndJSONEncode("contact", mPeerURI));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithTextAndJSONEncode("findSecret", IHelper::randomString((32*8)/5+1)));

            sectionBundleEl->adoptAsLastChild(sectionEl);
            peerEl->adoptAsLastChild(sectionBundleEl);

            signElement(sectionEl);
          }

          // public section "C"
          {
            ElementPtr sectionBundleEl = Element::create("sectionBundle");

            ElementPtr sectionEl = message::IMessageHelper::createElementWithID("section", "C");
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithTextAndJSONEncode("contact", mPeerURI));

            sectionBundleEl->adoptAsLastChild(sectionEl);
            peerEl->adoptAsLastChild(sectionBundleEl);

            signElement(sectionEl);
          }
          document->adoptAsLastChild(peerEl);
          outPublicPeerDocument = document;
        }

        // generate private peer file
        {
          mDocument = Document::create();

          ElementPtr privatePeerEl = Element::create("privatePeer");
          privatePeerEl->setAttribute("version", "1");

          SecureByteBlockPtr salt = IHelper::random(32);
          String saltAsString = IHelper::convertToBase64(*salt);

          // private peer section "A"
          {
            ElementPtr sectionBundleEl = Element::create("sectionBundle");

            ElementPtr sectionEl = message::IMessageHelper::createElementWithID("section", "A");
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("contact", mPeerURI));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("algorithm", OPENPEER_STACK_PEER_FILE_CIPHER));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("salt", saltAsString));

            String secretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase((const char *)((const BYTE *)(*mPassword))), "proof:" + contactID, IHelper::HashAlgorthm_SHA256));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("secretProof", secretProof));

            sectionBundleEl->adoptAsLastChild(sectionEl);
            privatePeerEl->adoptAsLastChild(sectionBundleEl);

            signElement(sectionEl);
          }

          // private peer section "B"
          {
            ElementPtr sectionBundleEl = Element::create("sectionBundle");

            ElementPtr sectionEl = message::IMessageHelper::createElementWithID("section", "B");
            //sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("contact", mPeerURI));

            SecureByteBlockPtr privateKey = mPrivateKey->save();

            String encryptedContact = encrypt("contact", saltAsString, mPeerURI);

            String encryptedPrivateKey = encrypt("privatekey", saltAsString, *privateKey);

            GeneratorPtr generator = Generator::createJSONGenerator();
            boost::shared_array<char> publicPeerAsString = generator->write(outPublicPeerDocument);

            String encryptedPublicPeer = encrypt("peer", saltAsString, (const char *)(publicPeerAsString.get()));

            DocumentPtr docPrivateData = Document::create();
            boost::shared_array<char> dataAsString = generator->write(docPrivateData);

            mPrivateDataDoc = docPrivateData->clone()->toDocument();

            String encryptedPrivateData = encrypt("data", saltAsString, (const char *)(dataAsString.get()));

            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("encryptedContact", encryptedContact));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("encryptedPrivateKey", encryptedPrivateKey));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("encryptedPeer", encryptedPublicPeer));
            sectionEl->adoptAsLastChild(message::IMessageHelper::createElementWithText("encryptedPrivateData", encryptedPrivateData));

            sectionBundleEl->adoptAsLastChild(sectionEl);
            privatePeerEl->adoptAsLastChild(sectionBundleEl);

            signElement(sectionEl);
          }

          mDocument->adoptAsLastChild(privatePeerEl);
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool PeerFilePrivate::loadFromElement(
                                            ElementPtr peerFileRootElement,
                                            DocumentPtr &outPublicPeerDocument
                                            )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFileRootElement)

        if ("privatePeer" != peerFileRootElement->getValue()) {
          ZS_LOG_ERROR(Basic, log("root element is not privatePeer"))
          return false;
        }

        mDocument = Document::create();
        mDocument->adoptAsLastChild(peerFileRootElement->clone());

        ElementPtr sectionAEl = findSection("A");
        ElementPtr sectionBEl = findSection("B");

        if ((!sectionAEl) || (!sectionBEl)) {
          ZS_LOG_ERROR(Detail, log("could not find either section 'A' or section 'B' in private peer file"))
          return false;
        }

        try {
          String contact = sectionAEl->findFirstChildElementChecked("contact")->getTextDecoded();
          String cipher = sectionAEl->findFirstChildElementChecked("algorithm")->getTextDecoded();
          String salt = sectionAEl->findFirstChildElementChecked("salt")->getTextDecoded();
          String secretProof = sectionAEl->findFirstChildElementChecked("secretProof")->getTextDecoded();

          String encryptedContact = sectionBEl->findFirstChildElementChecked("encryptedContact")->getTextDecoded();
          String encryptedPrivateKey = sectionBEl->findFirstChildElementChecked("encryptedPrivateKey")->getTextDecoded();
          String encryptedPeer = sectionBEl->findFirstChildElementChecked("encryptedPeer")->getTextDecoded();
          String encryptedPrivateData = sectionBEl->findFirstChildElementChecked("encryptedPrivateData")->getTextDecoded();

          // now to decrypt all this data (if possible)
          if (OPENPEER_STACK_PEER_FILE_CIPHER != cipher) {
            ZS_LOG_WARNING(Detail, log("unsupported cipher suite used in private peer"))
            return false;
          }

          String contactID;
          String domain;

          if (!IPeer::splitURI(contact, domain, contactID)) {
            ZS_LOG_WARNING(Detail, log("splitting peer URI failed") + ", peer URI=" + contact)
            return false;
          }

          String calculatedSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase((const char *)((const BYTE *)(*mPassword))), "proof:" + contactID, IHelper::HashAlgorthm_SHA256));

          if (calculatedSecretProof != secretProof) {
            ZS_LOG_ERROR(Detail, log("private peer file password appears to be incorrect, secret proof calculated=") + calculatedSecretProof + ", expecting=" + secretProof)
            return false;
          }

          String decryptedContact = decryptString("contact", salt, encryptedContact);
          SecureByteBlockPtr decryptedPrivateKey = decryptBuffer("privatekey", salt, encryptedPrivateKey);
          String decryptedPeer = decryptString("peer", salt, encryptedPeer);
          String decryptedData = decryptString("data", salt, encryptedPrivateData);

          if (decryptedContact != contact) {
            ZS_LOG_ERROR(Detail, log("contact does not match encrypted contact, expecting contact=") + contact)
            return false;
          }

          mPeerURI = contact;
          mPrivateKey = IRSAPrivateKey::load(*decryptedPrivateKey);
          if (!mPrivateKey) {
            ZS_LOG_ERROR(Detail, log("failed to load private key, peer URI=") + mPeerURI)
            return false;
          }

          outPublicPeerDocument = Document::createFromParsedJSON(decryptedPeer);
          if (!outPublicPeerDocument) {
            ZS_LOG_ERROR(Detail, log("failed to create public peer document from private peer file, peer URI=") + mPeerURI)
            return false;
          }

          mPrivateDataDoc = Document::createFromAutoDetect(decryptedData);

        } catch(CheckFailed &) {
          ZS_LOG_ERROR(Detail, log("failed to parse private peer file"))
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool PeerFilePrivate::verifySignatures(PeerFilePublicPtr peerFilePublic)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFilePublic)

        ElementPtr sectionAEl = findSection("A");
        ElementPtr sectionBEl = findSection("B");

        if ((!sectionAEl) || (!sectionBEl)) {
          ZS_LOG_ERROR(Detail, log("signature verification failed - could not find either section 'A' or section 'B' in private peer file"))
          return false;
        }

        if (mPeerURI != peerFilePublic->forPeerFilePrivate().getPeerURI()) {
          ZS_LOG_ERROR(Detail, log("public/private peer file URIs do not match, public=") + peerFilePublic->forPeerFilePrivate().getPeerURI() + ", private=" + mPeerURI)
          return false;
        }

        if (!peerFilePublic->forPeerFilePrivate().verifySignature(sectionAEl)) {
          ZS_LOG_ERROR(Detail, log("private peer file section A was not signed properly"))
          return false;
        }
        if (!peerFilePublic->forPeerFilePrivate().verifySignature(sectionBEl)) {
          ZS_LOG_ERROR(Detail, log("private peer file section B was not signed properly"))
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePrivate::findSection(const char *sectionID) const
      {
        if (!mDocument) return ElementPtr();

        ElementPtr peerRootEl = mDocument->getFirstChildElement();
        if (!peerRootEl) return ElementPtr();

        if ("privatePeer" != peerRootEl->getValue()) return ElementPtr();

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

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    const char *IPeerFilePrivate::toString(SignatureTypes signatureType)
    {
      switch (signatureType) {
        case SignatureType_FullPublicKey:   return "Full public key";
        case SignatureType_PeerURI:         return "Peer URI";
        case SignatureType_Fingerprint:     return "Fingerprint";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    String IPeerFilePrivate::toDebugString(IPeerFilePrivatePtr peerFilePrivate, bool includeCommaPrefix)
    {
      if (!peerFilePrivate) return includeCommaPrefix ? String(", peer file private=(null)") : String("peer file private=(null");
      return IPeerFiles::toDebugString(peerFilePrivate->getAssociatedPeerFiles(), includeCommaPrefix);
    }
  }
}
