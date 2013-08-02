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

#include <openpeer/stack/internal/stack_ServiceLockboxSession.h>
#include <openpeer/stack/internal/stack_ServiceIdentitySession.h>
#include <openpeer/stack/message/identity-lockbox/LockboxAccessRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxNamespaceGrantChallengeValidateRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxIdentitiesUpdateRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxContentGetRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxContentSetRequest.h>
#include <openpeer/stack/message/peer/PeerServicesGetRequest.h>

#include <openpeer/stack/internal/stack_BootstrappedNetwork.h>
#include <openpeer/stack/internal/stack_Account.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/IPeer.h>
#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePrivate.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/internal/stack_Stack.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

#define OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS (60*2)

#define OPENPEER_STACK_SERVICE_LOCKBOX_EXPIRES_TIME_PERCENTAGE_CONSUMED_CAUSES_REGENERATION (80)

//#define OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE "https://meta.openpeer.org/permission/private-peer-file"
//#define OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE "https://meta.openpeer.org/permission/identity-relogins"

#define WARNING_HACK_UNTIL_PERMISSIONS_URLS_ARE_DEFINED 1
#define WARNING_HACK_UNTIL_PERMISSIONS_URLS_ARE_DEFINED 2

#define OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE "https://hcs-javascript.hookflash.me/hfservice/static/permission/permission1"
#define OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE "https://hcs-javascript.hookflash.me/hfservice/static/permission/permission2"


#define OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_SECRET_VALUE_NAME "privatePeerFileSecret"
#define OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_VALUE_NAME "privatePeerFile"

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using services::IHelper;

      using CryptoPP::Weak::MD5;

      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      using message::identity_lockbox::LockboxAccessRequest;
      using message::identity_lockbox::LockboxAccessRequestPtr;
      using message::identity_lockbox::LockboxNamespaceGrantChallengeValidateRequest;
      using message::identity_lockbox::LockboxNamespaceGrantChallengeValidateRequestPtr;
      using message::identity_lockbox::LockboxIdentitiesUpdateRequest;
      using message::identity_lockbox::LockboxIdentitiesUpdateRequestPtr;
      using message::identity_lockbox::LockboxContentGetRequest;
      using message::identity_lockbox::LockboxContentGetRequestPtr;
      using message::identity_lockbox::LockboxContentSetRequest;
      using message::identity_lockbox::LockboxContentSetRequestPtr;
      using message::peer::PeerServicesGetRequest;
      using message::peer::PeerServicesGetRequestPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static void getNamespaces(NamespaceInfoMap &outNamespaces)
      {
        static const char *gPermissions[] = {
          OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE,
          OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE,
          NULL
        };

        for (int index = 0; NULL != gPermissions[index]; ++index)
        {
          NamespaceInfo info;
          info.mURL = gPermissions[index];
          outNamespaces[info.mURL] = info;
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceLockboxSession::ServiceLockboxSession(
                                                   IMessageQueuePtr queue,
                                                   BootstrappedNetworkPtr network,
                                                   IServiceLockboxSessionDelegatePtr delegate,
                                                   ServiceNamespaceGrantSessionPtr grantSession
                                                   ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(delegate ? IServiceLockboxSessionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate) : IServiceLockboxSessionDelegatePtr()),
        mBootstrappedNetwork(network),
        mGrantSession(grantSession),
        mCurrentState(SessionState_Pending),
        mLastError(0),
        mObtainedLock(false),
        mPeerFilesNeedUpload(false),
        mLoginIdentitySetToBecomeAssociated(false),
        mForceNewAccount(false)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      ServiceLockboxSession::~ServiceLockboxSession()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::init()
      {
        calculateAndNotifyIdentityChanges();  // calculate the identities hash for the firs ttime

        IBootstrappedNetworkForServices::prepare(mBootstrappedNetwork->forServices().getDomain(), mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      ServiceLockboxSessionPtr ServiceLockboxSession::convert(IServiceLockboxSessionPtr session)
      {
        return boost::dynamic_pointer_cast<ServiceLockboxSession>(session);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IServiceLockboxSession
      #pragma mark

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::toDebugString(IServiceLockboxSessionPtr session, bool includeCommaPrefix)
      {
        if (!session) return includeCommaPrefix ? String(", peer contact session=(null)") : String("peer contact session=");
        return ServiceLockboxSession::convert(session)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ServiceLockboxSessionPtr ServiceLockboxSession::login(
                                                            IServiceLockboxSessionDelegatePtr delegate,
                                                            IServiceLockboxPtr serviceLockbox,
                                                            IServiceNamespaceGrantSessionPtr grantSession,
                                                            IServiceIdentitySessionPtr identitySession,
                                                            bool forceNewAccount
                                                            )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!serviceLockbox)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantSession)
        ZS_THROW_INVALID_ARGUMENT_IF(!identitySession)

        ServiceLockboxSessionPtr pThis(new ServiceLockboxSession(IStackForInternal::queueStack(), BootstrappedNetwork::convert(serviceLockbox), delegate, ServiceNamespaceGrantSession::convert(grantSession)));
        pThis->mThisWeak = pThis;
        pThis->mLoginIdentity = ServiceIdentitySession::convert(identitySession);
        pThis->mForceNewAccount = forceNewAccount;
        if (forceNewAccount) {
          ZS_LOG_WARNING(Detail, pThis->log("forcing creation of a new lockbox account (user must have indicated their account is compromised or corrupted)"))
        }
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ServiceLockboxSessionPtr ServiceLockboxSession::relogin(
                                                              IServiceLockboxSessionDelegatePtr delegate,
                                                              IServiceLockboxPtr serviceLockbox,
                                                              IServiceNamespaceGrantSessionPtr grantSession,
                                                              const char *lockboxAccountID,
                                                              const SecureByteBlock &lockboxKey
                                                              )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!serviceLockbox)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantSession)
        ZS_THROW_INVALID_ARGUMENT_IF(!lockboxAccountID)
        ZS_THROW_INVALID_ARGUMENT_IF(lockboxKey.SizeInBytes() < 1)

        ServiceLockboxSessionPtr pThis(new ServiceLockboxSession(IStackForInternal::queueStack(), BootstrappedNetwork::convert(serviceLockbox), delegate, ServiceNamespaceGrantSession::convert(grantSession)));
        pThis->mThisWeak = pThis;
        pThis->mLockboxInfo.mAccountID = String(lockboxAccountID);
        pThis->mLockboxInfo.mKey = IHelper::clone(lockboxKey);
        pThis->mLockboxInfo.mHash = IHelper::convertToHex(*IHelper::hash(lockboxKey));
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IServiceLockboxPtr ServiceLockboxSession::getService() const
      {
        AutoRecursiveLock lock(getLock());
        return mBootstrappedNetwork;
      }

      //-----------------------------------------------------------------------
      IServiceLockboxSession::SessionStates ServiceLockboxSession::getState(
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
      IPeerFilesPtr ServiceLockboxSession::getPeerFiles() const
      {
        AutoRecursiveLock lock(getLock());
        return mPeerFiles;
      }

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::getAccountID() const
      {
        AutoRecursiveLock lock(getLock());
        return mLockboxInfo.mAccountID;
      }

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::getDomain() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mBootstrappedNetwork) return String();
        return mBootstrappedNetwork->forServices().getDomain();
      }

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::getStableID() const
      {
        AutoRecursiveLock lock(getLock());

        if (mLockboxInfo.mAccountID.isEmpty()) return String();
        if (!mBootstrappedNetwork) return String();

        return IHelper::convertToHex(*IHelper::hash(String("stable-id:") + mBootstrappedNetwork->forServices().getDomain() + ":" + mLockboxInfo.mAccountID));
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr ServiceLockboxSession::getLockboxKey() const
      {
        AutoRecursiveLock lock(getLock());

        return IHelper::clone(mLockboxInfo.mKey);
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySessionListPtr ServiceLockboxSession::getAssociatedIdentities() const
      {
        AutoRecursiveLock lock(getLock());
        ServiceIdentitySessionListPtr result(new ServiceIdentitySessionList);
        for (ServiceIdentitySessionMap::const_iterator iter = mAssociatedIdentities.begin(); iter != mAssociatedIdentities.end(); ++iter)
        {
          result->push_back((*iter).second);
        }
        return result;
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::associateIdentities(
                                                      const ServiceIdentitySessionList &identitiesToAssociate,
                                                      const ServiceIdentitySessionList &identitiesToRemove
                                                      )
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("unable to associate identity as already shutdown") + getDebugValueString())
          return;
        }

        for (ServiceIdentitySessionList::const_iterator iter = identitiesToAssociate.begin(); iter != identitiesToAssociate.end(); ++iter)
        {
          ServiceIdentitySessionPtr session = ServiceIdentitySession::convert(*iter);
          session->forLockbox().associate(mThisWeak.lock());
          mPendingUpdateIdentities[session->forLockbox().getID()] = session;
        }
        for (ServiceIdentitySessionList::const_iterator iter = identitiesToRemove.begin(); iter != identitiesToRemove.end(); ++iter)
        {
          ServiceIdentitySessionPtr session = ServiceIdentitySession::convert(*iter);
          mPendingRemoveIdentities[session->forLockbox().getID()] = session;
        }
        // handle the association now (but do it asynchronously)
        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        if (mGrantQuery) {
          mGrantQuery->cancel();
          mGrantQuery.reset();
        }

        if (mGrantWait) {
          mGrantWait->cancel();
          mGrantWait.reset();
        }

        if (mLockboxAccessMonitor) {
          mLockboxAccessMonitor->cancel();
          mLockboxAccessMonitor.reset();
        }
        if (mLockboxNamespaceGrantChallengeValidateMonitor) {
          mLockboxNamespaceGrantChallengeValidateMonitor->cancel();
          mLockboxNamespaceGrantChallengeValidateMonitor.reset();
        }
        if (mLockboxIdentitiesUpdateMonitor) {
          mLockboxIdentitiesUpdateMonitor->cancel();
          mLockboxIdentitiesUpdateMonitor.reset();
        }
        if (mLockboxContentGetMonitor) {
          mLockboxContentGetMonitor->cancel();
          mLockboxContentGetMonitor.reset();
        }
        if (mLockboxContentSetMonitor) {
          mLockboxContentSetMonitor->cancel();
          mLockboxContentSetMonitor.reset();
        }
        if (mPeerServicesGetMonitor) {
          mPeerServicesGetMonitor->cancel();
          mPeerServicesGetMonitor.reset();
        }

        if (mSaltQuery) {
          mSaltQuery->cancel();
          mSaltQuery.reset();
        }

        setState(SessionState_Shutdown);

        mAccount.reset();

        if (mLoginIdentity) {
          mLoginIdentity->forLockbox().notifyStateChanged();
          mLoginIdentity.reset();
        }

        for (ServiceIdentitySessionMap::iterator iter = mAssociatedIdentities.begin(); iter != mAssociatedIdentities.end(); ++iter)
        {
          ServiceIdentitySessionPtr session = (*iter).second;
          session->forLockbox().notifyStateChanged();
        }

        for (ServiceIdentitySessionMap::iterator iter = mPendingUpdateIdentities.begin(); iter != mPendingUpdateIdentities.end(); ++iter)
        {
          ServiceIdentitySessionPtr session = (*iter).second;
          session->forLockbox().notifyStateChanged();
        }

        for (ServiceIdentitySessionMap::iterator iter = mPendingUpdateIdentities.begin(); iter != mPendingUpdateIdentities.end(); ++iter)
        {
          ServiceIdentitySessionPtr session = (*iter).second;
          session->forLockbox().notifyStateChanged();
        }

        mAssociatedIdentities.clear();
        mPendingUpdateIdentities.clear();
        mPendingRemoveIdentities.clear();

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IMessageSource
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IServiceLockboxSessionForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::attach(AccountPtr account)
      {
        AutoRecursiveLock lock(getLock());
        mAccount = account;

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      Service::MethodPtr ServiceLockboxSession::findServiceMethod(
                                                                  const char *serviceType,
                                                                  const char *method
                                                                  ) const
      {
        if (NULL == serviceType) return Service::MethodPtr();
        if (NULL == method) return Service::MethodPtr();

        ServiceTypeMap::const_iterator found = mServicesByType.find(serviceType);
        if (found == mServicesByType.end()) return Service::MethodPtr();

        const Service *service = &(*found).second;

        Service::MethodMap::const_iterator foundMethod = service->mMethods.find(method);
        if (foundMethod == service->mMethods.end()) return Service::MethodPtr();

        Service::MethodPtr result(new Service::Method);
        (*result) = ((*foundMethod).second);

        return result;
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr ServiceLockboxSession::getBootstrappedNetwork() const
      {
        AutoRecursiveLock lock(getLock());
        return mBootstrappedNetwork;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IServiceLockboxSessionForServiceIdentity
      #pragma mark

      //-----------------------------------------------------------------------
      LockboxInfo ServiceLockboxSession::getLockboxInfo() const
      {
        AutoRecursiveLock lock(getLock());
        return mLockboxInfo;
      }

      //-----------------------------------------------------------------------
      IdentityInfo ServiceLockboxSession::getIdentityInfoForIdentity(
                                                                     ServiceIdentitySessionPtr session,
                                                                     IPeerFilesPtr *outPeerFiles
                                                                     ) const
      {
        if (outPeerFiles) {
          *outPeerFiles = IPeerFilesPtr();
        }

        AutoRecursiveLock lock(getLock());

        IdentityInfo info;

        info.mStableID = getStableID();

        if (mPeerFiles) {
          info.mPeerFilePublic = mPeerFiles->getPeerFilePublic();

          if (outPeerFiles) {
            *outPeerFiles = mPeerFiles;
          }
        }

        WORD priority = 0;

        for (ServiceIdentitySessionMap::const_iterator iter = mAssociatedIdentities.begin(); iter != mAssociatedIdentities.end(); ++iter)
        {
          const ServiceIdentitySessionPtr &identity = (*iter).second;

          if (identity->forLockbox().getID() == session->forLockbox().getID()) {
            break;
          }

          ++priority;
        }

        info.mPriority = priority;

        return info;
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::notifyStateChanged()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("notify state changed"))
        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IWakeDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::onWake()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("on wake"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork)
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
      #pragma mark ServiceLockboxSession => IServiceSaltFetchSignedSaltQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::onServiceSaltFetchSignedSaltCompleted(IServiceSaltFetchSignedSaltQueryPtr query)
      {
        ZS_LOG_DEBUG(log("salt service reported complete"))

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IServiceNamespaceGrantSessionForServicesWaitForWaitDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::onServiceNamespaceGrantSessionForServicesWaitComplete(IServiceNamespaceGrantSessionPtr session)
      {
        ZS_LOG_DEBUG(log("namespace grant session wait is ready to be obtained (if needed)"))

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IServiceSaltFetchSignedSaltQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::onServiceNamespaceGrantSessionForServicesQueryComplete(
                                                                                         IServiceNamespaceGrantSessionForServicesQueryPtr query,
                                                                                         ElementPtr namespaceGrantChallengeBundleEl
                                                                                         )
      {
        ZS_LOG_DEBUG(log("namespace grant session state changed"))

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<LockboxAccessResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorResultReceived(
                                                                     IMessageMonitorPtr monitor,
                                                                     LockboxAccessResultPtr result
                                                                     )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxAccessMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mLockboxAccessMonitor->cancel();
        mLockboxAccessMonitor.reset();

        LockboxInfo info = result->lockboxInfo();
        mLockboxInfo.mergeFrom(info, true);

        mServerIdentities = result->identities();

        NamespaceGrantChallengeInfo challengeInfo = result->namespaceGrantChallengeInfo();

        if (challengeInfo.mID.hasData()) {
          // a namespace grant challenge was issue
          NamespaceInfoMap namespaces;
          getNamespaces(namespaces);
          mGrantQuery = mGrantSession->forServices().query(mThisWeak.lock(), challengeInfo, namespaces);
        }

        step();

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorErrorResultReceived(
                                                                          IMessageMonitorPtr monitor,
                                                                          LockboxAccessResultPtr ignore, // will always be NULL
                                                                          message::MessageResultPtr result
                                                                          )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxAccessMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_WARNING(Detail, log("lockbox access failed"))

        setError(result->errorCode(), result->errorReason());
        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<LockboxNamespaceGrantChallengeValidateResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorResultReceived(
                                                                     IMessageMonitorPtr monitor,
                                                                     LockboxNamespaceGrantChallengeValidateResultPtr result
                                                                     )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxNamespaceGrantChallengeValidateMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mLockboxNamespaceGrantChallengeValidateMonitor->cancel();
        mLockboxNamespaceGrantChallengeValidateMonitor.reset();

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorErrorResultReceived(
                                                                          IMessageMonitorPtr monitor,
                                                                          LockboxNamespaceGrantChallengeValidateResultPtr ignore, // will always be NULL
                                                                          message::MessageResultPtr result
                                                                          )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxAccessMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_WARNING(Detail, log("lockbox access failed"))

        setError(result->errorCode(), result->errorReason());
        cancel();
        return true;
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<LockboxIdentitiesUpdateResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorResultReceived(
                                                                     IMessageMonitorPtr monitor,
                                                                     LockboxIdentitiesUpdateResultPtr result
                                                                     )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxIdentitiesUpdateMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mLockboxIdentitiesUpdateMonitor->cancel();
        mLockboxIdentitiesUpdateMonitor.reset();

        mServerIdentities = result->identities();

        step();

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorErrorResultReceived(
                                                                          IMessageMonitorPtr monitor,
                                                                          LockboxIdentitiesUpdateResultPtr ignore, // will always be NULL
                                                                          message::MessageResultPtr result
                                                                          )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxIdentitiesUpdateMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_WARNING(Detail, log("identities update failed"))

        setError(result->errorCode(), result->errorReason());
        cancel();
        return true;
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<LockboxContentGetResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorResultReceived(
                                                                     IMessageMonitorPtr monitor,
                                                                     LockboxContentGetResultPtr result
                                                                     )
      {
        typedef LockboxContentGetResult::NameValueMap NameValueMap;

        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxContentGetMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mLockboxContentGetMonitor->cancel();
        mLockboxContentGetMonitor.reset();

        ZS_LOG_DEBUG(log("content get completed"))

        mContent = result->namespaceURLNameValues();

        // add some bogus content just to ensure there is some values in the map
        if (mContent.size() < 1) {
          NameValueMap values;
          mContent["bogus"] = values;
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorErrorResultReceived(
                                                                          IMessageMonitorPtr monitor,
                                                                          LockboxContentGetResultPtr ignore, // will always be NULL
                                                                          message::MessageResultPtr result
                                                                          )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxContentGetMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_WARNING(Detail, log("content get failed"))

        setError(result->errorCode(), result->errorReason());
        cancel();
        return true;
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<LockboxContentSetResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorResultReceived(
                                                                     IMessageMonitorPtr monitor,
                                                                     LockboxContentSetResultPtr result
                                                                     )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxContentSetMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mLockboxContentSetMonitor->cancel();
        mLockboxContentSetMonitor.reset();

        ZS_LOG_DEBUG(log("content set completed"))

        mPeerFilesNeedUpload = false;

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorErrorResultReceived(
                                                                          IMessageMonitorPtr monitor,
                                                                          LockboxContentSetResultPtr ignore, // will always be NULL
                                                                          message::MessageResultPtr result
                                                                          )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLockboxContentSetMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_WARNING(Detail, log("content set failed"))

        setError(result->errorCode(), result->errorReason());
        cancel();
        return true;
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<PeerServicesGetResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorResultReceived(
                                                                     IMessageMonitorPtr monitor,
                                                                     PeerServicesGetResultPtr result
                                                                     )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mPeerServicesGetMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        mPeerServicesGetMonitor->cancel();
        mPeerServicesGetMonitor.reset();

        ZS_LOG_DEBUG(log("peer services get completed"))

        mServicesByType = result->servicesByType();
        if (mServicesByType.size() < 1) {
          // make sure to add at least one bogus service so we know this request completed
          Service service;
          service.mID = "bogus";
          service.mType = "bogus";

          mServicesByType[service.mType] = service;
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::handleMessageMonitorErrorResultReceived(
                                                                          IMessageMonitorPtr monitor,
                                                                          PeerServicesGetResultPtr ignore, // will always be NULL
                                                                          message::MessageResultPtr result
                                                                          )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mPeerServicesGetMonitor) {
          ZS_LOG_DEBUG(log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_WARNING(Detail, log("peer services get failed"))

        setError(result->errorCode(), result->errorReason());
        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ServiceLockboxSession::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::log(const char *message) const
      {
        return String("ServiceLockboxSession [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("lockbox id", string(mID), firstTime) +
               IBootstrappedNetwork::toDebugString(mBootstrappedNetwork) +
               Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("error code", 0 != mLastError ? string(mLastError) : String(), firstTime) +
               Helper::getDebugValue("error reason", mLastErrorReason, firstTime) +
               IBootstrappedNetwork::toDebugString(mBootstrappedNetwork) +
               Helper::getDebugValue("grant session", mGrantSession ? string(mGrantSession->forServices().getID()) : String(), firstTime) +
               Helper::getDebugValue("grant query", mGrantQuery ? string(mGrantQuery->getID()) : String(), firstTime) +
               Helper::getDebugValue("grant wait", mGrantWait ? string(mGrantWait->getID()) : String(), firstTime) +
               Helper::getDebugValue("lockbox access monitor", mLockboxAccessMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("lockbox grant validate monitor", mLockboxNamespaceGrantChallengeValidateMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("lockbox identities update monitor", mLockboxIdentitiesUpdateMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("lockbox content get monitor", mLockboxContentGetMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("lockbox content set monitor", mLockboxContentSetMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("peer services get monitor", mPeerServicesGetMonitor ? String("true") : String(), firstTime) +
               mLockboxInfo.getDebugValueString() +
               Helper::getDebugValue("login identity", mLoginIdentity ? String("true") : String(), firstTime) +
               IPeerFiles::toDebugString(mPeerFiles) +
               Helper::getDebugValue("obtained lock", mObtainedLock ? String("true") : String(), firstTime) +
               Helper::getDebugValue("peer files need upload", mPeerFilesNeedUpload ? String("true") : String(), firstTime) +
               Helper::getDebugValue("login identity set to become associated", mLoginIdentitySetToBecomeAssociated ? String("true") : String(), firstTime) +
               Helper::getDebugValue("force new account", mForceNewAccount ? String("true") : String(), firstTime) +
               Helper::getDebugValue("salt query id", mSaltQuery ? string(mSaltQuery->getID()) : String(), firstTime) +
               Helper::getDebugValue("services by type", mServicesByType.size() > 0 ? string(mServicesByType.size()) : String(), firstTime) +
               Helper::getDebugValue("server identities", mServerIdentities.size() > 0 ? string(mServicesByType.size()) : String(), firstTime) +
               Helper::getDebugValue("associated identities", mAssociatedIdentities.size() > 0 ? string(mAssociatedIdentities.size()) : String(), firstTime) +
               Helper::getDebugValue("last notification hash", mLastNotificationHash ? IHelper::convertToHex(*mLastNotificationHash) : String(), firstTime) +
               Helper::getDebugValue("pending updated identities", mPendingUpdateIdentities.size() > 0 ? string(mPendingUpdateIdentities.size()) : String(), firstTime) +
               Helper::getDebugValue("pending remove identities", mPendingRemoveIdentities.size() > 0 ? string(mPendingRemoveIdentities.size()) : String(), firstTime);
               Helper::getDebugValue("content", mContent.size() > 0 ? string(mContent.size()) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step - already shutdown"))
          cancel();
          return;
        }

        if (!stepBootstrapper()) goto post_step;
        if (!stepGrantLock()) goto post_step;
        if (!stepIdentityLogin()) goto post_step;
        if (!stepLockboxAccess()) goto post_step;
        if (!stepGrantLockClear()) goto post_step;
        if (!stepGrantChallenge()) goto post_step;
        if (!stepContentGet()) goto post_step;
        if (!stepPreparePeerFiles()) goto post_step;
        if (!stepUploadPeerFiles()) goto post_step;
        if (!stepServicesGet()) goto post_step;

        setState(SessionState_Ready);

        if (!stepLoginIdentityBecomeAssociated()) goto post_step;
        if (!stepConvertFromServerToRealIdentities()) goto post_step;
        if (!stepPruneDuplicatePendingIdentities()) goto post_step;
        if (!stepPruneShutdownIdentities()) goto post_step;
        if (!stepPendingAssociationAndRemoval()) goto post_step;

      post_step:
        postStep();
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepBootstrapper()
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

        ZS_LOG_ERROR(Detail, log("bootstrapped network failed for lockbox") + ", error=" + string(errorCode) + ", reason=" + reason)

        setError(errorCode, reason);
        cancel();
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepGrantLock()
      {
        if (mObtainedLock) {
          ZS_LOG_DEBUG(log("already informed browser window ready thus no need to make sure grant wait lock is obtained"))
          return true;
        }

        if (mGrantWait) {
          ZS_LOG_DEBUG(log("grant wait lock is already obtained"))
          return true;
        }

        mGrantWait = mGrantSession->forServices().obtainWaitToProceed(mThisWeak.lock());

        if (!mGrantWait) {
          ZS_LOG_DEBUG(log("waiting to obtain grant wait lock"))
          return false;
        }
        
        mObtainedLock = true;
        return true;
      }
      
      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepIdentityLogin()
      {
        if (!mLoginIdentity) {
          ZS_LOG_DEBUG(log("no identity being logged in"))
          return true;
        }

        if (mLoginIdentity->forLockbox().isShutdown()) {
          WORD errorCode = 0;
          String reason;

          mLoginIdentity->forLockbox().getState(&errorCode, &reason);

          if (0 == errorCode) {
            errorCode = IHTTP::HTTPStatusCode_ClientClosedRequest;
          }

          ZS_LOG_WARNING(Detail, log("shutting down lockbox because identity login is shutdown"))
          setError(errorCode, reason);
          return true;
        }

        ZS_LOG_DEBUG(log("associated login identity to lockbox"))

        // require the association now to ensure the identity changes in state cause lockbox state changes too
        mLoginIdentity->forLockbox().associate(mThisWeak.lock());

        if (mLoginIdentity->forLockbox().isLoginComplete()) {
          ZS_LOG_DEBUG(log("identity login is complete"))
          return true;
        }

        ZS_LOG_DEBUG(log("waiting for login to complete"))

        setState(SessionState_Pending);
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepLockboxAccess()
      {
        if (mLockboxAccessMonitor) {
          ZS_LOG_DEBUG(log("waiting for lockbox access monitor to complete"))
          return false;
        }

        if (mLockboxInfo.mAccessToken.hasData()) {
          ZS_LOG_DEBUG(log("already have a lockbox access key"))
          return true;
        }

        setState(SessionState_Pending);

        LockboxAccessRequestPtr request = LockboxAccessRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        if (mLoginIdentity) {
          IdentityInfo identityInfo = mLoginIdentity->forLockbox().getIdentityInfo();
          request->identityInfo(identityInfo);

          LockboxInfo lockboxInfo = mLoginIdentity->forLockbox().getLockboxInfo();
          mLockboxInfo.mergeFrom(lockboxInfo);

          if (!IHelper::isValidDomain(mLockboxInfo.mDomain)) {
            ZS_LOG_DEBUG(log("domain from identity is invalid, reseting to default domain") + ", domain=" + mLockboxInfo.mDomain)

            mLockboxInfo.mDomain = mBootstrappedNetwork->forServices().getDomain();

            // account/keying information must also be incorrect if domain is not valid
            mLockboxInfo.mKey.reset();
          }

          if (mBootstrappedNetwork->forServices().getDomain() != mLockboxInfo.mDomain) {
            ZS_LOG_DEBUG(log("default bootstrapper is not to be used for this lockbox as an altenative lockbox must be used thus preparing replacement bootstrapper"))

            mBootstrappedNetwork = BootstrappedNetwork::convert(IBootstrappedNetwork::prepare(mLockboxInfo.mDomain, mThisWeak.lock()));
            return false;
          }

          if (mForceNewAccount) {
            ZS_LOG_DEBUG(log("forcing a new lockbox account to be created for the identity"))
            mForceNewAccount = false;
            mLockboxInfo.mKey.reset();
          }

          if (!mLockboxInfo.mKey) {
            mLockboxInfo.mAccountID.clear();
            mLockboxInfo.mKey.reset();
            mLockboxInfo.mHash.clear();
            mLockboxInfo.mResetFlag = true;

            mLockboxInfo.mKey = IHelper::random(32);

            ZS_LOG_DEBUG(log("created new lockbox key") + ", identity half=" + IHelper::convertToBase64(*mLockboxInfo.mKey))
          }

          ZS_THROW_BAD_STATE_IF(!mLockboxInfo.mKey)

          ZS_LOG_DEBUG(log("creating lockbox key hash") + ", key=" + IHelper::convertToBase64(*mLockboxInfo.mKey))
          mLockboxInfo.mHash = IHelper::convertToHex(*IHelper::hash(*mLockboxInfo.mKey));
        }

        mLockboxInfo.mDomain = mBootstrappedNetwork->forServices().getDomain();

        request->grantID(mGrantSession->forServices().getGrantID());
        request->lockboxInfo(mLockboxInfo);

        NamespaceInfoMap namespaces;
        getNamespaces(namespaces);
        request->namespaceURLs(namespaces);

        mLockboxAccessMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<LockboxAccessResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity-lockbox", "lockbox-access", request);

        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepGrantLockClear()
      {
        if (!mGrantWait) {
          ZS_LOG_DEBUG(log("wait already cleared"))
          return true;
        }

        ZS_LOG_DEBUG(log("clearing grant wait"))

        mGrantWait->cancel();
        mGrantWait.reset();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepGrantChallenge()
      {
        if (mLockboxNamespaceGrantChallengeValidateMonitor) {
          ZS_LOG_DEBUG(log("waiting for lockbox namespace grant challenge validate monitor to complete"))
          return false;
        }

        if (!mGrantQuery) {
          ZS_LOG_DEBUG(log("no grant challenge query thus continuing..."))
          return true;
        }

        if (!mGrantQuery->isComplete()) {
          ZS_LOG_DEBUG(log("waiting for the grant query to complete"))
          return false;
        }

        ElementPtr bundleEl = mGrantQuery->getNamespaceGrantChallengeBundle();
        if (!bundleEl) {
          ZS_LOG_ERROR(Detail, log("namespaces were no granted in challenge"))
          setError(IHTTP::HTTPStatusCode_Forbidden, "namespaces were not granted to access lockbox");
          cancel();
          return false;
        }

        NamespaceInfoMap namespaces;
        getNamespaces(namespaces);

        for (NamespaceInfoMap::iterator iter = namespaces.begin(); iter != namespaces.end(); ++iter)
        {
          NamespaceInfo &namespaceInfo = (*iter).second;

          if (!mGrantSession->forServices().isNamespaceURLInNamespaceGrantChallengeBundle(bundleEl, namespaceInfo.mURL)) {
            ZS_LOG_WARNING(Detail, log("lockbox was not granted required namespace") + ", namespace" + namespaceInfo.mURL)
            setError(IHTTP::HTTPStatusCode_Forbidden, "namespaces were not granted to access lockbox");
            cancel();
            return false;
          }
        }

        mGrantQuery->cancel();
        mGrantQuery.reset();

        ZS_LOG_DEBUG(log("all namespaces required were correctly granted, notify the lockbox of the newly created access"))

        LockboxNamespaceGrantChallengeValidateRequestPtr request = LockboxNamespaceGrantChallengeValidateRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        request->lockboxInfo(mLockboxInfo);
        request->namespaceGrantChallengeBundle(bundleEl);

        mLockboxNamespaceGrantChallengeValidateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<LockboxNamespaceGrantChallengeValidateResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity-lockbox", "lockbox-namespace-grant-challenge-validate", request);

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepContentGet()
      {
        if (mLockboxContentGetMonitor) {
          ZS_LOG_DEBUG(log("waiting for content get to complete"))
          return false;
        }

        if (mContent.size() > 0) {
          ZS_LOG_DEBUG(log("content has been obtained already"))
          return true;
        }

        setState(SessionState_Pending);

        LockboxContentGetRequestPtr request = LockboxContentGetRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        NamespaceInfoMap namespaces;
        getNamespaces(namespaces);

        request->lockboxInfo(mLockboxInfo);
        request->namespaceInfos(namespaces);

        mLockboxContentGetMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<LockboxContentGetResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity-lockbox", "lockbox-content-get", request);

        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepPreparePeerFiles()
      {
        if (mPeerFiles) {
          ZS_LOG_DEBUG(log("peer files already created/loaded"))
          return true;
        }

        if (mSaltQuery) {
          if (!mSaltQuery->isComplete()) {
            ZS_LOG_DEBUG(log("waiting for salt query to complete"))
            return false;
          }

          WORD errorCode = 0;
          String reason;
          if (!mSaltQuery->wasSuccessful(&errorCode, &reason)) {
            ZS_LOG_ERROR(Detail, log("failed to fetch signed salt") + ", error=" + string(errorCode) + ", reason=" + reason)
            setError(errorCode, reason);
            cancel();
            return false;
          }
        }

        setState(SessionState_Pending);

        String privatePeerSecretStr = getContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_SECRET_VALUE_NAME);
        String privatePeerFileStr = getContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_VALUE_NAME);

        if ((privatePeerSecretStr.hasData()) &&
            (privatePeerFileStr.hasData())) {

          ZS_LOG_TRACE(log("loading private peer file"))

          // attempt to load the private peer file data
          DocumentPtr doc = Document::createFromParsedJSON(privatePeerFileStr);
          if (doc) {
            mPeerFiles = IPeerFiles::loadFromElement(privatePeerFileStr, doc->getFirstChildElement());
            if (!mPeerFiles) {
              ZS_LOG_WARNING(Detail, log("peer files failed to load (will generate new peer files)"))
            }
          }
        }

        if (mPeerFiles) {
          ZS_LOG_DEBUG(log("peer files successfully loaded"))

          IPeerFilePublicPtr peerFilePublic = mPeerFiles->getPeerFilePublic();
          ZS_THROW_BAD_STATE_IF(!peerFilePublic)

          Time created = peerFilePublic->getCreated();
          Time expires = peerFilePublic->getExpires();

          Time now = zsLib::now();

          if (now > expires) {
            ZS_LOG_WARNING(Detail, log("peer file expired") + IPeerFilePublic::toDebugString(peerFilePublic) + ", now=" + IHelper::timeToString(now))
            mPeerFiles.reset();
          }

          Duration totalLifetime = (expires - created);
          Duration lifeConsumed (now - created);

          if (((lifeConsumed.seconds() * 100) / totalLifetime.seconds()) > OPENPEER_STACK_SERVICE_LOCKBOX_EXPIRES_TIME_PERCENTAGE_CONSUMED_CAUSES_REGENERATION) {
            ZS_LOG_WARNING(Detail, log("peer file are past acceptable expiry window") + ", lifetime consumed seconds=" + string(lifeConsumed.seconds()) + ", " + string(totalLifetime.seconds()) + IPeerFilePublic::toDebugString(peerFilePublic) + ", now=" + IHelper::timeToString(now))
            mPeerFiles.reset();
          }

          if (mPeerFiles) {
            ZS_LOG_DEBUG(log("peer files are still valid"))
            return true;
          }

          ZS_LOG_DEBUG(log("peer files will be regenerated"))

          // erase out the current peer file information if it exists from memory (prevents them from becoming reloaded / retested later)
          clearContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_SECRET_VALUE_NAME);
          clearContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_VALUE_NAME);
        }

        if (!mSaltQuery) {
          IServiceSaltPtr saltService = IServiceSalt::createServiceSaltFrom(mBootstrappedNetwork);
          mSaltQuery = IServiceSaltFetchSignedSaltQuery::fetchSignedSalt(mThisWeak.lock(), saltService);

          ZS_LOG_DEBUG(log("waiting for signed salt query to complete"))
          return false;
        }

        ElementPtr signedSaltEl = mSaltQuery->getNextSignedSalt();
        if (!signedSaltEl) {
          ZS_LOG_ERROR(Detail, log("failed to obtain signed salt from salt query"))
          setError(IHTTP::HTTPStatusCode_PreconditionFailed, "signed salt query was successful but failed to obtain signed salt");
          cancel();
          return false;
        }

        ZS_LOG_DEBUG(log("generating peer files (may take a while)..."))
        setState(SessionState_PendingPeerFilesGeneration);

        mPeerFiles = IPeerFiles::generate(IHelper::randomString((32*8)/5+1), signedSaltEl);
        if (!mPeerFiles) {
          ZS_LOG_ERROR(Detail, log("failed to generate peer files"))
          setError(IHTTP::HTTPStatusCode_InternalServerError, "failed to generate peer files");
          cancel();
          return false;
        }

        ZS_LOG_DEBUG(log("peer files were generated"))
        setState(SessionState_Pending);

        IPeerFilePrivatePtr peerFilePrivate = mPeerFiles->getPeerFilePrivate();
        ZS_THROW_BAD_STATE_IF(!peerFilePrivate)

        SecureByteBlockPtr peerFileSecret = peerFilePrivate->getPassword();
        ZS_THROW_BAD_STATE_IF(!peerFileSecret)

        ElementPtr peerFileEl = peerFilePrivate->saveToElement();
        ZS_THROW_BAD_STATE_IF(!peerFileEl)

        GeneratorPtr generator = Generator::createJSONGenerator();
        boost::shared_array<char> output = generator->write(peerFileEl);

        privatePeerFileStr = output.get();

        setContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_SECRET_VALUE_NAME, IHelper::convertToString(*peerFileSecret));
        setContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_VALUE_NAME, privatePeerFileStr);

        mPeerFilesNeedUpload = true;
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepUploadPeerFiles()
      {
        typedef LockboxContentSetRequest::NamespaceURLNameValueMap NamespaceURLValueMap;
        typedef LockboxContentSetRequest::NameValueMap NamespaceValueMap;

        if (!mPeerFilesNeedUpload) {
          ZS_LOG_DEBUG(log("peer files do not need uploading"))
          return true;
        }

        if (mLockboxContentSetMonitor) {
          ZS_LOG_DEBUG(log("waiting for content set monitor to complete"))
          return false;
        }

        setState(SessionState_Pending);

        LockboxContentSetRequestPtr request = LockboxContentSetRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        request->lockboxInfo(mLockboxInfo);

        NamespaceValueMap values;
        values[OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_SECRET_VALUE_NAME] = getRawContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_SECRET_VALUE_NAME);
        values[OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_VALUE_NAME] = getRawContent(OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE, OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_VALUE_NAME);

        NamespaceURLValueMap namespaces;
        namespaces[OPENPEER_STACK_SERVICE_LOCKBOX_PRIVATE_PEER_FILE_NAMESPACE] = values;

        request->namespaceURLNameValues(namespaces);

        mLockboxContentSetMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<LockboxContentSetResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity-lockbox", "lockbox-content-set", request);

        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepServicesGet()
      {
        if (mServicesByType.size() > 0) {
          ZS_LOG_DEBUG(log("already download services"))
          return true;
        }

        if (mPeerServicesGetMonitor) {
          ZS_LOG_DEBUG(log("waiting for services get to complete"))
          return false;
        }

        setState(SessionState_Pending);

        ZS_LOG_DEBUG(log("requestion information about the peer services available"))

        PeerServicesGetRequestPtr request = PeerServicesGetRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        request->lockboxInfo(mLockboxInfo);

        mPeerServicesGetMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<PeerServicesGetResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("peer", "peer-services-get", request);
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepLoginIdentityBecomeAssociated()
      {
        if (!mLoginIdentity) {
          ZS_LOG_DEBUG(log("did not login with a login identity (thus no need to force it to associate)"))
          return true;
        }

        if (mLoginIdentitySetToBecomeAssociated) {
          ZS_LOG_DEBUG(log("login identity is already set to become associated"))
          return true;
        }

        ZS_LOG_DEBUG(log("login identity will become associated identity") + IServiceIdentitySession::toDebugString(mLoginIdentity))

        mLoginIdentitySetToBecomeAssociated = true;

        for (ServiceIdentitySessionMap::iterator associatedIter = mAssociatedIdentities.begin(); associatedIter != mAssociatedIdentities.end(); ++associatedIter)
        {
          ServiceIdentitySessionPtr &identity = (*associatedIter).second;
          if (identity->forLockbox().getID() == mLoginIdentity->forLockbox().getID()) {
            ZS_LOG_DEBUG(log("login identity is already associated"))
            return true;
          }
        }

        for (ServiceIdentitySessionMap::iterator pendingIter = mPendingUpdateIdentities.begin(); pendingIter != mPendingUpdateIdentities.end(); ++pendingIter)
        {
          ServiceIdentitySessionPtr &identity = (*pendingIter).second;
          if (identity->forLockbox().getID() == mLoginIdentity->forLockbox().getID()) {
            ZS_LOG_DEBUG(log("login identity is already in pending list"))
            return true;
          }
        }

        ZS_LOG_DEBUG(log("adding login identity to the pending list so that it will become associated (if it is not already known by the server)"))
        mPendingUpdateIdentities[mLoginIdentity->forLockbox().getID()] = mLoginIdentity;

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepConvertFromServerToRealIdentities()
      {
        if (mServerIdentities.size() < 1) {
          ZS_LOG_DEBUG(log("no server identities that need to be converted to identities"))
          return true;
        }

        for (IdentityInfoList::iterator iter = mServerIdentities.begin(); iter != mServerIdentities.end();)
        {
          IdentityInfoList::iterator current = iter;
          ++iter;

          IdentityInfo &info = (*current);

          bool foundMatch = false;

          for (ServiceIdentitySessionMap::iterator assocIter = mAssociatedIdentities.begin(); assocIter != mAssociatedIdentities.end(); ++assocIter)
          {
            ServiceIdentitySessionPtr &identitySession = (*assocIter).second;
            IdentityInfo associatedInfo = identitySession->forLockbox().getIdentityInfo();

            if ((info.mURI == associatedInfo.mURI) &&
                (info.mProvider == associatedInfo.mProvider)) {
              // found an existing match...
              ZS_LOG_DEBUG(log("found a match to a previously associated identity") + ", uri=" + info.mURI + ", provider=" + info.mProvider)
              foundMatch = true;
              break;
            }
          }

          if (foundMatch) continue;

          for (ServiceIdentitySessionMap::iterator pendingIter = mPendingUpdateIdentities.begin(); pendingIter != mPendingUpdateIdentities.end(); )
          {
            ServiceIdentitySessionMap::iterator pendingCurrentIter = pendingIter;
            ++pendingIter;

            ServiceIdentitySessionPtr &identitySession = (*pendingCurrentIter).second;
            IdentityInfo pendingInfo = identitySession->forLockbox().getIdentityInfo();

            if ((info.mURI == pendingInfo.mURI) &&
                (info.mProvider == pendingInfo.mProvider)) {
              // found an existing match...
              ZS_LOG_DEBUG(log("found a match to a pending identity (moving pending identity to associated identity)") + ", uri=" + info.mURI + ", provider=" + info.mProvider)

              // move the pending identity to the actual identity rather than creating a new identity
              mAssociatedIdentities[identitySession->forLockbox().getID()] = identitySession;

              foundMatch = true;
              break;
            }
          }

          if (foundMatch) continue;

          // no match to an existing identity, attempt a relogin
          String domain;
          String id;
          IServiceIdentity::splitURI(info.mURI, domain, id);

          BootstrappedNetworkPtr network = mBootstrappedNetwork;
          if (domain != info.mProvider) {
            // not using the lockbox provider, instead using the provider specified
            network = BootstrappedNetwork::convert(IBootstrappedNetwork::prepare(info.mProvider));
          }

          String hash = IHelper::convertToHex(*IHelper::hash(String("identity-relogin:") + info.mURI + ":" + info.mProvider));

          String reloginKey = getContent(OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE, hash);

          ZS_LOG_DEBUG(log("reloading identity") + ", identity uri=" + info.mURI + ", provider=" + info.mProvider + ", relogin key=" + reloginKey)

          ServiceIdentitySessionPtr identitySession = IServiceIdentitySessionForServiceLockbox::reload(network, mGrantSession, mThisWeak.lock(), info.mURI, reloginKey);
          mAssociatedIdentities[identitySession->forLockbox().getID()] = identitySession;
        }

        // all server identities should now be processed or matched
        mServerIdentities.clear();

        ZS_LOG_DEBUG(log("finished moving server identity to associated identities"))
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepPruneDuplicatePendingIdentities()
      {
        if (mPendingUpdateIdentities.size() < 1) {
          ZS_LOG_DEBUG(log("no identities pending update"))
          return true;
        }

        ZS_LOG_DEBUG(log("checking if any pending identities that are already associated that need to be pruned"))

        for (ServiceIdentitySessionMap::iterator assocIter = mAssociatedIdentities.begin(); assocIter != mAssociatedIdentities.end(); ++assocIter)
        {
          ServiceIdentitySessionPtr &identitySession = (*assocIter).second;
          IdentityInfo associatedInfo = identitySession->forLockbox().getIdentityInfo();

          for (ServiceIdentitySessionMap::iterator pendingUpdateIter = mPendingUpdateIdentities.begin(); pendingUpdateIter != mPendingUpdateIdentities.end();)
          {
            ServiceIdentitySessionMap::iterator pendingUpdateCurrentIter = pendingUpdateIter;
            ++pendingUpdateIter;

            ServiceIdentitySessionPtr &pendingIdentity = (*pendingUpdateCurrentIter).second;

            IdentityInfo pendingUpdateInfo = pendingIdentity->forLockbox().getIdentityInfo();
            if ((pendingUpdateInfo.mURI == associatedInfo.mURI) &&
                (pendingUpdateInfo.mProvider = associatedInfo.mProvider)) {
              ZS_LOG_DEBUG(log("identity pending update is actually already associated so remove it from the pending list (thus will prune this identity)") + ", uri=" + associatedInfo.mURI + ", provider=" + associatedInfo.mProvider)
              mPendingUpdateIdentities.erase(pendingUpdateCurrentIter);
              continue;
            }
          }
        }

        ZS_LOG_DEBUG(log("finished pruning duplicate pending identities"))
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepPruneShutdownIdentities()
      {
        ZS_LOG_DEBUG(log("pruning shutdown identities"))

        for (ServiceIdentitySessionMap::iterator pendingUpdateIter = mPendingUpdateIdentities.begin(); pendingUpdateIter != mPendingUpdateIdentities.end();)
        {
          ServiceIdentitySessionMap::iterator pendingUpdateCurrentIter = pendingUpdateIter;
          ++pendingUpdateIter;

          ServiceIdentitySessionPtr &pendingUpdateIdentity = (*pendingUpdateCurrentIter).second;
          if (!pendingUpdateIdentity->forLockbox().isShutdown()) continue;

          // cannot associate an identity that shutdown
          ZS_LOG_WARNING(Detail, log("pending identity shutdown unexpectedly") + IServiceIdentitySession::toDebugString(pendingUpdateIdentity));
          mPendingUpdateIdentities.erase(pendingUpdateCurrentIter);
        }

        ZS_LOG_DEBUG(log("pruning of shutdown identities complete"))

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceLockboxSession::stepPendingAssociationAndRemoval()
      {
        typedef LockboxContentSetRequest::NameValueMap NameValueMap;
        typedef LockboxContentSetRequest::NamespaceURLNameValueMap NamespaceURLNameValueMap;

        if ((mLockboxIdentitiesUpdateMonitor) ||
            (mLockboxContentSetMonitor)) {
          ZS_LOG_DEBUG(log("waiting for identities association or content set to complete before attempting to associate identities"))
          return false;
        }

        if ((mPendingUpdateIdentities.size() < 1) &&
            (mPendingRemoveIdentities.size() < 1)) {
          ZS_LOG_DEBUG(log("no identities are pending addition or removal"))
          return true;
        }

        NameValueMap reloginValues;
        NameValueMap signatureValues;

        ServiceIdentitySessionMap removedIdentities;
        ServiceIdentitySessionMap completedIdentities;

        for (ServiceIdentitySessionMap::iterator pendingRemovalIter = mPendingRemoveIdentities.begin(); pendingRemovalIter != mPendingRemoveIdentities.end();)
        {
          ServiceIdentitySessionMap::iterator pendingRemovalCurrentIter = pendingRemovalIter;
          ++pendingRemovalIter;

          ServiceIdentitySessionPtr &pendingRemovalIdentity = (*pendingRemovalCurrentIter).second;
          IdentityInfo pendingRemovalIdentityInfo = pendingRemovalIdentity->forLockbox().getIdentityInfo();

          ZS_LOG_DEBUG(log("checking if identity to be removed is in the udpate list") + IServiceIdentitySession::toDebugString(pendingRemovalIdentity))

          for (ServiceIdentitySessionMap::iterator pendingUpdateIter = mPendingUpdateIdentities.begin(); pendingUpdateIter != mPendingUpdateIdentities.end(); )
          {
            ServiceIdentitySessionMap::iterator pendingUpdateCurrentIter = pendingUpdateIter;
            ++pendingUpdateIter;

            ServiceIdentitySessionPtr &pendingUpdateIdentity = (*pendingUpdateCurrentIter).second;

            if (pendingUpdateIdentity->forLockbox().getID() == pendingRemovalIdentity->forLockbox().getID()) {
              ZS_LOG_DEBUG(log("identity being removed is in the pending list (thus will remove it from pending list)") + IServiceIdentitySession::toDebugString(pendingRemovalIdentity))

              mPendingUpdateIdentities.erase(pendingUpdateCurrentIter);
              continue;
            }
          }

          bool foundMatch = false;

          for (ServiceIdentitySessionMap::iterator associatedIter = mAssociatedIdentities.begin(); associatedIter != mAssociatedIdentities.end();)
          {
            ServiceIdentitySessionMap::iterator associatedCurrentIter = associatedIter;
            ++associatedIter;

            ServiceIdentitySessionPtr &associatedIdentity = (*associatedCurrentIter).second;

            if (associatedIdentity->forLockbox().getID() != pendingRemovalIdentity->forLockbox().getID()) continue;

            foundMatch = true;

            ZS_LOG_DEBUG(log("killing association to the associated identity") + IServiceIdentitySession::toDebugString(pendingRemovalIdentity))

            // clear relogin key (if present)
            {
              String hash = IHelper::convertToHex(*IHelper::hash(String("identity-relogin:") + pendingRemovalIdentityInfo.mURI + ":" + pendingRemovalIdentityInfo.mProvider));
              reloginValues[hash] = "-";
              clearContent(OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE, hash);
            }

            removedIdentities[pendingRemovalIdentity->forLockbox().getID()] = pendingRemovalIdentity;

            // force the identity to disassociate from the lockbox
            pendingRemovalIdentity->forLockbox().killAssociation(mThisWeak.lock());

            mAssociatedIdentities.erase(associatedCurrentIter);
            mPendingRemoveIdentities.erase(pendingRemovalCurrentIter);
          }

          if (foundMatch) continue;

          ZS_LOG_DEBUG(log("killing identity that was never associated") + IServiceIdentitySession::toDebugString(pendingRemovalIdentity))

          mPendingRemoveIdentities.erase(pendingRemovalCurrentIter);
        }

        for (ServiceIdentitySessionMap::iterator pendingUpdateIter = mPendingUpdateIdentities.begin(); pendingUpdateIter != mPendingUpdateIdentities.end();)
        {
          ServiceIdentitySessionMap::iterator pendingUpdateCurrentIter = pendingUpdateIter;
          ++pendingUpdateIter;

          ServiceIdentitySessionPtr &pendingUpdateIdentity = (*pendingUpdateCurrentIter).second;

          if (!pendingUpdateIdentity->forLockbox().isLoginComplete()) continue;

          ZS_LOG_DEBUG(log("pending identity is now logged in (thus can cause the association)") + IServiceIdentitySession::toDebugString(pendingUpdateIdentity))
          completedIdentities[pendingUpdateIdentity->forLockbox().getID()] = pendingUpdateIdentity;

          IdentityInfo info = pendingUpdateIdentity->forLockbox().getIdentityInfo();
          if (info.mReloginKey.hasData()) {
            String hash = IHelper::convertToHex(*IHelper::hash(String("identity-relogin:") + info.mURI + ":" + info.mProvider));

            setContent(OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE, hash, info.mReloginKey);
            String rawValue = getRawContent(OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE, hash);
            reloginValues[hash] = rawValue;
          }
        }

        NamespaceURLNameValueMap namespaces;

        if (reloginValues.size() > 0) {
          ZS_LOG_DEBUG(log("contains relogin values to update") + "values=" + string(reloginValues.size()))
          namespaces[OPENPEER_STACK_SERVICE_LOCKBOX_IDENTITY_RELOGINS_NAMESPACE] = reloginValues;
        }

        if (namespaces.size() > 0) {
          ZS_LOG_DEBUG(log("sending content set request"))

          LockboxContentSetRequestPtr request = LockboxContentSetRequest::create();
          request->domain(mBootstrappedNetwork->forServices().getDomain());
          request->lockboxInfo(mLockboxInfo);

          request->namespaceURLNameValues(namespaces);

          mLockboxContentSetMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<LockboxContentSetResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS));
          mBootstrappedNetwork->forServices().sendServiceMessage("identity-lockbox", "lockbox-content-set", request);
        }

        if ((removedIdentities.size() > 0) ||
            (completedIdentities.size() > 0))
        {
          IdentityInfoList removeInfos;
          IdentityInfoList updateInfos;

          for (ServiceIdentitySessionMap::iterator iter = removedIdentities.begin(); iter != removedIdentities.end(); ++iter)
          {
            ServiceIdentitySessionPtr &identity = (*iter).second;

            IdentityInfo info = identity->forLockbox().getIdentityInfo();

            if ((info.mURI.hasData()) &&
                (info.mProvider.hasData())) {
              ZS_LOG_DEBUG(log("adding identity to request removal list") + info.getDebugValueString())
              removeInfos.push_back(info);
            }
          }

          for (ServiceIdentitySessionMap::iterator iter = completedIdentities.begin(); iter != completedIdentities.end(); ++iter)
          {
            ServiceIdentitySessionPtr &identity = (*iter).second;

            IdentityInfo info = identity->forLockbox().getIdentityInfo();

            ZS_LOG_DEBUG(log("adding identity to request update list") + info.getDebugValueString())
            updateInfos.push_back(info);
          }

          if ((removeInfos.size() > 0) &&
              (updateInfos.size() > 0)) {

            ZS_LOG_DEBUG(log("sending update identities request"))

            LockboxIdentitiesUpdateRequestPtr request = LockboxIdentitiesUpdateRequest::create();
            request->domain(mBootstrappedNetwork->forServices().getDomain());
            request->lockboxInfo(mLockboxInfo);
            request->identitiesToUpdate(updateInfos);
            request->identitiesToRemove(removeInfos);

            mLockboxIdentitiesUpdateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<LockboxIdentitiesUpdateResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_LOCKBOX_TIMEOUT_IN_SECONDS));
            mBootstrappedNetwork->forServices().sendServiceMessage("identity-lockbox", "lockbox-identities-update", request);

            // NOTE: It's entirely possible the associate request can fail. Unfortunately, there is very little that can be done upon failure. The user will have to take some responsibility to keep their identities associated.
          }
        }

        ZS_LOG_DEBUG(log("associating and removing of identities completed") + ", updated=" + string(completedIdentities.size()) + ", removed=" + string(removedIdentities.size()))
        return true;
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::postStep()
      {
        calculateAndNotifyIdentityChanges();
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::setState(SessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState) + getDebugValueString())
        mCurrentState = state;

        if (mLoginIdentity) {
          mLoginIdentity->forLockbox().notifyStateChanged();
        }

        AccountPtr account = mAccount.lock();
        if (account) {
          account->forServiceLockboxSession().notifyServiceLockboxSessionStateChanged();
        }

        ServiceLockboxSessionPtr pThis = mThisWeak.lock();
        if ((pThis) &&
            (mDelegate)) {
          try {
            ZS_LOG_DEBUG(log("attempting to report state to delegate") + getDebugValueString())
            mDelegate->onServiceLockboxSessionStateChanged(pThis, mCurrentState);
          } catch (IServiceLockboxSessionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());

        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }
        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("erorr already set thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        mLastError = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + ", code=" + string(mLastError) + ", reason=" + mLastErrorReason + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      void ServiceLockboxSession::calculateAndNotifyIdentityChanges()
      {
        MD5 hasher;
        SecureByteBlockPtr output(new SecureByteBlock(hasher.DigestSize()));

        for (ServiceIdentitySessionMap::iterator iter = mAssociatedIdentities.begin(); iter != mAssociatedIdentities.end(); ++iter)
        {
          PUID id = (*iter).first;
          hasher.Update((const BYTE *)(&id), sizeof(id));
        }
        hasher.Final(*output);

        if (!mLastNotificationHash) {
          ZS_LOG_DEBUG(log("calculated identities for the first time"))
          mLastNotificationHash = output;
          return;
        }

        if (0 == IHelper::compare(*output, *mLastNotificationHash)) {
          // no change
          return;
        }

        mLastNotificationHash = output;

        if (mDelegate) {
          try {
            mDelegate->onServiceLockboxSessionAssociatedIdentitiesChanged(mThisWeak.lock());
          } catch(IServiceLockboxSessionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::getContent(
                                               const char *namespaceURL,
                                               const char *valueName
                                               ) const
      {
        typedef IHelper::SplitMap SplitMap;
        typedef LockboxContentGetResult::NameValueMap NameValueMap;

        ZS_THROW_INVALID_ARGUMENT_IF(!namespaceURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!valueName)

        if (!mLockboxInfo.mKey) {
          ZS_LOG_DEBUG(log("lockbox key is not known") + ", namespace=" + namespaceURL + ", value name=" + valueName + getDebugValueString())
          return String();
        }

        NamespaceURLNameValueMap::const_iterator found = mContent.find(namespaceURL);
        if (found == mContent.end()) {
          ZS_LOG_WARNING(Detail, log("content does not contain namespace") + ", namespace=" + namespaceURL + ", value name=" + valueName)
          return String();
        }

        const NameValueMap &values = (*found).second;

        NameValueMap::const_iterator foundValue = values.find(valueName);
        if (foundValue == values.end()) {
          ZS_LOG_WARNING(Detail, log("content does not contain namespace value") + ", namespace=" + namespaceURL + ", value name=" + valueName)
          return String();
        }

        const String &preSplitValue = (*foundValue).second;

        IHelper::SplitMap split;
        IHelper::split(preSplitValue, split, ':');

        if (split.size() != 2) {
          ZS_LOG_DEBUG(log("failed to split value into salt and encrypted value") + ", namespace=" + namespaceURL + ", value name=" + valueName + getDebugValueString())
          return String();
        }

        const String &salt = split[0];
        const String &value = split[1];

        ZS_LOG_TRACE(log("decrypting content using") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", key=" + IHelper::convertToBase64(*mLockboxInfo.mKey))

        SecureByteBlockPtr key = IHelper::hmac(*mLockboxInfo.mKey, (String("lockbox:") + namespaceURL + ":" + valueName).c_str(), IHelper::HashAlgorthm_SHA256);
        SecureByteBlockPtr iv = IHelper::hash(salt, IHelper::HashAlgorthm_MD5);

        SecureByteBlockPtr dataToConvert = IHelper::convertFromBase64(value);
        if (!dataToConvert) {
          ZS_LOG_WARNING(Detail, log("failed to decode data from base64") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", value=" + value + getDebugValueString())
          return String();
        }

        SecureByteBlockPtr result = IHelper::decrypt(*key, *iv, *dataToConvert);
        if (!result) {
          ZS_LOG_WARNING(Detail, log("failed to decrypt value") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", value=" + value + getDebugValueString())
          return String();
        }

        String output = IHelper::convertToString(*result);

        ZS_LOG_TRACE(log("obtained content") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", output=" + output)
        return output;
      }

      //-----------------------------------------------------------------------
      String ServiceLockboxSession::getRawContent(
                                                  const char *namespaceURL,
                                                  const char *valueName
                                                  ) const
      {
        typedef LockboxContentGetResult::NameValueMap NameValueMap;

        ZS_THROW_INVALID_ARGUMENT_IF(!namespaceURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!valueName)

        NamespaceURLNameValueMap::const_iterator found = mContent.find(namespaceURL);
        if (found == mContent.end()) {
          ZS_LOG_WARNING(Detail, log("content does not contain namespace") + ", namespace=" + namespaceURL + ", value name=" + valueName)
          return String();
        }

        const NameValueMap &values = (*found).second;

        NameValueMap::const_iterator foundValue = values.find(valueName);
        if (foundValue == values.end()) {
          ZS_LOG_WARNING(Detail, log("content does not contain namespace value") + ", namespace=" + namespaceURL + ", value name=" + valueName)
          return String();
        }

        const String &value = (*foundValue).second;

        ZS_LOG_TRACE(log("found raw content value") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", raw value=" + value)
        return value;
      }
      
      //-----------------------------------------------------------------------
      void ServiceLockboxSession::setContent(
                                             const char *namespaceURL,
                                             const char *valueName,
                                             const char *value
                                             )
      {
        typedef LockboxContentGetResult::NameValueMap NameValueMap;

        ZS_THROW_INVALID_ARGUMENT_IF(!namespaceURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!valueName)

        NamespaceURLNameValueMap::iterator found = mContent.find(namespaceURL);
        if (found == mContent.end()) {
          ZS_LOG_WARNING(Detail, log("content does not contain namespace") + ", namespace=" + namespaceURL + ", value name=" + valueName)

          NameValueMap empty;
          mContent[namespaceURL] = empty;
          found = mContent.find(namespaceURL);

          ZS_THROW_BAD_STATE_IF(found == mContent.end())
        }

        NameValueMap &values = (*found).second;

        if (!mLockboxInfo.mKey) {
          ZS_LOG_DEBUG(log("failed to create a combined key") + ", namespace=" + namespaceURL + ", value name=" + valueName + getDebugValueString())
          return;
        }

        ZS_LOG_TRACE(log("encrpting content using") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", key=" + IHelper::convertToBase64(*mLockboxInfo.mKey))

        String salt = IHelper::randomString((20*8)/5);

        SecureByteBlockPtr key = IHelper::hmac(*mLockboxInfo.mKey, (String("lockbox:") + namespaceURL + ":" + valueName).c_str(), IHelper::HashAlgorthm_SHA256);
        SecureByteBlockPtr iv = IHelper::hash(salt, IHelper::HashAlgorthm_MD5);

        SecureByteBlockPtr dataToConvert = IHelper::convertToBuffer(value);
        if (!dataToConvert) {
          ZS_LOG_WARNING(Detail, log("failed to prepare data to convert") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", value=" + value + getDebugValueString())
          return;
        }

        SecureByteBlockPtr result = IHelper::encrypt(*key, *iv, *dataToConvert);
        if (!result) {
          ZS_LOG_WARNING(Detail, log("failed to decrypt value") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", value=" + value + getDebugValueString())
          return;
        }

        String encodedValue = salt + ":" + IHelper::convertToBase64(*result);
        if (encodedValue.isEmpty()) {
          ZS_LOG_WARNING(Detail, log("failed to encode encrypted to base64") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", value=" + value + getDebugValueString())
          return;
        }

        ZS_LOG_TRACE(log("content was set") + ", namespace=" + namespaceURL + ", value name=" + valueName + ", value=" + value)
        values[valueName] = encodedValue;
      }
      
      //-----------------------------------------------------------------------
      void ServiceLockboxSession::clearContent(
                                               const char *namespaceURL,
                                               const char *valueName
                                               )
      {
        typedef LockboxContentGetResult::NameValueMap NameValueMap;

        ZS_THROW_INVALID_ARGUMENT_IF(!namespaceURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!valueName)

        NamespaceURLNameValueMap::iterator found = mContent.find(namespaceURL);
        if (found == mContent.end()) {
          ZS_LOG_WARNING(Detail, log("content does not contain namespace") + ", namespace=" + namespaceURL + ", value name=" + valueName)
          return;
        }

        NameValueMap &values = (*found).second;

        NameValueMap::iterator foundValue = values.find(valueName);
        if (foundValue == values.end()) {
          ZS_LOG_WARNING(Detail, log("content does not contain namespace value") + ", namespace=" + namespaceURL + ", value name=" + valueName)
          return;
        }

        ZS_LOG_TRACE(log("content value cleared") + ", namespace=" + namespaceURL + ", value name=" + valueName)
        values.erase(foundValue);
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
    #pragma mark IServiceLockboxSession
    #pragma mark


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceLockboxSession
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IServiceLockboxSession::toString(SessionStates state)
    {
      switch (state)
      {
        case SessionState_Pending:                                return "Pending";
        case SessionState_PendingPeerFilesGeneration:             return "Pending Peer File Generation";
        case SessionState_Ready:                                  return "Ready";
        case SessionState_Shutdown:                               return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    String IServiceLockboxSession::toDebugString(IServiceLockboxSessionPtr session, bool includeCommaPrefix)
    {
      return internal::ServiceLockboxSession::toDebugString(session, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IServiceLockboxSessionPtr IServiceLockboxSession::login(
                                                            IServiceLockboxSessionDelegatePtr delegate,
                                                            IServiceLockboxPtr serviceLockbox,
                                                            IServiceNamespaceGrantSessionPtr grantSession,
                                                            IServiceIdentitySessionPtr identitySession,
                                                            bool forceNewAccount
                                                            )
    {
      return internal::IServiceLockboxSessionFactory::singleton().login(delegate, serviceLockbox, grantSession, identitySession, forceNewAccount);
    }

    //-------------------------------------------------------------------------
    IServiceLockboxSessionPtr IServiceLockboxSession::relogin(
                                                              IServiceLockboxSessionDelegatePtr delegate,
                                                              IServiceLockboxPtr serviceLockbox,
                                                              IServiceNamespaceGrantSessionPtr grantSession,
                                                              const char *lockboxAccountID,
                                                              const SecureByteBlock &lockboxKey
                                                              )
    {
      return internal::IServiceLockboxSessionFactory::singleton().relogin(delegate, serviceLockbox, grantSession, lockboxAccountID, lockboxKey);
    }

  }
}
