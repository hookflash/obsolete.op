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
#include <zsLib/Proxy.h>


namespace hookflash
{
  namespace stack
  {
    interaction IBootstrappedNetwork
    {
      enum BootstrappedNetworkStates
      {
        BootstrappedNetworkState_Pending,
        BootstrappedNetworkState_Ready,
        BootstrappedNetworkState_ShuttingDown,
        BootstrappedNetworkState_Shutdown,
      };

      enum BootstrappedNetworkErrors
      {
        BootstrappedNetworkError_None,

        BootstrappedNetworkError_InternalError,

        BootstrappedNetworkError_HTTPRequestTimeout,
        BootstrappedNetworkError_HTTPFailure,
        BootstrappedNetworkError_HTTPResponseNotUnderstood,

        BootstrappedNetworkError_PeerFinderDNSLookupFailure,
      };

      static const char *toString(BootstrappedNetworkStates state);
      static const char *toString(BootstrappedNetworkErrors errorCode);

      static IBootstrappedNetworkPtr create(
                                            zsLib::IMessageQueuePtr queue,
                                            IBootstrappedNetworkDelegatePtr delegate,
                                            const char *userAgent,
                                            const char *networkURI,
                                            const char *turnServer,
                                            const char *turnServerUsername,
                                            const char *turnServerPassword,
                                            const char *stunServer
                                            );

      virtual BootstrappedNetworkStates getState() const = 0;
      virtual BootstrappedNetworkErrors getLastError() const = 0;

      virtual IBootstrappedNetworkSubscriptionPtr subscribe(IBootstrappedNetworkDelegatePtr delegate) = 0;

      // NOTE:  Caller does not have to wait until the class is "ready" before
      //        calling this method.
      virtual IBootstrappedNetworkFetchSignedSaltQueryPtr fetchSignedSalt(
                                                                          IBootstrappedNetworkFetchSignedSaltQueryDelegatePtr delegate,
                                                                          zsLib::UINT totalToFetch = 1
                                                                          ) = 0;

      virtual void close() = 0;
    };

    interaction IBootstrappedNetworkDelegate
    {
      typedef stack::IBootstrappedNetworkPtr IBootstrappedNetworkPtr;
      typedef IBootstrappedNetwork::BootstrappedNetworkStates BootstrappedNetworkStates;

      virtual void onBootstrappedNetworkStateChanged(
                                                     IBootstrappedNetworkPtr bootstrapper,
                                                     BootstrappedNetworkStates state
                                                     ) = 0;
    };

    interaction IBootstrappedNetworkSubscription
    {
      virtual void cancel() = 0;
    };

    interaction IBootstrappedNetworkFetchSignedSaltQuery
    {
      typedef zsLib::UINT UINT;
      typedef zsLib::XML::ElementPtr ElementPtr;

      virtual bool isComplete() const = 0;
      virtual UINT getTotalSignedSaltsAvailable() const = 0;
      virtual ElementPtr getNextSignedSalt() = 0;

      virtual void cancel() = 0;
    };

    interaction IBootstrappedNetworkFetchSignedSaltQueryDelegate
    {
      typedef stack::IBootstrappedNetworkPtr IBootstrappedNetworkPtr;
      typedef stack::IBootstrappedNetworkFetchSignedSaltQueryPtr IBootstrappedNetworkFetchSignedSaltQueryPtr;

      virtual void onBootstrappedNetworkFetchSignedSaltComplete(
                                                                IBootstrappedNetworkPtr bootstrapper,
                                                                IBootstrappedNetworkFetchSignedSaltQueryPtr query
                                                                ) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IBootstrappedNetworkDelegate)
ZS_DECLARE_PROXY_METHOD_2(onBootstrappedNetworkStateChanged, hookflash::stack::IBootstrappedNetworkPtr, hookflash::stack::IBootstrappedNetwork::BootstrappedNetworkStates)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IBootstrappedNetworkFetchSignedSaltQueryDelegate)
ZS_DECLARE_PROXY_METHOD_2(onBootstrappedNetworkFetchSignedSaltComplete, hookflash::stack::IBootstrappedNetworkPtr, hookflash::stack::IBootstrappedNetworkFetchSignedSaltQueryPtr)
ZS_DECLARE_PROXY_END()
