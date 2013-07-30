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

#include <openpeer/services/internal/types.h>
#include <openpeer/services/internal/services_CanonicalXML.h>
#include <openpeer/services/internal/services_DNS.h>
#include <openpeer/services/internal/services_DNSMonitor.h>
#include <openpeer/services/internal/services_Helper.h>
#include <openpeer/services/internal/services_HTTP.h>
#include <openpeer/services/internal/services_ICESocket.h>
#include <openpeer/services/internal/services_ICESocketSession.h>
#include <openpeer/services/internal/services_MessageLayerSecurityChannel.h>
#include <openpeer/services/internal/services_RSAPrivateKey.h>
#include <openpeer/services/internal/services_RSAPublicKey.h>
#include <openpeer/services/internal/services_RUDPChannel.h>
#include <openpeer/services/internal/services_RUDPChannelStream.h>
#include <openpeer/services/internal/services_RUDPICESocket.h>
#include <openpeer/services/internal/services_RUDPICESocketSession.h>
#include <openpeer/services/internal/services_RUDPListener.h>
#include <openpeer/services/internal/services_RUDPMessaging.h>
#include <openpeer/services/internal/services_STUNDiscovery.h>
#include <openpeer/services/internal/services_STUNRequester.h>
#include <openpeer/services/internal/services_STUNRequesterManager.h>
#include <openpeer/services/internal/services_TransportStream.h>
#include <openpeer/services/internal/services_TURNSocket.h>
