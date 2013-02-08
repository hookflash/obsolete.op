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

#include <hookflash/stack/IPeerContactProfile.h>
#include <hookflash/stack/internal/hookflashTypes.h>

#include <zsLib/String.h>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      class PeerContactProfile : public IPeerContactProfile
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::UINT UINT;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::XML::ElementPtr ElementPtr;
        typedef zsLib::XML::DocumentPtr DocumentPtr;

      protected:
        PeerContactProfile();

      public:
        static PeerContactProfilePtr convert(IPeerContactProfilePtr profile);

        static PeerContactProfilePtr createFromPreGenerated(
                                                            PeerFilesPtr peerFiles,
                                                            DocumentPtr document
                                                            );

        static PeerContactProfilePtr createFromXML(
                                                   ElementPtr contactProfileBundleOrContactProfileElement,
                                                   const char *contactProfileSecret = NULL,        // can be set later
                                                   PeerFilesPtr peerfiles = PeerFilesPtr()
                                                   );

        static PeerContactProfilePtr createContactProfileFromExistingContactProfile(
                                                                                    IPeerContactProfilePtr contactProfile,
                                                                                    const char *contactProfileSecret = NULL,        // can be set later
                                                                                    PeerFilesPtr peerfiles = PeerFilesPtr()
                                                                                    );

        static PeerContactProfilePtr createExternalFromPrivateProfile(ElementPtr privateProfileElement);

        virtual bool isReadOnly() const;
        virtual bool isExternal() const;

        virtual UINT getVersionNumber() const;
        virtual bool setVersionNumber(UINT versionNumber);

        virtual String getContactID() const;
        virtual String getContactProfileSecret() const;

        virtual bool hasContactProfileSecret() const;
        virtual bool usesContactProfileSecret(const char *contactProfileSecret);

        virtual void updateFrom(IPeerContactProfilePtr anotherVersion);

        virtual IPeerFilePublicPtr getPeerFilePublic() const;

        virtual ElementPtr getPublicProfile() const;
        virtual ElementPtr getPrivateProfile() const;

        virtual bool setPublicProfile(ElementPtr profileElement);

        virtual bool setPrivateProfile(ElementPtr profileElement);

        virtual ElementPtr saveToXML() const;

      protected:
        ElementPtr getContactProfileElement() const;
        void internalTemporaryGetContactProfileSecret();

      protected:
        PUID mID;
        mutable RecursiveLock mLock;
        PeerContactProfileWeakPtr mThisWeak;

        PeerFilesWeakPtr mPeerFiles;

        DocumentPtr mDocument;
        String mContactProfileSecret;
      };
    }
  }
}
