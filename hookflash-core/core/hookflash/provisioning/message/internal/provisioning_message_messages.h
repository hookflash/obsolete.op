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

#include <hookflash/provisioning/message/internal/hookflashTypes.h>
#include <hookflash/provisioning/message/internal/provisioning_message_LookupProfileRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_LookupProfileResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_CreateAccountRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_CreateAccountResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_AccessAccountRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_AccessAccountResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProfileGetRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProfileGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProfilePutRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProfilePutResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_SendIDValidationPINRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_SendIDValidationPINResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ValidateIDPINRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ValidateIDPINResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProviderLoginURLGetRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProviderLoginURLGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_OAuthLoginWebpageResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProviderAssociateURLGetRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_ProviderAssociateURLGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_OAuthLoginWebpageForAssociationResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PeerProfileLookupRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PeerProfileLookupResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_MultiPartyAPNSPushRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_MultiPartyAPNSPushResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart1Request.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart1Result.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart2Request.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordGetPart2Result.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordPINGetRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordPINGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_OAuthPasswordGetRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_OAuthPasswordGetResult.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordPutRequest.h>
#include <hookflash/provisioning/message/internal/provisioning_message_PasswordPutResult.h>
