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
    #pragma mark IServiceNamespaceGrant
    #pragma mark

    interaction IServiceNamespaceGrant
    {
      static IServiceNamespaceGrantPtr createServiceNamespaceGrantFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

      virtual PUID getID() const = 0;

      virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceNamespaceGrantSession
    #pragma mark

    interaction IServiceNamespaceGrantSession
    {
      enum SessionStates
      {
        SessionState_Pending,
        SessionState_WaitingForAssociationToAllServices,
        SessionState_WaitingForBrowserWindowToBeLoaded,
        SessionState_WaitingForBrowserWindowToBeMadeVisible,
        SessionState_WaitingForBrowserWindowToClose,
        SessionState_Ready,
        SessionState_Shutdown,
      };
      static const char *toString(SessionStates state);

      static String toDebugString(IServiceNamespaceGrantSessionPtr session, bool includeCommaPrefix = true);

      static IServiceNamespaceGrantSessionPtr create(
                                                     IServiceNamespaceGrantSessionDelegatePtr delegate,
                                                     IServiceNamespaceGrantPtr serviceNamespaceGrant,
                                                     const char *outerFrameURLUponReload,
                                                     const char *grantID,
                                                     const char *grantSecret
                                                     );

      virtual PUID getID() const = 0;

      virtual IServiceNamespaceGrantPtr getService() const = 0;

      virtual SessionStates getState(
                                     WORD *lastErrorCode,
                                     String *lastErrorReason
                                     ) const = 0;

      virtual String getGrantID() const = 0;
      virtual String getGrantSecret() const = 0;

      virtual void notifyAssocaitedToAllServicesComplete() = 0;

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
    #pragma mark IServiceNamespaceGrantSessionDelegate
    #pragma mark

    interaction IServiceNamespaceGrantSessionDelegate
    {
      typedef IServiceNamespaceGrantSession::SessionStates SessionStates;

      virtual void onServiceNamespaceGrantSessionStateChanged(
                                                              IServiceNamespaceGrantSessionPtr session,
                                                              SessionStates state
                                                              ) = 0;

      virtual void onServiceNamespaceGrantSessionPendingMessageForInnerBrowserWindowFrame(IServiceNamespaceGrantSessionPtr session) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IServiceNamespaceGrantSessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceNamespaceGrantSessionPtr, IServiceNamespaceGrantSessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceNamespaceGrantSessionDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onServiceNamespaceGrantSessionStateChanged, IServiceNamespaceGrantSessionPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onServiceNamespaceGrantSessionPendingMessageForInnerBrowserWindowFrame, IServiceNamespaceGrantSessionPtr)
ZS_DECLARE_PROXY_END()
