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

#include <hookflash/hookflashTypes.h>
#include <list>

#include <zsLib/Proxy.h>
#include <zsLib/XML.h>

namespace hookflash
{
  interaction IAccount
  {
    typedef zsLib::String String;
    typedef std::list<IConversationThreadPtr> ConversationThreadList;
    typedef zsLib::XML::ElementPtr ElementPtr;

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
      AccountError_StackFailed,
      AccountError_CallTransportFailed,
    };

    static const char *toString(AccountStates state);
    static const char *toString(AccountErrors errorCode);

    virtual void shutdown() = 0;

    virtual AccountStates getState() const = 0;
    virtual AccountErrors getLastError() const = 0;

    virtual IAccountSubscriptionPtr subscribe(IAccountDelegatePtr delegate) = 0;

    virtual IContactPtr getSelfContact() const = 0;
    virtual String getLocationID() const = 0;

    virtual ElementPtr savePrivatePeer() = 0;
    virtual ElementPtr savePublicPeer() = 0;

    virtual void notifyAboutContact(IContactPtr contact) = 0;

    virtual void hintAboutContactLocation(
                                          IContactPtr contact,
                                          const char *locationID
                                          ) = 0;

    virtual IConversationThreadPtr getConversationThreadByID(const char *threadID) const = 0;
    virtual void getConversationThreads(ConversationThreadList &outConversationThreads) const = 0;
  };

  interaction IAccountSubscription
  {
    virtual IAccountPtr getAccount() const = 0;
    virtual void cancel() = 0;
  };

  interaction IAccountDelegate
  {
    typedef IAccount::AccountStates AccountStates;

    virtual void onAccountStateChanged(
                                       IAccountPtr account,
                                       AccountStates state
                                       ) = 0;
  };
}

ZS_DECLARE_PROXY_BEGIN(hookflash::IAccountDelegate)
ZS_DECLARE_PROXY_METHOD_2(onAccountStateChanged, hookflash::IAccountPtr, hookflash::IAccount::AccountStates)
ZS_DECLARE_PROXY_END()
