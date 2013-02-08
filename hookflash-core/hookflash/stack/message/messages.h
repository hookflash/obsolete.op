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

#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/MessageRequest.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/message/MessageReply.h>
#include <hookflash/stack/message/MessageNotify.h>

#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/message/IMessageFactory.h>
#include <hookflash/stack/message/IMessageFactoryManager.h>
#include <hookflash/stack/message/MessageFactoryStack.h>

#include <hookflash/stack/message/PeerPublishRequest.h>
#include <hookflash/stack/message/PeerPublishResult.h>
#include <hookflash/stack/message/PeerGetRequest.h>
#include <hookflash/stack/message/PeerGetResult.h>
#include <hookflash/stack/message/PeerDeleteRequest.h>
#include <hookflash/stack/message/PeerDeleteResult.h>
#include <hookflash/stack/message/PeerSubscribeRequest.h>
#include <hookflash/stack/message/PeerSubscribeResult.h>
#include <hookflash/stack/message/PeerPublishNotifyRequest.h>
#include <hookflash/stack/message/PeerPublishNotifyResult.h>

#include <hookflash/stack/message/PeerToFinderSessionCreateRequest.h>
#include <hookflash/stack/message/PeerToFinderSessionCreateResult.h>
#include <hookflash/stack/message/PeerToFinderSessionDeleteRequest.h>
#include <hookflash/stack/message/PeerToFinderSessionDeleteResult.h>
#include <hookflash/stack/message/PeerKeepAliveRequest.h>
#include <hookflash/stack/message/PeerKeepAliveResult.h>

#include <hookflash/stack/message/PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindResult.h>
#include <hookflash/stack/message/PeerToFinderPeerLocationFindReply.h>

#include <hookflash/stack/message/PeerToPeerPeerIdentifyRequest.h>
#include <hookflash/stack/message/PeerToPeerPeerIdentifyResult.h>

#include <hookflash/stack/message/PeerToBootstrapperFindersGetRequest.h>
#include <hookflash/stack/message/PeerToBootstrapperFindersGetResult.h>
#include <hookflash/stack/message/PeerToBootstrapperServicesGetRequest.h>
#include <hookflash/stack/message/PeerToBootstrapperServicesGetResult.h>

#include <hookflash/stack/message/PeerToSaltSignedSaltGetRequest.h>
#include <hookflash/stack/message/PeerToSaltSignedSaltGetResult.h>

