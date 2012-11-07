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

#include <hookflash/services/internal/hookflashTypes.h>
#include <hookflash/services/ISTUNRequester.h>
#include <hookflash/services/STUNPacket.h>
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>
#include <zsLib/Proxy.h>

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      class STUNRequester : public zsLib::MessageQueueAssociator,
                            public ISTUNRequester,
                            public zsLib::ITimerDelegate
      {
      public:
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::Duration Duration;
        typedef zsLib::Time Time;
        typedef zsLib::TimerPtr TimerPtr;

      protected:
        STUNRequester(
                      IMessageQueuePtr queue,
                      ISTUNRequesterDelegatePtr delegate,
                      IPAddress serverIP,
                      STUNPacketPtr stun,
                      STUNPacket::RFCs usingRFC,
                      Duration maxTimeout
                      );
        void init();

      public:
        ~STUNRequester();

        static STUNRequesterPtr create(
                                       IMessageQueuePtr queue,
                                       ISTUNRequesterDelegatePtr delegate,
                                       IPAddress serverIP,
                                       STUNPacketPtr stun,
                                       STUNPacket::RFCs usingRFC,
                                       Duration maxTimeout = Duration()
                                       );


        bool handleSTUNPacket(
                              IPAddress fromIPAddress,
                              STUNPacketPtr packet
                              );

        // STUNRequester
        virtual PUID getID() const {return mID;}

        virtual bool isComplete() const;

        virtual void cancel();

        virtual void retryRequestNow();

        virtual IPAddress getServerIP() const;
        virtual STUNPacketPtr getRequest() const;

        virtual Duration getMaxTimeout() const;

        // ITimerDelegate
        virtual void onTimer(TimerPtr timer);

      protected:
        String log(const char *message) const;

        void internalCancel();
        void step();

      protected:
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
    }
  }
}
