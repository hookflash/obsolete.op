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

#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/internal/stack_PeerContactProfile.h>
#include <hookflash/stack/internal/stack_PeerFiles.h>
#include <hookflash/stack/internal/stack_PeerFilePrivate.h>
#include <hookflash/stack/internal/stack_PeerFilePublic.h>
#include <hookflash/services/IHelper.h>
#include <zsLib/XML.h>
#include <zsLib/Numeric.h>
#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>

#include <cryptopp/sha.h>
#include <cryptopp/osrng.h>
#include <cryptopp/secblock.h>

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
      typedef zsLib::ULONG ULONG;
      typedef zsLib::CSTR CSTR;
      typedef zsLib::String String;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::Text Text;
      typedef zsLib::XML::TextPtr TextPtr;
      typedef zsLib::XML::Document Document;
      typedef zsLib::XML::Element Element;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef CryptoPP::AutoSeededRandomPool AutoSeededRandomPool;
      typedef CryptoPP::SecByteBlock SecureByteBlock;
      typedef CryptoPP::SHA256 SHA256;

      //-----------------------------------------------------------------------
      String convertToBase64(
                             const BYTE *buffer,
                             ULONG bufferLengthInBytes
                             );

      //-----------------------------------------------------------------------
      void convertFromBase64(
                             const String &input,
                             SecureByteBlock &output
                             );

      //-----------------------------------------------------------------------
      void decryptAndNulTerminateFromBase64(
                                            const char *prefix,
                                            const char *password,
                                            const String &saltAsBase64,
                                            const String &input,
                                            SecureByteBlock &output
                                            );

      //-----------------------------------------------------------------------
      String encryptToBase64(
                                    const char *prefix,
                                    const char *password,
                                    const String &saltAsBase64,
                                    const BYTE *buffer,
                                    ULONG length
                                    );

      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      PeerContactProfile::PeerContactProfile() :
        mID(zsLib::createPUID())
      {
      }

      //---------------------------------------------------------------------
      PeerContactProfilePtr PeerContactProfile::convert(IPeerContactProfilePtr profile)
      {
        return boost::dynamic_pointer_cast<PeerContactProfile>(profile);
      }

      //---------------------------------------------------------------------
      PeerContactProfilePtr PeerContactProfile::createFromPreGenerated(
                                                                       PeerFilesPtr peerFiles,
                                                                       DocumentPtr document
                                                                       )
      {
        PeerContactProfilePtr pThis(new PeerContactProfile);
        pThis->mThisWeak = pThis;
        pThis->mPeerFiles = peerFiles;
        pThis->mDocument = document;
        pThis->internalTemporaryGetContactProfileSecret();
        return pThis;
      }

      //---------------------------------------------------------------------
      PeerContactProfilePtr PeerContactProfile::createFromXML(
                                                              ElementPtr contactProfileBundleOrContactProfileElement,
                                                              const char *contactProfileSecret,
                                                              PeerFilesPtr peerfiles
                                                              )
      {
        if (!contactProfileBundleOrContactProfileElement) return PeerContactProfilePtr();

        PeerContactProfilePtr pThis(new PeerContactProfile);
        pThis->mThisWeak = pThis;
        pThis->mPeerFiles = peerfiles;
        pThis->mDocument = Document::create();
        pThis->mDocument->adoptAsLastChild(contactProfileBundleOrContactProfileElement->clone());
        pThis->internalTemporaryGetContactProfileSecret();
        if (contactProfileSecret) {
          if (!pThis->usesContactProfileSecret(contactProfileSecret)) {
            return PeerContactProfilePtr();
          }
        }
        return pThis;
      }

      //---------------------------------------------------------------------
      PeerContactProfilePtr PeerContactProfile::createContactProfileFromExistingContactProfile(
                                                                                               IPeerContactProfilePtr contactProfile,
                                                                                               const char *contactProfileSecret,
                                                                                               PeerFilesPtr peerfiles
                                                                                               )
      {
        if (!contactProfile) return PeerContactProfilePtr();

        PeerContactProfilePtr pThis(new PeerContactProfile);
        pThis->mThisWeak = pThis;
        pThis->mPeerFiles = peerfiles;
        pThis->mDocument = Document::create();
        pThis->updateFrom(contactProfile);
        pThis->internalTemporaryGetContactProfileSecret();
        if (contactProfileSecret) {
          if (!pThis->usesContactProfileSecret(contactProfileSecret)) {
            return PeerContactProfilePtr();
          }
        }
        return pThis;
      }

      //---------------------------------------------------------------------
      PeerContactProfilePtr PeerContactProfile::createExternalFromPrivateProfile(ElementPtr privateProfileElement)
      {
        if (!privateProfileElement) return PeerContactProfilePtr();

        privateProfileElement = (privateProfileElement->clone())->toElementChecked();

        static const char *contactProfileSkeleton =
        "<contactProfile version=\"0\">\n"
        " <private>\n"
        "  <salt />\n"
        "  <proof cipher=\"sha256/aes256\" />\n"
        "  <encryptedProfile cipher=\"sha256/aes256\" />\n"
        "  <contactProfileSecret cipher=\"sha256/aes256\" />\n"
        " </private>\n"
        "</contactProfile>";

        DocumentPtr contactProfileDoc = Document::create();
        contactProfileDoc->parse(contactProfileSkeleton);

        String contactID = services::IHelper::randomString(32);
        String contactProfileSecret = services::IHelper::randomString(32);

        // generate external profile
        {
          std::string contactSalt;

          // generate salt
          {
            AutoSeededRandomPool rng;

            SecureByteBlock contactSaltRaw(32);
            rng.GenerateBlock(contactSaltRaw, contactSaltRaw.size());
            contactSalt = convertToBase64(contactSaltRaw, contactSaltRaw.size());
          }

          ElementPtr contactProfileElement = contactProfileDoc->getFirstChildElementChecked();
          contactProfileElement->setAttribute("id", contactID);

          ElementPtr privateElement = contactProfileElement->getFirstChildElementChecked();
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

          DocumentPtr privateProfileDocument = Document::create();
          privateProfileDocument->adoptAsLastChild(privateProfileElement);

          ULONG length = 0;
          boost::shared_array<char> output;
          output = privateProfileDocument->write(&length);

          ElementPtr encryptedProfileElement = proofElement->getNextSiblingElementChecked();
          String encryptedProfileString = encryptToBase64("profile", contactProfileSecret, contactSalt, (const BYTE *)(output.get()), length);
          TextPtr encryptProfileText = Text::create();
          encryptProfileText->setValue(encryptedProfileString);
          encryptedProfileElement->adoptAsLastChild(encryptProfileText);

          ElementPtr contactProfileSecretElement = encryptedProfileElement->getNextSiblingElementChecked();
          TextPtr contactProfileSecretText = Text::create();
          contactProfileSecretText->setValue(contactProfileSecret);
          contactProfileSecretElement->adoptAsLastChild(contactProfileSecretText);
        }

        PeerContactProfilePtr pThis(new PeerContactProfile);
        pThis->mThisWeak = pThis;
        pThis->mDocument = contactProfileDoc;
        pThis->mContactProfileSecret = contactProfileSecret;
        return pThis;
      }

      //---------------------------------------------------------------------
      bool PeerContactProfile::isReadOnly() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument->findFirstChildElement("contactProfileBundle")) {
          if (mDocument->findFirstChildElement("contactProfile")) return false;
        }
        return !(mPeerFiles.lock());
      }

      //---------------------------------------------------------------------
      bool PeerContactProfile::isExternal() const
      {
        AutoRecursiveLock lock(mLock);
        if (mDocument->findFirstChildElement("contactProfileBundle")) return false;
        if (mDocument->findFirstChildElement("contactProfile")) return true;
        return false;
      }

      //---------------------------------------------------------------------
      UINT PeerContactProfile::getVersionNumber() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return 0;

        ElementPtr contactProfileElement = getContactProfileElement();
        if (!contactProfileElement) return 0;
        String version = contactProfileElement->getAttributeValue("version");

        try {
          return (Numeric<UINT>(version));
        } catch(Numeric<UINT>::ValueOutOfRange &) {
        }
        return 0;
      }

      bool PeerContactProfile::setVersionNumber(UINT versionNumber)
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return false;

        PeerFilesPtr outer;
        IPeerFilePrivatePtr privatePeer;
        if (!isExternal()) {
          outer = mPeerFiles.lock();
          if (!outer) return false;

          privatePeer = outer->getPrivate();
          if (!privatePeer) return false;
        }

        ElementPtr contactProfileElement = getContactProfileElement();
        if (!contactProfileElement) return false;

        contactProfileElement->setAttribute("version", Stringize<UINT>(versionNumber).string());

        if (!isExternal()) {
          // now this this must be signed
          privatePeer->signElement(contactProfileElement);
        }

        return true;
      }

      //---------------------------------------------------------------------
      String PeerContactProfile::getContactID() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return String();

        ElementPtr contactProfileElement = getContactProfileElement();
        if (!contactProfileElement) return String();
        return contactProfileElement->getAttributeValue("id");
      }

      //---------------------------------------------------------------------
      String PeerContactProfile::getContactProfileSecret() const
      {
        AutoRecursiveLock lock(mLock);
        return mContactProfileSecret;
      }

      //---------------------------------------------------------------------
      bool PeerContactProfile::hasContactProfileSecret() const
      {
        AutoRecursiveLock lock(mLock);
        return !mContactProfileSecret.isEmpty();
      }

      //---------------------------------------------------------------------
      bool PeerContactProfile::usesContactProfileSecret(const char *contactProfileSecret)
      {
        AutoRecursiveLock lock(mLock);
        if (NULL == contactProfileSecret) return false;
        if (!mDocument) return false;

        try {
          ElementPtr contactProfileElement = getContactProfileElement();
          if (!contactProfileElement) return false;
          ElementPtr proofElement = contactProfileElement->findFirstChildElementChecked("private")->findFirstChildElementChecked("proof");
          String proofAsBase64 = proofElement->getText(true);
          SecureByteBlock proofHash;
          convertFromBase64(proofAsBase64, proofHash);

          SecureByteBlock calculatedProofHash(32);
          if (calculatedProofHash.size() != proofHash.size()) return false;

          SHA256 shaProof;
          shaProof.Update((const BYTE *)"proof:", strlen("proof:"));
          shaProof.Update((const BYTE *)contactProfileSecret, strlen(contactProfileSecret));
          shaProof.Final(calculatedProofHash);

          if (0 != memcmp(calculatedProofHash, proofHash, proofHash.size())) return false;

          // this is the secret and it is verified
          mContactProfileSecret = contactProfileSecret;

        } catch (zsLib::XML::Exceptions::CheckFailed &) {
          return false;
        }
        return true;
      }

      //---------------------------------------------------------------------
      void PeerContactProfile::updateFrom(IPeerContactProfilePtr inAnotherVersion)
      {
        AutoRecursiveLock lock(mLock);

        PeerContactProfilePtr anotherVersion = PeerContactProfile::convert(inAnotherVersion);
        if (!anotherVersion) return;

        AutoRecursiveLock lock2(anotherVersion->mLock);

        UINT versionThis = getVersionNumber();
        UINT versionOther = anotherVersion->getVersionNumber();

        String contactIDThis = getContactID();
        String contactIDOther = anotherVersion->getContactID();
        if (contactIDThis != contactIDOther) {
          if (0 != versionThis) return; // only on version 0 is the update allowed to a new contact profile ID
        }

        if (versionThis >= versionOther) return;
        mDocument = (anotherVersion->mDocument->clone())->toDocumentChecked();
        if (anotherVersion->hasContactProfileSecret())
          mContactProfileSecret = anotherVersion->mContactProfileSecret;
      }

      //---------------------------------------------------------------------
      IPeerFilePublicPtr PeerContactProfile::getPeerFilePublic() const
      {
        AutoRecursiveLock lock(mLock);
        PeerFilesPtr outer = mPeerFiles.lock();
        if (outer) {
          IPeerFilePublicPtr publicPeer = outer->getPublic();
          if (publicPeer) return publicPeer;
        }

        if (!mDocument) return IPeerFilePublicPtr();
        if (!hasContactProfileSecret()) return IPeerFilePublicPtr();

        try {
          ElementPtr contactProfileElement = getContactProfileElement();
          if (!contactProfileElement) return IPeerFilePublicPtr();
          ElementPtr privateElement = contactProfileElement->findFirstChildElementChecked("private");
          ElementPtr encryptedPeerElement = privateElement->findFirstChildElementChecked("encryptedPeer");
          ElementPtr saltElement = privateElement->findFirstChildElementChecked("salt");

          String saltAsBase64 = saltElement->getText(true);

          SecureByteBlock output;
          decryptAndNulTerminateFromBase64(
                                           "peer",
                                           mContactProfileSecret,
                                           saltAsBase64,
                                           encryptedPeerElement->getText(true),
                                           output
                                           );

          String parseData = (CSTR)((const BYTE *)output);
          DocumentPtr temp = Document::create();
          temp->parse(parseData);
          ElementPtr result = temp->findFirstChildElementChecked("peer");
          result->orphan();

          PeerFilePublicPtr publicPeer = PeerFilePublic::loadFromXML(result);
          return publicPeer;
        } catch (zsLib::XML::Exceptions::CheckFailed &) {
        }
        return IPeerFilePublicPtr();
      }

      //---------------------------------------------------------------------
      ElementPtr PeerContactProfile::getPublicProfile() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return ElementPtr();

        try {
          ElementPtr contactProfileElement = getContactProfileElement();
          if (!contactProfileElement) return ElementPtr();
          ElementPtr profileElement = contactProfileElement->findFirstChildElementChecked("public")->findFirstChildElementChecked("profile");
          return (profileElement->clone())->toElementChecked();
        } catch (zsLib::XML::Exceptions::CheckFailed &) {
        }
        return ElementPtr();
      }

      //---------------------------------------------------------------------
      ElementPtr PeerContactProfile::getPrivateProfile() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return ElementPtr();
        if (!hasContactProfileSecret()) return ElementPtr();

        try {
          ElementPtr contactProfileElement = getContactProfileElement();
          if (!contactProfileElement) return ElementPtr();
          ElementPtr privateElement = contactProfileElement->findFirstChildElementChecked("private");
          ElementPtr profileElement = privateElement->findFirstChildElementChecked("encryptedProfile");
          ElementPtr saltElement = privateElement->findFirstChildElementChecked("salt");

          String saltAsBase64 = saltElement->getText(true);

          SecureByteBlock output;
          decryptAndNulTerminateFromBase64(
                                           "profile",
                                           mContactProfileSecret,
                                           saltAsBase64,
                                           profileElement->getText(true),
                                           output
                                           );

          String parseData = (CSTR)((const BYTE *)output);
          DocumentPtr temp = Document::create();
          temp->parse(parseData);
          ElementPtr result = temp->findFirstChildElementChecked("profile");
          result->orphan();
          return result;
        } catch (zsLib::XML::Exceptions::CheckFailed &) {
        }
        return ElementPtr();
      }

      //---------------------------------------------------------------------
      bool PeerContactProfile::setPublicProfile(ElementPtr newProfileElement)
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return false;

        PeerFilesPtr outer = mPeerFiles.lock();
        if (!outer) return false;

        IPeerFilePrivatePtr privatePeer = outer->getPrivate();
        if (!privatePeer) return false;

        try {
          ElementPtr contactProfileElement = getContactProfileElement();
          if (!contactProfileElement) return false;
          ElementPtr publicElement = contactProfileElement->findFirstChildElement("public");
          ElementPtr profileElement;
          if (publicElement)
            profileElement = publicElement->findFirstChildElementChecked("profile");

          if (!newProfileElement) {
            // erasing the profile element to clean
            newProfileElement = Element::create();
            newProfileElement->setValue("profile");
          }

          if (profileElement) {
            profileElement->adoptAsNextSibling(newProfileElement);
            profileElement->orphan();
          } else {
            if (!publicElement) {
              publicElement = Element::create();
              publicElement->setValue("public");
              contactProfileElement->adoptAsFirstChild(publicElement);
            }
            publicElement->adoptAsLastChild(newProfileElement);
          }

          // upgrade the document version number
          UINT version = getVersionNumber();
          ++version;

          contactProfileElement->setAttribute("version", Stringize<UINT>(version).string());

          if (!isExternal()) {
            // now this this must be signed
            privatePeer->signElement(contactProfileElement);
          }

        } catch (zsLib::XML::Exceptions::CheckFailed &) {
          return false;
        }
        return true;
      }

      //---------------------------------------------------------------------
      bool PeerContactProfile::setPrivateProfile(ElementPtr newProfileElement)
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return false;

        PeerFilesPtr outer;
        IPeerFilePrivatePtr privatePeer;

        if (!isExternal()) {
          outer = mPeerFiles.lock();
          if (!outer) return false;

          privatePeer = outer->getPrivate();
          if (!privatePeer) return false;
        }

        try {
          ElementPtr contactProfileElement = getContactProfileElement();
          if (!contactProfileElement) return false;
          ElementPtr privateElement = contactProfileElement->findFirstChildElementChecked("private");
          ElementPtr encryptedProfileElement = privateElement->findFirstChildElement("encryptedProfile");
          ElementPtr saltElement = privateElement->findFirstChildElementChecked("salt");

          String saltAsBase64 = saltElement->getText(true);

          if (!newProfileElement) {
            // erasing the profile element to clean
            newProfileElement = Element::create();
            newProfileElement->setValue("profile");
          }

          DocumentPtr tempDoc = Document::create();
          tempDoc->adoptAsLastChild(newProfileElement->clone());

          // write out the new value
          boost::shared_array<char> output = tempDoc->write();

          // encrypt it
          String encryptedProfileAsBase64 = encryptToBase64(
                                                            "profile",
                                                            mContactProfileSecret.c_str(),
                                                            saltAsBase64,
                                                            (const BYTE *)(output.get()),
                                                            strlen(output.get())
                                                            );

          ElementPtr tempNewEncryptedProfileElement = Element::create();
          tempNewEncryptedProfileElement->setValue("encryptedProfile");

          TextPtr encryptedProfileText = Text::create();
          encryptedProfileText->setValue(encryptedProfileAsBase64);
          tempNewEncryptedProfileElement->adoptAsLastChild(encryptedProfileText);
          if (encryptedProfileElement) {
            encryptedProfileElement->adoptAsNextSibling(tempNewEncryptedProfileElement);
            encryptedProfileElement->orphan();
          } else {
            privateElement->adoptAsLastChild(tempNewEncryptedProfileElement);
          }

          // upgrade the document version number
          UINT version = getVersionNumber();
          ++version;

          contactProfileElement->setAttribute("version", Stringize<UINT>(version).string());

          if (!isExternal()) {
            // now this this must be signed
            privatePeer->signElement(contactProfileElement);
          }

        } catch (zsLib::XML::Exceptions::CheckFailed &) {
          return false;
        }
        return true;
      }

      //---------------------------------------------------------------------
      ElementPtr PeerContactProfile::saveToXML() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDocument) return ElementPtr();

        ElementPtr contactProfileBundleElement = mDocument->findFirstChildElement("contactProfileBundle");
        if (!contactProfileBundleElement) {
          ElementPtr contactProfileElement = mDocument->findFirstChildElement("contactProfile");
          if (!contactProfileElement) return ElementPtr();
          return contactProfileElement->clone()->toElementChecked();
        }

        return contactProfileBundleElement->clone()->toElementChecked();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      ElementPtr PeerContactProfile::getContactProfileElement() const
      {
        if (!mDocument) return ElementPtr();

        ElementPtr contactProfileBundleElement = mDocument->findFirstChildElement("contactProfileBundle");
        if (!contactProfileBundleElement) {
          ElementPtr contactProfileElement = mDocument->findFirstChildElement("contactProfile");
          if (contactProfileElement)
            return contactProfileElement;
          return ElementPtr();
        }
        return contactProfileBundleElement->findFirstChildElement("contactProfile");
      }

      //---------------------------------------------------------------------
      void PeerContactProfile::internalTemporaryGetContactProfileSecret()
      {
        if (!mDocument) return;

        try {
          ElementPtr contactProfileElement = getContactProfileElement();
          if (!contactProfileElement) return;

          ElementPtr privateElement = contactProfileElement->findFirstChildElementChecked("private");
          ElementPtr contactProfileSecretElement = privateElement->findFirstChildElementChecked("contactProfileSecret");

          String cotactProfileSecret = contactProfileSecretElement->getText();
          if (cotactProfileSecret.isEmpty()) return;
          mContactProfileSecret = cotactProfileSecret;
        } catch (zsLib::XML::Exceptions::CheckFailed &) {
        }
      }
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    IPeerContactProfilePtr IPeerContactProfile::createFromXML(
                                                              ElementPtr contactProfileElement,
                                                              const char *contactProfileSecret
                                                              )
    {
      return internal::PeerContactProfile::createFromXML(contactProfileElement, contactProfileSecret);
    }

    IPeerContactProfilePtr IPeerContactProfile::createExternalFromPrivateProfile(ElementPtr privateProfileElement)
    {
      return internal::PeerContactProfile::createExternalFromPrivateProfile(privateProfileElement);
    }
  }
}
