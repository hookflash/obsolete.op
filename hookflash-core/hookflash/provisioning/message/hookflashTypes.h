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
#include <hookflash/provisioning/IAccount.h>

#include <zsLib/zsTypes.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      class MessageFactoryProvisioning;
      typedef boost::shared_ptr<MessageFactoryProvisioning> MessageFactoryProvisioningPtr;
      typedef boost::weak_ptr<MessageFactoryProvisioning> MessageFactoryProvisioningWeakPtr;

      class LookupProfileRequest;
      typedef boost::shared_ptr<LookupProfileRequest> LookupProfileRequestPtr;
      typedef boost::weak_ptr<LookupProfileRequest> LookupProfileRequestWeakPtr;

      class LookupProfileResult;
      typedef boost::shared_ptr<LookupProfileResult> LookupProfileResultPtr;
      typedef boost::weak_ptr<LookupProfileResult> LookupProfileResultWeakPtr;

      class CreateAccountRequest;
      typedef boost::shared_ptr<CreateAccountRequest> CreateAccountRequestPtr;
      typedef boost::weak_ptr<CreateAccountRequest> CreateAccountRequestWeakPtr;

      class CreateAccountResult;
      typedef boost::shared_ptr<CreateAccountResult> CreateAccountResultPtr;
      typedef boost::weak_ptr<CreateAccountResult> CreateAccountResultWeakPtr;

      class AccessAccountRequest;
      typedef boost::shared_ptr<AccessAccountRequest> AccessAccountRequestPtr;
      typedef boost::weak_ptr<AccessAccountRequest> AccessAccountRequestWeakPtr;

      class AccessAccountResult;
      typedef boost::shared_ptr<AccessAccountResult> AccessAccountResultPtr;
      typedef boost::weak_ptr<AccessAccountResult> AccessAccountResultWeakPtr;

      class ProfileGetRequest;
      typedef boost::shared_ptr<ProfileGetRequest> ProfileGetRequestPtr;
      typedef boost::weak_ptr<ProfileGetRequest> ProfileGetRequestWeakPtr;

      class ProfileGetResult;
      typedef boost::shared_ptr<ProfileGetResult> ProfileGetResultPtr;
      typedef boost::weak_ptr<ProfileGetResult> ProfileGetResultWeakPtr;

      class ProfilePutRequest;
      typedef boost::shared_ptr<ProfilePutRequest> ProfilePutRequestPtr;
      typedef boost::weak_ptr<ProfilePutRequest> ProfilePutRequestWeakPtr;

      class ProfilePutResult;
      typedef boost::shared_ptr<ProfilePutResult> ProfilePutResultPtr;
      typedef boost::weak_ptr<ProfilePutResult> ProfilePutResultWeakPtr;

      class SendIDValidationPINRequest;
      typedef boost::shared_ptr<SendIDValidationPINRequest> SendIDValidationPINRequestPtr;
      typedef boost::weak_ptr<SendIDValidationPINRequest> SendIDValidationPINRequestWeakPtr;

      class SendIDValidationPINResult;
      typedef boost::shared_ptr<SendIDValidationPINResult> SendIDValidationPINResultPtr;
      typedef boost::weak_ptr<SendIDValidationPINResult> SendIDValidationPINResultWeakPtr;

      class ValidateIDPINRequest;
      typedef boost::shared_ptr<ValidateIDPINRequest> ValidateIDPINRequestPtr;
      typedef boost::weak_ptr<ValidateIDPINRequest> ValidateIDPINRequestWeakPtr;

      class ValidateIDPINResult;
      typedef boost::shared_ptr<ValidateIDPINResult> ValidateIDPINResultPtr;
      typedef boost::weak_ptr<ValidateIDPINResult> ValidateIDPINResultWeakPtr;

      class ProviderLoginURLGetRequest;
      typedef boost::shared_ptr<ProviderLoginURLGetRequest> ProviderLoginURLGetRequestPtr;
      typedef boost::weak_ptr<ProviderLoginURLGetRequest> ProviderLoginURLGetRequestWeakPtr;

      class ProviderLoginURLGetResult;
      typedef boost::shared_ptr<ProviderLoginURLGetResult> ProviderLoginURLGetResultPtr;
      typedef boost::weak_ptr<ProviderLoginURLGetResult> ProviderLoginURLGetResultWeakPtr;

      class OAuthLoginWebpageResult;
      typedef boost::shared_ptr<OAuthLoginWebpageResult> OAuthLoginWebpageResultPtr;
      typedef boost::weak_ptr<OAuthLoginWebpageResult> OAuthLoginWebpageResultWeakPtr;

      class ProviderAssociateURLGetRequest;
      typedef boost::shared_ptr<ProviderAssociateURLGetRequest> ProviderAssociateURLGetRequestPtr;
      typedef boost::weak_ptr<ProviderAssociateURLGetRequest> ProviderAssociateURLGetRequestWeakPtr;

      class ProviderAssociateURLGetResult;
      typedef boost::shared_ptr<ProviderAssociateURLGetResult> ProviderAssociateURLGetResultPtr;
      typedef boost::weak_ptr<ProviderAssociateURLGetResult> ProviderAssociateURLGetResultWeakPtr;

      class OAuthLoginWebpageForAssociationResult;
      typedef boost::shared_ptr<OAuthLoginWebpageForAssociationResult> OAuthLoginWebpageForAssociationResultPtr;
      typedef boost::weak_ptr<OAuthLoginWebpageForAssociationResult> OAuthLoginWebpageForAssociationResultWeakPtr;

      class PeerProfileLookupRequest;
      typedef boost::shared_ptr<PeerProfileLookupRequest> PeerProfileLookupRequestPtr;
      typedef boost::weak_ptr<PeerProfileLookupRequest> PeerProfileLookupRequestWeakPtr;

      class PeerProfileLookupResult;
      typedef boost::shared_ptr<PeerProfileLookupResult> PeerProfileLookupResultPtr;
      typedef boost::weak_ptr<PeerProfileLookupResult> PeerProfileLookupResultWeakPtr;

      class MultiPartyAPNSPushRequest;
      typedef boost::shared_ptr<MultiPartyAPNSPushRequest> MultiPartyAPNSPushRequestPtr;
      typedef boost::weak_ptr<MultiPartyAPNSPushRequest> MultiPartyAPNSPushRequestWeakPtr;

      class MultiPartyAPNSPushResult;
      typedef boost::shared_ptr<MultiPartyAPNSPushResult> MultiPartyAPNSPushResultPtr;
      typedef boost::weak_ptr<MultiPartyAPNSPushResult> MultiPartyAPNSPushResultWeakPtr;

      class PasswordGetPart1Request;
      typedef boost::shared_ptr<PasswordGetPart1Request> PasswordGetPart1RequestPtr;
      typedef boost::weak_ptr<PasswordGetPart1Request> PasswordGetPart1RequestWeakPtr;

      class PasswordGetPart1Result;
      typedef boost::shared_ptr<PasswordGetPart1Result> PasswordGetPart1ResultPtr;
      typedef boost::weak_ptr<PasswordGetPart1Result> PasswordGetPart1ResultWeakPtr;

      class PasswordGetPart2Request;
      typedef boost::shared_ptr<PasswordGetPart2Request> PasswordGetPart2RequestPtr;
      typedef boost::weak_ptr<PasswordGetPart2Request> PasswordGetPart2RequestWeakPtr;

      class PasswordGetPart2Result;
      typedef boost::shared_ptr<PasswordGetPart2Result> PasswordGetPart2ResultPtr;
      typedef boost::weak_ptr<PasswordGetPart2Result> PasswordGetPart2ResultWeakPtr;

      class PasswordPINGetRequest;
      typedef boost::shared_ptr<PasswordPINGetRequest> PasswordPINGetRequestPtr;
      typedef boost::weak_ptr<PasswordPINGetRequest> PasswordPINGetRequestWeakPtr;

      class PasswordPINGetResult;
      typedef boost::shared_ptr<PasswordPINGetResult> PasswordPINGetResultPtr;
      typedef boost::weak_ptr<PasswordPINGetResult> PasswordPINGetResultWeakPtr;

      class OAuthPasswordGetRequest;
      typedef boost::shared_ptr<OAuthPasswordGetRequest> OAuthPasswordGetRequestPtr;
      typedef boost::weak_ptr<OAuthPasswordGetRequest> OAuthPasswordGetRequestWeakPtr;

      class OAuthPasswordGetResult;
      typedef boost::shared_ptr<OAuthPasswordGetResult> OAuthPasswordGetResultPtr;
      typedef boost::weak_ptr<OAuthPasswordGetResult> OAuthPasswordGetResultWeakPtr;

      class PasswordPutRequest;
      typedef boost::shared_ptr<PasswordPutRequest> PasswordPutRequestPtr;
      typedef boost::weak_ptr<PasswordPutRequest> PasswordPutRequestWeakPtr;

      class PasswordPutResult;
      typedef boost::shared_ptr<PasswordPutResult> PasswordPutResultPtr;
      typedef boost::weak_ptr<PasswordPutResult> PasswordPutResultWeakPtr;
    }
  }
}
