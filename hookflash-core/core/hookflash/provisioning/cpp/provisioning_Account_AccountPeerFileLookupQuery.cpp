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

#include <hookflash/provisioning/message/PeerProfileLookupRequest.h>
#include <hookflash/provisioning/message/PeerProfileLookupResult.h>

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
      #pragma mark provisioning::Account::AccountPeerFileLookupQuery
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountPeerFileLookupQuery::AccountPeerFileLookupQuery(
                                                                      IMessageQueuePtr queue,
                                                                      AccountPtr outer,
                                                                      IAccountPeerFileLookupQueryDelegatePtr delegate,
                                                                      const UserIDList &userIDs
                                                                      ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IAccountPeerFileLookupQueryDelegateProxy::create(queue, delegate)),
        mSucceeded(false),
        mUserIDs(userIDs)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void Account::AccountPeerFileLookupQuery::init(const ContactIDList &associatedContactIDs)
      {
        message::PeerProfileLookupRequestPtr request = message::PeerProfileLookupRequest::create();
        request->userIDs(mUserIDs);
        request->contactIDs(associatedContactIDs);

        boost::shared_array<char> data = request->encode()->write();
        AccountPtr outer = mOuter.lock();
        mQuery = IHTTP::post(mThisWeak.lock(), outer->getStack()->getUserAgent(), outer->getURL(HTTPQueryType_PeerProfileLookup), data.get());
      }

      //-----------------------------------------------------------------------
      Account::AccountPeerFileLookupQuery::~AccountPeerFileLookupQuery()
      {
        mThisWeak.lock();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPeerFileLookupQuery => IAccountPeerFileLookupQuery
      #pragma mark

      //-----------------------------------------------------------------------
      bool Account::AccountPeerFileLookupQuery::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mQuery;
      }

      //-----------------------------------------------------------------------
      bool Account::AccountPeerFileLookupQuery::didSucceed() const
      {
        AutoRecursiveLock lock(getLock());
        return mSucceeded;
      }

      //-----------------------------------------------------------------------
      void Account::AccountPeerFileLookupQuery::cancel()
      {
        AutoRecursiveLock lock(getLock());

        AccountPeerFileLookupQueryPtr pThis = mThisWeak.lock();
        AccountPtr outer = mOuter.lock();

        if (pThis) {
          if (outer) {
            outer->notifyComplete(pThis);
          }

          if (mDelegate) {
            try
            {
              mDelegate->onAccountPeerFileLookupQueryComplete(pThis);
            } catch(IAccountPeerFileLookupQueryDelegateProxy::Exceptions::DelegateGone &) {
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
      void Account::AccountPeerFileLookupQuery::getUserIDs(UserIDList &outUserIDs) const
      {
        AutoRecursiveLock lock(getLock());
        outUserIDs = mUserIDs;
      }

      //-----------------------------------------------------------------------
      String Account::AccountPeerFileLookupQuery::getPublicPeerFileString(const UserID &userID) const
      {
        AutoRecursiveLock lock(getLock());
        UserIDToPublicPeerFileMap::const_iterator found = mPeerFiles.find(userID);
        if (found == mPeerFiles.end()) return String();

        const String &result = (*found).second;
        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPeerFileLookupQuery => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Account::AccountPeerFileLookupQuery::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
        // AutoRecursiveLock lock(getLock());
      }

      //-----------------------------------------------------------------------
      void Account::AccountPeerFileLookupQuery::onHTTPComplete(IHTTPQueryPtr query)
      {
        typedef provisioning::message::PeerProfileLookupResult::PublicPeerFileList PublicPeerFileList;

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
        if (!resultObj)
        {
          cancel();
          return;
        }

        provisioning::message::PeerProfileLookupResultPtr result = provisioning::message::PeerProfileLookupResult::convert(resultObj);
        if (!result) {
          cancel();
          return;
        }

        mSucceeded = true;
        const UserIDList &userIDs = result->userIDs();
        const PublicPeerFileList &publicPeerFiles = result->publicPeerFiles();

        UserIDList::const_iterator iter1 = userIDs.begin();
        PublicPeerFileList::const_iterator iter2 = publicPeerFiles.begin();

        for (; (iter1 != userIDs.end()) && (iter2 != publicPeerFiles.end()); ++iter1, ++iter2)
        {
          const String &userID = (*iter1);
          const String &publicPeerFile = (*iter2);

          mPeerFiles[userID] = publicPeerFile;
        }

        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPeerFileLookupQuery => friend Account
      #pragma mark

      //-----------------------------------------------------------------------
      Account::AccountPeerFileLookupQueryPtr Account::AccountPeerFileLookupQuery::lookup(
                                                                                         AccountPtr outer,
                                                                                         IAccountPeerFileLookupQueryDelegatePtr delegate,
                                                                                         const UserIDList &userIDs,
                                                                                         const ContactIDList &associatedContactIDs
                                                                                         )
      {
        AccountPeerFileLookupQueryPtr pThis(new AccountPeerFileLookupQuery(outer->getAssociatedMessageQueue(), outer, delegate, userIDs));
        pThis->mThisWeak = pThis;
        pThis->init(associatedContactIDs);
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark provisioning::Account::AccountPeerFileLookupQuery => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Account::AccountPeerFileLookupQuery::getLock() const
      {
        AccountPtr account = mOuter.lock();
        if (!account) return mBogusLock;
        return account->getLock();
      }

      //-----------------------------------------------------------------------
      String Account::AccountPeerFileLookupQuery::log(const char *message) const
      {
        return String("Account::AccountPeerFileLookupQuery [") + Stringize<PUID>(mID).string() + "] " + message;
      }



      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
