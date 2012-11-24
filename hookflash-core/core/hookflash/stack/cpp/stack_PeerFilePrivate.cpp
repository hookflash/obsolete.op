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

#include <hookflash/stack/internal/stack_PeerContactProfile.h>
#include <hookflash/stack/internal/stack_PeerFiles.h>
#include <hookflash/stack/internal/stack_PeerFilePublic.h>
#include <hookflash/stack/internal/stack_PeerFilePrivate.h>
#include <hookflash/services/ICanonicalXML.h>
#include <hookflash/services/IHelper.h>
#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/Numeric.h>
#include <zsLib/zsHelpers.h>

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


#define HOOKFLASH_GENERATE_REAL_RSA_KEYS


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Numeric;
      using zsLib::Stringize;
      using CryptoPP::CFB_Mode;

      typedef zsLib::BYTE BYTE;
      typedef zsLib::UINT UINT;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::CSTR CSTR;
      typedef zsLib::String String;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::Text Text;
      typedef zsLib::XML::TextPtr TextPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::Document Document;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef CryptoPP::ByteQueue ByteQueue;
      typedef CryptoPP::Base64Encoder Base64Encoder;
      typedef CryptoPP::Base64Decoder Base64Decoder;
      typedef CryptoPP::StringSink StringSink;
      typedef CryptoPP::Weak::MD5 MD5;
      typedef CryptoPP::SecByteBlock SecureByteBlock;
      typedef CryptoPP::AES AES;
      typedef CryptoPP::SHA256 SHA256;
      typedef CryptoPP::SHA1 SHA1;
      typedef CryptoPP::AutoSeededRandomPool AutoSeededRandomPool;
      typedef CryptoPP::HexEncoder HexEncoder;
      typedef CryptoPP::RSA::PrivateKey CryptoPP_PrivateKey;
      typedef CryptoPP::RSA::PublicKey CryptoPP_PublicKey;
      typedef CryptoPP::RSASSA_PKCS1v15_SHA_Signer CryptoPP_Signer;
      typedef PeerFilePrivate::RSAPrivateKeyPtr RSAPrivateKeyPtr;
      typedef PeerFilePublic::RSAPublicKey RSAPublicKey;
      typedef PeerFilePublic::RSAPublicKeyPtr RSAPublicKeyPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      String convertToBase64(
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
      void convertFromBase64(
                             const String &input,
                             SecureByteBlock &output
                             )
      {
        ByteQueue queue;
        queue.Put((BYTE *)input.c_str(), input.size());

        ByteQueue *outputQueue = new ByteQueue;
        Base64Decoder decoder(outputQueue);
        queue.CopyTo(decoder);
        decoder.MessageEnd();

        size_t outputLengthInBytes = (size_t)outputQueue->CurrentSize();
        output.CleanNew(outputLengthInBytes);

        outputQueue->Get(output, outputLengthInBytes);
      }

      //-----------------------------------------------------------------------
      static void getKeyInformation(
                                    const char *prefix,
                                    const char *password,
                                    const String &saltAsBase64,
                                    BYTE *aesIV,
                                    BYTE *aesKey
                                    )
      {
        if (!password) password = "";

        SecureByteBlock saltBinary;
        convertFromBase64(saltAsBase64, saltBinary);

        // need the salt as a hash (for use as the IV in the AES ecoder)
        MD5 saltMD5;
        saltMD5.Update(saltBinary, saltBinary.size());
        saltMD5.Final(aesIV);

        SecureByteBlock key(32);

        SHA256 keySHA256;

        keySHA256.Update((const BYTE *)prefix, strlen( prefix));
        keySHA256.Update((const BYTE *)":", strlen(":"));
        keySHA256.Update(saltBinary, saltBinary.size());
        keySHA256.Update((const BYTE *)":", strlen(":"));
        keySHA256.Update((const BYTE *)password, strlen(password));
        keySHA256.Final(key);

        memcpy(aesKey, key, 32);
      }

      //-----------------------------------------------------------------------
      String encryptToBase64(
                             const char *prefix,
                             const char *password,
                             const String &saltAsBase64,
                             const BYTE *buffer,
                             ULONG length
                             )
      {
        SecureByteBlock output(length);

        BYTE iv[AES::BLOCKSIZE];
        SecureByteBlock key(32);
        getKeyInformation(prefix, password, saltAsBase64, &(iv[0]), key);

        CFB_Mode<AES>::Encryption cfbEncryption(key, key.size(), iv);
        cfbEncryption.ProcessData(output, buffer, length);

        String result = convertToBase64(output, output.size());
        return result;
      }

      //-----------------------------------------------------------------------
      static void decryptFromBase64(
                                    const char *prefix,
                                    const char *password,
                                    const String &saltAsBase64,
                                    const String &input,
                                    SecureByteBlock &output
                                    )
      {
        output.New(0);
        if (input.isEmpty()) return;

        ByteQueue *outputQueue = new ByteQueue;
        Base64Decoder decoder(outputQueue);

        decoder.Put((const BYTE *)(input.c_str()), input.size());
        decoder.MessageEnd();
        size_t outputLengthInBytes = (size_t)outputQueue->CurrentSize();

        if (0 == outputLengthInBytes) return;

        SecureByteBlock inputRaw(outputLengthInBytes);
        output.CleanNew(outputLengthInBytes);

        outputQueue->Get(inputRaw, outputLengthInBytes);

        BYTE iv[AES::BLOCKSIZE];
        SecureByteBlock key(32);
        getKeyInformation(prefix, password, saltAsBase64, &(iv[0]), key);

        CFB_Mode<AES>::Decryption cfbDecryption(key, key.size(), iv);
        cfbDecryption.ProcessData(output, inputRaw, outputLengthInBytes);
      }

      //-----------------------------------------------------------------------
      void decryptAndNulTerminateFromBase64(
                                            const char *prefix,
                                            const char *password,
                                            const String &saltAsBase64,
                                            const String &input,
                                            SecureByteBlock &output
                                            )
      {
        output.CleanNew(1);
        if (input.isEmpty()) return;

        ByteQueue *outputQueue = new ByteQueue;
        Base64Decoder decoder(outputQueue);

        decoder.Put((const BYTE *)(input.c_str()), input.size());
        decoder.MessageEnd();
        size_t outputLengthInBytes = (size_t)outputQueue->CurrentSize();

        if (0 == outputLengthInBytes) return;

        SecureByteBlock inputRaw(outputLengthInBytes);
        output.CleanNew(outputLengthInBytes+1);

        outputQueue->Get(inputRaw, outputLengthInBytes);

        BYTE iv[AES::BLOCKSIZE];
        SecureByteBlock key(32);
        getKeyInformation(prefix, password, saltAsBase64, &(iv[0]), key);

        CFB_Mode<AES>::Decryption cfbDecryption(key, key.size(), iv);
        cfbDecryption.ProcessData(output, inputRaw, outputLengthInBytes);
      }

      //-----------------------------------------------------------------------
      static void actualSignElement(
                                    ElementPtr element,
                                    RSAPrivateKeyPtr privateKey
                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!privateKey)

        ElementPtr parent = element->getParentElement();
        ZS_THROW_INVALID_USAGE_IF(!parent)      // can only sign an element that is within an existing "bundle" element...

        String id = element->getAttributeValue("id");
        if (!id.isEmpty()) {
          // strip off any old signatures
          try {
            ElementPtr oldSignature = parent->findFirstChildElement("Signature");

            ElementPtr nextSignature;
            for (; oldSignature; oldSignature = nextSignature) {
              nextSignature = oldSignature->getNextSiblingElement();

              try {
                if ("Signature" != oldSignature->getValue()) continue;

                ElementPtr referenceElement = oldSignature->findFirstChildElementChecked("SignedInfo")->findFirstChildElementChecked("Reference");

                String existingID = referenceElement->getAttributeValue("id");
                if (id != existingID) {
                  ZS_LOG_WARNING(Debug, "Found signature but reference ids do not match, searching=" + id + ", found=" + existingID)
                  continue;
                }

                ZS_LOG_TRACE("Found existing signature object and stripping it, reference=" + existingID)

                // found it... strip it
                oldSignature->orphan();
                break;
              } catch(zsLib::XML::Exceptions::CheckFailed &) {
              }
            }
          } catch (zsLib::XML::Exceptions::CheckFailed &) {
          }
        } else {
          id = services::IHelper::randomString(20);
          element->setAttribute("id", id);
        }

        static const char *skeletonSignature =
        "<Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\">\n"
        " <SignedInfo>\n"
        "  <SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-sha1\" />\n"
        "  <Reference>\n"
        "   <DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\" />\n"
        "   <DigestValue></DigestValue>\n"
        "  </Reference>\n"
        " </SignedInfo>\n"
        " <SignatureValue></SignatureValue>\n"
        "</Signature>";

        DocumentPtr signDoc = Document::create();
        signDoc->parse(skeletonSignature);
        ElementPtr signatureEl = signDoc->getFirstChildElementChecked();

        signatureEl->orphan();

        ElementPtr signedInfoEl = signatureEl->getFirstChildElementChecked();

        ElementPtr reference = signedInfoEl->findFirstChildElementChecked("Reference");
        reference->setAttribute("URI", String("#") + id);

        String canonicalXML = hookflash::services::ICanonicalXML::convert(element);
        SecureByteBlock hashRaw(20);
        SHA1 sha1;
        sha1.Update((const BYTE *)(canonicalXML.c_str()), canonicalXML.size());
        sha1.Final(hashRaw);

        String hash = convertToBase64(hashRaw, hashRaw.size());

        ElementPtr digestValue = reference->findFirstChildElementChecked("DigestValue");
        TextPtr digestText = Text::create();
        digestText->setValue(hash);
        digestValue->adoptAsFirstChild(digestText);


        //.....................................................................
        // compute the signature on the canonical XML of the SignedInfo

        String canonicalSignedInfo = hookflash::services::ICanonicalXML::convert(signedInfoEl);

        SecureByteBlock signature(20);
        privateKey->sign(canonicalSignedInfo, signature);

        // singed hash value
        String signatureAsBase64 = convertToBase64(signature, signature.size());

        ElementPtr signatureValue = signatureEl->findFirstChildElementChecked("SignatureValue");
        TextPtr signatureText = Text::create();
        signatureText->setValue(signatureAsBase64);
        signatureValue->adoptAsFirstChild(signatureText);

        element->adoptAsNextSibling(signatureEl);
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
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate => IPeerFilePrivate
      #pragma mark

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePrivate::saveToXML() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return ElementPtr();

        ElementPtr peerRoot = mDocument->getFirstChildElement();
        if (!peerRoot) return ElementPtr();

        return (peerRoot->clone())->toElementChecked();
      }

      //-----------------------------------------------------------------------
      IPeerFilesPtr PeerFilePrivate::getPeerFiles() const
      {
        AutoRecursiveLock lock(mLock);
        return mOuter.lock();
      }

      //-----------------------------------------------------------------------
      UINT PeerFilePrivate::getVersionNumber() const
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
      bool PeerFilePrivate::containsSection(const char *sectionID) const
      {
        AutoRecursiveLock lock(mLock);
        return findSection(sectionID);
      }

      //-----------------------------------------------------------------------
      bool PeerFilePrivate::verifyPassword(const char *password) const
      {
        if (!password) password = "";

        String salt = getSaltAsBase64();
        if (salt.isEmpty()) return false;

        ElementPtr sectionBElement = findSection("A");
        ElementPtr secretProofElement = sectionBElement->findFirstChildElement("secretProof");
        String secretProofInBase64 = secretProofElement->getText();

        SecureByteBlock calculatedHashSecret(32);  // the caculated hash secret

        SecureByteBlock secretProofRaw;
        convertFromBase64(secretProofInBase64, secretProofRaw);
        if (calculatedHashSecret.size() != secretProofRaw.size()) return false;

        {
          SecureByteBlock rawSalt;

          convertFromBase64(salt, rawSalt);

          SecureByteBlock hashProof(32);
          SHA256 shaProof;
          shaProof.Update((const BYTE *)"proof:", strlen("proof:"));
          shaProof.Update((const BYTE *)password, strlen(password));
          shaProof.Final(hashProof);

          SHA256 shaSecret;
          shaSecret.Update((const BYTE *)"secret:", strlen("secret:"));
          shaSecret.Update(rawSalt, rawSalt.size());
          shaSecret.Update((const BYTE *)":", strlen(":"));
          shaSecret.Update(hashProof, hashProof.size());
          shaSecret.Final(calculatedHashSecret);
        }

        return (0 == memcmp(secretProofRaw, calculatedHashSecret, calculatedHashSecret.size()));
      }

      //-----------------------------------------------------------------------
      void PeerFilePrivate::getPrivateKeyInPCKS8(
                                                 const char *password,
                                                 SecureByteBlock &outRaw
                                                 ) const
      {
        outRaw.New(0);

        if (!verifyPassword(password)) return;

        ElementPtr sectionBElement = findSection("B");
        if (!sectionBElement) return;

        ElementPtr encryptedPrivateKeyElement = sectionBElement->findFirstChildElement("encryptedPrivateKey");
        if (!encryptedPrivateKeyElement) return;

        String encryptedPrivateKeyElementAsBase64 = encryptedPrivateKeyElement->getText();
        decryptFromBase64("privatekey", password, getSaltAsBase64(), encryptedPrivateKeyElementAsBase64, outRaw);
      }

      //-----------------------------------------------------------------------
      String PeerFilePrivate::getContactProfileSecret(const char *password) const
      {
        if (!verifyPassword(password)) return String();

        ElementPtr sectionBElement = findSection("B");
        if (!sectionBElement) return String();

        ElementPtr encryptedContactProfileSecretElement = sectionBElement->findFirstChildElement("encryptedContactProfileSecret");
        if (!encryptedContactProfileSecretElement) return String();

        SecureByteBlock outRaw;

        String encryptedContactProfileSecretElementAsBase64 = encryptedContactProfileSecretElement->getText();
        decryptAndNulTerminateFromBase64("profile", password, getSaltAsBase64(), encryptedContactProfileSecretElementAsBase64, outRaw);

        return (CSTR)((const BYTE *)outRaw);
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePrivate::getCaptcha(const char *password) const
      {
        return ElementPtr();
      }

      //-----------------------------------------------------------------------
      void PeerFilePrivate::signElement(ElementPtr elementToSign)
      {
        if (!mPrivateKey) {
          ZS_LOG_ERROR(Basic, log("unable to sign element as private key is missing"))
        }

        actualSignElement(elementToSign, mPrivateKey);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate => friend PeerFiles
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilesPtr PeerFilePrivate::generate(
                                             PeerFilesPtr peerFiles,
                                             const char *password,
                                             ElementPtr signedSalt
                                             )
      {
        PeerFilePrivatePtr pThis(new PeerFilePrivate(peerFiles));
        pThis->mThisWeak = pThis;
        pThis->mOuter = peerFiles;
        pThis->generate(password, signedSalt);
        peerFiles->mThisWeak = peerFiles;
        return peerFiles;
      }

      //-----------------------------------------------------------------------
      PeerFilesPtr PeerFilePrivate::loadFromXML(
                                                PeerFilesPtr peerFiles,
                                                const char *password,
                                                ElementPtr peerFileRootElement
                                                )
      {
        PeerFilePrivatePtr pThis(new PeerFilePrivate(peerFiles));
        pThis->mThisWeak = pThis;
        pThis->mOuter = peerFiles;
        bool loaded = pThis->loadFromXML(password, peerFileRootElement);
        if (!loaded)
          return PeerFilesPtr();
        peerFiles->mThisWeak = peerFiles;
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
      void PeerFilePrivate::generate(
                                     const char *password,
                                     ElementPtr signedSalt
                                     )
      {
        if (!password) password = "";

        AutoSeededRandomPool rng;

        String salt;
        String privateString;
        String publicString;
        String contactID;
        String contactProfileSecret = services::IHelper::randomString(32);

        std::string contactSalt;

        // generate salt
        {
          SecureByteBlock saltRaw(32);
          rng.GenerateBlock(saltRaw, saltRaw.size());
          salt = convertToBase64(saltRaw, saltRaw.size());

          SecureByteBlock contactSaltRaw(32);
          rng.GenerateBlock(contactSaltRaw, contactSaltRaw.size());
          contactSalt = convertToBase64(contactSaltRaw, contactSaltRaw.size());
        }

#ifdef HOOKFLASH_GENERATE_REAL_RSA_KEYS
        RSAPublicKeyPtr publicKey;
        SecureByteBlock publicKeyBuffer;
#endif //HOOKFLASH_GENERATE_REAL_RSA_KEYS

        {
#ifdef HOOKFLASH_GENERATE_REAL_RSA_KEYS
          SecureByteBlock byteBlock;

          mPrivateKey = RSAPrivateKey::generate(publicKeyBuffer);
          if (!mPrivateKey) {
            ZS_THROW_BAD_STATE(log("failed to generate a private/public key pair"))
          }

          mPrivateKey->save(byteBlock);
          publicKey = RSAPublicKey::load(publicKeyBuffer);
          if (!publicKey) {
            ZS_THROW_BAD_STATE(log("failed to load a public key from previously generated private key"))
          }
#else
          // generate fake private key and encrypt it immediately
          SecureByteBlock byteBlock(100);
          rng.GenerateBlock(byteBlock, byteBlock.size());
#endif //HOOKFLASH_GENERATE_REAL_RSA_KEYS

          privateString = encryptToBase64("privatekey", password, salt, byteBlock, byteBlock.size());
        }

        {
#ifdef HOOKFLASH_GENERATE_REAL_RSA_KEYS
          SecureByteBlock &byteBlock = publicKeyBuffer;
#else
          // generate fake public key
          SecureByteBlock byteBlock(100);
          rng.GenerateBlock(byteBlock, byteBlock.size());
#endif //HOOKFLASH_GENERATE_REAL_RSA_KEYS

          publicString = convertToBase64(byteBlock, byteBlock.size());
        }

        static const char *skeletonPublicPeer =
        "<peer version=\"1\">\n\n"

        "<sectionBundle xmlns=\"http://www.hookflash.com/openpeer/1.0/message\">\n"
        " <section id=\"A\">\n"
        "  <cipher>sha1/aes256</cipher>\n"
        "  <data></data>\n"
        " </section>\n"
        "</sectionBundle>\n\n"

        "<sectionBundle xmlns=\"http://www.hookflash.com/openpeer/1.0/message\">\n"
        " <section id=\"B\">\n"
        "  <contact />\n"
        "  <findSecret />\n"
        "  <uris>\n"
        "   <uri>peer://hookflash.com/contact:</uri>\n"
        "  </uris>\n"
        " </section>\n"
        "</sectionBundle>\n\n"

        "<sectionBundle xmlns=\"http://www.hookflash.com/openpeer/1.0/message\">\n"
        " <section id=\"C\">\n"
        "  <contact />\n"
        "  <uris>\n"
        "  </uris>\n"
        "  <identities>\n"
        "  </identities>\n"
        " </section>\n"
        "</sectionBundle>\n\n"

        "</peer>\n";

        static const char *skeletonKeyInfo =
        "<KeyInfo>\n"
        " <X509Data>\n"
        "  <X509Certificate></X509Certificate>\n"
        " </X509Data>\n"
        "</KeyInfo>\n";

        static const char *skeletonPrivatePeer =
        "<privatePeer version=\"1\">\n\n"
        "<sectionBundle xmlns=\"http://www.hookflash.com/openpeer/1.0/message\">\n"
        " <section id=\"A\">\n"
        "  <cipher>sha1/aes256</cipher>\n"
        "  <contact />\n"
        "  <salt />\n"
        "  <secretProof />\n"
        " </section>\n"
        "</sectionBundle>\n\n"

        "<sectionBundle xmlns=\"http://www.hookflash.com/openpeer/1.0/message\">\n"
        " <section id=\"B\">\n"
        "  <encryptedPrivateKey />\n"
        "  <encryptedPeer cipher=\"sha256/aes256\" />\n"
        "  <encryptedContactProfileSecret />\n"
        "  <encryptedPrivateData />\n"
        " </section>\n"
        "</sectionBundle>\n\n"

        "</privatePeer>";

        static const char *contactProfileSkeleton =
        "<contactProfileBundle xmlns=\"http://www.hookflash.com/openpeer/1.0/message\">\n"
        " <contactProfile version=\"1\">\n"
        "  <public>\n"
        "   <profile />\n"
        "  </public>\n"
        "  <private>\n"
        "   <salt />\n"
        "   <proof cipher=\"sha256/aes256\" />\n"
        "   <encryptedPeer cipher=\"sha256/aes256\" />\n"
        "   <encryptedProfile cipher=\"sha256/aes256\" />\n"
        "   <contactProfileSecret cipher=\"sha256/aes256\" />\n"
        "  </private>\n"
        " </contactProfile>\n"
        "</contactProfileBundle>\n";

        mDocument = Document::create();
        mDocument->parse(skeletonPrivatePeer);

        DocumentPtr publicDoc = Document::create();
        publicDoc->parse(skeletonPublicPeer);

        DocumentPtr contactProfileDoc = Document::create();
        contactProfileDoc->parse(contactProfileSkeleton);

        // build public section A
        {
          // doc->peer->sectionBundle->section A
          ElementPtr sectionABundle = publicDoc->getFirstChildElementChecked()->getFirstChildElementChecked();

          // sectionBundle->section A
          ElementPtr sectionA = sectionABundle->getFirstChildElementChecked();
          // sectionA->cipher
          ElementPtr cipher = sectionA->getFirstChildElementChecked();
          //            cipher->adoptAsNextSibling(signedSalt->clone());

          actualSignElement(sectionA, mPrivateKey);

          // going to put key info inside the signature
          ElementPtr signature = sectionA->getNextSiblingElementChecked();
          signature->getFirstChildElementChecked();

          DocumentPtr keyInfoDoc = Document::create();
          keyInfoDoc->parse(skeletonKeyInfo);
          ElementPtr keyInfo = keyInfoDoc->getFirstChildElementChecked();
          // KeyInfo->X509Data->X509Certificate
          ElementPtr x509Certificate = keyInfo->getFirstChildElementChecked()->getFirstChildElementChecked();
          TextPtr x509Text = Text::create();
          x509Text->setValue(publicString);
          x509Certificate->adoptAsLastChild(x509Text);

          keyInfo->orphan();
          signature->adoptAsLastChild(keyInfo);

          String canonicalSectionA = hookflash::services::ICanonicalXML::convert(sectionABundle);

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
        }

        //std::cout << (publicDoc->write()).get() << "\n";

        // build public section B
        {
          ElementPtr sectionBBundleElement = publicDoc->getFirstChildElementChecked()->getFirstChildElementChecked()->getNextSiblingElementChecked();
          ElementPtr contactElement = sectionBBundleElement->getFirstChildElementChecked()->getFirstChildElementChecked();
          contactElement->setAttribute("id", contactID);

          String findSecret = services::IHelper::randomString(32);

          ElementPtr findSecretElement = contactElement->getNextSiblingElementChecked();
          TextPtr findSecretText = Text::create();
          findSecretText->setValue(findSecret);
          findSecretElement->adoptAsLastChild(findSecretText);

          TextPtr uriText = findSecretElement->getNextSiblingElementChecked()->getFirstChildElementChecked()->getFirstChildChecked()->toTextChecked();
          uriText->setValue(uriText->getValue() + contactID);

          actualSignElement(sectionBBundleElement->getFirstChildElementChecked(), mPrivateKey);
        }

        //std::cout << (publicDoc->write()).get() << "\n";

        // build public section C
        {
          ElementPtr sectionBBundleElement = publicDoc->getFirstChildElementChecked()->getFirstChildElementChecked()->getNextSiblingElementChecked()->getNextSiblingElementChecked();
          ElementPtr contactElement = sectionBBundleElement->getFirstChildElementChecked()->getFirstChildElementChecked();
          contactElement->setAttribute("id", contactID);

          actualSignElement(sectionBBundleElement->getFirstChildElementChecked(), mPrivateKey);
        }

        //std::cout << (publicDoc->write()).get() << "\n";

        // build private section A
        {
          ElementPtr sectionAElement = mDocument->getFirstChildElementChecked()->getFirstChildElementChecked()->getFirstChildElementChecked();
          ElementPtr contactElement = sectionAElement->getFirstChildElementChecked()->getNextSiblingElementChecked();
          contactElement->setAttribute("id", contactID);

          ElementPtr saltElement = contactElement->getNextSiblingElementChecked();
          TextPtr saltText = Text::create();
          saltText->setValue(salt);
          saltElement->adoptAsLastChild(saltText);

          ElementPtr secretProofElement = saltElement->getNextSiblingElementChecked();
          TextPtr secretProofText = Text::create();

          // calculate the secret proof
          {
            SecureByteBlock rawSalt;

            convertFromBase64(salt, rawSalt);

            SecureByteBlock hashProof(32);
            SHA256 sha256Proof;
            sha256Proof.Update((const BYTE *)"proof:", strlen("proof:"));
            sha256Proof.Update((const BYTE *)password, strlen(password));
            sha256Proof.Final(hashProof);

            SecureByteBlock hashSecret(32);
            SHA256 sha256Secret;
            sha256Secret.Update((const BYTE *)"secret:", strlen("secret:"));
            sha256Secret.Update(rawSalt, rawSalt.size());
            sha256Secret.Update((const BYTE *)":", strlen(":"));
            sha256Secret.Update(hashProof, hashProof.size());
            sha256Secret.Final(hashSecret);
            String secret = convertToBase64(hashSecret, hashSecret.size());
            secretProofText->setValue(secret);
            secretProofElement->adoptAsLastChild(secretProofText);
          }

          ElementPtr sectionBElement = mDocument->getFirstChildElementChecked()->getFirstChildElementChecked()->getNextSiblingElementChecked()->getFirstChildElementChecked();
          ElementPtr encryptedPrivateKeyElement = sectionBElement->getFirstChildElementChecked();
          TextPtr encryptedPrivateKeyText = Text::create();
          encryptedPrivateKeyText->setValue(privateString);
          encryptedPrivateKeyElement->adoptAsLastChild(encryptedPrivateKeyText);

          ElementPtr encryptedPeerElement = encryptedPrivateKeyElement->getNextSiblingElementChecked();
          boost::shared_array<char> publicPeerAsString;
          publicPeerAsString = publicDoc->write();
          String encryptedPeerString = encryptToBase64("peer", password, salt, (const BYTE *)publicPeerAsString.get(), strlen(publicPeerAsString.get()));
          TextPtr encryptPeerText = Text::create();
          encryptPeerText->setValue(encryptedPeerString);
          encryptedPeerElement->adoptAsLastChild(encryptPeerText);

          ElementPtr encryptedContactProfileSecretElement = encryptedPeerElement->getNextSiblingElementChecked();
          String encryptedContactProfileSecretString = encryptToBase64("profile", password, salt, (const BYTE *)contactProfileSecret.c_str(), contactProfileSecret.size());
          TextPtr encryptContactProfileSecretText = Text::create();
          encryptContactProfileSecretText->setValue(encryptedContactProfileSecretString);
          encryptedContactProfileSecretElement->adoptAsLastChild(encryptContactProfileSecretText);

          actualSignElement(sectionAElement, mPrivateKey);
          actualSignElement(sectionBElement, mPrivateKey);
        }

        //std::cout << (mDocument->write()).get() << "\n";

        // build contact profile
        {
          ElementPtr contactProfileElement = contactProfileDoc->getFirstChildElementChecked()->getFirstChildElementChecked();
          contactProfileElement->setAttribute("id", contactID);
          ElementPtr publicElement = contactProfileElement->getFirstChildElementChecked();
          publicElement->adoptAsLastChild(publicDoc->getFirstChildElementChecked()->clone());

          ElementPtr privateElement = publicElement->getNextSiblingElementChecked();
          ElementPtr saltElement = privateElement->getFirstChildElementChecked();
          TextPtr saltText = Text::create();
          saltText->setValue(contactSalt);
          saltElement->adoptAsLastChild(saltText);

          SecureByteBlock contactProofHash(32);

          SHA256 contactProof;
          contactProof.Update((const BYTE *)"proof:", strlen("proof:"));
          contactProof.Update((const BYTE *)contactProfileSecret.c_str(), contactProfileSecret.size());
          contactProof.Final(contactProofHash);

          String contactProofInBase64 = convertToBase64(contactProofHash, contactProofHash.size());

          ElementPtr proofElement = saltElement->getNextSiblingElementChecked();
          TextPtr proofText = Text::create();
          proofText->setValue(contactProofInBase64);
          proofElement->adoptAsLastChild(proofText);

          ElementPtr encryptedPeerElement = proofElement->getNextSiblingElementChecked();
          boost::shared_array<char> publicPeerAsString;
          publicPeerAsString = publicDoc->write();
          String encryptedPeerString = encryptToBase64("peer", contactProfileSecret, contactSalt, (const BYTE *)publicPeerAsString.get(), strlen(publicPeerAsString.get()));
          TextPtr encryptPeerText = Text::create();
          encryptPeerText->setValue(encryptedPeerString);
          encryptedPeerElement->adoptAsLastChild(encryptPeerText);

          const char *emptyProfile = "<profile />";
          ElementPtr encryptedProfileElement = encryptedPeerElement->getNextSiblingElementChecked();
          String encryptedProfileString = encryptToBase64("profile", contactProfileSecret, contactSalt, (const BYTE *)emptyProfile, strlen(emptyProfile));
          TextPtr encryptProfileText = Text::create();
          encryptProfileText->setValue(encryptedProfileString);
          encryptedProfileElement->adoptAsLastChild(encryptProfileText);

          ElementPtr contactProfileSecretElement = encryptedProfileElement->getNextSiblingElementChecked();
          TextPtr contactProfileSecretText = Text::create();
          contactProfileSecretText->setValue(contactProfileSecret);
          contactProfileSecretElement->adoptAsLastChild(contactProfileSecretText);

          actualSignElement(contactProfileElement, mPrivateKey);
        }

        //std::cout << (contactProfileDoc->write()).get() << "\n";

        (mOuter.lock())->mPrivate = mThisWeak.lock();
        (mOuter.lock())->mPublic = PeerFilePublic::createFromPreGenerated(mOuter.lock(), publicDoc, publicKey);
        (mOuter.lock())->mContactProfile = PeerContactProfile::createFromPreGenerated(mOuter.lock(), contactProfileDoc);
      }

      //-----------------------------------------------------------------------
      bool PeerFilePrivate::loadFromXML(
                                        const char *password,
                                        ElementPtr peerFileRootElement
                                        )
      {
        if (!peerFileRootElement) return false;
        if (NULL == password) return false;

        mDocument = Document::create();
        mDocument->adoptAsLastChild(peerFileRootElement->clone());

        if (!verifyPassword(password)) {
          ZS_LOG_ERROR(Basic, log("Password does not verify properly for private peer file"))
          return false;
        }

        ElementPtr sectionBElement = findSection("B");
        if (!sectionBElement) return false;

        ElementPtr encryptedPeerElement = sectionBElement->findFirstChildElement("encryptedPeer");
        if (!encryptedPeerElement) return false;

        SecureByteBlock outRaw;

        String encryptedPeerAsBase64 = encryptedPeerElement->getText();
        decryptAndNulTerminateFromBase64("peer", password, getSaltAsBase64(), encryptedPeerAsBase64, outRaw);

        String decodedXML = (CSTR)((const BYTE *)outRaw);
        if (decodedXML.isEmpty()) return false;

        DocumentPtr publicDoc = Document::create();
        publicDoc->parse(decodedXML);

        PeerFilePublicPtr publicFile = PeerFilePublic::createFromPreGenerated(mOuter.lock(), publicDoc, RSAPublicKeyPtr());
        if (!publicFile) return false;

        (mOuter.lock())->mPrivate = mThisWeak.lock();
        (mOuter.lock())->mPublic = publicFile;
        if (!publicFile->containsSection("A")) return false;

        SecureByteBlock privateKeyBuffer;
        getPrivateKeyInPCKS8(password, privateKeyBuffer);
        if (privateKeyBuffer.SizeInBytes() < 1) {
          ZS_LOG_ERROR(Basic, log("failed to load private key from XML"))
          return false;
        }

        mPrivateKey = RSAPrivateKey::load(privateKeyBuffer);
        if (!mPrivateKey) {
          ZS_LOG_ERROR(Basic, log("loading of private key failed to validate"))
          return false;
        }

        return true;
      }

      //-----------------------------------------------------------------------
      String PeerFilePrivate::getSaltAsBase64() const
      {
        ElementPtr sectionAElement = findSection("A");
        if (!sectionAElement) return String();

        ElementPtr saltElement = sectionAElement->findFirstChildElement("salt");
        if (!saltElement) return String();

        return saltElement->getText();
      }

      //-----------------------------------------------------------------------
      ElementPtr PeerFilePrivate::findSection(const char *sectionID) const
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
      #pragma mark PeerFilePrivate::RSAPrivateKey
      #pragma mark

      //-----------------------------------------------------------------------
      PeerFilePrivate::RSAPrivateKey::RSAPrivateKey()
      {
      }

      //-------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePrivate::RSAPrivateKey => friend PeerFilePrivate
      #pragma mark

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr PeerFilePrivate::RSAPrivateKey::generate(SecureByteBlock &outPublicKeyBuffer)
      {
        AutoSeededRandomPool rng;

        RSAPrivateKeyPtr pThis(new RSAPrivateKey);

        pThis->mPrivateKey.GenerateRandomWithKeySize(rng, 2048);
        if (!pThis->mPrivateKey.Validate(rng, 3)) {
          ZS_LOG_ERROR(Basic, "failed to generate a new private key")
          return RSAPrivateKeyPtr();
        }

        CryptoPP_PublicKey rsaPublic(pThis->mPrivateKey);
        if (!rsaPublic.Validate(rng, 3)) {
          ZS_LOG_ERROR(Basic, "Failed to generate a public key for the new private key")
          return RSAPrivateKeyPtr();
        }

        ByteQueue byteQueue;
        rsaPublic.Save(byteQueue);

        size_t outputLengthInBytes = (size_t)byteQueue.CurrentSize();
        outPublicKeyBuffer.CleanNew(outputLengthInBytes);

        byteQueue.Get(outPublicKeyBuffer, outputLengthInBytes);

        return pThis;
      }

      //-----------------------------------------------------------------------
      RSAPrivateKeyPtr PeerFilePrivate::RSAPrivateKey::load(const SecureByteBlock &buffer)
      {
        AutoSeededRandomPool rng;

        ByteQueue byteQueue;
        byteQueue.LazyPut(buffer.BytePtr(), buffer.SizeInBytes());
        byteQueue.FinalizeLazyPut();

        RSAPrivateKeyPtr pThis(new RSAPrivateKey());

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
      void PeerFilePrivate::RSAPrivateKey::save(SecureByteBlock &outBuffer) const
      {
        ByteQueue byteQueue;
        mPrivateKey.Save(byteQueue);

        size_t outputLengthInBytes = (size_t)byteQueue.CurrentSize();
        outBuffer.CleanNew(outputLengthInBytes);

        byteQueue.Get(outBuffer, outputLengthInBytes);
      }

      //-----------------------------------------------------------------------
      void PeerFilePrivate::RSAPrivateKey::sign(
                                                const String &inStrDataToSign,
                                                SecureByteBlock &outSignatureResult
                                                ) const
      {
        AutoSeededRandomPool rng;

        CryptoPP_Signer signer(mPrivateKey);

        size_t length = signer.MaxSignatureLength();

        outSignatureResult.CleanNew(length);

        signer.SignMessage(rng, (const BYTE *)(inStrDataToSign.c_str()), inStrDataToSign.length(), outSignatureResult);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
