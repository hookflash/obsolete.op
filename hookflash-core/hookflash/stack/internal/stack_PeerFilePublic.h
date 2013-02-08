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

#include <hookflash/stack/IPeerFilePublic.h>
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
      #pragma mark PeerFilePublic
      #pragma mark

      class PeerFilePublic : public IPeerFilePublic
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::UINT UINT;
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::XML::ElementPtr ElementPtr;
        typedef zsLib::XML::DocumentPtr DocumentPtr;
        typedef CryptoPP::RSA::PublicKey CryptoPP_PublicKey;
        typedef CryptoPP::SecByteBlock SecureByteBlock;

        class RSAPublicKey;
        typedef boost::shared_ptr<RSAPublicKey> RSAPublicKeyPtr;
        typedef boost::weak_ptr<RSAPublicKey> RSAPublicKeyWeakPtr;

        friend class PeerFilePrivate;

      protected:
        PeerFilePublic();

        void init();

      public:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => IPeerFilePublic
        #pragma mark

        static PeerFilePublicPtr loadFromXML(ElementPtr publicPeerRootElement);

        virtual void updateFrom(IPeerFilePublicPtr anotherVersion);

        virtual ElementPtr saveToXML() const;

        virtual IPeerFilesPtr getPeerFiles() const;

        virtual bool isReadOnly() const;

        virtual UINT getVersionNumber() const;
        virtual bool containsSection(const char *sectionID) const;

        virtual String getContactID() const;
        virtual String calculateContactID() const;
        virtual String getFindSecret() const;

        virtual void getURIs(
                             const char *sectionID,
                             URIList &outURIs
                             ) const;

        virtual void getX509Certificate(SecureByteBlock &outRaw) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => friend PeerFilePrivate
        #pragma mark

        static PeerFilePublicPtr createFromPreGenerated(
                                                        PeerFilesPtr peerFiles,
                                                        DocumentPtr document,
                                                        RSAPublicKeyPtr publicKey
                                                        );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => (internal)
        #pragma mark

        static PeerFilePublicPtr convert(IPeerFilePublicPtr peerFile);

        String log(const char *message) const;

        ElementPtr findSection(const char *sectionID) const;

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic::RSAPublicKey
        #pragma mark

        class RSAPublicKey
        {
        public:
          friend class PeerFilePublic;
          friend class PeerFilePrivate;

        protected:
          RSAPublicKey();

        public:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PeerFilePrivate::PeerFilePublic => friend PeerFilePublic
          #pragma mark

          static RSAPublicKeyPtr load(const SecureByteBlock &buffer);

          void save(SecureByteBlock &outBuffer) const;

          bool verify(
                      const String &inOriginalStrDataSigned,
                      SecureByteBlock &inSignature
                      ) const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PeerFilePrivate::PeerFilePublic => friend PeerFilePrivate
          #pragma mark

          // (duplicate) static RSAPublicKeyPtr load(const SecureByteBlock &buffer);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark PeerFilePrivate::RSAPrivateKey => (data)
          #pragma mark

          CryptoPP_PublicKey mPublicKey;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;

        PeerFilePublicPtr mThisWeak;
        PeerFilesWeakPtr mOuter;

        DocumentPtr mDocument;

        RSAPublicKeyPtr mPublicKey;
      };
    }
  }
}
