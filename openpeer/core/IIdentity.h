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

#include <openpeer/core/types.h>

namespace hookflash
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIdentity
    #pragma mark

    interaction IIdentity
    {
      enum IdentityStates
      {
        IdentityState_Pending,
        IdentityState_PendingAssociation,
        IdentityState_WaitingAttachmentOfDelegate,
        IdentityState_WaitingForBrowserWindowToBeLoaded,
        IdentityState_WaitingForBrowserWindowToBeMadeVisible,
        IdentityState_WaitingForBrowserWindowToClose,
        IdentityState_Ready,
        IdentityState_Shutdown,
      };

      static const char *toString(IdentityStates state);

      static String toDebugString(IIdentityPtr identity, bool includeCommaPrefix = true);

      static IIdentityPtr login(
                                IAccountPtr account,
                                IIdentityDelegatePtr delegate,
                                const char *outerFrameURLUponReload,
                                const char *identityURI_or_identityBaseURI,
                                const char *identityProviderDomain // used when identity URI is of legacy or oauth-type
                                );

      virtual IdentityStates getState(
                                     WORD *outLastErrorCode,
                                     String *outLastErrorReason
                                     ) const = 0;

      virtual PUID getID() const = 0;

      virtual bool isDelegateAttached() const = 0;
      virtual void attachDelegate(
                                  IIdentityDelegatePtr delegate,
                                  const char *outerFrameURLUponReload
                                  ) = 0;

      virtual String getIdentityURI() const = 0;
      virtual String getIdentityProviderDomain() const = 0;
      virtual ElementPtr getSignedIdentityBundle() const = 0;

      virtual String getInnerBrowserWindowFrameURL() const = 0;

      virtual void notifyBrowserWindowVisible() = 0;
      virtual void notifyBrowserWindowClosed() = 0;

      virtual ElementPtr getNextMessageForInnerBrowerWindowFrame() = 0;
      virtual void handleMessageFromInnerBrowserWindowFrame(ElementPtr message) = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIdentityDelegate
    #pragma mark

    interaction IIdentityDelegate
    {
      typedef IIdentity::IdentityStates IdentityStates;

      virtual void onIdentityStateChanged(
                                          IIdentityPtr identity,
                                          IdentityStates state
                                          ) = 0;
      virtual void onIdentityPendingMessageForInnerBrowserWindowFrame(IIdentityPtr identity) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::IIdentityDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::IIdentityPtr, IIdentityPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::IIdentity::IdentityStates, IdentityStates)
ZS_DECLARE_PROXY_METHOD_2(onIdentityStateChanged, IIdentityPtr, IdentityStates)
ZS_DECLARE_PROXY_METHOD_1(onIdentityPendingMessageForInnerBrowserWindowFrame, IIdentityPtr)
ZS_DECLARE_PROXY_END()
