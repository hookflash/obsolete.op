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
#include <hookflash/stack/internal/stack_PeerContactProfile.h>
#include <hookflash/stack/internal/stack_PeerFilePublic.h>
#include <hookflash/stack/internal/stack_PeerFilePrivate.h>
#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

using namespace zsLib::XML;
using zsLib::PUID;

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      PeerFiles::PeerFiles() :
        mID(zsLib::createPUID())
      {
      }

      PeerFilesPtr PeerFiles::generatePeerFile(
                                               const char *password,
                                               zsLib::XML::ElementPtr signedSalt
                                               )
      {
        return PeerFilePrivate::generate(PeerFilesPtr(new PeerFiles), password, signedSalt);
      }

      PeerFilesPtr PeerFiles::loadPeerFilesFromXML(
                                                   const char *password,
                                                   zsLib::XML::ElementPtr privatePeerRootElement
                                                   )
      {
        return PeerFilePrivate::loadFromXML(PeerFilesPtr(new PeerFiles), password, privatePeerRootElement);
      }

      bool PeerFiles::loadContactProfileFromXML(
                                                const char *password,
                                                zsLib::XML::ElementPtr contactProfileRootElement
                                                )
      {
        IPeerFilePrivatePtr privatePeer = getPrivate();
        if (!privatePeer) return false;

        zsLib::String contactProfileSecret = privatePeer->getContactProfileSecret(password);
        if (contactProfileSecret.isEmpty()) return false;

        PeerContactProfilePtr contactProfile = PeerContactProfile::createFromXML(
                                                                                 contactProfileRootElement,
                                                                                 contactProfileSecret,
                                                                                 mThisWeak.lock()
                                                                                 );
        if (!contactProfile) return false;

        mContactProfile = contactProfile;
        return true;
      }

      bool PeerFiles::loadContactProfileFromExistingContactProfile(
                                                                   const char *password,
                                                                   IPeerContactProfilePtr contactProfile
                                                                   )
      {
        IPeerFilePrivatePtr privatePeer = getPrivate();
        if (!privatePeer) return false;

        zsLib::String contactProfileSecret = privatePeer->getContactProfileSecret(password);
        if (contactProfileSecret.isEmpty()) return false;

        PeerContactProfilePtr newContactProfile = PeerContactProfile::createContactProfileFromExistingContactProfile(
                                                                                                                     contactProfile,
                                                                                                                     contactProfileSecret,
                                                                                                                     mThisWeak.lock()
                                                                                                                     );
        if (!newContactProfile) return false;

        mContactProfile = newContactProfile;
        return true;
      }

      zsLib::XML::ElementPtr PeerFiles::savePrivateAndPublicToXML() const
      {
        if (!mPrivate) return ElementPtr();
        return mPrivate->saveToXML();
      }

      zsLib::XML::ElementPtr PeerFiles::saveContactProfileToXML() const
      {
        if (!mContactProfile) return ElementPtr();
        return mContactProfile->saveToXML();
      }

      IPeerFilePublicPtr PeerFiles::getPublic() const
      {
        return mPublic;
      }

      IPeerFilePrivatePtr PeerFiles::getPrivate() const
      {
        return mPrivate;
      }

      IPeerContactProfilePtr PeerFiles::getContactProfile() const
      {
        return mContactProfile;
      }
    }

    IPeerFilesPtr IPeerFiles::generatePeerFile(
                                               const char *password,
                                               zsLib::XML::ElementPtr signedSalt
                                               )
    {
      return internal::PeerFiles::generatePeerFile(password, signedSalt);
    }

    IPeerFilesPtr IPeerFiles::loadPeerFilesFromXML(
                                                   const char *password,
                                                   zsLib::XML::ElementPtr privatePeerRootElement
                                                   )
    {
      return internal::PeerFiles::loadPeerFilesFromXML(password, privatePeerRootElement);
    }
  }
}
