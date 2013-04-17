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

#include <hookflash/stack/types.h>
#include <hookflash/services/IICESocket.h>

#include <list>
#include <map>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      using zsLib::XML::Document;
      using zsLib::XML::Element;
      using zsLib::XML::Text;
      using zsLib::XML::TextPtr;
      using zsLib::XML::Attribute;
      using zsLib::XML::AttributePtr;

      typedef std::list<String> RouteList;
      typedef std::list<String> StringList;

      typedef hookflash::services::IICESocket::Candidate Candidate;
      typedef hookflash::services::IICESocket::CandidateList CandidateList;

      typedef stack::LocationInfo LocationInfo;
      typedef stack::LocationInfoList LocationInfoList;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Service
      #pragma mark

      struct Service
      {
        struct Method
        {
          typedef String MethodName;
          MethodName mName;
          String mURI;
          String mUsername;
          String mPassword;

          bool hasData() const;
          String getDebugValueString(bool includeCommaPrefix = true) const;
        };
        typedef boost::shared_ptr<Method> MethodPtr;
        typedef boost::weak_ptr<Method> MethodWeakPtr;
        typedef std::map<Method::MethodName, Method> MethodMap;
        typedef String ServiceID;
        typedef String Type;

        ServiceID mID;
        String mType;
        String mVersion;

        MethodMap mMethods;

        bool hasData() const;
        String getDebugValueString(bool includeCommaPrefix = true) const;
      };
      typedef boost::shared_ptr<Service> ServicePtr;
      typedef boost::weak_ptr<Service> ServiceWeakPtr;
      typedef std::map<Service::ServiceID, Service> ServiceMap;
      typedef std::map<Service::Type, Service> ServiceTypeMap;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Certificate
      #pragma mark

      struct Certificate
      {
        typedef String CertificateID;
        String mID;
        String mService;
        Time mExpires;
        IRSAPublicKeyPtr mPublicKey;

        bool hasData() const;
        String getDebugValueString(bool includeCommaPrefix = true) const;
      };
      typedef std::map<Certificate::CertificateID, Certificate> CertificateMap;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Finder
      #pragma mark

      struct Finder
      {
        String mID;
        String mTransport;
        String mSRV;
        IRSAPublicKeyPtr mPublicKey;
        WORD  mPriority;
        WORD  mWeight;
        String mRegion;
        Time mCreated;
        Time mExpires;

        Finder() :
          mPriority(0),
          mWeight(0)
        {}

        bool hasData() const;
        String getDebugValueString(bool includeCommaPrefix = true) const;
      };
      typedef std::list<Finder> FinderList;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IdentityInfo
      #pragma mark

      struct IdentityInfo
      {
        enum Dispositions
        {
          Disposition_NA, // not applicable
          Disposition_Update,
          Disposition_Remove,
        };

        struct Avatar
        {
          String mName;
          String mURL;
          int mWidth;
          int mHeight;

          Avatar() : mWidth(0), mHeight(0) {}
          bool hasData() const;
          String getDebugValueString(bool includeCommaPrefix = true) const;
        };
        typedef std::list<Avatar> AvatarList;

        static const char *toString(Dispositions diposition);
        static Dispositions toDisposition(const char *str);

        Dispositions mDisposition;

        String mAccessToken;
        String mAccessSecret;
        Time mAccessSecretExpires;
        String mAccessSecretProof;
        Time mAccessSecretProofExpires;

        String mBase;
        String mURI;
        String mProvider;

        String mStableID;
        IPeerFilePublicPtr mPeerFilePublic;

        WORD mPriority;
        WORD mWeight;

        Time mCreated;
        Time mUpdated;
        Time mExpires;

        String mName;
        String mProfile;
        String mVProfile;

        AvatarList mAvatars;

        IdentityInfo() : mDisposition(Disposition_NA), mPriority(0), mWeight(0) {}
        bool hasData() const;
        String getDebugValueString(bool includeCommaPrefix = true) const;

        void mergeFrom(
                       const IdentityInfo &source,
                       bool overwriteExisting = true
                       );
      };
      typedef std::list<IdentityInfo> IdentityInfoList;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LockboxInfo
      #pragma mark

      struct LockboxInfo
      {
        String mDomain;
        String mAccountID;

        String mAccessToken;
        String mAccessSecret;
        Time mAccessSecretExpires;
        String mAccessSecretProof;
        Time mAccessSecretProofExpires;

        String mKeyLockboxHalf;
        String mKeyIdentityHalf;
        String mHash;

        bool mResetFlag;

        LockboxInfo() : mResetFlag(false) {}
        bool hasData() const;
        String getDebugValueString(bool includeCommaPrefix = true) const;

        void mergeFrom(
                       const LockboxInfo &source,
                       bool overwriteExisting = true
                       );
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AgentInfo
      #pragma mark

      struct AgentInfo
      {
        String mUserAgent;
        String mName;
        String mImageURL;

        AgentInfo() {}
        bool hasData() const;
        String getDebugValueString(bool includeCommaPrefix = true) const;

        void mergeFrom(
                       const AgentInfo &source,
                       bool overwriteExisting = true
                       );
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (forwards)
      #pragma mark

      class Message;
      typedef boost::shared_ptr<Message> MessagePtr;
      typedef boost::weak_ptr<Message> MessageWeakPtr;

      interaction IMessageHelper;
      typedef boost::shared_ptr<IMessageHelper> IMessageHelperPtr;
      typedef boost::weak_ptr<IMessageHelper> IMessageHelperWeakPtr;

      interaction IMessageFactory;
      typedef boost::shared_ptr<IMessageFactory> IMessageFactoryPtr;
      typedef boost::weak_ptr<IMessageFactory> IMessageFactoryWeakPtr;

      class MessageRequest;
      typedef boost::shared_ptr<MessageRequest> MessageRequestPtr;
      typedef boost::weak_ptr<MessageRequest> MessageRequestWeakPtr;

      class MessageResult;
      typedef boost::shared_ptr<MessageResult> MessageResultPtr;
      typedef boost::weak_ptr<MessageResult> MessageResultWeakPtr;

      class MessageNotify;
      typedef boost::shared_ptr<MessageNotify> MessageNotifyPtr;
      typedef boost::weak_ptr<MessageNotify> MessageNotifyWeakPtr;

      class MessageReply;
      typedef boost::shared_ptr<MessageReply> MessageReplyPtr;
      typedef boost::weak_ptr<MessageReply> MessageReplyWeakPtr;

      namespace bootstrapper
      {
        class MessageFactoryBootstrapper;
        typedef boost::shared_ptr<MessageFactoryBootstrapper> MessageFactoryBootstrapperPtr;
        typedef boost::weak_ptr<MessageFactoryBootstrapper> MessageFactoryBootstrapperWeakPtr;

        class ServicesGetRequest;
        typedef boost::shared_ptr<ServicesGetRequest> ServicesGetRequestPtr;
        typedef boost::weak_ptr<ServicesGetRequest> ServicesGetRequestWeakPtr;

        class ServicesGetResult;
        typedef boost::shared_ptr<ServicesGetResult> ServicesGetResultPtr;
        typedef boost::weak_ptr<ServicesGetResult> ServicesGetResultWeakPtr;
      }

      namespace bootstrapped_finder
      {
        class MessageFactoryBootstrappedFinder;
        typedef boost::shared_ptr<MessageFactoryBootstrappedFinder> MessageFactoryBootstrappedFinderPtr;
        typedef boost::weak_ptr<MessageFactoryBootstrappedFinder> MessageFactoryBootstrappedFinderWeakPtr;

        class FindersGetRequest;
        typedef boost::shared_ptr<FindersGetRequest> FindersGetRequestPtr;
        typedef boost::weak_ptr<FindersGetRequest> FindersGetRequestWeakPtr;

        class FindersGetResult;
        typedef boost::shared_ptr<FindersGetResult> FindersGetResultPtr;
        typedef boost::weak_ptr<FindersGetResult> FindersGetResultWeakPtr;
      }

      namespace certificates
      {
        class MessageFactoryCertificates;
        typedef boost::shared_ptr<MessageFactoryCertificates> MessageFactoryCertificatesPtr;
        typedef boost::weak_ptr<MessageFactoryCertificates> MessageFactoryCertificatesWeakPtr;

        class CertificatesGetRequest;
        typedef boost::shared_ptr<CertificatesGetRequest> CertificatesGetRequestPtr;
        typedef boost::weak_ptr<CertificatesGetRequest> CertificatesGetRequestWeakPtr;

        class CertificatesGetResult;
        typedef boost::shared_ptr<CertificatesGetResult> CertificatesGetResultPtr;
        typedef boost::weak_ptr<CertificatesGetResult> CertificatesGetResultWeakPtr;
      }

      namespace identity
      {
        class MessageFactoryIdentity;
        typedef boost::shared_ptr<MessageFactoryIdentity> MessageFactoryIdentityPtr;
        typedef boost::weak_ptr<MessageFactoryIdentity> MessageFactoryIdentityWeakPtr;

        class IdentityAccessWindowNotify;
        typedef boost::shared_ptr<IdentityAccessWindowNotify> IdentityAccessWindowNotifyPtr;
        typedef boost::weak_ptr<IdentityAccessWindowNotify> IdentityAccessWindowNotifyWeakPtr;
        
        class IdentityAccessStartNotify;
        typedef boost::shared_ptr<IdentityAccessStartNotify> IdentityAccessStartNotifyPtr;
        typedef boost::weak_ptr<IdentityAccessStartNotify> IdentityAccessStartNotifyWeakPtr;

        class IdentityAccessCompleteNotify;
        typedef boost::shared_ptr<IdentityAccessCompleteNotify> IdentityAccessCompleteNotifyPtr;
        typedef boost::weak_ptr<IdentityAccessCompleteNotify> IdentityAccessCompleteNotifyWeakPtr;

        class IdentityAccessLockboxUpdateRequest;
        typedef boost::shared_ptr<IdentityAccessLockboxUpdateRequest> IdentityAccessLockboxUpdateRequestPtr;
        typedef boost::weak_ptr<IdentityAccessLockboxUpdateRequest> IdentityAccessLockboxUpdateRequestWeakPtr;

        class IdentityAccessLockboxUpdateResult;
        typedef boost::shared_ptr<IdentityAccessLockboxUpdateResult> IdentityAccessLockboxUpdateResultPtr;
        typedef boost::weak_ptr<IdentityAccessLockboxUpdateResult> IdentityAccessLockboxUpdateResultWeakPtr;
        
        class IdentityAccessValidateRequest;
        typedef boost::shared_ptr<IdentityAccessValidateRequest> IdentityAccessValidateRequestPtr;
        typedef boost::weak_ptr<IdentityAccessValidateRequest> IdentityAccessValidateRequestWeakPtr;

        class IdentityAccessValidateResult;
        typedef boost::shared_ptr<IdentityAccessValidateResult> IdentityAccessValidateResultPtr;
        typedef boost::weak_ptr<IdentityAccessValidateResult> IdentityAccessValidateResultWeakPtr;
        
        class IdentityLockupUpdateRequest;
        typedef boost::shared_ptr<IdentityLockupUpdateRequest> IdentityLockupUpdateRequestPtr;
        typedef boost::weak_ptr<IdentityLockupUpdateRequest> IdentityLockupUpdateRequestWeakPtr;

        class IdentityLockupUpdateResult;
        typedef boost::shared_ptr<IdentityLockupUpdateResult> IdentityLockupUpdateResultPtr;
        typedef boost::weak_ptr<IdentityLockupUpdateResult> IdentityLockupUpdateResultWeakPtr;

        class IdentitySignRequest;
        typedef boost::shared_ptr<IdentitySignRequest> IdentitySignRequestPtr;
        typedef boost::weak_ptr<IdentitySignRequest> IdentitySignRequestWeakPtr;

        class IdentitySignResult;
        typedef boost::shared_ptr<IdentitySignResult> IdentitySignResultPtr;
        typedef boost::weak_ptr<IdentitySignResult> IdentitySignResultWeakPtr;
      }

      namespace identity_lockbox
      {
        class MessageFactoryIdentityLockbox;
        typedef boost::shared_ptr<MessageFactoryIdentityLockbox> MessageFactoryIdentityLockboxPtr;
        typedef boost::weak_ptr<MessageFactoryIdentityLockbox> MessageFactoryIdentityLockboxWeakPtr;

        class LockboxAccessRequest;
        typedef boost::shared_ptr<LockboxAccessRequest> LockboxAccessRequestPtr;
        typedef boost::weak_ptr<LockboxAccessRequest> LockboxAccessRequestWeakPtr;

        class LockboxAccessResult;
        typedef boost::shared_ptr<LockboxAccessResult> LockboxAccessResultPtr;
        typedef boost::weak_ptr<LockboxAccessResult> LockboxAccessResultWeakPtr;

        class LockboxAccessValidateRequest;
        typedef boost::shared_ptr<LockboxAccessValidateRequest> LockboxAccessValidateRequestPtr;
        typedef boost::weak_ptr<LockboxAccessValidateRequest> LockboxAccessValidateRequestWeakPtr;

        class LockboxAccessValidateResult;
        typedef boost::shared_ptr<LockboxAccessValidateResult> LockboxAccessValidateResultPtr;
        typedef boost::weak_ptr<LockboxAccessValidateResult> LockboxAccessValidateResultWeakPtr;

        class LockboxIdentitiesUpdateRequest;
        typedef boost::shared_ptr<LockboxIdentitiesUpdateRequest> LockboxIdentitiesUpdateRequestPtr;
        typedef boost::weak_ptr<LockboxIdentitiesUpdateRequest> LockboxIdentitiesUpdateRequestWeakPtr;

        class LockboxIdentitiesUpdateResult;
        typedef boost::shared_ptr<LockboxIdentitiesUpdateResult> LockboxIdentitiesUpdateResultPtr;
        typedef boost::weak_ptr<LockboxIdentitiesUpdateResult> LockboxIdentitiesUpdateResultWeakPtr;

        class LockboxNamespaceGrantWindowNotify;
        typedef boost::shared_ptr<LockboxNamespaceGrantWindowNotify> LockboxNamespaceGrantWindowNotifyPtr;
        typedef boost::weak_ptr<LockboxNamespaceGrantWindowNotify> LockboxNamespaceGrantWindowNotifyWeakPtr;

        class LockboxNamespaceGrantStartNotify;
        typedef boost::shared_ptr<LockboxNamespaceGrantStartNotify> LockboxNamespaceGrantStartNotifyPtr;
        typedef boost::weak_ptr<LockboxNamespaceGrantStartNotify> LockboxNamespaceGrantStartNotifyWeakPtr;

        class LockboxNamespaceGrantCompleteNotify;
        typedef boost::shared_ptr<LockboxNamespaceGrantCompleteNotify> LockboxNamespaceGrantCompleteNotifyPtr;
        typedef boost::weak_ptr<LockboxNamespaceGrantCompleteNotify> LockboxNamespaceGrantCompleteNotifyWeakPtr;

        class LockboxContentGetRequest;
        typedef boost::shared_ptr<LockboxContentGetRequest> LockboxContentGetRequestPtr;
        typedef boost::weak_ptr<LockboxContentGetRequest> LockboxContentGetRequestWeakPtr;
        
        class LockboxContentGetResult;
        typedef boost::shared_ptr<LockboxContentGetResult> LockboxContentGetResultPtr;
        typedef boost::weak_ptr<LockboxContentGetResult> LockboxContentGetResultWeakPtr;
        
        class LockboxContentSetRequest;
        typedef boost::shared_ptr<LockboxContentSetRequest> LockboxContentSetRequestPtr;
        typedef boost::weak_ptr<LockboxContentSetRequest> LockboxContentSetRequestWeakPtr;

        class LockboxContentSetResult;
        typedef boost::shared_ptr<LockboxContentSetResult> LockboxContentSetResultPtr;
        typedef boost::weak_ptr<LockboxContentSetResult> LockboxContentSetResultWeakPtr;

        class LockboxAdminWindowNotify;
        typedef boost::shared_ptr<LockboxAdminWindowNotify> LockboxAdminWindowNotifyPtr;
        typedef boost::weak_ptr<LockboxAdminWindowNotify> LockboxAdminWindowNotifyWeakPtr;

        class LockboxAdminStartNotify;
        typedef boost::shared_ptr<LockboxAdminStartNotify> LockboxAdminStartNotifyPtr;
        typedef boost::weak_ptr<LockboxAdminStartNotify> LockboxAdminStartNotifyWeakPtr;

        class LockboxAdminCompleteNotify;
        typedef boost::shared_ptr<LockboxAdminCompleteNotify> LockboxAdminCompleteNotifyPtr;
        typedef boost::weak_ptr<LockboxAdminCompleteNotify> LockboxAdminCompleteNotifyWeakPtr;

        class LockboxNamespacePreapprovedGrantRequest;
        typedef boost::shared_ptr<LockboxNamespacePreapprovedGrantRequest> LockboxNamespacePreapprovedGrantRequestPtr;
        typedef boost::weak_ptr<LockboxNamespacePreapprovedGrantRequest> LockboxNamespacePreapprovedGrantRequestWeakPtr;

        class LockboxNamespacePreapprovedGrantResult;
        typedef boost::shared_ptr<LockboxNamespacePreapprovedGrantResult> LockboxNamespacePreapprovedGrantResultPtr;
        typedef boost::weak_ptr<LockboxNamespacePreapprovedGrantResult> LockboxNamespacePreapprovedGrantResultWeakPtr;
      }

      namespace identity_lookup
      {
        class MessageFactoryIdentityLookup;
        typedef boost::shared_ptr<MessageFactoryIdentityLookup> MessageFactoryIdentityLookupPtr;
        typedef boost::weak_ptr<MessageFactoryIdentityLookup> MessageFactoryIdentityLookupWeakPtr;

        class IdentityLookupCheckRequest;
        typedef boost::shared_ptr<IdentityLookupCheckRequest> IdentityLookupCheckRequestPtr;
        typedef boost::weak_ptr<IdentityLookupCheckRequest> IdentityLookupCheckRequestWeakPtr;
        
        class IdentityLookupCheckResult;
        typedef boost::shared_ptr<IdentityLookupCheckResult> IdentityLookupCheckResultPtr;
        typedef boost::weak_ptr<IdentityLookupCheckResult> IdentityLookupCheckResultWeakPtr;

        class IdentityLookupRequest;
        typedef boost::shared_ptr<IdentityLookupRequest> IdentityLookupRequestPtr;
        typedef boost::weak_ptr<IdentityLookupRequest> IdentityLookupRequestWeakPtr;

        class IdentityLookupResult;
        typedef boost::shared_ptr<IdentityLookupResult> IdentityLookupResultPtr;
        typedef boost::weak_ptr<IdentityLookupResult> IdentityLookupResultWeakPtr;
      }

      namespace peer
      {
        class MessageFactoryPeer;
        typedef boost::shared_ptr<MessageFactoryPeer> MessageFactoryPeerPtr;
        typedef boost::weak_ptr<MessageFactoryPeer> MessageFactoryPeerWeakPtr;

        class PeerServicesGetRequest;
        typedef boost::shared_ptr<PeerServicesGetRequest> PeerServicesGetRequestPtr;
        typedef boost::weak_ptr<PeerServicesGetRequest> PeerServicesGetRequestWeakPtr;
        
        class PeerServicesGetResult;
        typedef boost::shared_ptr<PeerServicesGetResult> PeerServicesGetResultPtr;
        typedef boost::weak_ptr<PeerServicesGetResult> PeerServicesGetResultWeakPtr;
      }

      namespace peer_common
      {
        class MessageFactoryPeerCommon;
        typedef boost::shared_ptr<MessageFactoryPeerCommon> MessageFactoryPeerCommonPtr;
        typedef boost::weak_ptr<MessageFactoryPeerCommon> MessageFactoryPeerCommonWeakPtr;

        class PeerPublishRequest;
        typedef boost::shared_ptr<PeerPublishRequest> PeerPublishRequestPtr;
        typedef boost::weak_ptr<PeerPublishRequest> PeerPublishRequestWeakPtr;

        class PeerPublishResult;
        typedef boost::shared_ptr<PeerPublishResult> PeerPublishResultPtr;
        typedef boost::weak_ptr<PeerPublishResult> PeerPublishResultWeakPtr;

        class PeerGetRequest;
        typedef boost::shared_ptr<PeerGetRequest> PeerGetRequestPtr;
        typedef boost::weak_ptr<PeerGetRequest> PeerGetRequestWeakPtr;

        class PeerGetResult;
        typedef boost::shared_ptr<PeerGetResult> PeerGetResultPtr;
        typedef boost::weak_ptr<PeerGetResult> PeerGetResultWeakPtr;

        class PeerDeleteRequest;
        typedef boost::shared_ptr<PeerDeleteRequest> PeerDeleteRequestPtr;
        typedef boost::weak_ptr<PeerDeleteRequest> PeerDeleteRequestWeakPtr;

        class PeerDeleteResult;
        typedef boost::shared_ptr<PeerDeleteResult> PeerDeleteResultPtr;
        typedef boost::weak_ptr<PeerDeleteResult> PeerDeleteResultWeakPtr;

        class PeerSubscribeRequest;
        typedef boost::shared_ptr<PeerSubscribeRequest> PeerSubscribeRequestPtr;
        typedef boost::weak_ptr<PeerSubscribeRequest> PeerSubscribeRequestWeakPtr;

        class PeerSubscribeResult;
        typedef boost::shared_ptr<PeerSubscribeResult> PeerSubscribeResultPtr;
        typedef boost::weak_ptr<PeerSubscribeResult> PeerSubscribeResultWeakPtr;

        class PeerPublishNotifyRequest;
        typedef boost::shared_ptr<PeerPublishNotifyRequest> PeerPublishNotifyRequestPtr;
        typedef boost::weak_ptr<PeerPublishNotifyRequest> PeerPublishNotifyRequestWeakPtr;

        class PeerPublishNotifyResult;
        typedef boost::shared_ptr<PeerPublishNotifyResult> PeerPublishNotifyResultPtr;
        typedef boost::weak_ptr<PeerPublishNotifyResult> PeerPublishNotifyResultWeakPtr;
      }

      namespace peer_finder
      {
        class MessageFactoryPeerFinder;
        typedef boost::shared_ptr<MessageFactoryPeerFinder> MessageFactoryPeerFinderPtr;
        typedef boost::weak_ptr<MessageFactoryPeerFinder> MessageFactoryPeerFinderWeakPtr;

        class PeerLocationFindRequest;
        typedef boost::shared_ptr<PeerLocationFindRequest> PeerLocationFindRequestPtr;
        typedef boost::weak_ptr<PeerLocationFindRequest> PeerLocationFindRequestWeakPtr;

        class PeerLocationFindResult;
        typedef boost::shared_ptr<PeerLocationFindResult> PeerLocationFindResultPtr;
        typedef boost::weak_ptr<PeerLocationFindResult> PeerLocationFindResultWeakPtr;

        class PeerLocationFindReply;
        typedef boost::shared_ptr<PeerLocationFindReply> PeerLocationFindReplyPtr;
        typedef boost::weak_ptr<PeerLocationFindReply> PeerLocationFindReplyWeakPtr;

        class SessionCreateRequest;
        typedef boost::shared_ptr<SessionCreateRequest> SessionCreateRequestPtr;
        typedef boost::weak_ptr<SessionCreateRequest> SessionCreateRequestWeakPtr;

        class SessionCreateResult;
        typedef boost::shared_ptr<SessionCreateResult> SessionCreateResultPtr;
        typedef boost::weak_ptr<SessionCreateResult> SessionCreateResultWeakPtr;

        class SessionDeleteRequest;
        typedef boost::shared_ptr<SessionDeleteRequest> SessionDeleteRequestPtr;
        typedef boost::weak_ptr<SessionDeleteRequest> SessionDeleteRequestWeakPtr;

        class SessionDeleteResult;
        typedef boost::shared_ptr<SessionDeleteResult> SessionDeleteResultPtr;
        typedef boost::weak_ptr<SessionDeleteResult> SessionDeleteResultWeakPtr;

        class SessionKeepAliveRequest;
        typedef boost::shared_ptr<SessionKeepAliveRequest> SessionKeepAliveRequestPtr;
        typedef boost::weak_ptr<SessionKeepAliveRequest> SessionKeepAliveRequestWeakPtr;

        class SessionKeepAliveResult;
        typedef boost::shared_ptr<SessionKeepAliveResult> SessionKeepAliveResultPtr;
        typedef boost::weak_ptr<SessionKeepAliveResult> SessionKeepAliveResultWeakPtr;
      }

      namespace peer_salt
      {
        typedef ElementPtr SignedSaltElementPtr;
        typedef std::list<SignedSaltElementPtr> SaltBundleList;

        class MessageFactoryPeerSalt;
        typedef boost::shared_ptr<MessageFactoryPeerSalt> MessageFactoryPeerSaltPtr;
        typedef boost::weak_ptr<MessageFactoryPeerSalt> MessageFactoryPeerSaltWeakPtr;

        class SignedSaltGetRequest;
        typedef boost::shared_ptr<SignedSaltGetRequest> SignedSaltGetRequestPtr;
        typedef boost::weak_ptr<SignedSaltGetRequest> SignedSaltGetRequestWeakPtr;

        class SignedSaltGetResult;
        typedef boost::shared_ptr<SignedSaltGetResult> SignedSaltGetResultPtr;
        typedef boost::weak_ptr<SignedSaltGetResult> SignedSaltGetResultWeakPtr;
      }

      namespace peer_to_peer
      {
        class MessageFactoryPeerToPeer;
        typedef boost::shared_ptr<MessageFactoryPeerToPeer> MessageFactoryPeerToPeerPtr;
        typedef boost::weak_ptr<MessageFactoryPeerToPeer> MessageFactoryPeerToPeerWeakPtr;

        class PeerIdentifyRequest;
        typedef boost::shared_ptr<PeerIdentifyRequest> PeerIdentifyRequestPtr;
        typedef boost::weak_ptr<PeerIdentifyRequest> PeerIdentifyRequestWeakPtr;

        class PeerIdentifyResult;
        typedef boost::shared_ptr<PeerIdentifyResult> PeerIdentifyResultPtr;
        typedef boost::weak_ptr<PeerIdentifyResult> PeerIdentifyResultWeakPtr;

        class PeerKeepAliveRequest;
        typedef boost::shared_ptr<PeerKeepAliveRequest> PeerKeepAliveRequestPtr;
        typedef boost::weak_ptr<PeerKeepAliveRequest> PeerKeepAliveRequestWeakPtr;

        class PeerKeepAliveResult;
        typedef boost::shared_ptr<PeerKeepAliveResult> PeerKeepAliveResultPtr;
        typedef boost::weak_ptr<PeerKeepAliveResult> PeerKeepAliveResultWeakPtr;
      }
    }
  }
}
