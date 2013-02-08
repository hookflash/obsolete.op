/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/stack/hookflashTypes.h>
#include <hookflash/stack/message/hookflashTypes.h>

#include <hookflash/services/IICESocket.h>
#include <zsLib/Proxy.h>

#include <zsLib/String.h>

namespace hookflash
{
  namespace stack
  {
    interaction IAccount
    {
      typedef zsLib::Time Time;
      typedef zsLib::String String;
      typedef zsLib::IMessageQueuePtr IMessageQueuePtr;

      enum AccountStates
      {
        AccountState_Pending,
        AccountState_Ready,
        AccountState_ShuttingDown,
        AccountState_Shutdown,
      };

      enum AccountErrors
      {
        AccountError_None,

        AccountError_InternalError,

        AccountError_BootstrappedNetworkFailed,
        AccountError_SocketUnexpectedlyClosed,
      };

      static const char *toString(AccountStates state);
      static const char *toString(AccountErrors state);

      static Time getAdjustedTime();

      static IAccountPtr create(
                                IMessageQueuePtr queue,
                                IBootstrappedNetworkPtr network,
                                IAccountDelegatePtr delegate,
                                IPeerFilesPtr peerFiles,
                                const char *password,
                                const char *deviceID,
                                const char *userAgent,
                                const char *os,
                                const char *system
                                );

      virtual AccountStates getState() const = 0;
      virtual AccountErrors getLastError() const = 0;

      virtual IPublicationRepositoryPtr getRepository() const = 0;

      virtual const String &getLocationID() const = 0;

      virtual bool sendFinderMessage(message::MessagePtr message) = 0;

      virtual IConnectionSubscriptionPtr subscribeToAllConnections(IConnectionSubscriptionDelegatePtr delegate) = 0;

      virtual IPeerSubscriptionPtr subscribePeerLocations(
                                                          IPeerFilePublicPtr remotePartyPublicPeerFile,
                                                          IPeerSubscriptionDelegatePtr delegate
                                                          ) = 0;

      virtual IPeerLocationPtr getPeerLocation(
                                               const char *contactID,
                                               const char *locationID
                                               ) const = 0;

      virtual void hintAboutNewPeerLocation(
                                            const char *contactID,
                                            const char *locationID
                                            ) = 0;

      virtual void shutdown() = 0;
    };

    interaction IAccountDelegate
    {
      typedef stack::IAccountPtr AccountPtr;
      typedef IAccount::AccountStates AccountStates;

      virtual void onAccountStateChanged(
                                         IAccountPtr account,
                                         AccountStates state
                                         ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IAccountDelegate)
ZS_DECLARE_PROXY_METHOD_2(onAccountStateChanged, hookflash::stack::IAccountPtr, hookflash::stack::IAccount::AccountStates)
ZS_DECLARE_PROXY_END()
