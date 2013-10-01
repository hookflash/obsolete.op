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

#include <openpeer/core/core.h>
#include <openpeer/core/internal/core.h>
#include <zsLib/Log.h>

namespace openpeer { namespace core { ZS_IMPLEMENT_SUBSYSTEM(openpeer_core) } }
namespace openpeer { namespace core { ZS_IMPLEMENT_SUBSYSTEM(openpeer_media) } }
namespace openpeer { namespace core { ZS_IMPLEMENT_SUBSYSTEM(openpeer_webrtc) } }
ZS_IMPLEMENT_SUBSYSTEM(openpeer_sdk)
namespace openpeer { namespace core { namespace application { ZS_IMPLEMENT_SUBSYSTEM(openpeer_application) } } }

namespace openpeer
{
  namespace core
  {
    bool ContactProfileInfo::hasData() const
    {
      return ((mContact) ||
              (mProfileBundleEl));
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark RolodexContact
    #pragma mark

    //-------------------------------------------------------------------------
    RolodexContact::RolodexContact() :
      mDisposition(Disposition_NA)
    {
    }

    //-------------------------------------------------------------------------
    bool RolodexContact::hasData() const
    {
      return ((Disposition_NA != mDisposition) ||
              (mIdentityURI.hasData()) ||
              (mIdentityProvider.hasData()) ||
              (mName.hasData()) ||
              (mProfileURL.hasData()) ||
              (mVProfileURL.hasData()) ||
              (mAvatars.size() > 0));
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IdentityContact
    #pragma mark

    //-------------------------------------------------------------------------
    IdentityContact::IdentityContact() :
      RolodexContact(),
      mPriority(0),
      mWeight(0)
    {
    }

    //-------------------------------------------------------------------------
    IdentityContact::IdentityContact(const RolodexContact &rolodexInfo) :
      mPriority(0),
      mWeight(0)
    {
      // rolodex disposition is "lost" as it has no meaning once translated into an actual identity structure
      mDisposition = rolodexInfo.mDisposition;
      mIdentityURI = rolodexInfo.mIdentityURI;
      mIdentityProvider = rolodexInfo.mIdentityProvider;

      mName = rolodexInfo.mName;
      mProfileURL = rolodexInfo.mProfileURL;
      mVProfileURL = rolodexInfo.mVProfileURL;
      mAvatars = rolodexInfo.mAvatars;
    }

    //-------------------------------------------------------------------------
    bool IdentityContact::hasData() const
    {
      return ((mIdentityURI.hasData()) ||
              (mIdentityProvider.hasData()) ||
              (mStableID.hasData()) ||
              (mPeerFilePublic) ||
              (mIdentityProofBundleEl) ||
              (0 != mPriority) ||
              (0 != mWeight) ||
              (Time() != mLastUpdated) ||
              (Time() != mExpires) ||
              (mName.hasData()) ||
              (mProfileURL.hasData()) ||
              (mVProfileURL.hasData()) ||
              (mAvatars.size() > 0));
    }    
  }
}
