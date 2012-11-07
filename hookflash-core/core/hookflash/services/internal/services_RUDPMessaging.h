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
#include <hookflash/services/IRUDPMessaging.h>
#include <hookflash/services/IRUDPChannel.h>

#include <boost/shared_array.hpp>

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      class RUDPMessaging : public zsLib::MessageQueueAssociator,
                            public IRUDPMessaging,
                            public IRUDPChannelDelegate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::BYTE BYTE;
        typedef zsLib::DWORD DWORD;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::String String;
        typedef zsLib::IPAddress IPAddress;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef IRUDPMessaging::MessageBuffer MessageBuffer;
        typedef boost::shared_array<BYTE> RecycledPacketBuffer;
        typedef std::list<RecycledPacketBuffer> RecycledPacketBufferList;

      protected:
        RUDPMessaging(
                     IMessageQueuePtr queue,
                     IRUDPMessagingDelegatePtr delegate,
                      ULONG maxMessageSizeInBytes
                     );

        void init();

      public:
        ~RUDPMessaging();

        //IRUDPMessaging
        static RUDPMessagingPtr acceptChannel(
                                              IMessageQueuePtr queue,
                                              IRUDPListenerPtr listener,
                                              IRUDPMessagingDelegatePtr delegate,
                                              ULONG maxMessageSizeInBytes
                                              );

        static RUDPMessagingPtr acceptChannel(
                                              IMessageQueuePtr queue,
                                              IRUDPICESocketSessionPtr session,
                                              IRUDPMessagingDelegatePtr delegate,
                                              ULONG maxMessageSizeInBytes
                                              );

        static RUDPMessagingPtr openChannel(
                                            IMessageQueuePtr queue,
                                            IRUDPICESocketSessionPtr session,
                                            IRUDPMessagingDelegatePtr delegate,
                                            const char *connectionInfo,
                                            ULONG maxMessageSizeInBytes
                                            );

        virtual PUID getID() const {return mID;}

        virtual RUDPMessagingStates getState() const;
        virtual RUDPMessagingShutdownReasons getShutdownReason() const;

        virtual void shutdown();

        virtual void shutdownDirection(Shutdown state);

        virtual bool send(
                          const BYTE *message,
                          ULONG messsageLengthInBytes
                          );

        virtual void setMaxMessageSizeInBytes(ULONG maxMessageSizeInBytes);

        virtual MessageBuffer getBufferLargeEnoughForNextMessage();

        virtual ULONG getNextReceivedMessageSizeInBytes();

        virtual ULONG receive(BYTE *outBuffer);

        virtual IPAddress getConnectedRemoteIP();

        virtual String getRemoteConnectionInfo();

        //IRUDPChannelDelegate
        virtual void onRDUPChannelStateChanged(
                                               IRUDPChannelPtr session,
                                               RUDPChannelStates state
                                               );

        virtual void onRUDPChannelReadReady(IRUDPChannelPtr session);
        virtual void onRUDPChannelWriteReady(IRUDPChannelPtr session);

      protected:
        bool isShuttingDown() const {return RUDPMessagingState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return RUDPMessagingState_Shutdown == mCurrentState;}

        String log(const char *message) const;

        void cancel();
        void setState(RUDPMessagingStates state);
        void setShutdownReason(RUDPMessagingShutdownReasons reason);

        IRUDPChannelPtr getChannel() const;

        void obtainNextMessageSize();
        void notifyReadReady();

        void getBuffer(RecycledPacketBuffer &outBuffer);
        void recycleBuffer(RecycledPacketBuffer &buffer);

        class AutoRecycleBuffer
        {
        public:
          AutoRecycleBuffer(RUDPMessaging &outer, RecycledPacketBuffer &buffer) : mOuter(outer), mBuffer(buffer) {}
          ~AutoRecycleBuffer() {mOuter.recycleBuffer(mBuffer);}
        private:
          RUDPMessaging &mOuter;
          RecycledPacketBuffer &mBuffer;
        };

      protected:
        mutable RecursiveLock mLock;
        RUDPMessagingWeakPtr mThisWeak;
        PUID mID;

        RUDPMessagingStates mCurrentState;
        RUDPMessagingShutdownReasons mShutdownReason;

        IRUDPMessagingDelegatePtr mDelegate;
        bool mInformedReadReady;
        bool mInformedWriteReady;

        RUDPMessagingPtr mGracefulShutdownReference;

        IRUDPChannelPtr mChannel;

        DWORD mNextMessageSizeInBytes;

        ULONG mMaxMessageSizeInBytes;

        RecycledPacketBufferList mRecycledBuffers;
      };
    }
  }
}
