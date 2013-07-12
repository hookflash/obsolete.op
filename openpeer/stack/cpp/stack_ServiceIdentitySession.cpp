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

#include <openpeer/stack/internal/stack_ServiceIdentitySession.h>
#include <openpeer/stack/internal/stack_ServiceLockboxSession.h>
#include <openpeer/stack/message/identity/IdentityAccessWindowRequest.h>
#include <openpeer/stack/message/identity/IdentityAccessWindowResult.h>
#include <openpeer/stack/message/identity/IdentityAccessStartNotify.h>
#include <openpeer/stack/message/identity/IdentityAccessCompleteNotify.h>
#include <openpeer/stack/message/identity/IdentityAccessLockboxUpdateRequest.h>
#include <openpeer/stack/message/identity/IdentityLookupUpdateRequest.h>
#include <openpeer/stack/message/identity/IdentitySignRequest.h>
#include <openpeer/stack/message/identity-lookup/IdentityLookupRequest.h>
#include <openpeer/stack/internal/stack_BootstrappedNetwork.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/IHelper.h>
#include <openpeer/stack/IPeer.h>
#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePrivate.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/message/IMessageHelper.h>
#include <openpeer/stack/internal/stack_Stack.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>

#include <zsLib/RegEx.h>

#define OPENPEER_STACK_SERVIC_IDENTITY_SIGN_CREATE_SHOULD_NOT_BE_BEFORE_NOW_IN_HOURS (72)

#define OPENPEER_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS (60*2)
#define OPENPEER_STACK_SERVICE_IDENTITY_MAX_PERCENTAGE_TIME_REMAINING_BEFORE_RESIGN_IDENTITY_REQUIRED (20) // at 20% of the remaining on the certificate before expiry, resign

#define OPENPEER_STACK_SERVIC_IDENTITY_SIGN_CREATE_SHOULD_NOT_BE_BEFORE_NOW_IN_HOURS (72)

#define OPENPEER_STACK_SERVICE_IDENTITY_ROLODEX_CONTACTS_NAMESPACE "https://openpeer.org/permission/rolodex-contacts"


namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      using zsLib::Stringize;

      using message::identity::IdentityAccessWindowRequest;
      using message::identity::IdentityAccessWindowRequestPtr;
      using message::identity::IdentityAccessWindowResult;
      using message::identity::IdentityAccessWindowResultPtr;
      using message::identity::IdentityAccessStartNotify;
      using message::identity::IdentityAccessStartNotifyPtr;
      using message::identity::IdentityAccessCompleteNotify;
      using message::identity::IdentityAccessCompleteNotifyPtr;
      using message::identity::IdentityAccessLockboxUpdateRequest;
      using message::identity::IdentityAccessLockboxUpdateRequestPtr;
      using message::identity::IdentityLookupUpdateRequest;
      using message::identity::IdentityLookupUpdateRequestPtr;
      using message::identity::IdentitySignRequest;
      using message::identity::IdentitySignRequestPtr;
      using message::identity_lookup::IdentityLookupRequest;
      using message::identity_lookup::IdentityLookupRequestPtr;

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
          OPENPEER_STACK_SERVICE_IDENTITY_ROLODEX_CONTACTS_NAMESPACE,
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
      static char getSafeSplitChar(const String &identifier)
      {
        const char *testChars = ",; :./\\*#!$%&@?~+=-_|^<>[]{}()";

        while (*testChars) {
          if (String::npos == identifier.find(*testChars)) {
            return *testChars;
          }

          ++testChars;
        }

        return 0;
      }
      
      //-----------------------------------------------------------------------
      static bool isSame(
                         IPeerFilePublicPtr peerFilePublic1,
                         IPeerFilePublicPtr peerFilePublic2
                         )
      {
        if (!peerFilePublic1) return false;
        if (!peerFilePublic2) return false;

        return peerFilePublic1->getPeerURI() == peerFilePublic2->getPeerURI();
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceIdentitySessionForServiceLockbox
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr IServiceIdentitySessionForServiceLockbox::reload(
                                                                                 BootstrappedNetworkPtr provider,
                                                                                 IServiceNamespaceGrantSessionPtr grantSession,
                                                                                 IServiceLockboxSessionPtr existingLockbox,
                                                                                 const char *identityURI,
                                                                                 const char *reloginKey
                                                                                 )
      {
        return IServiceIdentitySessionFactory::singleton().reload(provider, grantSession, existingLockbox, identityURI, reloginKey);
        return ServiceIdentitySessionPtr();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceIdentitySession::ServiceIdentitySession(
                                                     IMessageQueuePtr queue,
                                                     IServiceIdentitySessionDelegatePtr delegate,
                                                     BootstrappedNetworkPtr providerNetwork,
                                                     BootstrappedNetworkPtr identityNetwork,
                                                     ServiceNamespaceGrantSessionPtr grantSession,
                                                     ServiceLockboxSessionPtr existingLockbox,
                                                     const char *outerFrameURLUponReload
                                                     ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(delegate ? IServiceIdentitySessionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate) : IServiceIdentitySessionDelegatePtr()),
        mAssociatedLockbox(existingLockbox),
        mKillAssociation(false),
        mProviderBootstrappedNetwork(providerNetwork),
        mIdentityBootstrappedNetwork(identityNetwork),
        mGrantSession(grantSession),
        mCurrentState(SessionState_Pending),
        mLastReportedState(SessionState_Pending),
        mLastError(0),
        mOuterFrameURLUponReload(outerFrameURLUponReload),
        mBrowserWindowReady(false),
        mBrowserWindowVisible(false),
        mBrowserWindowClosed(false),
        mNeedsBrowserWindowVisible(false),
        mIdentityAccessStartNotificationSent(false),
        mLockboxUpdated(false),
        mIdentityLookupUpdated(false)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySession::~ServiceIdentitySession()
      {
        if(isNoop()) return;

        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::init()
      {
        if (mIdentityBootstrappedNetwork) {
          IBootstrappedNetworkForServices::prepare(mIdentityBootstrappedNetwork->forServices().getDomain(), mThisWeak.lock());
        }
        if (mProviderBootstrappedNetwork) {
          IBootstrappedNetworkForServices::prepare(mProviderBootstrappedNetwork->forServices().getDomain(), mThisWeak.lock());
        }

        // one or the other must be valid or a login is not possible
        ZS_THROW_BAD_STATE_IF((!mIdentityBootstrappedNetwork) && (!mProviderBootstrappedNetwork))
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr ServiceIdentitySession::convert(IServiceIdentitySessionPtr query)
      {
        return boost::dynamic_pointer_cast<ServiceIdentitySession>(query);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IServiceIdentitySession
      #pragma mark

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::toDebugString(IServiceIdentitySessionPtr session, bool includeCommaPrefix)
      {
        if (!session) return includeCommaPrefix ? String(", identity session=(null)") : String("identity session=(null)");
        return ServiceIdentitySession::convert(session)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr ServiceIdentitySession::loginWithIdentity(
                                                                          IServiceIdentitySessionDelegatePtr delegate,
                                                                          IServiceIdentityPtr provider,
                                                                          IServiceNamespaceGrantSessionPtr grantSession,
                                                                          IServiceLockboxSessionPtr existingLockbox,
                                                                          const char *outerFrameURLUponReload,
                                                                          const char *identityURI
                                                                          )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantSession)
        ZS_THROW_INVALID_ARGUMENT_IF(!outerFrameURLUponReload)
        ZS_THROW_INVALID_ARGUMENT_IF(!identityURI)

        if (!provider) {
          if (IServiceIdentity::isLegacy(identityURI)) {
            ZS_THROW_INVALID_ARGUMENT_IF(!provider) // provider can be derived from the identity but only if the identity contains a provider
          }
        }

        if ((!IServiceIdentity::isValid(identityURI)) &&
            (!IServiceIdentity::isValidBase(identityURI))) {
          ZS_LOG_ERROR(Detail, String("identity URI specified is not valid, uri=") + identityURI)
          return ServiceIdentitySessionPtr();
        }

        BootstrappedNetworkPtr identityNetwork;
        BootstrappedNetworkPtr providerNetwork = BootstrappedNetwork::convert(provider);

        if (IServiceIdentity::isValid(identityURI)) {
          if (!IServiceIdentity::isLegacy(identityURI)) {
            String domain;
            String identifier;
            IServiceIdentity::splitURI(identityURI, domain, identifier);
            identityNetwork = IBootstrappedNetworkForServices::prepare(domain);
          }
        }

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(IStackForInternal::queueStack(), delegate, providerNetwork, identityNetwork, ServiceNamespaceGrantSession::convert(grantSession), ServiceLockboxSession::convert(existingLockbox), outerFrameURLUponReload));
        pThis->mThisWeak = pThis;
        if (IServiceIdentity::isValidBase(identityURI)) {
          pThis->mIdentityInfo.mBase = identityURI;
        } else {
          pThis->mIdentityInfo.mURI = identityURI;
        }
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr ServiceIdentitySession::loginWithIdentityProvider(
                                                                                  IServiceIdentitySessionDelegatePtr delegate,
                                                                                  IServiceIdentityPtr provider,
                                                                                  IServiceNamespaceGrantSessionPtr grantSession,
                                                                                  IServiceLockboxSessionPtr existingLockbox,
                                                                                  const char *outerFrameURLUponReload,
                                                                                  const char *legacyIdentityBaseURI
                                                                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!provider)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantSession)
        ZS_THROW_INVALID_ARGUMENT_IF(!outerFrameURLUponReload)

        if (legacyIdentityBaseURI) {
          return loginWithIdentity(delegate, provider, grantSession, existingLockbox, outerFrameURLUponReload, legacyIdentityBaseURI);
        }

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(IStackForInternal::queueStack(), delegate, BootstrappedNetwork::convert(provider), BootstrappedNetworkPtr(), ServiceNamespaceGrantSession::convert(grantSession), ServiceLockboxSession::convert(existingLockbox), outerFrameURLUponReload));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr ServiceIdentitySession::loginWithIdentityBundle(
                                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                                IServiceIdentityPtr provider,
                                                                                IServiceNamespaceGrantSessionPtr grantSession,
                                                                                IServiceLockboxSessionPtr existingLockbox,
                                                                                const char *outerFrameURLUponReload,
                                                                                ElementPtr signedIdentityBundleEl
                                                                                )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!grantSession)
        ZS_THROW_INVALID_ARGUMENT_IF(!outerFrameURLUponReload)
        ZS_THROW_INVALID_ARGUMENT_IF(!signedIdentityBundleEl)

        ElementPtr signatureEl;
        ElementPtr signedIdentityEl = IHelper::getSignatureInfo(signedIdentityBundleEl, &signatureEl);

        if (!signedIdentityEl) {
          ZS_LOG_DEBUG("failed to obtain signature from signed identity")
          return ServiceIdentitySessionPtr();
        }

        String domain;
        String peerURI;
        String identityURI;

        try
        {
          domain = signatureEl->findFirstChildElementChecked("key")->findFirstChildElementChecked("domain")->getTextDecoded();
          peerURI = signedIdentityEl->findFirstChildElementChecked("contact")->getTextDecoded();
          identityURI = signedIdentityEl->findFirstChildElementChecked("uri")->getTextDecoded();
        } catch(CheckFailed &) {
          ZS_LOG_DEBUG("failed to obtain signature information from signed identity")
          return ServiceIdentitySessionPtr();
        }

        if (!IHelper::isValidDomain(domain)) {
          ZS_LOG_WARNING(Detail, "identity domain information is not valid, domain=" + domain)
          return ServiceIdentitySessionPtr();
        }
        if (!IPeer::isValid(peerURI)) {
          ZS_LOG_WARNING(Detail, "peer URI is not valid, peer URI=" + peerURI)
          return ServiceIdentitySessionPtr();
        }
        if (!IServiceIdentity::isValid(identityURI)) {
          ZS_LOG_WARNING(Detail, "identity URI is not valid, identity URI=" + identityURI)
          return ServiceIdentitySessionPtr();
        }

        BootstrappedNetworkPtr network = IBootstrappedNetworkForServices::prepare(domain);

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(IStackForInternal::queueStack(), delegate, BootstrappedNetwork::convert(provider), network, ServiceNamespaceGrantSession::convert(grantSession), ServiceLockboxSession::convert(existingLockbox), outerFrameURLUponReload));
        pThis->mThisWeak = pThis;
        pThis->mIdentityInfo.mURI = identityURI;
        pThis->mSignedIdentityBundleUncheckedEl = signedIdentityBundleEl->clone()->toElement();
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IServiceIdentityPtr ServiceIdentitySession::getService() const
      {
        AutoRecursiveLock lock(getLock());
        if (mIdentityBootstrappedNetwork) {
          return mIdentityBootstrappedNetwork;
        }
        return mProviderBootstrappedNetwork;
      }

      //-----------------------------------------------------------------------
      IServiceIdentitySession::SessionStates ServiceIdentitySession::getState(
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
      bool ServiceIdentitySession::isDelegateAttached() const
      {
        AutoRecursiveLock lock(getLock());
        return ((bool)mDelegate);
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::attachDelegate(
                                                  IServiceIdentitySessionDelegatePtr delegate,
                                                  const char *outerFrameURLUponReload
                                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!outerFrameURLUponReload)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        AutoRecursiveLock lock(getLock());

        mDelegate = IServiceIdentitySessionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate);
        mOuterFrameURLUponReload = (outerFrameURLUponReload ? String(outerFrameURLUponReload) : String());

        try {
          if (mCurrentState != mLastReportedState) {
            mDelegate->onServiceIdentitySessionStateChanged(mThisWeak.lock(), mCurrentState);
            mLastReportedState = mCurrentState;
          }
          if (mPendingMessagesToDeliver.size() > 0) {
            mDelegate->onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(mThisWeak.lock());
          }
        } catch(IServiceIdentitySessionDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
        }

        step();
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::getIdentityURI() const
      {
        AutoRecursiveLock lock(getLock());
        return mIdentityInfo.mURI.hasData() ? mIdentityInfo.mURI : mIdentityInfo.mBase;
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::getIdentityProviderDomain() const
      {
        AutoRecursiveLock lock(getLock());
        if (mIdentityBootstrappedNetwork) {
          return mIdentityBootstrappedNetwork->forServices().getDomain();
        }
        return mProviderBootstrappedNetwork->forServices().getDomain();
      }

      //-----------------------------------------------------------------------
      ElementPtr ServiceIdentitySession::getSignedIdentityBundle() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mSignedIdentityBundleVerfiedEl) return ElementPtr();
        return mSignedIdentityBundleVerfiedEl;
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::getInnerBrowserWindowFrameURL() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mActiveBootstrappedNetwork) return String();

        return mActiveBootstrappedNetwork->forServices().getServiceURI("identity", "identity-access-inner-frame");
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::notifyBrowserWindowVisible()
      {
        AutoRecursiveLock lock(getLock());
        mBrowserWindowVisible = true;
        step();
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::notifyBrowserWindowClosed()
      {
        AutoRecursiveLock lock(getLock());
        mBrowserWindowClosed = true;
        step();
      }

      //-----------------------------------------------------------------------
      DocumentPtr ServiceIdentitySession::getNextMessageForInnerBrowerWindowFrame()
      {
        AutoRecursiveLock lock(getLock());
        if (mPendingMessagesToDeliver.size() < 1) return DocumentPtr();

        DocumentPtr result = mPendingMessagesToDeliver.front();
        mPendingMessagesToDeliver.pop_front();

        if (ZS_GET_LOG_LEVEL() >= zsLib::Log::Trace) {
          GeneratorPtr generator = Generator::createJSONGenerator();
          boost::shared_array<char> jsonText = generator->write(result);
          ZS_LOG_TRACE(log("sending inner frame message") + ", message=" + (CSTR)(jsonText.get()))
        }

        if (mDelegate) {
          if (mPendingMessagesToDeliver.size() > 0) {
            try {
              ZS_LOG_DEBUG(log("notifying about another pending message for the inner frame"))
              mDelegate->onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(mThisWeak.lock());
            } catch (IServiceIdentitySessionDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
            }
          }
        }
        return result;
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::handleMessageFromInnerBrowserWindowFrame(DocumentPtr unparsedMessage)
      {
        if (ZS_GET_LOG_LEVEL() >= zsLib::Log::Trace) {
          GeneratorPtr generator = Generator::createJSONGenerator();
          boost::shared_array<char> jsonText = generator->write(unparsedMessage);
          ZS_LOG_TRACE(log("handling message from inner frame") + ", message=" + (CSTR)(jsonText.get()))
        }

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

        IdentityAccessWindowRequestPtr windowRequest = IdentityAccessWindowRequest::convert(message);
        if (windowRequest) {
          // send a result immediately
          IdentityAccessWindowResultPtr result = IdentityAccessWindowResult::create(windowRequest);
          sendInnerWindowMessage(result);

          if (windowRequest->ready()) {
            ZS_LOG_DEBUG(log("notified browser window ready"))
            mBrowserWindowReady = true;
          }

          if (windowRequest->visible()) {
            ZS_LOG_DEBUG(log("notified browser window needs to be made visible"))
            mNeedsBrowserWindowVisible = true;
          }

          IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        IdentityAccessCompleteNotifyPtr completeNotify = IdentityAccessCompleteNotify::convert(message);
        if (completeNotify) {

          const IdentityInfo &identityInfo = completeNotify->identityInfo();
          const LockboxInfo &lockboxInfo = completeNotify->lockboxInfo();

          ZS_LOG_DEBUG(log("received complete notification") + identityInfo.getDebugValueString() + lockboxInfo.getDebugValueString())

          mIdentityInfo.mergeFrom(completeNotify->identityInfo(), true);
          mLockboxInfo.mergeFrom(completeNotify->lockboxInfo(), true);

          if ((mIdentityInfo.mAccessToken.isEmpty()) ||
              (mIdentityInfo.mAccessSecret.isEmpty()) ||
              (mIdentityInfo.mURI.isEmpty())) {
            ZS_LOG_ERROR(Detail, log("failed to obtain access token/secret"))
            setError(IHTTP::HTTPStatusCode_Forbidden, "Login via identity provider failed");
            cancel();
            return;
          }

          IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();

          notifyLockboxStateChanged();
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
      void ServiceIdentitySession::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        mGraciousShutdownReference.reset();

        mPendingMessagesToDeliver.clear();

        if (mIdentityAccessLockboxUpdateMonitor) {
          mIdentityAccessLockboxUpdateMonitor->cancel();
          mIdentityAccessLockboxUpdateMonitor.reset();
        }

        if (mIdentityLookupUpdateMonitor) {
          mIdentityLookupUpdateMonitor->cancel();
          mIdentityLookupUpdateMonitor.reset();
        }

        if (mIdentitySignMonitor) {
          mIdentitySignMonitor->cancel();
          mIdentitySignMonitor.reset();
        }

        if (mIdentityLookupMonitor) {
          mIdentityLookupMonitor->cancel();
          mIdentityLookupMonitor.reset();
        }

        if (mGrantQuery) {
          mGrantQuery->cancel();
          mGrantQuery.reset();
        }

        if (mGrantWait) {
          mGrantWait->cancel();
          mGrantWait.reset();
        }

        setState(SessionState_Shutdown);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IMessageSource
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IServiceIdentitySessionForServiceLockbox
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr ServiceIdentitySession::reload(
                                                               BootstrappedNetworkPtr provider,
                                                               IServiceNamespaceGrantSessionPtr grantSession,
                                                               IServiceLockboxSessionPtr existingLockbox,
                                                               const char *identityURI,
                                                               const char *reloginKey
                                                               )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!existingLockbox)
        ZS_THROW_INVALID_ARGUMENT_IF(!provider)
        ZS_THROW_INVALID_ARGUMENT_IF(!existingLockbox)
        ZS_THROW_INVALID_ARGUMENT_IF(!identityURI)

        BootstrappedNetworkPtr identityNetwork;

        if (IServiceIdentity::isValid(identityURI)) {
          if (!IServiceIdentity::isLegacy(identityURI)) {
            String domain;
            String identifier;
            IServiceIdentity::splitURI(identityURI, domain, identifier);
            identityNetwork = IBootstrappedNetworkForServices::prepare(domain);
          }
        }

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(
                                                                   IStackForInternal::queueStack(),
                                                                   IServiceIdentitySessionDelegatePtr(),
                                                                   provider,
                                                                   identityNetwork,
                                                                   ServiceNamespaceGrantSession::convert(grantSession),
                                                                   ServiceLockboxSession::convert(existingLockbox),
                                                                   NULL
                                                                   ));
        pThis->mThisWeak = pThis;
        pThis->mAssociatedLockbox = ServiceLockboxSession::convert(existingLockbox);
        pThis->mIdentityInfo.mURI = identityURI;
        pThis->mIdentityInfo.mReloginKey = String(reloginKey);
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::associate(ServiceLockboxSessionPtr lockbox)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("associate called"))
        mAssociatedLockbox = lockbox;
        IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::killAssociation(ServiceLockboxSessionPtr peerContact)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("kill associate called"))

        if (mKillAssociation) {
          ZS_LOG_WARNING(Detail, log("asssoication already killed"))
          return;
        }

        // this shutdown must be performed graciously so that there is time to clean out the associations
        mGraciousShutdownReference = mThisWeak.lock();

        mAssociatedLockbox.reset();
        mKillAssociation = true;

        if (mIdentityAccessLockboxUpdateMonitor) {
          mIdentityAccessLockboxUpdateMonitor->cancel();
          mIdentityAccessLockboxUpdateMonitor.reset();
        }

        if (mIdentityLookupUpdateMonitor) {
          mIdentityLookupUpdateMonitor->cancel();
          mIdentityLookupUpdateMonitor.reset();
        }

        if (mIdentitySignMonitor) {
          mIdentitySignMonitor->cancel();
          mIdentitySignMonitor.reset();
        }

        if (mIdentityLookupMonitor) {
          mIdentityLookupMonitor->cancel();
          mIdentityLookupMonitor.reset();
        }

        mLockboxUpdated = false;
        mIdentityLookupUpdated = false;

        IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::notifyStateChanged()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("notify state changed"))
        IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::isLoginComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return mIdentityInfo.mAccessToken.hasData();
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::isShutdown() const
      {
        AutoRecursiveLock lock(getLock());
        return SessionState_Shutdown == mCurrentState;
      }

      //-----------------------------------------------------------------------
      IdentityInfo ServiceIdentitySession::getIdentityInfo() const
      {
        AutoRecursiveLock lock(getLock());
        return mIdentityInfo;
      }

      //-----------------------------------------------------------------------
      LockboxInfo ServiceIdentitySession::getLockboxInfo() const
      {
        AutoRecursiveLock lock(getLock());
        return mLockboxInfo;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IServiceIdentitySessionAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::onStep()
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
      #pragma mark ServiceIdentitySession => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork)
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
      #pragma mark ServiceIdentitySession => IServiceNamespaceGrantSessionForServicesWaitForWaitDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::onServiceNamespaceGrantSessionForServicesWaitComplete(IServiceNamespaceGrantSessionPtr session)
      {
        ZS_LOG_DEBUG(log("namespace grant waits have completed, can try again to obtain a wait (if waiting)"))

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IServiceNamespaceGrantSessionForServicesQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::onServiceNamespaceGrantSessionForServicesQueryComplete(
                                                                                          IServiceNamespaceGrantSessionForServicesQueryPtr query,
                                                                                          ElementPtr namespaceGrantChallengeBundleEl
                                                                                          )
      {
        ZS_LOG_DEBUG(log("namespace grant query completed"))

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IMessageMonitorResultDelegate<IdentityAccessLockboxUpdateResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorResultReceived(
                                                                      IMessageMonitorPtr monitor,
                                                                      IdentityAccessLockboxUpdateResultPtr result
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentityAccessLockboxUpdateMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mIdentityAccessLockboxUpdateMonitor->cancel();
        mIdentityAccessLockboxUpdateMonitor.reset();

        mLockboxUpdated = true;

        ZS_LOG_DEBUG(log("identity access lockbox update complete"))

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorErrorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           IdentityAccessLockboxUpdateResultPtr ignore, // will always be NULL
                                                                           message::MessageResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentityAccessLockboxUpdateMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        setError(result->errorCode(), result->errorReason());

        ZS_LOG_ERROR(Detail, log("identity access lockbox update failed"))

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IMessageMonitorResultDelegate<IdentityLoginCompleteResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorResultReceived(
                                                                      IMessageMonitorPtr monitor,
                                                                      IdentityLookupUpdateResultPtr result
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentityLookupUpdateMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mIdentityLookupUpdateMonitor->cancel();
        mIdentityLookupUpdateMonitor.reset();

        mIdentityLookupUpdated = true;

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorErrorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           IdentityLookupUpdateResultPtr ignore, // will always be NULL
                                                                           message::MessageResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentityLookupUpdateMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        setError(result->errorCode(), result->errorReason());

        ZS_LOG_DEBUG(log("identity login complete failed"))

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IMessageMonitorResultDelegate<IdentitySignResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorResultReceived(
                                                                      IMessageMonitorPtr monitor,
                                                                      IdentitySignResultPtr result
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentitySignMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mSignedIdentityBundleUncheckedEl = result->identityBundle();

        if (!mSignedIdentityBundleUncheckedEl) {
          ZS_LOG_ERROR(Detail, log("failed to obtain a signed identity bundle"))
          setError(IHTTP::HTTPStatusCode_CertError, "failed to obtain an identity bundle from the server");
          cancel();
          return true;
        }

        mSignedIdentityBundleUncheckedEl = mSignedIdentityBundleUncheckedEl->clone()->toElement();

        ZS_LOG_DEBUG(log("identity sign complete"))

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorErrorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           IdentitySignResultPtr ignore, // will always be NULL
                                                                           message::MessageResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentitySignMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        setError(result->errorCode(), result->errorReason());

        ZS_LOG_DEBUG(log("identity sign failed"))

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => IMessageMonitorResultDelegate<IdentitySignResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorResultReceived(
                                                                      IMessageMonitorPtr monitor,
                                                                      IdentityLookupResultPtr result
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentityLookupMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        // possible the identity lookup will not return any result, so we need to remember that it did in fact complete
        mPreviousLookupInfo.mURI = mIdentityInfo.mURI;

        const IdentityInfoList &infos = result->identities();
        if (infos.size() > 0) {
          mPreviousLookupInfo.mergeFrom(infos.front(), true);
        }

        ZS_LOG_DEBUG(log("identity lookup (of self) complete"))

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorErrorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           IdentityLookupResultPtr ignore, // will always be NULL
                                                                           message::MessageResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mIdentityLookupMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        setError(result->errorCode(), result->errorReason());

        ZS_LOG_DEBUG(log("identity lookup failed"))

        cancel();
        return true;
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceIdentitySession => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ServiceIdentitySession::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::log(const char *message) const
      {
        return String("ServiceIdentitySession [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("identity session id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("reported", toString(mLastReportedState), firstTime) +
               Helper::getDebugValue("error code", 0 != mLastError ? Stringize<typeof(mLastError)>(mLastError).string() : String(), firstTime) +
               Helper::getDebugValue("error reason", mLastErrorReason, firstTime) +
               Helper::getDebugValue("kill association", mKillAssociation ? String("true") : String(), firstTime) +
               (mIdentityInfo.hasData() ? mIdentityInfo.getDebugValueString() : String()) +
               IBootstrappedNetwork::toDebugString(mProviderBootstrappedNetwork) +
               IBootstrappedNetwork::toDebugString(mIdentityBootstrappedNetwork) +
               Helper::getDebugValue("active boostrapper", (mActiveBootstrappedNetwork ? (mIdentityBootstrappedNetwork == mActiveBootstrappedNetwork ? String("identity") : String("provider")) : String()), firstTime) +
               Helper::getDebugValue("grant session id", mGrantSession ? Stringize<PUID>(mGrantSession->forServices().getID()).string() : String(), firstTime) +
               Helper::getDebugValue("grant query id", mGrantQuery ? Stringize<PUID>(mGrantQuery->getID()).string() : String(), firstTime) +
               Helper::getDebugValue("grant wait id", mGrantWait ? Stringize<PUID>(mGrantWait->getID()).string() : String(), firstTime) +
               Helper::getDebugValue("identity access lockbox update monitor", mIdentityAccessLockboxUpdateMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("identity lookup update monitor", mIdentityLookupUpdateMonitor ? String("true") : String(), firstTime) +
               Helper::getDebugValue("identity sign monitor", mIdentitySignMonitor ? String("true") : String(), firstTime) +
               (mLockboxInfo.hasData() ? mLockboxInfo.getDebugValueString() : String()) +
               Helper::getDebugValue("browser window ready", mBrowserWindowReady ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser window visible", mBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser closed", mBrowserWindowClosed ? String("true") : String(), firstTime) +
               Helper::getDebugValue("need browser window visible", mNeedsBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("identity access start notification sent", mIdentityAccessStartNotificationSent ? String("true") : String(), firstTime) +
               Helper::getDebugValue("lockbox updated", mLockboxUpdated ? String("true") : String(), firstTime) +
               Helper::getDebugValue("identity lookup updated", mIdentityLookupUpdated ? String("true") : String(), firstTime) +
               (mPreviousLookupInfo.hasData() ? mPreviousLookupInfo.getDebugValueString() : String()) +
               Helper::getDebugValue("outer frame url", mOuterFrameURLUponReload, firstTime) +
               Helper::getDebugValue("signed element (verified)", mSignedIdentityBundleVerfiedEl ? String("true") : String(), firstTime) +
               Helper::getDebugValue("signed element (unchecked)", mSignedIdentityBundleUncheckedEl ? String("true") : String(), firstTime) +
               Helper::getDebugValue("signed element (old)", mSignedIdentityBundleOldEl ? String("true") : String(), firstTime) +
               Helper::getDebugValue("pending messages", mPendingMessagesToDeliver.size() > 1 ? Stringize<DocumentList::size_type>(mPendingMessagesToDeliver.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step - already shutdown"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!mDelegate) {
          ZS_LOG_DEBUG(log("step waiting for delegate to become attached"))
          setState(SessionState_WaitingAttachmentOfDelegate);
          return;
        }

        if (!stepBootstrapper()) return;
        if (!stepLoadBrowserWindow()) return;
        if (!stepIdentityAccessStartNotification()) return;
        if (!stepMakeBrowserWindowVisible()) return;
        if (!stepIdentityAccessCompleteNotification()) return;
        if (!stepLockboxAssociation()) return;
        if (!stepIdentityLookup()) return;
        if (!stepLockboxReady()) return;
        if (!stepLockboxUpdate()) return;
        if (!stepLookupUpdate()) return;
        if (!stepSign()) return;
        if (!stepAllRequestsCompleted()) return;
        if (!stepCloseBrowserWindow()) return;
        if (!stepClearWait()) return;

        if (mKillAssociation) {
          ZS_LOG_DEBUG(log("association is now killed") + getDebugValueString())
          setError(IHTTP::HTTPStatusCode_Gone, "assocation is now killed");
          cancel();
          return;
        }

        // signal the object is ready
        setState(SessionState_Ready);

        ZS_LOG_DEBUG(log("step complete") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepBootstrapper()
      {
        if (mActiveBootstrappedNetwork) {
          ZS_LOG_DEBUG(log("already have an active bootstrapper"))
          return true;
        }

        if (mKillAssociation) {
          ZS_LOG_WARNING(Detail, log("association is killed"))
          setError(IHTTP::HTTPStatusCode_Gone, "association is killed");
          cancel();
          return false;
        }

        setState(SessionState_Pending);

        if (mIdentityBootstrappedNetwork) {
          if (!mIdentityBootstrappedNetwork->forServices().isPreparationComplete()) {
            ZS_LOG_DEBUG(log("waiting for preparation of identity bootstrapper to complete"))
            return false;
          }

          WORD errorCode = 0;
          String reason;

          if (mIdentityBootstrappedNetwork->forServices().wasSuccessful(&errorCode, &reason)) {
            ZS_LOG_DEBUG(log("identity bootstrapper was successful thus using that as the active identity service"))
            mActiveBootstrappedNetwork = mIdentityBootstrappedNetwork;
            return true;
          }

          if (!mProviderBootstrappedNetwork) {
            ZS_LOG_ERROR(Detail, log("bootstrapped network failed for identity and there is no provider identity service specified") + ", error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", reason=" + reason)

            setError(errorCode, reason);
            cancel();
            return false;
          }
        }

        if (!mProviderBootstrappedNetwork) {
          ZS_LOG_ERROR(Detail, log("provider domain not specified for identity thus identity lookup cannot complete"))
          setError(IHTTP::HTTPStatusCode_BadGateway);
          cancel();
          return false;
        }

        if (!mProviderBootstrappedNetwork->forServices().isPreparationComplete()) {
          ZS_LOG_DEBUG(log("waiting for preparation of provider bootstrapper to complete"))
          return false;
        }

        WORD errorCode = 0;
        String reason;

        if (mProviderBootstrappedNetwork->forServices().wasSuccessful(&errorCode, &reason)) {
          ZS_LOG_DEBUG(log("provider bootstrapper was successful thus using that as the active identity service"))
          mActiveBootstrappedNetwork = mProviderBootstrappedNetwork;
          return true;
        }

        ZS_LOG_ERROR(Detail, log("bootstrapped network failed for provider") + ", error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", reason=" + reason)

        setError(errorCode, reason);
        cancel();
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepGrantCheck()
      {
        if (mBrowserWindowReady) {
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

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepLoadBrowserWindow()
      {
        if (mBrowserWindowReady) {
          ZS_LOG_DEBUG(log("browser window is ready"))
          return true;
        }

        if (mKillAssociation) {
          ZS_LOG_WARNING(Detail, log("association is killed"))
          setError(IHTTP::HTTPStatusCode_Gone, "association is killed");
          cancel();
          return false;
        }

        String url = getInnerBrowserWindowFrameURL();
        if (!url) {
          ZS_LOG_ERROR(Detail, log("bootstrapper did not return a valid inner window frame URL"))
          setError(IHTTP::HTTPStatusCode_NotFound);
          cancel();
          return false;
        }

        setState(SessionState_WaitingForBrowserWindowToBeLoaded);

        ZS_LOG_DEBUG(log("waiting for browser window to report it is loaded/ready"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepIdentityAccessStartNotification()
      {
        if (mIdentityAccessStartNotificationSent) {
          ZS_LOG_DEBUG(log("identity access start notification already sent"))
          return true;
        }

        if (mKillAssociation) {
          ZS_LOG_WARNING(Detail, log("association is killed"))
          setError(IHTTP::HTTPStatusCode_Gone, "association is killed");
          cancel();
          return false;
        }

        setState(SessionState_Pending);

        // make sure the provider domain is set to the active bootstrapper for the identity
        mIdentityInfo.mProvider = mActiveBootstrappedNetwork->forServices().getDomain();

        IdentityAccessStartNotifyPtr request = IdentityAccessStartNotify::create();
        request->domain(mActiveBootstrappedNetwork->forServices().getDomain());
        request->identityInfo(mIdentityInfo);

        request->browserVisibility(IdentityAccessStartNotify::BrowserVisibility_VisibleOnDemand);
        request->popup(false);

        request->outerFrameURL(mOuterFrameURLUponReload);

        sendInnerWindowMessage(request);

        mIdentityAccessStartNotificationSent = true;
        return true;
      }
      
      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepMakeBrowserWindowVisible()
      {
        if (mBrowserWindowVisible) {
          ZS_LOG_DEBUG(log("browser window is visible"))
          return true;
        }

        if (!mNeedsBrowserWindowVisible) {
          ZS_LOG_DEBUG(log("browser window was not requested to become visible"))
          return false;
        }

        if (mKillAssociation) {
          ZS_LOG_WARNING(Detail, log("association is killed"))
          setError(IHTTP::HTTPStatusCode_Gone, "association is killed");
          cancel();
          return false;
        }

        ZS_LOG_DEBUG(log("waiting for browser window to become visible"))
        setState(SessionState_WaitingForBrowserWindowToBeMadeVisible);
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepIdentityAccessCompleteNotification()
      {
        if (mIdentityInfo.mAccessToken.hasData()) {
          ZS_LOG_DEBUG(log("idenity access complete notification received"))
          return true;
        }

        if (mKillAssociation) {
          ZS_LOG_WARNING(Detail, log("association is killed"))
          setError(IHTTP::HTTPStatusCode_Gone, "association is killed");
          cancel();
          return false;
        }

        setState(SessionState_Pending);

        ZS_LOG_DEBUG(log("waiting for identity access complete notification"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepLockboxAssociation()
      {
        if (mAssociatedLockbox.lock()) {
          ZS_LOG_DEBUG(log("lockbox associated"))
          return true;
        }

        if (mKillAssociation) {
          ZS_LOG_DEBUG(log("do not need an association to the lockbox if association is being killed"))
          return true;
        }

        setState(SessionState_WaitingForAssociationToLockbox);
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepIdentityLookup()
      {
        if (mKillAssociation) {
          ZS_LOG_DEBUG(log("do not need to perform identity lookup when lockbox association is being killed"))
          return true;
        }

        if (mPreviousLookupInfo.mURI.hasData()) {
          ZS_LOG_DEBUG(log("identity lookup has already completed"))
          return true;
        }

        if (mIdentityLookupMonitor) {
          ZS_LOG_DEBUG(log("identity lookup already in progress (but not going to wait for it to complete to continue"))
          return true;
        }

        IdentityLookupRequestPtr request = IdentityLookupRequest::create();
        request->domain(mActiveBootstrappedNetwork->forServices().getDomain());

        IdentityLookupRequest::Provider provider;

        String domain;
        String id;

        IServiceIdentity::splitURI(mIdentityInfo.mURI, domain, id);

        provider.mBase = IServiceIdentity::joinURI(domain, NULL);
        char safeChar[2];
        safeChar[0] = getSafeSplitChar(id);;
        safeChar[1] = 0;

        provider.mSeparator = String(&(safeChar[0]));
        provider.mIdentities = id;

        IdentityLookupRequest::ProviderList providers;
        providers.push_back(provider);

        request->providers(providers);

        mIdentityLookupMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityLookupResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
        mActiveBootstrappedNetwork->forServices().sendServiceMessage("identity-lookup", "identity-lookup", request);

        setState(SessionState_Pending);
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepLockboxReady()
      {
        if (mKillAssociation) {
          ZS_LOG_DEBUG(log("do not need lockbox to be ready if association is being killed"))
          return true;
        }
        
        ServiceLockboxSessionPtr lockbox = mAssociatedLockbox.lock();
        if (!lockbox) {
          return stepLockboxAssociation();
        }

        WORD errorCode = 0;
        String reason;
        IServiceLockboxSession::SessionStates state = lockbox->forServiceIdentity().getState(&errorCode, &reason);

        switch (state) {
          case IServiceLockboxSession::SessionState_Pending:
          case IServiceLockboxSession::SessionState_PendingPeerFilesGeneration: {
            ZS_LOG_DEBUG(log("waiting for lockbox to ready"))
            return false;
          }
          case IServiceLockboxSession::SessionState_Ready: {
            ZS_LOG_DEBUG(log("lockbox is ready"))
            return true;
          }
          case IServiceLockboxSession::SessionState_Shutdown: {
            ZS_LOG_ERROR(Detail, log("lockbox shutdown") + ", error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", reason=" + reason)

            setError(errorCode, reason);
            cancel();
            return false;
          }
        }

        ZS_LOG_DEBUG(log("unknown lockbox state") + getDebugValueString())

        ZS_THROW_BAD_STATE("unknown lockbox state")
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepLockboxUpdate()
      {
        if (mLockboxUpdated) {
          ZS_LOG_DEBUG(log("lockbox update already complete"))
          return true;
        }

        if (mIdentityAccessLockboxUpdateMonitor) {
          ZS_LOG_DEBUG(log("lockbox update already in progress (but does not prevent other events from continuing"))
          return true;
        }

        if (mKillAssociation) {
          setState(SessionState_Pending);

          IdentityAccessLockboxUpdateRequestPtr request = IdentityAccessLockboxUpdateRequest::create();
          request->domain(mActiveBootstrappedNetwork->forServices().getDomain());
          request->identityInfo(mIdentityInfo);

          ZS_LOG_DEBUG(log("clearing lockbox information (but not preventing other requests from continuing)"))

          mIdentityAccessLockboxUpdateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityAccessLockboxUpdateResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
          mActiveBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-access-lockbox-update", request);

          return true;
        }

        ServiceLockboxSessionPtr lockbox = mAssociatedLockbox.lock();
        if (!lockbox) {
          return stepLockboxAssociation();
        }

        setState(SessionState_Pending);

        LockboxInfo lockboxInfo = lockbox->forServiceIdentity().getLockboxInfo();
        if ((lockboxInfo.mDomain == mLockboxInfo.mDomain) &&
            (lockboxInfo.mKeyIdentityHalf == mLockboxInfo.mKeyIdentityHalf)) {
          ZS_LOG_DEBUG(log("lockbox info already updated correctly"))
          mLockboxUpdated = true;
          return true;
        }

        mLockboxInfo.mergeFrom(lockboxInfo, true);

        IdentityAccessLockboxUpdateRequestPtr request = IdentityAccessLockboxUpdateRequest::create();
        request->domain(mActiveBootstrappedNetwork->forServices().getDomain());
        request->identityInfo(mIdentityInfo);
        request->lockboxInfo(lockboxInfo);

        ZS_LOG_DEBUG(log("updating lockbox information (but not preventing other requests from continuing)"))

        mIdentityAccessLockboxUpdateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityAccessLockboxUpdateResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
        sendInnerWindowMessage(request);

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepLookupUpdate()
      {
        if (mIdentityLookupUpdated) {
          ZS_LOG_DEBUG(log("lookup already updated"))
          return true;
        }

        if (mIdentityLookupUpdateMonitor) {
          ZS_LOG_DEBUG(log("lookup update already in progress (but does not prevent other events from completing)"))
          return true;
        }

        if (mKillAssociation) {
          ZS_LOG_DEBUG(log("clearing identity lookup information (but not preventing other requests from continuing)"))

          mIdentityInfo.mStableID.clear();
          mIdentityInfo.mPeerFilePublic.reset();
          mIdentityInfo.mPriority = 0;
          mIdentityInfo.mWeight = 0;

          IdentityLookupUpdateRequestPtr request = IdentityLookupUpdateRequest::create();
          request->domain(mActiveBootstrappedNetwork->forServices().getDomain());
          request->identityInfo(mIdentityInfo);

          mIdentityLookupUpdateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityLookupUpdateResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
          mActiveBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-lookup-update", request);

          return true;
        }

        if (mPreviousLookupInfo.mURI.isEmpty()) {
          ZS_LOG_DEBUG(log("waiting for identity lookup to complete (but will not prevent other events from completing)"))
          return true;
        }

        ServiceLockboxSessionPtr lockbox = mAssociatedLockbox.lock();
        if (!lockbox) {
          return stepLockboxAssociation();
        }

        setState(SessionState_Pending);

        IPeerFilesPtr peerFiles;
        IdentityInfo identityInfo = lockbox->forServiceIdentity().getIdentityInfoForIdentity(mThisWeak.lock(), &peerFiles);
        mIdentityInfo.mergeFrom(identityInfo, true);

        if ((identityInfo.mStableID == mPreviousLookupInfo.mStableID) &&
            (isSame(identityInfo.mPeerFilePublic, mPreviousLookupInfo.mPeerFilePublic)) &&
            (identityInfo.mPriority == mPreviousLookupInfo.mPriority) &&
            (identityInfo.mWeight == mPreviousLookupInfo.mWeight)) {
          ZS_LOG_DEBUG(log("identity information already up-to-date"))
          mIdentityLookupUpdated = true;
          return true;
        }

        ZS_LOG_DEBUG(log("updating identity lookup information (but not preventing other requests from continuing)"))

        IdentityLookupUpdateRequestPtr request = IdentityLookupUpdateRequest::create();
        request->domain(mActiveBootstrappedNetwork->forServices().getDomain());
        request->peerFiles(peerFiles);
        request->lockboxInfo(mLockboxInfo);
        request->identityInfo(mIdentityInfo);

        mIdentityLookupUpdateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityLookupUpdateResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
        mActiveBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-lookup-update", request);

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepSign()
      {
        if (mKillAssociation) {
          ZS_LOG_DEBUG(log("no need to sign identity if the association is being killed"))
          return true;
        }

        if (mSignedIdentityBundleVerfiedEl) {
          ZS_LOG_DEBUG(log("identity bundle already verified"))
          return true;
        }

        if (mIdentitySignMonitor) {
          ZS_LOG_DEBUG(log("waiting for sign monitor to complete (but not preventing other requests from continuing)"))
          return true;
        }

        setState(SessionState_Pending);

        ServiceLockboxSessionPtr lockbox = mAssociatedLockbox.lock();
        if (!lockbox) {
          return stepLockboxAssociation();
        }

        if (!mSignedIdentityBundleUncheckedEl) {
          // attempt to get a signed identity bundle from the lockbox
          mSignedIdentityBundleUncheckedEl = lockbox->forServiceIdentity().getSignatureForIdentity(mThisWeak.lock());
        }

        if (!mSignedIdentityBundleUncheckedEl) {
          // obtained a signed bundle since one is not found
          IdentitySignRequestPtr request = IdentitySignRequest::create();
          request->domain(mActiveBootstrappedNetwork->forServices().getDomain());
          request->identityInfo(mIdentityInfo);

          mIdentityLookupUpdateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentitySignResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
          mActiveBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-sign", request);

          return true;
        }

        // scope: validate the identity bundle
        {
          // figure out of the signed identity bundle still has enough life time left
          try {
            ElementPtr signatureEl;
            ElementPtr signedEl = IHelper::getSignatureInfo(mSignedIdentityBundleUncheckedEl, &signatureEl);

            if (!mActiveBootstrappedNetwork->forServices().isValidSignature(signedEl)) {
              ZS_LOG_WARNING(Detail, log("bootstrapper reported signature as invalid"))
              goto invalid_certificate;
            }

            Time created = IMessageHelper::stringToTime(signedEl->findFirstChildElementChecked("created")->getTextDecoded());
            Time expires = IMessageHelper::stringToTime(signedEl->findFirstChildElementChecked("expires")->getTextDecoded());
            Time now = zsLib::now();

            if ((Time() == created) ||
                (Time() == expires) ||
                (created > expires) ||
                (now + Hours(OPENPEER_STACK_SERVIC_IDENTITY_SIGN_CREATE_SHOULD_NOT_BE_BEFORE_NOW_IN_HOURS) < created)) {
              ZS_LOG_WARNING(Detail, log("certificate has invalid date range") + ", created=" + IMessageHelper::timeToString(created) + ", expires=" + IMessageHelper::timeToString(expires) + ", now=" + IMessageHelper::timeToString(now))
              goto invalid_certificate;
            }

            if (now > expires) {
              ZS_LOG_WARNING(Detail, log("certificate has expired"))
              goto invalid_certificate;
            }

            long durationFromCreated = (now - created).total_seconds();
            long durationToTotal = (expires - created).total_seconds();

            if ((durationFromCreated * 100) / durationToTotal > (100-OPENPEER_STACK_SERVICE_IDENTITY_MAX_PERCENTAGE_TIME_REMAINING_BEFORE_RESIGN_IDENTITY_REQUIRED)) {
              ZS_LOG_WARNING(Detail, log("resign identity required since certificate will expire soon") + ", created=" + IMessageHelper::timeToString(created) + ", expires=" + IMessageHelper::timeToString(expires) + ", now=" + IMessageHelper::timeToString(now))
              goto invalid_certificate;
            }

            mSignedIdentityBundleVerfiedEl = mSignedIdentityBundleUncheckedEl;
            goto valid_certificate;
          } catch(CheckFailed &) {
            ZS_LOG_DEBUG(log("failed to obtain information from signature"))
            goto invalid_certificate;
          }

        invalid_certificate:
          if (mSignedIdentityBundleOldEl) {
            // already have an old bundle thus there is no point to trying again to download a new bundle
            ZS_LOG_ERROR(Detail, log("download new certificate but the signature returned was not valid"))
            setError(IHTTP::HTTPStatusCode_ServiceUnavailable, "downloaded signed identity is not valid");
            cancel();
            return false;
          }

          ZS_LOG_DEBUG(log("signed identity bundle did not pass validation check (will attempt to sign a new identity bundle)"))

          mSignedIdentityBundleOldEl = mSignedIdentityBundleUncheckedEl;
          mSignedIdentityBundleUncheckedEl.reset();

          // try again shortly...
          IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
          return true;

        valid_certificate:
          mSignedIdentityBundleVerfiedEl = mSignedIdentityBundleUncheckedEl;
          ZS_LOG_DEBUG(log("identity signature is valid"))
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepAllRequestsCompleted()
      {
        if (!mLockboxUpdated) {
          ZS_LOG_DEBUG(log("waiting for lockbox update to complete"))
          return false;
        }
        if (!mIdentityLookupUpdated) {
          ZS_LOG_DEBUG(log("waiting for identity lookup update to complete"))
          return false;
        }

        if (!mKillAssociation) {
          if (!mSignedIdentityBundleVerfiedEl) {
            ZS_LOG_DEBUG(log("waiting to validate a signed identity bundle"))
            return false;
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepCloseBrowserWindow()
      {
        if (mBrowserWindowClosed) {
          ZS_LOG_DEBUG(log("browser window is closed"))
          return true;
        }

        ZS_LOG_DEBUG(log("waiting for browser window to close"))

        setState(SessionState_WaitingForBrowserWindowToClose);

        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepClearWait()
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
      void ServiceIdentitySession::setState(SessionStates state)
      {
        if (state != mCurrentState) {
          ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState))
          mCurrentState = state;

          notifyLockboxStateChanged();
        }

        if (mLastReportedState != mCurrentState) {
          ServiceIdentitySessionPtr pThis = mThisWeak.lock();
          if ((pThis) &&
              (mDelegate)) {
            try {
              ZS_LOG_DEBUG(log("attempting to report state to delegate") + getDebugValueString())
              mDelegate->onServiceIdentitySessionStateChanged(pThis, mCurrentState);
              mLastReportedState = mCurrentState;
            } catch (IServiceIdentitySessionDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());
        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set thus ignoring new error") + ", new error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", new reason=" + reason + getDebugValueString())
          return;
        }

        mLastError = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::notifyLockboxStateChanged()
      {
        ServiceLockboxSessionPtr lockbox = mAssociatedLockbox.lock();
        if (!lockbox) return;

        ZS_LOG_DEBUG(log("notifying lockbox of state change"))
        lockbox->forServiceIdentity().notifyStateChanged();
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::sendInnerWindowMessage(MessagePtr message)
      {
        DocumentPtr doc = message->encode();
        mPendingMessagesToDeliver.push_back(doc);

        if (1 != mPendingMessagesToDeliver.size()) {
          ZS_LOG_DEBUG(log("already had previous messages to deliver, no need to send another notification"))
          return;
        }

        ServiceIdentitySessionPtr pThis = mThisWeak.lock();

        if ((pThis) &&
            (mDelegate)) {
          try {
            ZS_LOG_DEBUG(log("attempting to notify of message to browser window needing to be delivered"))
            mDelegate->onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(pThis);
          } catch(IServiceIdentitySessionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
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
    #pragma mark IServiceIdentitySession
    #pragma mark

    //-------------------------------------------------------------------------
    bool IServiceIdentity::isValid(const char *identityURI)
    {
      if (!identityURI) {
        ZS_LOG_WARNING(Detail, String("identity URI is not valid as it is NULL, uri=(null)"))
        return false;
      }

      zsLib::RegEx e("^identity:\\/\\/([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}\\/.+$");
      if (!e.hasMatch(identityURI)) {
        zsLib::RegEx e2("^identity:[a-zA-Z0-9\\-_]{0,61}:.+$");
        if (!e2.hasMatch(identityURI)) {
          ZS_LOG_WARNING(Detail, String("ServiceIdentity [] identity URI is not valid, uri=") + identityURI)
          return false;
        }
      }
      return true;
    }

    //-------------------------------------------------------------------------
    bool IServiceIdentity::isValidBase(const char *identityBase)
    {
      if (!identityBase) {
        ZS_LOG_WARNING(Detail, String("identity base is not valid as it is NULL, uri=(null)"))
        return false;
      }

      zsLib::RegEx e("^identity:\\/\\/([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}\\/$");
      if (!e.hasMatch(identityBase)) {
        zsLib::RegEx e2("^identity:[a-zA-Z0-9\\-_]{0,61}:$");
        if (!e2.hasMatch(identityBase)) {
          ZS_LOG_WARNING(Detail, String("ServiceIdentity [] identity base URI is not valid, uri=") + identityBase)
          return false;
        }
      }
      return true;
    }

    //-------------------------------------------------------------------------
    bool IServiceIdentity::isLegacy(const char *identityURI)
    {
      if (!identityURI) {
        ZS_LOG_WARNING(Detail, String("identity URI is not valid as it is NULL, uri=(null)"))
        return false;
      }

      zsLib::RegEx e("^identity:[a-zA-Z0-9\\-_]{0,61}:.*$");
      if (!e.hasMatch(identityURI)) {
        return false;
      }
      return true;
    }

    //-------------------------------------------------------------------------
    bool IServiceIdentity::splitURI(
                                    const char *inIdentityURI,
                                    String &outDomainOrLegacyType,
                                    String &outIdentifier,
                                    bool *outIsLegacy
                                    )
    {
      String identityURI(inIdentityURI ? inIdentityURI : "");

      identityURI.trim();
      if (outIsLegacy) *outIsLegacy = false;

      // scope: check legacy identity
      {
        zsLib::RegEx e("^identity:[a-zA-Z0-9\\-_]{0,61}:.*$");
        if (e.hasMatch(identityURI)) {

          // find second colon
          size_t startPos = strlen("identity:");
          size_t colonPos = identityURI.find(':', identityURI.find(':')+1);

          ZS_THROW_BAD_STATE_IF(colonPos == String::npos)

          outDomainOrLegacyType = identityURI.substr(startPos, colonPos - startPos);
          outIdentifier = identityURI.substr(colonPos + 1);

          if (outIsLegacy) *outIsLegacy = true;
          return true;
        }
      }

      zsLib::RegEx e("^identity:\\/\\/([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}\\/.*$");
      if (!e.hasMatch(identityURI)) {
        ZS_LOG_WARNING(Detail, String("ServiceIdentity [] identity URI is not valid, uri=") + identityURI)
        return false;
      }

      size_t startPos = strlen("identity://");
      size_t slashPos = identityURI.find('/', startPos);

      ZS_THROW_BAD_STATE_IF(slashPos == String::npos)

      outDomainOrLegacyType = identityURI.substr(startPos, slashPos - startPos);
      outIdentifier = identityURI.substr(slashPos + 1);

      outDomainOrLegacyType.toLower();
      return true;
    }

    //-------------------------------------------------------------------------
    String IServiceIdentity::joinURI(
                                     const char *inDomainOrType,
                                     const char *inIdentifier
                                     )
    {
      String domainOrType(inDomainOrType);
      String identifier(inIdentifier);

      domainOrType.trim();
      identifier.trim();

      if (String::npos == domainOrType.find('.')) {
        // this is legacy

        String result = "identity:" + domainOrType + ":" + identifier;
        if (identifier.hasData()) {
          if (!isValid(result)) {
            ZS_LOG_WARNING(Detail, "IServiceIdentity [] invalid identity URI created after join, URI=" + result)
            return String();
          }
        } else {
          if (!isValidBase(result)) {
            ZS_LOG_WARNING(Detail, "IServiceIdentity [] invalid identity URI created after join, URI=" + result)
            return String();
          }
        }
        return result;
      }

      domainOrType.toLower();

      String result = "identity://" + domainOrType + "/" + identifier;
      if (identifier.hasData()) {
        if (!isValid(result)) {
          ZS_LOG_WARNING(Detail, "IServiceIdentity [] invalid identity URI created after join, URI=" + result)
          return String();
        }
      } else {
        if (!isValidBase(result)) {
          ZS_LOG_WARNING(Detail, "IServiceIdentity [] invalid identity URI created after join, URI=" + result)
          return String();
        }
      }
      return result;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentitySession
    #pragma mark

    //-------------------------------------------------------------------------
    String IServiceIdentitySession::toDebugString(IServiceIdentitySessionPtr session, bool includeCommaPrefix)
    {
      return internal::ServiceIdentitySession::toDebugString(session, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    const char *IServiceIdentitySession::toString(SessionStates state)
    {
      switch (state)
      {
        case SessionState_Pending:                                  return "Pending";
        case SessionState_WaitingAttachmentOfDelegate:              return "Waiting Attachment of Delegate";
        case SessionState_WaitingForBrowserWindowToBeLoaded:        return "Waiting for Browser Window to be Loaded";
        case SessionState_WaitingForBrowserWindowToBeMadeVisible:   return "Waiting for Browser Window to be Made Visible";
        case SessionState_WaitingForBrowserWindowToClose:           return "Waiting for Browser Window to Close";
        case SessionState_WaitingForAssociationToLockbox:           return "Waiting for Association to Lockbox";
        case SessionState_Ready:                                    return "Ready";
        case SessionState_Shutdown:                                 return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    IServiceIdentitySessionPtr IServiceIdentitySession::loginWithIdentity(
                                                                          IServiceIdentitySessionDelegatePtr delegate,
                                                                          IServiceIdentityPtr provider,
                                                                          IServiceNamespaceGrantSessionPtr grantSession,
                                                                          IServiceLockboxSessionPtr existingLockbox,
                                                                          const char *outerFrameURLUponReload,
                                                                          const char *identityURI
                                                                          )
    {
      return internal::IServiceIdentitySessionFactory::singleton().loginWithIdentity(delegate, provider, grantSession, existingLockbox, outerFrameURLUponReload, identityURI);
    }

    //-------------------------------------------------------------------------
    IServiceIdentitySessionPtr IServiceIdentitySession::loginWithIdentityProvider(
                                                                                  IServiceIdentitySessionDelegatePtr delegate,
                                                                                  IServiceIdentityPtr provider,
                                                                                  IServiceNamespaceGrantSessionPtr grantSession,
                                                                                  IServiceLockboxSessionPtr existingLockbox,
                                                                                  const char *outerFrameURLUponReload,
                                                                                  const char *legacyIdentityBaseURI
                                                                                  )
    {
      return internal::IServiceIdentitySessionFactory::singleton().loginWithIdentityProvider(delegate, provider, grantSession, existingLockbox, outerFrameURLUponReload, legacyIdentityBaseURI);
    }

    //-------------------------------------------------------------------------
    IServiceIdentitySessionPtr IServiceIdentitySession::loginWithIdentityBundle(
                                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                                IServiceIdentityPtr provider,
                                                                                IServiceNamespaceGrantSessionPtr grantSession,
                                                                                IServiceLockboxSessionPtr existingLockbox,
                                                                                const char *outerFrameURLUponReload,
                                                                                ElementPtr signedIdentityBundle
                                                                                )
    {
      return internal::IServiceIdentitySessionFactory::singleton().loginWithIdentityBundle(delegate, provider, grantSession, existingLockbox, outerFrameURLUponReload, signedIdentityBundle);
    }
  }
}
