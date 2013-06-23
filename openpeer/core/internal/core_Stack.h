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

#include <hookflash/core/IStack.h>
#include <hookflash/core/internal/types.h>

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IStackForShutdownCheckAgain
      #pragma mark

      interaction IStackForShutdownCheckAgain
      {
        IStackForShutdownCheckAgain &forShutdownCheckAgain() {return *this;}
        const IStackForShutdownCheckAgain &forShutdownCheckAgain() const {return *this;}

        virtual void notifyShutdownCheckAgain() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IStackForInternal
      #pragma mark

      interaction IStackForInternal
      {
        IStackForInternal &forInternal() {return *this;}
        const IStackForInternal &forInternal() const {return *this;}

        static const String &appID();
        static const String &appName();
        static const String &appImageURL();
        static const String &appURL();
        static const String &userAgent();
        static const String &deviceID();
        static const String &os();
        static const String &system();

        static IMessageQueuePtr queueApplication();
        static IMessageQueuePtr queueCore();
        static IMessageQueuePtr queueMedia();
        static IMessageQueuePtr queueServices();

        static IMediaEngineDelegatePtr        mediaEngineDelegate();
        static IConversationThreadDelegatePtr conversationThreadDelegate();
        static ICallDelegatePtr               callDelegate();

        static void finalShutdown();
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack
      #pragma mark

      class Stack : public IStack,
                    public IStackForInternal,
                    public IStackForShutdownCheckAgain,
                    public IStackMessageQueue
      {
      public:
        friend interaction IStack;
        friend interaction IStackMessageQueue;
        friend interaction IStackForInternal;
        friend interaction IStackForShutdownCheckAgain;

      protected:
        Stack();

        static StackPtr convert(IStackPtr stack);

        static StackPtr create();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => IStack
        #pragma mark

        static StackPtr singleton();

        virtual void setup(
                           IStackDelegatePtr stackDelegate,
                           IMediaEngineDelegatePtr mediaEngineDelegate,
                           const char *appID,
                           const char *appName,
                           const char *appImageURL,
                           const char *appURL,
                           const char *userAgent,
                           const char *deviceID,
                           const char *os,
                           const char *system
                           );

        virtual void shutdown();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => IStackMessageQueue
        #pragma mark

        // static IStackMessageQueuePtr singleton();

        virtual void interceptProcessing(IStackMessageQueueDelegatePtr delegate);

        virtual void notifyProcessMessageFromCustomThread();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => IStackForShutdownCheckAgain
        #pragma mark

        void notifyShutdownCheckAgain();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => IStackForInternal
        #pragma mark

        virtual const String &getAppID() const {return mAppID;}
        virtual const String &getAppName() const {return mAppName;}
        virtual const String &getAppImageURL() const {return mAppImageURL;}
        virtual const String &getAppURL() const {return mAppURL;}
        virtual const String &getUserAgent() const {return mUserAgent;}
        virtual const String &getDeviceID() const {return mDeviceID;}
        virtual const String &getOS() const {return mOS;}
        virtual const String &getSystem() const {return mSystem;}

        virtual IMessageQueuePtr getQueueApplication() const;
        virtual IMessageQueuePtr getQueueCore() const;
        virtual IMessageQueuePtr getQueueMedia() const;
        virtual IMessageQueuePtr getQueueServices() const;

        virtual IMediaEngineDelegatePtr        getMediaEngineDelegate() const;

        virtual void finalShutdown();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => (internal)
        #pragma mark

        void makeReady();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        PUID mID;
        StackWeakPtr mThisWeak;

        IShutdownCheckAgainDelegatePtr mShutdownCheckAgainDelegate;

        IMessageQueueThreadPtr mApplicationThreadQueue;
        MessageQueueThreadPtr  mCoreThreadQueue;
        MessageQueueThreadPtr  mMediaThreadQueue;
        MessageQueueThreadPtr  mServicesThreadQueue;

        IStackDelegatePtr              mStackDelegate;
        IMediaEngineDelegatePtr        mMediaEngineDelegate;

        IStackMessageQueueDelegatePtr  mStackMessageQueueDelegate;

        String mAppID;
        String mAppName;
        String mAppImageURL;
        String mAppURL;
        String mUserAgent;
        String mDeviceID;
        String mOS;
        String mSystem;
      };
    }
  }
}
