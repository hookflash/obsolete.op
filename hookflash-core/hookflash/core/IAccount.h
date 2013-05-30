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

#include <hookflash/core/types.h>

namespace hookflash
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccount
    #pragma mark

    interaction IAccount
    {
      enum AccountStates
      {
        AccountState_Pending,
        AccountState_PendingPeerFilesGeneration,
        AccountState_WaitingForAssociationToIdentity,
        AccountState_WaitingForBrowserWindowToBeLoaded,
        AccountState_WaitingForBrowserWindowToBeMadeVisible,
        AccountState_WaitingForBrowserWindowToClose,
        AccountState_Ready,
        AccountState_ShuttingDown,
        AccountState_Shutdown,
      };

      static const char *toString(AccountStates state);

      static String toDebugString(IAccountPtr account, bool includeCommaPrefix = true);

      static IAccountPtr login(
                               IAccountDelegatePtr delegate,
                               IConversationThreadDelegatePtr conversationThreadDelegate,
                               ICallDelegatePtr callDelegate,
                               const char *lockboxOuterFrameURLUponReload,
                               const char *lockboxServiceDomain,
                               const char *lockboxGrantID,
                               bool forceCreateNewLockboxAccount = false
                               );

      static IAccountPtr relogin(
                                 IAccountDelegatePtr delegate,
                                 IConversationThreadDelegatePtr conversationThreadDelegate,
                                 ICallDelegatePtr callDelegate,
                                 const char *lockboxOuterFrameURLUponReload,
                                 ElementPtr reloginInformation
                                 );

      virtual PUID getID() const = 0;

      virtual AccountStates getState(
                                     WORD *outErrorCode,
                                     String *outErrorReason
                                     ) const = 0;

      virtual ElementPtr getReloginInformation() const = 0;   // NOTE: will return ElementPtr() is relogin information is not available yet

      virtual String getLocationID() const = 0;

      virtual void shutdown() = 0;

      virtual ElementPtr savePeerFilePrivate() const = 0;
      virtual SecureByteBlockPtr getPeerFilePrivateSecret() const = 0;

      virtual IdentityListPtr getAssociatedIdentities() const = 0;
      virtual void removeIdentities(const IdentityList &identitiesToRemove) = 0;

      virtual String getInnerBrowserWindowFrameURL() const = 0;

      virtual void notifyBrowserWindowVisible() = 0;
      virtual void notifyBrowserWindowClosed() = 0;

      virtual ElementPtr getNextMessageForInnerBrowerWindowFrame() = 0;
      virtual void handleMessageFromInnerBrowserWindowFrame(ElementPtr unparsedMessage) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountDelegate
    #pragma mark

    interaction IAccountDelegate
    {
      typedef IAccount::AccountStates AccountStates;

      virtual void onAccountStateChanged(
                                         IAccountPtr account,
                                         AccountStates state
                                         ) = 0;

      virtual void onAccountAssociatedIdentitiesChanged(IAccountPtr account) = 0;

      virtual void onAccountPendingMessageForInnerBrowserWindowFrame(IAccountPtr account) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::IAccountDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::IAccountPtr, IAccountPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::IAccount::AccountStates, AccountStates)
ZS_DECLARE_PROXY_METHOD_2(onAccountStateChanged, IAccountPtr, AccountStates)
ZS_DECLARE_PROXY_METHOD_1(onAccountAssociatedIdentitiesChanged, IAccountPtr)
ZS_DECLARE_PROXY_METHOD_1(onAccountPendingMessageForInnerBrowserWindowFrame, IAccountPtr)
ZS_DECLARE_PROXY_END()
