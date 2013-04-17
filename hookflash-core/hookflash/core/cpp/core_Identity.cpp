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

#include <hookflash/core/internal/core_Identity.h>
#include <hookflash/core/internal/core_Stack.h>

#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/IHelper.h>

#include <zsLib/Stringize.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace hookflash { namespace core { ZS_DECLARE_SUBSYSTEM(hookflash_core) } }


namespace hookflash
{
  namespace core
  {
    using stack::IServiceIdentitySession;

    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static IServiceIdentitySession::SessionStates toState(IIdentity::IdentityStates state)
      {
        switch (state)
        {
          case IIdentity::IdentityState_Pending:                                  return IServiceIdentitySession::SessionState_Pending;
          case IIdentity::IdentityState_WaitingAttachment:                        return IServiceIdentitySession::SessionState_WaitingAttachment;
          case IIdentity::IdentityState_WaitingToLoadBrowserWindow:               return IServiceIdentitySession::SessionState_WaitingToLoadBrowserWindow;
          case IIdentity::IdentityState_WaitingToMakeBrowserWindowVisible:        return IServiceIdentitySession::SessionState_WaitingToMakeBrowserWindowVisible;
          case IIdentity::IdentityState_WaitingLoginCompleteBrowserRedirection:   return IServiceIdentitySession::SessionState_WaitingLoginCompleteBrowserRedirection;
          case IIdentity::IdentityState_WaitingAssociation:                       return IServiceIdentitySession::SessionState_WaitingAssociation;
          case IIdentity::IdentityState_Ready:                                    return IServiceIdentitySession::SessionState_Ready;
          case IIdentity::IdentityState_Shutdown:                                 return IServiceIdentitySession::SessionState_Shutdown;
        }

        return IServiceIdentitySession::SessionState_Pending;
      }

      //-----------------------------------------------------------------------
      static IIdentity::IdentityStates toState(IServiceIdentitySession::SessionStates state)
      {
        switch (state)
        {
          case IServiceIdentitySession::SessionState_Pending:                                 return IIdentity::IdentityState_Pending;
          case IServiceIdentitySession::SessionState_WaitingAttachment:                       return IIdentity::IdentityState_WaitingAttachment;
          case IServiceIdentitySession::SessionState_WaitingToLoadBrowserWindow:              return IIdentity::IdentityState_WaitingToLoadBrowserWindow;
          case IServiceIdentitySession::SessionState_WaitingToMakeBrowserWindowVisible:       return IIdentity::IdentityState_WaitingToMakeBrowserWindowVisible;
          case IServiceIdentitySession::SessionState_WaitingLoginCompleteBrowserRedirection:  return IIdentity::IdentityState_WaitingLoginCompleteBrowserRedirection;
          case IServiceIdentitySession::SessionState_WaitingAssociation:                      return IIdentity::IdentityState_WaitingAssociation;
          case IServiceIdentitySession::SessionState_Ready:                                   return IIdentity::IdentityState_Ready;
          case IServiceIdentitySession::SessionState_Shutdown:                                return IIdentity::IdentityState_Shutdown;
        }

        return IIdentity::IdentityState_Pending;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IIdentityForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      IdentityPtr IIdentityForAccount::createFromExistingSession(IServiceIdentitySessionPtr session)
      {
        return IIdentityFactory::singleton().createFromExistingSession(session);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Identity
      #pragma mark

      //-----------------------------------------------------------------------
      Identity::Identity(IMessageQueuePtr queue) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID())
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void Identity::init()
      {
      }

      //-----------------------------------------------------------------------
      Identity::~Identity()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      IdentityPtr Identity::convert(IIdentityPtr contact)
      {
        return boost::dynamic_pointer_cast<Identity>(contact);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Identity => IIdentity
      #pragma mark

      //-----------------------------------------------------------------------
      String Identity::toDebugString(IIdentityPtr identity, bool includeCommaPrefix)
      {
        if (!identity) return includeCommaPrefix ? String(", identity=(null)") : String("identity=(null)");
        return Identity::convert(identity)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      IdentityPtr Identity::login(
                                  IIdentityDelegatePtr delegate,
                                  const char *redirectAfterLoginCompleteURL,
                                  const char *inIdentityURI_or_identityBaseURI,
                                  const char *inIdentityProviderDomain
                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!redirectAfterLoginCompleteURL)
        ZS_THROW_INVALID_ARGUMENT_IF((!inIdentityURI_or_identityBaseURI) && (!inIdentityProviderDomain))

        IdentityPtr pThis(new Identity(IStackForInternal::queueCore()));
        pThis->mThisWeak = pThis;
        pThis->mDelegate = IIdentityDelegateProxy::createWeak(IStackForInternal::queueApplication(), delegate);

        String identity(inIdentityURI_or_identityBaseURI ? String(inIdentityURI_or_identityBaseURI) : String());
        String domain(inIdentityProviderDomain ? String(inIdentityProviderDomain) : String());

        if (domain.hasData()) {
          if (!stack::IHelper::isValidDomain(domain)) {
            ZS_LOG_ERROR(Detail, pThis->log("domain specified was not valid") + ", domain=" + domain)
            return IdentityPtr();
          }
        }

        if (IServiceIdentity::isLegacy(identity)) {
          if (domain.isEmpty()) {
            ZS_LOG_ERROR(Detail, pThis->log("domain for legacy identity is required") + ", identity=" + identity)
            return IdentityPtr();
          }
        }

        IServiceIdentityPtr provider;
        if (domain.hasData()) {
          ZS_LOG_DEBUG(pThis->log("preparing bootstrapped network domain") + ", identity=" + identity + ", domain=" + domain)
          IBootstrappedNetworkPtr network = IBootstrappedNetwork::prepare(domain);
          if (!network) {
            ZS_LOG_ERROR(Detail, pThis->log("bootstrapper failed for domain specified") + ", identity=" + identity + ", domain=" + domain)
            return IdentityPtr();
          }
          provider = IServiceIdentity::createServiceIdentityFrom(network);
          ZS_THROW_BAD_STATE_IF(!provider)
        }

        if (IServiceIdentity::isValid(identity)) {
          // this is a fully validated identity scenario
          pThis->mSession = IServiceIdentitySession::loginWithIdentity(pThis, redirectAfterLoginCompleteURL, identity, provider);
        } else {
          if (!IServiceIdentity::isValidBase(identity)) {
            ZS_LOG_ERROR(Detail, pThis->log("identit specified is not valid") + ", identity=" + identity + ", domain=" + domain)
            return IdentityPtr();
          }
            pThis->mSession = IServiceIdentitySession::loginWithIdentityTBD(pThis, redirectAfterLoginCompleteURL, provider, identity);
        }

        pThis->init();

        return pThis;
      }

      //-----------------------------------------------------------------------
      IIdentity::IdentityStates Identity::getState(
                                                   WORD *outLastErrorCode,
                                                   String *outLastErrorReason
                                                   ) const
      {
        return toState(mSession->getState(outLastErrorCode, outLastErrorReason));
      }

      //-----------------------------------------------------------------------
      bool Identity::isAttached() const
      {
        return mSession->isAttached();
      }

      //-----------------------------------------------------------------------
      void Identity::attach(
                            const char *redirectAfterLoginCompleteURL,
                            IIdentityDelegatePtr delegate
                            )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!redirectAfterLoginCompleteURL)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        mDelegate = IIdentityDelegateProxy::createWeak(IStackForInternal::queueApplication(), delegate);
        mSession->attach(redirectAfterLoginCompleteURL, mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      String Identity::getIdentityURI() const
      {
        return mSession->getIdentityURI();
      }

      //-----------------------------------------------------------------------
      String Identity::getIdentityProviderDomain() const
      {
        return mSession->getIdentityProviderDomain();
      }

      //-----------------------------------------------------------------------
      String Identity::getIdentityReloginAccessKey() const
      {
        return mSession->getIdentityReloginAccessKey();
      }

      //-----------------------------------------------------------------------
      ElementPtr Identity::getSignedIdentityBundle() const
      {
        ElementPtr bundleEl = mSession->getSignedIdentityBundle();
        return bundleEl->clone()->toElement();
      }

      //-----------------------------------------------------------------------
      String Identity::getIdentityLoginURL() const
      {
        return mSession->getIdentityLoginURL();
      }

      //-----------------------------------------------------------------------
      Time Identity::getLoginExpires() const
      {
        return mSession->getLoginExpires();
      }

      //-----------------------------------------------------------------------
      void Identity::notifyBrowserWindowVisible()
      {
        mSession->notifyBrowserWindowVisible();
      }

      //-----------------------------------------------------------------------
      void Identity::notifyLoginCompleteBrowserWindowRedirection()
      {
        mSession->notifyLoginCompleteBrowserWindowRedirection();
      }

      //-----------------------------------------------------------------------
      ElementPtr Identity::getNextMessageForInnerBrowerWindowFrame()
      {
        DocumentPtr doc = mSession->getNextMessageForInnerBrowerWindowFrame();
        if (!doc) return ElementPtr();
        ElementPtr root = doc->getFirstChildElement();
        root->orphan();
        return root;
      }

      //-----------------------------------------------------------------------
      void Identity::handleMessageFromInnerBrowserWindowFrame(ElementPtr message)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!message)
        DocumentPtr doc = Document::create();
        doc->adoptAsLastChild(message);
        mSession->handleMessageFromInnerBrowserWindowFrame(doc);
      }

      //-----------------------------------------------------------------------
      void Identity::cancel()
      {
        mSession->cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Identity => IIdentityForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      IdentityPtr Identity::createFromExistingSession(IServiceIdentitySessionPtr session)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!session)

        IdentityPtr pThis(new Identity(IStackForInternal::queueCore()));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IServiceIdentitySessionPtr Identity::getSession() const
      {
        return mSession;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Identity => IServiceIdentitySessionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Identity::onServiceIdentitySessionStateChanged(
                                                          IServiceIdentitySessionPtr session,
                                                          SessionStates state
                                                          )
      {
        ZS_THROW_BAD_STATE_IF(!mDelegate)
        try {
          mDelegate->onIdentityStateChanged(mThisWeak.lock(), toState(state));
        } catch (IIdentityDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      void Identity::onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session)
      {
        ZS_THROW_BAD_STATE_IF(!mDelegate)
        try {
          mDelegate->onIdentityPendingMessageForInnerBrowserWindowFrame(mThisWeak.lock());
        } catch (IIdentityDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Identity => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Identity::log(const char *message) const
      {
        return String("Identity [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String Identity::getDebugValueString(bool includeCommaPrefix) const
      {
        return IServiceIdentitySession::toDebugString(mSession, includeCommaPrefix);
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
    #pragma mark IIdentity
    #pragma mark

    //-------------------------------------------------------------------------
    String IIdentity::toDebugString(IIdentityPtr identity, bool includeCommaPrefix)
    {
      return internal::Identity::toDebugString(identity);
    }

    //-------------------------------------------------------------------------
    const char *IIdentity::toString(IdentityStates state)
    {
      return IServiceIdentitySession::toString(internal::toState(state));
    }

    //-------------------------------------------------------------------------
    IIdentityPtr IIdentity::login(
                                  IIdentityDelegatePtr delegate,
                                  const char *redirectAfterLoginCompleteURL,
                                  const char *identityURI_or_identityBaseURI,
                                  const char *identityProviderDomain
                                  )
    {
      return internal::IIdentityFactory::singleton().login(delegate, redirectAfterLoginCompleteURL, identityURI_or_identityBaseURI, identityProviderDomain);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
  }
}
