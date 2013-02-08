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
        String mURIEncrypted;
        String mHash;
        String mProvider;

        String mContactUserID;
        String mContact;
        String mContactFindSecret;
        String mPrivatePeerFileSalt;
        String mPrivatePeerFileSecretEncrypted;

        Time mLastReset;
        String mReloginAccessKey;
        String mReloginAccessKeyEncrypted;

        String mSecret;
        String mSecretSalt;
        String mSecretEncrypted;
        String mSecretDecryptionKeyEncrypted;

        WORD mPriority;
        WORD mWeight;

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

        class IdentityLoginStartRequest;
        typedef boost::shared_ptr<IdentityLoginStartRequest> IdentityLoginStartRequestPtr;
        typedef boost::weak_ptr<IdentityLoginStartRequest> IdentityLoginStartRequestWeakPtr;

        class IdentityLoginStartResult;
        typedef boost::shared_ptr<IdentityLoginStartResult> IdentityLoginStartResultPtr;
        typedef boost::weak_ptr<IdentityLoginStartResult> IdentityLoginStartResultWeakPtr;

        class IdentityLoginNotify;
        typedef boost::shared_ptr<IdentityLoginNotify> IdentityLoginNotifyPtr;
        typedef boost::weak_ptr<IdentityLoginNotify> IdentityLoginNotifyWeakPtr;

        class IdentityLoginBrowserWindowControlNotify;
        typedef boost::shared_ptr<IdentityLoginBrowserWindowControlNotify> IdentityLoginBrowserWindowControlNotifyPtr;
        typedef boost::weak_ptr<IdentityLoginBrowserWindowControlNotify> IdentityLoginBrowserWindowControlNotifyWeakPtr;

        class IdentityLoginCompleteRequest;
        typedef boost::shared_ptr<IdentityLoginCompleteRequest> IdentityLoginCompleteRequestPtr;
        typedef boost::weak_ptr<IdentityLoginCompleteRequest> IdentityLoginCompleteRequestWeakPtr;

        class IdentityLoginCompleteResult;
        typedef boost::shared_ptr<IdentityLoginCompleteResult> IdentityLoginCompleteResultPtr;
        typedef boost::weak_ptr<IdentityLoginCompleteResult> IdentityLoginCompleteResultWeakPtr;

        class IdentityAssociateRequest;
        typedef boost::shared_ptr<IdentityAssociateRequest> IdentityAssociateRequestPtr;
        typedef boost::weak_ptr<IdentityAssociateRequest> IdentityAssociateRequestWeakPtr;

        class IdentityAssociateResult;
        typedef boost::shared_ptr<IdentityAssociateResult> IdentityAssociateResultPtr;
        typedef boost::weak_ptr<IdentityAssociateResult> IdentityAssociateResultWeakPtr;

        class IdentitySignRequest;
        typedef boost::shared_ptr<IdentitySignRequest> IdentitySignRequestPtr;
        typedef boost::weak_ptr<IdentitySignRequest> IdentitySignRequestWeakPtr;

        class IdentitySignResult;
        typedef boost::shared_ptr<IdentitySignResult> IdentitySignResultPtr;
        typedef boost::weak_ptr<IdentitySignResult> IdentitySignResultWeakPtr;
      }

      namespace identity_lookup
      {
        class MessageFactoryIdentityLookup;
        typedef boost::shared_ptr<MessageFactoryIdentityLookup> MessageFactoryIdentityLookupPtr;
        typedef boost::weak_ptr<MessageFactoryIdentityLookup> MessageFactoryIdentityLookupWeakPtr;

        class IdentityLookupRequest;
        typedef boost::shared_ptr<IdentityLookupRequest> IdentityLookupRequestPtr;
        typedef boost::weak_ptr<IdentityLookupRequest> IdentityLookupRequestWeakPtr;

        class IdentityLookupResult;
        typedef boost::shared_ptr<IdentityLookupResult> IdentityLookupResultPtr;
        typedef boost::weak_ptr<IdentityLookupResult> IdentityLookupResultWeakPtr;
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

      namespace peer_contact
      {
        class MessageFactoryPeerContact;
        typedef boost::shared_ptr<MessageFactoryPeerContact> MessageFactoryPeerContactPtr;
        typedef boost::weak_ptr<MessageFactoryPeerContact> MessageFactoryPeerContactWeakPtr;

        class PublicPeerFilesGetRequest;
        typedef boost::shared_ptr<PublicPeerFilesGetRequest> PublicPeerFilesGetRequestPtr;
        typedef boost::weak_ptr<PublicPeerFilesGetRequest> PublicPeerFilesGetRequestWeakPtr;

        class PublicPeerFilesGetResult;
        typedef boost::shared_ptr<PublicPeerFilesGetResult> PublicPeerFilesGetResultPtr;
        typedef boost::weak_ptr<PublicPeerFilesGetResult> PublicPeerFilesGetResultWeakPtr;

        class PeerContactLoginRequest;
        typedef boost::shared_ptr<PeerContactLoginRequest> PeerContactLoginRequestPtr;
        typedef boost::weak_ptr<PeerContactLoginRequest> PeerContactLoginRequestWeakPtr;

        class PeerContactLoginResult;
        typedef boost::shared_ptr<PeerContactLoginResult> PeerContactLoginResultPtr;
        typedef boost::weak_ptr<PeerContactLoginResult> PeerContactLoginResultWeakPtr;

        class PrivatePeerFileGetRequest;
        typedef boost::shared_ptr<PrivatePeerFileGetRequest> PrivatePeerFileGetRequestPtr;
        typedef boost::weak_ptr<PrivatePeerFileGetRequest> PrivatePeerFileGetRequestWeakPtr;

        class PrivatePeerFileGetResult;
        typedef boost::shared_ptr<PrivatePeerFileGetResult> PrivatePeerFileGetResultPtr;
        typedef boost::weak_ptr<PrivatePeerFileGetResult> PrivatePeerFileGetResultWeakPtr;

        class PrivatePeerFileSetRequest;
        typedef boost::shared_ptr<PrivatePeerFileSetRequest> PrivatePeerFileSetRequestPtr;
        typedef boost::weak_ptr<PrivatePeerFileSetRequest> PrivatePeerFileSetRequestWeakPtr;

        class PrivatePeerFileSetResult;
        typedef boost::shared_ptr<PrivatePeerFileSetResult> PrivatePeerFileSetResultPtr;
        typedef boost::weak_ptr<PrivatePeerFileSetResult> PrivatePeerFileSetResultWeakPtr;

        class PeerContactIdentityAssociateRequest;
        typedef boost::shared_ptr<PeerContactIdentityAssociateRequest> PeerContactIdentityAssociateRequestPtr;
        typedef boost::weak_ptr<PeerContactIdentityAssociateRequest> PeerContactIdentityAssociateRequestWeakPtr;

        class PeerContactIdentityAssociateResult;
        typedef boost::shared_ptr<PeerContactIdentityAssociateResult> PeerContactIdentityAssociateResultPtr;
        typedef boost::weak_ptr<PeerContactIdentityAssociateResult> PeerContactIdentityAssociateResultWeakPtr;

        class PeerContactServicesGetRequest;
        typedef boost::shared_ptr<PeerContactServicesGetRequest> PeerContactServicesGetRequestPtr;
        typedef boost::weak_ptr<PeerContactServicesGetRequest> PeerContactServicesGetRequestWeakPtr;

        class PeerContactServicesGetResult;
        typedef boost::shared_ptr<PeerContactServicesGetResult> PeerContactServicesGetResultPtr;
        typedef boost::weak_ptr<PeerContactServicesGetResult> PeerContactServicesGetResultWeakPtr;
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
