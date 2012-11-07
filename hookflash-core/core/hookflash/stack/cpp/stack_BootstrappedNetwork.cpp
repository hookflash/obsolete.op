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

#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/IHelper.h>

#include <hookflash/stack/message/PeerToBootstrapperServicesGetRequest.h>
#include <hookflash/stack/message/PeerToBootstrapperServicesGetResult.h>
#include <hookflash/stack/message/PeerToBootstrapperFindersGetRequest.h>
#include <hookflash/stack/message/PeerToBootstrapperFindersGetResult.h>
#include <hookflash/stack/message/PeerToSaltSignedSaltGetRequest.h>
#include <hookflash/stack/message/PeerToSaltSignedSaltGetResult.h>

#include <hookflash/services/ICanonicalXML.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>

#define HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_REQUEST_TIMEOUT_IN_SEECONDS (60)
#define HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST "services-get"
#define HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST "finders-get"
#define HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SALT_GET_REQUEST "salt-get"

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::BYTE BYTE;
      typedef zsLib::WORD WORD;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::UINT UINT;
      typedef zsLib::CSTR CSTR;
      typedef zsLib::String String;
      typedef zsLib::IPAddress IPAddress;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::Document Document;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::Seconds Seconds;
      typedef CryptoPP::AutoSeededRandomPool AutoSeededRandomPool;
      typedef CryptoPP::SHA1 SHA1;
      typedef services::ICanonicalXML ICanonicalXML;
      typedef services::IDNS IDNS;
      typedef services::IDNS::SRVResultPtr SRVResultPtr;
      typedef services::IHTTP IHTTP;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      IBootstrappedNetworkForAccountPtr IBootstrappedNetworkForAccount::convert(IBootstrappedNetworkPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForAccountFinder
      #pragma mark

      //-----------------------------------------------------------------------
      IBootstrappedNetworkForAccountFinderPtr IBootstrappedNetworkForAccountFinder::convert(IBootstrappedNetworkPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      IBootstrappedNetworkForAccountFinderPtr IBootstrappedNetworkForAccountFinder::convert(IBootstrappedNetworkForAccountPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetwork::BootstrappedNetwork(zsLib::IMessageQueuePtr queue) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(IBootstrappedNetwork::BootstrappedNetworkState_Pending),
        mLastError(IBootstrappedNetwork::BootstrappedNetworkError_None)
      {
        message::MessageFactoryStack::singleton();
      }

      //-----------------------------------------------------------------------
      BootstrappedNetwork::~BootstrappedNetwork()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG("destroyed")
        cancel();
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::init()
      {
        AutoRecursiveLock lock(mLock);
        IBootstrappedNetworkAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IBootstrappedNetwork
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetwork::create(
                                                         zsLib::IMessageQueuePtr queue,
                                                         stack::IBootstrappedNetworkDelegatePtr delegate,
                                                         const char *userAgent,
                                                         const char *networkURI,
                                                         const char *turnServer,
                                                         const char *turnServerUsername,
                                                         const char *turnServerPassword,
                                                         const char *stunServer
                                                         )
      {
        BootstrappedNetworkPtr pThis(new BootstrappedNetwork(queue));
        pThis->mThisWeak = pThis;
        pThis->mDelegates[0] = IBootstrappedNetworkDelegateProxy::createWeak(queue, delegate);
        pThis->mUserAgent = (userAgent ? userAgent : "");
        pThis->mURI = (networkURI ? networkURI : "");
        pThis->mURI = pThis->mURI + "/" + HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST;
        pThis->mTURNServer = (turnServer ? turnServer : "");
        pThis->mTURNServerUsername = (turnServerUsername ? turnServerUsername : "");
        pThis->mTURNServerPassword = (turnServerPassword ? turnServerPassword : "");
        pThis->mSTUNServer = (stunServer ? stunServer : "");
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IBootstrappedNetwork::BootstrappedNetworkStates BootstrappedNetwork::getState() const
      {
        AutoRecursiveLock lock(mLock);
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IBootstrappedNetworkSubscriptionPtr BootstrappedNetwork::subscribe(IBootstrappedNetworkDelegatePtr delegate)
      {
        ZS_THROW_INVALID_USAGE_IF(!delegate)
        delegate = (IBootstrappedNetworkDelegateProxy::createWeak(getAssociatedMessageQueue(), delegate));

        AutoRecursiveLock lock(mLock);

        SubscriptionPtr subscription = Subscription::create(mThisWeak.lock());

        if (!isPending()) {
          try {
            delegate->onBootstrappedNetworkStateChanged(mThisWeak.lock(), mCurrentState);
          } catch(IBootstrappedNetworkDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        if (isShutdown()) return subscription;

        mDelegates[subscription->getID()] = delegate;
        return subscription;
      }

      //-----------------------------------------------------------------------
      IBootstrappedNetworkFetchSignedSaltQueryPtr BootstrappedNetwork::fetchSignedSalt(
                                                                                       IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr delegate,
                                                                                       UINT totalToFetch
                                                                                       )
      {
        ZS_THROW_INVALID_USAGE_IF(!delegate)

        AutoRecursiveLock lock(mLock);

        QueryPtr query = Query::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, totalToFetch);
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("attempting to fetch signed salt after the bootstrapper is shutting down or is shutdown already"))
          query->cancel();
          return query;
        }

        if (isReady()) {
          // short circuit the process since its ready to go now...
          String saltURL = mSaltURI + HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SALT_GET_REQUEST;
          query->notify(mUserAgent, saltURL);
          return query;
        }

        // remember the pending request
        mPendingQueries[query->mID] = query;
        return query;
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::close()
      {
        AutoRecursiveLock lock(mLock);
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IBootstrappedNetworkForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::getTURNAndSTUNServers(
                                                      String &outTURNServer,
                                                      String &outTURNServerUsername,
                                                      String &outTURNServerPassword,
                                                      String &outSTUNServer
                                                      )
      {
        AutoRecursiveLock lock(mLock);

        // clone values
        outTURNServer = (CSTR)mTURNServer;
        outTURNServerUsername = (CSTR)mTURNServerUsername;
        outTURNServerPassword = (CSTR)mTURNServerPassword;
        outSTUNServer = (CSTR)mSTUNServer;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IBootstrappedNetworkForAccountFinder
      #pragma mark

      //-----------------------------------------------------------------------
      IPAddress BootstrappedNetwork::getNextPeerFinder()
      {
        AutoRecursiveLock lock(mLock);

        if (!mSRVPeerFinderFinal) return IPAddress();

        bool cloned = false;

        IPAddress result;

        while (result.isAddressEmpty()) {
          if (!mSRVClonedPeerFinders) {
            mSRVClonedPeerFinders = IDNS::cloneSRV(mSRVPeerFinderFinal);
            cloned = true;
          }

          bool found = IDNS::extractNextIP(mSRVClonedPeerFinders, result);
          if ((!result.isAddressEmpty()) &&
              (!result.isPortEmpty())) {
            ZS_LOG_BASIC(log("found next finder IP") + ", ip address=" + result.string())
            break;
          }

          result.clear();

          if (!found) {
            if (cloned) {
              ZS_LOG_WARNING(Detail, log("failure to find any peer finders in SRV result"))
              break;
            }
            mSRVClonedPeerFinders.reset();
          }
        }

        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IDNSDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onLookupCompleted(IDNSQueryPtr query)
      {
        AutoRecursiveLock lock(mLock);

        FinderLookupMap::iterator found = mFinderLookups.find(query->getID());
        if (found == mFinderLookups.end()) {
          ZS_LOG_WARNING(Detail, log("DNS query was not one which matches any known outstanding queries") + ", query ID=" + Stringize<IDNSQuery::PUID>(query->getID()).string())
          return;
        }

        for (FinderLookupMap::iterator iter = mFinderLookups.begin(); iter != mFinderLookups.end(); ++iter) {
          IDNSQueryPtr &query = (*iter).second;
          if (!query->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for finder DNS query to complete") + ", query ID=" + Stringize<IDNSQuery::PUID>(query->getID()).string())
            return;
          }
        }

        // all queries have completed
        IDNS::SRVResultList resultList;

        for (FinderLookupMap::iterator iter = mFinderLookups.begin(); iter != mFinderLookups.end(); ++iter) {
          IDNSQueryPtr &query = (*iter).second;
          SRVResultPtr result = query->getSRV();

          if (!result) {
            ZS_LOG_WARNING(Detail, log("finder DNS query failed") + ", query ID=" + Stringize<IDNSQuery::PUID>(query->getID()).string())
            continue;
          }
          resultList.push_back(result);
        }

        mSRVPeerFinderFinal = IDNS::mergeSRVs(resultList);

        if (!mSRVPeerFinderFinal) {
          ZS_LOG_ERROR(Basic, log("finder DNS lookups failed"))
          setLastError(IBootstrappedNetwork::BootstrappedNetworkError_PeerFinderDNSLookupFailure);
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
      #pragma mark BootstrappedNetwork => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
        AutoRecursiveLock lock(mLock);
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onHTTPComplete(IHTTPQueryPtr query)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!query)

        AutoRecursiveLock lock(mLock);

        if (!query->wasSuccessful()) {
          if (query == mServiceQuery) {
            ZS_LOG_ERROR(Basic, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST) + " request failed")))
            setLastError(IBootstrappedNetwork::BootstrappedNetworkError_HTTPFailure);
            cancel();
            return;
          }
          if (query == mFindersQuery) {
            ZS_LOG_ERROR(Basic, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST) + " request failed")))
            setLastError(IBootstrappedNetwork::BootstrappedNetworkError_HTTPFailure);
            cancel();
            return;
          }

          ZS_LOG_WARNING(Detail, log("unknown query is ignored") + ", query=" + Stringize<PUID>(query->getID()).string())
          return;
        }

        String result;
        query->readDataAsString(result);

        DocumentPtr doc = Document::create();
        doc->parse(result);

        message::MessagePtr message = message::Message::create(doc);
        if (message) {
          if (IMessageRequester::handleMessage(message)) {
            ZS_LOG_DEBUG(log("message requester handled request"))
            return;
          }
        }

        if (query == mServiceQuery) {
          ZS_LOG_ERROR(Basic, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST) + " failed")))
          setLastError(IBootstrappedNetwork::BootstrappedNetworkError_HTTPResponseNotUnderstood);
          cancel();
          return;
        }

        if (query == mFindersQuery) {
          ZS_LOG_ERROR(Basic, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST) + " failed")))
          setLastError(IBootstrappedNetwork::BootstrappedNetworkError_HTTPResponseNotUnderstood);
          cancel();
          return;
        }

        ZS_LOG_WARNING(Detail, log("unknown query is ignored") + ", query=" + Stringize<PUID>(query->getID()).string())
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IMessageRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      bool BootstrappedNetwork::handleMessageRequesterMessageReceived(
                                                                      IMessageRequesterPtr requester,
                                                                      message::MessagePtr message
                                                                      )
      {
        if (message::Message::MessageType_Result != message->messageType()) {
          ZS_LOG_WARNING(Detail, log("requester message received was not a result (ignored)") + ", request ID=" + requester->getMonitoredMessageID())
          return false;
        }

        message::MessageResultPtr result = message::MessageResult::convert(message);
        if (result->hasAttribute(message::MessageResult::AttributeType_ErrorCode)) {
          ZS_LOG_WARNING(Detail, log("requester result received had an error") + ", request ID=" + requester->getMonitoredMessageID() + ", error=" + Stringize<WORD>(result->errorCode()).string() + ", reason=" + result->errorReason())
          return false;
        }

        if (requester == mServiceRequester) {
          if (message::MessageFactoryStack::Method_PeerToBootstrapper_ServicesGet != (message::MessageFactoryStack::Methods)result->method()) {
            return false;
          }

          ZS_LOG_DEBUG(log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST) + " result received")))

          message::PeerToBootstrapperServicesGetResultPtr serviceResult = message::PeerToBootstrapperServicesGetResult::convert(message);

          typedef message::ServiceList ServiceList;
          typedef message::Service Service;

          const ServiceList &services = serviceResult->services();
          for (ServiceList::const_iterator iter = services.begin(); iter != services.end(); ++iter)
          {
            const Service &service = (*iter);
            if ("bootstrapper" == service.mType) {
              mBootstrapperURI = service.mURL;
            }
            if ("salt" == service.mType) {
              mSaltURI = service.mURL;
            }
          }

          if ((mBootstrapperURI.isEmpty()) ||
              (mSaltURI.isEmpty())) {
            ZS_LOG_ERROR(Detail, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST) + " failed to get some services")) + ", bootstrapper=" + mBootstrapperURI + ", salt=" + mSaltURI)
            return false;
          }

          IBootstrappedNetworkAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
          return true;
        }
        if (requester == mFindersRequester) {
          if (message::MessageFactoryStack::Method_PeerToBootstrapper_FindersGet != (message::MessageFactoryStack::Methods)result->method()) {
            return false;
          }

          message::PeerToBootstrapperFindersGetResultPtr findersResult = message::PeerToBootstrapperFindersGetResult::convert(message);

          typedef message::FinderList FinderList;
          typedef message::Finder Finder;

          const FinderList &finders = findersResult->finders();
          for (FinderList::const_iterator iter = finders.begin(); iter != finders.end(); ++iter)
          {
            const Finder &finder = (*iter);

            IDNSQueryPtr dnsQuery = IDNS::lookupSRV(mThisWeak.lock(), finder.mSRV, "finder", "udp", 5446, finder.mPriority, finder.mWeight);
            if (!dnsQuery) {
              ZS_LOG_WARNING(Detail, log("DNS lookup failed"))
              continue;
            }
            mFinderLookups[dnsQuery->getID()] = dnsQuery;
          }

          if (mFinderLookups.size() < 1) {
            ZS_LOG_ERROR(Detail, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST) + " failed to find any finders")) + ", bootstrapper=" + mBootstrapperURI + HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST)
            return false;
          }

          IBootstrappedNetworkAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
          return true;
        }

        ZS_LOG_WARNING(Detail, log("unknown requester response is ignored") + ", request ID=" + requester->getMonitoredMessageID())
        return false;
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onMessageRequesterTimedOut(IMessageRequesterPtr requester)
      {
        if (requester == mServiceRequester) {
          ZS_LOG_ERROR(Detail, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST) + " request timedout")))
          setLastError(IBootstrappedNetwork::BootstrappedNetworkError_HTTPRequestTimeout);
          cancel();
          return;
        }

        if (requester == mFindersRequester) {
          ZS_LOG_ERROR(Detail, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST) + " request timedout")))
          setLastError(IBootstrappedNetwork::BootstrappedNetworkError_HTTPRequestTimeout);
          cancel();
          return;
        }

        ZS_LOG_WARNING(Detail, log("unknown requester timeout is ignored") + ", request ID=" + requester->getMonitoredMessageID())
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IBootstrappedNetworkAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onStep()
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DEBUG(log("on step"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => friend class Subscription
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::removeSubscription(SubscriptionPtr subscription)
      {
        DelegateMap::iterator found = mDelegates.find(subscription->getID());
        if (found == mDelegates.end()) return;

        mDelegates.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => friend class Query
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::removeQuery(QueryPtr query)
      {
        PendingQueriesMap::iterator found = mPendingQueries.find(query->getID());
        if (found == mPendingQueries.end()) return;

        mPendingQueries.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      zsLib::String BootstrappedNetwork::log(const char *message) const
      {
        return String("BootstrappedNetwork [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::cancel()
      {
        AutoRecursiveLock lock(mLock);  // just in case

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("cancel called but already shutdown"))
          return;
        }

        ZS_LOG_DEBUG(log("cancel called"))

        setCurrentState(IBootstrappedNetwork::BootstrappedNetworkState_ShuttingDown);

        PendingQueriesMap pendingQueries = mPendingQueries;
        for (PendingQueriesMap::iterator iter = pendingQueries.begin(); iter != pendingQueries.end(); ++iter)
        {
          QueryPtr &query = (*iter).second;
          query->cancel();
        }

        for (FinderLookupMap::iterator iter = mFinderLookups.begin(); iter != mFinderLookups.end(); ++iter)
        {
          IDNSQueryPtr &query = (*iter).second;
          query->cancel();
        }

        if (mServiceQuery) {
          mServiceQuery->cancel();
          mServiceQuery.reset();
        }

        if (mFindersQuery) {
          mFindersQuery->cancel();
          mFindersQuery.reset();
        }

        if (mServiceRequester) {
          mServiceRequester->cancel();
          mServiceRequester.reset();
        }

        if (mFindersRequester) {
          mFindersRequester->cancel();
          mFindersRequester.reset();
        }

        setCurrentState(IBootstrappedNetwork::BootstrappedNetworkState_Shutdown);

        mDelegates.clear();
        mPendingQueries.clear();

        mSRVPeerFinderFinal.reset();
        mSRVClonedPeerFinders.reset();
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::step()
      {
        AutoRecursiveLock lock(mLock);  // just in case

        ZS_LOG_DEBUG(log("step"))

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("step called cancel"))
          cancel();
          return;
        }

        if (!mServiceQuery) {
          message::PeerToBootstrapperServicesGetRequestPtr request = message::PeerToBootstrapperServicesGetRequest::create();
          DocumentPtr doc = request->encode();
          boost::shared_array<char> buffer = doc->write();
          mServiceQuery = IHTTP::post(mThisWeak.lock(), mUserAgent, mURI, buffer.get());

          mServiceRequester = IMessageRequester::monitorRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_REQUEST_TIMEOUT_IN_SEECONDS));

          if ((!mServiceQuery) ||
              (!mServiceRequester)) {
            ZS_LOG_ERROR(Basic, log("service query failed"))
            setLastError(IBootstrappedNetwork::BootstrappedNetworkError_InternalError);
            cancel();
            return;
          }
        }

        if ((mBootstrapperURI.isEmpty()) ||
            (mSaltURI.isEmpty())) {
          ZS_LOG_DEBUG(log(String("waiting for " + String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SERVICES_GET_REQUEST) + " to complete")))
          return;
        }

        if (!mFindersQuery) {
          message::PeerToBootstrapperFindersGetRequestPtr request = message::PeerToBootstrapperFindersGetRequest::create();
          DocumentPtr doc = request->encode();
          boost::shared_array<char> buffer = doc->write();
          mFindersQuery = IHTTP::post(mThisWeak.lock(), mUserAgent, String(mBootstrapperURI + HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST), buffer.get());

          mFindersRequester = IMessageRequester::monitorRequest(mThisWeak.lock(), request, Seconds(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_REQUEST_TIMEOUT_IN_SEECONDS));

          if ((!mFindersQuery) ||
              (!mFindersRequester)) {
            ZS_LOG_ERROR(Basic, log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_FINDERS_GET_REQUEST) + " query failed")))
            setLastError(IBootstrappedNetwork::BootstrappedNetworkError_InternalError);
            cancel();
            return;
          }
        }

        if (!mSRVPeerFinderFinal) {
          ZS_LOG_DEBUG(log("waiting for finder DNS lookups to complete"))
          return;
        }

        setCurrentState(IBootstrappedNetwork::BootstrappedNetworkState_Ready);
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::setCurrentState(IBootstrappedNetwork::BootstrappedNetworkStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("network state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        BootstrappedNetworkPtr pThis = mThisWeak.lock();

        if (pThis) {
          for (DelegateMap::iterator delIter = mDelegates.begin(); delIter != mDelegates.end(); )
          {
            DelegateMap::iterator current = delIter;
            ++delIter;

            IBootstrappedNetworkDelegatePtr &delegate = (*current).second;

            try {
              delegate->onBootstrappedNetworkStateChanged(pThis, mCurrentState);
            } catch(IBootstrappedNetworkDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
              mDelegates.erase(current);
            }
          }
        }

        if (isReady()) {
          String saltURL = mSaltURI + HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SALT_GET_REQUEST;

          PendingQueriesMap pending = mPendingQueries;
          mPendingQueries.clear();

          for (PendingQueriesMap::iterator iter = pending.begin(); iter != pending.end(); ++iter)
          {
            (*iter).second->notify(mUserAgent, saltURL);
          }
        }
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::setLastError(BootstrappedNetworkErrors error)
      {
        if (error == mLastError) return;

        ZS_LOG_ERROR(Detail, log("network error") + ", last error=" + toString(error) + ", was=" + toString(mLastError))

        mLastError = error;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Subscription
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetwork::Subscription::Subscription() :
        mID(zsLib::createPUID())
      {
      }

      //-----------------------------------------------------------------------
      BootstrappedNetwork::Subscription::~Subscription()
      {
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      BootstrappedNetwork::SubscriptionPtr BootstrappedNetwork::Subscription::create(BootstrappedNetworkPtr network)
      {
        SubscriptionPtr pThis(new Subscription);
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Subscription => IBootstrappedNetworkSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::Subscription::cancel()
      {
        AutoRecursiveLock lock(getLock());

        BootstrappedNetworkPtr outer = mOuter.lock();
        SubscriptionPtr pThis = mThisWeak.lock();
        if ((pThis) &&
            (outer)){
          outer->removeSubscription(pThis);
        }

        mOuter.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Subscription => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &BootstrappedNetwork::Subscription::getLock() const
      {
        BootstrappedNetworkPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Query
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetwork::Query::Query(
                                        IMessageQueuePtr queue,
                                        BootstrappedNetworkPtr outer,
                                        IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr delegate,
                                        UINT totalToFetch
                                        ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IBootstrappedNetworkFetchSignedSaltQueryDelegateProxy::createWeak(getAssociatedMessageQueue(), delegate)),
        mRequestingSalts(totalToFetch)
      {
      }

      //-----------------------------------------------------------------------
      BootstrappedNetwork::Query::~Query()
      {
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      BootstrappedNetwork::QueryPtr BootstrappedNetwork::Query::create(
                                                                       IMessageQueuePtr queue,
                                                                       BootstrappedNetworkPtr network,
                                                                       IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr delegate,
                                                                       UINT totalToFetch
                                                                       )
      {
        QueryPtr pThis(new Query(queue, network, delegate, totalToFetch));
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Query => IBootstrappedNetworkFetchSignedSaltQueryDelegate
      #pragma mark
      bool BootstrappedNetwork::Query::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) return true;
        if (mSaltBundles.size() > 0) return true;

        if (!mSaltQuery) return false;
        return mSaltQuery->isComplete();
      }

      //-----------------------------------------------------------------------
      UINT BootstrappedNetwork::Query::getTotalSignedSaltsAvailable() const
      {
        return mSaltBundles.size();
      }

      //-----------------------------------------------------------------------
      ElementPtr BootstrappedNetwork::Query::getNextSignedSalt()
      {
        AutoRecursiveLock lock(getLock());

        if (mSaltBundles.size() < 1) return ElementPtr();

        ElementPtr saltBundle = mSaltBundles.front();
        mSaltBundles.pop_front();

        return saltBundle;
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::Query::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (mSaltQuery) {
          mSaltQuery->cancel();
          mSaltQuery.reset();
        }

        BootstrappedNetworkPtr outer = mOuter.lock();
        QueryPtr pThis = mThisWeak.lock();

        if ((pThis) &&
            (outer)) {
          if (mDelegate) {
            try {
              mDelegate->onBootstrappedNetworkFetchSignedSaltComplete(outer, pThis);
            } catch (IBootstrappedNetworkFetchSignedSaltQueryDelegateProxy::Exceptions::DelegateGone &) {
            }
          }
          outer->removeQuery(pThis);
        }

        mOuter.reset();
        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Query => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::Query::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
        // ignored
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::Query::onHTTPComplete(IHTTPQueryPtr query)
      {
        AutoRecursiveLock lock(getLock());

        if (query != mSaltQuery) {
          ZS_LOG_WARNING(Detail, log("unknown query is ignored") + ", query=" + Stringize<PUID>(query->getID()).string())
          return;
        }

        if (!query->wasSuccessful()) {
          ZS_LOG_ERROR(Basic, log("salt request failed"))
          cancel();
          return;
        }

        String resultStr;
        query->readDataAsString(resultStr);

        DocumentPtr doc = Document::create();
        doc->parse(resultStr);

        message::MessagePtr message = message::Message::create(doc);

        if (message::Message::MessageType_Result != message->messageType()) {
          ZS_LOG_ERROR(Detail, log("salt message HTTP response received was not a result (ignored)"))
          cancel();
          return;
        }

        message::MessageResultPtr result = message::MessageResult::convert(message);
        if (result->hasAttribute(message::MessageResult::AttributeType_ErrorCode)) {
          ZS_LOG_ERROR(Detail, log("salt result received had an error") + ", error=" + Stringize<WORD>(result->errorCode()).string() + ", reason=" + result->errorReason())
          cancel();
          return;
        }

        if (message::MessageFactoryStack::Method_PeerToSalt_SignedSaltGet != (message::MessageFactoryStack::Methods)result->method()) {
          ZS_LOG_ERROR(Detail, log("salt result received was not of the expected type"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log(String(String(HOOKFLASH_STACK_BOOTSTRAPPEDNETWORK_SALT_GET_REQUEST) + " result received")))

        message::PeerToSaltSignedSaltGetResultPtr saltResult = message::PeerToSaltSignedSaltGetResult::convert(message);

        mSaltBundles = saltResult->saltBundles();

        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Query => friend BootstrappedNetwork
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::Query::notify(
                                              const String &userAgent,
                                              const String &saltURL
                                              )
      {
        ZS_THROW_BAD_STATE_IF(mSaltQuery)

        message::PeerToSaltSignedSaltGetRequestPtr request = message::PeerToSaltSignedSaltGetRequest::create();
        request->salts(mRequestingSalts);
        DocumentPtr doc = request->encode();
        boost::shared_array<char> buffer = doc->write();

        mSaltQuery = IHTTP::post(mThisWeak.lock(), userAgent, saltURL, buffer.get());
        if (!mSaltQuery) {
          ZS_LOG_ERROR(Basic, log("failed to create salt query"))
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork::Query => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &BootstrappedNetwork::Query::getLock() const
      {
        BootstrappedNetworkPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String BootstrappedNetwork::Query::log(const char *message) const
      {
        return String("BootstrappedNetwork::Query [") + Stringize<PUID>(mID).string() + "] " + message;
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
    #pragma mark IBootstrappedNetwork
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IBootstrappedNetwork::toString(BootstrappedNetworkStates state)
    {
      switch (state) {
        case BootstrappedNetworkState_Pending:      return "Pending";
        case BootstrappedNetworkState_Ready:        return "Ready";
        case BootstrappedNetworkState_ShuttingDown: return "Shutting down";
        case BootstrappedNetworkState_Shutdown:     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IBootstrappedNetwork::toString(BootstrappedNetworkErrors error)
    {
      switch (error) {
        case BootstrappedNetworkError_None:                         return "None";

        case BootstrappedNetworkError_InternalError:                return "Internal error";

        case BootstrappedNetworkError_HTTPRequestTimeout:           return "HTTP request timeout";
        case BootstrappedNetworkError_HTTPFailure:                  return "HTTP failure";
        case BootstrappedNetworkError_HTTPResponseNotUnderstood:    return "HTTP response was not understood";

        case BootstrappedNetworkError_PeerFinderDNSLookupFailure:   return "Peer finder DNS lookup failure";
      }

      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    IBootstrappedNetworkPtr IBootstrappedNetwork::create(
                                                         zsLib::IMessageQueuePtr queue,
                                                         IBootstrappedNetworkDelegatePtr delegate,
                                                         const char *userAgent,
                                                         const char *networkURI,
                                                         const char *turnServer,
                                                         const char *turnServerUsername,
                                                         const char *turnServerPassword,
                                                         const char *stunServer
                                                         )
    {
      return internal::BootstrappedNetwork::create(queue, delegate, userAgent, networkURI, turnServer, turnServerUsername, turnServerPassword, stunServer);
    }
  }
}
