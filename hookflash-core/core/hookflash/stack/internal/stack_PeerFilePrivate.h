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

#pragma once

#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/internal/hookflashTypes.h>

#include <cryptopp/rsa.h>
#include <cryptopp/secblock.h>

namespace hookflash
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
      #pragma mark PeerFilePrivate
      #pragma mark

      class PeerFilePrivate : public IPeerFilePrivate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::BYTE BYTE;
        typedef zsLib::UINT UINT;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::XML::DocumentPtr DocumentPtr;
        typedef CryptoPP::RSA::PrivateKey CryptoPP_PrivateKey;
        typedef CryptoPP::SecByteBlock SecureByteBlock;

        class RSAPrivateKey;
        typedef boost::shared_ptr<RSAPrivateKey> RSAPrivateKeyPtr;
        typedef boost::weak_ptr<RSAPrivateKey> RSAPrivateKeyWeakPtr;

        friend class PeerFiles;

      protected:
        PeerFilePrivate(PeerFilesPtr peerFiles);

      public:
        typedef IPeerFilePrivate::URIList URIList;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePrivate => IPeerFilePrivate
        #pragma mark

        virtual ElementPtr saveToXML() const;

        virtual IPeerFilesPtr getPeerFiles() const;

        virtual UINT getVersionNumber() const;
        virtual bool containsSection(const char *sectionID) const;

        virtual bool verifyPassword(const char *password) const;

        virtual void getPrivateKeyInPCKS8(
                                          const char *password,
                                          SecureByteBlock &outRaw
                                          ) const;

        virtual String getContactProfileSecret(const char *password) const;
        virtual ElementPtr getCaptcha(const char *password) const;

        virtual void signElement(ElementPtr elementToSign);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePrivate => friend PeerFiles
        #pragma mark

        static PeerFilesPtr generate(
                                     PeerFilesPtr peerFiles,
                                     const char *password,
                                     ElementPtr signedSalt
                                     );

        static PeerFilesPtr loadFromXML(
                                        PeerFilesPtr peerFiles,
                                        const char *password,
                                        ElementPtr peerFileRootElement
                                        );

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePrivate => (internal)
        #pragma mark

        String log(const char *message) const;

        void generate(
                      const char *password,
                      ElementPtr signedSalt
                      );

        bool loadFromXML(
                         const char *password,
                         ElementPtr peerFileRootElement
                         );

        String getSaltAsBase64() const;
        ElementPtr findSection(const char *sectionID) const;

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePrivate::RSAPrivateKey
        #pragma mark

        class RSAPrivateKey
        {
        public:
          friend class PeerFilePrivate;

        protected:
          RSAPrivateKey();

        public:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PeerFilePrivate::RSAPrivateKey => friend PeerFilePrivate
          #pragma mark

          static RSAPrivateKeyPtr generate(SecureByteBlock &outPublicKeyBuffer);

          static RSAPrivateKeyPtr load(const SecureByteBlock &buffer);

          void save(SecureByteBlock &outBuffer) const;

          void sign(
                    const String &inStrDataToSign,
                    SecureByteBlock &outSignatureResult
                    ) const;

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PeerFilePrivate::RSAPrivateKey => (data)
          #pragma mark

          CryptoPP_PrivateKey mPrivateKey;
        };

      protected:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePrivate => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;

        PeerFilePrivateWeakPtr mThisWeak;
        PeerFilesWeakPtr mOuter;

        DocumentPtr mDocument;

        RSAPrivateKeyPtr mPrivateKey;
      };
    }
  }
}
