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

#include <hookflash/provisioning/internal/hookflashTypes.h>
#include <hookflash/internal/hookflashTypes.h>
#include <hookflash/provisioning/message/hookflashTypes.h>
#include <hookflash/provisioning/IAccount.h>
#include <hookflash/IAccount.h>
#include <hookflash/services/IHTTP.h>
#include <zsLib/Proxy.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace internal
    {
      interaction IAccountAsync;
      typedef boost::shared_ptr<IAccountAsync> IAccountAsyncPtr;
      typedef boost::weak_ptr<IAccountAsync> IAccountAsyncWeakPtr;
      typedef zsLib::Proxy<IAccountAsync> IAccountAsyncProxy;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::IAccountAsync
      #pragma mark

      interaction IAccountAsync
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account
      #pragma mark

      class Account : public IAccount,
                      public zsLib::MessageQueueAssociator,
                      public hookflash::services::IHTTPQueryDelegate,
                      public hookflash::IAccountDelegate,
                      public hookflash::provisioning::IAccountIdentityLookupQueryDelegate,
                      public IAccountAsync
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef hookflash::internal::StackPtr StackPtr;
        typedef provisioning::IAccount::AccountStates AccountStates;
        typedef services::IHTTP IHTTP;
        typedef services::IHTTPQueryPtr IHTTPQueryPtr;

        class AccountOAuthIdentityAssociation;
        typedef boost::shared_ptr<AccountOAuthIdentityAssociation> AccountOAuthIdentityAssociationPtr;
        typedef boost::weak_ptr<AccountOAuthIdentityAssociation> AccountOAuthIdentityAssociationWeakPtr;
        friend class AccountOAuthIdentityAssociation;

        class AccountPush;
        typedef boost::shared_ptr<AccountPush> AccountPushPtr;
        typedef boost::weak_ptr<AccountPush> AccountPushWeakPtr;
        friend class AccountPush;

        class AccountIdentityLookupQuery;
        typedef boost::shared_ptr<AccountIdentityLookupQuery> AccountIdentityLookupQueryPtr;
        typedef boost::weak_ptr<AccountIdentityLookupQuery> AccountIdentityLookupQueryWeakPtr;
        friend class AccountIdentityLookupQuery;

        class AccountPeerFileLookupQuery;
        typedef boost::shared_ptr<AccountPeerFileLookupQuery> AccountPeerFileLookupQueryPtr;
        typedef boost::weak_ptr<AccountPeerFileLookupQuery> AccountPeerFileLookupQueryWeakPtr;
        friend class AccountPeerFileLookupQuery;

        enum LoginScenarios
        {
          LoginScenario_FirstTimeLogin,
          LoginScenario_FirstTimeOAuthLogin,
          LoginScenario_Relogin,
        };
        
        static const char *toString(LoginScenarios scenario);

        enum HTTPQueryTypes
        {
          HTTPQueryType_LookupProfile,
          HTTPQueryType_CreateAccount,
          HTTPQueryType_AccessAccount,
          HTTPQueryType_ProfileGet,
          HTTPQueryType_ProfilePut,
          HTTPQueryType_SendIDValidationPIN,
          HTTPQueryType_ValidateIDPIN,
          HTTPQueryType_ProviderLoginURLGet,
          HTTPQueryType_ProviderAssociateURLGet,
          HTTPQueryType_PeerProfileLookup,
          HTTPQueryType_MultiPartyAPNSPush,
          HTTPQueryType_OAuthPasswordGetPart1,
          HTTPQueryType_OAuthPasswordGetPart2,
          HTTPQueryType_PasswordGetPart1,
          HTTPQueryType_PasswordGetPart2,
          HTTPQueryType_PasswordPINGet1,
          HTTPQueryType_PasswordPINGet2,
          HTTPQueryType_PasswordPutPart1,
          HTTPQueryType_PasswordPutPart2,
        };

        static const char *toString(HTTPQueryTypes queryType);

      protected:
        Account(
                IMessageQueuePtr queue,
                IStackPtr stack,
                LoginScenarios scenario,
                provisioning::IAccountDelegatePtr provisioningDelegate,
                hookflash::IAccountDelegatePtr openpeerDelegate,
                const char *provisioningURI,
                const char *deviceToken
                );

        void init();

      public:
        ~Account();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => IAccount
        #pragma mark

        static AccountPtr firstTimeLogin(
                                         IStackPtr stack,
                                         provisioning::IAccountDelegatePtr provisioningDelegate,
                                         hookflash::IAccountDelegatePtr openpeerDelegate,
                                         const char *provisioningURI,
                                         const char *deviceToken,
                                         const char *name,
                                         const IdentityInfoList &knownIdentities
                                         );

        static AccountPtr firstTimeOAuthLogin(
                                              IStackPtr stack,
                                              provisioning::IAccountDelegatePtr provisioningDelegate,
                                              hookflash::IAccountDelegatePtr openpeerDelegate,
                                              const char *provisioningURI,
                                              const char *deviceToken,
                                              IdentityTypes oauthIdentityType
                                              );

        static AccountPtr relogin(
                                  IStackPtr stack,
                                  provisioning::IAccountDelegatePtr provisioningDelegate,
                                  hookflash::IAccountDelegatePtr openpeerDelegate,
                                  const char *provisioningURI,
                                  const char *deviceToken,
                                  const char *userID,
                                  const char *accountSalt,
                                  const char *passwordNonce,
                                  const char *password,
                                  ElementPtr privatePeerFile,
                                  Time lastProfileUpdatedTimestamp,
                                  const IdentityInfoList &previousIdentities
                                  );

        virtual void shutdown();

        virtual IdentityID getAuthorizationPINIdentity() const;
        virtual void setAuthorizationPIN(const char *authorizationPIN);

        virtual String getOAuthLoginURL() const;
        virtual void completeOAuthLoginProcess(ElementPtr xmlResultFromJavascript);

        virtual String getUserID() const;
        virtual String getAccountSalt() const;
        virtual String getPasswordNonce() const;
        virtual String getPassword() const;
        virtual ElementPtr getPrivatePeerFile() const;
        virtual Time getLastProfileUpdatedTime() const;

        virtual hookflash::IAccountPtr getOpenPeerAccount() const;

        virtual AccountStates getState() const;
        virtual AccountErrorCodes getLastError() const;

        virtual void getIdentities(IdentityInfoList &outIdentities) const;
        virtual void setIdentities(const IdentityInfoList &identities);

        virtual IdentityValidationStates getIdentityValidationState(const IdentityID &identity) const;
        virtual void validateIdentitySendPIN(const IdentityID &identity);
        virtual void validateIdentityComplete(
                                              const IdentityID &identity,
                                              const char *identityPIN
                                              );

        virtual IAccountOAuthIdentityAssociationPtr associateOAuthIdentity(
                                                                           IdentityTypes type,
                                                                           IAccountOAuthIdentityAssociationDelegatePtr delegate
                                                                           );

        virtual IAccountPushPtr apnsPush(
                                         IAccountPushDelegatePtr delegate,
                                         const UserIDList &userIDs,
                                         const char *messageType,
                                         const char *message
                                         );

        virtual IAccountIdentityLookupQueryPtr lookup(
                                                      IAccountIdentityLookupQueryDelegatePtr delegate,
                                                      const IdentityIDList &identities
                                                      );

        virtual IAccountPeerFileLookupQueryPtr lookup(
                                                      IAccountPeerFileLookupQueryDelegatePtr delegate,
                                                      const UserIDList &userIDs,
                                                      const ContactIDList &associatedContactIDs
                                                      );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => hookflash::IAccountDelegate
        #pragma mark

        virtual void onAccountStateChanged(
                                           hookflash::IAccountPtr account,
                                           hookflash::IAccount::AccountStates state
                                           );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => IHTTPQueryDelegate
        #pragma mark

        virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
        virtual void onHTTPComplete(IHTTPQueryPtr query);
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => IAccountIdentityLookupQueryDelegate
        #pragma mark

        virtual void onAccountIdentityLookupQueryComplete(IAccountIdentityLookupQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => IAccountAsync
        #pragma mark

        virtual void onStep();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => friend AccountOAuthIdentityAssociation
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) StackPtr getStack() const {return mStack;}
        // (duplicate) String getURL(HTTPQueryTypes queryType) const;

        // (duplicate) virtual String getUserID() const;
        String getAccessKey() const;
        String getAccessSecret() const;

        void notifyWaitingForProfileGet(AccountOAuthIdentityAssociationPtr association);
        void notifyComplete(AccountOAuthIdentityAssociationPtr association);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => friend AccountPush
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) StackPtr getStack() const {return mStack;}
        // (duplicate) String getURL(HTTPQueryTypes queryType) const;

        void notifyComplete(AccountPushPtr apnsPush);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => friend AccountIdentityLookupQuery
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) StackPtr getStack() const {return mStack;}
        // (duplicate) String getURL(HTTPQueryTypes queryType) const;

        // (duplicate) virtual String getUserID() const;
        // (duplicate) String getAccessKey() const;
        // (duplicate) String getAccessSecret() const;
        String getLocationID() const;

        void notifyComplete(AccountIdentityLookupQueryPtr lookup);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => friend AccountPeerFileLookupQuery
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) StackPtr getStack() const {return mStack;}
        // (duplicate) String getURL(HTTPQueryTypes queryType) const;

        void notifyComplete(AccountPeerFileLookupQueryPtr lookup);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => (internal)
        #pragma mark

        bool isShuttingDown() const {return IAccount::AccountState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return IAccount::AccountState_Shutdown == mCurrentState;}

        String log(const char *message) const;

        RecursiveLock &getLock() const {return mLock;}
        StackPtr getStack() const {return mStack;}

        void cancel();
        bool stepFirstTimeLogin();
        bool stepFirstTimeOAuthLogin();
        bool stepRelogin();
        void stepProfileGetSet();
        void step();
        void fixIdentityProof();
        void fixIdentityProof(IdentityInfoList &infoList);
        void fixIdentitiesUpdated();

        void setCurrentState(AccountStates state);
        void setLastError(AccountErrorCodes error);

        String getURL(HTTPQueryTypes queryType) const;

      public:

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account::AccountOAuthIdentityAssociation
        #pragma mark

        class AccountOAuthIdentityAssociation : public zsLib::MessageQueueAssociator,
                                                public hookflash::provisioning::IAccountOAuthIdentityAssociation,
                                                public hookflash::services::IHTTPQueryDelegate
        {
        public:
          friend class Account;

        protected:
          AccountOAuthIdentityAssociation(
                                          IMessageQueuePtr queue,
                                          AccountPtr outer,
                                          IAccountOAuthIdentityAssociationDelegatePtr delegate,
                                          IdentityTypes type
                                          );

          void init();

        public:
          ~AccountOAuthIdentityAssociation();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => IAccountOAuthIdentityAssociation
          #pragma mark

          virtual bool isComplete() const;
          virtual bool didSucceed() const;
          virtual void cancel();

          virtual String getOAuthLoginURL() const;

          virtual IdentityTypes getProviderType() const;
          
          virtual String getProviderUniqueID() const;
          virtual String getProviderOAuthAccessToken() const;
          virtual String getProviderEncryptedOAuthAccessSecret() const;

          virtual void completeOAuthLoginProcess(ElementPtr xmlResultFromJavascript);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => IHTTPQueryDelegate
          #pragma mark

          virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
          virtual void onHTTPComplete(IHTTPQueryPtr query);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => friend Account
          #pragma mark

          static AccountOAuthIdentityAssociationPtr associate(
                                                              AccountPtr outer,
                                                              IAccountOAuthIdentityAssociationDelegatePtr delegate,
                                                              IdentityTypes type
                                                              );

          PUID getID() const {return mID;}

          // (duplicate) virtual void cancel();

          // (duplicate) virtual IdentityTypes getProviderType() const;
          // (duplicate) virtual String getProviderUniqueID() const;
          
          Time getProfileUpdatedTimestamp() const;
          void notifyProfileGetComplete(const Time &updated);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          AccountOAuthIdentityAssociationWeakPtr mThisWeak;
          AccountWeakPtr mOuter;

          IAccountOAuthIdentityAssociationDelegatePtr mDelegate;

          IHTTPQueryPtr mQuery;
          bool mSucceeded;

          IdentityTypes mType;

          String mOAuthLoginURL;

          String mProviderUniqueID;
          String mProviderOAuthAccessToken;
          String mProviderEncryptedOAuthAccessSecret;
          Time mLastProfileUpdateTimestamp;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account::AccountPush
        #pragma mark

        class AccountPush : public zsLib::MessageQueueAssociator,
                            public hookflash::provisioning::IAccountPush,
                            public hookflash::services::IHTTPQueryDelegate
        {
        public:
          friend class Account;

        protected:
          AccountPush(
                      IMessageQueuePtr queue,
                      AccountPtr outer,
                      IAccountPushDelegatePtr delegate
                      );

          void init(
                    const UserIDList &userIDs,
                    const char *messageType,
                    const char *message
                    );

        public:
          ~AccountPush();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPush => IAccountPush
          #pragma mark

          virtual bool isComplete() const;
          virtual bool didSucceed() const;

          virtual void cancel();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPush => IHTTPQueryDelegate
          #pragma mark

          virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
          virtual void onHTTPComplete(IHTTPQueryPtr query);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPush => friend Account
          #pragma mark

          static AccountPushPtr push(
                                     AccountPtr outer,
                                     IAccountPushDelegatePtr delegate,
                                     const UserIDList &userIDs,
                                     const char *messageType,
                                     const char *message
                                     );

          PUID getID() const {return mID;}

          // (duplicate) virtual void cancel();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPush => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPush => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          AccountPushWeakPtr mThisWeak;
          AccountWeakPtr mOuter;

          IAccountPushDelegatePtr mDelegate;

          IHTTPQueryPtr mQuery;
          bool mSucceeded;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account::AccountIdentityLookupQuery
        #pragma mark

        class AccountIdentityLookupQuery : public zsLib::MessageQueueAssociator,
                                           public hookflash::provisioning::IAccountIdentityLookupQuery,
                                           public hookflash::services::IHTTPQueryDelegate
        {
        public:
          typedef IAccount::LookupProfileInfo LookupProfileInfo;

          friend class Account;

        protected:
          AccountIdentityLookupQuery(
                                     IMessageQueuePtr queue,
                                     AccountPtr outer,
                                     IAccountIdentityLookupQueryDelegatePtr delegate,
                                     const IdentityIDList &identities
                                     );

          void init();

        public:
          ~AccountIdentityLookupQuery();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountIdentityLookupQuery => IAccountIdentityLookupQuery
          #pragma mark

          virtual bool isComplete() const;
          virtual bool didSucceed() const;

          virtual void cancel();

          virtual void getIdentities(IdentityIDList &outIdentities) const;
          virtual bool getLookupProfile(
                                        const IdentityID &inIdentity,
                                        LookupProfileInfo &outInfo
                                        ) const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountIdentityLookupQuery => IHTTPQueryDelegate
          #pragma mark

          virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
          virtual void onHTTPComplete(IHTTPQueryPtr query);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountIdentityLookupQuery => friend Account
          #pragma mark

          static AccountIdentityLookupQueryPtr lookup(
                                                      AccountPtr outer,
                                                      IAccountIdentityLookupQueryDelegatePtr delegate,
                                                      const IdentityIDList &identities
                                                      );

          PUID getID() const {return mID;}

          // (duplicate) virtual void cancel();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountIdentityLookupQuery => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountIdentityLookupQuery => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          AccountIdentityLookupQueryWeakPtr mThisWeak;
          AccountWeakPtr mOuter;

          IAccountIdentityLookupQueryDelegatePtr mDelegate;

          IHTTPQueryPtr mQuery;
          bool mSucceeded;

          IdentityIDList mIdentities;

          typedef std::map<IdentityID, LookupProfileInfo> LookupProfileMap;
          LookupProfileMap mProfiles;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account::AccountPeerFileLookupQuery
        #pragma mark

        class AccountPeerFileLookupQuery : public zsLib::MessageQueueAssociator,
                                           public hookflash::provisioning::IAccountPeerFileLookupQuery,
                                           public hookflash::services::IHTTPQueryDelegate
        {
        public:
          friend class Account;

        protected:
          AccountPeerFileLookupQuery(
                                     IMessageQueuePtr queue,
                                     AccountPtr outer,
                                     IAccountPeerFileLookupQueryDelegatePtr delegate,
                                     const UserIDList &userIDs
                                     );

          void init(const ContactIDList &associatedContactIDs);

        public:
          ~AccountPeerFileLookupQuery();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPeerFileLookupQuery => IAccountPeerFileLookupQuery
          #pragma mark

          virtual bool isComplete() const;
          virtual bool didSucceed() const;

          virtual void cancel();

          virtual void getUserIDs(UserIDList &outUserIDs) const;
          virtual String getPublicPeerFileString(const UserID &userID) const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPeerFileLookupQuery => IHTTPQueryDelegate
          #pragma mark

          virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
          virtual void onHTTPComplete(IHTTPQueryPtr query);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPeerFileLookupQuery => friend Account
          #pragma mark

          static AccountPeerFileLookupQueryPtr lookup(
                                                      AccountPtr outer,
                                                      IAccountPeerFileLookupQueryDelegatePtr delegate,
                                                      const UserIDList &userIDs,
                                                      const ContactIDList &associatedContactIDs
                                                      );

          PUID getID() const {return mID;}

          // (duplicate) virtual void cancel();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPeerFileLookupQuery => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark provisioning::Account::AccountPeerFileLookupQuery => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          AccountPeerFileLookupQueryWeakPtr mThisWeak;
          AccountWeakPtr mOuter;

          IAccountPeerFileLookupQueryDelegatePtr mDelegate;

          IHTTPQueryPtr mQuery;
          bool mSucceeded;

          UserIDList mUserIDs;

          typedef String PublicPeerFile;
          typedef std::map<UserID, PublicPeerFile> UserIDToPublicPeerFileMap;

          UserIDToPublicPeerFileMap mPeerFiles;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark provisioning::Account => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        PUID mID;
        AccountWeakPtr mThisWeak;
        AccountPtr mGracefulShutdownReference;

        StackPtr mStack;
        IAccountDelegatePtr mDelegate;
        String mProvisioningURI;

        AccountStates mCurrentState;
        AccountErrorCodes mLastError;

        hookflash::internal::IAccountForProvisioningAccountPtr mOpenPeerAccount;
        hookflash::IAccountDelegatePtr mOpenPeerAccountDelegate;
        
        LoginScenarios mLoginScenario;

        String mDeviceToken;
        String mUserID;
        String mAccountSalt;
        String mAccessKey;
        String mAccessSecret;
        String mPasswordNonce;
        String mPasswordHash;
        String mPassword;
        ElementPtr mPrivatePeerFile;
        String mName;
        Time mLastProfileUpdatedTime;

        String mNetworkURI;
        String mTurnServer;
        String mTurnUsername;
        String mTurnPassword;
        String mStunServer;

        IdentityInfoList mIdentities;
        String mIdentitiesHash;

        typedef std::map<PUID, AccountOAuthIdentityAssociationPtr> AccountOAuthIdentityAssociationMap;
        AccountOAuthIdentityAssociationMap mOAuthIdentityAssociations;

        typedef std::map<PUID, AccountPushPtr> AccountPushMap;
        AccountPushMap mAPNSPushes;

        typedef std::map<PUID, AccountIdentityLookupQueryPtr> IdentityLookupQueryMap;
        IdentityLookupQueryMap mIdentityLookupQueries;

        typedef std::map<PUID, AccountPeerFileLookupQueryPtr> PeerFileLookupQueryMap;
        PeerFileLookupQueryMap mPeerFileLookupQueries;

        // first time login
        IAccountIdentityLookupQueryPtr mFirstTimeIdentityLookup;

        // first time login - account creation
        IHTTPQueryPtr mCreateAccountHTTPQuery;
        IHTTPQueryPtr mProfilePutHTTPQuery;

        // first time login - existing account
        IdentityID mLoginIdentity;
        IHTTPQueryPtr mPasswordPINGet1Query;
        IHTTPQueryPtr mPasswordPINGet2Query;
        String mPasswordValidationKey1;
        String mPasswordValidationKey2;
        String mAuthorizationPIN;
        String mEncryptedPasswordPart1;
        String mEncryptedPasswordHash;
        String mEncryptedEncryptionKeyPart1;
        String mValidationPassProofPart1;

        IHTTPQueryPtr mPasswordGetPart1Query;
        IHTTPQueryPtr mPasswordGetPart2Query;

        // first time oauth login
        IdentityTypes mProviderType;
        String mProviderUniqueID;
        String mProviderOAuthAccessToken;
        String mProviderEncryptedOAuthAccessSecret;
        IHTTPQueryPtr mProviderLoginURLGetHTTPQuery;
        String mOAuthLoginURL;

        String mPasswordPart1;
        String mPasswordPart2;

        IHTTPQueryPtr mOAuthPasswordGet1HTTPQuery;
        IHTTPQueryPtr mOAuthPasswordGet2HTTPQuery;

        // relogin
        IHTTPQueryPtr mAccessAccountHTTPQuery;

        // multiple-usage
        IHTTPQueryPtr mProfileGetHTTPQuery;

        IHTTPQueryPtr mPasswordPut1HTTPQuery;
        IHTTPQueryPtr mPasswordPut2HTTPQuery;

        // profile get requesting
        Time mWaitingForProfileGetAtTimestamp;
        Time mActiveProfileGetTimestamp;

        // pending identities
        IdentityInfoList mPendingIdentitiesToSet;
        IdentityIDList mPendingIdentitiesToRemove;

        IHTTPQueryPtr mPendingIdentitiesProfilePutHTTPQuery;

        // validate identity send PIN or validate PIN requests
        typedef std::map<IdentityID, IHTTPQueryPtr> ValidateIdentityRequestMap;
        ValidateIdentityRequestMap mValidations;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::provisioning::internal::IAccountAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
