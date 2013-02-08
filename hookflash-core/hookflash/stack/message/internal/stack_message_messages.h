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

#include <hookflash/stack/message/internal/hookflashTypes.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/internal/stack_message_MessageFactoryManager.h>

#include <hookflash/stack/message/internal/stack_message_PeerPublishRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerGetResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerDeleteRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerDeleteResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerSubscribeRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerSubscribeResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishNotifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishNotifyResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionCreateRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionCreateResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionDeleteRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionDeleteResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerKeepAliveRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerKeepAliveResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindReply.h>

#include <hookflash/stack/message/internal/stack_message_PeerToPeerPeerIdentifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToPeerPeerIdentifyResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperFindersGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperFindersGetResult.h>
#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperServicesGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToBootstrapperServicesGetResult.h>

#include <hookflash/stack/message/internal/stack_message_PeerToSaltSignedSaltGetRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerToSaltSignedSaltGetResult.h>
