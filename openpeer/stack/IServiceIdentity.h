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

#pragma once

#include <hookflash/stack/types.h>


namespace hookflash
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentity
    #pragma mark

    interaction IServiceIdentity
    {
      static bool isValid(const char *identityURI);
      static bool isValidBase(const char *identityBase);
      static bool isLegacy(const char *identityURI);

      static bool splitURI(
                           const char *identityURI,
                           String &outDomainOrLegacyType,
                           String &outIdentifier,
                           bool *outIsLegacy = NULL
                           );

      static String joinURI(
                            const char *domainOrType,
                            const char *identifier
                            );


      static IServiceIdentityPtr createServiceIdentityFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

      virtual PUID getID() const = 0;

      virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const = 0;

      virtual String getDomain() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentitySession
    #pragma mark

    interaction IServiceIdentitySession
    {
      enum SessionStates
      {
        SessionState_Pending,
        SessionState_WaitingAttachmentOfDelegate,
        SessionState_WaitingForBrowserWindowToBeLoaded,
        SessionState_WaitingForBrowserWindowToBeMadeVisible,
        SessionState_WaitingForBrowserWindowToClose,
        SessionState_WaitingForAssociationToLockbox,
        SessionState_Ready,
        SessionState_Shutdown,
      };

      static const char *toString(SessionStates state);

      static String toDebugString(IServiceIdentitySessionPtr session, bool includeCommaPrefix = true);

      // use when the identity URI is known (or partially known), provider is required if type is legacy
      static IServiceIdentitySessionPtr loginWithIdentity(
                                                          IServiceIdentitySessionDelegatePtr delegate,
                                                          IServiceIdentityPtr provider,
                                                          IServiceNamespaceGrantSessionPtr grantSession,
                                                          IServiceLockboxSessionPtr existingLockbox,            // pass NULL IServiceLockboxSessionPtr() if none exists
                                                          const char *outerFrameURLUponReload,
                                                          const char *identityURI
                                                          );

      // use when provider is known but nothing more
      static IServiceIdentitySessionPtr loginWithIdentityProvider(
                                                                  IServiceIdentitySessionDelegatePtr delegate,
                                                                  IServiceIdentityPtr provider,
                                                                  IServiceNamespaceGrantSessionPtr grantSession,
                                                                  IServiceLockboxSessionPtr existingLockbox,    // pass NULL IServiceLockboxSessionPtr() if none exists
                                                                  const char *outerFrameURLUponReload,
                                                                  const char *legacyIdentityBaseURI = NULL
                                                                  );

      // use when a signed identity bundle is available
      static IServiceIdentitySessionPtr loginWithIdentityBundle(
                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                IServiceIdentityPtr provider,
                                                                IServiceNamespaceGrantSessionPtr grantSession,
                                                                IServiceLockboxSessionPtr existingLockbox,      // pass NULL IServiceLockboxSessionPtr() if none exists
                                                                const char *outerFrameURLUponReload,
                                                                ElementPtr signedIdentityBundle
                                                                );

      virtual PUID getID() const = 0;

      virtual IServiceIdentityPtr getService() const = 0;

      virtual SessionStates getState(
                                     WORD *outLastErrorCode,
                                     String *outLastErrorReason
                                     ) const = 0;

      virtual bool isDelegateAttached() const = 0;
      virtual void attachDelegate(
                                  IServiceIdentitySessionDelegatePtr delegate,
                                  const char *outerFrameURLUponReload
                                  ) = 0;

      virtual String getIdentityURI() const = 0;
      virtual String getIdentityProviderDomain() const = 0;
      virtual ElementPtr getSignedIdentityBundle() const = 0;   // must clone if you intend to adopt

      virtual String getInnerBrowserWindowFrameURL() const = 0;

      virtual void notifyBrowserWindowVisible() = 0;
      virtual void notifyBrowserWindowClosed() = 0;

      virtual DocumentPtr getNextMessageForInnerBrowerWindowFrame() = 0;
      virtual void handleMessageFromInnerBrowserWindowFrame(DocumentPtr unparsedMessage) = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentitySessionDelegate
    #pragma mark

    interaction IServiceIdentitySessionDelegate
    {
      typedef IServiceIdentitySession::SessionStates SessionStates;

      virtual void onServiceIdentitySessionStateChanged(
                                                        IServiceIdentitySessionPtr session,
                                                        SessionStates state
                                                        ) = 0;

      virtual void onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IServiceIdentitySessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceIdentitySessionPtr, IServiceIdentitySessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceIdentitySessionDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onServiceIdentitySessionStateChanged, IServiceIdentitySessionPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame, IServiceIdentitySessionPtr)
ZS_DECLARE_PROXY_END()
