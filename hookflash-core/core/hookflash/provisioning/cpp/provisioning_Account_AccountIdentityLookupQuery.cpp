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

#include <hookflash/provisioning/message/LookupProfileRequest.h>
#include <hookflash/provisioning/message/LookupProfileResult.h>

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

      typedef zsLib::Time Time;
      typedef zsLib::String String;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountIdentityLookupQuery
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountIdentityLookupQuery::AccountIdentityLookupQuery(
                                                                      IMessageQueuePtr queue,
                                                                      AccountPtr outer,
                                                                      IAccountIdentityLookupQueryDelegatePtr delegate,
                                                                      const IdentityIDList &identities
                                                                      ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IAccountIdentityLookupQueryDelegateProxy::create(queue, delegate)),
        mOuter(outer),
        mSucceeded(false),
        mIdentities(identities)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void Account::AccountIdentityLookupQuery::init()
      {
        message::LookupProfileRequestPtr request = message::LookupProfileRequest::create();
        request->identities(mIdentities);

        boost::shared_array<char> data = request->encode()->write();
        AccountPtr outer = mOuter.lock();
        mQuery = IHTTP::post(mThisWeak.lock(), outer->getStack()->getUserAgent(), outer->getURL(HTTPQueryType_LookupProfile), data.get());
      }

      //-----------------------------------------------------------------------
      Account::AccountIdentityLookupQuery::~AccountIdentityLookupQuery()
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
      #pragma mark provisioning::Account::AccountIdentityLookupQuery => IAccountIdentityLookupQuery
      #pragma mark

      //-----------------------------------------------------------------------
      bool Account::AccountIdentityLookupQuery::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mQuery;
      }

      //-----------------------------------------------------------------------
      bool Account::AccountIdentityLookupQuery::didSucceed() const
      {
        AutoRecursiveLock lock(getLock());
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      void Account::AccountIdentityLookupQuery::cancel()
      {
        AutoRecursiveLock lock(getLock());

        AccountIdentityLookupQueryPtr pThis = mThisWeak.lock();
        AccountPtr outer = mOuter.lock();

        if (pThis) {
          if (outer) {
            outer->notifyComplete(pThis);
          }

          if (mDelegate) {
            try
            {
              mDelegate->onAccountIdentityLookupQueryComplete(pThis);
            } catch(IAccountIdentityLookupQueryDelegateProxy::Exceptions::DelegateGone &) {
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
      void Account::AccountIdentityLookupQuery::getIdentities(IdentityIDList &outIdentities) const
      {
        AutoRecursiveLock lock(getLock());
        outIdentities = mIdentities;
      }

      //-----------------------------------------------------------------------
      bool Account::AccountIdentityLookupQuery::getLookupProfile(const IdentityID &inIdentity, LookupProfileInfo &outInfo) const
      {
        AutoRecursiveLock lock(getLock());
        LookupProfileMap::const_iterator found = mProfiles.find(inIdentity);
        if (found == mProfiles.end()) return false;

        const LookupProfileInfo &info = (*found).second;
        outInfo = info;
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountIdentityLookupQuery => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::AccountIdentityLookupQuery::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
        // AutoRecursiveLock lock(getLock());
      }

      //-----------------------------------------------------------------------
      void Account::AccountIdentityLookupQuery::onHTTPComplete(IHTTPQueryPtr query)
      {
        typedef IAccount::LookupProfileInfoList LookupProfileInfoList;

        if (query != mQuery) {
          ZS_LOG_WARNING(Detail, log("query http complete notification ignored from obsolete query"))
          return;
        }

        AutoRecursiveLock lock(getLock());

        String resultStr;
        query->readDataAsString(resultStr);

        DocumentPtr doc = zsLib::XML::Document::create();
        doc->parse(resultStr.c_str());

        stack::message::MessagePtr message = stack::message::Message::create(doc);

        stack::message::MessageResultPtr resultObj = stack::message::MessageResult::convert(message);
        if (!resultObj) {
          cancel();
          return;
        }

        provisioning::message::LookupProfileResultPtr result = provisioning::message::LookupProfileResult::convert(resultObj);
        if (!result) {
          cancel();
          return;
        }

        mSucceeded = true;
        const LookupProfileInfoList &profiles = result->profiles();
        for (LookupProfileInfoList::const_iterator iter = profiles.begin(); iter != profiles.end(); ++iter)
        {
          const LookupProfileInfo &profile = (*iter);
          IdentityID identity(profile.mIdentityType, profile.mIdentityUniqueID);

          mProfiles[identity] = profile;
        }

        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountIdentityLookupQuery => friend Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountIdentityLookupQueryPtr Account::AccountIdentityLookupQuery::lookup(
                                                                                         AccountPtr outer,
                                                                                         IAccountIdentityLookupQueryDelegatePtr delegate,
                                                                                         const IdentityIDList &identities
                                                                                         )
      {
        AccountIdentityLookupQueryPtr pThis(new AccountIdentityLookupQuery(outer->getAssociatedMessageQueue(), outer, delegate, identities));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountIdentityLookupQuery => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Account::AccountIdentityLookupQuery::getLock() const
      {
        AccountPtr account = mOuter.lock();
        if (!account) return mBogusLock;
        return account->getLock();
      }

      //-----------------------------------------------------------------------
      String Account::AccountIdentityLookupQuery::log(const char *message) const
      {
        return String("Account::AccountIdentityLookupQuery [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
