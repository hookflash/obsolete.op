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

#include <hookflash/stack/message/internal/stack_message_messages.h>
#include <hookflash/stack/internal/stack_Helper.h>

#include <zsLib/Log.h>
#include <zsLib/Stringize.h>

namespace hookflash { namespace stack { namespace message { ZS_IMPLEMENT_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      using zsLib::Stringize;

      using stack::internal::Helper;

      //-----------------------------------------------------------------------
      const char *IdentityInfo::toString(Dispositions disposition)
      {
        switch (disposition)
        {
          case Disposition_NA:        return "";
          case Disposition_Update:    return "update";
          case Disposition_Remove:    return "remove";
        }
        return "";
      }

      //-----------------------------------------------------------------------
      IdentityInfo::Dispositions IdentityInfo::toDisposition(const char *inStr)
      {
        if (!inStr) return Disposition_NA;
        String str(inStr);
        if ("update" == str) return Disposition_Update;
        if ("remove" == str) return Disposition_Remove;
        return Disposition_NA;
      }

      //-----------------------------------------------------------------------
      bool Service::hasData() const
      {
        return ((mID.hasData()) ||
                (mType.hasData()) ||
                (mVersion.hasData()) ||
                (mMethods.size() > 0));
      }

      //-----------------------------------------------------------------------
      String Service::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("service id", mID, firstTime) +
               Helper::getDebugValue("type", mType, firstTime) +
               Helper::getDebugValue("version", mVersion, firstTime) +
               Helper::getDebugValue("methods", mMethods.size() > 0 ? Stringize<size_t>(mMethods.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      bool Service::Method::hasData() const
      {
        return ((mName.hasData()) ||
                (mURI.hasData()) ||
                (mUsername.hasData()) ||
                (mPassword.hasData()));
      }

      //-----------------------------------------------------------------------
      String Service::Method::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("service method name", mName, firstTime) +
               Helper::getDebugValue("uri", mURI, firstTime) +
               Helper::getDebugValue("username", mUsername, firstTime) +
               Helper::getDebugValue("password", mPassword, firstTime);
      }

      //-----------------------------------------------------------------------
      bool Certificate::hasData() const
      {
        return (mID.hasData()) ||
               (mService.hasData()) ||
               (Time() != mExpires) ||
               ((bool)mPublicKey);
      }

      //-----------------------------------------------------------------------
      String Certificate::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("certificate id", mID, firstTime) +
               Helper::getDebugValue("service", mService, firstTime) +
               Helper::getDebugValue("expires", Time() != mExpires ? IMessageHelper::timeToString(mExpires) : String(), firstTime) +
               Helper::getDebugValue("public key", mPublicKey ? String("true") : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      bool Finder::hasData() const
      {
        return (mID.hasData()) ||
               (mTransport.hasData()) ||
               (mSRV.hasData()) ||
               ((bool)mPublicKey) ||
               (mPriority != 0) ||
               (mWeight != 0) ||
               (mRegion.hasData()) ||
               (Time() != mCreated) ||
               (Time() != mExpires);
      }

      //-----------------------------------------------------------------------
      String Finder::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("finder id", mID, firstTime) +
               Helper::getDebugValue("transport", mTransport, firstTime) +
               Helper::getDebugValue("srv", mSRV, firstTime) +
               Helper::getDebugValue("public key", mPublicKey ? String("true") : String(), firstTime) +
               Helper::getDebugValue("priority", 0 != mPriority ? Stringize<typeof(mPriority)>(mPriority).string() : String(), firstTime) +
               Helper::getDebugValue("weight", 0 != mWeight ? Stringize<typeof(mWeight)>(mWeight).string() : String(), firstTime) +
               Helper::getDebugValue("region", mRegion, firstTime) +
               Helper::getDebugValue("created", Time() != mCreated ? IMessageHelper::timeToString(mCreated) : String(), firstTime) +
               Helper::getDebugValue("expires", Time() != mExpires ? IMessageHelper::timeToString(mExpires) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      bool IdentityInfo::Avatar::hasData() const
      {
        return ((mName.hasData()) ||
                (mURL.hasData()) ||
                (0 != mWidth) ||
                (0 != mHeight));
      }

      //-----------------------------------------------------------------------
      String IdentityInfo::Avatar::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("avatar name", mName, firstTime) +
               Helper::getDebugValue("url", mURL, firstTime) +
               Helper::getDebugValue("width", 0 != mWidth ? Stringize<typeof(mWidth)>(mWidth).string() : String(), firstTime) +
               Helper::getDebugValue("height", 0 != mHeight ? Stringize<typeof(mHeight)>(mHeight).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      bool IdentityInfo::hasData() const
      {
        return ((Disposition_NA != mDisposition) ||

                (mAccessToken.hasData()) ||
                (mAccessSecret.hasData()) ||
                (Time() != mAccessSecretExpires) ||
                (mAccessSecretProof.hasData()) ||
                (Time() != mAccessSecretProofExpires) ||

                (mBase.hasData()) ||
                (mURI.hasData()) ||
                (mURIEncrypted.hasData()) ||
                (mHash.hasData()) ||
                (mProvider.hasData()) ||

                (mContactUserID.hasData()) ||
                (mContact.hasData()) ||
                (mContactFindSecret.hasData()) ||
                (mPrivatePeerFileSalt.hasData()) ||
                (mPrivatePeerFileSecretEncrypted.hasData()) ||

                (Time() != mLastReset) ||
                (mReloginAccessKey.hasData()) ||
                (mReloginAccessKeyEncrypted.hasData()) ||
                (0 != mPriority) ||
                (0 != mWeight) ||

                (mSecret.hasData()) ||
                (mSecretSalt.hasData()) ||
                (mSecretEncrypted.hasData()) ||
                (mSecretDecryptionKeyEncrypted.hasData()) ||

                (Time() != mUpdated) ||
                (Time() != mExpires) ||

                (mName.hasData()) ||
                (mProfile.hasData()) ||
                (mVProfile.hasData()) ||

                (mAvatars.size() > 0));
      }

      //-----------------------------------------------------------------------
      String IdentityInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("disposition", IdentityInfo::Disposition_NA != mDisposition ? String(toString(mDisposition)) : String(), firstTime) +
               Helper::getDebugValue("access token", mAccessToken, firstTime) +
               Helper::getDebugValue("access secret", mAccessSecret, firstTime) +
               Helper::getDebugValue("access secret expires", Time() != mAccessSecretExpires ? IMessageHelper::timeToString(mAccessSecretExpires) : String(), firstTime) +
               Helper::getDebugValue("access secret proof", mAccessSecretProof, firstTime) +
               Helper::getDebugValue("access secret expires", Time() != mAccessSecretProofExpires ? IMessageHelper::timeToString(mAccessSecretProofExpires) : String(), firstTime) +
               Helper::getDebugValue("identity base", mBase, firstTime) +
               Helper::getDebugValue("identity", mURI, firstTime) +
               Helper::getDebugValue("identity (encrypted)", mURIEncrypted, firstTime) +
               Helper::getDebugValue("identity (hash)", mHash, firstTime) +
               Helper::getDebugValue("identity provider", mProvider, firstTime) +
               Helper::getDebugValue("contact user ID", mContactUserID, firstTime) +
               Helper::getDebugValue("contact", mContact, firstTime) +
               Helper::getDebugValue("contact find secret", mContactFindSecret, firstTime) +
               Helper::getDebugValue("private peer file salt", mPrivatePeerFileSalt, firstTime) +
               Helper::getDebugValue("private peer secret (encrypted)", mPrivatePeerFileSecretEncrypted, firstTime) +
               Helper::getDebugValue("last reset", Time() != mLastReset ? IMessageHelper::timeToString(mLastReset) : String(), firstTime) +
               Helper::getDebugValue("relogin access key", mReloginAccessKey, firstTime) +
               Helper::getDebugValue("relogin access key (encrypted)", mReloginAccessKeyEncrypted, firstTime) +
               Helper::getDebugValue("priority", 0 != mPriority ? Stringize<typeof(mPriority)>(mPriority).string() : String(), firstTime) +
               Helper::getDebugValue("weight", 0 != mWeight ? Stringize<typeof(mWeight)>(mPriority).string() : String(), firstTime) +
               Helper::getDebugValue("secret", mSecret, firstTime) +
               Helper::getDebugValue("secret salt", mSecretSalt, firstTime) +
               Helper::getDebugValue("secret (encrypted)", mSecretEncrypted, firstTime) +
               Helper::getDebugValue("secret decryption key (encrypted)", mSecretDecryptionKeyEncrypted, firstTime) +
               Helper::getDebugValue("updated", Time() != mUpdated ? IMessageHelper::timeToString(mUpdated) : String(), firstTime) +
               Helper::getDebugValue("expires", Time() != mExpires ? IMessageHelper::timeToString(mExpires) : String(), firstTime) +
               Helper::getDebugValue("name", mName, firstTime) +
               Helper::getDebugValue("profile", mProfile, firstTime) +
               Helper::getDebugValue("vprofile", mVProfile, firstTime) +
               Helper::getDebugValue("avatars", mAvatars.size() > 0 ? Stringize<typeof(size_t)>(mAvatars.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      static void merge(String &result, const String &source, bool overwrite)
      {
        if (source.isEmpty()) return;
        if (result.hasData()) {
          if (!overwrite) return;
        }
        result = source;
      }

      //-----------------------------------------------------------------------
      static void merge(Time &result, const Time &source, bool overwrite)
      {
        if (Time() == source) return;
        if (Time() != result) {
          if (!overwrite) return;
        }
        result = source;
      }

      //-----------------------------------------------------------------------
      static void merge(WORD &result, WORD source, bool overwrite)
      {
        if (0 == source) return;
        if (0 != result) {
          if (!overwrite) return;
        }
        result = source;
      }

      //-----------------------------------------------------------------------
      static void merge(IdentityInfo::Dispositions &result, IdentityInfo::Dispositions source, bool overwrite)
      {
        if (IdentityInfo::Disposition_NA == source) return;
        if (IdentityInfo::Disposition_NA != result) {
          if (!overwrite) return;
        }
        result = source;
      }

      //-----------------------------------------------------------------------
      static void merge(IdentityInfo::AvatarList &result, const IdentityInfo::AvatarList &source, bool overwrite)
      {
        if (source.size() < 1) return;
        if (result.size() > 0) {
          if (!overwrite) return;
        }
        result = source;
      }

      //-----------------------------------------------------------------------
      void IdentityInfo::mergeFrom(
                                   const IdentityInfo &source,
                                   bool overwriteExisting
                                   )
      {
        merge(mDisposition, source.mDisposition, overwriteExisting);

        merge(mAccessToken, source.mAccessToken, overwriteExisting);
        merge(mAccessSecret, source.mAccessSecret, overwriteExisting);
        merge(mAccessSecretExpires, source.mAccessSecretExpires, overwriteExisting);
        merge(mAccessSecretProof, source.mAccessSecretProof, overwriteExisting);
        merge(mAccessSecretProofExpires, source.mAccessSecretProofExpires, overwriteExisting);

        merge(mBase, source.mBase, overwriteExisting);
        merge(mURI, source.mURI, overwriteExisting);
        merge(mURIEncrypted, source.mURIEncrypted, overwriteExisting);
        merge(mHash, source.mHash, overwriteExisting);
        merge(mProvider, source.mProvider, overwriteExisting);

        merge(mContactUserID, source.mContactUserID, overwriteExisting);
        merge(mContact, source.mContact, overwriteExisting);
        merge(mContactFindSecret, source.mContactFindSecret, overwriteExisting);
        merge(mPrivatePeerFileSalt, source.mPrivatePeerFileSalt, overwriteExisting);
        merge(mPrivatePeerFileSecretEncrypted, source.mPrivatePeerFileSecretEncrypted, overwriteExisting);


        merge(mLastReset, source.mLastReset, overwriteExisting);
        merge(mReloginAccessKey, source.mReloginAccessKey, overwriteExisting);
        merge(mReloginAccessKeyEncrypted, source.mReloginAccessKeyEncrypted, overwriteExisting);

        merge(mSecret, source.mSecret, overwriteExisting);
        merge(mSecretSalt, source.mSecretSalt, overwriteExisting);
        merge(mSecretEncrypted, source.mSecretEncrypted, overwriteExisting);
        merge(mSecretDecryptionKeyEncrypted, source.mSecretDecryptionKeyEncrypted, overwriteExisting);

        merge(mPriority, source.mPriority, overwriteExisting);
        merge(mWeight, source.mWeight, overwriteExisting);

        merge(mUpdated, source.mUpdated, overwriteExisting);
        merge(mExpires, source.mExpires, overwriteExisting);

        merge(mName, source.mName, overwriteExisting);
        merge(mProfile, source.mProfile, overwriteExisting);
        merge(mVProfile, source.mVProfile, overwriteExisting);
        merge(mAvatars, source.mAvatars, overwriteExisting);
      }
    }
  }
}
