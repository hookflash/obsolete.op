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

#include <hookflash/provisioning/message/hookflashTypes.h>

#include <hookflash/provisioning/message/MessageFactoryProvisioning.h>
#include <hookflash/provisioning/message/LookupProfileRequest.h>
#include <hookflash/provisioning/message/LookupProfileResult.h>
#include <hookflash/provisioning/message/CreateAccountRequest.h>
#include <hookflash/provisioning/message/CreateAccountResult.h>
#include <hookflash/provisioning/message/AccessAccountRequest.h>
#include <hookflash/provisioning/message/AccessAccountResult.h>
#include <hookflash/provisioning/message/ProfileGetRequest.h>
#include <hookflash/provisioning/message/ProfileGetResult.h>
#include <hookflash/provisioning/message/ProfilePutRequest.h>
#include <hookflash/provisioning/message/ProfilePutResult.h>
#include <hookflash/provisioning/message/SendIDValidationPINRequest.h>
#include <hookflash/provisioning/message/SendIDValidationPINResult.h>
#include <hookflash/provisioning/message/ValidateIDPINRequest.h>
#include <hookflash/provisioning/message/ValidateIDPINResult.h>
#include <hookflash/provisioning/message/ProviderLoginURLGetRequest.h>
#include <hookflash/provisioning/message/ProviderLoginURLGetResult.h>
#include <hookflash/provisioning/message/OAuthLoginWebpageResult.h>
#include <hookflash/provisioning/message/ProviderAssociateURLGetRequest.h>
#include <hookflash/provisioning/message/ProviderAssociateURLGetResult.h>
#include <hookflash/provisioning/message/OAuthLoginWebpageForAssociationResult.h>
#include <hookflash/provisioning/message/PeerProfileLookupRequest.h>
#include <hookflash/provisioning/message/PeerProfileLookupResult.h>
#include <hookflash/provisioning/message/MultiPartyAPNSPushRequest.h>
#include <hookflash/provisioning/message/MultiPartyAPNSPushResult.h>
#include <hookflash/provisioning/message/PasswordGetPart1Request.h>
#include <hookflash/provisioning/message/PasswordGetPart1Result.h>
#include <hookflash/provisioning/message/PasswordGetPart2Request.h>
#include <hookflash/provisioning/message/PasswordGetPart2Result.h>
#include <hookflash/provisioning/message/PasswordPINGetRequest.h>
#include <hookflash/provisioning/message/PasswordPINGetResult.h>
#include <hookflash/provisioning/message/OAuthPasswordGetRequest.h>
#include <hookflash/provisioning/message/OAuthPasswordGetResult.h>
#include <hookflash/provisioning/message/PasswordPutRequest.h>
#include <hookflash/provisioning/message/PasswordPutResult.h>
