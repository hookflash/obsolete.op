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

#include <openpeer/stack/types.h>
#include <openpeer/stack/message/types.h>

#include <zsLib/types.h>
#include <zsLib/Proxy.h>

#include <boost/shared_array.hpp>

#include <list>

namespace openpeer
{
  namespace core
  {
    using zsLib::PUID;
    using zsLib::WORD;
    using zsLib::ULONG;
    using zsLib::Time;
    using zsLib::String;
    using zsLib::PTRNUMBER;
    typedef PTRNUMBER SubsystemID;

    using zsLib::XML::Element;
    using zsLib::XML::ElementPtr;

    using openpeer::stack::SecureByteBlock;
    using openpeer::stack::SecureByteBlockPtr;
    using openpeer::stack::message::IMessageHelper;

    using openpeer::stack::IPeerFilePublic;
    using openpeer::stack::IPeerFilePublicPtr;

    // other types
    struct ContactProfileInfo;
    struct IdentityContact;
    struct RolodexContact;

    interaction IContact;
    typedef boost::shared_ptr<IContact> IContactPtr;
    typedef boost::weak_ptr<IContact> IContactWeakPtr;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ContactProfileInfo
    #pragma mark

    struct ContactProfileInfo
    {
      IContactPtr mContact;
      ElementPtr mProfileBundleEl;

      bool hasData() const;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark RolodexContact
    #pragma mark

    struct RolodexContact
    {
      struct Avatar
      {
        String mName;
        String mURL;
        int mWidth;
        int mHeight;
      };
      typedef std::list<Avatar> AvatarList;

      enum Dispositions
      {
        Disposition_NA,
        Disposition_Update,
        Disposition_Remove,
      };

      Dispositions mDisposition;
      String mIdentityURI;
      String mIdentityProvider;

      String mName;
      String mProfileURL;
      String mVProfileURL;

      AvatarList mAvatars;

      RolodexContact();
      bool hasData() const;
    };
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IdentityContact
    #pragma mark

    struct IdentityContact : public RolodexContact
    {
      String mStableID;

      IPeerFilePublicPtr mPeerFilePublic;
      ElementPtr mIdentityProofBundleEl;

      WORD mPriority;
      WORD mWeight;

      Time mLastUpdated;
      Time mExpires;

      IdentityContact();
      IdentityContact(const RolodexContact &);
      bool hasData() const;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark (other)
    #pragma mark

    interaction IAccount;
    typedef boost::shared_ptr<IAccount> IAccountPtr;
    typedef boost::weak_ptr<IAccount> IAccountWeakPtr;

    interaction IAccountDelegate;
    typedef boost::shared_ptr<IAccountDelegate> IAccountDelegatePtr;
    typedef boost::weak_ptr<IAccountDelegate> IAccountDelegateWeakPtr;
    typedef zsLib::Proxy<IAccountDelegate> IAccountDelegateProxy;

    interaction ICache;
    typedef boost::shared_ptr<ICache> ICachePtr;
    typedef boost::weak_ptr<ICache> ICacheWeakPtr;

    interaction ICacheDelegate;
    typedef boost::shared_ptr<ICacheDelegate> ICacheDelegatePtr;
    typedef boost::weak_ptr<ICacheDelegate> ICacheDelegateWeakPtr;

    interaction ICall;
    typedef boost::shared_ptr<ICall> ICallPtr;
    typedef boost::weak_ptr<ICall> ICallWeakPtr;

    interaction ICallDelegate;
    typedef boost::shared_ptr<ICallDelegate> ICallDelegatePtr;
    typedef boost::weak_ptr<ICallDelegate> ICallDelegateWeakPtr;
    typedef zsLib::Proxy<ICallDelegate> ICallDelegateProxy;

    interaction ICacheDelegate;
    typedef boost::shared_ptr<ICallDelegate> ICallDelegatePtr;
    typedef boost::weak_ptr<ICallDelegate> ICallDelegateWeakPtr;

    interaction ILoggerDelegate;
    typedef boost::shared_ptr<ILoggerDelegate> ILoggerDelegatePtr;
    typedef boost::weak_ptr<ILoggerDelegate> ILoggerDelegateWeakPtr;

    interaction IConversationThread;
    typedef boost::shared_ptr<IConversationThread> IConversationThreadPtr;
    typedef boost::weak_ptr<IConversationThread> IConversationThreadWeakPtr;

    interaction IConversationThreadDelegate;
    typedef boost::shared_ptr<IConversationThreadDelegate> IConversationThreadDelegatePtr;
    typedef boost::weak_ptr<IConversationThreadDelegate> IConversationThreadDelegateWeakPtr;
    typedef zsLib::Proxy<IConversationThreadDelegate> IConversationThreadDelegateProxy;

    interaction IContactPeerFilePublicLookup;
    typedef boost::shared_ptr<IContactPeerFilePublicLookup> IContactPeerFilePublicLookupPtr;
    typedef boost::weak_ptr<IContactPeerFilePublicLookup> IContactPeerFilePublicLookupWeakPtr;

    interaction IContactPeerFilePublicLookupDelegate;
    typedef boost::shared_ptr<IContactPeerFilePublicLookupDelegate> IContactPeerFilePublicLookupDelegatePtr;
    typedef boost::weak_ptr<IContactPeerFilePublicLookupDelegate> IContactPeerFilePublicLookupDelegateWeakPtr;
    typedef zsLib::Proxy<IContactPeerFilePublicLookupDelegate> IContactPeerFilePublicLookupDelegateProxy;

    interaction IIdentity;
    typedef boost::shared_ptr<IIdentity> IIdentityPtr;
    typedef boost::weak_ptr<IIdentity> IIdentityWeakPtr;

    interaction IIdentityDelegate;
    typedef boost::shared_ptr<IIdentityDelegate> IIdentityDelegatePtr;
    typedef boost::weak_ptr<IIdentityDelegate> IIdentityDelegateWeakPtr;
    typedef zsLib::Proxy<IIdentityDelegate> IIdentityDelegateProxy;

    interaction IIdentityLookup;
    typedef boost::shared_ptr<IIdentityLookup> IIdentityLookupPtr;
    typedef boost::weak_ptr<IIdentityLookup> IIdentityLookupWeakPtr;

    interaction IIdentityLookupDelegate;
    typedef boost::shared_ptr<IIdentityLookupDelegate> IIdentityLookupDelegatePtr;
    typedef boost::weak_ptr<IIdentityLookupDelegate> IIdentityLookupDelegateWeakPtr;
    typedef zsLib::Proxy<IIdentityLookupDelegate> IIdentityLookupDelegateProxy;

    interaction IMediaEngine;
    typedef boost::shared_ptr<IMediaEngine> IMediaEnginePtr;
    typedef boost::weak_ptr<IMediaEngine> IMediaEngineWeakPtr;

    interaction IMediaEngineDelegate;
    typedef boost::shared_ptr<IMediaEngineDelegate> IMediaEngineDelegatePtr;
    typedef boost::weak_ptr<IMediaEngineDelegate> IMediaEngineDelegateWeakPtr;
    typedef zsLib::Proxy<IMediaEngineDelegate> IMediaEngineDelegateProxy;
    
    interaction IMediaEngineObsolete;
    typedef boost::shared_ptr<IMediaEngineObsolete> IMediaEngineObsoletePtr;
    typedef boost::weak_ptr<IMediaEngineObsolete> IMediaEngineObsoleteWeakPtr;
    
    interaction IMediaEngineDelegateObsolete;
    typedef boost::shared_ptr<IMediaEngineDelegateObsolete> IMediaEngineDelegateObsoletePtr;
    typedef boost::weak_ptr<IMediaEngineDelegateObsolete> IMediaEngineDelegateObsoleteWeakPtr;
    typedef zsLib::Proxy<IMediaEngineDelegateObsolete> IMediaEngineDelegateObsoleteProxy;
    
    interaction IMediaSession;
    typedef boost::shared_ptr<IMediaSession> IMediaSessionPtr;
    typedef boost::weak_ptr<IMediaSession> IMediaSessionWeakPtr;
    
    interaction IMediaSessionDelegate;
    typedef boost::shared_ptr<IMediaSessionDelegate> IMediaSessionDelegatePtr;
    typedef boost::weak_ptr<IMediaSessionDelegate> IMediaSessionDelegateWeakPtr;
    typedef zsLib::Proxy<IMediaSessionDelegate> IMediaSessionelegateProxy;
    
    typedef std::list<IMediaSessionPtr> MediaSessionList;
    typedef boost::shared_ptr<MediaSessionList> MediaSessionListPtr;
    typedef boost::weak_ptr<MediaSessionList> MediaSessionListWeakPtr;

    interaction IMediaStream;
    typedef boost::shared_ptr<IMediaStream> IMediaStreamPtr;
    typedef boost::weak_ptr<IMediaStream> IMediaStreamWeakPtr;
    
    interaction ILocalSendAudioStream;
    typedef boost::shared_ptr<ILocalSendAudioStream> ILocalSendAudioStreamPtr;
    typedef boost::weak_ptr<ILocalSendAudioStream> ILocalSendAudioStreamWeakPtr;
    
    interaction IRemoteReceiveAudioStream;
    typedef boost::shared_ptr<IRemoteReceiveAudioStream> IRemoteReceiveAudioStreamPtr;
    typedef boost::weak_ptr<IRemoteReceiveAudioStream> IRemoteReceiveAudioStreamWeakPtr;
    
    interaction IRemoteSendAudioStream;
    typedef boost::shared_ptr<IRemoteSendAudioStream> IRemoteSendAudioStreamPtr;
    typedef boost::weak_ptr<IRemoteSendAudioStream> IRemoteSendAudioStreamWeakPtr;
    
    interaction ILocalSendVideoStream;
    typedef boost::shared_ptr<ILocalSendVideoStream> ILocalSendVideoStreamPtr;
    typedef boost::weak_ptr<ILocalSendVideoStream> ILocalSendVideoStreamWeakPtr;
    
    interaction IRemoteReceiveVideoStream;
    typedef boost::shared_ptr<IRemoteReceiveVideoStream> IRemoteReceiveVideoStreamPtr;
    typedef boost::weak_ptr<IRemoteReceiveVideoStream> IRemoteReceiveVideoStreamWeakPtr;
    
    interaction IRemoteSendVideoStream;
    typedef boost::shared_ptr<IRemoteSendVideoStream> IRemoteSendVideoStreamPtr;
    typedef boost::weak_ptr<IRemoteSendVideoStream> IRemoteSendVideoStreamWeakPtr;

    interaction IMediaStreamDelegate;
    typedef boost::shared_ptr<IMediaStreamDelegate> IMediaStreamDelegatePtr;
    typedef boost::weak_ptr<IMediaStreamDelegate> IMediaStreamDelegateWeakPtr;
    typedef zsLib::Proxy<IMediaStreamDelegate> IMediaStreamDelegateProxy;
    
    typedef std::list<IMediaStreamPtr> MediaStreamList;
    typedef boost::shared_ptr<MediaStreamList> MediaStreamListPtr;
    typedef boost::weak_ptr<MediaStreamList> MediaStreamListWeakPtr;

    interaction IStack;
    typedef boost::shared_ptr<IStack> IStackPtr;
    typedef boost::weak_ptr<IStack> IStackWeakPtr;

    interaction IStackDelegate;
    typedef boost::shared_ptr<IStackDelegate> IStackDelegatePtr;
    typedef boost::weak_ptr<IStackDelegate> IStackDelegateWeakPtr;
    typedef zsLib::Proxy<IStackDelegate> IStackDelegateProxy;

    interaction IStackAutoCleanup;
    typedef boost::shared_ptr<IStackAutoCleanup> IStackAutoCleanupPtr;
    typedef boost::weak_ptr<IStackAutoCleanup> IStackAutoCleanupWeakPtr;

    interaction IStackMessageQueue;
    typedef boost::shared_ptr<IStackMessageQueue> IStackMessageQueuePtr;
    typedef boost::weak_ptr<IStackMessageQueue> IStackMessageQueueWeakPtr;

    interaction IStackMessageQueueDelegate;
    typedef boost::shared_ptr<IStackMessageQueueDelegate> IStackMessageQueueDelegatePtr;
    typedef boost::weak_ptr<IStackMessageQueueDelegate> IStackMessageQueueDelegateWeakPtr;
    
    typedef std::list<IContactPtr> ContactList;
    typedef boost::shared_ptr<ContactList> ContactListPtr;
    typedef boost::weak_ptr<ContactList> ContactListWeakPtr;

    typedef std::list<ContactProfileInfo> ContactProfileInfoList;
    typedef boost::shared_ptr<ContactProfileInfoList> ContactProfileInfoListPtr;
    typedef boost::weak_ptr<ContactProfileInfoList> ContactProfileInfoListWeakPtr;

    typedef std::list<IConversationThreadPtr> ConversationThreadList;
    typedef boost::shared_ptr<ConversationThreadList> ConversationThreadListPtr;
    typedef boost::weak_ptr<ConversationThreadList> ConversationThreadListWeakPtr;

    typedef std::list<IIdentityPtr> IdentityList;
    typedef boost::shared_ptr<IdentityList> IdentityListPtr;
    typedef boost::weak_ptr<IdentityList> IdentityListWeakPtr;

    typedef std::list<IdentityContact> IdentityContactList;
    typedef boost::shared_ptr<IdentityContactList> IdentityContactListPtr;
    typedef boost::weak_ptr<IdentityContactList> IdentityContactListWeakPtr;

    typedef std::list<RolodexContact> RolodexContactList;
    typedef boost::shared_ptr<RolodexContactList> RolodexContactListPtr;
    typedef boost::weak_ptr<RolodexContactList> RolodexContactListWeakPtr;
  }
}
