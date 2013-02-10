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

#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/internal/types.h>


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
      #pragma mark IPeerFilePublicForPeerFiles
      #pragma mark

      interaction IPeerFilePublicForPeerFiles
      {
        IPeerFilePublicForPeerFiles &forPeerFiles() {return *this;}
        const IPeerFilePublicForPeerFiles &forPeerFiles() const {return *this;}

        virtual String getDebugValueString(bool includeCommaPrefix = true) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerFilePublicForPeerFilePrivate
      #pragma mark

      interaction IPeerFilePublicForPeerFilePrivate
      {
        IPeerFilePublicForPeerFilePrivate &forPeerFilePrivate() {return *this;}
        const IPeerFilePublicForPeerFilePrivate &forPeerFilePrivate() const {return *this;}

        static PeerFilePublicPtr createFromPublicKey(
                                                     PeerFilesPtr peerFiles,
                                                     DocumentPtr publicDoc,
                                                     IRSAPublicKeyPtr publicKey,
                                                     const String &peerURI
                                                     );

        static PeerFilePublicPtr loadFromElement(
                                                 PeerFilesPtr peerFiles,
                                                 DocumentPtr publicDoc
                                                 );

        virtual String getPeerURI() const = 0;

        virtual bool verifySignature(ElementPtr signedEl) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerFilePublic
      #pragma mark

      class PeerFilePublic : public IPeerFilePublic,
                             public IPeerFilePublicForPeerFiles,
                             public IPeerFilePublicForPeerFilePrivate
      {
      public:
        friend interaction IPeerFilePublicFactory;
        friend interaction IPeerFilePublic;

      protected:
        PeerFilePublic();

        void init();

      public:
        ~PeerFilePublic();

        static PeerFilePublicPtr convert(IPeerFilePublicPtr peerFilePublic);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => IPeerFilePublic
        #pragma mark

        static String toDebugString(IPeerFilePublicPtr peerFilePublic, bool includePrefixComma = true);

        static PeerFilePublicPtr loadFromElement(ElementPtr publicPeerRootElement);

        virtual PUID getID() const {return mID;}

        virtual ElementPtr saveToElement() const;

        virtual String getPeerURI() const;
        virtual Time getCreated() const;
        virtual Time getExpires() const;
        virtual String getFindSecret() const;
        virtual ElementPtr getSignedSaltBundle() const;

        virtual IdentityBundleElementListPtr getIdentityBundles() const;

        virtual IPeerFilesPtr getAssociatedPeerFiles() const;
        virtual IPeerFilePrivatePtr getAssociatedPeerFilePrivate() const;

        virtual IRSAPublicKeyPtr getPublicKey() const;

        virtual bool verifySignature(ElementPtr signedEl) const;

        virtual SecureByteBlockPtr encrypt(const SecureByteBlock &buffer) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => IPeerFiles
        #pragma mark

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => IPeerFilePublicForPeerFilePrivate
        #pragma mark

        static PeerFilePublicPtr createFromPublicKey(
                                                     PeerFilesPtr peerFiles,
                                                     DocumentPtr publicDoc,
                                                     IRSAPublicKeyPtr publicKey,
                                                     const String &peerURI
                                                     );

        static PeerFilePublicPtr loadFromElement(
                                                 PeerFilesPtr peerFiles,
                                                 DocumentPtr publicDoc
                                                 );

        // (duplicate) virtual String getPeerURI() const;

        // (duplciate) virtual bool verifySignature(ElementPtr signedEl) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => (internal)
        #pragma mark

        String log(const char *message) const;

        bool load();

        ElementPtr findSection(const char *sectionID) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerFilePublic => (data)
        #pragma mark

        PUID mID;

        PeerFilePublicPtr mThisWeak;
        PeerFilesWeakPtr mOuter;

        DocumentPtr mDocument;
        String mPeerURI;

        IRSAPublicKeyPtr mPublicKey;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerFilePublicFactory
      #pragma mark

      interaction IPeerFilePublicFactory
      {
        static IPeerFilePublicFactory &singleton();

        virtual PeerFilePublicPtr loadFromElement(ElementPtr publicPeerRootElement);

        virtual PeerFilePublicPtr createFromPublicKey(
                                                      PeerFilesPtr peerFiles,
                                                      DocumentPtr publicDoc,
                                                      IRSAPublicKeyPtr publicKey,
                                                      const String &peerURI
                                                      );

        virtual PeerFilePublicPtr loadFromElement(
                                                  PeerFilesPtr peerFiles,
                                                  DocumentPtr publicDoc
                                                  );
      };
      
    }
  }
}
