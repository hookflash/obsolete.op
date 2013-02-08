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

#include <hookflash/hookflashTypes.h>
#include <zsLib/Proxy.h>
#include <zsLib/XML.h>

#include <list>

namespace hookflash
{
  namespace provisioning
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccount
    #pragma mark

    interaction IAccount
    {
      typedef zsLib::PUID PUID;
      typedef zsLib::WORD WORD;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef zsLib::XML::ElementPtr ElementPtr;

      enum AccountStates
      {
        AccountState_None,
        AccountState_Pending,
        AccountState_PendingAuthorizationPIN,
        AccountState_PendingOAuthLogin,
        AccountState_Ready,
        AccountState_ShuttingDown,
        AccountState_Shutdown
      };

      enum AccountErrorCodes
      {
        AccountErrorCode_None,
        AccountErrorCode_ServerCommunicationError,
        AccountErrorCode_PasswordFailure,
        AccountErrorCode_OAuthFailure,
        AccountErrorCode_OAuthUserRejection,
        AccountErrorCode_AuthorizationPINIncorrect,
        AccountErrorCode_AuthorizationPINTooManyAttempts,
        AccountErrorCode_AuthorizationPINTooManyAttemptsTryAgainLater,
        AccountErrorCode_OpenPeerAccountFailure,
        AccountErrorCode_InternalFailure,
      };

      enum IdentityValidationStates
      {
        IdentityValidationState_None,

        IdentityValidationState_Optional,
        IdentityValidationState_Required,
        IdentityValidationState_Pending,
        IdentityValidationState_Completed,

        IdentityValidationState_Last = IdentityValidationState_Completed,
      };

      enum IdentityValidationResultCode
      {
        IdentityValidationResultCode_Success,
        IdentityValidationResultCode_ServerCommunicationError,
        IdentityValidationResultCode_PINIncorrectTryAgain,
        IdentityValidationResultCode_PINIncorrectTooManyTimes,
        IdentityValidationResultCode_TooManyPINFailuresTryAgainLater,
      };

      enum IdentityTypes
      {
        IdentityType_None,

        IdentityType_Email,
        IdentityType_PhoneNumber,
        IdentityType_LinkedInID,
        IdentityType_FacebookID,
        IdentityType_FacebookUsername,
        IdentityType_TwitterID,
        IdentityType_TwitterUsername,
        
        IdentityType_AddressBook,

        IdentityType_Last = IdentityType_AddressBook,
      };

      static const char *toString(AccountStates state);
      static const char *toString(AccountErrorCodes errorCode);
      static const char *toString(IdentityValidationStates state);
      static const char *toString(IdentityValidationResultCode resultCode);
      static const char *toString(IdentityTypes type);

      static bool isTraditionalIdentity(IdentityTypes type);
      static bool isSocialIdentity(IdentityTypes type);
      static const char *toCodeString(IdentityTypes type);
      static IdentityTypes toIdentity(const char *identityStr);
      static IdentityValidationStates toValidationState(const char *validationState);

      typedef String UniqueID;
      typedef std::pair<IdentityTypes, UniqueID> IdentityID;

      struct IdentityInfo
      {
        IdentityTypes mType;
        String mUniqueID;

        String mUniqueIDProof;                      // only set if known (will automatically be set)

        IdentityValidationStates mValidationState;  // only set if known (i.e. being restored for relogin)
        String mValidationID;                       // only set if known (i.e. being restored for relogin)

        WORD mPriority;
        WORD mWeight;

        IdentityInfo();
        bool hasData() const;
      };

      typedef std::list<IdentityID> IdentityIDList;
      typedef std::list<IdentityInfo> IdentityInfoList;

      struct LookupProfileInfo
      {
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef provisioning::IAccount::IdentityTypes IdentityTypes;
        typedef provisioning::IAccount::IdentityValidationStates IdentityValidationStates;
        
        IdentityTypes mIdentityType;
        String mIdentityUniqueID;
        
        String mUserID;
        String mContactID;

        Time mLastProfileUpdateTimestamp;

        WORD mPriority;
        WORD mWeight;

        String mAvatarURL;

        LookupProfileInfo();
        bool hasData() const;
      };

      typedef std::list<LookupProfileInfo> LookupProfileInfoList;
      
      typedef String UserID;
      typedef std::list<UserID> UserIDList;
      typedef String ContactID;
      typedef std::list<ContactID> ContactIDList;

      static IAccountPtr firstTimeLogin(
                                        IStackPtr stack,
                                        provisioning::IAccountDelegatePtr provisioningDelegate,
                                        hookflash::IAccountDelegatePtr openpeerDelegate,
                                        const char *provisioningURI,
                                        const char *deviceToken,
                                        const char *name,
                                        const IdentityInfoList &knownIdentities
                                        );

      static IAccountPtr firstTimeOAuthLogin(
                                             IStackPtr stack,
                                             provisioning::IAccountDelegatePtr provisioningDelegate,
                                             hookflash::IAccountDelegatePtr openpeerDelegate,
                                             const char *provisioningURI,
                                             const char *deviceToken,
                                             IdentityTypes oauthIdentityType
                                             );

      static IAccountPtr relogin(
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

      virtual PUID getID() const = 0;

      virtual void shutdown() = 0;

      virtual IdentityID getAuthorizationPINIdentity() const = 0;
      virtual void setAuthorizationPIN(const char *authorizationPIN) = 0;

      virtual String getOAuthLoginURL() const = 0;
      virtual void completeOAuthLoginProcess(ElementPtr xmlResultFromJavascript) = 0;

      virtual String getUserID() const = 0;
      virtual String getAccountSalt() const = 0;
      virtual String getPasswordNonce() const = 0;
      virtual String getPassword() const = 0;
      virtual ElementPtr getPrivatePeerFile() const = 0;
      virtual Time getLastProfileUpdatedTime() const = 0;

      virtual hookflash::IAccountPtr getOpenPeerAccount() const = 0;

      virtual AccountStates getState() const = 0;
      virtual AccountErrorCodes getLastError() const = 0;

      virtual void getIdentities(IdentityInfoList &outIdentities) const = 0;
      virtual void setIdentities(const IdentityInfoList &identities) = 0;

      virtual IdentityValidationStates getIdentityValidationState(const IdentityID &identity) const = 0;
      virtual void validateIdentitySendPIN(const IdentityID &identity) = 0;
      virtual void validateIdentityComplete(
                                            const IdentityID &identity,
                                            const char *identityPIN
                                            ) = 0;

      virtual IAccountOAuthIdentityAssociationPtr associateOAuthIdentity(
                                                                         IdentityTypes type,
                                                                         IAccountOAuthIdentityAssociationDelegatePtr delegate
                                                                         ) = 0;

      virtual IAccountPushPtr apnsPush(
                                       IAccountPushDelegatePtr delegate,
                                       const UserIDList &userIDs,
                                       const char *messageType,
                                       const char *message
                                       ) = 0;

      virtual IAccountIdentityLookupQueryPtr lookup(
                                                    IAccountIdentityLookupQueryDelegatePtr delegate,
                                                    const IdentityIDList &identities
                                                    ) = 0;

      virtual IAccountPeerFileLookupQueryPtr lookup(
                                                    IAccountPeerFileLookupQueryDelegatePtr delegate,
                                                    const UserIDList &userIDs,
                                                    const ContactIDList &associatedContactIDs
                                                    ) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountDelegate
    #pragma mark

    interaction IAccountDelegate
    {
      typedef IAccount::AccountStates AccountStates;
      typedef IAccount::AccountErrorCodes AccountErrorCodes;
      typedef IAccount::IdentityID IdentityID;
      typedef IAccount::IdentityValidationResultCode IdentityValidationResultCode;

      virtual void onProvisioningAccountStateChanged(
                                                     IAccountPtr account,
                                                     AccountStates state
                                                     ) = 0;

      virtual void onProvisioningAccountError(
                                              IAccountPtr account,
                                              AccountErrorCodes error
                                              ) = 0;

      virtual void onProvisioningAccountProfileChanged(IAccountPtr account) = 0;

      virtual void onProvisioningAccountIdentityValidationResult(
                                                                 IAccountPtr account,
                                                                 IdentityID identity,
                                                                 IdentityValidationResultCode result
                                                                 ) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountOAuthIdentityAssociation
    #pragma mark

    interaction IAccountOAuthIdentityAssociation
    {
      typedef zsLib::PUID PUID;
      typedef zsLib::String String;
      typedef IAccount::IdentityTypes IdentityTypes;
      typedef zsLib::XML::ElementPtr ElementPtr;

      virtual PUID getID() const = 0;

      virtual bool isComplete() const = 0;
      virtual bool didSucceed() const = 0;

      virtual void cancel() = 0;
      
      virtual String getOAuthLoginURL() const = 0;

      virtual IdentityTypes getProviderType() const = 0;

      virtual String getProviderUniqueID() const = 0;
      virtual String getProviderOAuthAccessToken() const = 0;
      virtual String getProviderEncryptedOAuthAccessSecret() const = 0;

      virtual void completeOAuthLoginProcess(ElementPtr xmlResultFromJavascript) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountOAuthIdentityAssociationDelegate
    #pragma mark

    interaction IAccountOAuthIdentityAssociationDelegate
    {
      virtual void onAccountOAuthIdentityAssociationProviderURLReady(IAccountOAuthIdentityAssociationPtr association) = 0;
      virtual void onAccountOAuthIdentityAssociationComplete(IAccountOAuthIdentityAssociationPtr association) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountPush
    #pragma mark

    interaction IAccountPush
    {
      typedef zsLib::PUID PUID;

      virtual PUID getID() const = 0;

      virtual bool isComplete() const = 0;
      virtual bool didSucceed() const = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountPushDelegate
    #pragma mark

    interaction IAccountPushDelegate
    {
      virtual void onAccountPushComplete(IAccountPushPtr pushObject) = 0;
    };


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountIdentityLookupQuery
    #pragma mark

    interaction IAccountIdentityLookupQuery
    {
      typedef zsLib::PUID PUID;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef IAccount::IdentityID IdentityID;
      typedef IAccount::IdentityIDList IdentityIDList;
      typedef IAccount::LookupProfileInfo LookupProfileInfo;
      
      virtual PUID getID() const = 0;

      virtual bool isComplete() const = 0;
      virtual bool didSucceed() const = 0;

      virtual void cancel() = 0;

      virtual void getIdentities(IdentityIDList &outIdentities) const = 0;
      virtual bool getLookupProfile(
                                    const IdentityID &inIdentity,
                                    LookupProfileInfo &outInfo
                                    ) const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountIdentityLookupQueryDelegate
    #pragma mark

    interaction IAccountIdentityLookupQueryDelegate
    {
      virtual void onAccountIdentityLookupQueryComplete(IAccountIdentityLookupQueryPtr query) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountPeerFileLookupQuery
    #pragma mark

    interaction IAccountPeerFileLookupQuery
    {
      typedef zsLib::PUID PUID;
      typedef zsLib::String String;
      typedef IAccount::UserID UserID;
      typedef IAccount::UserIDList UserIDList;

      virtual PUID getID() const = 0;

      virtual bool isComplete() const = 0;
      virtual bool didSucceed() const = 0;

      virtual void cancel() = 0;

      virtual void getUserIDs(UserIDList &outUserIDs) const = 0;
      virtual String getPublicPeerFileString(const UserID &userID) const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccountPeerFileLookupQueryDelegate
    #pragma mark

    interaction IAccountPeerFileLookupQueryDelegate
    {
      virtual void onAccountPeerFileLookupQueryComplete(IAccountPeerFileLookupQueryPtr query) = 0;
    };
  }
}


ZS_DECLARE_PROXY_BEGIN(hookflash::provisioning::IAccountDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccountPtr, IAccountPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccount::AccountStates, AccountStates)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccount::AccountErrorCodes, AccountErrorCodes)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccount::IdentityID, IdentityID)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccount::IdentityValidationResultCode, IdentityValidationResultCode)
ZS_DECLARE_PROXY_METHOD_2(onProvisioningAccountStateChanged, IAccountPtr, AccountStates)
ZS_DECLARE_PROXY_METHOD_2(onProvisioningAccountError, IAccountPtr, AccountErrorCodes)
ZS_DECLARE_PROXY_METHOD_1(onProvisioningAccountProfileChanged, IAccountPtr)
ZS_DECLARE_PROXY_METHOD_3(onProvisioningAccountIdentityValidationResult, IAccountPtr, IdentityID, IdentityValidationResultCode)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::provisioning::IAccountOAuthIdentityAssociationDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccountOAuthIdentityAssociationPtr, IAccountOAuthIdentityAssociationPtr)
ZS_DECLARE_PROXY_METHOD_1(onAccountOAuthIdentityAssociationProviderURLReady, IAccountOAuthIdentityAssociationPtr)
ZS_DECLARE_PROXY_METHOD_1(onAccountOAuthIdentityAssociationComplete, IAccountOAuthIdentityAssociationPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::provisioning::IAccountPushDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccountPushPtr, IAccountPushPtr)
ZS_DECLARE_PROXY_METHOD_1(onAccountPushComplete, IAccountPushPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::provisioning::IAccountIdentityLookupQueryDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccountIdentityLookupQueryPtr, IAccountIdentityLookupQueryPtr)
ZS_DECLARE_PROXY_METHOD_1(onAccountIdentityLookupQueryComplete, IAccountIdentityLookupQueryPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::provisioning::IAccountPeerFileLookupQueryDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::provisioning::IAccountPeerFileLookupQueryPtr, IAccountPeerFileLookupQueryPtr)
ZS_DECLARE_PROXY_METHOD_1(onAccountPeerFileLookupQueryComplete, IAccountPeerFileLookupQueryPtr)
ZS_DECLARE_PROXY_END()

