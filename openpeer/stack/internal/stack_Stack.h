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

#include <openpeer/stack/internal/types.h>
#include <openpeer/stack/IStack.h>

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
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

        static AgentInfo agentInfo();

        static IMessageQueuePtr queueDelegate();
        static IMessageQueuePtr queueStack();
        static IMessageQueuePtr queueServices();
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack
      #pragma mark

      class Stack : public IStack,
                    public IStackForInternal
      {
      public:
        friend interaction IStack;
        friend interaction IStackForInternal;

      protected:
        Stack();

      public:
        ~Stack();

        static StackPtr create();
        static StackPtr convert(IStackPtr stack);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => IStack
        #pragma mark

        static StackPtr singleton();

        virtual void setup(
                           IMessageQueuePtr defaultDelegateMessageQueue,
                           IMessageQueuePtr stackMessageQueue,
                           IMessageQueuePtr servicesMessageQueue,
                           const char *appID,       // organization assigned ID for the application e.g. "com.xyz123.app1"
                           const char *appName,     // a branded human readable application name, e.g. "Hookflash"
                           const char *appImageURL, // an HTTPS downloadable branded image for the application
                           const char *appURL,      // an HTTPS URL webpage / website that offers more information about application
                           const char *userAgent,   // e.g. "hookflash/1.0.1001a (iOS/iPad)"
                           const char *deviceID,    // e.g. uuid of device "7bff560b84328f161494eabcba5f8b47a316be8b"
                           const char *os,          // e.g. "iOS 5.0.3
                           const char *system       // e.g. "iPad 2"
                           );

        virtual PUID getID() const {return mID;}

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

        virtual void getAgentInfo(AgentInfo &result) const;

        virtual IMessageQueuePtr queueDelegate() const;
        virtual IMessageQueuePtr queueStack() const;
        virtual IMessageQueuePtr queueServices() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => (internal)
        #pragma mark

        String log(const char *message) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Stack => (data)
        #pragma mark

        PUID mID;
        StackWeakPtr mThisWeak;
        RecursiveLock mLock;

        String mAppID;
        String mAppName;
        String mAppImageURL;
        String mAppURL;
        String mUserAgent;
        String mDeviceID;
        String mOS;
        String mSystem;

        IMessageQueuePtr mStackQueue;
        IMessageQueuePtr mServicesQueue;
        IMessageQueuePtr mDelegateQueue;
      };
    }
  }
}
