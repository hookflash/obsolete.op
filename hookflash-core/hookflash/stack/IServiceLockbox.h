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
    #pragma mark IServiceLockbox
    #pragma mark

    interaction IServiceLockbox
    {
      static IServiceLockboxPtr createServiceLockboxFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

      virtual PUID getID() const = 0;

      virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceLockboxSession
    #pragma mark

    interaction IServiceLockboxSession
    {
      enum SessionStates
      {
        SessionState_Pending,
        SessionState_PendingPeerFilesGeneration,
        SessionState_WaitingForBrowserWindowToBeLoaded,
        SessionState_WaitingForBrowserWindowToBeMadeVisible,
        SessionState_WaitingForBrowserWindowToClose,
        SessionState_Ready,
        SessionState_Shutdown,
      };
      static const char *toString(SessionStates state);

      static String toDebugString(IServiceLockboxSessionPtr session, bool includeCommaPrefix = true);

      static IServiceLockboxSessionPtr login(
                                             IServiceLockboxSessionDelegatePtr delegate,
                                             IServiceLockboxPtr ServiceLockbox,
                                             IServiceIdentitySessionPtr identitySession,
                                             const char *outerFrameURLUponReload,
                                             const char *lockboxGrantID,
                                             bool forceNewAccount = false
                                             );

      static IServiceLockboxSessionPtr relogin(
                                               IServiceLockboxSessionDelegatePtr delegate,
                                               IServiceLockboxPtr serviceLockbox,
                                               const char *outerFrameURLUponReload,
                                               const char *lockboxAccountID,
                                               const char *lockboxGrantID,
                                               const char *identityHalfLockboxKey,
                                               const char *lockboxHalfLockboxKey
                                               );

      virtual PUID getID() const = 0;

      virtual IServiceLockboxPtr getService() const = 0;

      virtual SessionStates getState(
                                     WORD *lastErrorCode,
                                     String *lastErrorReason
                                     ) const = 0;

      virtual IPeerFilesPtr getPeerFiles() const = 0;

      virtual String getAccountID() const = 0;
      virtual String getDomain() const = 0;
      virtual String getStableID() const = 0;

      virtual void getLockboxKey(
                                 SecureByteBlockPtr &outIdentityHalf,
                                 SecureByteBlockPtr &outLockboxHalf
                                 ) = 0;

      virtual ServiceIdentitySessionListPtr getAssociatedIdentities() const = 0;
      virtual void associateIdentities(
                                       const ServiceIdentitySessionList &identitiesToAssociate,
                                       const ServiceIdentitySessionList &identitiesToRemove
                                       ) = 0;

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
    #pragma mark IServiceLockboxSessionDelegate
    #pragma mark

    interaction IServiceLockboxSessionDelegate
    {
      typedef IServiceLockboxSession::SessionStates SessionStates;

      virtual void onServiceLockboxSessionStateChanged(
                                                       IServiceLockboxSessionPtr session,
                                                       SessionStates state
                                                       ) = 0;
      virtual void onServiceLockboxSessionAssociatedIdentitiesChanged(IServiceLockboxSessionPtr session) = 0;

      virtual void onServiceLockboxSessionPendingMessageForInnerBrowserWindowFrame(IServiceLockboxSessionPtr session) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IServiceLockboxSessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceLockboxSessionPtr, IServiceLockboxSessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceLockboxSessionDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onServiceLockboxSessionStateChanged, IServiceLockboxSessionPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onServiceLockboxSessionAssociatedIdentitiesChanged, IServiceLockboxSessionPtr)
ZS_DECLARE_PROXY_METHOD_1(onServiceLockboxSessionPendingMessageForInnerBrowserWindowFrame, IServiceLockboxSessionPtr)
ZS_DECLARE_PROXY_END()
