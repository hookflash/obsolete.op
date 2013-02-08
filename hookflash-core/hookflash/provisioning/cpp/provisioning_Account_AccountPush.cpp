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

#include <hookflash/provisioning/message/MultiPartyAPNSPushRequest.h>
#include <hookflash/provisioning/message/MultiPartyAPNSPushResult.h>

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
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPush
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountPush::AccountPush(
                                        IMessageQueuePtr queue,
                                        AccountPtr outer,
                                        IAccountPushDelegatePtr delegate
                                        ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IAccountPushDelegateProxy::create(queue, delegate)),
        mOuter(outer),
        mSucceeded(false)
      {
      }

      //-----------------------------------------------------------------------
      void Account::AccountPush::init(
                                      const UserIDList &userIDs,
                                      const char *messageType,
                                      const char *message
                                      )
      {
        message::MultiPartyAPNSPushRequestPtr request = message::MultiPartyAPNSPushRequest::create();

        AccountPtr outer = mOuter.lock();

        request->userID(outer->getUserID());
        request->accessKey(outer->getAccessKey());
        request->accessSecret(outer->getAccessSecret());
        request->locationID(outer->getLocationID());

        request->msgType(messageType ? messageType : "");
        request->msgData(message ? message : "");
        request->userIDs(userIDs);

        boost::shared_array<char> data = request->encode()->write();
        mQuery = IHTTP::post(mThisWeak.lock(), outer->getStack()->getUserAgent(), outer->getURL(HTTPQueryType_MultiPartyAPNSPush), data.get());
      }

      //-----------------------------------------------------------------------
      Account::AccountPush::~AccountPush()
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
      #pragma mark provisioning::Account::AccountPush => IAccountPush
      #pragma mark

      //-----------------------------------------------------------------------
      bool Account::AccountPush::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mQuery;
      }

      //-----------------------------------------------------------------------
      bool Account::AccountPush::didSucceed() const
      {
        AutoRecursiveLock lock(getLock());
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      void Account::AccountPush::cancel()
      {
        AutoRecursiveLock lock(getLock());

        AccountPushPtr pThis = mThisWeak.lock();
        AccountPtr outer = mOuter.lock();

        if (pThis) {
          if (outer) {
            outer->notifyComplete(pThis);
          }

          if (mDelegate) {
            try
            {
              mDelegate->onAccountPushComplete(pThis);
            } catch(IAccountPushDelegateProxy::Exceptions::DelegateGone &) {
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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPush => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::AccountPush::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
        // AutoRecursiveLock lock(getLock());
      }

      //-----------------------------------------------------------------------
      void Account::AccountPush::onHTTPComplete(IHTTPQueryPtr query)
      {
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
        if (resultObj->hasError()) {
          cancel();
          return;
        }

        provisioning::message::MultiPartyAPNSPushResultPtr result = provisioning::message::MultiPartyAPNSPushResult::convert(resultObj);
        if (!result) {
          cancel();
          return;
        }

        mSucceeded = true;
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPush => friend Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountPushPtr Account::AccountPush::push(
                                                         AccountPtr outer,
                                                         IAccountPushDelegatePtr delegate,
                                                         const UserIDList &userIDs,
                                                         const char *messageType,
                                                         const char *message
                                                         )
      {
        AccountPushPtr pThis(new AccountPush(outer->getAssociatedMessageQueue(), outer, delegate));
        pThis->mThisWeak = pThis;
        pThis->init(userIDs, messageType, message);
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPush => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Account::AccountPush::getLock() const
      {
        AccountPtr account = mOuter.lock();
        if (!account) return mBogusLock;
        return account->getLock();
      }

      //-----------------------------------------------------------------------
      String Account::AccountPush::log(const char *message) const
      {
        return String("Account::AccountPush [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
