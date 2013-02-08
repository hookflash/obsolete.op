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

#include <zsLib/zsTypes.h>
#include <zsLib/Proxy.h>

namespace hookflash
{
  namespace provisioning2
  {
    interaction IAccount;
    typedef boost::shared_ptr<IAccount> IAccountPtr;
    typedef boost::weak_ptr<IAccount> IAccountWeakPtr;

    interaction IAccountDelegate;
    typedef boost::shared_ptr<IAccountDelegate> IAccountDelegatePtr;
    typedef boost::weak_ptr<IAccountDelegate> IAccountDelegateWeakPtr;
    typedef zsLib::Proxy<IAccountDelegate> IAccountDelegateProxy;

    interaction IIdentityLoginSession;
    typedef boost::shared_ptr<IIdentityLoginSession> IIdentityLoginSessionPtr;
    typedef boost::weak_ptr<IIdentityLoginSession> IIdentityLoginSessionWeakPtr;

    interaction IIdentityLoginSessionDelegate;
    typedef boost::shared_ptr<IIdentityLoginSessionDelegate> IIdentityLoginSessionDelegatePtr;
    typedef boost::weak_ptr<IIdentityLoginSessionDelegate> IIdentityLoginSessionDelegateWeakPtr;

    interaction IAccountIdentityLookupQuery;
    typedef boost::shared_ptr<IAccountIdentityLookupQuery> IAccountIdentityLookupQueryPtr;
    typedef boost::weak_ptr<IAccountIdentityLookupQuery> IAccountIdentityLookupQueryWeakPtr;

    interaction IAccountIdentityLookupQueryDelegate;
    typedef boost::shared_ptr<IAccountIdentityLookupQueryDelegate> IAccountIdentityLookupQueryDelegatePtr;
    typedef boost::weak_ptr<IAccountIdentityLookupQueryDelegate> IAccountIdentityLookupQueryDelegateWeakPtr;
    typedef zsLib::Proxy<IAccountIdentityLookupQueryDelegate> IAccountIdentityLookupQueryDelegateProxy;

    interaction IAccountPeerFileLookupQuery;
    typedef boost::shared_ptr<IAccountPeerFileLookupQuery> IAccountPeerFileLookupQueryPtr;
    typedef boost::weak_ptr<IAccountPeerFileLookupQuery> IAccountPeerFileLookupQueryWeakPtr;

    interaction IAccountPeerFileLookupQueryDelegate;
    typedef boost::shared_ptr<IAccountPeerFileLookupQueryDelegate> IAccountPeerFileLookupQueryDelegatePtr;
    typedef boost::weak_ptr<IAccountPeerFileLookupQueryDelegate> IAccountPeerFileLookupQueryDelegateWeakPtr;
    typedef zsLib::Proxy<IAccountPeerFileLookupQueryDelegate> IAccountPeerFileLookupQueryDelegateProxy;
  }
}
