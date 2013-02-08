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

#include <hookflash/provisioning/hookflashTypes.h>

#include <zsLib/zsTypes.h>
#include <zsLib/Proxy.h>

#include <boost/shared_array.hpp>

namespace hookflash
{
  //interaction declaration
  interaction IAccount;
  typedef boost::shared_ptr<IAccount> IAccountPtr;
  typedef boost::weak_ptr<IAccount> IAccountWeakPtr;

  interaction IAccountSubscription;
  typedef boost::shared_ptr<IAccountSubscription> IAccountSubscriptionPtr;
  typedef boost::weak_ptr<IAccountSubscription> IAccountSubscriptionWeakPtr;

  interaction IAccountDelegate;
  typedef boost::shared_ptr<IAccountDelegate> IAccountDelegatePtr;
  typedef boost::weak_ptr<IAccountDelegate> IAccountDelegateWeakPtr;
  typedef zsLib::Proxy<IAccountDelegate> IAccountDelegateProxy;

  interaction ICall;
  typedef boost::shared_ptr<ICall> ICallPtr;
  typedef boost::weak_ptr<ICall> ICallWeakPtr;

  interaction ICallDelegate;
  typedef boost::shared_ptr<ICallDelegate> ICallDelegatePtr;
  typedef boost::weak_ptr<ICallDelegate> ICallDelegateWeakPtr;
  typedef zsLib::Proxy<ICallDelegate> ICallDelegateProxy;

  interaction IClient;
  typedef boost::shared_ptr<IClient> IClientPtr;
  typedef boost::weak_ptr<IClient> IClientWeakPtr;

  interaction IClientDelegate;
  typedef boost::shared_ptr<IClientDelegate> IClientDelegatePtr;
  typedef boost::weak_ptr<IClientDelegate> IClientDelegateWeakPtr;

  interaction IClientLogDelegate;
  typedef boost::shared_ptr<IClientLogDelegate> IClientLogDelegatePtr;
  typedef boost::weak_ptr<IClientLogDelegate> IClientLogDelegateWeakPtr;

  interaction IConversationThread;
  typedef boost::shared_ptr<IConversationThread> IConversationThreadPtr;
  typedef boost::weak_ptr<IConversationThread> IConversationThreadWeakPtr;

  interaction IConversationThreadDelegate;
  typedef boost::shared_ptr<IConversationThreadDelegate> IConversationThreadDelegatePtr;
  typedef boost::weak_ptr<IConversationThreadDelegate> IConversationThreadDelegateWeakPtr;
  typedef zsLib::Proxy<IConversationThreadDelegate> IConversationThreadDelegateProxy;

  interaction IContact;
  typedef boost::shared_ptr<IContact> IContactPtr;
  typedef boost::weak_ptr<IContact> IContactWeakPtr;
  
  interaction IMediaEngine;
  typedef boost::shared_ptr<IMediaEngine> IMediaEnginePtr;
  typedef boost::weak_ptr<IMediaEngine> IMediaEngineWeakPtr;
  
  interaction IMediaEngineDelegate;
  typedef boost::shared_ptr<IMediaEngineDelegate> IMediaEngineDelegatePtr;
  typedef boost::weak_ptr<IMediaEngineDelegate> IMediaEngineDelegateWeakPtr;
  typedef zsLib::Proxy<IMediaEngineDelegate> IMediaEngineDelegateProxy;

  interaction ISocialSignin;
  typedef boost::shared_ptr<ISocialSignin> ISocialSigninPtr;
  typedef boost::weak_ptr<ISocialSignin> ISocialSigninWeakPtr;

  interaction IStack;
  typedef boost::shared_ptr<IStack> IStackPtr;
  typedef boost::weak_ptr<IStack> IStackWeakPtr;

  interaction IStackDelegate;
  typedef boost::shared_ptr<IStackDelegate> IStackDelegatePtr;
  typedef boost::weak_ptr<IStackDelegate> IStackDelegateWeakPtr;
  typedef zsLib::Proxy<IStackDelegate> IStackDelegateProxy;

}
