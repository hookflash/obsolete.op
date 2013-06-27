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

#include <openpeer/stack/message/internal/stack_message_messages.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/IPeerFilePublic.h>

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
               (mProtocols.size() > 0) ||
               ((bool)mPublicKey) ||
               (mPriority != 0) ||
               (mWeight != 0) ||
               (mRegion.hasData()) ||
               (Time() != mCreated) ||
               (Time() != mExpires);
      }

      static String getProtocolsDebugValueString(const Finder::ProtocolList &protocols, bool &ioFirstTime)
      {
        String result;
        ULONG index = 0;
        for (Finder::ProtocolList::const_iterator iter = protocols.begin(); iter != protocols.end(); ++iter)
        {
          const Finder::Protocol &protocol = (*iter);
          result += Helper::getDebugValue((String("transport") + Stringize<typeof(index)>(index).string()).c_str(), protocol.mTransport, ioFirstTime);
          result += Helper::getDebugValue((String("srv") + Stringize<typeof(index)>(index).string()).c_str(), protocol.mSRV, ioFirstTime);
        }
        return result;
      }

      //-----------------------------------------------------------------------
      String Finder::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("finder id", mID, firstTime) +
               getProtocolsDebugValueString(mProtocols, firstTime) +
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

                (mReloginKey.hasData()) ||

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
        return Helper::getDebugValue("identity disposition", IdentityInfo::Disposition_NA != mDisposition ? String(toString(mDisposition)) : String(), firstTime) +
               Helper::getDebugValue("identity access token", mAccessToken, firstTime) +
               Helper::getDebugValue("identity access secret", mAccessSecret, firstTime) +
               Helper::getDebugValue("identity access secret expires", Time() != mAccessSecretExpires ? IMessageHelper::timeToString(mAccessSecretExpires) : String(), firstTime) +
               Helper::getDebugValue("identity access secret proof", mAccessSecretProof, firstTime) +
               Helper::getDebugValue("identity access secret expires", Time() != mAccessSecretProofExpires ? IMessageHelper::timeToString(mAccessSecretProofExpires) : String(), firstTime) +
               Helper::getDebugValue("identity relogin key", mReloginKey, firstTime) +
               Helper::getDebugValue("identity base", mBase, firstTime) +
               Helper::getDebugValue("identity", mURI, firstTime) +
               Helper::getDebugValue("identity provider", mProvider, firstTime) +
               Helper::getDebugValue("identity stable ID", mStableID, firstTime) +
               IPeerFilePublic::toDebugString(mPeerFilePublic, !firstTime) +
               Helper::getDebugValue("priority", 0 != mPriority ? Stringize<typeof(mPriority)>(mPriority).string() : String(), firstTime) +
               Helper::getDebugValue("weight", 0 != mWeight ? Stringize<typeof(mWeight)>(mPriority).string() : String(), firstTime) +
               Helper::getDebugValue("created", Time() != mCreated ? IMessageHelper::timeToString(mCreated) : String(), firstTime) +
               Helper::getDebugValue("updated", Time() != mUpdated ? IMessageHelper::timeToString(mUpdated) : String(), firstTime) +
               Helper::getDebugValue("expires", Time() != mExpires ? IMessageHelper::timeToString(mExpires) : String(), firstTime) +
               Helper::getDebugValue("name", mName, firstTime) +
               Helper::getDebugValue("profile", mProfile, firstTime) +
               Helper::getDebugValue("vprofile", mVProfile, firstTime) +
               Helper::getDebugValue("avatars", mAvatars.size() > 0 ? Stringize<AvatarList::size_type>(mAvatars.size()).string() : String(), firstTime);
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
      static void merge(SecureByteBlockPtr &result, const SecureByteBlockPtr &source, bool overwrite)
      {
        if (!source) return;
        if (result) {
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

        merge(mReloginKey, source.mReloginKey, overwriteExisting);

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
      #pragma mark message::LockboxInfo
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

                (mKeyIdentityHalf) ||
                (mKeyLockboxHalf) ||
                (mHash.hasData()) ||

                (mResetFlag));
      }

      //-----------------------------------------------------------------------
      String LockboxInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("lockbox domain", mDomain, firstTime) +
               Helper::getDebugValue("lockbox account ID", mAccountID, firstTime) +
               Helper::getDebugValue("lockbox access token", mAccessToken, firstTime) +
               Helper::getDebugValue("lockbox access secret", mAccessSecret, firstTime) +
               Helper::getDebugValue("lockbox access secret expires", Time() != mAccessSecretExpires ? IMessageHelper::timeToString(mAccessSecretExpires) : String(), firstTime) +
               Helper::getDebugValue("lockbox access secret proof", mAccessSecretProof, firstTime) +
               Helper::getDebugValue("lockbox access secret expires", Time() != mAccessSecretProofExpires ? IMessageHelper::timeToString(mAccessSecretProofExpires) : String(), firstTime) +
               Helper::getDebugValue("lockbox key (identity half)", mKeyIdentityHalf ? String((const char *) mKeyIdentityHalf->BytePtr()) : String(), firstTime) +
               Helper::getDebugValue("lockbox key (lockbox half)", mKeyLockboxHalf ? String((const char *) mKeyLockboxHalf->BytePtr()) : String(), firstTime) +
               Helper::getDebugValue("lockbox hash", mHash, firstTime) +
               Helper::getDebugValue("lockbox reset", mResetFlag ? "true" : "false", firstTime);
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
                (mImageURL.hasData()) ||
                (mAgentURL.hasData()));
      }

      //-----------------------------------------------------------------------
      String AgentInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("user agent", mUserAgent, firstTime) +
               Helper::getDebugValue("name", mName, firstTime) +
               Helper::getDebugValue("image url", mImageURL, firstTime) +
               Helper::getDebugValue("agent url", mAgentURL, firstTime);
      }

      //-----------------------------------------------------------------------
      void AgentInfo::mergeFrom(
                                const AgentInfo &source,
                                bool overwriteExisting
                                )
      {
        merge(mUserAgent, source.mUserAgent, overwriteExisting);
        merge(mName, source.mName, overwriteExisting);
        merge(mImageURL, source.mImageURL, overwriteExisting);
        merge(mAgentURL, source.mAgentURL, overwriteExisting);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::NamespaceGrantChallengeInfo
      #pragma mark

      //-----------------------------------------------------------------------
      bool NamespaceGrantChallengeInfo::hasData() const
      {
        return ((mID.hasData()) ||
                (mName.hasData()) ||
                (mImageURL.hasData()) ||
                (mServiceURL.hasData()) ||
                (mDomains.hasData()));
      }

      //-----------------------------------------------------------------------
      String NamespaceGrantChallengeInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("grant challenge ID", mID, firstTime) +
               Helper::getDebugValue("service name", mName, firstTime) +
               Helper::getDebugValue("image url", mImageURL, firstTime) +
               Helper::getDebugValue("service url", mServiceURL, firstTime) +
               Helper::getDebugValue("domains", mDomains, firstTime);
      }

      //-----------------------------------------------------------------------
      void NamespaceGrantChallengeInfo::mergeFrom(
                                                  const NamespaceGrantChallengeInfo &source,
                                                  bool overwriteExisting
                                                  )
      {
        merge(mID, source.mID, overwriteExisting);
        merge(mName, source.mName, overwriteExisting);
        merge(mImageURL, source.mImageURL, overwriteExisting);
        merge(mServiceURL, source.mServiceURL, overwriteExisting);
        merge(mDomains, source.mDomains, overwriteExisting);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::NamespaceInfo
      #pragma mark

      //-----------------------------------------------------------------------
      bool NamespaceInfo::hasData() const
      {
        return ((mURL.hasData()) ||
                (Time() != mLastUpdated));
      }

      //-----------------------------------------------------------------------
      String NamespaceInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("namespace url", mURL, firstTime) +
               Helper::getDebugValue("last updated", Time() != mLastUpdated ? IMessageHelper::timeToString(mLastUpdated) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void NamespaceInfo::mergeFrom(
                                    const NamespaceInfo &source,
                                    bool overwriteExisting
                                    )
      {
        merge(mURL, source.mURL, overwriteExisting);
        merge(mLastUpdated, source.mLastUpdated, overwriteExisting);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark message::RolodexInfo
      #pragma mark

      //-----------------------------------------------------------------------
      bool RolodexInfo::hasData() const
      {
        return ((mServerToken.hasData()) ||

                (mAccessToken.hasData()) ||
                (mAccessSecret.hasData()) ||
                (Time() != mAccessSecretExpires) ||
                (mAccessSecretProof.hasData()) ||
                (Time() != mAccessSecretProofExpires) ||

                (mVersion.hasData()) ||
                (Time() != mUpdateNext) ||

                (mRefreshFlag));
      }

      //-----------------------------------------------------------------------
      String RolodexInfo::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("rolodex server token", mServerToken, firstTime) +
               Helper::getDebugValue("rolodex access token", mAccessToken, firstTime) +
               Helper::getDebugValue("rolodex access secret", mAccessSecret, firstTime) +
               Helper::getDebugValue("rolodex access secret expires", Time() != mAccessSecretExpires ? IMessageHelper::timeToString(mAccessSecretExpires) : String(), firstTime) +
               Helper::getDebugValue("rolodex access secret proof", mAccessSecretProof, firstTime) +
               Helper::getDebugValue("rolodex access secret expires", Time() != mAccessSecretProofExpires ? IMessageHelper::timeToString(mAccessSecretProofExpires) : String(), firstTime) +
               Helper::getDebugValue("rolodex version", mVersion, firstTime) +
               Helper::getDebugValue("rolodex update next", Time() != mUpdateNext ? IMessageHelper::timeToString(mUpdateNext) : String(), firstTime) +
               Helper::getDebugValue("refresh", mRefreshFlag ? "true" : "false", firstTime);
      }

      //-----------------------------------------------------------------------
      void RolodexInfo::mergeFrom(
                                  const RolodexInfo &source,
                                  bool overwriteExisting
                                  )
      {
        merge(mServerToken, source.mServerToken, overwriteExisting);

        merge(mAccessToken, source.mAccessToken, overwriteExisting);
        merge(mAccessSecret, source.mAccessSecret, overwriteExisting);
        merge(mAccessSecretExpires, source.mAccessSecretExpires, overwriteExisting);
        merge(mAccessSecretProof, source.mAccessSecretProof, overwriteExisting);
        merge(mAccessSecretProofExpires, source.mAccessSecretProofExpires, overwriteExisting);

        merge(mVersion, source.mVersion, overwriteExisting);
        merge(mUpdateNext, source.mUpdateNext, overwriteExisting);

        merge(mRefreshFlag, source.mRefreshFlag, overwriteExisting);
      }

    }
  }
}
