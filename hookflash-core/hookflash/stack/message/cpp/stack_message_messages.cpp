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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::Service
      #pragma mark

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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::Certificate
      #pragma mark

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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::Finder
      #pragma mark

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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::IdentityInfo::Avatar
      #pragma mark

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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::IdentityInfo
      #pragma mark

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
                (mProvider.hasData()) ||

                (mStableID.hasData()) ||
                (mPeerFilePublic) ||

                (0 != mPriority) ||
                (0 != mWeight) ||

                (Time() != mCreated) ||
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
               Helper::getDebugValue("identity provider", mProvider, firstTime) +
               Helper::getDebugValue("stable ID", mStableID, firstTime) +
               IPeerFilePublic::toDebugString(mPeerFilePublic, !firstTime) +
               Helper::getDebugValue("priority", 0 != mPriority ? Stringize<typeof(mPriority)>(mPriority).string() : String(), firstTime) +
               Helper::getDebugValue("weight", 0 != mWeight ? Stringize<typeof(mWeight)>(mPriority).string() : String(), firstTime) +
               Helper::getDebugValue("created", Time() != mCreated ? IMessageHelper::timeToString(mCreated) : String(), firstTime) +
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
      static void merge(bool &result, bool source, bool overwrite)
      {
        if (!source) return;
        if (result) {
          if (!overwrite) return;
        }
        result = source;
      }

      //-----------------------------------------------------------------------
      static void merge(IPeerFilePublicPtr &result, const IPeerFilePublicPtr &source, bool overwrite)
      {
        if (!source) return;
        if (result) {
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
        merge(mProvider, source.mProvider, overwriteExisting);

        merge(mStableID, source.mStableID, overwriteExisting);
        merge(mPeerFilePublic, source.mPeerFilePublic, overwriteExisting);

        merge(mPriority, source.mPriority, overwriteExisting);
        merge(mWeight, source.mWeight, overwriteExisting);

        merge(mCreated, source.mCreated, overwriteExisting);
        merge(mUpdated, source.mUpdated, overwriteExisting);
        merge(mExpires, source.mExpires, overwriteExisting);

        merge(mName, source.mName, overwriteExisting);
        merge(mProfile, source.mProfile, overwriteExisting);
        merge(mVProfile, source.mVProfile, overwriteExisting);
        merge(mAvatars, source.mAvatars, overwriteExisting);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::IdentityInfo
      #pragma mark

      //-----------------------------------------------------------------------
      bool LockboxInfo::hasData() const
      {
        return ((mDomain.hasData()) ||
                (mAccountID.hasData()) ||

                (mAccessToken.hasData()) ||
                (mAccessSecret.hasData()) ||
                (Time() != mAccessSecretExpires) ||
                (mAccessSecretProof.hasData()) ||
                (Time() != mAccessSecretProofExpires) ||

                (mKeyIdentityHalf.hasData()) ||
                (mKeyLockboxHalf.hasData()) ||
                (mHash.hasData()) ||

                (mResetFlag));
      }

      //-----------------------------------------------------------------------
      String LockboxInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("domain", mDomain, firstTime) +
               Helper::getDebugValue("account ID", mAccountID, firstTime) +
               Helper::getDebugValue("access token", mAccessToken, firstTime) +
               Helper::getDebugValue("access secret", mAccessSecret, firstTime) +
               Helper::getDebugValue("access secret expires", Time() != mAccessSecretExpires ? IMessageHelper::timeToString(mAccessSecretExpires) : String(), firstTime) +
               Helper::getDebugValue("access secret proof", mAccessSecretProof, firstTime) +
               Helper::getDebugValue("access secret expires", Time() != mAccessSecretProofExpires ? IMessageHelper::timeToString(mAccessSecretProofExpires) : String(), firstTime) +
               Helper::getDebugValue("key (identity half)", mKeyIdentityHalf, firstTime) +
               Helper::getDebugValue("key (lockbox half)", mKeyLockboxHalf, firstTime) +
               Helper::getDebugValue("hash", mHash, firstTime) +
               Helper::getDebugValue("reset", mResetFlag ? "true" : "false", firstTime);
      }

      //-----------------------------------------------------------------------
      void LockboxInfo::mergeFrom(
                                  const LockboxInfo &source,
                                  bool overwriteExisting
                                  )
      {
        merge(mDomain, source.mDomain, overwriteExisting);
        merge(mAccountID, source.mAccountID, overwriteExisting);

        merge(mAccessToken, source.mAccessToken, overwriteExisting);
        merge(mAccessSecret, source.mAccessSecret, overwriteExisting);
        merge(mAccessSecretExpires, source.mAccessSecretExpires, overwriteExisting);
        merge(mAccessSecretProof, source.mAccessSecretProof, overwriteExisting);
        merge(mAccessSecretProofExpires, source.mAccessSecretProofExpires, overwriteExisting);

        merge(mKeyIdentityHalf, source.mKeyIdentityHalf, overwriteExisting);
        merge(mKeyLockboxHalf, source.mKeyLockboxHalf, overwriteExisting);
        merge(mHash, source.mHash, overwriteExisting);
        merge(mResetFlag, source.mResetFlag, overwriteExisting);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::AgentInfo
      #pragma mark

      //-----------------------------------------------------------------------
      bool AgentInfo::hasData() const
      {
        return ((mUserAgent.hasData()) ||
                (mName.hasData()) ||
                (mImageURL.hasData()));
      }

      //-----------------------------------------------------------------------
      String AgentInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("user agent", mUserAgent, firstTime) +
               Helper::getDebugValue("name", mName, firstTime) +
               Helper::getDebugValue("image url", mImageURL, firstTime);
      }

      //-----------------------------------------------------------------------
      void AgentInfo::mergeFrom(
                                const LockboxInfo &source,
                                bool overwriteExisting
                                )
      {
        merge(mUserAgent, source.mUserAgent, overwriteExisting);
        merge(mName, source.mName, overwriteExisting);
        merge(mImageURL, source.mImageURL, overwriteExisting);
      }

    }
  }
}
