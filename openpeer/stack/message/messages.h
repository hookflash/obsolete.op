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

#include <openpeer/stack/message/Message.h>
#include <openpeer/stack/message/MessageRequest.h>
#include <openpeer/stack/message/MessageResult.h>
#include <openpeer/stack/message/MessageReply.h>
#include <openpeer/stack/message/MessageNotify.h>

#include <openpeer/stack/message/IMessageHelper.h>
#include <openpeer/stack/message/IMessageFactory.h>
#include <openpeer/stack/message/IMessageFactoryManager.h>

#include <openpeer/stack/message/bootstrapped-finder/MessageFactoryBootstrappedFinder.h>
#include <openpeer/stack/message/bootstrapped-finder/FindersGetRequest.h>
#include <openpeer/stack/message/bootstrapped-finder/FindersGetResult.h>

#include <openpeer/stack/message/bootstrapper/MessageFactoryBootstrapper.h>
#include <openpeer/stack/message/bootstrapper/ServicesGetRequest.h>
#include <openpeer/stack/message/bootstrapper/ServicesGetResult.h>

#include <openpeer/stack/message/certificates/MessageFactoryCertificates.h>
#include <openpeer/stack/message/certificates/CertificatesGetRequest.h>
#include <openpeer/stack/message/certificates/CertificatesGetResult.h>

#include <openpeer/stack/message/identity/MessageFactoryIdentity.h>
#include <openpeer/stack/message/identity/IdentityAccessWindowRequest.h>
#include <openpeer/stack/message/identity/IdentityAccessWindowResult.h>
#include <openpeer/stack/message/identity/IdentityAccessStartNotify.h>
#include <openpeer/stack/message/identity/IdentityAccessCompleteNotify.h>
#include <openpeer/stack/message/identity/IdentityAccessLockboxUpdateRequest.h>
#include <openpeer/stack/message/identity/IdentityAccessLockboxUpdateResult.h>
#include <openpeer/stack/message/identity/IdentityAccessRolodexCredentialsGetRequest.h>
#include <openpeer/stack/message/identity/IdentityAccessRolodexCredentialsGetResult.h>
#include <openpeer/stack/message/identity/IdentityLookupUpdateRequest.h>
#include <openpeer/stack/message/identity/IdentityLookupUpdateResult.h>

#include <openpeer/stack/message/identity-lockbox/MessageFactoryIdentityLockbox.h>

#include <openpeer/stack/message/identity-lookup/MessageFactoryIdentityLookup.h>
#include <openpeer/stack/message/identity-lookup/IdentityLookupCheckRequest.h>
#include <openpeer/stack/message/identity-lookup/IdentityLookupCheckResult.h>
#include <openpeer/stack/message/identity-lookup/IdentityLookupRequest.h>
#include <openpeer/stack/message/identity-lookup/IdentityLookupResult.h>

#include <openpeer/stack/message/peer/MessageFactoryPeer.h>

#include <openpeer/stack/message/peer-common/MessageFactoryPeerCommon.h>
#include <openpeer/stack/message/peer-common/PeerDeleteRequest.h>
#include <openpeer/stack/message/peer-common/PeerDeleteResult.h>
#include <openpeer/stack/message/peer-common/PeerGetRequest.h>
#include <openpeer/stack/message/peer-common/PeerGetResult.h>
#include <openpeer/stack/message/peer-common/PeerPublishNotifyRequest.h>
#include <openpeer/stack/message/peer-common/PeerPublishNotifyResult.h>
#include <openpeer/stack/message/peer-common/PeerPublishRequest.h>
#include <openpeer/stack/message/peer-common/PeerPublishResult.h>
#include <openpeer/stack/message/peer-common/PeerSubscribeRequest.h>
#include <openpeer/stack/message/peer-common/PeerSubscribeResult.h>

#include <openpeer/stack/message/identity-lockbox/MessageFactoryIdentityLockbox.h>
#include <openpeer/stack/message/identity-lockbox/LockboxAccessRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxAccessResult.h>
#include <openpeer/stack/message/identity-lockbox/LockboxNamespaceGrantChallengeValidateRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxNamespaceGrantChallengeValidateResult.h>
#include <openpeer/stack/message/identity-lockbox/LockboxIdentitiesUpdateRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxIdentitiesUpdateResult.h>
#include <openpeer/stack/message/identity-lockbox/LockboxContentGetRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxContentGetResult.h>
#include <openpeer/stack/message/identity-lockbox/LockboxContentSetRequest.h>
#include <openpeer/stack/message/identity-lockbox/LockboxContentSetResult.h>

#include <openpeer/stack/message/namespace-grant/MessageFactoryNamespaceGrant.h>
#include <openpeer/stack/message/namespace-grant/NamespaceGrantWindowRequest.h>
#include <openpeer/stack/message/namespace-grant/NamespaceGrantWindowResult.h>
#include <openpeer/stack/message/namespace-grant/NamespaceGrantCompleteNotify.h>

#include <openpeer/stack/message/rolodex/MessageFactoryRolodex.h>
#include <openpeer/stack/message/rolodex/RolodexAccessRequest.h>
#include <openpeer/stack/message/rolodex/RolodexAccessResult.h>
#include <openpeer/stack/message/rolodex/RolodexNamespaceGrantChallengeValidateRequest.h>
#include <openpeer/stack/message/rolodex/RolodexNamespaceGrantChallengeValidateResult.h>
#include <openpeer/stack/message/rolodex/RolodexContactsGetRequest.h>
#include <openpeer/stack/message/rolodex/RolodexContactsGetResult.h>

#include <openpeer/stack/message/peer-finder/MessageFactoryPeerFinder.h>
#include <openpeer/stack/message/peer-finder/PeerLocationFindRequest.h>
#include <openpeer/stack/message/peer-finder/PeerLocationFindResult.h>
#include <openpeer/stack/message/peer-finder/PeerLocationFindReply.h>
#include <openpeer/stack/message/peer-finder/SessionCreateRequest.h>
#include <openpeer/stack/message/peer-finder/SessionCreateResult.h>
#include <openpeer/stack/message/peer-finder/SessionDeleteRequest.h>
#include <openpeer/stack/message/peer-finder/SessionDeleteResult.h>
#include <openpeer/stack/message/peer-finder/SessionKeepAliveRequest.h>
#include <openpeer/stack/message/peer-finder/SessionKeepAliveResult.h>

#include <openpeer/stack/message/peer-salt/MessageFactoryPeerSalt.h>
#include <openpeer/stack/message/peer-salt/SignedSaltGetRequest.h>
#include <openpeer/stack/message/peer-salt/SignedSaltGetResult.h>

#include <openpeer/stack/message/peer-to-peer/MessageFactoryPeerToPeer.h>
#include <openpeer/stack/message/peer-to-peer/PeerIdentifyRequest.h>
#include <openpeer/stack/message/peer-to-peer/PeerIdentifyResult.h>
#include <openpeer/stack/message/peer-to-peer/PeerKeepAliveRequest.h>
#include <openpeer/stack/message/peer-to-peer/PeerKeepAliveResult.h>
