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
    #pragma mark IServicePeerContact
    #pragma mark

    interaction IServicePeerContact
    {
      static IServicePeerContactPtr createServicePeerContactFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

      virtual PUID getID() const = 0;

      virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServicePeerContactSession
    #pragma mark

    interaction IServicePeerContactSession
    {
      enum SessionStates
      {
        SessionState_Pending,
        SessionState_Ready,
        SessionState_Shutdown,
      };
      static const char *toString(SessionStates state);

      static String toDebugString(IServicePeerContactSessionPtr session, bool includeCommaPrefix = true);

      static IServicePeerContactSessionPtr login(
                                                 IServicePeerContactSessionDelegatePtr delegate,
                                                 IServicePeerContactPtr servicePeerContact,
                                                 IServiceIdentitySessionPtr identitySession
                                                 );
      static IServicePeerContactSessionPtr relogin(
                                                   IServicePeerContactSessionDelegatePtr delegate,
                                                   IPeerFilesPtr existingPeerFiles
                                                   );

      virtual PUID getID() const = 0;

      virtual IServicePeerContactPtr getService() const = 0;

      virtual SessionStates getState(
                                     WORD *lastErrorCode,
                                     String *lastErrorReason
                                     ) const = 0;

      virtual IPeerFilesPtr getPeerFiles() const = 0;
      virtual String getContactUserID() const = 0;

      virtual ServiceIdentitySessionListPtr getAssociatedIdentities() const = 0;
      virtual void associateIdentities(
                                       const ServiceIdentitySessionList &identitiesToAssociate,
                                       const ServiceIdentitySessionList &identitiesToRemove
                                       ) = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServicePeerContactSessionDelegate
    #pragma mark

    interaction IServicePeerContactSessionDelegate
    {
      typedef IServicePeerContactSession::SessionStates SessionStates;

      virtual void onServicePeerContactSessionStateChanged(
                                                           IServicePeerContactSessionPtr session,
                                                           SessionStates state
                                                           ) = 0;
      virtual void onServicePeerContactSessionAssociatedIdentitiesChanged(IServicePeerContactSessionPtr session) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IServicePeerContactSessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServicePeerContactSessionPtr, IServicePeerContactSessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServicePeerContactSessionDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onServicePeerContactSessionStateChanged, IServicePeerContactSessionPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onServicePeerContactSessionAssociatedIdentitiesChanged, IServicePeerContactSessionPtr)
ZS_DECLARE_PROXY_END()
