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

#include <hookflash/internal/hookflashTypes.h>

#include <hookflash/hookflashTypes.h>

namespace hookflash
{
  namespace internal
  {
    class Account;
    typedef boost::shared_ptr<Account> AccountPtr;
    typedef boost::weak_ptr<Account> AccountWeakPtr;

    interaction IAccountForCall;
    typedef boost::shared_ptr<IAccountForCall> IAccountForCallPtr;
    typedef boost::weak_ptr<IAccountForCall> IAccountForCallWeakPtr;

    interaction IAccountForConversationThread;
    typedef boost::shared_ptr<IAccountForConversationThread> IAccountForConversationThreadPtr;
    typedef boost::weak_ptr<IAccountForConversationThread> IAccountForConversationThreadWeakPtr;

    interaction IAccountForContact;
    typedef boost::shared_ptr<IAccountForContact> IAccountForContactPtr;
    typedef boost::weak_ptr<IAccountForContact> IAccountForContactWeakPtr;

    interaction IAccountForProvisioningAccount;
    typedef boost::shared_ptr<IAccountForProvisioningAccount> IAccountForProvisioningAccountPtr;
    typedef boost::weak_ptr<IAccountForProvisioningAccount> IAccountForProvisioningAccountWeakPtr;

    interaction IAccountAsyncDelegate;
    typedef boost::shared_ptr<IAccountAsyncDelegate> IAccountAsyncDelegatePtr;
    typedef boost::weak_ptr<IAccountAsyncDelegate> IAccountAsyncDelegateWeakPtr;
    typedef zsLib::Proxy<IAccountAsyncDelegate> IAccountAsyncDelegateProxy;

    class Call;
    typedef boost::shared_ptr<Call> CallPtr;
    typedef boost::weak_ptr<Call> CallWeakPtr;

    interaction ICallForConversationThread;
    typedef boost::shared_ptr<ICallForConversationThread> ICallForConversationThreadPtr;
    typedef boost::weak_ptr<ICallForConversationThread> ICallForConversationThreadWeakPtr;

    interaction ICallForCallTransport;
    typedef boost::shared_ptr<ICallForCallTransport> ICallForCallTransportPtr;
    typedef boost::weak_ptr<ICallForCallTransport> ICallForCallTransportWeakPtr;

    interaction ICallAsync;
    typedef boost::shared_ptr<ICallAsync> ICallAsyncPtr;
    typedef boost::weak_ptr<ICallAsync> ICallAsyncWeakPtr;
    typedef zsLib::Proxy<ICallAsync> ICallAsyncProxy;

    class CallTransport;
    typedef boost::shared_ptr<CallTransport> CallTransportPtr;
    typedef boost::weak_ptr<CallTransport> CallTransportWeakPtr;

    interaction ICallTransport;
    typedef boost::shared_ptr<ICallTransport> ICallTransportPtr;
    typedef boost::weak_ptr<ICallTransport> ICallTransportWeakPtr;

    interaction ICallTransportDelegate;
    typedef boost::shared_ptr<ICallTransportDelegate> ICallTransportDelegatePtr;
    typedef boost::weak_ptr<ICallTransportDelegate> ICallTransportDelegateWeakPtr;
    typedef zsLib::Proxy<ICallTransportDelegate> ICallTransportDelegateProxy;

    interaction ICallTransportForAccount;
    typedef boost::shared_ptr<ICallTransportForAccount> ICallTransportForAccountPtr;
    typedef boost::weak_ptr<ICallTransportForAccount> ICallTransportForAccountWeakPtr;

    interaction ICallTransportForCall;
    typedef boost::shared_ptr<ICallTransportForCall> ICallTransportForCallPtr;
    typedef boost::weak_ptr<ICallTransportForCall> ICallTransportForCallWeakPtr;

    interaction ICallTransportAsync;
    typedef boost::shared_ptr<ICallTransportAsync> ICallTransportAsyncPtr;
    typedef boost::weak_ptr<ICallTransportAsync> ICallTransportAsyncWeakPtr;
    typedef zsLib::Proxy<ICallTransportAsync> ICallTransportAsyncProxy;

    class Client;
    typedef boost::shared_ptr<Client> ClientPtr;
    typedef boost::weak_ptr<Client> ClientWeakPtr;

    class Contact;
    typedef boost::shared_ptr<Contact> ContactPtr;
    typedef boost::weak_ptr<Contact> ContactWeakPtr;

    interaction IContactForAccount;
    typedef boost::shared_ptr<IContactForAccount> IContactForAccountPtr;
    typedef boost::weak_ptr<IContactForAccount> IContactForAccountWeakPtr;

    interaction IContactForConversationThread;
    typedef boost::shared_ptr<IContactForConversationThread> IContactForConversationThreadPtr;
    typedef boost::weak_ptr<IContactForConversationThread> IContactForConversationThreadWeakPtr;

    interaction IContactForCall;
    typedef boost::shared_ptr<IContactForCall> IContactForCallPtr;
    typedef boost::weak_ptr<IContactForCall> IContactForCallWeakPtr;

    class ConversationThread;
    typedef boost::shared_ptr<ConversationThread> ConversationThreadPtr;
    typedef boost::weak_ptr<ConversationThread> ConversationThreadWeakPtr;

    interaction IConversationThreadAsync;
    typedef boost::shared_ptr<IConversationThreadAsync> IConversationThreadAsyncPtr;
    typedef boost::weak_ptr<IConversationThreadAsync> IConversationThreadAsyncWeakPtr;
    typedef zsLib::Proxy<IConversationThreadAsync> IConversationThreadAsyncProxy;

    interaction IConversationThreadForAccount;
    typedef boost::shared_ptr<IConversationThreadForAccount> IConversationThreadForAccountPtr;
    typedef boost::weak_ptr<IConversationThreadForAccount> IConversationThreadForAccountWeakPtr;

    interaction IConversationThreadHostSlaveBase;
    typedef boost::shared_ptr<IConversationThreadHostSlaveBase> IConversationThreadHostSlaveBasePtr;
    typedef boost::weak_ptr<IConversationThreadHostSlaveBase> IConversationThreadHostSlaveBaseWeakPtr;

    interaction IConversationThreadForHostOrSlave;
    typedef boost::shared_ptr<IConversationThreadForHostOrSlave> IConversationThreadForHostOrSlavePtr;
    typedef boost::weak_ptr<IConversationThreadForHostOrSlave> IConversationThreadForHostOrSlaveWeakPtr;

    interaction IConversationThreadForHost;
    typedef boost::shared_ptr<IConversationThreadForHost> IConversationThreadForHostPtr;
    typedef boost::weak_ptr<IConversationThreadForHost> IConversationThreadForHostWeakPtr;

    interaction IConversationThreadForSlave;
    typedef boost::shared_ptr<IConversationThreadForSlave> IConversationThreadForSlavePtr;
    typedef boost::weak_ptr<IConversationThreadForSlave> IConversationThreadForSlaveWeakPtr;

    interaction IConversationThreadForCall;
    typedef boost::shared_ptr<IConversationThreadForCall> IConversationThreadForCallPtr;
    typedef boost::weak_ptr<IConversationThreadForCall> IConversationThreadForCallWeakPtr;

    class ConversationThreadHost;
    typedef boost::shared_ptr<ConversationThreadHost> ConversationThreadHostPtr;
    typedef boost::weak_ptr<ConversationThreadHost> ConversationThreadHostWeakPtr;

    interaction IConversationThreadHostForConversationThread;
    typedef boost::shared_ptr<IConversationThreadHostForConversationThread> IConversationThreadHostForConversationThreadPtr;
    typedef boost::weak_ptr<IConversationThreadHostForConversationThread> IConversationThreadHostForConversationThreadWeakPtr;

    interaction IConversationThreadHostAsync;
    typedef boost::shared_ptr<IConversationThreadHostAsync> IConversationThreadHostAsyncPtr;
    typedef boost::weak_ptr<ConversationThreadHost> IConversationThreadHostAsyncWeakPtr;
    typedef zsLib::Proxy<IConversationThreadHostAsync> IConversationThreadHostAsyncProxy;

    class ConversationThreadSlave;
    typedef boost::shared_ptr<ConversationThreadSlave> ConversationThreadSlavePtr;
    typedef boost::weak_ptr<ConversationThreadSlave> ConversationThreadSlaveWeakPtr;

    interaction IConversationThreadSlaveForConversationThread;
    typedef boost::shared_ptr<IConversationThreadSlaveForConversationThread> IConversationThreadSlaveForConversationThreadPtr;
    typedef boost::weak_ptr<IConversationThreadSlaveForConversationThread> IConversationThreadSlaveForConversationThreadWeakPtr;

    interaction IConversationThreadSlaveAsync;
    typedef boost::shared_ptr<IConversationThreadSlaveAsync> IConversationThreadSlaveAsyncPtr;
    typedef boost::weak_ptr<IConversationThreadSlaveAsync> IConversationThreadSlaveAsyncWeakPtr;
    typedef zsLib::Proxy<IConversationThreadSlaveAsync> IConversationThreadSlaveAsyncProxy;

    interaction IConversationThreadDocumentFetcher;
    typedef boost::shared_ptr<IConversationThreadDocumentFetcher> IConversationThreadDocumentFetcherPtr;
    typedef boost::weak_ptr<IConversationThreadDocumentFetcher> IConversationThreadDocumentFetcherWeakPtr;

    interaction IConversationThreadDocumentFetcherDelegate;
    typedef boost::shared_ptr<IConversationThreadDocumentFetcherDelegate> IConversationThreadDocumentFetcherDelegatePtr;
    typedef boost::weak_ptr<IConversationThreadDocumentFetcherDelegate> IConversationThreadDocumentFetcherDelegateWeakPtr;
    typedef zsLib::Proxy<IConversationThreadDocumentFetcherDelegate> IConversationThreadDocumentFetcherDelegateProxy;

    interaction ConversationThreadDocumentFetcher;
    typedef boost::shared_ptr<ConversationThreadDocumentFetcher> ConversationThreadDocumentFetcherPtr;
    typedef boost::weak_ptr<ConversationThreadDocumentFetcher> ConversationThreadDocumentFetcherWeakPtr;

    class MediaEngine;
    typedef boost::shared_ptr<MediaEngine> MediaEnginePtr;
    typedef boost::weak_ptr<MediaEngine> MediaEngineWeakPtr;

    interaction IMediaEngineForStack;
    typedef boost::shared_ptr<IMediaEngineForStack> IMediaEngineForStackPtr;
    typedef boost::weak_ptr<IMediaEngineForStack> IMediaEngineForStackWeakPtr;

    interaction IMediaEngineForCallTransport;
    typedef boost::shared_ptr<IMediaEngineForCallTransport> IMediaEngineForCallTransportPtr;
    typedef boost::weak_ptr<IMediaEngineForCallTransport> IMediaEngineForCallTransportWeakPtr;
    
    interaction IMediaEngineForTestApplication;
    typedef boost::shared_ptr<IMediaEngineForTestApplication> IMediaEngineForTestApplicationPtr;
    typedef boost::weak_ptr<IMediaEngineForTestApplication> IMediaEngineForTestApplicationWeakPtr;

    class Settings;
    typedef boost::shared_ptr<Settings> SettingsPtr;
    typedef boost::weak_ptr<Settings> SettingsWeakPtr;

    class Stack;
    typedef boost::shared_ptr<Stack> StackPtr;
    typedef boost::weak_ptr<Stack> StackWeakPtr;

    class VideoViewPort;
    typedef boost::shared_ptr<VideoViewPort> VideoViewPortPtr;
    typedef boost::weak_ptr<VideoViewPort> VideoViewPortWeakPtr;
  }
}
