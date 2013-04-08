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

#include <hookflash/stack/internal/stack_ServiceIdentitySession.h>
#include <hookflash/stack/internal/stack_ServiceLockboxSession.h>
#include <hookflash/stack/message/identity/IdentityLoginStartRequest.h>
#include <hookflash/stack/message/identity/IdentityLoginNotify.h>
#include <hookflash/stack/message/identity/IdentityLoginBrowserWindowControlNotify.h>
#include <hookflash/stack/message/identity/IdentityLoginCompleteRequest.h>
#include <hookflash/stack/message/identity/IdentityAssociateRequest.h>
#include <hookflash/stack/message/identity/IdentitySignRequest.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/IPeer.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/internal/stack_Stack.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>

#include <boost/regex.hpp>

#define HOOKFLASH_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS (60*2)
#define HOOKFLASH_STACK_SERVICE_IDENTITY_MAX_PERCENTAGE_TIME_REMAINING_BEFORE_RESIGN_IDENTITY_REQUIRED (20) // at 20% of the remaining on the certificate before expiry, resign

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      using zsLib::Stringize;

      using message::identity::IdentityLoginStartRequest;
      using message::identity::IdentityLoginStartRequestPtr;
      using message::identity::IdentityLoginNotify;
      using message::identity::IdentityLoginNotifyPtr;
      using message::identity::IdentityLoginBrowserWindowControlNotify;
      using message::identity::IdentityLoginBrowserWindowControlNotifyPtr;
      using message::identity::IdentityLoginCompleteRequest;
      using message::identity::IdentityLoginCompleteRequestPtr;
      using message::identity::IdentityAssociateRequest;
      using message::identity::IdentityAssociateRequestPtr;
      using message::identity::IdentitySignRequest;
      using message::identity::IdentitySignRequestPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceIdentitySessionForServiceLockbox
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr IServiceIdentitySessionForServiceLockbox::relogin(
                                                                                      BootstrappedNetworkPtr network,
                                                                                      const char *identityReloginAccessKey
                                                                                      )
      {
        return IServiceIdentitySessionFactory::singleton().relogin(IServiceIdentitySessionDelegatePtr(), NULL, network, identityReloginAccessKey);
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
                                                     BootstrappedNetworkPtr network,
                                                     IServiceIdentitySessionDelegatePtr delegate,
                                                     const char *redirectAfterLoginCompleteURL
                                                     ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(delegate ? IServiceIdentitySessionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate) : IServiceIdentitySessionDelegatePtr()),
        mKillAssociation(false),
        mBootstrappedNetwork(network),
        mCurrentState(SessionState_Pending),
        mLastReportedState(SessionState_Pending),
        mLastError(0),
        mLoadedLoginURL(false),
        mBrowserWindowVisible(false),
        mBrowserRedirectionComplete(false),
        mClientLoginSecret(IHelper::randomString(32)),
        mClientToken(IHelper::randomString(32)),
        mNeedsBrowserWindowVisible(false),
        mNotificationSentToInnerBrowserWindow(false),
        mRedirectAfterLoginCompleteURL(redirectAfterLoginCompleteURL ? String(redirectAfterLoginCompleteURL) : String()),
        mSignedIdentityBundleValidityChecked(false),
        mDownloadedSignedIdentityBundle(false)
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
        IBootstrappedNetworkForServices::prepare(mBootstrappedNetwork->forServices().getDomain(), mThisWeak.lock());
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
                                                                          const char *redirectAfterLoginCompleteURL,
                                                                          const char *identityURI,
                                                                          IServiceIdentityPtr provider
                                                                          )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!redirectAfterLoginCompleteURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!identityURI)

        if (!provider) {
          if (IServiceIdentity::isLegacy(identityURI)) {
            ZS_THROW_INVALID_ARGUMENT_IF(!provider)
          }
        }

        if ((!IServiceIdentity::isValid(identityURI)) ||
            (!IServiceIdentity::isValidBase(identityURI))) {
          ZS_LOG_ERROR(Detail, String("identity URI specified is not valid, uri=") + identityURI)
          return ServiceIdentitySessionPtr();
        }

        BootstrappedNetworkPtr network = BootstrappedNetwork::convert(provider);
        if (!network) {
          String domain;
          String identifier;
          IServiceIdentity::splitURI(identityURI, domain, identifier);
          network = IBootstrappedNetworkForServices::prepare(domain);
        }

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(IStackForInternal::queueStack(), BootstrappedNetwork::convert(provider), delegate, redirectAfterLoginCompleteURL));
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
      ServiceIdentitySessionPtr ServiceIdentitySession::loginWithIdentityTBD(
                                                                             IServiceIdentitySessionDelegatePtr delegate,
                                                                             const char *redirectAfterLoginCompleteURL,
                                                                             IServiceIdentityPtr provider,
                                                                             const char *legacyIdentityBaseURI
                                                                             )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!redirectAfterLoginCompleteURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!provider)

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(IStackForInternal::queueStack(), BootstrappedNetwork::convert(provider), delegate, redirectAfterLoginCompleteURL));
        pThis->mThisWeak = pThis;
        pThis->mIdentityInfo.mBase = (legacyIdentityBaseURI ? String(legacyIdentityBaseURI) : String());
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr ServiceIdentitySession::loginWithIdentityBundle(
                                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                                const char *redirectAfterLoginCompleteURL,
                                                                                ElementPtr signedIdentityBundleEl
                                                                                )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!redirectAfterLoginCompleteURL)
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

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(IStackForInternal::queueStack(), network, delegate, redirectAfterLoginCompleteURL));
        pThis->mThisWeak = pThis;
        pThis->mIdentityInfo.mURI = identityURI;
        pThis->mSignedIdentityBundleEl = signedIdentityBundleEl->clone()->toElement();
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ServiceIdentitySessionPtr ServiceIdentitySession::relogin(
                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                const char *redirectAfterLoginCompleteURL,
                                                                IServiceIdentityPtr provider,
                                                                const char *identityReloginAccessKey
                                                                )
      {
        if (delegate) {
          ZS_THROW_INVALID_ARGUMENT_IF(!redirectAfterLoginCompleteURL)
        }
        ZS_THROW_INVALID_ARGUMENT_IF(!provider)
        ZS_THROW_INVALID_ARGUMENT_IF(!identityReloginAccessKey)

        ServiceIdentitySessionPtr pThis(new ServiceIdentitySession(IStackForInternal::queueStack(), BootstrappedNetwork::convert(provider), delegate, redirectAfterLoginCompleteURL));
        pThis->mThisWeak = pThis;
        pThis->mIdentityInfo.mReloginAccessKey = (identityReloginAccessKey ? String(identityReloginAccessKey) : String());
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IServiceIdentityPtr ServiceIdentitySession::getService() const
      {
        AutoRecursiveLock lock(getLock());
        return mBootstrappedNetwork;
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
      bool ServiceIdentitySession::isAttached() const
      {
        AutoRecursiveLock lock(getLock());
        return ((bool)mDelegate);
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::attach(
                                          const char *redirectAfterLoginCompleteURL,
                                          IServiceIdentitySessionDelegatePtr delegate
                                          )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!redirectAfterLoginCompleteURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        AutoRecursiveLock lock(getLock());

        mDelegate = IServiceIdentitySessionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate);
        mRedirectAfterLoginCompleteURL = (redirectAfterLoginCompleteURL ? String(redirectAfterLoginCompleteURL) : String());

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
        return mBootstrappedNetwork->forServices().getDomain();
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::getIdentityReloginAccessKey() const
      {
        AutoRecursiveLock lock(getLock());
        return mIdentityInfo.mReloginAccessKey;
      }

      //-----------------------------------------------------------------------
      ElementPtr ServiceIdentitySession::getSignedIdentityBundle() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mSignedIdentityBundleEl) return ElementPtr();
        return mSignedIdentityBundleEl;
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::getIdentityLoginURL() const
      {
        AutoRecursiveLock lock(getLock());
        return mIdentityLoginURL;
      }

      //-----------------------------------------------------------------------
      Time ServiceIdentitySession::getLoginExpires() const
      {
        AutoRecursiveLock lock(getLock());
        return mIdentityLoginExpires;
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::notifyBrowserWindowVisible()
      {
        AutoRecursiveLock lock(getLock());
        mBrowserWindowVisible = true;
        step();
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::notifyLoginCompleteBrowserWindowRedirection()
      {
        AutoRecursiveLock lock(getLock());
        mBrowserRedirectionComplete = true;
        step();
      }

      //-----------------------------------------------------------------------
      DocumentPtr ServiceIdentitySession::getNextMessageForInnerBrowerWindowFrame()
      {
        AutoRecursiveLock lock(getLock());
        if (mPendingMessagesToDeliver.size() < 1) return DocumentPtr();

        DocumentPtr result = mPendingMessagesToDeliver.front();
        mPendingMessagesToDeliver.pop_front();

        if (mDelegate) {
          if (mPendingMessagesToDeliver.size() > 0) {
            try {
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

        IdentityLoginBrowserWindowControlNotifyPtr requestBrowserControl = IdentityLoginBrowserWindowControlNotify::convert(message);
        if (requestBrowserControl) {
          if (requestBrowserControl->hasAttribute(IdentityLoginBrowserWindowControlNotify::AttributeType_Visible)) {
            if (requestBrowserControl->ready()) {
              ZS_LOG_DEBUG(log("notifying browser window ready"))
              mLoadedLoginURL = true;
            }
            if (requestBrowserControl->visible()) {
              ZS_LOG_DEBUG(log("notifying browser window needs to be made visible"))
              mNeedsBrowserWindowVisible = true;
            }
            IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
            return;
          }

          ZS_LOG_WARNING(Debug, log("browser window control request was not understood"))
          MessageResultPtr result = MessageResult::create(requestBrowserControl, IHTTP::HTTPStatusCode_NotImplemented);
          sendInnerWindowMessage(result);
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

        mPendingMessagesToDeliver.clear();

        if (mLoginStartMonitor) {
          mLoginStartMonitor->cancel();
          mLoginStartMonitor.reset();
        }

        if (mLoginCompleteMonitor) {
          mLoginCompleteMonitor->cancel();
          mLoginCompleteMonitor.reset();
        }

        if (mAssociateMonitor) {
          mAssociateMonitor->cancel();
          mAssociateMonitor.reset();
        }

        if (mSignMonitor) {
          mSignMonitor->cancel();
          mSignMonitor.reset();
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
      void ServiceIdentitySession::associate(ServiceLockboxSessionPtr peerContact)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("associate called"))
        mAssociatedPeerContact = peerContact;
        IServiceIdentitySessionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::killAssociation(ServiceLockboxSessionPtr peerContact)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("associate called"))
        mAssociatedPeerContact.reset();
        mKillAssociation = true;
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
      SecureByteBlockPtr ServiceIdentitySession::getPrivatePeerFileSecret() const
      {
        AutoRecursiveLock lock(getLock());
        return mPrivatePeerFileSecret;
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
      #pragma mark ServiceIdentitySession => IMessageMonitorResultDelegate<IdentityLoginStartResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorResultReceived(
                                                                      IMessageMonitorPtr monitor,
                                                                      IdentityLoginStartResultPtr result
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLoginStartMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mLoginStartMonitor->cancel();
        mLoginStartMonitor.reset();

        mServerToken = result->serverToken();

        if ("browser-window" != result->mode().mType) {
          ZS_LOG_WARNING(Detail, log("mode is not understood") + ", mode=" + result->mode().mType)
          setError(IHTTP::HTTPStatusCode_NotImplemented, "Login mode type from server is not supported");
          cancel();
          return true;
        }

        mIdentityLoginURL = result->mode().mIdentityLoginURL;
        mIdentityLoginCompleteURL = result->identityLoginCompleteURL();
        mIdentityLoginExpires = result->expires();

        if ((mIdentityLoginURL.isEmpty()) ||
            (mIdentityLoginCompleteURL.isEmpty())) {
          ZS_LOG_ERROR(Detail, log("missing login or login complete URL") + ", login URL=" + mIdentityLoginURL + ", complete URL=" + mIdentityLoginCompleteURL)
          setError(IHTTP::HTTPStatusCode_PreconditionFailed, "Missing critical data in result from server");
          cancel();
          return true;
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorErrorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           IdentityLoginStartResultPtr ignore, // will always be NULL
                                                                           message::MessageResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLoginStartMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        setError(result->errorCode(), result->errorReason());

        ZS_LOG_DEBUG(log("identity login start failed"))

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
                                                                      IdentityLoginCompleteResultPtr result
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLoginCompleteMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mLoginCompleteMonitor->cancel();
        mLoginCompleteMonitor.reset();

        mIdentityInfo.mergeFrom(result->identityInfo());

        if (mIdentityInfo.mURIEncrypted.hasData())
        {
          // mURI
          // key=hmac(<client-login-secret>, "identity:" + <client-token> + ":" + <server-token>)
          // iv=hash(<client-token> + ":" + <server-token>)
          SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKey(mClientLoginSecret), "identity:" + mClientToken + ":" + mServerToken);
          SecureByteBlockPtr iv = IHelper::hash(mClientToken + ":" + mServerToken, IHelper::HashAlgorthm_MD5);

          String uri = IHelper::convertToString(*IHelper::decrypt(*key, *iv, *IHelper::convertFromBase64(mIdentityInfo.mURIEncrypted)));

          if (mIdentityInfo.mHash.hasData()) {
            // validate against the hash
            if (0 != IHelper::compare(*IHelper::convertFromHex(mIdentityInfo.mHash), *IHelper::hash(uri))) {
              ZS_LOG_ERROR(Detail, log("URI decrypted did not match hash") + ", URI=" + uri + ", expecting=" + mIdentityInfo.mHash + ", calculated=" + IHelper::convertToHex(*IHelper::hash(uri)))
              setError(IHTTP::HTTPStatusCode_Forbidden, "Identity URI returned from server does not match computed hash");
              cancel();
              return true;
            }
          }

          if (!IServiceIdentity::isValid(uri)) {
            ZS_LOG_ERROR(Detail, log("encrypted identity URI returned is not valid") + ", URI=" + uri)
            setError(IHTTP::HTTPStatusCode_NotAcceptable, "Decrypted identity URI returned from server is not valid");
            cancel();
            return true;
          }

          mIdentityInfo.mURI = uri;
        }

        if (mIdentityInfo.mReloginAccessKeyEncrypted.hasData()) {
          // mReloginAccessKey
          // key = hmac(<client-login-secret>, "relogin-access-key:" + <client-token> + ":" + <server-token>)
          // iv=hash(<client-token> + ":" + <server-token>)
          SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKey(mClientLoginSecret), "relogin-access-key:" + mClientToken + ":" + mServerToken);
          SecureByteBlockPtr iv = IHelper::hash(mClientToken + ":" + mServerToken, IHelper::HashAlgorthm_MD5);

          mIdentityInfo.mReloginAccessKey = IHelper::convertToString(*IHelper::decrypt(*key, *iv, *IHelper::convertFromBase64(mIdentityInfo.mReloginAccessKeyEncrypted)));
        }

        // try to decrypt the identity secret
        if ((mIdentityInfo.mSecretEncrypted.hasData()) &&
            (mIdentityInfo.mSecretSalt.hasData()) &&
            (mIdentityInfo.mSecretDecryptionKeyEncrypted.hasData()))
        {
          String secretDecryptionKey;

          {
            // secretDecryptionKey
            // key=hmac(<client-login-secret>, "identity-secret-decryption-key:" + <client-token> + ":" + <server-token>)
            // iv=hash(<client-token> + ":" + <server-token>)

            SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKey(mClientLoginSecret), "identity-secret-decryption-key:" + mClientToken + ":" + mServerToken);
            SecureByteBlockPtr iv = IHelper::hash(mClientToken + ":" + mServerToken, IHelper::HashAlgorthm_MD5);

            secretDecryptionKey = IHelper::convertToString(*IHelper::decrypt(*key, *iv, *IHelper::convertFromBase64(mIdentityInfo.mSecretDecryptionKeyEncrypted)));
          }

          {
            // mSecret
            // key = hmac(<identity-secret-decryption-key>, "identity-secret:" + base64(<identity-secret-salt>))
            // iv=hash(base64(<identity-secret-salt>))
            SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKey(secretDecryptionKey), "identity-secret:" + mIdentityInfo.mSecretSalt);
            SecureByteBlockPtr iv = IHelper::hash("", IHelper::HashAlgorthm_MD5);

            mIdentityInfo.mSecret = IHelper::convertToString(*IHelper::decrypt(*key, *iv, *IHelper::convertFromBase64(mIdentityInfo.mSecretEncrypted)));
          }
        }

        if ((mIdentityInfo.mPrivatePeerFileSecretEncrypted.hasData()) &&
            (mIdentityInfo.mPrivatePeerFileSalt.hasData()) &&
            (mIdentityInfo.mSecret.hasData()))
        {
          // mPrivatePeerFileSecret
          // key=hmac(<identity-secret>, "private-peer-file-secret:" + base64(<private-peer-file-salt>))
          // iv=hash(base64(<private-peer-file-salt>))
          SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKey(mIdentityInfo.mSecret), "private-peer-file-secret:" + mIdentityInfo.mPrivatePeerFileSalt);
          SecureByteBlockPtr iv = IHelper::hash(mIdentityInfo.mPrivatePeerFileSalt, IHelper::HashAlgorthm_MD5);

          mPrivatePeerFileSecret = IHelper::makeBufferStringSafe(*IHelper::decrypt(*key, *iv, *IHelper::convertFromBase64(mIdentityInfo.mPrivatePeerFileSecretEncrypted)));
        }

        // should no longer just have a base
        mIdentityInfo.mBase.clear();

        if ((mIdentityInfo.mAccessToken.isEmpty()) ||
            (mIdentityInfo.mAccessSecret.isEmpty()) ||
            (mIdentityInfo.mURI.isEmpty()) ||
            (mIdentityInfo.mSecret.isEmpty())) {
          ZS_LOG_ERROR(Detail, log("missing information for login complete") + mIdentityInfo.getDebugValueString())
          setError(IHTTP::HTTPStatusCode_PreconditionFailed, "Identity login complete result returned from server missing critical data");
          cancel();
          return true;
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorErrorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           IdentityLoginCompleteResultPtr ignore, // will always be NULL
                                                                           message::MessageResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLoginCompleteMonitor) {
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
      #pragma mark ServiceIdentitySession => IMessageMonitorResultDelegate<IdentityAssociateResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorResultReceived(
                                                                      IMessageMonitorPtr monitor,
                                                                      IdentityAssociateResultPtr result
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mAssociateMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mAssociateMonitor->cancel();
        mAssociateMonitor.reset();

        if (mKillAssociation) {
          ZS_LOG_DEBUG(log("association is gone now logging out of identity"))
          cancel();
          return true;
        }

        step();

        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::handleMessageMonitorErrorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           IdentityAssociateResultPtr ignore, // will always be NULL
                                                                           message::MessageResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mLoginCompleteMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        setError(result->errorCode(), result->errorReason());

        ZS_LOG_DEBUG(log("identity associate failed"))

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
        if (monitor != mLoginCompleteMonitor) {
          ZS_LOG_WARNING(Detail, log("monitor notified for obsolete request"))
          return false;
        }

        mSignedIdentityBundleEl = result->identityBundle();
        if (mSignedIdentityBundleEl) {
          mSignedIdentityBundleEl = mSignedIdentityBundleEl->clone()->toElement();
        }

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
        if (monitor != mSignMonitor) {
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
        return String("ServiceIdentitySession [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ServiceIdentitySession::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("identity session id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               (mIdentityInfo.hasData() ? mIdentityInfo.getDebugValueString() : String()) +
               IBootstrappedNetwork::toDebugString(mBootstrappedNetwork) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("reported", toString(mLastReportedState), firstTime) +
               Helper::getDebugValue("error code", 0 != mLastError ? Stringize<typeof(mLastError)>(mLastError).string() : String(), firstTime) +
               Helper::getDebugValue("error reason", mLastErrorReason, firstTime) +
               Helper::getDebugValue("loaded login url", mLoadedLoginURL ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser window visible", mBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("browser redirection complete", mBrowserRedirectionComplete ? String("true") : String(), firstTime) +
               Helper::getDebugValue("client login secret", mClientLoginSecret, firstTime) +
               Helper::getDebugValue("client token", mClientToken, firstTime) +
               Helper::getDebugValue("server token", mServerToken, firstTime) +
               Helper::getDebugValue("need browser window visible", mNeedsBrowserWindowVisible ? String("true") : String(), firstTime) +
               Helper::getDebugValue("notification sent", mNotificationSentToInnerBrowserWindow ? String("true") : String(), firstTime) +
               Helper::getDebugValue("redirection url", mRedirectAfterLoginCompleteURL, firstTime) +
               Helper::getDebugValue("identity login url", mIdentityLoginURL, firstTime) +
               Helper::getDebugValue("identity login complete url", mIdentityLoginCompleteURL, firstTime) +
               Helper::getDebugValue("identity login expires", Time() != mIdentityLoginExpires ? IMessageHelper::timeToString(mIdentityLoginExpires) : String(), firstTime) +
               Helper::getDebugValue("secret", mPrivatePeerFileSecret ? IHelper::convertToString(*mPrivatePeerFileSecret) : String(), firstTime) +
               Helper::getDebugValue("bundle element", mSignedIdentityBundleEl ? String("true") : String(), firstTime) +
               Helper::getDebugValue("bundle checked", mSignedIdentityBundleValidityChecked ? String("true") : String(), firstTime) +
               Helper::getDebugValue("bundle downloaded", mDownloadedSignedIdentityBundle ? String("true") : String(), firstTime) +
               Helper::getDebugValue("pending messages", mPendingMessagesToDeliver.size() > 1 ? Stringize<size_t>(mPendingMessagesToDeliver.size()).string() : String(), firstTime);
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
          setState(SessionState_WaitingAttachment);
          return;
        }

        if (!stepLoginStart()) return;
        if (!stepBrowserWinodw()) return;
        if (!stepLoginComplete()) return;
        if (!stepAssociate()) return;
        if (!stepSign()) return;

        // signal the object is ready
        setState(SessionState_Ready);

        ZS_LOG_DEBUG(log("step complete") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepLoginStart()
      {
        if (mServerToken.hasData()) {
          ZS_LOG_DEBUG(log("step login start is done"))
          return true;
        }

        if (mLoginStartMonitor) {
          ZS_LOG_DEBUG(log("waiting for login start monitor"))
          return false;
        }

        if (!mBootstrappedNetwork->forServices().isPreparationComplete()) {
          ZS_LOG_DEBUG(log("waiting for bootstrapper to complete"))
          return false;
        }

        WORD errorCode = 0;
        String reason;

        if (!mBootstrappedNetwork->forServices().wasSuccessful(&errorCode, &reason)) {
          ZS_LOG_WARNING(Detail, log("login failed because of bootstrapper failure"))
          setError(errorCode, reason);
          cancel();
          return false;
        }

        mIdentityInfo.mProvider = mBootstrappedNetwork->forServices().getDomain();

        IdentityLoginStartRequestPtr request = IdentityLoginStartRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());
        request->clientToken(mClientToken);

        mLoginStartMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityLoginStartResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-login-start", request);
        setState(SessionState_Pending);

        ZS_LOG_DEBUG(log("sending login start request"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepBrowserWinodw()
      {
        if (!mLoadedLoginURL) {
          setState(SessionState_WaitingToLoadBrowserWindow);
          return false;
        }

        if ((mNeedsBrowserWindowVisible) &&
            (!mBrowserWindowVisible)) {
          setState(SessionState_WaitingToMakeBrowserWindowVisible);
          return false;
        }

        // check to see if notification has been sent to inner browser window
        if (!mNotificationSentToInnerBrowserWindow) {
          IdentityLoginNotifyPtr request = IdentityLoginNotify::create();
          request->domain(mBootstrappedNetwork->forServices().getDomain());

          request->identityInfo(mIdentityInfo);
          request->clientToken(mClientToken);
          request->serverToken(mServerToken);
          request->browserVisibility(IdentityLoginNotify::BrowserVisibility_VisibleOnDemand);
          request->postLoginRedirectURL(mRedirectAfterLoginCompleteURL);
          request->clientLoginSecret(mClientLoginSecret);

          sendInnerWindowMessage(request);

          mNotificationSentToInnerBrowserWindow = true;
        }

        if (!mBrowserRedirectionComplete) {
          setState(SessionState_WaitingLoginCompleteBrowserRedirection);
          return false;
        }

        ZS_LOG_DEBUG(log("step browser window is done"))
        return true;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepLoginComplete()
      {
        if (mIdentityInfo.mAccessToken.hasData()) {
          ZS_LOG_DEBUG(log("step login complete is done"))
          return true;
        }

        if (mLoginCompleteMonitor) {
          ZS_LOG_DEBUG(log("waiting for login complete monitor"))
          return false;
        }

        IdentityLoginCompleteRequestPtr request = IdentityLoginCompleteRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());
        request->clientToken(mClientToken);
        request->serverToken(mServerToken);

        mLoginCompleteMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityLoginCompleteResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-login-complete", request);
        setState(SessionState_Pending);

        ZS_LOG_DEBUG(log("sending login complete request"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepAssociate()
      {
        if (mAssociateMonitor) {
          ZS_LOG_DEBUG(log("waiting for associate monitor to complete"))
          return false;
        }

        if (mKillAssociation) {
          IdentityAssociateRequestPtr request = IdentityAssociateRequest::create();
          request->domain(mBootstrappedNetwork->forServices().getDomain());

          request->identityInfo(mIdentityInfo);

          mAssociateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityLoginCompleteResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
          mBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-associate", request);
          ZS_LOG_DEBUG(log("removing association request"))
          return false;
        }

        ServiceLockboxSessionPtr associatedPeerContact = mAssociatedPeerContact.lock();
        if (!associatedPeerContact) {
          setState(SessionState_WaitingAssociation);
          ZS_LOG_DEBUG(log("waiting for identity to become associated to a peer contact"))
          return false;
        }

        if (IServiceLockboxSession::SessionState_Shutdown == associatedPeerContact->forServiceIdentity().getState()) {
          ZS_LOG_WARNING(Detail, log("service peer contact session is shutdown thus identity must shutdown"))
          setError(IHTTP::HTTPStatusCode_ResetContent, "Peer contact assoicated with identity is already shutdown");
          cancel();
          return false;
        }

        if (IServiceLockboxSession::SessionState_Ready != associatedPeerContact->forServiceIdentity().getState()) {
          setState(SessionState_Pending);
          ZS_LOG_DEBUG(log("waiting for service peer contact session state to be ready"))
          return false;
        }

        IPeerFilesPtr peerFiles = associatedPeerContact->forServiceIdentity().getPeerFiles();
        IPeerFilePublicPtr peerFilePublic;
        if (peerFiles) {
          peerFilePublic = peerFiles->getPeerFilePublic();
        }
        if ((!peerFiles) ||
            (!peerFilePublic)) {
          ZS_LOG_ERROR(Detail, log("peer files must be valid if associatd peer contact is associated and ready"))
          setError(IHTTP::HTTPStatusCode_InternalServerError, "Peer files are not valid");
          cancel();
          return false;
        }

        if (mIdentityInfo.mContactUserID.hasData()) {
          // check to see if it matches what we believe
          if (mIdentityInfo.mContactUserID != associatedPeerContact->forServiceIdentity().getContactUserID()) {
            ZS_LOG_WARNING(Detail, log("contact user ID changed") + IServiceLockboxSession::toDebugString(associatedPeerContact) + mIdentityInfo.getDebugValueString())
            goto not_same_contact;
          }

          if (mIdentityInfo.mContact != peerFilePublic->getPeerURI()) {
            ZS_LOG_WARNING(Detail, log("peer URI changed") + IPeerFilePublic::toDebugString(peerFilePublic) + mIdentityInfo.getDebugValueString())
            goto not_same_contact;
          }
          goto same_contact;

        not_same_contact:
          mIdentityInfo.mContactUserID.clear();
          mIdentityInfo.mContact.clear();

        same_contact:
          ZS_LOG_DEBUG(log("contact matches?") + ", matches=" + (mIdentityInfo.mContactUserID.hasData() ? "true" : "false") + mIdentityInfo.getDebugValueString())
        }

        if (mIdentityInfo.mContactUserID.hasData()) {
          ZS_LOG_DEBUG(log("step associate is done") + mIdentityInfo.getDebugValueString())
          return true;
        }

        setState(SessionState_Pending);

        mIdentityInfo.mContactUserID = associatedPeerContact->forServiceIdentity().getContactUserID();
        mIdentityInfo.mContact = peerFilePublic->getPeerURI();

        if ((mIdentityInfo.mContactUserID.isEmpty()) ||
            (mIdentityInfo.mContact.isEmpty())) {
          ZS_LOG_ERROR(Detail, log("contact user ID and peer URI must be valid is peer contact is associated and ready"))
          setError(IHTTP::HTTPStatusCode_InternalServerError, "Missing critical information from associated peer contact");
          cancel();
        }

        IdentityAssociateRequestPtr request = IdentityAssociateRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        request->identityInfo(mIdentityInfo);
        request->peerFiles(peerFiles);

        mAssociateMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentityLoginCompleteResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-associate", request);
        ZS_LOG_DEBUG(log("sending associate request"))
        return false;
      }

      //-----------------------------------------------------------------------
      bool ServiceIdentitySession::stepSign()
      {
        if (mSignMonitor) {
          ZS_LOG_DEBUG(log("waiting for sign monitor to complete"))
          return false;
        }

        if (!mSignedIdentityBundleValidityChecked) {
          if (mSignedIdentityBundleEl) {

            // figure out of the signed identity bundle still has enough life time left
            try {
              ElementPtr signatureEl;
              ElementPtr signedEl = IHelper::getSignatureInfo(mSignedIdentityBundleEl, &signatureEl);

              if (!mBootstrappedNetwork->forServices().isValidSignature(signedEl)) {
                ZS_LOG_WARNING(Detail, log("bootstrapper reported signature as invalid"))
                goto invalid_certificate;
              }

              Time created = IMessageHelper::stringToTime(signedEl->findFirstChildElementChecked("created")->getTextDecoded());
              Time expires = IMessageHelper::stringToTime(signedEl->findFirstChildElementChecked("expires")->getTextDecoded());
              Time now = zsLib::now();

              if ((Time() == created) ||
                  (Time() == expires) ||
                  (created > expires) ||
                  (now < created)) {
                ZS_LOG_WARNING(Detail, log("certificate has invalid date range") + ", created=" + IMessageHelper::timeToString(created) + ", expires=" + IMessageHelper::timeToString(expires) + ", now=" + IMessageHelper::timeToString(now))
                goto invalid_certificate;
              }

              if (now > expires) {
                ZS_LOG_WARNING(Detail, log("certificate has expired"))
                goto invalid_certificate;
              }

              long durationFromCreated = (now - created).total_seconds();
              long durationToTotal = (expires - created).total_seconds();

              if ((durationFromCreated * 100) / durationToTotal > (100-HOOKFLASH_STACK_SERVICE_IDENTITY_MAX_PERCENTAGE_TIME_REMAINING_BEFORE_RESIGN_IDENTITY_REQUIRED)) {
                ZS_LOG_WARNING(Detail, log("resign identity required since certificate will expire soon") + ", created=" + IMessageHelper::timeToString(created) + ", expires=" + IMessageHelper::timeToString(expires) + ", now=" + IMessageHelper::timeToString(now))
                goto invalid_certificate;
              }
              mSignedIdentityBundleValidityChecked = true;
              goto valid_certificate;
            } catch(CheckFailed &) {
              ZS_LOG_DEBUG(log("failed to obtain information from signature"))
              goto invalid_certificate;
            }

          invalid_certificate:
            mSignedIdentityBundleEl.reset();

            if (mDownloadedSignedIdentityBundle) {
              ZS_LOG_ERROR(Detail, log("download new certificate but the signature returned was not valid"))
              setError(IHTTP::HTTPStatusCode_ServiceUnavailable, "Downloaded signed identity is not valid");
              cancel();
              return false;
            }

          valid_certificate:
            ZS_LOG_DEBUG(log("certificate valid?") + ", valid=" + (mSignedIdentityBundleEl ? "true:" : "false"))
          }
        }

        if (mSignedIdentityBundleEl) {
          ZS_LOG_DEBUG(log("already downloaded valid certificate"))
          return true;
        }

        // need to obtain signed identity bundle, but only if peer contact has been associated...
        IdentitySignRequestPtr request = IdentitySignRequest::create();
        request->domain(mBootstrappedNetwork->forServices().getDomain());

        request->identityInfo(mIdentityInfo);

        mSignMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<IdentitySignResult>::convert(mThisWeak.lock()), request, Seconds(HOOKFLASH_STACK_SERVICE_IDENTITY_TIMEOUT_IN_SECONDS));
        mBootstrappedNetwork->forServices().sendServiceMessage("identity", "identity-sign", request);
        ZS_LOG_DEBUG(log("sending sign request"))
        return false;
      }

      //-----------------------------------------------------------------------
      void ServiceIdentitySession::setState(SessionStates state)
      {
        if (state != mCurrentState) {
          ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState))
          mCurrentState = state;

          ServiceLockboxSessionPtr associatedPeerContact = mAssociatedPeerContact.lock();
          if (associatedPeerContact) {
            associatedPeerContact->forServiceIdentity().notifyStateChanged();
          }
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

      const boost::regex e("^identity:\\/\\/([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}\\/.+$");
      if (!boost::regex_match(identityURI, e)) {
        const boost::regex e2("^identity:[a-zA-Z0-9\\-_]{0,61}:.+$");
        if (!boost::regex_match(identityURI, e2)) {
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

      const boost::regex e("^identity:\\/\\/([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}\\/$");
      if (!boost::regex_match(identityBase, e)) {
        const boost::regex e2("^identity:[a-zA-Z0-9\\-_]{0,61}:$");
        if (!boost::regex_match(identityBase, e2)) {
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

      const boost::regex e("^identity:[a-zA-Z0-9\\-_]{0,61}:.*$");
      if (!boost::regex_match(identityURI, e)) {
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
        const boost::regex e("^identity:[a-zA-Z0-9\\-_]{0,61}:.*$");
        if (boost::regex_match(identityURI, e)) {

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

      const boost::regex e("^identity:\\/\\/([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}\\/.*$");
      if (!boost::regex_match(identityURI, e)) {
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
      String domainOrType(inDomainOrType ? inDomainOrType : "");
      String identifier(inIdentifier ? inIdentifier : "");

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
        case SessionState_WaitingAttachment:                        return "Waiting Attachment";
        case SessionState_WaitingToLoadBrowserWindow:               return "Waiting to Load Browser Window";
        case SessionState_WaitingToMakeBrowserWindowVisible:        return "Waiting to Make Browser Window Visible";
        case SessionState_WaitingLoginCompleteBrowserRedirection:   return "Waiting for Login Complete Browser Redirect";
        case SessionState_WaitingAssociation:                       return "Waiting Association";
        case SessionState_Ready:                                    return "Ready";
        case SessionState_Shutdown:                                 return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    IServiceIdentitySessionPtr IServiceIdentitySession::loginWithIdentity(
                                                                          IServiceIdentitySessionDelegatePtr delegate,
                                                                          const char *redirectAfterLoginCompleteURL,
                                                                          const char *identityURI,
                                                                          IServiceIdentityPtr provider
                                                                          )
    {
      return internal::IServiceIdentitySessionFactory::singleton().loginWithIdentity(delegate, redirectAfterLoginCompleteURL, identityURI, provider);
    }

    //-------------------------------------------------------------------------
    IServiceIdentitySessionPtr IServiceIdentitySession::loginWithIdentityTBD(
                                                                             IServiceIdentitySessionDelegatePtr delegate,
                                                                             const char *redirectAfterLoginCompleteURL,
                                                                             IServiceIdentityPtr provider,
                                                                             const char *legacyIdentityBaseURI
                                                                             )
    {
      return internal::IServiceIdentitySessionFactory::singleton().loginWithIdentityTBD(delegate, redirectAfterLoginCompleteURL, provider, legacyIdentityBaseURI);
    }

    //-------------------------------------------------------------------------
    IServiceIdentitySessionPtr IServiceIdentitySession::loginWithIdentityBundle(
                                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                                const char *redirectAfterLoginCompleteURL,
                                                                                ElementPtr signedIdentityBundle
                                                                                )
    {
      return internal::IServiceIdentitySessionFactory::singleton().loginWithIdentityBundle(delegate, redirectAfterLoginCompleteURL, signedIdentityBundle);
    }

    //-------------------------------------------------------------------------
    IServiceIdentitySessionPtr IServiceIdentitySession::relogin(
                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                const char *redirectAfterLoginCompleteURL,
                                                                IServiceIdentityPtr provider,
                                                                const char *identityReloginAccessKey
                                                                )
    {
      return internal::IServiceIdentitySessionFactory::singleton().relogin(delegate, redirectAfterLoginCompleteURL, provider, identityReloginAccessKey);
    }
  }
}
