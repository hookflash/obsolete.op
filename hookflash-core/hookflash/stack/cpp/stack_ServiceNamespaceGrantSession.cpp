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

#include <hookflash/stack/internal/stack_ServiceNamespaceGrantSession.h>
#include <hookflash/stack/message/namespace-grant/NamespaceGrantWindowRequest.h>
#include <hookflash/stack/message/namespace-grant/NamespaceGrantWindowResult.h>
#include <hookflash/stack/message/namespace-grant/NamespaceGrantStartNotify.h>
#include <hookflash/stack/message/namespace-grant/NamespaceGrantCompleteNotify.h>
#include <hookflash/stack/message/namespace-grant/NamespaceGrantValidateRequest.h>

#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/internal/stack_Stack.h>

#include <zsLib/Log.h>
//#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>


#define HOOKFLASH_STACK_SERVICE_NAMESPACE_GRANT_TIMEOUT_IN_SECONDS (60*2)

#define HOOKFLASH_STACK_SERVICE_NAMESPACE_GRANT_TIME_BEFORE_EXPIRY_MUST_REFRESH_GRANT_IN_SECONDS (2*60)
#define HOOKFLASH_STACK_SERVICE_NAMESPACE_GRANT_MIN_TIME_BETWEEN_VALIDATION_REQUESTS_IN_SECONDS (60*60*24)

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      using message::namespace_grant::NamespaceGrantWindowRequest;
      using message::namespace_grant::NamespaceGrantWindowRequestPtr;
      using message::namespace_grant::NamespaceGrantWindowResult;
      using message::namespace_grant::NamespaceGrantWindowResultPtr;
      using message::namespace_grant::NamespaceGrantStartNotify;
      using message::namespace_grant::NamespaceGrantStartNotifyPtr;
      using message::namespace_grant::NamespaceGrantCompleteNotify;
      using message::namespace_grant::NamespaceGrantCompleteNotifyPtr;
      using message::namespace_grant::NamespaceGrantValidateRequest;
      using message::namespace_grant::NamespaceGrantValidateRequestPtr;
      using message::namespace_grant::NamespaceGrantValidateResult;
      using message::namespace_grant::NamespaceGrantValidateResultPtr;

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
                                                                 BootstrappedNetworkPtr network,
                                                                 IServiceNamespaceGrantSessionDelegatePtr delegate,
                                                                 const char *outerFrameURLUponReload,
                                                                 const char *grantID,
                                                                 const char *grantSecret
                                                                 ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(delegate ? IServiceNamespaceGrantSessionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate) : IServiceNamespaceGrantSessionDelegatePtr()),
        mBootstrappedNetwork(network),
        mCurrentState(SessionState_Pending),
        mLastError(0),
        mOuterFrameURLUponReload(outerFrameURLUponReload),
        mRedoGrantBrowserProcessIfNeeded(true),
        mNotifiedAssociatedToAllServicesComplete(false),
        mBrowserWindowReady(false),
        mBrowserWindowVisible(false),
        mBrowserWindowClosed(false),
        mNeedsBrowserWindowVisible(false),
        mTotalNamesapceValidationsIssued(0),
        mReceivedNamespaceGrantCompleteNotify(false),
        mNamespaceGrantStartNotificationSent(false),
        mHasGrantedAllRequiredNamespaces(false)
      {
        mGrantInfo.mID = String(grantID);
        mGrantInfo.mSecret = String(grantSecret);

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
        IBootstrappedNetworkForServices::prepare(mBootstrappedNetwork->forServices().getDomain(), mThisWeak.lock());
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
                                                                           IServiceNamespaceGrantPtr serviceNamespaceGrant,
                                                                           const char *outerFrameURLUponReload,
                                                                           const char *grantID,
                                                                           const char *grantSecret
                                                                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!serviceNamespaceGrant)
        ZS_THROW_INVALID_ARGUMENT_IF(!outerFrameURLUponReload)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantID)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantSecret)

        ServiceNamespaceGrantSessionPtr pThis(new ServiceNamespaceGrantSession(IStackForInternal::queueStack(), BootstrappedNetwork::convert(serviceNamespaceGrant), delegate, outerFrameURLUponReload, grantID, grantSecret));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IServiceNamespaceGrantPtr ServiceNamespaceGrantSession::getService() const
      {
        AutoRecursiveLock lock(getLock());
        return mBootstrappedNetwork;
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
        return mGrantInfo.mID;
      }
      
      //-----------------------------------------------------------------------
      String ServiceNamespaceGrantSession::getGrantSecret() const
      {
        AutoRecursiveLock lock(getLock());
        return mGrantInfo.mID;
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::notifyAssocaitedToAllServicesComplete()
      {
        AutoRecursiveLock lock(getLock());
        mNotifiedAssociatedToAllServicesComplete = true;
        step();
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

          const GrantInfo &grantInfo = completeNotify->grantInfo();
          mGrantInfo.mergeFrom(grantInfo, true);

          mNamespacesGranted = completeNotify->namespaceInfos();

          if (mNamespacesGranted.size() < 1) {
            ZS_LOG_WARNING(Detail, log("no namespaces were not granted from namespace grant complete notification"))
            setError(IHTTP::HTTPStatusCode_Forbidden, "Namespaces were not granted");
            cancel();
            return;
          }

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

        mSubscriptions.clear();
        
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
      IServiceNamespaceGrantSessionForServicesSubscriptionPtr ServiceNamespaceGrantSession::subscribe(IServiceNamespaceGrantSessionForServicesDelegatePtr delegate)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        AutoRecursiveLock lock(getLock());
        SubscriptionPtr subscription = Subscription::create(mThisWeak.lock(), IServiceNamespaceGrantSessionForServicesDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate));

        mSubscriptions[subscription->getID()] = subscription;
        subscription->notifyStateChanged(mCurrentState);
        return subscription;
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::grantNamespaces(const NamespaceInfoMap &namespaces)
      {
        if (namespaces.size() < 1) return;

        AutoRecursiveLock lock(getLock());

        bool needGranting = false;

        for (NamespaceInfoMap::const_iterator iter = namespaces.begin(); iter != namespaces.end(); ++iter)
        {
          const NamespaceInfo &info = (*iter).second;
          if (info.mURL.hasData()) {
            mNamespacesToGrant[info.mURL] = info;

            if (mNamespacesGranted.find(info.mURL) == mNamespacesGranted.end()) {
              needGranting = true;
            }
          }
        }

        if (needGranting) {
          mHasGrantedAllRequiredNamespaces = false;
          mRedoGrantBrowserProcessIfNeeded = true;

          IServiceNamespaceGrantSessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
        }
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::isNamespaceGranted(const char *namespaceURL) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!namespaceURL)
        AutoRecursiveLock lock(getLock());

        NamespaceInfoMap::const_iterator found = mNamespacesGranted.find(namespaceURL);
        if (found == mNamespacesGranted.end()) {
          ZS_LOG_WARNING(Trace, log("namespace is not granted") + ", namespace=" + namespaceURL)
          return false;
        }
        ZS_LOG_DEBUG(log("namespace is not granted") + ", namespace=" + namespaceURL)
        return true;
      }

      //-----------------------------------------------------------------------
      GrantInfo ServiceNamespaceGrantSession::getGrantInfo() const
      {
        AutoRecursiveLock lock(getLock());
        return mGrantInfo;
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
      #pragma mark ServiceNamespaceGrantSession => IMessageMonitorResultDelegate<LockboxAccessResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::handleMessageMonitorResultReceived(
                                                                            IMessageMonitorPtr monitor,
                                                                            NamespaceGrantValidateResultPtr result
                                                                            )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mNamespaceGrantValidateMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mNamespaceGrantValidateMonitor->cancel();
        mNamespaceGrantValidateMonitor.reset();

        GrantInfo grantInfo = result->grantInfo();
        mGrantInfo.mergeFrom(grantInfo, true);

        mNamespacesGranted = result->namespaceInfos();

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::handleMessageMonitorErrorResultReceived(
                                                                                 IMessageMonitorPtr monitor,
                                                                                 NamespaceGrantValidateResultPtr ignore, // will always be NULL
                                                                                 message::MessageResultPtr result
                                                                                 )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mNamespaceGrantValidateMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mNamespaceGrantValidateMonitor->cancel();
        mNamespaceGrantValidateMonitor.reset();

        ZS_LOG_WARNING(Detail, log("namespace grant failed"))

        if (mTotalNamesapceValidationsIssued > 1) {
          ZS_LOG_ERROR(Detail, log("namespace grant failed to obtain refresh"))
          setError(result->errorCode(), result->errorReason());
          cancel();
          return true;
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession => friend class Subscription
      #pragma mark

      void ServiceNamespaceGrantSession::notifySubscriptionGone(PUID subscriptionID)
      {
        AutoRecursiveLock lock(getLock());
        SubscriptionMap::iterator found = mSubscriptions.find(subscriptionID);
        if (found == mSubscriptions.end()) {
          ZS_LOG_WARNING(Debug, log("subscription already gone") + ", subscription ID=" + Stringize<typeof(subscriptionID)>(subscriptionID).string())
          return;
        }
        ZS_LOG_WARNING(Debug, log("removing subscription") + ", subscription ID=" + Stringize<typeof(subscriptionID)>(subscriptionID).string())
        mSubscriptions.erase(found);
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
               mGrantInfo.getDebugValueString() +
               Helper::getDebugValue("redo browser grant process", mRedoGrantBrowserProcessIfNeeded ? String("true") : String(), firstTime) +
               Helper::getDebugValue("notified associated to all services complete", mNotifiedAssociatedToAllServicesComplete ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser window ready", mBrowserWindowReady ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser window visible", mBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser window closed", mBrowserWindowClosed ? String("true") : String(), firstTime) +
               Helper::getDebugValue("need browser window visible", mNeedsBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("total namespace validations issued", mTotalNamesapceValidationsIssued > 0 ? Stringize<typeof(mTotalNamesapceValidationsIssued)>(mTotalNamesapceValidationsIssued) : String(), firstTime) +
               Helper::getDebugValue("last namespace validations issued", Time() != mLastNamespaceValidateIssued ? IMessageHelper::timeToString(mLastNamespaceValidateIssued) : String(), firstTime) +
               Helper::getDebugValue("namespace grant start notification", mNamespaceGrantStartNotificationSent ? String("true") : String(), firstTime) +
               Helper::getDebugValue("received namespace grant complete notification", mReceivedNamespaceGrantCompleteNotify ? String("true") : String(), firstTime) +
               Helper::getDebugValue("pending messages", mPendingMessagesToDeliver.size() > 0 ? Stringize<DocumentList::size_type>(mPendingMessagesToDeliver.size()) : String(), firstTime) +
               Helper::getDebugValue("namespaces to grant", mNamespacesToGrant.size() > 0 ? Stringize<NamespaceInfoMap::size_type>(mNamespacesToGrant.size()) : String(), firstTime) +
               Helper::getDebugValue("namespaces granted", mNamespacesGranted.size() > 0 ? Stringize<NamespaceInfoMap::size_type>(mNamespacesGranted.size()) : String(), firstTime) +
               Helper::getDebugValue("has all required namespace grants", mHasGrantedAllRequiredNamespaces ? String("true") : String(), firstTime) +
               Helper::getDebugValue("subscriptions", mSubscriptions.size() > 0 ? Stringize<SubscriptionMap::size_type>(mSubscriptions.size()) : String(), firstTime);
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
        if (!stepBootstrapper()) goto post_step;
        if (!stepSendInitialValidate()) goto post_step;
        if (!stepCheckAllNamespaces()) goto post_step;
        if (!stepLoadGrantWindow()) goto post_step;
        if (!stepMakeGrantWindowVisible()) goto post_step;
        if (!stepSendNamespaceGrantStartNotification()) goto post_step;
        if (!stepWaitForPermission()) goto post_step;
        if (!stepCloseBrowserWindow()) goto post_step;

        setState(SessionState_Ready);

        if (!stepExpiresCheck()) goto post_step;

      post_step:
        postStep();
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepWaitForServices()
      {
        if (mNotifiedAssociatedToAllServicesComplete) {
          ZS_LOG_DEBUG(log("all services have been associated"))
          return true;
        }

        ZS_LOG_DEBUG(log("waiting for all services to be associated to the namespace grant service"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepBootstrapper()
      {
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
      bool ServiceNamespaceGrantSession::stepSendInitialValidate()
      {
        if (mNamespaceGrantValidateMonitor) {
          ZS_LOG_DEBUG(log("waiting for namespace grant validate to complete"))
          return false;
        }

        if (0 != mTotalNamesapceValidationsIssued) {
          ZS_LOG_DEBUG(log("already sent initial validate request"))
          return true;
        }

        ++mTotalNamesapceValidationsIssued;

        NamespaceGrantValidateRequestPtr request = NamespaceGrantValidateRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        request->purpose("namespace-grant-check");
        request->grantInfo(mGrantInfo);
        request->namespaceURLs(mNamespacesToGrant);

        mNamespaceGrantValidateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<NamespaceGrantValidateResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_NAMESPACE_GRANT_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("namespace-grant", "namespace-grant-validate", request);
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepCheckAllNamespaces()
      {
        if (mHasGrantedAllRequiredNamespaces) {
          ZS_LOG_DEBUG(log("all required namespaces have been granted"))
          return true;
        }

        for (NamespaceInfoMap::iterator iterToGrant = mNamespacesToGrant.begin(); iterToGrant != mNamespacesToGrant.end(); ++iterToGrant)
        {
          const NamespaceInfo &toGrantInfo = (*iterToGrant).second;
          bool found = false;
          for (NamespaceInfoMap::iterator iterGranted = mNamespacesGranted.begin(); iterGranted != mNamespacesGranted.end(); ++iterGranted)
          {
            const NamespaceInfo &grantedInfo = (*iterGranted).second;
            if (toGrantInfo.mURL == grantedInfo.mURL) {
              found = true;
              break;
            }
          }

          if (!found) {
            ZS_LOG_DEBUG(log("at least one namespace requested is not granted (will continue processing)") + ", missing namespace=" + toGrantInfo.mURL)

            if ((SessionState_Ready == mCurrentState) &&
                (mRedoGrantBrowserProcessIfNeeded)) {

              // prepare to load browser window all over again
              mRedoGrantBrowserProcessIfNeeded = false;
              mBrowserWindowReady = false;
              mBrowserWindowVisible = false;
              mBrowserWindowClosed = false;
              mNeedsBrowserWindowVisible = false;
              mNamespaceGrantStartNotificationSent = false;
              mReceivedNamespaceGrantCompleteNotify = false;

              mPendingMessagesToDeliver.clear();

              setState(SessionState_Pending);

              ZS_LOG_DEBUG(log("redoing stages required to perform grant"))
              IServiceNamespaceGrantSessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
              return false;
            }
            return true;  // do not prevent next steps from continuing
          }
        }

        ZS_LOG_DEBUG(log("have all required namespace grants"))

        mHasGrantedAllRequiredNamespaces = true;
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceNamespaceGrantSession::stepLoadGrantWindow()
      {
        if (mHasGrantedAllRequiredNamespaces) {
          ZS_LOG_DEBUG(log("all required namespaces have been granted"))
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

        request->outerFrameURL(mOuterFrameURLUponReload);
        request->grantInfo(mGrantInfo);
        request->namespaceURLs(mNamespacesToGrant);
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
      bool ServiceNamespaceGrantSession::stepExpiresCheck()
      {
        if (mLastNamespaceValidateIssued + Seconds(HOOKFLASH_STACK_SERVICE_NAMESPACE_GRANT_MIN_TIME_BETWEEN_VALIDATION_REQUESTS_IN_SECONDS) > zsLib::now()) {
          if (zsLib::now() + Seconds(HOOKFLASH_STACK_SERVICE_NAMESPACE_GRANT_TIME_BEFORE_EXPIRY_MUST_REFRESH_GRANT_IN_SECONDS) < mGrantInfo.mExpires) {
            ZS_LOG_DEBUG(log("cannot revalidate right now as it is too soon to last validation"))
            return true;
          }
          ZS_LOG_DEBUG(log("must revalidate grant now as it is too close to expiry") + getDebugValueString())
        } else {
          ZS_LOG_DEBUG(log("time to reissue the namespace grant validation") + getDebugValueString())
        }

        ++mTotalNamesapceValidationsIssued;

        setState(SessionState_Pending);

        NamespaceGrantValidateRequestPtr request = NamespaceGrantValidateRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        request->purpose("namespace-grant-refresh");
        request->grantInfo(mGrantInfo);
        request->namespaceURLs(mNamespacesToGrant);

        mNamespaceGrantValidateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<NamespaceGrantValidateResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_NAMESPACE_GRANT_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("namespace-grant", "namespace-grant-validate", request);
        return false;
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

        for (SubscriptionMap::iterator iter = mSubscriptions.begin(); iter != mSubscriptions.end(); ++iter)
        {
          SubscriptionPtr subscription = (*iter).second.lock();
          if (subscription) {
            subscription->notifyStateChanged(mCurrentState);
          }
        }

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
      #pragma mark ServiceNamespaceGrantSession::Subscription
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::Subscription::Subscription(
                                                               ServiceNamespaceGrantSessionPtr outer,
                                                               IServiceNamespaceGrantSessionForServicesDelegatePtr delegate
                                                               ) :
      mID(zsLib::createPUID()),
      mOuter(outer),
      mDelegate(delegate)
      {
      }

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::Subscription::~Subscription()
      {
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Subscription => IServiceNamespaceGrantSessionForServices
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::Subscription::cancel()
      {
        if (mOuter) {
          mOuter->notifySubscriptionGone(mID);
          mOuter.reset();
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession::Subscription => friend class ServiceNamespaceGrantSession
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceNamespaceGrantSession::SubscriptionPtr ServiceNamespaceGrantSession::Subscription::create(
                                                                                                       ServiceNamespaceGrantSessionPtr outer,
                                                                                                       IServiceNamespaceGrantSessionForServicesDelegatePtr delegate
                                                                                                       )
      {
        SubscriptionPtr pThis(new Subscription(outer, delegate));
        return pThis;
      }

      //-----------------------------------------------------------------------
      void ServiceNamespaceGrantSession::Subscription::notifyStateChanged(SessionStates state)
      {
        if (!mDelegate) return;
        if (!mOuter) return;

        try {
          mDelegate->onServiceNamespaceGrantSessionStateChanged(mOuter, state);
        } catch(IServiceNamespaceGrantSessionForServicesDelegateProxy::Exceptions::DelegateGone &) {
        }
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
        case SessionState_WaitingForAssociationToAllServices:     return "Waiting for Association to All Services";
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
                                                                           IServiceNamespaceGrantPtr serviceNamespaceGrant,
                                                                           const char *outerFrameURLUponReload,
                                                                           const char *grantID,
                                                                           const char *grantSecret
                                                                           )
    {
      return internal::IServiceNamespaceGrantSessionFactory::singleton().create(delegate, serviceNamespaceGrant, outerFrameURLUponReload, grantID, grantSecret);
    }

  }
}
