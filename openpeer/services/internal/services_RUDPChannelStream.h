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
#include <openpeer/services/internal/services_IRUDPChannelStream.h>
#include <zsLib/Timer.h>
#include <zsLib/Exception.h>

#include <boost/shared_array.hpp>

#include <map>
#include <list>

#pragma warning(push)
#pragma warning(disable:4290)

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
      #pragma mark IRUDPChannelStreamAsync
      #pragma mark

      interaction IRUDPChannelStreamAsync
      {
        virtual void onSendNow() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream
      #pragma mark

      class RUDPChannelStream : public Noop,
                                public MessageQueueAssociator,
                                public IRUDPChannelStream,
                                public ITimerDelegate,
                                public IRUDPChannelStreamAsync
      {
      public:
        friend interaction IRUDPChannelStreamFactory;

        typedef boost::shared_array<BYTE> RecycleBuffer;
        typedef std::list<RecycleBuffer> RecycleBufferList;

        struct BufferedPacket;
        typedef boost::shared_ptr<BufferedPacket> BufferedPacketPtr;

        typedef std::map<QWORD, BufferedPacketPtr> BufferedPacketMap;

        struct BufferedData;
        typedef boost::shared_ptr<BufferedData> BufferedDataPtr;

        typedef std::list<BufferedDataPtr> BufferedDataList;

        struct Exceptions
        {
          ZS_DECLARE_CUSTOM_EXCEPTION(IllegalACK)
        };

      protected:
        RUDPChannelStream(
                          IMessageQueuePtr queue,
                          IRUDPChannelStreamDelegatePtr delegate,
                          QWORD nextSequenceNumberToUseForSending,
                          QWORD nextSequenberNumberExpectingToReceive,
                          WORD sendingChannelNumber,
                          WORD receivingChannelNumber,
                          DWORD minimumNegotiatedRTTInMilliseconds
                          );
        RUDPChannelStream(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~RUDPChannelStream();

      protected:

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannelStream => IRUDPChannelStream
        #pragma mark

        static RUDPChannelStreamPtr create(
                                           IMessageQueuePtr queue,
                                           IRUDPChannelStreamDelegatePtr delegate,
                                           QWORD nextSequenceNumberToUseForSending,
                                           QWORD nextSequenberNumberExpectingToReceive,
                                           WORD sendingChannelNumber,
                                           WORD receivingChannelNumber,
                                           DWORD minimumNegotiatedRTTInMilliseconds
                                           );

        virtual PUID getID() const {return mID;}

        virtual RUDPChannelStreamStates getState() const;
        virtual RUDPChannelStreamShutdownReasons getShutdownReason() const;

        virtual void shutdown(bool shutdownOnlyOnceAllDataSent = false);

        virtual void shutdownDirection(Shutdown state);

        virtual void holdSendingUntilReceiveSequenceNumber(QWORD sequenceNumber);

        virtual ULONG receive(
                              BYTE *outBuffer,
                              ULONG bufferSizeAvailableInBytes
                              );

        virtual bool doesReceiveHaveMoreDataAvailable();

        virtual ULONG getReceiveSizeAvailableInBytes();

        virtual bool send(
                          const BYTE *buffer,
                          ULONG bufferLengthInBytes
                          );

        virtual ULONG getSendSize();

        virtual bool handlePacket(
                                  RUDPPacketPtr packet,
                                  boost::shared_array<BYTE> originalBuffer,
                                  ULONG originalBufferLengthInBytes,
                                  bool ecnMarked
                                  );

        virtual void notifySocketWriteReady();

        virtual void handleExternalAck(
                                       PUID guarenteedDeliveryRequestID,
                                       QWORD nextSequenceNumber,
                                       QWORD greatestSequenceNumberReceived,
                                       QWORD greatestSequenceNumberFullyReceived,
                                       const BYTE *externalVector,
                                       ULONG externalVectorLengthInBytes,
                                       bool vpFlag,
                                       bool pgFlag,
                                       bool xpFlag,
                                       bool dpFlag,
                                       bool ecFlag
                                       );

        virtual void getState(
                              QWORD &nextSequenceNumber,
                              QWORD &greatestSequenceNumberReceived,
                              QWORD &greatestSequenceNumberFullyReceived,
                              BYTE *outVector,
                              ULONG &outVectorSizeInBytes,
                              ULONG maxVectorSizeInBytes,
                              bool &outVPFlag,
                              bool &outPGFlag,
                              bool &outXPFlag,
                              bool &outDPFlag,
                              bool &outECFlag
                              );

        virtual void notifyExternalACKSent(QWORD ackedSequenceNumber);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannelStream => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannelStream => IRUDPChannelStreamAsync
        #pragma mark

        virtual void onSendNow();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannelStream => (internal)
        #pragma mark

        String log(const char *message) const;
        bool isShuttingDown() {return RUDPChannelStreamState_ShuttingDown == mCurrentState;}
        bool isShutdown() {return RUDPChannelStreamState_Shutdown == mCurrentState;}

        void cancel();
        void setState(RUDPChannelStreamStates state);
        void setShutdownReason(RUDPChannelStreamShutdownReasons reason);

        bool sendNowHelper(
                           IRUDPChannelStreamDelegatePtr &delegate,
                           const BYTE *buffer,
                           ULONG packetLengthInBytes
                           );
        bool sendNow();   // returns true if new packets were sent that weren't sent before
        void sendNowCleanup();
        void handleAck(
                       QWORD outNextSequenceNumber,
                       QWORD outGreatestSequenceNumberReceived,
                       QWORD outGreatestSequenceNumberFullyReceived,
                       const BYTE *outVector,
                       ULONG vectorLengthInBytes,
                       bool vpFlag,
                       bool pgFlag,
                       bool xpFlag,
                       bool dpFlag,
                       bool ecFlag
                       ) throw(Exceptions::IllegalACK);

        void handleECN();
        void handleDuplicate();
        void handlePacketLoss();
        void handleUnfreezing();

        void deliverReadPackets();
        ULONG getFromWriteBuffer(
                                 BYTE *outBuffer,
                                 ULONG maxFillSize
                                 );

        void notifyReadReadyAgainIfData();
        void notifyWriteReadyAgainIfSpace();

        void getBuffer(
                       RecycleBuffer &outBuffer,
                       ULONG &ioBufferAllocLengthInBytes
                       );
        void freeBuffer(
                        RecycleBuffer &ioBuffer,
                        ULONG bufferAllocLengthInBytes
                        );

        bool getRandomFlag();

        void closeOnAllDataSent();

      public:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannelStream::BufferedPacket
        #pragma mark

        struct BufferedPacket
        {
          static BufferedPacketPtr create();

          void flagAsReceivedByRemoteParty(
                                           ULONG &ioTotalPacketsToResend,
                                           ULONG &ioAvailableBatons
                                           );

          void flagForResending(ULONG &ioTotalPacketsToResend);
          void doNotResend(ULONG &ioTotalPacketsToResend);

          void consumeBaton(ULONG &ioAvailableBatons);
          void releaseBaton(ULONG &ioAvailableBatons);

          QWORD mSequenceNumber;

          Time mTimeSentOrReceived;

          RUDPPacketPtr mRUDPPacket;
          boost::shared_array<BYTE> mPacket;
          ULONG mPacketLengthInBytes;

          // used for sending packets
          bool mXORedParityToNow;               // only used on buffered packets being sent over the wire to keep track of the current parity state to "this" packet

          bool mHoldsBaton;                     // this packet holds a baton
          bool mFlaggedAsFailedToReceive;       // this packet was flagged that it was never received by the remote party (only flagged once)
          bool mFlagForResendingInNextBurst;    // this packet needs to be resent at the next possible burst window
        };

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannelStream::BufferedData
        #pragma mark

        struct BufferedData
        {
          static BufferedDataPtr create();

          boost::shared_array<BYTE> mBuffer;
          ULONG mBufferLengthInBytes;  // how large is the buffer in bytes
          ULONG mAllocSizeInBytes;     // how large was the buffer when it was allocated

          ULONG mConsumed;             // how much of the buffer has been consumed
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RUDPChannelStream => (data)
        #pragma mark

        mutable RecursiveLock mLock;

        RUDPChannelStreamWeakPtr mThisWeak;
        PUID mID;

        IRUDPChannelStreamDelegatePtr mDelegate;

        bool mInformedReadReady;
        bool mInformedWriteReady;

        bool mDidReceiveWriteReady;

        RUDPChannelStreamStates mCurrentState;
        RUDPChannelStreamShutdownReasons mShutdownReason;

        WORD mSendingChannelNumber;
        WORD mReceivingChannelNumber;

        Duration mMinimumRTT;
        Duration mCalculatedRTT;
        QWORD mNextSequenceNumber;        // next sequence number to use for sending
        bool mXORedParityToNow;           // the current parity of all packets sent to this point

        QWORD mGSNR;                      // Greatest Sequence Number Received (this is the remote party's sequence number)
        QWORD mGSNFR;                     // Greatest Sequence Number Fully Received (this is the remote party's sequence number)
        bool mGSNRParity;                 // This is the parity bit from the remote party
        bool mXORedParityToGSNFR;         // what is the combined parity of packets received up to the GSNFR from the remote party

        QWORD mWaitToSendUntilReceivedRemoteSequenceNumber;

        Shutdown mShutdownState;

        bool mDuplicateReceived;
        bool mECNReceived;

        Time mLastDeliveredReadData;

        bool mAttemptingSendNow;

        BufferedPacketMap mSendingPackets;
        BufferedPacketMap mReceivedPackets;

        BufferedDataList mReadData;
        BufferedDataList mWriteData;

        RecycleBufferList mRecycleBuffers;

        ULONG mRandomPoolPos;
        BYTE mRandomPool[256];

        ULONG mTotalPacketsToResend;

        // congestion control parameters
        ULONG mAvailableBurstBatons;                        // how many "batons" (aka relay style batons) are available for sending new bursts right now

        TimerPtr mBurstTimer;                               // this timer will be used to consume the available batons until they are gone (the timer will be cancelled when there is no more available batons or there is no more data to send)

        // If there is no burst timer and no "batons" available then when this
        // timer fires an external ACK must be delivered to ensure that data
        // has in fact been delivered to the other side.
        TimerPtr mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer;

        TimerPtr mAddToAvailableBurstBatonsTimer;           // add to the batons available when this timer fires (this timer is only active as long as there is data to send)
        Duration mAddToAvailableBurstBatonsDuation;         // every time there is new congestion this duration is doubled

        ULONG mPacketsPerBurst;                             // how many packets to deliver in a single burst

        bool mBandwidthIncreaseFrozen;                      // the bandwidth increase routine is currently frozen because an insufficient time without issues has not occurerd
        Time mStartedSendingAtTime;                         // when did the sending activate again (so when the final ACK comes in the total duration can be calculated)
        Duration mTotalSendingPeriodWithoutIssues;          // how long has there been a successful period of sending without and sending difficulties

        QWORD mForceACKOfSentPacketsAtSendingSequnceNumber;  // when the ACK reply comes back we can be sure of the state of lost packets up to this sequence number
        PUID mForceACKOfSentPacketsRequestID;                // the identification of the request that is causing the force
        bool mForceACKNextTimePossible;                      // force an ACK at the next possibel interval
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelStreamFactory
      #pragma mark

      interaction IRUDPChannelStreamFactory
      {
        static IRUDPChannelStreamFactory &singleton();

        virtual RUDPChannelStreamPtr create(
                                            IMessageQueuePtr queue,
                                            IRUDPChannelStreamDelegatePtr delegate,
                                            QWORD nextSequenceNumberToUseForSending,
                                            QWORD nextSequenberNumberExpectingToReceive,
                                            WORD sendingChannelNumber,
                                            WORD receivingChannelNumber,
                                            DWORD minimumNegotiatedRTTInMilliseconds
                                            );
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::internal::IRUDPChannelStreamAsync)
ZS_DECLARE_PROXY_METHOD_0(onSendNow)
ZS_DECLARE_PROXY_END()

#pragma warning(pop)
