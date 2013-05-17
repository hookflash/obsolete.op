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

#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/MessageRequest.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/message/MessageReply.h>
#include <hookflash/stack/message/MessageNotify.h>

#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/message/IMessageFactory.h>
#include <hookflash/stack/message/IMessageFactoryManager.h>

#include <hookflash/stack/message/bootstrapped-finder/MessageFactoryBootstrappedFinder.h>
#include <hookflash/stack/message/bootstrapped-finder/FindersGetRequest.h>
#include <hookflash/stack/message/bootstrapped-finder/FindersGetResult.h>

#include <hookflash/stack/message/bootstrapper/MessageFactoryBootstrapper.h>
#include <hookflash/stack/message/bootstrapper/ServicesGetRequest.h>
#include <hookflash/stack/message/bootstrapper/ServicesGetResult.h>

#include <hookflash/stack/message/certificates/MessageFactoryCertificates.h>
#include <hookflash/stack/message/certificates/CertificatesGetRequest.h>
#include <hookflash/stack/message/certificates/CertificatesGetResult.h>

#include <hookflash/stack/message/identity/MessageFactoryIdentity.h>
#include <hookflash/stack/message/identity/IdentityAccessWindowRequest.h>
#include <hookflash/stack/message/identity/IdentityAccessWindowResult.h>
#include <hookflash/stack/message/identity/IdentityAccessStartNotify.h>
#include <hookflash/stack/message/identity/IdentityAccessCompleteNotify.h>
#include <hookflash/stack/message/identity/IdentityAccessLockboxUpdateRequest.h>
#include <hookflash/stack/message/identity/IdentityAccessLockboxUpdateResult.h>
#include <hookflash/stack/message/identity/IdentityLookupUpdateRequest.h>
#include <hookflash/stack/message/identity/IdentityLookupUpdateResult.h>
#include <hookflash/stack/message/identity/IdentitySignRequest.h>
#include <hookflash/stack/message/identity/IdentitySignResult.h>

#include <hookflash/stack/message/identity-lockbox/MessageFactoryIdentityLockbox.h>

#include <hookflash/stack/message/identity-lookup/MessageFactoryIdentityLookup.h>
#include <hookflash/stack/message/identity-lookup/IdentityLookupCheckRequest.h>
#include <hookflash/stack/message/identity-lookup/IdentityLookupCheckResult.h>
#include <hookflash/stack/message/identity-lookup/IdentityLookupRequest.h>
#include <hookflash/stack/message/identity-lookup/IdentityLookupResult.h>

#include <hookflash/stack/message/peer/MessageFactoryPeer.h>

#include <hookflash/stack/message/peer-common/MessageFactoryPeerCommon.h>
#include <hookflash/stack/message/peer-common/PeerDeleteRequest.h>
#include <hookflash/stack/message/peer-common/PeerDeleteResult.h>
#include <hookflash/stack/message/peer-common/PeerGetRequest.h>
#include <hookflash/stack/message/peer-common/PeerGetResult.h>
#include <hookflash/stack/message/peer-common/PeerPublishNotifyRequest.h>
#include <hookflash/stack/message/peer-common/PeerPublishNotifyResult.h>
#include <hookflash/stack/message/peer-common/PeerPublishRequest.h>
#include <hookflash/stack/message/peer-common/PeerPublishResult.h>
#include <hookflash/stack/message/peer-common/PeerSubscribeRequest.h>
#include <hookflash/stack/message/peer-common/PeerSubscribeResult.h>

#include <hookflash/stack/message/identity-lockbox/MessageFactoryIdentityLockbox.h>
#include <hookflash/stack/message/identity-lockbox/LockboxAccessRequest.h>
#include <hookflash/stack/message/identity-lockbox/LockboxAccessResult.h>
#include <hookflash/stack/message/identity-lockbox/LockboxIdentitiesUpdateRequest.h>
#include <hookflash/stack/message/identity-lockbox/LockboxIdentitiesUpdateResult.h>
#include <hookflash/stack/message/identity-lockbox/LockboxNamespaceGrantWindowRequest.h>
#include <hookflash/stack/message/identity-lockbox/LockboxNamespaceGrantWindowResult.h>
#include <hookflash/stack/message/identity-lockbox/LockboxNamespaceGrantCompleteNotify.h>
#include <hookflash/stack/message/identity-lockbox/LockboxContentGetRequest.h>
#include <hookflash/stack/message/identity-lockbox/LockboxContentGetResult.h>
#include <hookflash/stack/message/identity-lockbox/LockboxContentSetRequest.h>
#include <hookflash/stack/message/identity-lockbox/LockboxContentSetResult.h>
#include <hookflash/stack/message/identity-lockbox/LockboxAdminWindowRequest.h>
#include <hookflash/stack/message/identity-lockbox/LockboxAdminWindowResult.h>
#include <hookflash/stack/message/identity-lockbox/LockboxAdminStartNotify.h>
#include <hookflash/stack/message/identity-lockbox/LockboxAdminCompleteNotify.h>

#include <hookflash/stack/message/peer-finder/MessageFactoryPeerFinder.h>
#include <hookflash/stack/message/peer-finder/PeerLocationFindRequest.h>
#include <hookflash/stack/message/peer-finder/PeerLocationFindResult.h>
#include <hookflash/stack/message/peer-finder/PeerLocationFindReply.h>
#include <hookflash/stack/message/peer-finder/SessionCreateRequest.h>
#include <hookflash/stack/message/peer-finder/SessionCreateResult.h>
#include <hookflash/stack/message/peer-finder/SessionDeleteRequest.h>
#include <hookflash/stack/message/peer-finder/SessionDeleteResult.h>
#include <hookflash/stack/message/peer-finder/SessionKeepAliveRequest.h>
#include <hookflash/stack/message/peer-finder/SessionKeepAliveResult.h>

#include <hookflash/stack/message/peer-salt/MessageFactoryPeerSalt.h>
#include <hookflash/stack/message/peer-salt/SignedSaltGetRequest.h>
#include <hookflash/stack/message/peer-salt/SignedSaltGetResult.h>

#include <hookflash/stack/message/peer-to-peer/MessageFactoryPeerToPeer.h>
#include <hookflash/stack/message/peer-to-peer/PeerIdentifyRequest.h>
#include <hookflash/stack/message/peer-to-peer/PeerIdentifyResult.h>
#include <hookflash/stack/message/peer-to-peer/PeerKeepAliveRequest.h>
#include <hookflash/stack/message/peer-to-peer/PeerKeepAliveResult.h>
