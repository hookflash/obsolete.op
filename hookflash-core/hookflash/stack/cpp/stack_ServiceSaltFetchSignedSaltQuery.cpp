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

#include <hookflash/stack/internal/stack_ServiceSaltFetchSignedSaltQuery.h>
#include <hookflash/stack/message/peer-salt/SignedSaltGetRequest.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/internal/stack_Stack.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>

#define HOOKFLASH_STACK_SERVICE_SALT_FETCH_SIGNED_SALT_QUERY_GET_TIMEOUT_IN_SECONDS (60*2)

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      using zsLib::Stringize;

      using message::peer_salt::SignedSaltGetRequest;
      using message::peer_salt::SignedSaltGetRequestPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceSaltFetchSignedSaltQuery
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceSaltFetchSignedSaltQuery::ServiceSaltFetchSignedSaltQuery(
                                                                       IMessageQueuePtr queue,
                                                                       IServiceSaltFetchSignedSaltQueryDelegatePtr delegate,
                                                                       IServiceSaltPtr serviceSalt,
                                                                       UINT totalToFetch
                                                                       ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IServiceSaltFetchSignedSaltQueryDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mBootstrappedNetwork(BootstrappedNetwork::convert(serviceSalt)),
        mLastError(0),
        mTotalToFetch(totalToFetch)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      ServiceSaltFetchSignedSaltQuery::~ServiceSaltFetchSignedSaltQuery()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      void ServiceSaltFetchSignedSaltQuery::init()
      {
        IBootstrappedNetworkForServices::prepare(mBootstrappedNetwork->forServices().getDomain(), mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      ServiceSaltFetchSignedSaltQueryPtr ServiceSaltFetchSignedSaltQuery::convert(IServiceSaltFetchSignedSaltQueryPtr query)
      {
        return boost::dynamic_pointer_cast<ServiceSaltFetchSignedSaltQuery>(query);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceSaltFetchSignedSaltQuery => IServiceSaltFetchSignedSaltQuery
      #pragma mark

      //-----------------------------------------------------------------------
      String ServiceSaltFetchSignedSaltQuery::toDebugString(IServiceSaltFetchSignedSaltQueryPtr query, bool includeCommaPrefix)
      {
        if (!query) return includeCommaPrefix ? String(", fetch signed salt query=(null)") : String();
        return ServiceSaltFetchSignedSaltQuery::convert(query)->getDebugValueString();
      }

      //-----------------------------------------------------------------------
      ServiceSaltFetchSignedSaltQueryPtr ServiceSaltFetchSignedSaltQuery::fetchSignedSalt(
                                                                                          IServiceSaltFetchSignedSaltQueryDelegatePtr delegate,
                                                                                          IServiceSaltPtr serviceSalt,
                                                                                          UINT totalToFetch
                                                                                          )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!serviceSalt)
        ZS_THROW_INVALID_ARGUMENT_IF(totalToFetch < 1)

        ServiceSaltFetchSignedSaltQueryPtr pThis(new ServiceSaltFetchSignedSaltQuery(IStackForInternal::queueStack(), delegate, serviceSalt, totalToFetch));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IServiceSaltPtr ServiceSaltFetchSignedSaltQuery::getService() const
      {
        AutoRecursiveLock lock(getLock());
        return mBootstrappedNetwork;
      }

      //-----------------------------------------------------------------------
      bool ServiceSaltFetchSignedSaltQuery::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return (0 == mTotalToFetch);
      }

      //-----------------------------------------------------------------------
      bool ServiceSaltFetchSignedSaltQuery::wasSuccessful(
                                                          WORD *outErrorCode,
                                                          String *outErrorReason
                                                          ) const
      {
        AutoRecursiveLock lock(getLock());
        if (0 != mTotalToFetch) return false;

        if (outErrorCode) {
          *outErrorCode = mLastError;
        }
        if (outErrorReason) {
          *outErrorReason = mLastErrorReason;
        }

        return (0 == mLastError);
      }

      //-----------------------------------------------------------------------
      UINT ServiceSaltFetchSignedSaltQuery::getTotalSignedSaltsAvailable() const
      {
        AutoRecursiveLock lock(getLock());
        return mSaltBundles.size();
      }

      //-----------------------------------------------------------------------
      ElementPtr ServiceSaltFetchSignedSaltQuery::getNextSignedSalt()
      {
        AutoRecursiveLock lock(getLock());
        ElementPtr result = mSaltBundles.front();
        mSaltBundles.pop_front();
        return result;
      }

      //-----------------------------------------------------------------------
      void ServiceSaltFetchSignedSaltQuery::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (0 == mTotalToFetch) {
          ZS_LOG_DEBUG(log("already cancelled"))
          return;
        }

        ZS_LOG_DEBUG(log("cancel called"))

        mTotalToFetch = 0;

        if (mSaltMonitor) {
          mSaltMonitor->cancel();
          mSaltMonitor.reset();
        }

        ServiceSaltFetchSignedSaltQueryPtr pThis = mThisWeak.lock();
        if ((pThis) &&
            (mDelegate)) {
          try {
            mDelegate->onServiceSaltFetchSignedSaltCompleted(pThis);
          } catch(IServiceCertificatesValidateQueryDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        mDelegate.reset();

        mBootstrappedNetwork.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceSaltFetchSignedSaltQuery => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceSaltFetchSignedSaltQuery::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork)
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
      #pragma mark ServiceSaltFetchSignedSaltQuery => IMessageMonitorResultDelegate<SignedSaltGetResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceSaltFetchSignedSaltQuery::handleMessageMonitorResultReceived(
                                                                               IMessageMonitorPtr monitor,
                                                                               SignedSaltGetResultPtr response
                                                                               )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSaltMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        ZS_LOG_DEBUG(log("salt get finished successfully"))
        mSaltBundles = response->saltBundles();

        if (mSaltBundles.size() < mTotalToFetch) {
          ZS_LOG_WARNING(Detail, log("failed to obtain the number of salts requested"))

          mLastError = IHTTP::HTTPStatusCode_NoContent;
          mLastErrorReason = IHTTP::toString(IHTTP::toStatusCode(mLastError));
          mSaltBundles.clear();
        }

        for (SaltBundleList::iterator iter = mSaltBundles.begin(); iter != mSaltBundles.end(); ++iter)
        {
          ElementPtr bundleEl = (*iter);
          if (!mBootstrappedNetwork->forServices().isValidSignature(bundleEl)) {
            ZS_LOG_WARNING(Detail, log("bundle returned was improperly signed"))

            mLastError = IHTTP::HTTPStatusCode_NoCert;
            mLastErrorReason = IHTTP::toString(IHTTP::toStatusCode(mLastError));
            mSaltBundles.clear();
            break;
          }
          ZS_LOG_DEBUG(log("salt bundle passed validation"))
        }

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceSaltFetchSignedSaltQuery::handleMessageMonitorErrorResultReceived(
                                                                                    IMessageMonitorPtr monitor,
                                                                                    SignedSaltGetResultPtr response, // will always be NULL
                                                                                    message::MessageResultPtr result
                                                                                    )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mSaltMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mLastError = result->errorCode();
        mLastErrorReason = result->errorReason();

        ZS_LOG_DEBUG(log("salt get failed") + getDebugValueString())

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceSaltFetchSignedSaltQuery => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ServiceSaltFetchSignedSaltQuery::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String ServiceSaltFetchSignedSaltQuery::log(const char *message) const
      {
        return String("ServiceSaltFetchSignedSaltQuery [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ServiceSaltFetchSignedSaltQuery::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("fetch signed salt id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               IBootstrappedNetwork::toDebugString(mBootstrappedNetwork) +
               IMessageMonitor::toDebugString(mSaltMonitor) +
               Helper::getDebugValue("salt bundles", mSaltBundles.size() > 0 ? Stringize<size_t>(mSaltBundles.size()).string() : String(), firstTime) +
               Helper::getDebugValue("total to fetch", 0 != mTotalToFetch ? Stringize<typeof(mTotalToFetch)>(mTotalToFetch).string() : String(), firstTime) +
               Helper::getDebugValue("error code", 0 != mLastError ? Stringize<typeof(mLastError)>(mLastError).string() : String(), firstTime) +
               Helper::getDebugValue("error reason", mLastErrorReason, firstTime);
      }

      //-----------------------------------------------------------------------
      void ServiceSaltFetchSignedSaltQuery::step()
      {
        if (0 == mTotalToFetch) {
          ZS_LOG_DEBUG(log("step proceeding to cancel"))
          cancel();
          return;
        }

        if (mSaltMonitor) {
          ZS_LOG_DEBUG(log("waiting for salt monitor"))
          return;
        }

        if (!mBootstrappedNetwork->forServices().isPreparationComplete()) {
          ZS_LOG_DEBUG(log("waiting for bootstrapper to complete"))
          return;
        }

        if (!mBootstrappedNetwork->forServices().wasSuccessful(&mLastError, &mLastErrorReason)) {
          ZS_LOG_WARNING(Detail, log("salt fetch failed because of bootstrapper failure"))
          cancel();
          return;
        }

        SignedSaltGetRequestPtr request = SignedSaltGetRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());
        request->salts(mTotalToFetch);

        mSaltMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<SignedSaltGetResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_SALT_FETCH_SIGNED_SALT_QUERY_GET_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("signed-salt", "signed-salt-get", request);
        ZS_LOG_DEBUG(log("sending signed salt get request"))
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
    #pragma mark IServiceSaltFetchSignedSaltQuery
    #pragma mark

    //-------------------------------------------------------------------------
    String IServiceSaltFetchSignedSaltQuery::toDebugString(IServiceSaltFetchSignedSaltQueryPtr query, bool includeCommaPrefix)
    {
      return internal::ServiceSaltFetchSignedSaltQuery::toDebugString(query, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IServiceSaltFetchSignedSaltQueryPtr IServiceSaltFetchSignedSaltQuery::fetchSignedSalt(
                                                                                          IServiceSaltFetchSignedSaltQueryDelegatePtr delegate,
                                                                                          IServiceSaltPtr serviceSalt,
                                                                                          UINT totalToFetch
                                                                                          )
    {
      return internal::IServiceSaltFetchSignedSaltQueryFactory::singleton().fetchSignedSalt(delegate, serviceSalt, totalToFetch);
    }
  }
}
