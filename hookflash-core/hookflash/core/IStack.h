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

#include <hookflash/core/types.h>

namespace hookflash
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IStack
    #pragma mark

    interaction IStack
    {
      static IStackPtr singleton();

      //-----------------------------------------------------------------------
      // PURPOSE: Setup the Open Peer stack.
      // NOTE:    Only called once. Must be called once before any other routines
      //          will funciton. Must be called from the GUI thread.
      virtual void setup(
                         IStackDelegatePtr stackDelegate,
                         IMediaEngineDelegatePtr mediaEngineDelegate,
                         const char *deviceID,   // e.g. uuid of device "7bff560b84328f161494eabcba5f8b47a316be8b"
                         const char *userAgent,  // e.g. "hookflash/1.0.1001a (iOS/iPad)"
                         const char *os,         // e.g. "iOS 5.0.3
                         const char *system      // e.g. "iPad 2"
                         ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Shutdown the Open Peer stack.
      // NOTE:    The shutdown process may take some time and the
      //          "IStackDelegate::onStackShutdown" will be notified when it
      //          has completed. After it has completed, the application
      //          can be safely exited.
      virtual void shutdown() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IStackDelegate
    #pragma mark

    interaction IStackDelegate
    {
      virtual void onStackShutdown(IStackAutoCleanupPtr ignoreThisArgument) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IStackMessageQueue
    #pragma mark

    interaction IStackMessageQueue
    {
      static IStackMessageQueuePtr singleton();

      //-----------------------------------------------------------------------
      // PURPOSE: Intercept the processing of event messages from within the
      //          default message queue so they can be processed/executed from
      //          within the context of a custom thread.
      // NOTE:    Can only be called once. Once override, everytime
      //          "IStackMessageQueueDelegate::onStackMessageQueueWakeUpCustomThreadAndProcess"
      //          is called on the delegate the delegate must wake up the main
      //          thread then call "IStackMessageQueue::notifyProcessMessageFromCustomThread"
      //          from the main thread.
      //
      //          MUST be called BEFORE called "IStack::setup"
      virtual void interceptProcessing(IStackMessageQueueDelegatePtr delegate) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notify that a message can be processed from the custom thread.
      // NOTE:    Only call this routine from within the context of running from
      //          the custom thread.
      virtual void notifyProcessMessageFromCustomThread() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IStackMessageQueueDelegate
    #pragma mark

    interaction IStackMessageQueueDelegate
    {
      //-----------------------------------------------------------------------
      // PURPOSE: Notifies that a message needs to be processed from within the
      //          context of the custom thread.
      virtual void onStackMessageQueueWakeUpCustomThreadAndProcess() = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::IStackDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::IStackAutoCleanupPtr, IStackAutoCleanupPtr)
ZS_DECLARE_PROXY_METHOD_1(onStackShutdown, IStackAutoCleanupPtr)
ZS_DECLARE_PROXY_END()
