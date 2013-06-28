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
#include <openpeer/services/ISTUNRequester.h>
#include <openpeer/services/STUNPacket.h>
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>
#include <zsLib/Proxy.h>

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISTUNRequesterForSTUNRequesterManager
      #pragma mark

      interaction ISTUNRequesterForSTUNRequesterManager
      {
        ISTUNRequesterForSTUNRequesterManager &forManager() {return *this;}
        const ISTUNRequesterForSTUNRequesterManager &forManager() const {return *this;}

        virtual bool handleSTUNPacket(
                                      IPAddress fromIPAddress,
                                      STUNPacketPtr packet
                                      ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNRequester
      #pragma mark

      class STUNRequester : public Noop,
                            public MessageQueueAssociator,
                            public ISTUNRequester,
                            public ISTUNRequesterForSTUNRequesterManager,
                            public ITimerDelegate
      {
      public:
        friend interaction ISTUNRequesterFactory;

      protected:
        STUNRequester(
                      IMessageQueuePtr queue,
                      ISTUNRequesterDelegatePtr delegate,
                      IPAddress serverIP,
                      STUNPacketPtr stun,
                      STUNPacket::RFCs usingRFC,
                      Duration maxTimeout
                      );
        
        STUNRequester(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};
        
        void init();

      public:
        ~STUNRequester();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequester => STUNRequester
        #pragma mark

        static STUNRequesterPtr create(
                                       IMessageQueuePtr queue,
                                       ISTUNRequesterDelegatePtr delegate,
                                       IPAddress serverIP,
                                       STUNPacketPtr stun,
                                       STUNPacket::RFCs usingRFC,
                                       Duration maxTimeout = Duration()
                                       );

        virtual PUID getID() const {return mID;}

        virtual bool isComplete() const;

        virtual void cancel();

        virtual void retryRequestNow();

        virtual IPAddress getServerIP() const;
        virtual STUNPacketPtr getRequest() const;

        virtual Duration getMaxTimeout() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequester => ISTUNRequesterForSTUNRequesterManager
        #pragma mark

        virtual bool handleSTUNPacket(
                                      IPAddress fromIPAddress,
                                      STUNPacketPtr packet
                                      );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequester => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequester => (internal)
        #pragma mark

        String log(const char *message) const;

        void internalCancel();
        void step();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequester => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        STUNRequesterWeakPtr mThisWeak;
        PUID mID;

        ISTUNRequesterDelegatePtr mDelegate;
        STUNPacketPtr mSTUNRequest;

        IPAddress mServerIP;

        TimerPtr mTimer;
        TimerPtr mMaxTimeTimer;

        Duration mCurrentTimeout;
        ULONG mTryNumber;

        STUNPacket::RFCs mUsingRFC;

        Time mRequestStartTime;
        Duration mMaxTimeout;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISTUNRequesterFactory
      #pragma mark

      interaction ISTUNRequesterFactory
      {
        static ISTUNRequesterFactory &singleton();

        virtual STUNRequesterPtr create(
                                        IMessageQueuePtr queue,
                                        ISTUNRequesterDelegatePtr delegate,
                                        IPAddress serverIP,
                                        STUNPacketPtr stun,
                                        STUNPacket::RFCs usingRFC,
                                        Duration maxTimeout = Duration()
                                        );
      };

    }
  }
}
