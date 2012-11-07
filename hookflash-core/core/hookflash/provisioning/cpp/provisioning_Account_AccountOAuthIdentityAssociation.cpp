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

#include <hookflash/provisioning/message/ProviderAssociateURLGetRequest.h>
#include <hookflash/provisioning/message/ProviderAssociateURLGetResult.h>
#include <hookflash/provisioning/message/OAuthLoginWebpageForAssociationResult.h>

#include <hookflash/internal/hookflash_Stack.h>

#include <zsLib/zsHelpers.h>

namespace hookflash { namespace provisioning { ZS_DECLARE_SUBSYSTEM(hookflash_provisioning) } }

namespace hookflash
{
  namespace provisioning
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountOAuthIdentityAssociation
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountOAuthIdentityAssociation::AccountOAuthIdentityAssociation(
                                                                                IMessageQueuePtr queue,
                                                                                AccountPtr outer,
                                                                                IAccountOAuthIdentityAssociationDelegatePtr delegate,
                                                                                IdentityTypes type
                                                                                ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IAccountOAuthIdentityAssociationDelegateProxy::create(queue, delegate)),
        mOuter(outer),
        mSucceeded(false),
        mType(type)
      {
      }

      //-----------------------------------------------------------------------
      void Account::AccountOAuthIdentityAssociation::init()
      {
        AccountPtr outer = mOuter.lock();

        message::ProviderAssociateURLGetRequestPtr request = message::ProviderAssociateURLGetRequest::create();
        request->providerType(mType);
        request->userID(outer->getUserID());
        request->accessKey(outer->getAccessKey());
        request->accessSecret(outer->getAccessSecret());

        boost::shared_array<char> data = request->encode()->write();
        mQuery = IHTTP::post(mThisWeak.lock(), outer->getStack()->getUserAgent(), outer->getURL(HTTPQueryType_ProviderAssociateURLGet), data.get());
      }

      //-----------------------------------------------------------------------
      Account::AccountOAuthIdentityAssociation::~AccountOAuthIdentityAssociation()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => IAccountOAuthIdentityAssociation
      #pragma mark

      //-----------------------------------------------------------------------
      bool Account::AccountOAuthIdentityAssociation::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mQuery;
      }

      //-----------------------------------------------------------------------
      bool Account::AccountOAuthIdentityAssociation::didSucceed() const
      {
        AutoRecursiveLock lock(getLock());
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      void Account::AccountOAuthIdentityAssociation::cancel()
      {
        AutoRecursiveLock lock(getLock());

        AccountOAuthIdentityAssociationPtr pThis = mThisWeak.lock();
        AccountPtr outer = mOuter.lock();

        if (pThis) {
          if (outer) {
            outer->notifyComplete(pThis);
          }

          if (mDelegate) {
            try
            {
              mDelegate->onAccountOAuthIdentityAssociationComplete(pThis);
            } catch(IAccountOAuthIdentityAssociationDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
            }
          }
        }

        mDelegate.reset();

        if (mQuery) {
          mQuery->cancel();
          mQuery.reset();
        }
      }

      //-----------------------------------------------------------------------
      String Account::AccountOAuthIdentityAssociation::getOAuthLoginURL() const
      {
        AutoRecursiveLock lock(getLock());
        return mOAuthLoginURL;
      }

      //-----------------------------------------------------------------------
      provisioning::IAccount::IdentityTypes Account::AccountOAuthIdentityAssociation::getProviderType() const
      {
        AutoRecursiveLock lock(getLock());
        return mType;
      }
      
      //-----------------------------------------------------------------------
      String Account::AccountOAuthIdentityAssociation::getProviderUniqueID() const
      {
        AutoRecursiveLock lock(getLock());
        return mProviderUniqueID;
      }
      
      //-----------------------------------------------------------------------
      String Account::AccountOAuthIdentityAssociation::getProviderOAuthAccessToken() const
      {
        AutoRecursiveLock lock(getLock());
        return mProviderOAuthAccessToken;
      }

      //-----------------------------------------------------------------------
      String Account::AccountOAuthIdentityAssociation::getProviderEncryptedOAuthAccessSecret() const
      {
        AutoRecursiveLock lock(getLock());
        return mProviderEncryptedOAuthAccessSecret;
      }

      //-----------------------------------------------------------------------
      void Account::AccountOAuthIdentityAssociation::completeOAuthLoginProcess(ElementPtr xmlResultFromJavascript)
      {
        AutoRecursiveLock lock(getLock());

        if (!xmlResultFromJavascript) {
          ZS_LOG_WARNING(Detail, log("passed in NULL completion for oauth login process thus will cancel request"))
          cancel();
          return;
        }

        stack::message::MessagePtr message = stack::message::Message::create(xmlResultFromJavascript);

        stack::message::MessageResultPtr resultObj = stack::message::MessageResult::convert(message);
        if (resultObj->hasError()) {
          ZS_LOG_WARNING(Detail, log("message result was an error"))
          cancel();
          return;
        }

        provisioning::message::OAuthLoginWebpageForAssociationResultPtr result = provisioning::message::OAuthLoginWebpageForAssociationResult::convert(resultObj);
        if (!result) {
          ZS_LOG_WARNING(Detail, log("could not convert into proper message result object"))
          cancel();
          return;
        }

        mProviderUniqueID = result->providerUniqueID();
        mProviderOAuthAccessToken = result->providerOAuthAccessToken();
        mProviderEncryptedOAuthAccessSecret = result->providerEncryptedOAuthAccessSecret();
        mLastProfileUpdateTimestamp = result->lastProfileUpdateTimestamp();

        AccountOAuthIdentityAssociationPtr pThis = mThisWeak.lock();
        AccountPtr outer = mOuter.lock();

        if ((!pThis) ||
            (!outer)) {
          ZS_LOG_WARNING(Detail, log("could not complete request as object(s) are destroyed"))
          cancel();
          return;
        }

        outer->notifyWaitingForProfileGet(pThis);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::AccountOAuthIdentityAssociation::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
        // AutoRecursiveLock lock(getLock());
      }

      //-----------------------------------------------------------------------
      void Account::AccountOAuthIdentityAssociation::onHTTPComplete(IHTTPQueryPtr query)
      {
        AutoRecursiveLock lock(getLock());
        
        if (query != mQuery) {
          ZS_LOG_WARNING(Detail, log("query http complete notification ignored from obsolete query"))
          return;
        }

        String resultStr;
        query->readDataAsString(resultStr);

        DocumentPtr doc = zsLib::XML::Document::create();
        doc->parse(resultStr.c_str());

        stack::message::MessagePtr message = stack::message::Message::create(doc);

        stack::message::MessageResultPtr resultObj = stack::message::MessageResult::convert(message);
        if (resultObj->hasError()) {
          ZS_LOG_WARNING(Detail, log("message result was an error"))
          cancel();
          return;
        }

        provisioning::message::ProviderAssociateURLGetResultPtr result = provisioning::message::ProviderAssociateURLGetResult::convert(resultObj);
        if (!result) {
          ZS_LOG_WARNING(Detail, log("could not convert into proper message result object"))
          cancel();
          return;
        }

        mOAuthLoginURL = result->providerLoginURL();

        AccountOAuthIdentityAssociationPtr pThis = mThisWeak.lock();

        if ((!pThis) ||
            (!mDelegate)) {
          ZS_LOG_WARNING(Detail, log("could not complete request as object(s) are destroyed"))
          cancel();
          return;
        }

        try {
          mDelegate->onAccountOAuthIdentityAssociationProviderURLReady(pThis);
        } catch(IAccountOAuthIdentityAssociationDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
          cancel();
          return;
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => friend Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountOAuthIdentityAssociationPtr Account::AccountOAuthIdentityAssociation::associate(
                                                                                                      AccountPtr outer,
                                                                                                      IAccountOAuthIdentityAssociationDelegatePtr delegate,
                                                                                                      IdentityTypes type
                                                                                                      )
      {
        AccountOAuthIdentityAssociationPtr pThis(new AccountOAuthIdentityAssociation(outer->getAssociatedMessageQueue(), outer, delegate, type));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      Time Account::AccountOAuthIdentityAssociation::getProfileUpdatedTimestamp() const
      {
        AutoRecursiveLock lock(getLock());
        return mLastProfileUpdateTimestamp;
      }

      //-----------------------------------------------------------------------
      void Account::AccountOAuthIdentityAssociation::notifyProfileGetComplete(const Time &updated)
      {
        AutoRecursiveLock lock(getLock());
        if (updated < mLastProfileUpdateTimestamp) {
          ZS_LOG_WARNING(Detail, log("profile update does not update this association as its too early in time"))
          return;
        }

        ZS_LOG_DEBUG(log("account association now complete"))
        mSucceeded = true;
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountOAuthIdentityAssociation => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Account::AccountOAuthIdentityAssociation::getLock() const
      {
        AccountPtr account = mOuter.lock();
        if (!account) return mBogusLock;
        return account->getLock();
      }

      //-----------------------------------------------------------------------
      String Account::AccountOAuthIdentityAssociation::log(const char *message) const
      {
        return String("Account::AccountOAuthIdentityAssociation [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
