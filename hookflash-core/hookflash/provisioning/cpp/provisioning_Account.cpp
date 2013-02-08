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

#include <hookflash/provisioning/hookflashTypes.h>
#include <hookflash/provisioning/internal/provisioning_Account.h>
#include <hookflash/internal/hookflash_Stack.h>
#include <hookflash/internal/hookflash_Client.h>
#include <hookflash/internal/hookflash_Account.h>

#include <hookflash/provisioning/message/CreateAccountRequest.h>
#include <hookflash/provisioning/message/CreateAccountResult.h>
#include <hookflash/provisioning/message/AccessAccountRequest.h>
#include <hookflash/provisioning/message/AccessAccountResult.h>
#include <hookflash/provisioning/message/ProfileGetRequest.h>
#include <hookflash/provisioning/message/ProfileGetResult.h>
#include <hookflash/provisioning/message/ProfilePutRequest.h>
#include <hookflash/provisioning/message/ProfilePutResult.h>
#include <hookflash/provisioning/message/SendIDValidationPINRequest.h>
#include <hookflash/provisioning/message/SendIDValidationPINResult.h>
#include <hookflash/provisioning/message/ValidateIDPINRequest.h>
#include <hookflash/provisioning/message/ValidateIDPINResult.h>
#include <hookflash/provisioning/message/ProviderLoginURLGetRequest.h>
#include <hookflash/provisioning/message/ProviderLoginURLGetResult.h>
#include <hookflash/provisioning/message/OAuthLoginWebpageResult.h>
#include <hookflash/provisioning/message/OAuthPasswordGetRequest.h>
#include <hookflash/provisioning/message/OAuthPasswordGetResult.h>
#include <hookflash/provisioning/message/PasswordGetPart1Request.h>
#include <hookflash/provisioning/message/PasswordGetPart1Result.h>
#include <hookflash/provisioning/message/PasswordGetPart2Request.h>
#include <hookflash/provisioning/message/PasswordGetPart2Result.h>
#include <hookflash/provisioning/message/PasswordPINGetRequest.h>
#include <hookflash/provisioning/message/PasswordPINGetResult.h>
#include <hookflash/provisioning/message/PasswordPutRequest.h>
#include <hookflash/provisioning/message/PasswordPutResult.h>
#include <hookflash/stack/message/Message.h>

#include <hookflash/IXML.h>

#include <hookflash/stack/IHelper.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/zsHelpers.h>

#include <cryptopp/sha.h>
#include <cryptopp/osrng.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

#define HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX "http://"

#define HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_1_PREFIX "http://password1."
#define HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_2_PREFIX "http://password2."

#define HOOKFLASH_PROVISIONING_HTTP_LOOKUP_SERVER_PREFIX "http://"

#define HOOKFLASH_PROVISIONING_HTTP_QUERY_LOOKUP_PROFILE "/lookup"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_CREATE_ACCOUNT "/user/createAccount"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_ACCESS_ACCOUNT "/user/accessAccount"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PROFILE_GET "/user/profileGet"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PROFILE_PUT "/user/profilePut"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_SEND_ID_VALIDATION_PIN "/user/sendIDValidationPIN"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_VALIDATE_ID_PIN "/user/validateIDPIN"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PROVIDER_LOGIN_URL_GET "/user/providerLoginURLGet"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PROVIDER_ASSOCIATE_URL_GET "/user/providerAssociateURLGet"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PEER_PROFILE_LOOKUP "/user/peerProfileLookup"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_MULTIPARTY_APNS_PUSH "/user/multipartyAPNSPush"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_GET_PART1 "/user/passwordGet"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_GET_PART2 "/user/passwordGet"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_PIN_GET "/user/passwordPINGet"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_OAUTH_PASSWORD_GET "/user/oauthPasswordGet"
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_PUT "/user/passwordPut"

#define HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_INVALID (401)
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_INVALID_TOO_MANY_ATTEMPTS (403)
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_TRY_AGAIN_LATER (420)
#define HOOKFLASH_PROVISIONING_HTTP_QUERY_TIMESTAMP_MISMATCH_CONFLICT (409)

#define HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PASSWORD_CONFLICT (409)

namespace hookflash { namespace provisioning { ZS_DECLARE_SUBSYSTEM(hookflash_provisioning) } }

namespace hookflash
{
  namespace provisioning
  {
    namespace internal
    {
      using zsLib::Stringize;
      using CryptoPP::CFB_Mode;

      typedef zsLib::BYTE BYTE;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef hookflash::internal::Client Client;
      typedef hookflash::internal::Stack Stack;
      typedef CryptoPP::SecByteBlock SecureByteBlock;
      typedef CryptoPP::SHA1 SHA1;
      typedef CryptoPP::AutoSeededRandomPool AutoSeededRandomPool;
      typedef CryptoPP::Weak::MD5 MD5;
      typedef CryptoPP::AES AES;
      typedef CryptoPP::SHA256 SHA256;
      typedef CryptoPP::SHA1 SHA1;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static void splitPassword(
                                const String &password,
                                String &outPart1,
                                String &outPart2
                                )
      {
        SecureByteBlock buffer1(password.length());
        SecureByteBlock buffer2(password.length());

        CryptoPP::AutoSeededRandomPool rng;
        rng.GenerateBlock(buffer1, buffer1.size());

        const BYTE *p1 = (const BYTE *)password.c_str();
        const BYTE *p2 = buffer1.BytePtr();

        BYTE *output = buffer2.BytePtr();

        size_t size = password.length();

        for (; size > 0; --size, ++output, ++p1, ++p2)
        {
          (*output) = (*p1) ^ (*p2);
        }

        outPart1 = stack::IHelper::convertToBase64(buffer1, buffer1.size());
        outPart2 = stack::IHelper::convertToBase64(buffer2, buffer2.size());
      }

      //-----------------------------------------------------------------------
      static String mergePassword(
                                  const String &part1,
                                  const String &part2
                                  )
      {
        SecureByteBlock block1;
        SecureByteBlock block2;

        stack::IHelper::convertFromBase64(part1, block1);
        stack::IHelper::convertFromBase64(part2, block2);

        size_t size1 = block1.SizeInBytes();
        size_t size2 = block2.SizeInBytes();

        if (size1 != size2) return String();
        if (0 == size1) return String();

        BYTE *p1 = block1.BytePtr();
        BYTE *p2 = block2.BytePtr();

        if ((!p1) || (!p2)) return String();

        SecureByteBlock outputBuffer;
        outputBuffer.CleanNew(size1+sizeof(BYTE));

        BYTE *output = outputBuffer.BytePtr();
        if (!output) return String();

        for (; size1 > 0; --size1, ++output, ++p1, ++p2)
        {
          (*output) = (*p1) ^ (*p2);
        }
        
        return (const char *)(outputBuffer.BytePtr());
      }

      //-----------------------------------------------------------------------
      static String calculateHash(
                                  const char *prefix,
                                  const String &accountSalt,
                                  const String &passwordNonce,
                                  const String &password
                                  )
      {
        String input = prefix + accountSalt + ":" + passwordNonce + ":" + password;

        SecureByteBlock result(20);

        SHA1 sha1;

        sha1.Update((const BYTE *)input.c_str(), strlen(input.c_str()));
        sha1.Final(result);

        return stack::IHelper::convertToHex(result, result.size());
      }

      //-----------------------------------------------------------------------
      static String calculateUniqueIDHashInput(
                                               const String &accountSalt,
                                               const String &passwordNonce,
                                               const String &password
                                               )
      {
        return calculateHash("unique:", accountSalt, passwordNonce, password);
      }

      //-----------------------------------------------------------------------
      static String calculateHash(
                                  const char *prefix,
                                  const String &userID,
                                  const String &clientNonce,
                                  const String &accountSalt,
                                  const String &passwordNonce,
                                  const String &password
                                  )
      {
        String hash = calculateHash(prefix, accountSalt, passwordNonce, password);
        String input = "verify:" + userID + ":" + clientNonce + ":" + hash;

        SecureByteBlock result(20);

        SHA1 sha1;

        sha1.Update((const BYTE *)input.c_str(), strlen(input.c_str()));
        sha1.Final(result);

        return stack::IHelper::convertToHex(result, result.size());
      }

      //-----------------------------------------------------------------------
      static String calculateUniqueIDHash(
                                          IAccount::IdentityTypes type,
                                          const String &uniqueID,
                                          const String &accountSalt,
                                          const String &passwordNonce,
                                          const String &password
                                          )
      {
        String uniqueHash = calculateUniqueIDHashInput(accountSalt, passwordNonce, password);
        String input = String("unique:") + IAccount::toCodeString(type) + ":" + uniqueID + ":" + uniqueHash;

        SecureByteBlock result(20);

        SHA1 sha1;

        sha1.Update((const BYTE *)input.c_str(), strlen(input.c_str()));
        sha1.Final(result);

        return stack::IHelper::convertToHex(result, result.size());
      }

      //-----------------------------------------------------------------------
      static void getKeyInformation(
                                    const char *prefix,
                                    const char *password,
                                    const String &salt,
                                    BYTE *aesIV,
                                    BYTE *aesKey
                                    )
      {
        if (!password) password = "";
        
        // need the salt as a hash (for use as the IV in the AES ecoder)
        MD5 saltMD5;
        saltMD5.Update((const BYTE *)salt.c_str(), salt.length());
        saltMD5.Final(aesIV);
        
        SecureByteBlock key(32);
        
        SHA256 keySHA256;
        
        keySHA256.Update((const BYTE *)prefix, strlen( prefix));
        keySHA256.Update((const BYTE *)":", strlen(":"));
        keySHA256.Update((const BYTE *)salt.c_str(), salt.length());
        keySHA256.Update((const BYTE *)":", strlen(":"));
        keySHA256.Update((const BYTE *)password, strlen(password));
        keySHA256.Final(key);
        
        memcpy(aesKey, key, 32);
      }

      //-----------------------------------------------------------------------
      static void decryptFromBase64(
                                    const char *prefix,
                                    const char *password,
                                    const String &salt,
                                    const String &inputBase64,
                                    SecureByteBlock &output
                                    )
      {
        output.New(0);
        if (inputBase64.isEmpty()) return;
        
        SecureByteBlock inputRaw;
        stack::IHelper::convertFromBase64(inputBase64, inputRaw);
        
        output.CleanNew(inputRaw.SizeInBytes());
        
        BYTE iv[AES::BLOCKSIZE];
        SecureByteBlock key(32);
        getKeyInformation(prefix, password, salt, &(iv[0]), key);
        
        CFB_Mode<AES>::Decryption cfbDecryption(key, key.size(), iv, 1);
        cfbDecryption.ProcessData(output, inputRaw, inputRaw.SizeInBytes());
      }

      //-----------------------------------------------------------------------
      static String decryptFromBase64(
                                      const char *prefix,
                                      const char *password,
                                      const String &salt,
                                      const String &inputBase64
                                      )
      {
        SecureByteBlock output;
        decryptFromBase64(prefix, password, salt, inputBase64, output);

        SecureByteBlock outputNulTerminated;
        outputNulTerminated.CleanNew(output.SizeInBytes()+sizeof(char));

        memcpy(outputNulTerminated, output, output.SizeInBytes());

        return (const char *)((const BYTE *)outputNulTerminated);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account
      #pragma mark

      //-----------------------------------------------------------------------
      const char *Account::toString(LoginScenarios scenario)
      {
        switch (scenario) {
          case LoginScenario_FirstTimeLogin:      return "First time login";
          case LoginScenario_FirstTimeOAuthLogin: return "First time oauth login";
          case LoginScenario_Relogin:             return "Relogin";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      const char *Account::toString(HTTPQueryTypes queryType)
      {
        switch (queryType) {
          case HTTPQueryType_LookupProfile:           return "Lookup Profile";
          case HTTPQueryType_CreateAccount:           return "Create Account";
          case HTTPQueryType_AccessAccount:           return "Access Account";
          case HTTPQueryType_ProfileGet:              return "Profile Get";
          case HTTPQueryType_ProfilePut:              return "Profile Put";
          case HTTPQueryType_SendIDValidationPIN:     return "Send ID Validation PIN";
          case HTTPQueryType_ValidateIDPIN:           return "Validate ID PIN";
          case HTTPQueryType_ProviderLoginURLGet:     return "Provider Login URL Get";
          case HTTPQueryType_ProviderAssociateURLGet: return "Provider Associate URL Get";
          case HTTPQueryType_PeerProfileLookup:       return "Peer Profile Lookup";
          case HTTPQueryType_MultiPartyAPNSPush:      return "Multi Party APNS Push";
          case HTTPQueryType_OAuthPasswordGetPart1:   return "OAuth Password Get Part 1";
          case HTTPQueryType_OAuthPasswordGetPart2:   return "OAuth Password Get Part 2";
          case HTTPQueryType_PasswordGetPart1:        return "Password Get Part 1";
          case HTTPQueryType_PasswordGetPart2:        return "Password Get Part 2";
          case HTTPQueryType_PasswordPINGet1:         return "Password PIN Get 1";
          case HTTPQueryType_PasswordPINGet2:         return "Password PIN Get 2";
          case HTTPQueryType_PasswordPutPart1:        return "Password Put Part 1";
          case HTTPQueryType_PasswordPutPart2:        return "Password Put Part 2";
        }

        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      Account::Account(
                       IMessageQueuePtr queue,
                       IStackPtr stack,
                       LoginScenarios scenario,
                       provisioning::IAccountDelegatePtr provisioningDelegate,
                       hookflash::IAccountDelegatePtr openpeerDelegate,
                       const char *provisioningURI,
                       const char *deviceToken
                       ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mStack(Stack::convert(stack)),
        mDelegate(IAccountDelegateProxy::createWeak(Client::guiQueue(), provisioningDelegate)),
        mCurrentState(IAccount::AccountState_None),
        mLastError(IAccount::AccountErrorCode_None),
        mOpenPeerAccountDelegate(hookflash::IAccountDelegateProxy::createWeak(Client::guiQueue(), openpeerDelegate)),
        mProvisioningURI(provisioningURI ? provisioningURI : ""),
        mDeviceToken(deviceToken ? deviceToken : ""),
        mAccountSalt(stack::IHelper::randomString(32)),
        mLoginScenario(scenario)
      {
        ZS_LOG_BASIC(log("created"))

        mLoginIdentity.first = IAccount::IdentityType_None;
      }

      //-----------------------------------------------------------------------
      void Account::init()
      {
        IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      Account::~Account()
      {
        ZS_LOG_BASIC(log("destroyed"))

        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => IAccount
      #pragma mark

      //-----------------------------------------------------------------------
      AccountPtr Account::firstTimeLogin(
                                         IStackPtr stack,
                                         provisioning::IAccountDelegatePtr provisioningDelegate,
                                         hookflash::IAccountDelegatePtr openpeerDelegate,
                                         const char *provisioningURI,
                                         const char *deviceToken,
                                         const char *name,
                                         const IdentityInfoList &knownIdentities
                                         )
      {
        AccountPtr pThis(new Account(Client::hookflashQueue(), stack, LoginScenario_FirstTimeLogin, provisioningDelegate, openpeerDelegate, provisioningURI, deviceToken));
        pThis->mThisWeak = pThis;
        pThis->mName = (name ? name : "");

        pThis->mIdentities = knownIdentities;
        pThis->fixIdentityProof();
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      AccountPtr Account::firstTimeOAuthLogin(
                                              IStackPtr stack,
                                              provisioning::IAccountDelegatePtr provisioningDelegate,
                                              hookflash::IAccountDelegatePtr openpeerDelegate,
                                              const char *provisioningURI,
                                              const char *deviceToken,
                                              IdentityTypes oauthIdentityType
                                              )
      {
        AccountPtr pThis(new Account(Client::hookflashQueue(), stack, LoginScenario_FirstTimeOAuthLogin, provisioningDelegate, openpeerDelegate, provisioningURI, deviceToken));
        pThis->mThisWeak = pThis;
        pThis->mProviderType = oauthIdentityType;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      AccountPtr Account::relogin(
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
                                  )
      {
        AccountPtr pThis(new Account(Client::hookflashQueue(), stack, LoginScenario_Relogin, provisioningDelegate, openpeerDelegate, provisioningURI, deviceToken));
        pThis->mThisWeak = pThis;
        pThis->mUserID = (userID ? userID : "");
        pThis->mAccountSalt = (accountSalt ? accountSalt : "");
        pThis->mPasswordNonce = (passwordNonce ? passwordNonce : "");
        pThis->mPassword = (password ? password : "");
        pThis->mPrivatePeerFile = privatePeerFile;
        pThis->mLastProfileUpdatedTime = lastProfileUpdatedTimestamp;
        pThis->mIdentities = previousIdentities;
        pThis->fixIdentityProof();
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void Account::shutdown()
      {
        ZS_LOG_DEBUG(log("shutdown called"))

        AutoRecursiveLock lock(mLock);
        cancel();
      }

      //-----------------------------------------------------------------------
      IAccount::IdentityID Account::getAuthorizationPINIdentity() const
      {
        AutoRecursiveLock lock(mLock);
        return mLoginIdentity;
      }

      //-----------------------------------------------------------------------
      void Account::setAuthorizationPIN(const char *authorizationPIN)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!authorizationPIN)

        AutoRecursiveLock lock(mLock);

        if ((AccountState_PendingAuthorizationPIN != mCurrentState) ||
            (!mPassword.isEmpty())) {
          setLastError(AccountErrorCode_PasswordFailure);
          cancel();
          return;
        }

        if (AccountErrorCode_AuthorizationPINIncorrect == mLastError) {
          mLastError = AccountErrorCode_None;
        }

        mAuthorizationPIN = authorizationPIN;
        mEncryptedPasswordPart1.clear();
        mEncryptedPasswordHash.clear();
        mEncryptedEncryptionKeyPart1.clear();
        mValidationPassProofPart1.clear();

        if (mPasswordGetPart1Query) {
          mPasswordGetPart1Query->cancel();
          mPasswordGetPart1Query.reset();
        }
        if (mPasswordGetPart2Query) {
          mPasswordGetPart2Query->cancel();
          mPasswordGetPart2Query.reset();
        }

        IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      String Account::getOAuthLoginURL() const
      {
        AutoRecursiveLock lock(mLock);
        return mOAuthLoginURL;
      }

      //-----------------------------------------------------------------------
      void Account::completeOAuthLoginProcess(ElementPtr xmlResultFromJavascript)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!xmlResultFromJavascript)

        AutoRecursiveLock lock(mLock);

        if (AccountState_PendingOAuthLogin != mCurrentState) {
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
          return;
        }

        stack::message::MessagePtr message = stack::message::Message::create(xmlResultFromJavascript);

        if (!message->isResult()) {
          setLastError(AccountErrorCode_ServerCommunicationError);
          cancel();
          return;
        }

        stack::message::MessageResultPtr resultObj = stack::message::MessageResult::convert(message);
        if (resultObj->hasError()) {
          setLastError(AccountErrorCode_OAuthFailure);
          cancel();
          return;
        }

        message::OAuthLoginWebpageResultPtr result = message::OAuthLoginWebpageResult::convert(message);

        if (!result) {
          setLastError(AccountErrorCode_OAuthFailure);
          cancel();
          return;
        }

        mUserID = result->userID();
        mAccountSalt = result->accountSalt();
        mAccessKey = result->accessKey();
        mAccessSecret = result->accessSecret();
        mName = result->name();
        mLastProfileUpdatedTime = result->lastProfileUpdateTimestamp();

        mNetworkURI = result->networkURI();
        mTurnServer = result->turnServer();
        mTurnUsername = result->turnUsername();
        mTurnPassword = result->turnPassword();
        mStunServer = result->stunServer();

        mProviderUniqueID = result->providerUniqueID();
        mProviderOAuthAccessToken = result->providerOAuthAccessToken();
        mProviderEncryptedOAuthAccessSecret = result->providerEncryptedOAuthAccessSecret();

        if (result->created()) {
          mPassword = stack::IHelper::randomString(32);
          mPasswordNonce = services::IHelper::randomString(32);

          IdentityInfo newInfo;
          newInfo.mType = mProviderType;
          newInfo.mUniqueID = mProviderUniqueID;
          mIdentities.push_back(newInfo);

          fixIdentityProof();

          mOpenPeerAccount = hookflash::internal::IAccountForProvisioningAccount::generate(
                                                                                           mStack,
                                                                                           mOpenPeerAccountDelegate,
                                                                                           ElementPtr(),
                                                                                           ElementPtr(),
                                                                                           mPassword,
                                                                                           mNetworkURI,
                                                                                           mTurnServer,
                                                                                           mTurnUsername,
                                                                                           mTurnPassword,
                                                                                           mStunServer
                                                                                           );

          if (!mOpenPeerAccount) {
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
            return;
          }

          mOpenPeerAccount->subscribe(mThisWeak.lock());
        }

        IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      String Account::getUserID() const
      {
        AutoRecursiveLock lock(mLock);
        return mUserID;
      }

      //-----------------------------------------------------------------------
      String Account::getAccountSalt() const
      {
        AutoRecursiveLock lock(mLock);
        return mAccountSalt;
      }

      //-----------------------------------------------------------------------
      String Account::getPasswordNonce() const
      {
        AutoRecursiveLock lock(mLock);
        return mPasswordNonce;
      }

      //-----------------------------------------------------------------------
      String Account::getPassword() const
      {
        AutoRecursiveLock lock(mLock);
        return mPassword;
      }

      //-----------------------------------------------------------------------
      ElementPtr Account::getPrivatePeerFile() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mOpenPeerAccount) return ElementPtr();
        return mOpenPeerAccount->savePrivatePeer();
      }

      //-----------------------------------------------------------------------
      Time Account::getLastProfileUpdatedTime() const
      {
        AutoRecursiveLock lock(mLock);
        return mLastProfileUpdatedTime;
      }

      //-----------------------------------------------------------------------
      hookflash::IAccountPtr Account::getOpenPeerAccount() const
      {
        AutoRecursiveLock lock(mLock);
        return mOpenPeerAccount->convertIAccount();
      }

      //-----------------------------------------------------------------------
      IAccount::AccountStates Account::getState() const
      {
        AutoRecursiveLock lock(mLock);
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IAccount::AccountErrorCodes Account::getLastError() const
      {
        AutoRecursiveLock lock(mLock);
        return mLastError;
      }

      //-----------------------------------------------------------------------
      void Account::getIdentities(IdentityInfoList &outIdentities) const
      {
        AutoRecursiveLock lock(mLock);
        outIdentities = mIdentities;
      }

      //-----------------------------------------------------------------------
      void Account::setIdentities(const IdentityInfoList &inIdentities)
      {
        AutoRecursiveLock lock(mLock);
        
        IdentityInfoList identities = inIdentities;
        fixIdentityProof(identities);

        // first check if there are any of these identities pending removal, if so, then add to the pending "set" instead...
        for (IdentityInfoList::const_iterator iterID = identities.begin(); iterID != identities.end(); ++iterID)
        {
          const IdentityInfo &info = (*iterID);
          
          IdentityID id = IdentityID(info.mType, info.mUniqueID);

          IdentityIDList::iterator found = find(mPendingIdentitiesToRemove.begin(), mPendingIdentitiesToRemove.end(), id);
          if (found != mPendingIdentitiesToRemove.end()) {
            mPendingIdentitiesToRemove.erase(found);
          }
        }

        IdentityIDList addToPendingRemovalList;

        // now check if the identity used to exist but is no longer set...
        for (IdentityInfoList::iterator iter = mIdentities.begin(); iter != mIdentities.end(); )
        {
          IdentityInfoList::iterator current = iter;
          ++iter;

          IdentityInfo &existingIdentity = (*current);

          IdentityInfoList::const_iterator found = identities.begin();
          for (; found != identities.end(); ++found)
          {
            const IdentityInfo &foundInfo = (*found);
            if (foundInfo.mType != existingIdentity.mType) continue;
            if (foundInfo.mUniqueID != existingIdentity.mUniqueID) continue;
            break;
          }

          if (found == identities.end()) {
            addToPendingRemovalList.push_back(IdentityID(existingIdentity.mType, existingIdentity.mUniqueID));
          }

          // this identity is gone
          mIdentities.erase(current);
        }

        for (IdentityInfoList::iterator iterIdentities = mPendingIdentitiesToSet.begin(); iterIdentities != mPendingIdentitiesToSet.end(); ++iterIdentities)
        {
          IdentityInfo &pendingIdentity = (*iterIdentities);

          IdentityInfoList::const_iterator found = identities.begin();
          for (; found != identities.end(); ++found)
          {
            const IdentityInfo &foundInfo = (*found);
            if (foundInfo.mType != pendingIdentity.mType) continue;
            if (foundInfo.mUniqueID != pendingIdentity.mUniqueID) continue;
            break;
          }

          if (found == identities.end()) {
            addToPendingRemovalList.push_back(IdentityID(pendingIdentity.mType, pendingIdentity.mUniqueID));
          }
        }

        // add the new entries to the pending removal list if not already in the list
        for (IdentityIDList::iterator iter = addToPendingRemovalList.begin(); iter != addToPendingRemovalList.end(); ++iter)
        {
          const IdentityID &id = (*iter);
          IdentityIDList::iterator found = find(mPendingIdentitiesToRemove.begin(), mPendingIdentitiesToRemove.end(), id);
          if (found == mPendingIdentitiesToRemove.end()) {
            mPendingIdentitiesToRemove.push_back(id);
          }
        }

        // now check for new identities which are not added to the pending list yet
        for (IdentityInfoList::const_iterator iterInfo = identities.begin(); iterInfo != identities.end(); ++iterInfo)
        {
          const IdentityInfo &info = (*iterInfo);

          bool found = false;

          for (IdentityInfoList::iterator pendingSetIter = mPendingIdentitiesToSet.begin(); pendingSetIter != mPendingIdentitiesToSet.end(); ++pendingSetIter)
          {
            IdentityInfo &pendingInfo = (*pendingSetIter);
            if (pendingInfo.mType != info.mType) continue;
            if (pendingInfo.mUniqueID != info.mUniqueID) continue;

            if ((pendingInfo.mUniqueIDProof != info.mUniqueIDProof) ||
                (pendingInfo.mPriority != info.mPriority) ||
                (pendingInfo.mWeight != info.mWeight)) {
              // update the pending info since value has changed
              pendingInfo = info;
            }

            // but still mark it as found
            found = true;
          }

          if (!found) {
            for (IdentityInfoList::iterator identityIter = mIdentities.begin(); identityIter != mIdentities.end(); ++identityIter)
            {
              IdentityInfo &existingInfo = (*identityIter);

              // if any value has changed it must be added to pending list
              if (existingInfo.mType != info.mType) continue;
              if (existingInfo.mUniqueID != info.mUniqueID) continue;
              if (existingInfo.mUniqueIDProof != info.mUniqueIDProof) continue;
              if (existingInfo.mPriority != info.mPriority) continue;
              if (existingInfo.mWeight != info.mWeight) continue;

              found = true;
            }
          }

          if (!found) {
            // we must add to the pending list...
            mPendingIdentitiesToSet.push_back(info);
          }
        }

        // now we must fix the identity proofs for each of the identities
        fixIdentityProof();

        // finally we must cause a "step" to happen to push the values to the server
        IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      IAccount::IdentityValidationStates Account::getIdentityValidationState(const IdentityID &identity) const
      {
        AutoRecursiveLock lock(mLock);
        
        for (IdentityInfoList::const_iterator iter = mIdentities.begin(); iter != mIdentities.end(); ++iter)
        {
          const IdentityInfo &info = (*iter);
          if (info.mType != identity.first) continue;
          if (info.mUniqueID != identity.second) continue;

          return info.mValidationState;
        }

        return IAccount::IdentityValidationState_Optional;
      }

      //-----------------------------------------------------------------------
      void Account::validateIdentitySendPIN(const IdentityID &identity)
      {
        AutoRecursiveLock lock(mLock);

        if ((isShuttingDown()) ||
            (isShuttingDown())) {
          ZS_LOG_WARNING(Detail, log("not send identity validation PIN while shutting down/shutdown"))
          return;
        }

        if (mAccessSecret.isEmpty()) {
          if (mDelegate) {
            try {
              mDelegate->onProvisioningAccountIdentityValidationResult(mThisWeak.lock(), identity, IAccount::IdentityValidationResultCode_ServerCommunicationError);
            } catch (IAccountDelegateProxy::Exceptions::DelegateGone &) {
            }
          }

          ZS_LOG_WARNING(Detail, log("not logged in thus cannot send identity validation PIN"))
          return;
        }

        message::SendIDValidationPINRequestPtr request = message::SendIDValidationPINRequest::create();
        request->userID(mUserID);
        request->accessKey(mAccessKey);
        request->accessSecret(mAccessSecret);
        request->identityType(identity.first);
        request->identityUniqueID(identity.second);
        boost::shared_array<char> data = request->encode()->write();

        mValidations[identity] = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_SendIDValidationPIN), data.get());
      }

      //-----------------------------------------------------------------------
      void Account::validateIdentityComplete(
                                             const IdentityID &identity,
                                             const char *identityPIN
                                             )
      {
        AutoRecursiveLock lock(mLock);

        if ((isShuttingDown()) ||
            (isShuttingDown())) {
          ZS_LOG_WARNING(Detail, log("not validate identity PIN while shutting down/shutdown"))
          return;
        }

        if (mAccessSecret.isEmpty()) {
          if (mDelegate) {
            try {
              mDelegate->onProvisioningAccountIdentityValidationResult(mThisWeak.lock(), identity, IAccount::IdentityValidationResultCode_ServerCommunicationError);
            } catch (IAccountDelegateProxy::Exceptions::DelegateGone &) {
            }
          }

          ZS_LOG_WARNING(Detail, log("not logged in thus cannot validate PIN"))
          return;
        }

        message::ValidateIDPINRequestPtr request = message::ValidateIDPINRequest::create();
        request->userID(mUserID);
        request->accessKey(mAccessKey);
        request->accessSecret(mAccessSecret);
        request->identityType(identity.first);
        request->identityUniqueID(identity.second);
        request->identityPIN(identityPIN ? identityPIN : "");
        boost::shared_array<char> data = request->encode()->write();

        mValidations[identity] = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ValidateIDPIN), data.get());
      }

      //-----------------------------------------------------------------------
      IAccountOAuthIdentityAssociationPtr Account::associateOAuthIdentity(
                                                                          IdentityTypes type,
                                                                          IAccountOAuthIdentityAssociationDelegatePtr delegate
                                                                          )
      {
        ZS_THROW_INVALID_USAGE_IF(IdentityType_None == type)

        AutoRecursiveLock lock(mLock);

        ZS_THROW_INVALID_USAGE_IF(!isSocialIdentity(type))

        if ((mAccessKey.isEmpty()) ||
            (mAccessSecret.isEmpty())) {
          ZS_LOG_ERROR(Detail, log("unable to associate identity when not logged in"))
          return IAccountOAuthIdentityAssociationPtr();
        }

        AccountOAuthIdentityAssociationPtr query = AccountOAuthIdentityAssociation::associate(mThisWeak.lock(), delegate, type);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          query->cancel();
          return query;
        }

        mOAuthIdentityAssociations[query->getID()] = query;
        return query;
      }

      //-----------------------------------------------------------------------
      IAccountPushPtr Account::apnsPush(
                                        IAccountPushDelegatePtr delegate,
                                        const UserIDList &userIDs,
                                        const char *messageType,
                                        const char *message
                                        )
      {
        ZS_THROW_INVALID_USAGE_IF(userIDs.size() < 1)

        AutoRecursiveLock lock(mLock);

        AccountPushPtr query = AccountPush::push(mThisWeak.lock(), delegate, userIDs, messageType, message);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          query->cancel();
          return query;
        }

        mAPNSPushes[query->getID()] = query;
        return query;
      }

      //-----------------------------------------------------------------------
      IAccountIdentityLookupQueryPtr Account::lookup(
                                                     IAccountIdentityLookupQueryDelegatePtr delegate,
                                                     const IdentityIDList &identities
                                                     )
      {
        ZS_THROW_INVALID_USAGE_IF(identities.size() < 1)

        AutoRecursiveLock lock(mLock);

        AccountIdentityLookupQueryPtr query = AccountIdentityLookupQuery::lookup(mThisWeak.lock(), delegate, identities);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          query->cancel();
          return query;
        }

        mIdentityLookupQueries[query->getID()] = query;
        return query;
      }

      //-----------------------------------------------------------------------
      IAccountPeerFileLookupQueryPtr Account::lookup(
                                                     IAccountPeerFileLookupQueryDelegatePtr delegate,
                                                     const UserIDList &userIDs,
                                                     const ContactIDList &associatedContactIDs
                                                     )
      {
        ZS_THROW_INVALID_USAGE_IF(userIDs.size() < 1)
        ZS_THROW_INVALID_USAGE_IF(userIDs.size() != associatedContactIDs.size())

        AutoRecursiveLock lock(mLock);

        AccountPeerFileLookupQueryPtr query = AccountPeerFileLookupQuery::lookup(mThisWeak.lock(), delegate, userIDs, associatedContactIDs);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          query->cancel();
          return query;
        }

        mPeerFileLookupQueries[query->getID()] = query;
        return query;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => hookflash::IAccountDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onAccountStateChanged(
                                          hookflash::IAccountPtr account,
                                          hookflash::IAccount::AccountStates state
                                          )
      {
        AutoRecursiveLock lock(mLock);

        if (hookflash::internal::IAccountForProvisioningAccount::convert(account) != mOpenPeerAccount) {
          ZS_LOG_WARNING(Debug, log("notification from an obsolete account"))
          return;
        }

        if ((hookflash::IAccount::AccountState_Shutdown == state) ||
            (hookflash::IAccount::AccountState_ShuttingDown == state)) {
          if (hookflash::IAccount::AccountError_None != account->getLastError()) {
            setLastError(AccountErrorCode_OpenPeerAccountFailure);
          }
          cancel();
          return;
        }
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
//        AutoRecursiveLock lock(mLock);
      }
      
      //-----------------------------------------------------------------------
      void Account::onHTTPComplete(IHTTPQueryPtr query)
      {
        AutoRecursiveLock lock(mLock);

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("http query complete ignores as alread shutdown"))
          return;
        }

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Debug, log("already shutdown during HTTP complete"))
          return;
        }

        bool foundValidationQuery = false;
        IdentityID foundID;
        for (ValidateIdentityRequestMap::iterator iter = mValidations.begin(); iter != mValidations.end(); ++iter)
        {
          IHTTPQueryPtr &validateQuery = (*iter).second;
          if (query == validateQuery) {
            ZS_LOG_DEBUG(log("matches send/validate identity PIN query"))
          }
          foundValidationQuery = true;
          foundID = (*iter).first;
          mValidations.erase(iter);
          break;
        }

        if (!query->wasSuccessful()) {
          if ((query == mCreateAccountHTTPQuery) ||
              (query == mPasswordPINGet1Query) ||
              (query == mPasswordPINGet2Query) ||
              (query == mPasswordGetPart1Query) ||
              (query == mPasswordGetPart2Query) ||
              (query == mProviderLoginURLGetHTTPQuery) ||
              (query == mOAuthPasswordGet1HTTPQuery) ||
              (query == mOAuthPasswordGet2HTTPQuery) ||
              (query == mProfileGetHTTPQuery) ||
              (query == mProfilePutHTTPQuery) ||
              (query == mPasswordPut1HTTPQuery) ||
              (query == mPasswordPut2HTTPQuery) ||
              (query == mAccessAccountHTTPQuery) ||
              (query == mPendingIdentitiesProfilePutHTTPQuery) ||
              (foundValidationQuery)) {
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }
          return;
        }

        String resultStr;
        query->readDataAsString(resultStr);

        DocumentPtr doc = zsLib::XML::Document::create();
        doc->parse(resultStr.c_str());

        stack::message::MessagePtr message = stack::message::Message::create(doc);

        if (!message->isResult()) {
          if ((query == mCreateAccountHTTPQuery) ||
              (query == mPasswordPINGet1Query) ||
              (query == mPasswordPINGet2Query) ||
              (query == mPasswordGetPart1Query) ||
              (query == mPasswordGetPart2Query) ||
              (query == mProviderLoginURLGetHTTPQuery) ||
              (query == mOAuthPasswordGet1HTTPQuery) ||
              (query == mOAuthPasswordGet2HTTPQuery) ||
              (query == mProfileGetHTTPQuery) ||
              (query == mProfilePutHTTPQuery) ||
              (query == mPasswordPut1HTTPQuery) ||
              (query == mPasswordPut2HTTPQuery) ||
              (query == mAccessAccountHTTPQuery) ||
              (query == mPendingIdentitiesProfilePutHTTPQuery) ||
              (foundValidationQuery)) {
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }
          return;
        }

        stack::message::MessageResultPtr resultObj = stack::message::MessageResult::convert(message);
        if (resultObj->hasError()) {

          if (foundValidationQuery) {
            
            IAccount::IdentityValidationResultCode resultCode = IAccount::IdentityValidationResultCode_ServerCommunicationError;

            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_INVALID == resultObj->errorCode()) {
              resultCode = IAccount::IdentityValidationResultCode_PINIncorrectTryAgain;
            }
            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_INVALID_TOO_MANY_ATTEMPTS == resultObj->errorCode()) {
              resultCode = IAccount::IdentityValidationResultCode_PINIncorrectTooManyTimes;
            }
            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_TRY_AGAIN_LATER == resultObj->errorCode()) {
              resultCode = IAccount::IdentityValidationResultCode_TooManyPINFailuresTryAgainLater;
            }
            if (mDelegate) {
              try {
                mDelegate->onProvisioningAccountIdentityValidationResult(mThisWeak.lock(), foundID, resultCode);
              } catch(IAccountDelegateProxy::Exceptions::DelegateGone &) {
              }
            }
            return;
          }

          if (query == mPendingIdentitiesProfilePutHTTPQuery) {
            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_TIMESTAMP_MISMATCH_CONFLICT == resultObj->errorCode()) {
              mPendingIdentitiesProfilePutHTTPQuery->cancel();
              mPendingIdentitiesProfilePutHTTPQuery.reset();

              // force a profile get request to complete before performing a profile PUT request
              Time now = zsLib::now();
              if (Time() == mWaitingForProfileGetAtTimestamp) {
                mWaitingForProfileGetAtTimestamp = now;
              }
              if (now > mWaitingForProfileGetAtTimestamp) {
                mWaitingForProfileGetAtTimestamp = now;
              }
              IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
              return;
            }
          }

          if ((query == mPasswordGetPart1Query) ||
              (query == mPasswordGetPart2Query) ||
              (query == mPasswordPINGet1Query) ||
              (query == mPasswordPINGet2Query)) {

            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_INVALID == resultObj->errorCode()) {
              setLastError(AccountErrorCode_AuthorizationPINIncorrect);
              return;
            }

            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_INVALID_TOO_MANY_ATTEMPTS == resultObj->errorCode()) {
              setLastError(AccountErrorCode_AuthorizationPINTooManyAttempts);
              cancel();
              return;
            }

            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PIN_TRY_AGAIN_LATER == resultObj->errorCode()) {
              setLastError(AccountErrorCode_AuthorizationPINTooManyAttemptsTryAgainLater);
              cancel();
              return;
            }
          }

          if ((query == mPasswordPINGet1Query) ||
              (query == mPasswordPINGet2Query)) {

            if (HOOKFLASH_PROVISIONING_HTTP_QUERY_ERROR_PASSWORD_CONFLICT == resultObj->errorCode()) {
              setLastError(AccountErrorCode_PasswordFailure);
              cancel();
              return;
            }
          }

          if ((query == mCreateAccountHTTPQuery) ||
              (query == mPasswordPINGet1Query) ||
              (query == mPasswordPINGet2Query) ||
              (query == mPasswordGetPart1Query) ||
              (query == mPasswordGetPart2Query) ||
              (query == mProviderLoginURLGetHTTPQuery) ||
              (query == mOAuthPasswordGet1HTTPQuery) ||
              (query == mOAuthPasswordGet2HTTPQuery) ||
              (query == mProfileGetHTTPQuery) ||
              (query == mProfilePutHTTPQuery) ||
              (query == mPasswordPut1HTTPQuery) ||
              (query == mPasswordPut2HTTPQuery) ||
              (query == mAccessAccountHTTPQuery) ||
              (query == mPendingIdentitiesProfilePutHTTPQuery) ||
              (foundValidationQuery)) {
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }
          return;
        }

        if (foundValidationQuery) {
          if (mDelegate) {
            try {
              mDelegate->onProvisioningAccountIdentityValidationResult(mThisWeak.lock(), foundID, IAccount::IdentityValidationResultCode_Success);
            } catch(IAccountDelegateProxy::Exceptions::DelegateGone &) {
            }
          }
          return;
        }

        //.....................................................................
        if (query == mCreateAccountHTTPQuery) {
          message::CreateAccountResultPtr result = message::CreateAccountResult::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to create account result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }
          
          
          mUserID = result->userID();
          mAccessKey = result->accessKey();
          mAccessSecret = result->accessSecret();

          mLastProfileUpdatedTime = result->lastProfileUpdateTimestamp();
          
          mNetworkURI = result->networkURI();
          mTurnServer = result->turnServer();
          mTurnUsername = result->turnUsername();
          mTurnPassword = result->turnPassword();
          mStunServer = result->stunServer();

          mIdentities = result->profiles();

          fixIdentityProof();

          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        //.....................................................................
        if ((query == mPasswordPINGet1Query) ||
            (query == mPasswordPINGet2Query)) {
          message::PasswordPINGetResultPtr result = message::PasswordPINGetResult::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to password pin get result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          String salt = result->accountSalt();
          String passwordNonce = result->passwordNonce();

          if (!mAccountSalt.isEmpty()) {
            if (salt != mAccountSalt) {
              setLastError(AccountErrorCode_PasswordFailure);
              cancel();
              return;
            }
          }

          mAccountSalt = salt;

          if (!mPasswordNonce.isEmpty()) {
            if (passwordNonce != mPasswordNonce) {
              setLastError(AccountErrorCode_PasswordFailure);
              cancel();
              return;
            }
          }

          mPasswordNonce = passwordNonce;
          if (query == mPasswordPINGet1Query) {
            mPasswordValidationKey1 = result->validationKey();
          } else {
            mPasswordValidationKey2 = result->validationKey();
          }

          if ((!mPasswordValidationKey1.isEmpty()) &&
              (!mPasswordValidationKey2.isEmpty())) {
            setCurrentState(AccountState_PendingAuthorizationPIN);
          }

          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        //.....................................................................
        if (query == mPasswordGetPart1Query) {
          message::PasswordGetPart1ResultPtr result = message::PasswordGetPart1Result::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to password get part 1 result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          if ((result->accountSalt() != mAccountSalt) ||
              (result->passwordNonce() != mPasswordNonce)) {

            ZS_LOG_ERROR(Basic, log("account salt or password nonce mismatch"))

            setLastError(AccountErrorCode_PasswordFailure);
            cancel();
            return;
          }

          mEncryptedPasswordPart1 = result->encryptedPasswordPart1();
          mEncryptedPasswordHash = result->encryptedPasswordHash();
          mEncryptedEncryptionKeyPart1 = result->encryptedEncryptionKeyPart1();
          mValidationPassProofPart1 = result->validationPassProofPart1();

          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        //.....................................................................
        if (query == mPasswordGetPart2Query) {
          message::PasswordGetPart2ResultPtr result = message::PasswordGetPart2Result::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to password get 2 result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }
          
          if ((result->accountSalt() != mAccountSalt) ||
              (result->passwordNonce() != mPasswordNonce)) {
            ZS_LOG_ERROR(Basic, log("account salt or password nonce mismatch"))
            setLastError(AccountErrorCode_PasswordFailure);
            cancel();
            return;
          }

          String encryptionKey = result->encryptionKeyPart1();

          String passwordPart1 = decryptFromBase64("password", encryptionKey, mAccountSalt, mEncryptedPasswordPart1);
          String passwordHashFrom1 = decryptFromBase64("hash", encryptionKey, mAccountSalt, mEncryptedPasswordHash);

          String passwordPart2 = result->passwordPart2();
          String passwordHashFrom2 = result->passwordHash();

          if (passwordHashFrom1 != passwordHashFrom2) {
            setLastError(AccountErrorCode_AuthorizationPINIncorrect);
            cancel();
            return;
          }

          mPassword = mergePassword(passwordPart1, passwordPart2);
          String hash = calculateHash("password:", mAccountSalt, mPasswordNonce, mPassword);

          if (hash != passwordHashFrom1) {
            setLastError(AccountErrorCode_AuthorizationPINIncorrect);
            cancel();
            return;
          }

          mPasswordHash = hash;

          fixIdentityProof();

          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        //.....................................................................
        if (query == mProviderLoginURLGetHTTPQuery) {
          message::ProviderLoginURLGetResultPtr result = message::ProviderLoginURLGetResult::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to provider login URL get result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          mOAuthLoginURL = result->providerLoginURL();
          if (mOAuthLoginURL.isEmpty()) {
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          setCurrentState(AccountState_PendingOAuthLogin);
          return;
        }

        //.....................................................................
        if ((query == mOAuthPasswordGet1HTTPQuery) ||
            (query == mOAuthPasswordGet2HTTPQuery)) {
          message::OAuthPasswordGetResultPtr result = message::OAuthPasswordGetResult::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to oauth password get result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          if (mAccountSalt != result->accountSalt()) {
            setLastError(AccountErrorCode_PasswordFailure);
            cancel();
            return;
          }

          if (!mPasswordNonce.isEmpty()) {
            if (mPasswordNonce != result->passwordNonce()) {
              setLastError(AccountErrorCode_PasswordFailure);
              cancel();
              return;
            }
          }
          if (!mPasswordHash.isEmpty()) {
            if (mPasswordHash != result->passwordHash()) {
              setLastError(AccountErrorCode_PasswordFailure);
              cancel();
              return;
            }
          }

          mPasswordNonce = result->passwordNonce();

          if (query == mOAuthPasswordGet1HTTPQuery) {
            mPasswordPart1 = result->passwordPart();
          }
          if (query == mOAuthPasswordGet2HTTPQuery) {
            mPasswordPart2 = result->passwordPart();
          }

          if ((!mPasswordPart1.isEmpty()) &&
              (!mPasswordPart2.isEmpty())) {
            mPassword = mergePassword(mPasswordPart1, mPasswordPart2);

            fixIdentityProof();
          }

          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        //.....................................................................
        if (query == mProfileGetHTTPQuery) {
          message::ProfileGetResultPtr result = message::ProfileGetResult::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to profile get result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          if (!mPrivatePeerFile){
            mPrivatePeerFile = IXML::createFromString(result->privatePeerFile());
          }
          mName = result->name();
          mLastProfileUpdatedTime = result->lastProfileUpdateTimestamp();
          mIdentities = result->profiles();

          fixIdentitiesUpdated();

          if (Time() != mActiveProfileGetTimestamp) {
            for (AccountOAuthIdentityAssociationMap::iterator iter = mOAuthIdentityAssociations.begin(); iter != mOAuthIdentityAssociations.end(); )
            {
              AccountOAuthIdentityAssociationMap::iterator current = iter;
              ++iter;

              AccountOAuthIdentityAssociationPtr &association = (*current).second;
              association->notifyProfileGetComplete(mActiveProfileGetTimestamp);
            }
          }

          mActiveProfileGetTimestamp = Time();

          if (!mOpenPeerAccount) {
            mOpenPeerAccount = hookflash::internal::IAccountForProvisioningAccount::loadExisting(mStack, mOpenPeerAccountDelegate, mPrivatePeerFile, mPassword, mNetworkURI, mTurnServer, mTurnUsername, mTurnPassword, mStunServer);
          }
          
          if (!mOpenPeerAccount) {
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
            return;
          }
          
          mOpenPeerAccount->subscribe(mThisWeak.lock());
          
          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        //.....................................................................
        if ((query == mPasswordPut1HTTPQuery) ||
            (query == mPasswordPut2HTTPQuery)) {
          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        if (query == mAccessAccountHTTPQuery) {
          message::AccessAccountResultPtr result = message::AccessAccountResult::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to access acount result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          mAccessKey = result->accessKey();
          mAccessSecret = result->accessSecret();

          mNetworkURI = result->networkURI();
          mTurnServer = result->turnServer();
          mTurnUsername = result->turnUsername();
          mTurnPassword = result->turnPassword();
          mStunServer = result->stunServer();

          if (result->lastProfileUpdateTimestamp() == mLastProfileUpdatedTime) {
            mOpenPeerAccount = hookflash::internal::IAccountForProvisioningAccount::loadExisting(mStack, mOpenPeerAccountDelegate, mPrivatePeerFile, mPassword, mNetworkURI, mTurnServer, mTurnUsername, mTurnPassword, mStunServer);
          }

          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        //.....................................................................
        if ((query == mProfilePutHTTPQuery) ||
            (query == mPendingIdentitiesProfilePutHTTPQuery)) {
          message::ProfilePutResultPtr result = message::ProfilePutResult::convert(message);
          if (!result) {
            ZS_LOG_ERROR(Basic, log("failed to convert to profile put result"))
            setLastError(AccountErrorCode_ServerCommunicationError);
            cancel();
            return;
          }

          mLastProfileUpdatedTime = result->lastProfileUpdateTimestamp();
          mIdentities = result->profiles();

          fixIdentitiesUpdated();

          if (query == mPendingIdentitiesProfilePutHTTPQuery) {
            mPendingIdentitiesProfilePutHTTPQuery->cancel();
            mPendingIdentitiesProfilePutHTTPQuery.reset();
          }

          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => IAccountIdentityLookupQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onAccountIdentityLookupQueryComplete(IAccountIdentityLookupQueryPtr query)      
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DEBUG(log("onAccountIdentityLookupQueryComplete called"))

        if (query != mFirstTimeIdentityLookup) {
          ZS_LOG_WARNING(Detail, log("obsolete query event called"))
          return;
        }

        if (!query->didSucceed()) {
          setLastError(AccountErrorCode_ServerCommunicationError);
          cancel();
          return;
        }

        IdentityIDList identities;
        query->getIdentities(identities);

        for (IdentityIDList::iterator iter = identities.begin(); iter != identities.end(); ++iter)
        {
          IdentityID &id = (*iter);
          LookupProfileInfo info;
          if (query->getLookupProfile(id, info)) {
            mLoginIdentity = id;
            mUserID = info.mUserID;
            ZS_LOG_DEBUG(log("identity maps to existing user ID") + ", user ID=" + mUserID + ", identity type=" + IAccount::toString(id.first) + ", identity unique ID=" + id.second);
            break;
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => IAccountAsync
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::onStep()
      {
        ZS_LOG_DEBUG(log("onStep"))
        AutoRecursiveLock lock(mLock);
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => friend AccountOAuthIdentityAssociation
      #pragma mark

      //-----------------------------------------------------------------------
      String Account::getAccessKey() const
      {
        AutoRecursiveLock lock(mLock);
        return mAccessKey;
      }

      //-----------------------------------------------------------------------
      String Account::getAccessSecret() const
      {
        AutoRecursiveLock lock(mLock);
        return mAccessSecret;
      }

      //-----------------------------------------------------------------------
      void Account::notifyWaitingForProfileGet(AccountOAuthIdentityAssociationPtr association)
      {
        AutoRecursiveLock lock(mLock);

        Time updated = association->getProfileUpdatedTimestamp();
        if (Time() == mWaitingForProfileGetAtTimestamp) {
          mWaitingForProfileGetAtTimestamp = updated;
        } else {
          if (updated > mWaitingForProfileGetAtTimestamp) {
            mWaitingForProfileGetAtTimestamp = updated;
          }
        }

        IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void Account::notifyComplete(AccountOAuthIdentityAssociationPtr association)
      {
        AutoRecursiveLock lock(mLock);
        AccountOAuthIdentityAssociationMap::iterator found = mOAuthIdentityAssociations.find(association->getID());
        if (found == mOAuthIdentityAssociations.end()) return;

        mOAuthIdentityAssociations.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => friend AccountPush
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::notifyComplete(AccountPushPtr apnsPush)
      {
        AutoRecursiveLock lock(mLock);
        AccountPushMap::iterator found = mAPNSPushes.find(apnsPush->getID());
        if (found == mAPNSPushes.end()) return;

        mAPNSPushes.erase(found);
      }
      
      //-----------------------------------------------------------------------
      String Account::getLocationID() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mOpenPeerAccount) return String();
        return mOpenPeerAccount->getLocationID();
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => friend AccountIdentityLookupQuery
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::notifyComplete(AccountIdentityLookupQueryPtr lookup)
      {
        AutoRecursiveLock lock(mLock);
        IdentityLookupQueryMap::iterator found = mIdentityLookupQueries.find(lookup->getID());
        if (found == mIdentityLookupQueries.end()) return;

        mIdentityLookupQueries.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => friend AccountPeerFileLookupQuery
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::notifyComplete(AccountPeerFileLookupQueryPtr lookup)
      {
        AutoRecursiveLock lock(mLock);
        PeerFileLookupQueryMap::iterator found = mPeerFileLookupQueries.find(lookup->getID());
        if (found == mPeerFileLookupQueries.end()) return;

        mPeerFileLookupQueries.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Account::log(const char *message) const
      {
        return String("provisioning::Account [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void Account::cancel()
      {
        AutoRecursiveLock lock(mLock);

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        setCurrentState(AccountState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mGracefulShutdownReference) {
          if (mOpenPeerAccount) {
            mOpenPeerAccount->shutdown();

            if (hookflash::IAccount::AccountState_Shutdown != mOpenPeerAccount->getState()) {
              ZS_LOG_WARNING(Debug, log("waiting for openpeer account shutdown to complete"))
              return;
            }
          }
          if (mPasswordPut1HTTPQuery) {
            if (!mPasswordPut1HTTPQuery->isComplete()) {
              ZS_LOG_WARNING(Debug, log("waiting for for password put 1 to complete"))
              return;
            }
          }
          if (mPasswordPut2HTTPQuery) {
            if (!mPasswordPut2HTTPQuery->isComplete()) {
              ZS_LOG_WARNING(Debug, log("waiting for for password put 2 to complete"))
              return;
            }
          }
        }

        // clear out identity associations
        {
          AccountOAuthIdentityAssociationMap temp = mOAuthIdentityAssociations;
          for (AccountOAuthIdentityAssociationMap::iterator iter = temp.begin(); iter != temp.end(); ++iter)
          {
            AccountOAuthIdentityAssociationPtr &association = (*iter).second;
            association->cancel();
          }

          mOAuthIdentityAssociations.clear();
        }

        // clear out apns pushes
        {
          AccountPushMap temp = mAPNSPushes;
          for (AccountPushMap::iterator iter = temp.begin(); iter != temp.end(); ++iter)
          {
            AccountPushPtr &query = (*iter).second;
            query->cancel();
          }

          mAPNSPushes.clear();
        }

        // clear out identity lookups
        {
          IdentityLookupQueryMap temp = mIdentityLookupQueries;
          for (IdentityLookupQueryMap::iterator iter = temp.begin(); iter != temp.end(); ++iter)
          {
            AccountIdentityLookupQueryPtr &query = (*iter).second;
            query->cancel();
          }

          mIdentityLookupQueries.clear();
        }

        // clear out peer file lookups
        {
          PeerFileLookupQueryMap temp = mPeerFileLookupQueries;
          for (PeerFileLookupQueryMap::iterator iter = temp.begin(); iter != temp.end(); ++iter)
          {
            AccountPeerFileLookupQueryPtr &query = (*iter).second;
            query->cancel();
          }

          mPeerFileLookupQueries.clear();
        }

        setCurrentState(AccountState_Shutdown);

        mGracefulShutdownReference.reset();

        mDelegate.reset();
        mOpenPeerAccountDelegate.reset();

        if (mOpenPeerAccount) {
          mOpenPeerAccount->shutdown();
          mOpenPeerAccount.reset();
        }

        // first time login
        if (mFirstTimeIdentityLookup) {
          mFirstTimeIdentityLookup->cancel();
          mFirstTimeIdentityLookup.reset();
        }

        if (mCreateAccountHTTPQuery) {
          mCreateAccountHTTPQuery->cancel();
          mCreateAccountHTTPQuery.reset();
        }

        if (mProfilePutHTTPQuery) {
          mProfilePutHTTPQuery->cancel();
          mProfilePutHTTPQuery.reset();
        }

        if (mPasswordPINGet1Query) {
          mPasswordPINGet1Query->cancel();
          mPasswordPINGet1Query.reset();
        }
        if (mPasswordPINGet2Query) {
          mPasswordPINGet2Query->cancel();
          mPasswordPINGet2Query.reset();
        }
        if (mPasswordGetPart1Query) {
          mPasswordGetPart1Query->cancel();
          mPasswordGetPart1Query.reset();
        }
        if (mPasswordGetPart2Query) {
          mPasswordGetPart2Query->cancel();
          mPasswordGetPart2Query.reset();
        }

        // first time oauth login
        if (mProviderLoginURLGetHTTPQuery) {
          mProviderLoginURLGetHTTPQuery->cancel();
          mProviderLoginURLGetHTTPQuery.reset();
        }

        if (mOAuthPasswordGet1HTTPQuery) {
          mOAuthPasswordGet1HTTPQuery->cancel();
          mOAuthPasswordGet1HTTPQuery.reset();
        }
        if (mOAuthPasswordGet2HTTPQuery) {
          mOAuthPasswordGet2HTTPQuery->cancel();
          mOAuthPasswordGet2HTTPQuery.reset();
        }

        // relogin
        if (mAccessAccountHTTPQuery) {
          mAccessAccountHTTPQuery->cancel();
          mAccessAccountHTTPQuery.reset();
        }

        // multi-purpose
        if (mProfileGetHTTPQuery) {
          mProfileGetHTTPQuery->cancel();
          mProfileGetHTTPQuery.reset();
        }
        if (mPasswordPut1HTTPQuery) {
          mPasswordPut1HTTPQuery->cancel();
          mPasswordPut1HTTPQuery.reset();
        }
        if (mPasswordPut2HTTPQuery) {
          mPasswordPut2HTTPQuery->cancel();
          mPasswordPut2HTTPQuery.reset();
        }
        
        // pending identities
        if (mPendingIdentitiesProfilePutHTTPQuery) {
          mPendingIdentitiesProfilePutHTTPQuery->cancel();
          mPendingIdentitiesProfilePutHTTPQuery.reset();
        }
      }

      //-----------------------------------------------------------------------
      bool Account::stepFirstTimeLogin()
      {
        if (!mFirstTimeIdentityLookup) {
          IdentityIDList identities;
          for (IdentityInfoList::iterator iter = mIdentities.begin(); iter != mIdentities.end(); ++iter)
          {
            IdentityInfo &info = (*iter);
            identities.push_back(IdentityID(info.mType, info.mUniqueID));
          }

          mFirstTimeIdentityLookup = lookup(mThisWeak.lock(), identities);
        }

        if (!mFirstTimeIdentityLookup) {
          ZS_LOG_ERROR(Basic, log("failed to create first time identity lookup"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
          return false;
        }
        
        if (!mFirstTimeIdentityLookup->isComplete()) {
          ZS_LOG_DEBUG(log("waiting for first time identity lookup"))
          return false;
        }

        if (mUserID.isEmpty()) {

          if (!mCreateAccountHTTPQuery) {
            mPassword = stack::IHelper::randomString(32);
            mPasswordNonce = services::IHelper::randomString(32);

            fixIdentityProof();
            
            setCurrentState(AccountState_Pending);

            message::CreateAccountRequestPtr request = message::CreateAccountRequest::create();
            request->deviceUUID(mStack->getDeviceID());
            request->deviceToken(mDeviceToken);
            request->accountSalt(mAccountSalt);
            request->passwordNonce(mPasswordNonce);
            String hash = calculateHash("proof:", mAccountSalt, mPasswordNonce, mPassword);
            request->passwordProofHash(hash);
            request->name(mName);
            request->profiles(mIdentities);

            boost::shared_array<char> data = request->encode()->write();

            mCreateAccountHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_CreateAccount), data.get());
          }

          if (!mCreateAccountHTTPQuery) {
            ZS_LOG_ERROR(Basic, log("failed to create create account request"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
          }
          
          if (!mCreateAccountHTTPQuery->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for create account to complete"))
            return false;
          }
        }

        if (mCreateAccountHTTPQuery)
        {
          if ((!mPasswordPut1HTTPQuery) ||
              (!mPasswordPut2HTTPQuery)) {
            message::PasswordPutRequestPtr request = message::PasswordPutRequest::create();
            request->accountSalt(mAccountSalt);
            request->userID(mUserID);
            request->accessKey(mAccessKey);
            request->accessSecret(mAccessSecret);
            request->passwordNonce(mPasswordNonce);
            String hash = calculateHash("password:", mAccountSalt, mPasswordNonce, mPassword);
            request->passwordHash(hash);
            String uniqueIDProofInput = calculateUniqueIDHashInput(mAccountSalt, mPasswordNonce, mPassword);
            request->uniqueIDProofInput(uniqueIDProofInput);

            String part1;
            String part2;
            splitPassword(mPassword, part1, part2);

            request->passwordPart(part1);
            boost::shared_array<char> data1 = request->encode()->write();

            request->passwordPart(part2);
            boost::shared_array<char> data2 = request->encode()->write();

            mPasswordPut1HTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordPutPart1), data1.get());
            mPasswordPut2HTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordPutPart2), data2.get());
          }

          if ((!mPasswordPut1HTTPQuery) ||
              (!mPasswordPut2HTTPQuery)) {
            ZS_LOG_ERROR(Basic, log("password put request should have been created"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
            return false;
          }

          if ((!mPasswordPut1HTTPQuery->isComplete()) ||
              (!mPasswordPut2HTTPQuery->isComplete())) {
            ZS_LOG_DEBUG(log("waiting for password put to complete"))
            return false;
          }

          if (!mOpenPeerAccount) {
            mOpenPeerAccount = hookflash::internal::IAccountForProvisioningAccount::generate(
                                                                                             mStack,
                                                                                             mOpenPeerAccountDelegate,
                                                                                             ElementPtr(),
                                                                                             ElementPtr(),
                                                                                             mPassword,
                                                                                             mNetworkURI,
                                                                                             mTurnServer,
                                                                                             mTurnUsername,
                                                                                             mTurnPassword,
                                                                                             mStunServer
                                                                                             );
            if (!mOpenPeerAccount) {
              ZS_LOG_ERROR(Basic, log("openpeer account should have been created"))
              setLastError(AccountErrorCode_InternalFailure);
              cancel();
              return false;
            }
            
            mOpenPeerAccount->subscribe(mThisWeak.lock());
          }


          if (hookflash::IAccount::AccountState_Ready != mOpenPeerAccount->getState()) {
            ZS_LOG_DEBUG(log("waiting for account to be ready"))
            return false;
          }
          
          if (!mProfilePutHTTPQuery) {
            message::ProfilePutRequestPtr request = message::ProfilePutRequest::create();
            request->userID(mUserID);
            request->accessKey(mAccessKey);
            request->accessSecret(mAccessSecret);
            request->lastProfileUpdateTimestamp(mLastProfileUpdatedTime);
            request->passwordNonce(mPasswordNonce);

            String hash = calculateHash("proof:", mAccountSalt, mPasswordNonce, mPassword);
            request->passwordProofHash(hash);

            request->contactID(mOpenPeerAccount->getContactID());
            request->privatePeerFile(IXML::convertToString(mOpenPeerAccount->savePrivatePeer()));
            request->publicPeerFile(IXML::convertToString(mOpenPeerAccount->savePublicPeer()));
            request->name(mName);
            request->profiles(mIdentities);

            boost::shared_array<char> data = request->encode()->write();
            
            mProfilePutHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ProfilePut), data.get());
          }

          if (!mProfilePutHTTPQuery) {
            ZS_LOG_ERROR(Basic, log("failed to create profile put request"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
          }

          if (!mProfilePutHTTPQuery->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for profile put to complete"))
            return false;
          }
          return true;
        }

        if ((!mPasswordPINGet1Query) &&
            (!mPasswordPINGet2Query)) {

          mAccountSalt.clear();

          message::PasswordPINGetRequestPtr request = message::PasswordPINGetRequest::create();
          request->deviceUUID(mStack->getDeviceID());
          request->userID(mUserID);
          String clientNonce = stack::IHelper::randomString(32);
          request->clientNonce(clientNonce);
          request->identityType(mLoginIdentity.first);
          request->identityUniqueID(mLoginIdentity.second);

          boost::shared_array<char> data1 = request->encode()->write();
          boost::shared_array<char> data2 = request->encode()->write();

          mPasswordPINGet1Query = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordPINGet1), data1.get());
          mPasswordPINGet2Query = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordPINGet2), data2.get());
        }

        if ((!mPasswordPINGet1Query) ||
            (!mPasswordPINGet2Query)) {
          ZS_LOG_ERROR(Basic, log("password pin get request should have been created"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
          return false;
        }

        if ((!mPasswordPINGet1Query->isComplete()) ||
            (!mPasswordPINGet2Query->isComplete())) {
          ZS_LOG_DEBUG(log("waiting for password pin get to complete"))
          return false;
        }

        if ((mPasswordValidationKey1.isEmpty()) ||
            (mPasswordValidationKey2.isEmpty())) {
          ZS_LOG_ERROR(Basic, log("failed to obtain password validation key"))
          setLastError(AccountErrorCode_PasswordFailure);
          cancel();
          return false;
        }

        if (mAuthorizationPIN.isEmpty()) {
          ZS_LOG_DEBUG(log("waiting for authorization PIN to be set"))
          return false;
        }

        if (!mPasswordGetPart1Query) {
          message::PasswordGetPart1RequestPtr request = message::PasswordGetPart1Request::create();
          request->userID(mUserID);
          request->validationKey1(mPasswordValidationKey1);
          request->validationKey2(mPasswordValidationKey2);

          String::size_type length = mAuthorizationPIN.length();
          length /= 2;
          
          String partialPIN = mAuthorizationPIN.substr(0, length);
          request->pinDigits(partialPIN);

          boost::shared_array<char> data = request->encode()->write();

          mPasswordGetPart1Query = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordGetPart1), data.get());
        }

        if (!mPasswordGetPart1Query) {
          ZS_LOG_ERROR(Basic, log("failed to create password get part 1 request"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
        }

        if (!mPasswordGetPart1Query->isComplete()) {
          ZS_LOG_DEBUG(log("waiting for password get part 1 to complete"))
          return false;
        }

        if (!mPasswordGetPart2Query) {
          message::PasswordGetPart2RequestPtr request = message::PasswordGetPart2Request::create();
          request->userID(mUserID);
          request->validationKey1(mPasswordValidationKey1);
          request->validationKey2(mPasswordValidationKey2);
          request->encryptedEncryptionKeyPart1(mEncryptedEncryptionKeyPart1);
          request->validationPassProofPart1(mValidationPassProofPart1);

          String::size_type length = mAuthorizationPIN.length();
          length /= 2;
          String::size_type remaining = mAuthorizationPIN.length() - length;

          String partialPIN = mAuthorizationPIN.substr(length, remaining);
          request->pinDigits(partialPIN);

          boost::shared_array<char> data = request->encode()->write();

          mPasswordGetPart2Query = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordGetPart2), data.get());
        }

        if (!mPasswordGetPart2Query) {
          ZS_LOG_ERROR(Basic, log("failed to create password get part 2 request"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
        }

        if (!mPasswordGetPart2Query->isComplete()) {
          ZS_LOG_DEBUG(log("waiting for password get part 2 to complete"))
          return false;
        }

        mLastProfileUpdatedTime = Time();

        // now relogged in
        mLoginScenario = LoginScenario_Relogin;

        IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
        return false;
      }

      //-----------------------------------------------------------------------
      bool Account::stepFirstTimeOAuthLogin()
      {
        // this is a oauth-login scenario....

        if (!mProviderLoginURLGetHTTPQuery) {
          message::ProviderLoginURLGetRequestPtr request = message::ProviderLoginURLGetRequest::create();
          request->providerType(mProviderType);
          request->deviceUUID(mStack->getDeviceID());
          request->deviceToken(mDeviceToken);
          request->accountSalt(mAccountSalt);
          boost::shared_array<char> data = request->encode()->write();
          mProviderLoginURLGetHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ProviderLoginURLGet), data.get());
        }

        if (!mProviderLoginURLGetHTTPQuery) {
          ZS_LOG_ERROR(Basic, log("provider login url get request should have been created"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
          return false;
        }

        if (!mProviderLoginURLGetHTTPQuery->isComplete()) {
          ZS_LOG_DEBUG(log("waiting on provider login url get"))
          return false;
        }

        if (!mOpenPeerAccount) {
          if ((!mOAuthPasswordGet1HTTPQuery) &&
              (!mOAuthPasswordGet2HTTPQuery)) {
            message::OAuthPasswordGetRequestPtr request = message::OAuthPasswordGetRequest::create();
            request->userID(mUserID);
            request->accessKey(mAccessKey);
            request->accessSecret(mAccessSecret);
            request->providerType(mProviderType);
            request->providerUniqueID(mProviderUniqueID);
            request->providerOAuthAccessToken(mProviderOAuthAccessToken);
            request->providerEncryptedOAuthAccessSecret(mProviderEncryptedOAuthAccessSecret);
            boost::shared_array<char> data = request->encode()->write();

            mOAuthPasswordGet1HTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_OAuthPasswordGetPart1), data.get());
            mOAuthPasswordGet2HTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_OAuthPasswordGetPart2), data.get());
          }

          if ((!mOAuthPasswordGet1HTTPQuery) ||
              (!mOAuthPasswordGet2HTTPQuery)) {
            ZS_LOG_ERROR(Basic, log("oauth password get requests should have been created"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
            return false;
          }

          if ((!mOAuthPasswordGet1HTTPQuery->isComplete()) ||
              (!mOAuthPasswordGet2HTTPQuery->isComplete())) {
            ZS_LOG_DEBUG(log("waiting on oauth password get 1 or 2"))
            return false;
          }

          if (mPassword.isEmpty()) {
            ZS_LOG_ERROR(Basic, log("password should now be available but it isn't"))
            setLastError(AccountErrorCode_PasswordFailure);
            cancel();
            return false;
          }

          if (!mProfileGetHTTPQuery) {
            mActiveProfileGetTimestamp = mWaitingForProfileGetAtTimestamp;

            message::ProfileGetRequestPtr request = message::ProfileGetRequest::create();
            request->userID(mUserID);
            request->accessKey(mAccessKey);
            request->accessSecret(mAccessSecret);
            boost::shared_array<char> data = request->encode()->write();

            mProfileGetHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ProfileGet), data.get());
          }

          if (!mProfileGetHTTPQuery) {
            ZS_LOG_ERROR(Basic, log("profile get request should have been created"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
            return false;
          }

          if (!mProfileGetHTTPQuery->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for profile get to complete"))
            return false;
          }
        }

        if (!mOpenPeerAccount) {
          ZS_LOG_ERROR(Basic, log("openpeer account should have been created"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
          return false;
        }

        if (hookflash::IAccount::AccountState_Ready != mOpenPeerAccount->getState()) {
          ZS_LOG_DEBUG(log("waiting for account to be ready"))
          return false;
        }

        if ((!mOAuthPasswordGet1HTTPQuery) &&
            (!mOAuthPasswordGet2HTTPQuery)) {
          // account was generated, we need to perform a password "put"...

          if ((!mPasswordPut1HTTPQuery) ||
              (!mPasswordPut2HTTPQuery)) {
            message::PasswordPutRequestPtr request = message::PasswordPutRequest::create();
            request->accountSalt(mAccountSalt);
            request->userID(mUserID);
            request->accessKey(mAccessKey);
            request->accessSecret(mAccessSecret);
            request->passwordNonce(mPasswordNonce);
            String hash = calculateHash("password:", mAccountSalt, mPasswordNonce, mPassword);
            request->passwordHash(hash);
            String uniqueIDProofInput = calculateUniqueIDHashInput(mAccountSalt, mPasswordNonce, mPassword);
            request->uniqueIDProofInput(uniqueIDProofInput);

            String part1;
            String part2;
            splitPassword(mPassword, part1, part2);

            request->passwordPart(part1);
            boost::shared_array<char> data1 = request->encode()->write();

            request->passwordPart(part2);
            boost::shared_array<char> data2 = request->encode()->write();

            mPasswordPut1HTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordPutPart1), data1.get());
            mPasswordPut2HTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_PasswordPutPart2), data2.get());
          }

          if ((!mPasswordPut1HTTPQuery) ||
              (!mPasswordPut2HTTPQuery)) {
            ZS_LOG_ERROR(Basic, log("password put request should have been created"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
            return false;
          }

          if ((!mPasswordPut1HTTPQuery->isComplete()) ||
              (!mPasswordPut2HTTPQuery->isComplete())) {
            ZS_LOG_DEBUG(log("waiting for password put to complete"))
            return false;
          }
          
          if (!mProfilePutHTTPQuery) {
            message::ProfilePutRequestPtr request = message::ProfilePutRequest::create();
            request->userID(mUserID);
            request->accessKey(mAccessKey);
            request->accessSecret(mAccessSecret);
            request->lastProfileUpdateTimestamp(mLastProfileUpdatedTime);
            request->passwordNonce(mPasswordNonce);
            
            String hash = calculateHash("proof:", mAccountSalt, mPasswordNonce, mPassword);
            request->passwordProofHash(hash);
            
            request->contactID(mOpenPeerAccount->getContactID());
            request->privatePeerFile(IXML::convertToString(mOpenPeerAccount->savePrivatePeer()));
            request->publicPeerFile(IXML::convertToString(mOpenPeerAccount->savePublicPeer()));
            request->name(mName);
            request->profiles(mIdentities);
            
            boost::shared_array<char> data = request->encode()->write();
            
            mProfilePutHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ProfilePut), data.get());
          }
          
          if (!mProfilePutHTTPQuery) {
            ZS_LOG_ERROR(Basic, log("failed to create profile put request"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
          }
          
          if (!mProfilePutHTTPQuery->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for profile put to complete"))
            return false;
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool Account::stepRelogin()
      {
        if (!mAccessAccountHTTPQuery) {
          message::AccessAccountRequestPtr request = message::AccessAccountRequest::create();
          request->deviceUUID(mStack->getDeviceID());
          request->deviceToken(mDeviceToken);
          request->userID(mUserID);
          request->passwordNonce(mPasswordNonce);
          String clientNonce = stack::IHelper::randomString(32);
          request->clientNonce(clientNonce);

          String passwordHash = calculateHash("proof:", mUserID, clientNonce, mAccountSalt, mPasswordNonce, mPassword);
          request->passwordHash(passwordHash);

          boost::shared_array<char> data = request->encode()->write();
          mAccessAccountHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_AccessAccount), data.get());
        }

        if (!mAccessAccountHTTPQuery) {
          ZS_LOG_ERROR(Basic, log("access account request should have been created"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
          return false;
        }

        if (!mAccessAccountHTTPQuery->isComplete()) {
          ZS_LOG_DEBUG(log("waiting for access account to complete"))
          return false;
        }

        if (!mOpenPeerAccount) {
          if (!mProfileGetHTTPQuery) {
            message::ProfileGetRequestPtr request = message::ProfileGetRequest::create();
            request->userID(mUserID);
            request->accessKey(mAccessKey);
            request->accessSecret(mAccessSecret);
            boost::shared_array<char> data = request->encode()->write();

            mProfileGetHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ProfileGet), data.get());
          }

          if (!mProfileGetHTTPQuery) {
            ZS_LOG_ERROR(Basic, log("profile get request should have been created"))
            setLastError(AccountErrorCode_InternalFailure);
            cancel();
            return false;
          }

          if (!mProfileGetHTTPQuery->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for profile get to complete"))
            return false;
          }
        }

        if (!mOpenPeerAccount) {
          ZS_LOG_ERROR(Basic, log("openpeer account should have been created"))
          setLastError(AccountErrorCode_InternalFailure);
          cancel();
          return false;
        }

        if (hookflash::IAccount::AccountState_Ready != mOpenPeerAccount->getState()) {
          ZS_LOG_DEBUG(log("waiting for account to be ready"))
        }

        return true;
      }

      //-----------------------------------------------------------------------
      void Account::stepProfileGetSet()
      {
        if (Time() != mWaitingForProfileGetAtTimestamp)
        {
          if (mProfileGetHTTPQuery) {
            if (!mProfileGetHTTPQuery->isComplete()) {
              ZS_LOG_DEBUG(log("waiting for previous profile get to complete before starting a new one"))
              return;
            }

            mProfileGetHTTPQuery->cancel();
            mProfileGetHTTPQuery.reset();
          }

          mActiveProfileGetTimestamp = mWaitingForProfileGetAtTimestamp;
          mWaitingForProfileGetAtTimestamp = Time();
          
          message::ProfileGetRequestPtr request = message::ProfileGetRequest::create();
          request->userID(mUserID);
          request->accessKey(mAccessKey);
          request->accessSecret(mAccessSecret);
          boost::shared_array<char> data = request->encode()->write();
          
          mProfileGetHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ProfileGet), data.get());
          return;
        }

        if (mProfileGetHTTPQuery) {
          if (!mProfileGetHTTPQuery->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for profile get HTTP request to complete"))
            return;
          }
        }

        if (mPendingIdentitiesProfilePutHTTPQuery) {
          ZS_LOG_DEBUG(log("waiting for pending profile PUT request to complete"))
          return;
        }

        if ((mPendingIdentitiesToSet.size() < 1) &&
            (mPendingIdentitiesToRemove.size() < 1)) {
          ZS_LOG_DEBUG(log("no pending identities that need setting"))
          return;
        }

        message::ProfilePutRequestPtr request = message::ProfilePutRequest::create();
        request->userID(mUserID);
        request->accessKey(mAccessKey);
        request->accessSecret(mAccessSecret);
        request->profiles(mPendingIdentitiesToSet);
        boost::shared_array<char> data = request->encode()->write();

        mPendingIdentitiesProfilePutHTTPQuery = IHTTP::post(mThisWeak.lock(), mStack->getUserAgent(), getURL(HTTPQueryType_ProfilePut), data.get());
      }

      //-----------------------------------------------------------------------
      void Account::step()
      {
        AutoRecursiveLock lock(mLock);

        ZS_LOG_DEBUG(log("step") + ", scenario=" + toString(mLoginScenario))

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("step calling cancel since shutting down/shutdown"))
          cancel();
          return;
        }
        
        switch (mLoginScenario) {
          case LoginScenario_FirstTimeLogin:        if (!stepFirstTimeLogin()) return; break;
          case LoginScenario_FirstTimeOAuthLogin:   if (!stepFirstTimeOAuthLogin()) return; break;
          case LoginScenario_Relogin:               if (!stepRelogin()) return; break;
        }
        
        stepProfileGetSet();

        // set to ready when all done...
        setCurrentState(IAccount::AccountState_Ready);
      }

      //-----------------------------------------------------------------------
      void Account::fixIdentityProof()
      {
        AutoRecursiveLock lock(mLock);

        if ((mAccountSalt.isEmpty()) ||
            (mPasswordNonce.isEmpty()) ||
            (mPassword.isEmpty())) {
          ZS_LOG_WARNING(Detail, log("identities proof cannot be set because account isn't ready"))
          return;
        }

        fixIdentityProof(mIdentities);
        fixIdentityProof(mPendingIdentitiesToSet);
      }

      //-----------------------------------------------------------------------
      void Account::fixIdentityProof(IdentityInfoList &infoList)
      {
        AutoRecursiveLock lock(mLock);
        
        if ((mAccountSalt.isEmpty()) ||
            (mPasswordNonce.isEmpty()) ||
            (mPassword.isEmpty())) {
          ZS_LOG_WARNING(Detail, log("identities proof cannot be set because account isn't ready"))
          return;
        }

        for (IdentityInfoList::iterator iter = infoList.begin(); iter != infoList.end(); ++iter)
        {
          IdentityInfo &info = (*iter);

          info.mUniqueIDProof = calculateUniqueIDHash(info.mType, info.mUniqueID, mAccountSalt, mPasswordNonce, mPassword);
        }
      }

      //-----------------------------------------------------------------------
      void Account::fixIdentitiesUpdated()
      {
        AutoRecursiveLock lock(mLock);

        fixIdentityProof();

        // check to see which pending entries are already set for the profile
        for (IdentityInfoList::iterator iterPending = mPendingIdentitiesToSet.begin(); iterPending != mPendingIdentitiesToSet.end(); )
        {
          IdentityInfoList::iterator current = iterPending;
          ++iterPending;

          IdentityInfo &pendingInfo = (*current);
          for (IdentityInfoList::iterator iterExisting = mIdentities.begin(); iterExisting != mIdentities.end(); ++iterExisting)
          {
            IdentityInfo &existingInfo = (*iterExisting);
            if (pendingInfo.mType != existingInfo.mType) continue;
            if (pendingInfo.mUniqueID != existingInfo.mUniqueID) continue;
            if (pendingInfo.mUniqueIDProof != existingInfo.mUniqueIDProof) continue;
            if (pendingInfo.mPriority != existingInfo.mPriority) continue;
            if (pendingInfo.mWeight != existingInfo.mWeight) continue;
            if (pendingInfo.mUniqueIDProof != existingInfo.mUniqueIDProof) continue;

            mPendingIdentitiesToSet.erase(current);
            break;
          }
        }

        // check to see which pending removals are already gone from the profile
        for (IdentityIDList::iterator iterPending = mPendingIdentitiesToRemove.begin(); iterPending != mPendingIdentitiesToRemove.end(); )
        {
          IdentityIDList::iterator current = iterPending;
          ++iterPending;

          IdentityID &pendingID = (*current);

          bool found = false;

          for (IdentityInfoList::iterator iterExisting = mIdentities.begin(); iterExisting != mIdentities.end(); ++iterExisting)
          {
            IdentityInfo &existingInfo = (*iterExisting);
            if (pendingID.first != existingInfo.mType) continue;
            if (pendingID.second != existingInfo.mUniqueID) continue;

            found = true;
            mPendingIdentitiesToRemove.erase(current);
            break;
          }

          if (found) {
            ZS_LOG_WARNING(Debug, log("still needing to remove an identity") + ", identity type=" + IAccount::toString(pendingID.first) + ", unique ID=" + pendingID.second)
            continue;
          }
        }

        // if the identities have changed, the hash result will have changed
        SecureByteBlock shaResult(20);

        SHA1 sha1;
        for (IdentityInfoList::iterator iter = mIdentities.begin(); iter != mIdentities.end(); ++iter)
        {
          IdentityInfo &info = (*iter);

          const char *typeStr = IAccount::toString(info.mType);
          const char *validationStr = IAccount::toString(info.mValidationState);

          String priorityStr = Stringize<WORD>(info.mPriority);
          String weightStr = Stringize<WORD>(info.mWeight);

          sha1.Update((const BYTE *)typeStr, strlen(typeStr));
          sha1.Update((const BYTE *)":", strlen(":"));
          sha1.Update((const BYTE *)info.mUniqueID.c_str(), info.mUniqueID.length());
          sha1.Update((const BYTE *)":", strlen(":"));
          sha1.Update((const BYTE *)info.mValidationID.c_str(), info.mValidationID.length());
          sha1.Update((const BYTE *)":", strlen(":"));
          sha1.Update((const BYTE *)validationStr, strlen(validationStr));
          sha1.Update((const BYTE *)":", strlen(":"));
          sha1.Update((const BYTE *)priorityStr.c_str(), priorityStr.length());
          sha1.Update((const BYTE *)":", strlen(":"));
          sha1.Update((const BYTE *)weightStr.c_str(), weightStr.length());
        }
        sha1.Final(shaResult);

        String hashStr = stack::IHelper::convertToHex(shaResult, shaResult.size());

        if (hashStr != mIdentitiesHash) {
          mIdentitiesHash = hashStr;

          if (mDelegate) {
            try {
              mDelegate->onProvisioningAccountProfileChanged(mThisWeak.lock());
            } catch (IAccountDelegateProxy::Exceptions::DelegateGone &) {
            }
          }
        }

        // still more stuff to set/remove so force another async notification to occur
        if ((mPendingIdentitiesToSet.size() > 0) ||
            (mPendingIdentitiesToRemove.size() > 0)) {
          ZS_LOG_WARNING(Debug, log("still more pending identities to be set or removed") + ", pending set total=" + Stringize<IdentityInfoList::size_type>(mPendingIdentitiesToSet.size()).string() + ", pending remove total=" + Stringize<IdentityIDList::size_type>(mPendingIdentitiesToRemove.size()).string())
          IAccountAsyncProxy::create(mThisWeak.lock())->onStep();
        }
      }

      //-----------------------------------------------------------------------
      void Account::setCurrentState(AccountStates state)
      {
        AutoRecursiveLock lock(mLock);
        if (state == mCurrentState) return;

        //ZS_LOG_BASIC(log("state change") + ", current state=" + IAccount::toString(mCurrentState) + ", new state=" + IAccount::toString(state))

        mCurrentState = state;

        AccountPtr pThis = mThisWeak.lock();
        if (!pThis) {
          ZS_LOG_WARNING(Detail, log("unable to obtain pointer to this"))
          return;
        }

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
          return;
        }

        try {
          mDelegate->onProvisioningAccountStateChanged(pThis, state);
        } catch(IAccountDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
        }
      }

      //-----------------------------------------------------------------------
      void Account::setLastError(AccountErrorCodes error)
      {
        AutoRecursiveLock lock(mLock);
        if (error == mLastError) return;

        if (AccountErrorCode_None != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set") + ", current error=" + IAccount::toString(mLastError) + ", new error=" + IAccount::toString(error))
          return;
        }

        ZS_LOG_BASIC(log("error set") + ", current error=" + IAccount::toString(mLastError) + ", new error=" + IAccount::toString(error))

        mLastError = error;

        AccountPtr pThis = mThisWeak.lock();
        if (!pThis) {
          ZS_LOG_WARNING(Detail, log("unable to obtain pointer to this"))
          return;
        }

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
          return;
        }

        try {
          mDelegate->onProvisioningAccountError(pThis, error);
        } catch(IAccountDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
        }
      }

      //-----------------------------------------------------------------------
      String Account::getURL(HTTPQueryTypes queryType) const
      {
        ZS_LOG_DEBUG(log("get URL requested") + ", type=" + toString(queryType))

        switch (queryType) {
          case HTTPQueryType_LookupProfile:           return HOOKFLASH_PROVISIONING_HTTP_LOOKUP_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_LOOKUP_PROFILE;
          case HTTPQueryType_CreateAccount:           return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_CREATE_ACCOUNT;
          case HTTPQueryType_AccessAccount:           return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_ACCESS_ACCOUNT;
          case HTTPQueryType_ProfileGet:              return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PROFILE_GET;
          case HTTPQueryType_ProfilePut:              return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PROFILE_PUT;
          case HTTPQueryType_SendIDValidationPIN:     return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_SEND_ID_VALIDATION_PIN;
          case HTTPQueryType_ValidateIDPIN:           return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_VALIDATE_ID_PIN;
          case HTTPQueryType_ProviderLoginURLGet:     return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PROVIDER_LOGIN_URL_GET;
          case HTTPQueryType_ProviderAssociateURLGet: return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PROVIDER_ASSOCIATE_URL_GET;
          case HTTPQueryType_PeerProfileLookup:       return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PEER_PROFILE_LOOKUP;
          case HTTPQueryType_MultiPartyAPNSPush:      return HOOKFLASH_PROVISIONING_HTTP_MAINDB_SERVER_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_MULTIPARTY_APNS_PUSH;
          case HTTPQueryType_OAuthPasswordGetPart1:   return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_1_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_OAUTH_PASSWORD_GET;
          case HTTPQueryType_OAuthPasswordGetPart2:   return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_2_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_OAUTH_PASSWORD_GET;
          case HTTPQueryType_PasswordGetPart1:        return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_1_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_GET_PART1;
          case HTTPQueryType_PasswordGetPart2:        return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_2_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_GET_PART2;
          case HTTPQueryType_PasswordPINGet1:         return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_1_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_PIN_GET;
          case HTTPQueryType_PasswordPINGet2:         return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_2_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_PIN_GET;
          case HTTPQueryType_PasswordPutPart1:        return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_1_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_PUT;
          case HTTPQueryType_PasswordPutPart2:        return HOOKFLASH_PROVISIONING_HTTP_PASSWORD_SERVER_2_PREFIX + mProvisioningURI + HOOKFLASH_PROVISIONING_HTTP_QUERY_PASSWORD_PUT;
        }
        return String();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccount
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IAccount::toString(AccountStates state)
    {
      switch (state)
      {
        case AccountState_None:                     return "None";
        case AccountState_Pending:                  return "Pending";
        case AccountState_PendingAuthorizationPIN:  return "Pending authorization PIN";
        case AccountState_PendingOAuthLogin:        return "Pending OAuth login";
        case AccountState_Ready:                    return "Ready";
        case AccountState_ShuttingDown:             return "Shutting down";
        case AccountState_Shutdown:                 return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IAccount::toString(AccountErrorCodes error)
    {
      switch (error)
      {
        case AccountErrorCode_None:                                         return "None";
        case AccountErrorCode_ServerCommunicationError:                     return "Server communication error";
        case AccountErrorCode_PasswordFailure:                              return "Password failure";
        case AccountErrorCode_OAuthFailure:                                 return "OAuth failure";
        case AccountErrorCode_OAuthUserRejection:                           return "OAuth user rejection";
        case AccountErrorCode_AuthorizationPINIncorrect:                    return "Authorization PIN incorrect";
        case AccountErrorCode_AuthorizationPINTooManyAttempts:              return "Authorization PIN too many attempts";
        case AccountErrorCode_AuthorizationPINTooManyAttemptsTryAgainLater: return "Authorization PIN too many attempts try again later";
        case AccountErrorCode_OpenPeerAccountFailure:                       return "Open peer account failure";
        case AccountErrorCode_InternalFailure:                              return "Internal failure";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IAccount::toString(IdentityValidationStates state)
    {
      switch (state)
      {
        case IdentityValidationState_None:        return "none";
        case IdentityValidationState_Optional:    return "optional";
        case IdentityValidationState_Required:    return "required";
        case IdentityValidationState_Pending:     return "pending";
        case IdentityValidationState_Completed:   return "completed";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IAccount::toString(IdentityValidationResultCode code)
    {
      switch (code)
      {
        case IdentityValidationResultCode_Success:                          return "Success";
        case IdentityValidationResultCode_ServerCommunicationError:         return "Server communication error";
        case IdentityValidationResultCode_PINIncorrectTryAgain:             return "PIN incorrect try again";
        case IdentityValidationResultCode_PINIncorrectTooManyTimes:         return "PIN incorrect too many attempts";
        case IdentityValidationResultCode_TooManyPINFailuresTryAgainLater:  return "Too many PIN failures try again later";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IAccount::toString(IdentityTypes type)
    {
      switch (type)
      {
        case IdentityType_None:               return "None";
        case IdentityType_Email:              return "Email";
        case IdentityType_PhoneNumber:        return "Phone number";
        case IdentityType_LinkedInID:         return "LinkedIn ID";
        case IdentityType_FacebookID:         return "Facebook ID";
        case IdentityType_FacebookUsername:   return "Facebook Username";
        case IdentityType_TwitterID:          return "Twitter ID";
        case IdentityType_TwitterUsername:    return "Twitter Username";
        case IdentityType_AddressBook:        return "Address book";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    bool IAccount::isTraditionalIdentity(IdentityTypes type)
    {
      switch (type)
      {
        case IdentityType_None:             return false;
        case IdentityType_Email:            return true;
        case IdentityType_PhoneNumber:      return true;
        case IdentityType_LinkedInID:       return false;
        case IdentityType_FacebookID:       return false;
        case IdentityType_FacebookUsername: return false;
        case IdentityType_TwitterID:        return false;
        case IdentityType_TwitterUsername:  return false;
        case IdentityType_AddressBook:      return true;
          
      }
      return false;
    }

    //-------------------------------------------------------------------------
    bool IAccount::isSocialIdentity(IdentityTypes type)
    {
      switch (type)
      {
        case IdentityType_None:             return false;
        case IdentityType_Email:            return false;
        case IdentityType_PhoneNumber:      return false;
        case IdentityType_LinkedInID:       return true;
        case IdentityType_FacebookID:       return true;
        case IdentityType_FacebookUsername: return true;
        case IdentityType_TwitterID:        return true;
        case IdentityType_TwitterUsername:  return true;
        case IdentityType_AddressBook:      return false;
      }
      return false;
    }

    //-------------------------------------------------------------------------
    const char *IAccount::toCodeString(IdentityTypes type)
    {
      switch (type)
      {
        case IdentityType_None:             return "";
        case IdentityType_Email:            return "e";
        case IdentityType_PhoneNumber:      return "p";
        case IdentityType_LinkedInID:       return "l";
        case IdentityType_FacebookID:       return "f";
        case IdentityType_FacebookUsername: return "F";
        case IdentityType_TwitterID:        return "t";
        case IdentityType_TwitterUsername:  return "T";
        case IdentityType_AddressBook:      return "a";
      }
      return "";
    }

    //-------------------------------------------------------------------------
    IAccount::IdentityTypes IAccount::toIdentity(const char *inIdentityStr)
    {
      String identityStr(inIdentityStr ? inIdentityStr : "");

      for (int loop = (int)IdentityType_None; loop <= IdentityType_Last; ++loop)
      {
        if (identityStr == toCodeString((IdentityTypes)loop)) {
          return (IdentityTypes)loop;
        }
      }

      for (int loop = (int)IdentityType_None; loop <= IdentityType_Last; ++loop)
      {
        if (identityStr == toString((IdentityTypes)loop)) {
          return (IdentityTypes)loop;
        }
      }

      return IdentityType_None;
    }

    //-------------------------------------------------------------------------
    IAccount::IdentityValidationStates IAccount::toValidationState(const char *inValidationState)
    {
      String str(inValidationState ? inValidationState : "");

      for (int loop = (int)IdentityValidationState_None; loop <= IdentityValidationState_Last; ++loop)
      {
        if (str == toString((IdentityValidationStates)loop)) {
          return (IdentityValidationStates)loop;
        }
      }

      return IdentityValidationState_None;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccount::IdentityInfo
    #pragma mark

    //-------------------------------------------------------------------------
    IAccount::IdentityInfo::IdentityInfo() :
      mType(IdentityType_None),
      mValidationState(IdentityValidationState_None),
      mPriority(0),
      mWeight(0)
    {}

    //-------------------------------------------------------------------------
    bool IAccount::IdentityInfo::hasData() const
    {
      return ((IdentityType_None != mType) ||
              (!mUniqueID.isEmpty()) ||
              (!mUniqueIDProof.isEmpty()) ||
              (IdentityValidationState_None != mValidationState) ||
              (!mValidationID.isEmpty()) ||
              (0.0f != mPriority) ||
              (0.0f != mWeight));
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    IAccount::LookupProfileInfo::LookupProfileInfo() :
      mIdentityType(IdentityType_None),
      mPriority(0),
      mWeight(0)
    {}

    //-------------------------------------------------------------------------
    bool IAccount::LookupProfileInfo::hasData() const
    {
      return ((IdentityType_None != mIdentityType) ||
              (!mIdentityUniqueID.isEmpty()) ||
              (!mUserID.isEmpty()) ||
              (!mContactID.isEmpty()) ||
              (Time() != mLastProfileUpdateTimestamp) ||
              (0 != mPriority) ||
              (0 != mWeight));
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning::IAccount
    #pragma mark

    //-------------------------------------------------------------------------
    IAccountPtr IAccount::firstTimeLogin(
                                         IStackPtr stack,
                                         provisioning::IAccountDelegatePtr provisioningDelegate,
                                         hookflash::IAccountDelegatePtr openpeerDelegate,
                                         const char *provisioningURI,
                                         const char *deviceToken,
                                         const char *name,
                                         const IdentityInfoList &knownIdentities
                                         )
    {
      return internal::Account::firstTimeLogin(stack, provisioningDelegate, openpeerDelegate, provisioningURI, deviceToken, name, knownIdentities);
    }

    //-------------------------------------------------------------------------
    IAccountPtr IAccount::firstTimeOAuthLogin(
                                              IStackPtr stack,
                                              provisioning::IAccountDelegatePtr provisioningDelegate,
                                              hookflash::IAccountDelegatePtr openpeerDelegate,
                                              const char *provisioningURI,
                                              const char *deviceToken,
                                              IdentityTypes oauthIdentityType
                                              )
    {
      return internal::Account::firstTimeOAuthLogin(stack, provisioningDelegate, openpeerDelegate, provisioningURI, deviceToken, oauthIdentityType);
    }

    //-------------------------------------------------------------------------
    IAccountPtr IAccount::relogin(
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
                                  )
    {
      return internal::Account::relogin(stack, provisioningDelegate, openpeerDelegate, provisioningURI, deviceToken, userID, accountSalt, passwordNonce, password, privatePeerFile, lastProfileUpdatedTimestamp, previousIdentities);
    }
  }
}
