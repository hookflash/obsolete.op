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

#include <openpeer/stack/internal/stack_ServiceNamespaceGrantSession.h>
#include <openpeer/stack/message/namespace-grant/NamespaceGrantWindowRequest.h>
#include <openpeer/stack/message/namespace-grant/NamespaceGrantWindowResult.h>
#include <openpeer/stack/message/namespace-grant/NamespaceGrantStartNotify.h>
#include <openpeer/stack/message/namespace-grant/NamespaceGrantCompleteNotify.h>

#include <openpeer/stack/internal/stack_BootstrappedNetwork.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/IHelper.h>
#include <openpeer/stack/message/IMessageHelper.h>
#include <openpeer/stack/internal/stack_Stack.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>


namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      using message::namespace_grant::NamespaceGrantWindowRequest;
      using message::namespace_grant::NamespaceGrantWindowRequestPtr;
      using message::namespace_grant::NamespaceGrantWindowResult;
      using message::namespace_grant::NamespaceGrantWindowResultPtr;
      using message::namespace_grant::NamespaceGrantStartNotify;
      using message::namespace_grant::NamespaceGrantStartNotifyPtr;
      using message::namespace_grant::NamespaceGrantCompleteNotify;
      using message::namespace_grant::NamespaceGrantCompleteNotifyPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::ServiceNamespaceGrantSession(
                                                                 IMessageQueuePtr queue,
                                                                 IServiceNamespaceGrantSessionDelegatePtr delegate,
                                                                 const char *outerFrameURLUponReload,
                                                                 const char *grantID
                                                                 ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(delegate ? IServiceNamespaceGrantSessionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate) : IServiceNamespaceGrantSessionDelegatePtr()),
        mCurrentState(SessionState_Pending),
        mLastError(0),
        mOuterFrameURLUponReload(outerFrameURLUponReload),
        mGrantID(grantID),
        mBrowserWindowReady(false),
        mBrowserWindowVisible(false),
        mBrowserWindowClosed(false),
        mNeedsBrowserWindowVisible(false),
        mReceivedNamespaceGrantCompleteNotify(false),
        mNamespaceGrantStartNotificationSent(false),
        mTotalWaits(0)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::~ServiceNamespaceGrantSession()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::init()
      {
      }

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSessionPtr ServiceNamespaceGrantSession::convert(IServiceNamespaceGrantSessionPtr session)
      {
        return boost::dynamic_pointer_cast<ServiceNamespaceGrantSession>(session);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => IServiceNamespaceGrantSession
      #pragma mark

      //-----------------------------------------------------------------------
      String ServiceNamespaceGrantSession::toDebugString(IServiceNamespaceGrantSessionPtr session, bool includeCommaPrefix)
      {
        if (!session) return includeCommaPrefix ? String(", peer contact session=(null)") : String("peer contact session=");
        return ServiceNamespaceGrantSession::convert(session)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSessionPtr ServiceNamespaceGrantSession::create(
                                                                           IServiceNamespaceGrantSessionDelegatePtr delegate,
                                                                           const char *outerFrameURLUponReload,
                                                                           const char *grantID
                                                                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!outerFrameURLUponReload)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantID)

        ServiceNamespaceGrantSessionPtr pThis(new ServiceNamespaceGrantSession(IStackForInternal::queueStack(), delegate, outerFrameURLUponReload, grantID));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::SessionStates ServiceNamespaceGrantSession::getState(
                                                                                         WORD *outLastErrorCode,
                                                                                         String *outLastErrorReason
                                                                                         ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }


      //-----------------------------------------------------------------------
      String ServiceNamespaceGrantSession::getGrantID() const
      {
        AutoRecursiveLock lock(getLock());
        return mGrantID;
      }
      
      //-----------------------------------------------------------------------
      String ServiceNamespaceGrantSession::getInnerBrowserWindowFrameURL() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mBootstrappedNetwork) return String();
        return mBootstrappedNetwork->forServices().getServiceURI("namespace-grant", "namespace-grant-inner-frame");
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::notifyBrowserWindowVisible()
      {
        AutoRecursiveLock lock(getLock());
        mBrowserWindowVisible = true;
        step();
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::notifyBrowserWindowClosed()
      {
        AutoRecursiveLock lock(getLock());
        mBrowserWindowClosed = true;
        step();
      }
      
      //-----------------------------------------------------------------------
      DocumentPtr ServiceNamespaceGrantSession::getNextMessageForInnerBrowerWindowFrame()
      {
        AutoRecursiveLock lock(getLock());
        if (mPendingMessagesToDeliver.size() < 1) return DocumentPtr();

        DocumentPtr result = mPendingMessagesToDeliver.front();
        mPendingMessagesToDeliver.pop_front();

        if (mDelegate) {
          if (mPendingMessagesToDeliver.size() > 0) {
            try {
              mDelegate->onServiceNamespaceGrantSessionPendingMessageForInnerBrowserWindowFrame(mThisWeak.lock());
            } catch (IServiceIdentitySessionDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
            }
          }
        }

        return result;
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::handleMessageFromInnerBrowserWindowFrame(DocumentPtr unparsedMessage)
      {
        typedef NamespaceGrantCompleteNotify::NamespaceGranthallengeBundleList NamespaceGranthallengeBundleList;
        MessagePtr message = Message::create(unparsedMessage, mThisWeak.lock());
        if (IMessageMonitor::handleMessageReceived(message)) {
          ZS_LOG_DEBUG(log("message handled via message monitor"))
          return;
        }

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot handle message when shutdown"))
          return;
        }

        NamespaceGrantWindowRequestPtr windowRequest = NamespaceGrantWindowRequest::convert(message);
        if (windowRequest) {
          ZS_LOG_DEBUG(log("received window request"))

          // send a result immediately
          NamespaceGrantWindowResultPtr result = NamespaceGrantWindowResult::create(windowRequest);
          sendInnerWindowMessage(result);

          if (windowRequest->ready()) {
            ZS_LOG_DEBUG(log("notifying browser window ready"))
            mBrowserWindowReady = true;
          }

          if (windowRequest->visible()) {
            ZS_LOG_DEBUG(log("notifying browser window needs to be made visible"))
            mNeedsBrowserWindowVisible = true;
          }

          IServiceNamespaceGrantSessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        NamespaceGrantCompleteNotifyPtr completeNotify = NamespaceGrantCompleteNotify::convert(message);
        if (completeNotify) {

          ZS_LOG_DEBUG(log("received namespace grant complete notification"))

          const NamespaceGranthallengeBundleList &bundles = completeNotify->bundles();

          for (NamespaceGranthallengeBundleList::const_iterator iter = bundles.begin(); iter != bundles.end(); ++iter)
          {
            ElementPtr bundleEl = (*iter);

            if (!bundleEl) {
              ZS_LOG_WARNING(Detail, log("bundle element returned from namespace grant complete was null (which is not legal)"))
              continue;
            }

            ElementPtr namespaceGrantChallengeEl = bundleEl->findFirstChildElement("namespaceGrantChallenge");
            if (!namespaceGrantChallengeEl) {
              ZS_LOG_WARNING(Detail, log("bundle element missing \"namespaceGrantChallenge\" from grant grant bundle (which is not legal)"))
              continue;
            }

            String challengeID = IMessageHelper::getAttributeID(namespaceGrantChallengeEl);

            bool found = false;

            for (QueryMap::iterator iterQuery = mQueriesInProcess.begin(); iterQuery != mQueriesInProcess.end();)
            {
              QueryMap::iterator iterQueryCurrent = iterQuery;
              ++iterQuery;

              QueryPtr query = (*iterQueryCurrent).second;

              if (query->getChallengeInfo().mID == challengeID) {
                ZS_LOG_DEBUG(log("found query that matches challenge ID") + ", challenge ID=" + challengeID)
                query->notifyComplete(bundleEl);
                mQueriesInProcess.erase(iterQueryCurrent);
                found = true;
                break;
              }
            }
            if (found) continue;

            ZS_LOG_WARNING(Detail, log("did not find query that matched challenge ID") + ", challenge ID=" + challengeID)
          }

          for (QueryMap::iterator iterQuery = mQueriesInProcess.begin(); iterQuery != mQueriesInProcess.end(); ++iterQuery)
          {
            QueryPtr query = (*iterQuery).second;
            ZS_LOG_WARNING(Detail, log("did not find signed bundle for challenge ID") + ", challenge ID=" + query->getChallengeInfo().mID)
            query->notifyComplete(ElementPtr());
          }
          mQueriesInProcess.clear();

          mReceivedNamespaceGrantCompleteNotify = true;

          IServiceNamespaceGrantSessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        if ((message->isRequest()) ||
            (message->isNotify())) {
          ZS_LOG_WARNING(Debug, log("request was not understood"))
          MessageResultPtr result = MessageResult::create(message, IHTTP::HTTPStatusCode_NotImplemented);
          sendInnerWindowMessage(result);
          return;
        }
        
        ZS_LOG_WARNING(Detail, log("message result ignored since it was not monitored"))
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        if (mNamespaceGrantValidateMonitor) {
          mNamespaceGrantValidateMonitor->cancel();
          mNamespaceGrantValidateMonitor.reset();
        }

        setState(SessionState_Shutdown);

        // cancel all queries in progress
        {
          for (QueryMap::iterator iterQuery = mQueriesInProcess.begin(); iterQuery != mQueriesInProcess.end(); ++iterQuery)
          {
            QueryPtr query = (*iterQuery).second;
            ZS_LOG_WARNING(Detail, log("did not find signed bundle for challenge ID") + ", challenge ID=" + query->getChallengeInfo().mID)
            query->notifyComplete(ElementPtr());
          }
          mQueriesInProcess.clear();
        }

        // cancel all pending queries
        {
          for (QueryMap::iterator iterQuery = mPendingQueries.begin(); iterQuery != mPendingQueries.end(); ++iterQuery)
          {
            QueryPtr query = (*iterQuery).second;
            ZS_LOG_WARNING(Detail, log("did not find signed bundle for challenge ID") + ", challenge ID=" + query->getChallengeInfo().mID)
            query->notifyComplete(ElementPtr());
          }
          mPendingQueries.clear();
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => IMessageSource
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => IServiceNamespaceGrantSessionForServices
      #pragma mark

      //-----------------------------------------------------------------------
      IServiceNamespaceGrantSessionForServicesWaitPtr ServiceNamespaceGrantSession::obtainWaitToProceed(
                                                                                                        IServiceNamespaceGrantSessionForServicesWaitForWaitDelegatePtr waitForWaitUponFailingToObtainDelegate
                                                                                                        )
      {
        AutoRecursiveLock lock(getLock());

        {
          if (isShutdown()) {
            ZS_LOG_WARNING(Detail, log("obtained lock during shutdown (legal, but frowned upon)"))

            WaitPtr wait = Wait::create(mThisWeak.lock());
            ++mTotalWaits;
            return wait;
          }

          if (mQueriesInProcess.size() > 0) {
            ZS_LOG_DEBUG(log("cannot obtain lock while queries are in progress"))
            goto failure_to_obtain;
          }
          if (mBrowserWindowReady) {
            ZS_LOG_DEBUG(log("cannot obtain lock while browser window is active"))
            goto failure_to_obtain;
          }

          WaitPtr wait = Wait::create(mThisWeak.lock());
          ++mTotalWaits;
          return wait;
        }

      failure_to_obtain:

        if (waitForWaitUponFailingToObtainDelegate) {
          ZS_LOG_DEBUG(log("will inform delegate when it can try to obtain another wait lock again"))
          mWaitingDelegates.push_back(IServiceNamespaceGrantSessionForServicesWaitForWaitDelegateProxy::createWeak(IStackForInternal::queueDelegate(), waitForWaitUponFailingToObtainDelegate));
        }

        return IServiceNamespaceGrantSessionForServicesWaitPtr();
      }

      //-----------------------------------------------------------------------
      IServiceNamespaceGrantSessionForServicesQueryPtr ServiceNamespaceGrantSession::query(
                                                                                           IServiceNamespaceGrantSessionForServicesQueryDelegatePtr delegate,
                                                                                           const NamespaceGrantChallengeInfo &challengeInfo,
                                                                                           const NamespaceInfoMap &namespaces
                                                                                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        AutoRecursiveLock lock(getLock());
        QueryPtr query = Query::create(mThisWeak.lock(), IServiceNamespaceGrantSessionForServicesQueryDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate), challengeInfo, namespaces);

        if (isShutdown()) {
          query->notifyComplete(ElementPtr());
          return query;
        }

        mPendingQueries[query->getID()] = query;
        return query;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::isNamespaceURLInNamespaceGrantChallengeBundle(
                                                                                       ElementPtr bundleEl,
                                                                                       const char *namespaceURL
                                                                                       ) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!namespaceURL)

        if (!bundleEl) {
          ZS_LOG_WARNING(Detail, log("challenge bundle passed in is null so it cannot contain the namespace"))
          return false;
        }

        ElementPtr namespaceGrantChallengeEl = bundleEl->findFirstChildElement("namespaceGrantChallenge");
        if (!namespaceGrantChallengeEl) {
          ZS_LOG_WARNING(Detail, log("missing \"namespaceGrantChallenge\" inside challenge bundle thus cannot contain a namespace"))
          return false;
        }

        ElementPtr namespacesEl = namespaceGrantChallengeEl->findFirstChildElement("namespaces");
        if (!namespacesEl) {
          ZS_LOG_WARNING(Detail, log("missing \"namespaces\" inside challenge bundle thus cannot contain a namespace"))
          return false;
        }
        ElementPtr namespaceEl = namespacesEl->findFirstChildElement("namespace");
        while (namespaceEl) {
          String namespaceID = IMessageHelper::getAttributeID(namespaceEl);
          if (namespaceID == namespaceURL) {
            ZS_LOG_TRACE(log("found namespace inside challenge bundle") + ", namespace=" + namespaceURL)
            return true;
          }
          namespaceEl = namespaceEl->findNextSiblingElement("namespace");
        }
        ZS_LOG_WARNING(Debug, log("did not find namespace inside challenge bundle") + ", namespace=" + namespaceURL)
        return false;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => IServiceNamespaceGrantSessionAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::onStep()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("on step"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork)
      {
        ZS_LOG_DEBUG(log("bootstrapper reported complete"))

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => friend class Query
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::notifyQueryGone(PUID queryID)
      {
        ZS_LOG_WARNING(Debug, log("removing query") + ", query ID=" + Stringize<typeof(queryID)>(queryID).string())

        AutoRecursiveLock lock(getLock());
        {
          QueryMap::iterator found = mQueriesInProcess.find(queryID);
          if (found != mQueriesInProcess.end()) {
            mQueriesInProcess.erase(found);
            return;
          }
        }
        {
          QueryMap::iterator found = mPendingQueries.find(queryID);
          if (found != mPendingQueries.end()) {
            mPendingQueries.erase(found);
            return;
          }
        }

        ZS_LOG_WARNING(Debug, log("query already gone") + ", query ID=" + Stringize<typeof(queryID)>(queryID).string())
        return;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => friend class Wait
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::notifyWaitGone(PUID waitID)
      {
        ZS_LOG_WARNING(Debug, log("removing wait") + ", wait ID=" + Stringize<typeof(waitID)>(waitID).string())

        AutoRecursiveLock lock(getLock());

        ZS_THROW_BAD_STATE_IF(0 == mTotalWaits)

        --mTotalWaits;
        if (0 != mTotalWaits) return;

        ServiceNamespaceGrantSessionPtr pThis = mThisWeak.lock();
        if (pThis) {
          for (WaitingDelegateList::iterator iter = mWaitingDelegates.begin(); iter != mWaitingDelegates.end(); ++iter)
          {
            try {
              IServiceNamespaceGrantSessionForServicesWaitForWaitDelegatePtr delegate = (*iter);
              delegate->onServiceNamespaceGrantSessionForServicesWaitComplete(pThis);
            } catch (IServiceNamespaceGrantSessionForServicesQueryDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
            }
          }
        }

        mWaitingDelegates.clear();

        IServiceNamespaceGrantSessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ServiceNamespaceGrantSession::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String ServiceNamespaceGrantSession::log(const char *message) const
      {
        return String("ServiceNamespaceGrantSession [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ServiceNamespaceGrantSession::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("namespace grant id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("delegate", mDelegate ? String("true") : String() , firstTime) +
               IBootstrappedNetwork::toDebugString(mBootstrappedNetwork) +
               Helper::getDebugValue("namespace grant validate", mNamespaceGrantValidateMonitor ? String("true") : String() , firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("error code", 0 != mLastError ? Stringize<typeof(mLastError)>(mLastError).string() : String(), firstTime) +
               Helper::getDebugValue("error reason", mLastErrorReason, firstTime) +
               Helper::getDebugValue("grant ID", mGrantID, firstTime) +
               Helper::getDebugValue("browser window ready", mBrowserWindowReady ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser window visible", mBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser window closed", mBrowserWindowClosed ? String("true") : String(), firstTime) +
               Helper::getDebugValue("need browser window visible", mNeedsBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("namespace grant start notification", mNamespaceGrantStartNotificationSent ? String("true") : String(), firstTime) +
               Helper::getDebugValue("received namespace grant complete notification", mReceivedNamespaceGrantCompleteNotify ? String("true") : String(), firstTime) +
               Helper::getDebugValue("pending messages", mPendingMessagesToDeliver.size() > 0 ? Stringize<DocumentList::size_type>(mPendingMessagesToDeliver.size()) : String(), firstTime) +
               Helper::getDebugValue("total waits", 0 != mTotalWaits ? Stringize<typeof(mTotalWaits)>(mTotalWaits) : String(), firstTime) +
               Helper::getDebugValue("queries in process", mQueriesInProcess.size() > 0 ? Stringize<QueryMap::size_type>(mQueriesInProcess.size()) : String(), firstTime) +
               Helper::getDebugValue("pending queries", mPendingQueries.size() > 0 ? Stringize<QueryMap::size_type>(mPendingQueries.size()) : String(), firstTime) +
               Helper::getDebugValue("waiting delegates", mWaitingDelegates.size() > 0 ? Stringize<WaitingDelegateList::size_type>(mWaitingDelegates.size()) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step - already shutdown"))
          cancel();
          return;
        }

        if (!stepWaitForServices()) goto post_step;
        if (!stepPrepareQueries()) goto post_step;
        if (!stepBootstrapper()) goto post_step;
        if (!stepLoadGrantWindow()) goto post_step;
        if (!stepMakeGrantWindowVisible()) goto post_step;
        if (!stepSendNamespaceGrantStartNotification()) goto post_step;
        if (!stepWaitForPermission()) goto post_step;
        if (!stepCloseBrowserWindow()) goto post_step;

        setState(SessionState_Ready);

        if (!stepRestart()) goto post_step;

      post_step:
        postStep();
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepWaitForServices()
      {
        if (0 == mTotalWaits) {
          ZS_LOG_DEBUG(log("no services have asked for this session to wait"))
          return true;
        }

        ZS_LOG_DEBUG(log("waiting for all services to register their queries with the namespace grant service"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepPrepareQueries()
      {
        typedef String Domain;
        typedef ULONG Usage;
        typedef std::map<Domain, Usage> DomainUsageMap;
        typedef IHelper::SplitMap SplitMap;

        if (mQueriesInProcess.size() > 0) {
          ZS_LOG_DEBUG(log("queries have already been prepared"))
          return true;
        }

        if (mBrowserWindowReady) {
          ZS_LOG_DEBUG(log("cannot prepare queries while browser window is still active"))
          return true;
        }

        if (mPendingQueries.size() < 1) {
          ZS_LOG_DEBUG(log("no queries are pending so nothing to do"))
          return true;
        }

        String mostFoundDomain;
        Usage mostFoundUsage = 0;

        DomainUsageMap domainUsages;

        for (QueryMap::iterator iter = mPendingQueries.begin(); iter != mPendingQueries.end(); ++iter)
        {
          QueryPtr query = (*iter).second;

          SplitMap domains;
          IHelper::split(query->getChallengeInfo().mDomains, domains, ',');

          for (SplitMap::iterator iterSplit = domains.begin(); iterSplit != domains.end(); ++iterSplit) {
            String domain = (*iterSplit).second;
            if (!IHelper::isValidDomain(domain)) {
              ZS_LOG_WARNING(Debug, log("told to use invalid domain") + ", domain=" + domain)
              continue;
            }
            DomainUsageMap::iterator found = domainUsages.find(domain);
            if (found == domainUsages.end()) {
              ZS_LOG_DEBUG(log("found first usage of domain") + ", domain=" + domain)
              domainUsages[domain] = 1;
              if (0 == mostFoundUsage) {
                mostFoundUsage = 1;
                mostFoundDomain = domain;
              }
              continue;
            }

            Usage &usage = (*found).second;
            ++usage;
            if (usage > mostFoundUsage) {
              mostFoundUsage = usage;
              mostFoundDomain = domain;
            }

            ZS_LOG_DEBUG(log("found another usage of domain") + ", domain=" + domain + ", total=" + Stringize<typeof(usage)>(usage).string())
          }
        }

        ZS_LOG_DEBUG(log("most found domain") + ", domain=" + mostFoundDomain)

        if (mostFoundDomain.isEmpty()) {
          ZS_LOG_WARNING(Detail, log("cannot satisfy any of the pending domain because no usageable domain was found"))

          // cancel all pending queries
          for (QueryMap::iterator iterQuery = mPendingQueries.begin(); iterQuery != mPendingQueries.end(); ++iterQuery)
          {
            QueryPtr query = (*iterQuery).second;
            ZS_LOG_WARNING(Detail, log("did not find signed bundle for challenge ID") + ", challenge ID=" + query->getChallengeInfo().mID)
            query->notifyComplete(ElementPtr());
          }
          mPendingQueries.clear();
          return true;
        }

        for (QueryMap::iterator iter = mPendingQueries.begin(); iter != mPendingQueries.end(); )
        {
          QueryMap::iterator current = iter;
          ++iter;

          QueryPtr query = (*current).second;

          SplitMap domains;
          IHelper::split(query->getChallengeInfo().mDomains, domains, ',');

          for (SplitMap::iterator iterSplit = domains.begin(); iterSplit != domains.end(); ++iterSplit) {
            String domain = (*iterSplit).second;
            if (domain == mostFoundDomain) {
              ZS_LOG_DEBUG(log("query is going to execute now") + ", domain=" + domain + ", query ID=" + Stringize<PUID>(query->getID()).string())
              mQueriesInProcess[query->getID()] = query;
              mPendingQueries.erase(current);
              break;
            }
          }
        }

        if (mPendingQueries.size() < 1) {
          ZS_LOG_DEBUG(log("all queries will now execute"))
        }

        mBootstrappedNetwork = IBootstrappedNetworkForServices::prepare(mostFoundDomain, mThisWeak.lock());
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepBootstrapper()
      {
        if (!mBootstrappedNetwork) {
          ZS_LOG_DEBUG(log("no boostrapped network thus nothing to do"))
          return true;
        }

        if (!mBootstrappedNetwork->forServices().isPreparationComplete()) {
          setState(SessionState_Pending);

          ZS_LOG_DEBUG(log("waiting for preparation of lockbox bootstrapper to complete"))
          return false;
        }

        WORD errorCode = 0;
        String reason;

        if (mBootstrappedNetwork->forServices().wasSuccessful(&errorCode, &reason)) {
          ZS_LOG_DEBUG(log("lockbox bootstrapper was successful"))
          return true;
        }

        ZS_LOG_ERROR(Detail, log("bootstrapped network failed for lockbox") + ", error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", reason=" + reason)

        setError(errorCode, reason);
        cancel();
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepLoadGrantWindow()
      {
        if (!mBootstrappedNetwork) {
          ZS_LOG_DEBUG(log("no boostrapped network thus nothing to do"))
          return true;
        }

        if (mBrowserWindowReady) {
          ZS_LOG_DEBUG(log("grant window is ready"))
          return true;
        }

        setState(SessionState_WaitingForBrowserWindowToBeLoaded);
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepMakeGrantWindowVisible()
      {
        if (!mBrowserWindowReady) {
          ZS_LOG_DEBUG(log("all required namespaces have been granted"))
          return true;
        }

        if (!mNeedsBrowserWindowVisible) {
          ZS_LOG_DEBUG(log("browser window does not need to be visible"))
          return true;
        }

        if (mBrowserWindowVisible) {
          ZS_LOG_DEBUG(log("grant window is visible"))
          return true;
        }

        setState(SessionState_WaitingForBrowserWindowToBeMadeVisible);
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepSendNamespaceGrantStartNotification()
      {
        typedef NamespaceGrantStartNotify::NamespaceGrantChallengeInfoAndNamespacesList NamespaceGrantChallengeInfoAndNamespacesList;
        typedef NamespaceGrantStartNotify::NamespaceGrantChallengeInfoAndNamespaces NamespaceGrantChallengeInfoAndNamespaces;

        if (!mBrowserWindowReady) {
          ZS_LOG_DEBUG(log("all required namespaces have been granted"))
          return true;
        }

        if (mNamespaceGrantStartNotificationSent) {
          ZS_LOG_DEBUG(log("browser window namespace grant start notification already sent"))
          return true;
        }

        setState(SessionState_Pending);

        NamespaceGrantStartNotifyPtr request = NamespaceGrantStartNotify::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        NamespaceGrantChallengeInfoAndNamespacesList process;

        for (QueryMap::iterator iter = mQueriesInProcess.begin(); iter != mQueriesInProcess.end(); ++iter)
        {
          QueryPtr query = (*iter).second;

          NamespaceGrantChallengeInfoAndNamespaces data(query->getChallengeInfo(), query->getNamespaces());

          ZS_LOG_DEBUG(log("going to process challenge") + ", challenge ID=" + query->getChallengeInfo().mID)

          process.push_back(NamespaceGrantChallengeInfoAndNamespaces(query->getChallengeInfo(), query->getNamespaces()));
        }

        request->challenges(process);

        request->outerFrameURL(mOuterFrameURLUponReload);
        request->popup(false);
        request->browserVisibility(NamespaceGrantStartNotify::BrowserVisibility_VisibleOnDemand);

        mNamespaceGrantStartNotificationSent = true;

        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepWaitForPermission()
      {
        if (!mBrowserWindowReady) {
          ZS_LOG_DEBUG(log("all required namespaces have been granted"))
          return true;
        }

        if (mReceivedNamespaceGrantCompleteNotify) {
          ZS_LOG_DEBUG(log("received grant complete notification"))
          return true;
        }

        ZS_LOG_DEBUG(log("waiting for permission to be granted to the lockbox namespaces"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepCloseBrowserWindow()
      {
        if (!mBrowserWindowReady) {
          ZS_LOG_DEBUG(log("all required namespaces have been granted"))
          return true;
        }

        if (mBrowserWindowClosed) {
          ZS_LOG_DEBUG(log("browser window has been closed"))
          return true;
        }

        ZS_LOG_DEBUG(log("waiting for browser window to close"))
        setState(SessionState_WaitingForBrowserWindowToClose);

        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepRestart()
      {
        ZS_LOG_DEBUG(log("reseting namespace grant session to allow another query session to start"))

        mBootstrappedNetwork.reset();

        mBrowserWindowReady = false;
        mBrowserWindowVisible = false;
        mBrowserWindowClosed = false;

        mNeedsBrowserWindowVisible = false;
        mNamespaceGrantStartNotificationSent = false;
        mReceivedNamespaceGrantCompleteNotify = false;

        ZS_THROW_BAD_STATE_IF(mQueriesInProcess.size() > 0)
        ZS_THROW_BAD_STATE_IF(0 != mTotalWaits)

        if (mPendingQueries.size() > 0) {
          IServiceNamespaceGrantSessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::postStep()
      {
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::setState(SessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState) + getDebugValueString())
        mCurrentState = state;

        ServiceNamespaceGrantSessionPtr pThis = mThisWeak.lock();
        if ((pThis) &&
            (mDelegate)) {
          try {
            ZS_LOG_DEBUG(log("attempting to report state to delegate") + getDebugValueString())
            mDelegate->onServiceNamespaceGrantSessionStateChanged(pThis, mCurrentState);
          } catch (IServiceNamespaceGrantSessionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());

        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }
        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("erorr already set thus ignoring new error") + ", new error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", new reason=" + reason + getDebugValueString())
          return;
        }

        mLastError = errorCode;
        mLastErrorReason = reason;
        ZS_LOG_ERROR(Detail, log("error set") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::sendInnerWindowMessage(MessagePtr message)
      {
        DocumentPtr doc = message->encode();
        mPendingMessagesToDeliver.push_back(doc);

        if (1 != mPendingMessagesToDeliver.size()) {
          ZS_LOG_DEBUG(log("already had previous messages to deliver, no need to send another notification"))
          return;
        }

        ServiceNamespaceGrantSessionPtr pThis = mThisWeak.lock();

        if ((pThis) &&
            (mDelegate)) {
          try {
            ZS_LOG_DEBUG(log("attempting to notify of message to browser window needing to be delivered"))
            mDelegate->onServiceNamespaceGrantSessionPendingMessageForInnerBrowserWindowFrame(pThis);
          } catch(IServiceIdentitySessionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Wait
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::Wait::Wait(ServiceNamespaceGrantSessionPtr outer) :
        mID(zsLib::createPUID()),
        mOuter(outer)
      {
      }

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::Wait::~Wait()
      {
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Wait => IServiceNamespaceGrantSessionForServicesWait
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::Wait::cancel()
      {

        ServiceNamespaceGrantSessionPtr outer;
        {
          AutoRecursiveLock lock(mLock);
          outer = mOuter;
          mOuter.reset();
        }

        if (!outer) return;

        outer->notifyWaitGone(mID);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Wait => friend class ServiceNamespaceGrantSession
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::WaitPtr ServiceNamespaceGrantSession::Wait::create(ServiceNamespaceGrantSessionPtr outer)
      {
        WaitPtr pThis(new Wait(outer));
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Query
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::Query::Query(
                                                 ServiceNamespaceGrantSessionPtr outer,
                                                 IServiceNamespaceGrantSessionForServicesQueryDelegatePtr delegate,
                                                 const NamespaceGrantChallengeInfo &challengeInfo,
                                                 const NamespaceInfoMap &namespaces
                                                 ) :
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(delegate),
        mChallengeInfo(challengeInfo),
        mNamespaces(namespaces)
      {
      }

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::Query::~Query()
      {
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Query => IServiceNamespaceGrantSessionForServices
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::Query::cancel()
      {
        AutoRecursiveLock lock(getLock());

        notifyComplete(ElementPtr());

        ServiceNamespaceGrantSessionPtr outer = mOuter.lock();

        if (outer) {
          outer->notifyQueryGone(mID);
          mOuter.reset();
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::Query::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return (bool)(!mDelegate);
      }

      //-----------------------------------------------------------------------
      ElementPtr ServiceNamespaceGrantSession::Query::getNamespaceGrantChallengeBundle() const
      {
        AutoRecursiveLock lock(getLock());
        return mNamespaceGrantChallengeBundleEl;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Subscription => friend class ServiceNamespaceGrantSession
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::QueryPtr ServiceNamespaceGrantSession::Query::create(
                                                                                         ServiceNamespaceGrantSessionPtr outer,
                                                                                         IServiceNamespaceGrantSessionForServicesQueryDelegatePtr delegate,
                                                                                         const NamespaceGrantChallengeInfo &challengeInfo,
                                                                                         const NamespaceInfoMap &namespaces
                                                                                         )
      {
        QueryPtr pThis(new Query(outer, delegate, challengeInfo, namespaces));
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::Query::notifyComplete(ElementPtr bundleEl)
      {
        AutoRecursiveLock lock(getLock());

        mNamespaceGrantChallengeBundleEl = bundleEl;

        if (!mDelegate) return;

        QueryPtr pThis = mThisWeak.lock();
        if (!pThis) return;

        try {
          mDelegate->onServiceNamespaceGrantSessionForServicesQueryComplete(pThis, bundleEl);
        } catch(IServiceNamespaceGrantSessionForServicesQueryDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, "ServiceNamespaceGrantSession::Query [] delegate gone")
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Query => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ServiceNamespaceGrantSession::Query::getLock() const
      {
        ServiceNamespaceGrantSessionPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
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
    #pragma mark IServiceNamespaceGrantSession
    #pragma mark


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceNamespaceGrantSession
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IServiceNamespaceGrantSession::toString(SessionStates state)
    {
      switch (state)
      {
        case SessionState_Pending:                                return "Pending";
        case SessionState_WaitingForBrowserWindowToBeLoaded:      return "Waiting for Browser Window to be Loaded";
        case SessionState_WaitingForBrowserWindowToBeMadeVisible: return "Waiting for Browser Window to be Made Visible";
        case SessionState_WaitingForBrowserWindowToClose:         return "Waiting for Browser Window to Close";
        case SessionState_Ready:                                  return "Ready";
        case SessionState_Shutdown:                               return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    String IServiceNamespaceGrantSession::toDebugString(IServiceNamespaceGrantSessionPtr session, bool includeCommaPrefix)
    {
      return internal::ServiceNamespaceGrantSession::toDebugString(session, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IServiceNamespaceGrantSessionPtr IServiceNamespaceGrantSession::create(
                                                                           IServiceNamespaceGrantSessionDelegatePtr delegate,
                                                                           const char *outerFrameURLUponReload,
                                                                           const char *grantID
                                                                           )
    {
      return internal::IServiceNamespaceGrantSessionFactory::singleton().create(delegate, outerFrameURLUponReload, grantID);
    }

  }
}
