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

#include <hookflash/hookflashTypes.h>
#include <zsLib/Proxy.h>

namespace hookflash
{
  interaction IStack
  {
    // NOTE: Only one stack object should ever be created.
    static IStackPtr create(
                            IStackDelegatePtr stackDelegate,
                            IMediaEngineDelegatePtr mediaEngineDelegate,
                            IConversationThreadDelegatePtr conversationThreadDelegate,
                            ICallDelegatePtr callDelegate,
                            const char *deviceID,   // e.g. uuid of device "7bff560b84328f161494eabcba5f8b47a316be8b"
                            const char *userAgent,  // e.g. "hookflash/1.0.1001a (iOS/iPad)"
                            const char *os,         // e.g. "iOS 5.0.3
                            const char *system      // e.g. "iPad 2"
                            );

    virtual void startShutdown() = 0;
  };

  interaction IStackDelegate
  {
    virtual void onShutdownReady() = 0;
  };
}

ZS_DECLARE_PROXY_BEGIN(hookflash::IStackDelegate)
ZS_DECLARE_PROXY_METHOD_0(onShutdownReady)
ZS_DECLARE_PROXY_END()
