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

#include <openpeer/services/internal/services_RUDPChannelStream.h>
#include <openpeer/services/internal/services_Helper.h>

#include <openpeer/services/RUDPPacket.h>

#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#include <cryptopp/osrng.h>

#include <algorithm>

#pragma warning(push)
#pragma warning(disable:4290)

#define OPENPEER_SERVICES_RUDP_MINIMUM_RECOMMENDED_RTT_IN_MILLISECONDS (40)
#define OPENPEER_SERVICES_RUDP_MINIMUM_BURST_TIMER_IN_MILLISECONDS (20)
#define OPENPEER_SERVICES_RUDP_DEFAULT_CALCULATE_RTT_IN_MILLISECONDS (200)

#define OPENPEER_SERVICES_MINIMUM_DATA_BUFFER_LENGTH_ALLOCATED_IN_BYTES (16*1024)
#define OPENPEER_SERVICES_MAX_RECYCLE_BUFFERS 16

#define OPENPEER_SERVICES_MAX_WINDOW_TO_NEXT_SEQUENCE_NUMBER (256)

#define OPENPEER_SERVICES_MAX_EXPAND_WINDOW_SINCE_LAST_READ_DELIVERED_IN_SECONDS (10)

#define OPENPEER_SERVICES_UNFREEZE_AFTER_SECONDS_OF_GOOD_TRANSMISSION (10)
#define OPENPEER_SERVICES_DEFAULT_PACKETS_PER_BURST (3)

//#define OPENPEER_INDUCE_FAKE_PACKET_LOSS
#define OPENPEER_INDUCE_FAKE_PACKET_LOSS_PERCENTAGE (10)


#ifdef OPENPEER_INDUCE_FAKE_PACKET_LOSS
#define WARNING_INDUCING_FAKE_PACKET_LOSS 1
#define WARNING_INDUCING_FAKE_PACKET_LOSS 2
#endif //OPENPEER_INDUCE_FAKE_PACKET_LOSS


namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      using services::internal::IRUDPChannelStreamPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static bool logicalXOR(bool value1, bool value2) {
        return (0 != ((value1 ? 1 : 0) ^ (value2 ? 1 : 0)));
      }

      //-----------------------------------------------------------------------
      static String sequenceToString(QWORD value)
      {
        return string(value) + " (" + string(value & 0xFFFFFF) + ")";
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelStream
      #pragma mark

      //-----------------------------------------------------------------------
      const char *IRUDPChannelStream::toString(RUDPChannelStreamStates state)
      {
        switch (state) {
          case RUDPChannelStreamState_Connected:            return "Connected";
          case RUDPChannelStreamState_DirectionalShutdown:  return "Directional shutdown";
          case RUDPChannelStreamState_ShuttingDown:         return "Shutting down";
          case RUDPChannelStreamState_Shutdown:             return "Shutdown";
          default:  break;
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      const char *IRUDPChannelStream::toString(RUDPChannelStreamShutdownReasons reason)
      {
        return IRUDPChannel::toString((IRUDPChannel::RUDPChannelShutdownReasons)reason);
      }

      //-----------------------------------------------------------------------
      void IRUDPChannelStream::getRecommendedStartValues(
                                                         QWORD &outRecommendedNextSequenceNumberForSending,
                                                         DWORD &outMinimumRecommendedRTTInMilliseconds,
                                                         CongestionAlgorithmList &outLocalAlgorithms,
                                                         CongestionAlgorithmList &outRemoteAlgoirthms
                                                         )
      {
        AutoSeededRandomPool rng;

        rng.GenerateBlock((BYTE *)(&outRecommendedNextSequenceNumberForSending), sizeof(outRecommendedNextSequenceNumberForSending));
#if UINT_MAX <= 0xFFFFFFFF
        QWORD temp = 1;
        temp = (temp << 48)-1;
        outRecommendedNextSequenceNumberForSending = (outRecommendedNextSequenceNumberForSending & temp); // can only be 48 bits maximum at the start
#else
        outRecommendedNextSequenceNumberForSending = (outRecommendedNextSequenceNumberForSending & 0xFFFFFFFFFF); // can only be 48 bits maximum at the start
#endif

        // not allowed to be "0"
        if (0 == outRecommendedNextSequenceNumberForSending)
          outRecommendedNextSequenceNumberForSending = 1;

        outMinimumRecommendedRTTInMilliseconds = OPENPEER_SERVICES_RUDP_MINIMUM_RECOMMENDED_RTT_IN_MILLISECONDS;

        outLocalAlgorithms.clear();
        outRemoteAlgoirthms.clear();

        outLocalAlgorithms.push_back(IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp);
        outRemoteAlgoirthms.push_back(IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp);
      }

      //-----------------------------------------------------------------------
      bool IRUDPChannelStream::getResponseToOfferedAlgorithms(
                                                              const CongestionAlgorithmList &offeredAlgorithmsForLocal,
                                                              const CongestionAlgorithmList &offeredAlgorithmsForRemote,
                                                              CongestionAlgorithmList &outResponseAlgorithmsForLocal,
                                                              CongestionAlgorithmList &outResponseAlgorithmsForRemote
                                                              )
      {
        CongestionAlgorithmList::const_iterator findLocal = find(offeredAlgorithmsForLocal.begin(), offeredAlgorithmsForLocal.end(), IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp);
        CongestionAlgorithmList::const_iterator findRemote = find(offeredAlgorithmsForRemote.begin(), offeredAlgorithmsForRemote.end(), IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp);
        if (offeredAlgorithmsForLocal.end() == findLocal)
          return false;
        if (offeredAlgorithmsForRemote.end() == findRemote)
          return false;

        outResponseAlgorithmsForLocal.clear();
        outResponseAlgorithmsForRemote.clear();

        // only need to select a preferred if the preferred does not match our only choice of "TCPLikeWindow" that available at this time
        if (offeredAlgorithmsForLocal.begin() != findLocal)
          outResponseAlgorithmsForLocal.push_back(IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp);

        if (offeredAlgorithmsForRemote.begin() != findRemote)
          outResponseAlgorithmsForRemote.push_back(IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp);

        return true;
      }

      //-------------------------------------------------------------------------
      String IRUDPChannelStream::toDebugString(IRUDPChannelStreamPtr stream, bool includeCommaPrefix)
      {
        return IRUDPChannelStream::toDebugString(stream, includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      IRUDPChannelStreamPtr IRUDPChannelStream::create(
                                                       IMessageQueuePtr queue,
                                                       IRUDPChannelStreamDelegatePtr delegate,
                                                       QWORD nextSequenceNumberToUseForSending,
                                                       QWORD nextSequenberNumberExpectingToReceive,
                                                       WORD sendingChannelNumber,
                                                       WORD receivingChannelNumber,
                                                       DWORD minimumNegotiatedRTT,
                                                       CongestionAlgorithms algorithmForLocal,
                                                       CongestionAlgorithms algorithmForRemote
                                                       )
      {
        return internal::IRUDPChannelStreamFactory::singleton().create(
                                                                       queue,
                                                                       delegate,
                                                                       nextSequenceNumberToUseForSending,
                                                                       nextSequenberNumberExpectingToReceive,
                                                                       sendingChannelNumber,
                                                                       receivingChannelNumber,
                                                                       minimumNegotiatedRTT
                                                                       );
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPChannelStream::RUDPChannelStream(
                                           IMessageQueuePtr queue,
                                           IRUDPChannelStreamDelegatePtr delegate,
                                           QWORD nextSequenceNumberToUseForSending,
                                           QWORD nextSequenberNumberExpectingToReceive,
                                           WORD sendingChannelNumber,
                                           WORD receivingChannelNumber,
                                           DWORD minimumNegotiatedRTTInMilliseconds
                                           ) :
        MessageQueueAssociator(queue),
        mDelegate(IRUDPChannelStreamDelegateProxy::createWeak(queue, delegate)),
        mDidReceiveWriteReady(true),
        mCurrentState(RUDPChannelStreamState_Connected),
        mSendingChannelNumber(sendingChannelNumber),
        mReceivingChannelNumber(receivingChannelNumber),
        mMinimumRTT(Milliseconds(minimumNegotiatedRTTInMilliseconds)),
        mCalculatedRTT(Milliseconds(OPENPEER_SERVICES_RUDP_DEFAULT_CALCULATE_RTT_IN_MILLISECONDS)),
        mNextSequenceNumber(nextSequenceNumberToUseForSending),
        mGSNR(nextSequenberNumberExpectingToReceive-1),
        mGSNFR(nextSequenberNumberExpectingToReceive-1),
        mShutdownState(IRUDPChannel::Shutdown_None),
        mLastDeliveredReadData(zsLib::now()),
        mAvailableBurstBatons(1),
        mAddToAvailableBurstBatonsDuation(Milliseconds(OPENPEER_SERVICES_RUDP_DEFAULT_CALCULATE_RTT_IN_MILLISECONDS)),
        mPacketsPerBurst(OPENPEER_SERVICES_DEFAULT_PACKETS_PER_BURST),
        mStartedSendingAtTime(zsLib::now()),
        mTotalSendingPeriodWithoutIssues(Milliseconds(0)),
        mForceACKOfSentPacketsRequestID(0)
      {
        ZS_LOG_BASIC(log("created"))
        if (mCalculatedRTT < mMinimumRTT)
          mCalculatedRTT = mMinimumRTT;

        CryptoPP::AutoSeededRandomPool rng;
        rng.GenerateBlock(&(mRandomPool[0]), sizeof(mRandomPool));
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::init()
      {
        AutoRecursiveLock lock(mLock);

        notifyWriteReadyAgainIfSpace();
      }

      //-----------------------------------------------------------------------
      RUDPChannelStream::~RUDPChannelStream()
      {
        if(isNoop()) return;
        
        ZS_LOG_BASIC(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      RUDPChannelStreamPtr RUDPChannelStream::convert(IRUDPChannelStreamPtr stream)
      {
        return boost::dynamic_pointer_cast<RUDPChannelStream>(stream);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream => IRUDPChannelStream
      #pragma mark

      //-----------------------------------------------------------------------
      String RUDPChannelStream::toDebugString(IRUDPChannelStreamPtr stream, bool includeCommaPrefix)
      {
        if (!stream) return String(includeCommaPrefix ? ", rudp channel stream=(null)" : "rudp channel stream=(null)");

        RUDPChannelStreamPtr pThis = RUDPChannelStream::convert(stream);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      RUDPChannelStreamPtr RUDPChannelStream::create(
                                                     IMessageQueuePtr queue,
                                                     IRUDPChannelStreamDelegatePtr delegate,
                                                     QWORD nextSequenceNumberToUseForSending,
                                                     QWORD nextSequenberNumberExpectingToReceive,
                                                     WORD sendingChannelNumber,
                                                     WORD receivingChannelNumber,
                                                     DWORD minimumNegotiatedRTT
                                                     )
      {
        RUDPChannelStreamPtr pThis(new RUDPChannelStream(
                                                         queue,
                                                         delegate,
                                                         nextSequenceNumberToUseForSending,
                                                         nextSequenberNumberExpectingToReceive,
                                                         sendingChannelNumber,
                                                         receivingChannelNumber,
                                                         minimumNegotiatedRTT
                                                         ));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }
      
      //-----------------------------------------------------------------------
      IRUDPChannelStream::RUDPChannelStreamStates RUDPChannelStream::getState(
                                                                              WORD *outLastErrorCode,
                                                                              String *outLastErrorReason
                                                                              ) const
      {
        AutoRecursiveLock lock(mLock);
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::shutdown(bool shutdownOnlyOnceAllDataSent)
      {
        ZS_LOG_DETAIL(log("shutdown called") + ", only when data sent=" + (shutdownOnlyOnceAllDataSent ? "true" : "false"))

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("shutdown called but already cancelled"))
          return;
        }

        AutoRecursiveLock lock(mLock);
        if (!shutdownOnlyOnceAllDataSent) {
          cancel();
          return;
        }

        shutdownDirection(IRUDPChannel::Shutdown_Receive);
        setState(RUDPChannelStreamState_ShuttingDown);

        closeOnAllDataSent();
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::shutdownDirection(Shutdown state)
      {
        AutoRecursiveLock lock(mLock);

        ZS_LOG_DETAIL(log("shutdown direction called") + ", state=" + string(state) + ", existing=" + string(mShutdownState))
        mShutdownState = static_cast<Shutdown>(mShutdownState | state);  // you cannot stop shutting down what has already been shutting down
        if (0 != (IRUDPChannel::Shutdown_Receive & mShutdownState)) {
          // clear out the read data entirely - effectively acts as an ignore filter on the received data
          mReadData.clear();
        }
        if (isShutdown()) return;
        if (isShuttingDown()) return;

        setState(RUDPChannelStreamState_DirectionalShutdown);
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::holdSendingUntilReceiveSequenceNumber(QWORD sequenceNumber)
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DETAIL(log("hold sending until receive sequence number") + ", sequence number=" + sequenceToString(sequenceNumber))
        get(mWaitToSendUntilReceivedRemoteSequenceNumber) = sequenceNumber;

        if (0 == mWaitToSendUntilReceivedRemoteSequenceNumber) {
          // the hold was manually removed, try to deliver data now...
          (IRUDPChannelStreamAsyncProxy::create(mThisWeak.lock()))->onSendNow();
        }
      }

      //-----------------------------------------------------------------------
      ULONG RUDPChannelStream::receive(
                                       BYTE *outBuffer,
                                       ULONG bufferSizeAvailableInBytes
                                       )
      {
        ZS_LOG_DEBUG(log("receive called") + ", buffer size=" + string(bufferSizeAvailableInBytes))

        AutoRecursiveLock lock(mLock);
        get(mInformedReadReady) = false; // if this method was called in response to a read-ready event then clear the read-ready flag so the event can fire again

        if (0 == bufferSizeAvailableInBytes) {
          notifyReadReadyAgainIfData();
          return 0;
        }

        ZS_THROW_INVALID_USAGE_IF(!outBuffer)

        ULONG bytesRead = 0;

        for (BufferedDataList::iterator readIter = mReadData.begin(); readIter != mReadData.end(); )
        {
          BufferedDataList::iterator current = readIter;
          ++readIter;

          BufferedDataPtr buffer = (*current);
          ULONG available = (buffer->mBufferLengthInBytes - buffer->mConsumed);
          ULONG fillSize = (available > bufferSizeAvailableInBytes ? bufferSizeAvailableInBytes : available);
          memcpy(outBuffer, buffer->mBuffer.get() + buffer->mConsumed, fillSize);
          outBuffer += fillSize;
          bufferSizeAvailableInBytes -= fillSize;
          buffer->mConsumed += fillSize;
          bytesRead += fillSize;

          if (buffer->mConsumed == buffer->mBufferLengthInBytes) {
            freeBuffer(buffer->mBuffer, buffer->mAllocSizeInBytes);  // recyle the buffer for later
            mReadData.erase(current);  // the buffer is exhasted so drop it
          }
          if (0 == bufferSizeAvailableInBytes) {
            notifyReadReadyAgainIfData();
            return bytesRead;
          }
        }

        notifyReadReadyAgainIfData();
        return bytesRead;
      }

      //-----------------------------------------------------------------------
      bool RUDPChannelStream::doesReceiveHaveMoreDataAvailable()
      {
        AutoRecursiveLock lock(mLock);
        get(mInformedReadReady) = false;  // if this method was called in response to a read-ready event then clear the read-ready flag so the event can fire again

        for (BufferedDataList::iterator iter = mReadData.begin(); iter != mReadData.end(); ++iter)
        {
          BufferedDataPtr &buffer = (*iter);
          ULONG available = (buffer->mBufferLengthInBytes - buffer->mConsumed);
          if (0 != available)
            return true;
        }
        return false;
      }

      //-----------------------------------------------------------------------
      ULONG RUDPChannelStream::getReceiveSizeAvailableInBytes()
      {
        AutoRecursiveLock lock(mLock);
        get(mInformedReadReady) = false;  // if this method was called in response to a read-ready event then clear the read-ready flag so the event can fire again

        ULONG totalAvailable = 0;

        for (BufferedDataList::iterator iter = mReadData.begin(); iter != mReadData.end(); ++iter)
        {
          BufferedDataPtr &buffer = (*iter);
          ULONG available = (buffer->mBufferLengthInBytes - buffer->mConsumed);
          totalAvailable += available;
        }

        ZS_LOG_DEBUG(log("receive get size called") + ", result size=" + string(totalAvailable))
        return totalAvailable;
      }

      //-----------------------------------------------------------------------
      bool RUDPChannelStream::send(
                                   const BYTE *buffer,
                                   ULONG bufferLengthInBytes
                                   )
      {
        ZS_LOG_DEBUG(log("send called") + ", size=" + string(bufferLengthInBytes))

        ZS_THROW_INVALID_USAGE_IF(!buffer)

        AutoRecursiveLock lock(mLock);
        get(mInformedWriteReady) = false;  // if this method was called in response to a write-ready event then clear the write-ready flag so the event can fire again

        if (isShutdown()) return false;

        if (0 != (IRUDPChannel::Shutdown_Send & mShutdownState)) return false;

        if (0 == bufferLengthInBytes) {
          notifyWriteReadyAgainIfSpace();
          return true;
        }

        if (mWriteData.size() > 0) {
          // try to put the buffer into the tail end of the previous buffer (if room allows)
          BufferedDataPtr last = mWriteData.back();
          ULONG available = last->mAllocSizeInBytes - last->mBufferLengthInBytes;
          available = (available > bufferLengthInBytes ? bufferLengthInBytes : available);
          if (0 != available) {
            ZS_LOG_DEBUG(log("adding to existing buffer") + ", available=" + string(available))
            memcpy(&((last->mBuffer.get())[last->mBufferLengthInBytes]), buffer, available);

            last->mBufferLengthInBytes += available;

            buffer += available;
            bufferLengthInBytes -= available;

            if (0 == bufferLengthInBytes) {
              (IRUDPChannelStreamAsyncProxy::create(mThisWeak.lock()))->onSendNow();
              return true;
            }
          }
        }

        ZS_LOG_DEBUG(log("adding to new buffer") + ", size=" + string(bufferLengthInBytes))

        // we have data that cannot fit into the last buffer, create a new data buffer
        BufferedDataPtr data = BufferedData::create();

        ULONG allocSize = bufferLengthInBytes;
        getBuffer(data->mBuffer, allocSize);

        data->mBufferLengthInBytes = bufferLengthInBytes;
        data->mAllocSizeInBytes = allocSize;

        memcpy(data->mBuffer.get(), buffer, bufferLengthInBytes);
        mWriteData.push_back(data);

        (IRUDPChannelStreamAsyncProxy::create(mThisWeak.lock()))->onSendNow();
        return true;
      }

      //-----------------------------------------------------------------------
      ULONG RUDPChannelStream::getSendSize()
      {
        AutoRecursiveLock lock(mLock);
        ULONG totalSending = 0;

        for (BufferedDataList::iterator iter = mWriteData.begin(); iter != mWriteData.end(); ++iter)
        {
          BufferedDataPtr &buffer = (*iter);
          ULONG sending = (buffer->mBufferLengthInBytes - buffer->mConsumed);
          totalSending += sending;
        }

        ZS_LOG_DEBUG(log("get send size called") + ", result=" + string(totalSending))
        return totalSending;
      }

      //-----------------------------------------------------------------------
      bool RUDPChannelStream::handlePacket(
                                           RUDPPacketPtr packet,
                                           boost::shared_array<BYTE> originalBuffer,
                                           ULONG originalBufferLengthInBytes,
                                           bool ecnMarked
                                           )
      {
        ZS_LOG_TRACE(log("handle packet called") + ", size=" + string(originalBufferLengthInBytes) + ", ecn=" + (ecnMarked ? "true" : "false"))

        bool fireExternalACKIfNotSent = false;

        // scope: handle packet
        {
          AutoRecursiveLock lock(mLock);
          if (!mDelegate) return false;

          if (packet->mChannelNumber != mReceivingChannelNumber) {
            ZS_LOG_WARNING(Debug, log("incoming channel mismatch") + ", channel=" + string(mReceivingChannelNumber) + ", packet channel=" + string(packet->mChannelNumber))
            return false;
          }

          get(mECNReceived) = (mECNReceived || ecnMarked);

          QWORD sequenceNumber = packet->getSequenceNumber(mGSNR);

          // we no longer have to wait on a send once we find the correct sequence number
          if (sequenceNumber >= mWaitToSendUntilReceivedRemoteSequenceNumber)
            get(mWaitToSendUntilReceivedRemoteSequenceNumber) = 0;

          if (sequenceNumber <= mGSNFR) {
            ZS_LOG_WARNING(Debug, log("received duplicate packet") + ", GSNFR=" + sequenceToString(mGSNFR) + ", packet sequence number=" + sequenceToString(sequenceNumber))
            get(mDuplicateReceived) = true;
            return true;
          }

          // we can't process packets that are beyond the window in which we can process
          if (sequenceNumber > (mGSNR + OPENPEER_SERVICES_MAX_WINDOW_TO_NEXT_SEQUENCE_NUMBER)) {
            ZS_LOG_WARNING(Debug, log("received packet beyond allowed window") + ", GSNR=" + sequenceToString(mGSNR) + ", packet sequence number=" + sequenceToString(sequenceNumber))
            return false;
          }

          // this packet is within the window but before we accept it we will process its ack to see if it makes any sense
          try {
            handleAck(
                      sequenceNumber,
                      packet->getGSNR(mNextSequenceNumber),                     // this is the remote party's GSNR
                      packet->getGSNFR(mNextSequenceNumber),                    // this is the remote party's GSNFR
                      &(packet->mVector[0]),
                      packet->mVectorLengthInBytes,
                      packet->isFlagSet(RUDPPacket::Flag_VP_VectorParity),
                      packet->isFlagSet(RUDPPacket::Flag_PG_ParityGSNR),
                      packet->isFlagSet(RUDPPacket::Flag_XP_XORedParityToGSNFR),
                      packet->isFlagSet(RUDPPacket::Flag_DP_DuplicatePacket),
                      packet->isFlagSet(RUDPPacket::Flag_EC_ECNPacket)
                      );
          } catch(Exceptions::IllegalACK &) {
            ZS_LOG_WARNING(Debug, log("received illegal ACK") + ", packet sequence number=" + sequenceToString(sequenceNumber))
            setError(RUDPChannelStreamShutdownReason_IllegalStreamState, "received illegal ack");
            cancel();
            return true;
          }

          // we handled the ack now receive the data...
          BufferedPacketMap::iterator findIter = mReceivedPackets.find(sequenceNumber);
          if (findIter != mReceivedPackets.end()) {
            ZS_LOG_WARNING(Debug, log("received packet is duplicated and already exist in pending buffers thus dropping packet") + ", packet sequence number=" + sequenceToString(sequenceNumber))
            // we have already received and processed this packet
            get(mDuplicateReceived) = true;
            return true;
          }

          // allow any packet to be delivered between the mGSNFR to the default window size to be added to the buffer (since it helps move the window)
          if (sequenceNumber > mGSNFR+OPENPEER_SERVICES_MAX_WINDOW_TO_NEXT_SEQUENCE_NUMBER) {

            if (sequenceNumber > mGSNR) {
              Time current = zsLib::now();

              Duration maxDuration = (mCalculatedRTT * 3);
              if (maxDuration > Seconds(OPENPEER_SERVICES_MAX_EXPAND_WINDOW_SINCE_LAST_READ_DELIVERED_IN_SECONDS)) {
                // The remote party could have intentionally caused a really large
                // RTT in order to open a very large buffer window in the receiver
                // thus we have to prevent them expanding the window massively big
                // during the calculated RTT and intentionally leaving gaps with the
                // idea to overload the receivers capacity. To prevent this we will
                // calculate how many packets we actually receive during a 4 second
                // window and limit our outstanding capacity to that window.
                maxDuration = Seconds(OPENPEER_SERVICES_MAX_EXPAND_WINDOW_SINCE_LAST_READ_DELIVERED_IN_SECONDS);
              }

              // if this packet is attempting to expand the window, only allow expansion until the last delivered packet is 3xRTT old
              if ((mLastDeliveredReadData + (mCalculatedRTT * 3)) < current) {
                ZS_LOG_WARNING(Debug, log("last deliverd ready data too old and expanding window thus dropping packet") + ", GSNR=" + sequenceToString(mGSNR) + ", packet sequence number=" + sequenceToString(sequenceNumber))
                return false;
              }
            }
          }

          // put the packet in order
          BufferedPacketPtr bufferedPacket = BufferedPacket::create();
          bufferedPacket->mSequenceNumber = sequenceNumber;
          bufferedPacket->mRUDPPacket = packet;
          bufferedPacket->mPacket = originalBuffer;
          bufferedPacket->mPacketLengthInBytes = originalBufferLengthInBytes;

          mReceivedPackets[sequenceNumber] = bufferedPacket;
          if (sequenceNumber > mGSNR) {
            mGSNR = sequenceNumber;
            get(mGSNRParity) = packet->isFlagSet(RUDPPacket::Flag_PS_ParitySending);
          }

          // if set then remote wants an ACK right away - if we are able to send data packets then we will be able to ACK the packet right now without an external ACK
          if (packet->isFlagSet(RUDPPacket::Flag_AR_ACKRequired))
            fireExternalACKIfNotSent = true;

          ZS_LOG_TRACE(log("accepting packet into window") + ", packet sequence number=" + sequenceToString(sequenceNumber) + ", GSNR=" + sequenceToString(mGSNR) + ", GSNR parity=" + (mGSNRParity ? "on" : "off") + ", ack required=" + (fireExternalACKIfNotSent ? "true" : "false"))

          deliverReadPackets();

        } // scope

        // because we have possible new ACKs the window might have progressed, attempt to send more data now
        bool sent = sendNow();  // WARNING: this method cannot be called from within a lock
        if (sent) {
          notifyWriteReadyAgainIfSpace();
          return true;
        }
        if (!fireExternalACKIfNotSent) return true;

        AutoRecursiveLock lock(mLock);
        if (!mDelegate) return true;

        // we were unable to deliver any more data so ask to deliver an external ACK immediately
        try {
          mDelegate->onRUDPChannelStreamSendExternalACKNow(mThisWeak.lock(), false);
        } catch(IRUDPChannelStreamDelegateProxy::Exceptions::DelegateGone &) {
          setError(RUDPChannelStreamShutdownReason_DelegateGone, "delegate gone");
          cancel();
          return true;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::notifySocketWriteReady()
      {
        ZS_LOG_TRACE(log("socket write ready called"))
        AutoRecursiveLock lock(mLock);
        if (!mDelegate) return;
        if (mDidReceiveWriteReady) return;
        mDidReceiveWriteReady = true;
        (IRUDPChannelStreamAsyncProxy::create(mThisWeak.lock()))->onSendNow();
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::handleExternalAck(
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
                                                )
      {
        ZS_LOG_TRACE(log("handle external ACK called") +
                              ", forced ACK ID=" + string(mForceACKOfSentPacketsRequestID) +
                              ", guarenteed delivery request ID=" + string(guarenteedDeliveryRequestID) +
                              ", sequence number=" + sequenceToString(nextSequenceNumber) +
                              ", GSNR=" + sequenceToString(greatestSequenceNumberReceived) +
                              ", GSNFR=" + sequenceToString(greatestSequenceNumberFullyReceived))

        AutoRecursiveLock lock(mLock);
        if (!mDelegate) {
          ZS_LOG_WARNING(Trace, log("delegate gone thus ignoring external ACK"))
          return;
        }

        if (mSendingPackets.size() < 1) {
          // cancel any forced ACK if the sending size goes down to zero
          mForceACKOfSentPacketsRequestID = 0;
        }

        if (nextSequenceNumber >= mWaitToSendUntilReceivedRemoteSequenceNumber)
          get(mWaitToSendUntilReceivedRemoteSequenceNumber) = 0;

        try {
          handleAck(
                    nextSequenceNumber,
                    greatestSequenceNumberReceived,
                    greatestSequenceNumberFullyReceived,
                    externalVector,
                    externalVectorLengthInBytes,
                    vpFlag,
                    pgFlag,
                    xpFlag,
                    dpFlag,
                    ecFlag
                    );
        } catch(Exceptions::IllegalACK &) {
          ZS_LOG_TRACE(log("received illegal ACK"))
          setError(RUDPChannelStreamShutdownReason_IllegalStreamState, "received illegal ack");
          cancel();
          return;
        }

        if ((0 != guarenteedDeliveryRequestID) &&
            (mForceACKOfSentPacketsRequestID == guarenteedDeliveryRequestID) &&
            (mSendingPackets.size() > 0)) {

          // We just received an ACK from the remote party which is the one we
          // were forcing an ACK to ensure that the packets we are trying to
          // resend get delivered.

          mForceACKOfSentPacketsRequestID = 0;
          get(mForceACKNextTimePossible) = false;

          bool firstTime = true;

          QWORD sequenceNumber = 0;
          for (BufferedPacketMap::iterator iter = mSendingPackets.begin(); iter != mSendingPackets.end(); ++iter) {
            sequenceNumber = (*iter).first;

            if (sequenceNumber > mForceACKOfSentPacketsAtSendingSequnceNumber) {
              break;
            }

            BufferedPacketPtr &packet = (*iter).second;
            if (firstTime) {
              firstTime = false;
              ZS_LOG_TRACE(log("force ACK starting to process")  +
                                  ", starting at ACK sequence number=" + sequenceToString(sequenceNumber) +
                                  ", forced ACK to sequence number=" + sequenceToString(mForceACKOfSentPacketsAtSendingSequnceNumber) +
                                  ", batons available=" + string(mAvailableBurstBatons))
            }

            packet->flagForResending(mTotalPacketsToResend);  // if this packet was not ACKed but should be resent because it never arrived after the current forced ACK replied
            packet->releaseBaton(mAvailableBurstBatons);      // reclaim the baton if holding since this packet needs to be resent and never arrived
          }

          ZS_LOG_TRACE(log("forced ACK cannot ACK beyond the forced ACK point") +
                                  ", stopped at ACK sequence number=" + sequenceToString(sequenceNumber) +
                                  ", forced ACK to sequence number=" + sequenceToString(mForceACKOfSentPacketsAtSendingSequnceNumber) +
                                  ", batons available=" + string(mAvailableBurstBatons))
        }

        if (mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer) {
          // since an ACK did arrive, we can cancel this timer which ensures an ACK will arrive
          ZS_LOG_TRACE(log("cancelling ensure data arrived timer because we did receive an ACK") + ", old timer ID=" + string(mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->getID()))
          mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->cancel();
          mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer.reset();
        }

        (IRUDPChannelStreamAsyncProxy::create(mThisWeak.lock()))->onSendNow();
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::getState(
                                       QWORD &outNextSequenceNumber,
                                       QWORD &outGreatestSequenceNumberReceived,
                                       QWORD &outGreatestSequenceNumberFullyReceived,
                                       BYTE *outVector,
                                       ULONG &outVectorSizeInBytes,
                                       ULONG maxVectorSizeInBytes,
                                       bool &outVPFlag,
                                       bool &outPGFlag,
                                       bool &outXPFlag,
                                       bool &outDPFlag,
                                       bool &outECFlag
                                       )
      {
        AutoRecursiveLock lock(mLock);
        outNextSequenceNumber = mNextSequenceNumber;
        outGreatestSequenceNumberReceived = mGSNR;
        outGreatestSequenceNumberFullyReceived = mGSNFR;

        String vectorParityField;

        outVPFlag = mXORedParityToGSNFR;
        if ((outVector) &&
            (0 != maxVectorSizeInBytes)) {

          RUDPPacket::VectorEncoderState state;
          RUDPPacket::vectorEncoderStart(state, mGSNR, mGSNFR, mXORedParityToGSNFR, outVector, maxVectorSizeInBytes);

          QWORD sequenceNumber = mGSNFR+1;

          // create a vector until the vector is full or we run out of packets that we have received
          for (BufferedPacketMap::iterator iter = mReceivedPackets.begin(); iter != mReceivedPackets.end(); ++iter) {
            BufferedPacketPtr packet = (*iter).second;
            bool added = true;
            while (sequenceNumber < packet->mSequenceNumber)
            {
              added = RUDPPacket::vectorEncoderAdd(state, RUDPPacket::VectorState_NotReceived, false);
              if (!added)
                break;

              if (ZS_IS_LOGGING(Trace)) { vectorParityField += "."; }
              ++sequenceNumber;
            }
            if (!added)
              break;

            if (sequenceNumber == packet->mSequenceNumber) {
              added = RUDPPacket::vectorEncoderAdd(
                                                   state,
                                                   (packet->mRUDPPacket->isFlagSet(RUDPPacket::Flag_EC_ECNPacket) ? RUDPPacket::VectorState_ReceivedECNMarked : RUDPPacket::VectorState_Received),
                                                   packet->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending)
                                                   );
              if (!added)
                break;

              ++sequenceNumber;
              if (ZS_IS_LOGGING(Trace)) { vectorParityField += (packet->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending) ? "X" : "x"); }
            }
          }
          RUDPPacket::vectorEncoderFinalize(state, outVPFlag, outVectorSizeInBytes);
        }

        outPGFlag = mGSNRParity;
        outXPFlag = mXORedParityToGSNFR;
        outDPFlag = mDuplicateReceived;
        outECFlag = mECNReceived;

        ZS_LOG_TRACE(
                     log("get current state")
                     + ", next sequence number=" + sequenceToString(outNextSequenceNumber)
                     + ", GSNR=" + sequenceToString(outGreatestSequenceNumberReceived)
                     + ", GSNFR=" + sequenceToString(outGreatestSequenceNumberFullyReceived)
                     + ", vector=" + vectorParityField
                     + ", vector size=" + string(outVectorSizeInBytes)
                     + ", vp=" + (outVPFlag ? "true" : "false")
                     + ", pg=" + (outPGFlag ? "true" : "false")
                     + ", xp=" + (outXPFlag ? "true" : "false")
                     + ", dp=" + (outDPFlag ? "true" : "false")
                     + ", ec=" + (outECFlag ? "true" : "false")
                     )
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::notifyExternalACKSent(QWORD ackedSequenceNumber)
      {
        ZS_LOG_TRACE(log("external ACK sent"))
        AutoRecursiveLock lock(mLock);
        get(mDuplicateReceived) = false;
        get(mECNReceived) = false;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPChannelStream::onTimer(TimerPtr timer)
      {
        ZS_LOG_TRACE(log("tick") + ", timer ID=" + string(timer->getID()))

        {
          AutoRecursiveLock lock(mLock);

          PUID burstID = (mBurstTimer ? mBurstTimer->getID() : 0);
          PUID ensureID = (mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer ? mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->getID() : 0);
          PUID addID = (mAddToAvailableBurstBatonsTimer ? mAddToAvailableBurstBatonsTimer->getID() : 0);

          ZS_LOG_TRACE(log("tick") + ", comparing timer ID=" + string(timer->getID()) +
                       ", burst ID=" + string(burstID) +
                       ", ensure ID=" + string(ensureID) +
                       ", addID ID=" + string(addID))

          if (timer == mBurstTimer) {
            ZS_LOG_TRACE(log("burst timer is firing") + ", timer ID=" + string(timer->getID()))
            goto quickExitToSendNow;
          }

          if (timer == mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer) {
            ZS_LOG_TRACE(log("ensuring data has arrived by causing an external ACK") + ", timer ID=" + string(timer->getID()))
            // this is only fired once if there is data that we want to force an ACK from the remote party
            mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->cancel();
            mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer.reset();

            // we will use the "force" ACK mechanism to ensure that data has arrived
            get(mForceACKNextTimePossible) = true;
            goto quickExitToSendNow;
          }

          if (timer == mAddToAvailableBurstBatonsTimer) {
            ZS_LOG_TRACE(log("available burst batons timer fired") + ", timer ID=" + string(timer->getID()))

            if (0 == (rand()%2)) {
              ++mAvailableBurstBatons;
              ZS_LOG_TRACE(log("creating a new sending burst baton now") + ", batons available=" + string(mAvailableBurstBatons))
            } else {
              ++mPacketsPerBurst;
              ZS_LOG_TRACE(log("increasing the packets per burst") + ", packets per burst=" + string(mPacketsPerBurst))
            }
            goto quickExitToSendNow;
          }

          ZS_LOG_TRACE(log("unknown time has fired") + ", timer ID=" + string(timer->getID()))
        }

      quickExitToSendNow:
        sendNow();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream => IRUDPChannelStreamAsync
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPChannelStream::onSendNow()
      {
        ZS_LOG_TRACE(log("on send now called"))
        sendNow();  // do NOT call from within a lock
        notifyWriteReadyAgainIfSpace();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String RUDPChannelStream::log(const char *message) const
      {
        return String("RUDPChannelStream [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      String RUDPChannelStream::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(mLock);
        bool firstTime = !includeCommaPrefix;
        return

        Helper::getDebugValue("rudp channel stream ID", string(mID), firstTime) +

        Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +

        Helper::getDebugValue("state", IRUDPChannelStream::toString(mCurrentState), firstTime) +
        Helper::getDebugValue("last error", 0 != mLastError ? string(mLastError) : String(), firstTime) +
        Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +

        Helper::getDebugValue("informed read ready", mInformedReadReady ? String("true") : String(), firstTime) +
        Helper::getDebugValue("informed write ready", mInformedWriteReady ? String("true") : String(), firstTime) +

        Helper::getDebugValue("did receive write ready", mDidReceiveWriteReady ? String("true") : String(), firstTime) +

        Helper::getDebugValue("sending channel number", 0 != mSendingChannelNumber ? string(mSendingChannelNumber) : String(), firstTime) +
        Helper::getDebugValue("receiving channel number", 0 != mReceivingChannelNumber ? string(mReceivingChannelNumber) : String(), firstTime) +

        Helper::getDebugValue("minimum RTT (ms)", Duration() != mMinimumRTT ? string(mMinimumRTT.total_milliseconds()) : String(), firstTime) +
        Helper::getDebugValue("calculated RTT (ms)", Duration() != mCalculatedRTT ? string(mCalculatedRTT.total_milliseconds()) : String(), firstTime) +

        Helper::getDebugValue("next sequence number", 0 != mNextSequenceNumber ? string(mNextSequenceNumber) : String(), firstTime) +

        Helper::getDebugValue("xor parity to now", mXORedParityToNow ? "1" : "0", firstTime) +

        Helper::getDebugValue("GSNR", 0 != mGSNR ? string(mGSNR) : String(), firstTime) +
        Helper::getDebugValue("GSNFR", 0 != mGSNFR ? string(mGSNFR) : String(), firstTime) +

        Helper::getDebugValue("GSNR parity", mGSNRParity ? "1" : "0", firstTime) +
        Helper::getDebugValue("xor parity to GSNFR", mXORedParityToGSNFR ? "1" : "0", firstTime) +

        Helper::getDebugValue("wait to send until received sequence number", 0 != mWaitToSendUntilReceivedRemoteSequenceNumber ? string(mWaitToSendUntilReceivedRemoteSequenceNumber) : String(), firstTime) +

        Helper::getDebugValue("shutdown state", IRUDPChannel::toString(mShutdownState), firstTime) +

        Helper::getDebugValue("duplicate received", mDuplicateReceived ? String("true") : String(), firstTime) +
        Helper::getDebugValue("ECN received", mECNReceived ? String("true") : String(), firstTime) +

        Helper::getDebugValue("last delivered read data", Time() != mLastDeliveredReadData ? IHelper::timeToString(mLastDeliveredReadData) : String(), firstTime) +

        Helper::getDebugValue("attempting send now", mAttemptingSendNow ? String("true") : String(), firstTime) +

        Helper::getDebugValue("sending packets", mSendingPackets.size() > 0 ? string(mSendingPackets.size()) : String(), firstTime) +
        Helper::getDebugValue("received packets", mReceivedPackets.size() > 0 ? string(mReceivedPackets.size()) : String(), firstTime) +

        Helper::getDebugValue("read data", mReadData.size() > 0 ? string(mReadData.size()) : String(), firstTime) +
        Helper::getDebugValue("write data", mWriteData.size() > 0 ? string(mWriteData.size()) : String(), firstTime) +

        Helper::getDebugValue("recycled buffers", mRecycleBuffers.size() > 0 ? string(mRecycleBuffers.size()) : String(), firstTime) +

        Helper::getDebugValue("random pool pos", 0 != mRandomPoolPos ? string(mRandomPoolPos) : String(), firstTime) +

        Helper::getDebugValue("total packets to resend", 0 != mTotalPacketsToResend ? string(mTotalPacketsToResend) : String(), firstTime) +

        Helper::getDebugValue("available burst batons", 0 != mAvailableBurstBatons ? string(mAvailableBurstBatons) : String(), firstTime) +

        Helper::getDebugValue("burst timer", mBurstTimer ? String("true") : String(), firstTime) +

        Helper::getDebugValue("ensure data has arrived when no more burst batons available timer", mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer ? String("true") : String(), firstTime) +

        Helper::getDebugValue("add to available burst batons timer", mAddToAvailableBurstBatonsTimer ? String("true") : String(), firstTime) +
        Helper::getDebugValue("add to available burst batons duration (ms)", Duration() != mAddToAvailableBurstBatonsDuation ? string(mAddToAvailableBurstBatonsDuation.total_milliseconds()) : String(), firstTime) +

        Helper::getDebugValue("total packets per burst", 0 != mPacketsPerBurst ? string(mPacketsPerBurst) : String(), firstTime) +

        Helper::getDebugValue("bandwidth increase frozen", mBandwidthIncreaseFrozen ? String("true") : String(), firstTime) +
        Helper::getDebugValue("started sending time", Time() != mStartedSendingAtTime ? string(mStartedSendingAtTime) : String(), firstTime) +
        Helper::getDebugValue("total sending period without issues (ms)", Duration() != mTotalSendingPeriodWithoutIssues ? string(mTotalSendingPeriodWithoutIssues.total_milliseconds()) : String(), firstTime) +

        Helper::getDebugValue("force ACKs of sent packets sending sequence number", 0 != mForceACKOfSentPacketsAtSendingSequnceNumber ? string(mForceACKOfSentPacketsAtSendingSequnceNumber) : String(), firstTime) +
        Helper::getDebugValue("force ACKs of sent packets request ID", 0 != mForceACKOfSentPacketsRequestID ? string(mForceACKOfSentPacketsRequestID) : String(), firstTime) +

        Helper::getDebugValue("force ACK next time possible", mForceACKNextTimePossible ? String("true") : String(), firstTime);
      }
      
      //-----------------------------------------------------------------------
      void RUDPChannelStream::cancel()
      {
        AutoRecursiveLock lock(mLock);          // just in case...

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("cancel already complete"))
          return;
        }

        ZS_LOG_TRACE(log("cancel called"))

        setState(RUDPChannelStreamState_Shutdown);

        mDelegate.reset();

        mSendingPackets.clear();
        mReceivedPackets.clear();

        mReadData.clear();
        mWriteData.clear();

        mRecycleBuffers.clear();

        if (mBurstTimer) {
          mBurstTimer->cancel();
          mBurstTimer.reset();
        }

        if (mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer) {
          mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->cancel();
          mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer.reset();
        }

        if (mAddToAvailableBurstBatonsTimer) {
          mAddToAvailableBurstBatonsTimer->cancel();
          mAddToAvailableBurstBatonsTimer.reset();
        }

        ZS_LOG_TRACE(log("cancel complete"))
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::setState(RUDPChannelStreamStates state)
      {
        if (mCurrentState == state) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))
        mCurrentState = state;

        RUDPChannelStreamPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onRUDPChannelStreamStateChanged(pThis, mCurrentState);
          } catch(IRUDPChannelStreamDelegateProxy::Exceptions::DelegateGone &) {
          }
        }
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());
        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("already shutting down thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        get(mLastError) = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + ", code=" + string(mLastError) + ", reason=" + mLastErrorReason + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      bool RUDPChannelStream::sendNowHelper(
                                            IRUDPChannelStreamDelegatePtr &delegate,
                                            const BYTE *buffer,
                                            ULONG packetLengthInBytes
                                            )
      {
#ifdef OPENPEER_INDUCE_FAKE_PACKET_LOSS
        bool forcePacketLoss = ((rand() % 100) < OPENPEER_INDUCE_FAKE_PACKET_LOSS_PERCENTAGE);
        if (forcePacketLoss) {
          ZS_LOG_WARNING(Trace, log("faking packet loss in deliver attempt"))
        }
        return = (forcePacketLoss ? true : (delegate->notifyRUDPChannelStreamSendPacket(mThisWeak.lock(), buffer, packetLengthInBytes)));
#else
        return delegate->notifyRUDPChannelStreamSendPacket(mThisWeak.lock(), buffer, packetLengthInBytes);
#endif //OPENPEER_INDUCE_FAKE_PACKET_LOSS
      }

      //-----------------------------------------------------------------------
      bool RUDPChannelStream::sendNow()
      {
        //*********************************************************************
        //*********************************************************************
        //                              WARNING
        //*********************************************************************
        // This method calls a delegate synchronously thus cannot be called
        // from within a lock.
        //*********************************************************************

        BufferedPacketPtr firstPacketCreated;
        BufferedPacketPtr lastPacketSent;
        IRUDPChannelStreamDelegatePtr delegate;

        //.....................................................................
        // NOTE: The unwinder is used to protect against this class from being
        //       called twice from an unlocked thread plus ensurs that
        //       the protection variable is unset in the event of a stack
        //       unwind (i.e. any quick "return" from method)
        class Unwinder
        {
        public:
          Unwinder(RecursiveLock &memberLock, bool &value) : mLock(memberLock), mValue(value), mWasSet(false) {}
          ~Unwinder() { if (!mWasSet) return; AutoRecursiveLock lock(mLock); mValue = false; }

          void set() { AutoRecursiveLock lock(mLock); mValue = true; mWasSet = true; }
        private:
          RecursiveLock &mLock;
          bool &mValue;
          bool mWasSet;
        };

        Unwinder protect(mLock, mAttemptingSendNow);

        {
          AutoRecursiveLock lock(mLock);

          ZS_LOG_TRACE(log("send now called") +
                            ", available burst batons=" + string(mAvailableBurstBatons) +
                            ", packets per burst=" + string(mPacketsPerBurst) +
                            ", resend=" + string(mTotalPacketsToResend) +
                            ", send size=" + string(mSendingPackets.size()) +
                            ", write data=" + string(mWriteData.size()))

          if (isShutdown()) {
            ZS_LOG_TRACE(log("already shutdown thus aborting send now"))
            return false;
          }
          delegate = mDelegate;

          // we cannot deliver data if we are waiting to send
          if (0 != mWaitToSendUntilReceivedRemoteSequenceNumber) {
            ZS_LOG_TRACE(log("cannot send as waiting for sequence number") + ", waiting sequence number=" + sequenceToString(mWaitToSendUntilReceivedRemoteSequenceNumber))
            return false;
          }

          // if the socket isn't available for writing then we cannot send data
          if (!mDidReceiveWriteReady) {
            ZS_LOG_TRACE(log("cannot send as write not ready"))
            return false;
          }

          if (mAttemptingSendNow) {
            ZS_LOG_TRACE(log("already attempting to send thus aborting"))
            return false; // already in the middle of an attempt at sending so cannot create or send more packets now
          }

          protect.set();
        }

        //.....................................................................
        //.....................................................................
        //.....................................................................
        //.....................................................................
        try {
          ULONG packetsToSend = 1;

          // scope: check out if we can send now
          {
            AutoRecursiveLock lock(mLock);
            if (0 == mAvailableBurstBatons) {
              ZS_LOG_TRACE(log("no batons for bursting available for sending data thus aborting send routine"))
              goto sendNowQuickExit;
            }

            packetsToSend = mPacketsPerBurst;
          }

          while (0 != packetsToSend)
          {
            BufferedPacketPtr attemptToDeliver;
            boost::shared_array<BYTE> attemptToDeliverBuffer;
            ULONG attemptToDeliverBufferSizeInBytes = 0;

            // scope: grab the next buffer to be resent over the wire
            {
              AutoRecursiveLock lock(mLock);

              if (0 != mTotalPacketsToResend) {
                BufferedPacketMap::iterator iter = mSendingPackets.begin();
                if (lastPacketSent) {
                  iter = mSendingPackets.find(lastPacketSent->mSequenceNumber);
                }
                if (iter == mSendingPackets.end()) {
                  iter = mSendingPackets.begin();
                }

                for (; iter != mSendingPackets.end(); ++iter) {
                  BufferedPacketPtr &packet = (*iter).second;
                  if (packet->mFlagForResendingInNextBurst) {
                    attemptToDeliver = packet;
                    attemptToDeliverBuffer = packet->mPacket;
                    attemptToDeliverBufferSizeInBytes = packet->mPacketLengthInBytes;
                  }
                }
              }
            }

            if (!attemptToDeliver) {
              AutoRecursiveLock lock(mLock);

              // there are no packets to be resent so attempt to create a new packet to send...

              if (mWriteData.size() < 1)
                goto sendNowQuickExit;

              // we need to start breaking up new packets immediately that will be sent over the wire
              RUDPPacketPtr newPacket = RUDPPacket::create();
              newPacket->setSequenceNumber(mNextSequenceNumber);
              newPacket->setGSN(mGSNR, mGSNFR);
              newPacket->mChannelNumber = mSendingChannelNumber;
              newPacket->setFlag(RUDPPacket::Flag_PS_ParitySending, getRandomFlag());
              newPacket->setFlag(RUDPPacket::Flag_PG_ParityGSNR, mGSNRParity);
              newPacket->setFlag(RUDPPacket::Flag_XP_XORedParityToGSNFR, mXORedParityToGSNFR);
              newPacket->setFlag(RUDPPacket::Flag_DP_DuplicatePacket, mDuplicateReceived);
              get(mDuplicateReceived) = false;
              newPacket->setFlag(RUDPPacket::Flag_EC_ECNPacket, mECNReceived);
              get(mECNReceived) = false;

              if (!firstPacketCreated) {
                String vectorParityField; // for debugging

                // we have to create a vector now on the packet
                RUDPPacket::VectorEncoderState state;
                newPacket->vectorEncoderStart(state, mGSNR, mGSNFR, mXORedParityToGSNFR);

                QWORD sequenceNumber = mGSNFR+1;

                // create a vector until the vector is full or we run out of packets
                for (BufferedPacketMap::iterator iter = mReceivedPackets.begin(); iter != mReceivedPackets.end(); ++iter) {
                  BufferedPacketPtr packet = (*iter).second;
                  bool added = true;
                  while (sequenceNumber < packet->mSequenceNumber)
                  {
                    added = RUDPPacket::vectorEncoderAdd(state, RUDPPacket::VectorState_NotReceived, false);
                    if (!added)
                      break;

                    if (ZS_IS_LOGGING(Trace)) { vectorParityField += "."; }
                    ++sequenceNumber;
                  }
                  if (!added)
                    break;

                  if (sequenceNumber == packet->mSequenceNumber) {
                    added = RUDPPacket::vectorEncoderAdd(
                                                         state,
                                                         (packet->mRUDPPacket->isFlagSet(RUDPPacket::Flag_EC_ECNPacket) ? RUDPPacket::VectorState_ReceivedECNMarked : RUDPPacket::VectorState_Received),
                                                         packet->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending)
                                                         );
                    if (!added)
                      break;

                    ++sequenceNumber;
                    if (ZS_IS_LOGGING(Trace)) { vectorParityField += (packet->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending) ? "X" : "x"); }
                  }
                }
                newPacket->vectorEncoderFinalize(state);

                ZS_LOG_TRACE(
                             log("generating RUDP packet ACK on first packet")
                             + ", sequence number=" + sequenceToString(mNextSequenceNumber)
                             + ", GSNR=" + sequenceToString(mGSNR)
                             + ", GSNFR=" + sequenceToString(mGSNFR)
                             + ", vector=" + vectorParityField
                             + ", vector size=" + string(newPacket->mVectorLengthInBytes)
                             + ", ps=" + (newPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending) ? "true" : "false")
                             + ", pg=" + (newPacket->isFlagSet(RUDPPacket::Flag_PG_ParityGSNR) ? "true" : "false")
                             + ", xp=" + (newPacket->isFlagSet(RUDPPacket::Flag_XP_XORedParityToGSNFR) ? "true" : "false")
                             + ", dp=" + (newPacket->isFlagSet(RUDPPacket::Flag_DP_DuplicatePacket) ? "true" : "false")
                             + ", ec=" + (newPacket->isFlagSet(RUDPPacket::Flag_EC_ECNPacket) ? "true" : "false")
                             + ", vp=" + (newPacket->isFlagSet(RUDPPacket::Flag_VP_VectorParity) ? "true" : "false")
                             )
              } else {
                // copy the vector from the first packet
                newPacket->setFlag(RUDPPacket::Flag_VP_VectorParity, firstPacketCreated->mRUDPPacket->isFlagSet(RUDPPacket::Flag_VP_VectorParity));
                memcpy(&(newPacket->mVector[0]), &(firstPacketCreated->mRUDPPacket->mVector[0]), sizeof(newPacket->mVector));
                newPacket->mVectorLengthInBytes = firstPacketCreated->mRUDPPacket->mVectorLengthInBytes;
              }

              BYTE temp[OPENPEER_SERVICES_RUDP_MAX_PACKET_SIZE_WHEN_PMTU_IS_NOT_KNOWN];

              ULONG availableBytes = newPacket->getRoomAvailableForData(OPENPEER_SERVICES_RUDP_MAX_PACKET_SIZE_WHEN_PMTU_IS_NOT_KNOWN);

              ULONG bytesRead = getFromWriteBuffer(&(temp[0]), availableBytes);
              newPacket->mData = &(temp[0]);
              newPacket->mDataLengthInBytes = static_cast<WORD>(bytesRead);

              if ((mWriteData.size() < 1) ||
                  (1 == packetsToSend)) {
                newPacket->setFlag(RUDPPacket::Flag_AR_ACKRequired);
                if (mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer) {
                  ZS_LOG_TRACE(log("since a newly created packet has an ACK we will cancel the current ensure timer") + ", timer ID=" + string(mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->getID()))
                  // since this packet requires an ACK this packet will act as a implicit method to hopefully get an ACK from the remote party...
                  mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->cancel();
                  mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer.reset();
                }
              }

              boost::shared_array<BYTE> packetizedBuffer;
              ULONG packetizedLength = 0;
              newPacket->packetize(packetizedBuffer, packetizedLength);

              BufferedPacketPtr bufferedPacket = BufferedPacket::create();
              bufferedPacket->mSequenceNumber = mNextSequenceNumber;
              get(mXORedParityToNow) = internal::logicalXOR(mXORedParityToNow, newPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending));   // have to keep track of the current parity of all packets sent until this point
              bufferedPacket->mXORedParityToNow = mXORedParityToNow;          // when the remore party reports their GSNFR parity in an ACK, this value is required to verify it is accurate
              bufferedPacket->mRUDPPacket = newPacket;
              bufferedPacket->mPacket = packetizedBuffer;
              bufferedPacket->mPacketLengthInBytes = packetizedLength;

              ZS_LOG_TRACE(
                           log("adding buffer to pending list")
                           + ", sequence number=" + sequenceToString(mNextSequenceNumber)
                           + ", packet size=" + string(packetizedLength)
                           + ", GSNR=" + sequenceToString(mGSNR)
                           + ", GSNFR=" + sequenceToString(mGSNFR)
                           + ", vector size=" + string(newPacket->mVectorLengthInBytes)
                           + ", ps=" + (newPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending) ? "true" : "false")
                           + ", pg=" + (newPacket->isFlagSet(RUDPPacket::Flag_PG_ParityGSNR) ? "true" : "false")
                           + ", xp=" + (newPacket->isFlagSet(RUDPPacket::Flag_XP_XORedParityToGSNFR) ? "true" : "false")
                           + ", dp=" + (newPacket->isFlagSet(RUDPPacket::Flag_DP_DuplicatePacket) ? "true" : "false")
                           + ", ec=" + (newPacket->isFlagSet(RUDPPacket::Flag_EC_ECNPacket) ? "true" : "false")
                           + ", ar=" + (newPacket->isFlagSet(RUDPPacket::Flag_AR_ACKRequired) ? "true" : "false")
                           + ", vp=" + (newPacket->isFlagSet(RUDPPacket::Flag_VP_VectorParity) ? "true" : "false")
                           )

              if (mSendingPackets.size() == 0) {
                // this is the starting point where we are sending packets
                mStartedSendingAtTime = zsLib::now();
              }
              mSendingPackets[mNextSequenceNumber] = bufferedPacket;

              ++mNextSequenceNumber;

              if (!firstPacketCreated) {
                // remember this as the first packet created
                firstPacketCreated = bufferedPacket;
              }

              attemptToDeliver = bufferedPacket;
              attemptToDeliverBuffer = packetizedBuffer;
              attemptToDeliverBufferSizeInBytes = packetizedLength;
            }

            if (!attemptToDeliver) {
              ZS_LOG_TRACE(log("no more packets to send at this time"))
              goto sendNowQuickExit;
            }

            ZS_LOG_TRACE(log("attempting to (re)send packet") + ", sequence number=" + sequenceToString(attemptToDeliver->mSequenceNumber) + ", packets to send=" + string(packetsToSend))
            bool sent = sendNowHelper(delegate, attemptToDeliverBuffer.get(), attemptToDeliverBufferSizeInBytes);
            if (!sent) {
              ZS_LOG_WARNING(Trace, log("unable to send data onto wire as data failed to send") + ", sequence number=" + sequenceToString(attemptToDeliver->mSequenceNumber))
              if (firstPacketCreated == attemptToDeliver) {
                // failed to deliver any packet over the wire...
                firstPacketCreated.reset();
              }
              goto sendNowQuickExit;
            }

            // successfully (re)sent the packet...

            lastPacketSent = attemptToDeliver;
            --packetsToSend;  // total packets to send in the burst is now decreased

            AutoRecursiveLock lock(mLock);
            if (attemptToDeliver->mFlagForResendingInNextBurst) {
              ZS_LOG_TRACE(log("flag for resending in next burst is set this will force an ACK next time possible"))

              get(mForceACKNextTimePossible) = true;                // we need to force an ACK when there is resent data to ensure it has arrived
              attemptToDeliver->doNotResend(mTotalPacketsToResend); // if this was marked for resending, then clear it now since it is resent
            }
          }
        } catch(IRUDPChannelStreamDelegateProxy::Exceptions::DelegateGone &) {
          AutoRecursiveLock lock(mLock);
          ZS_LOG_WARNING(Trace, log("delegate gone thus cannot send packet"))
          setError(RUDPChannelStreamShutdownReason_DelegateGone, "delegate gone");
          cancel();
        }

        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------

      sendNowQuickExit:
        AutoRecursiveLock lock(mLock);
        if (lastPacketSent) {
          if (lastPacketSent->mPacket) {  // make sure the packet hasn't already been released
            // the last packet sent over the wire will hold the baton
            lastPacketSent->consumeBaton(mAvailableBurstBatons);
          }
        }
        sendNowCleanup();

        return firstPacketCreated;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::sendNowCleanup()
      {
        ZS_LOG_TRACE(log("starting send now cleaup routine") +
                            ", packets to resend=" + string(mTotalPacketsToResend) +
                            ", available batons=" + string(mAvailableBurstBatons) +
                            ", packets per burst=" + string(mPacketsPerBurst) +
                            ", write size=" + string(mWriteData.size()) +
                            ", sending size=" + string(mSendingPackets.size()) +
                            ", force ACK next time possible=" + (mForceACKNextTimePossible ? "true" : "false") +
                            ", force ACK ID=" + string(mForceACKOfSentPacketsRequestID) +
                            ", forced sequence number=" + sequenceToString(mForceACKOfSentPacketsAtSendingSequnceNumber))

        if (isShutdown()) {
          ZS_LOG_TRACE(log("already shutdown thus aborting"))
        }

        handleUnfreezing();

        bool burstTimerRequired = false;
        bool forceACKOfSentPacketsRequired = false;
        bool ensureDataHasArrivedTimer = false;
        bool addBatonsTimer = (!mBandwidthIncreaseFrozen) && (0 == mTotalPacketsToResend) && ((mSendingPackets.size() > 0) || (mWriteData.size() > 0));

        if (0 != mAvailableBurstBatons)
        {
          // there is available batons so the burst timer should be alive if there is data ready to send
          burstTimerRequired = ((mSendingPackets.size() > 0) && (0 != mTotalPacketsToResend)) ||
                                (mWriteData.size() > 0);
        }

        if (mSendingPackets.size() > 0) {
          // there is unacked sent packets in the buffer waiting for an ACK...

          // because there is unacked send data we should make sure a timer is setup to eventually ACK this data
          ensureDataHasArrivedTimer = true;

          // also we should force a new ACK if none of the
          forceACKOfSentPacketsRequired = true;

          if ((0 != mAvailableBurstBatons) &&
              (mWriteData.size() > 0)) {

            // but if there is available batons and write data outstanding then
            // there's no need to setup a timer to ensure the data to be acked
            // or force the data to be acked right away
            ensureDataHasArrivedTimer = false;
            forceACKOfSentPacketsRequired = mForceACKNextTimePossible;  // only required in this case if there was  request to force the ACK next time possible
          }

          if (0 != mForceACKOfSentPacketsRequestID) {
            forceACKOfSentPacketsRequired = false;  // cannot force again since there is already an outstanding request
          }

          if ((forceACKOfSentPacketsRequired) &&
              (!mForceACKNextTimePossible)) {
            // if we are forcing a packet now but not because we are required
            // to do it next time possible then we should see if there is
            // already an outstanding ACK require packet holding a baton
            // in which case we don't need to force an ACK immediately
            for (BufferedPacketMap::iterator iter = mSendingPackets.begin(); iter != mSendingPackets.end(); ++iter) {
              BufferedPacketPtr &packet = (*iter).second;

              if ((packet->mHoldsBaton) &&
                  (packet->mRUDPPacket->isFlagSet(RUDPPacket::Flag_AR_ACKRequired)) &&
                  (!packet->mFlagForResendingInNextBurst)) {
                // this packet holds a baton and is required to ACK so it's
                // possible that the ACK will eventually arrive so no need for
                // force an ACK just yet...
                forceACKOfSentPacketsRequired = false;
                break;
              }
            }
          }

          if (forceACKOfSentPacketsRequired) {
            // if an ACK is being forced to send right away then no need
            // to start a timer to eventually force an ACK since it is already
            // happening right now...
            ensureDataHasArrivedTimer = false;
          }
        }

        if (mSendingPackets.size() < 1) {
          // cancel any forced ACK if the sending size goes down to zero (since there is no longer a need to force
          ZS_LOG_TRACE(log("cleanup cancelling forced ACK since all data is now ACKed"))
          mForceACKOfSentPacketsRequestID = 0;
          forceACKOfSentPacketsRequired = false;
        }

        if ((forceACKOfSentPacketsRequired) ||
            (0 != mForceACKOfSentPacketsRequestID)) {
          // while there is an outstanding force in progress do not add to the
          // available burst batons and there is no need for the ensure data
          // has arrived timer as the forcing of the ACK will ensure this...
          addBatonsTimer = false;
          ensureDataHasArrivedTimer = false;
        }

        if (burstTimerRequired) {
          if (!mBurstTimer) {
            Duration burstDuration = mCalculatedRTT / ((int)mAvailableBurstBatons);

            // all available bursts should happen in one RTT
            mBurstTimer = Timer::create(mThisWeak.lock(), burstDuration);
            if (burstDuration < Milliseconds(OPENPEER_SERVICES_RUDP_MINIMUM_BURST_TIMER_IN_MILLISECONDS)) {
              burstDuration = Milliseconds(OPENPEER_SERVICES_RUDP_MINIMUM_BURST_TIMER_IN_MILLISECONDS);
            }

            ZS_LOG_TRACE(log("creating a burst timer since there is data to send and available batons to send it") + ", timer ID=" + string(mBurstTimer->getID()) + ", available batons=" + string(mAvailableBurstBatons) + ", write size=" + string(mWriteData.size()) + ", sending size=" + string(mSendingPackets.size()) + ", burst duration=" + string(burstDuration.total_milliseconds()) + ", calculated RTT=" + string(mCalculatedRTT.total_milliseconds()))
          }
        } else {
          if (mBurstTimer) {
            ZS_LOG_TRACE(log("cancelling the burst timer since there are no batons available or there is no more data to send") + ", timer ID=" + string(mBurstTimer->getID()) + ", available batons=" + string(mAvailableBurstBatons) + ", write size=" + string(mWriteData.size()) + ", sending size=" + string(mSendingPackets.size()))

            mBurstTimer->cancel();
            mBurstTimer.reset();
          }
        }

        if (ensureDataHasArrivedTimer) {
          if (!mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer) {
            Duration ensureDuration = (mCalculatedRTT*3)/2;

            // The timer is set to fire at 1.5 x calculated RTT
            mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer = Timer::create(mThisWeak.lock(), ensureDuration, false);

            ZS_LOG_TRACE(log("starting ensure timer to make sure packets get acked") + ", timer ID=" + string(mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->getID()) + ", available batons=" + string(mAvailableBurstBatons) + ", write size=" + string(mWriteData.size()) + ", sending size=" + string(mSendingPackets.size()) + ", ensure duration=" + string(ensureDuration.total_milliseconds()) + ", calculated RTT=" + string(mCalculatedRTT.total_milliseconds()))
          }
        } else {
          if (mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer) {
            ZS_LOG_TRACE(log("stopping ensure timer as batons available for sending still and there is outstanding unacked send data in the buffer") + ", timer ID=" + string(mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->getID()) + ", available batons=" + string(mAvailableBurstBatons) + ", write size=" + string(mWriteData.size()) + ", sending size=" + string(mSendingPackets.size()))
            mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer->cancel();
            mEnsureDataHasArrivedWhenNoMoreBurstBatonsAvailableTimer.reset();
          }
        }

        if (forceACKOfSentPacketsRequired) {
          mForceACKOfSentPacketsRequestID = zsLib::createPUID();
          get(mForceACKOfSentPacketsAtSendingSequnceNumber) = mNextSequenceNumber - 1;
          get(mForceACKNextTimePossible) = false;

          ZS_LOG_TRACE(log("forcing an ACK immediately") + ", ack ID=" + string(mForceACKOfSentPacketsRequestID) + ", forced sequence number=" + sequenceToString(mForceACKOfSentPacketsAtSendingSequnceNumber) + ", available batons=" + string(mAvailableBurstBatons) + ", write size=" + string(mWriteData.size()) + ", sending size=" + string(mSendingPackets.size()))

          try {
            mDelegate->onRUDPChannelStreamSendExternalACKNow(mThisWeak.lock(), true, mForceACKOfSentPacketsRequestID);
          } catch(IRUDPChannelStreamDelegateProxy::Exceptions::DelegateGone &) {
            setError(RUDPChannelStreamShutdownReason_DelegateGone, "delegate gone");
            cancel();
            return;
          }
        }

        if (addBatonsTimer) {
          if (!mAddToAvailableBurstBatonsTimer) {
            mAddToAvailableBurstBatonsTimer = Timer::create(mThisWeak.lock(), mAddToAvailableBurstBatonsDuation);
            ZS_LOG_TRACE(log("creating a new add to available batons timer") + ", timer ID=" + string(mAddToAvailableBurstBatonsTimer->getID()) + ", frozen=" + (mBandwidthIncreaseFrozen ? "true" : "false") + ", available batons=" + string(mAvailableBurstBatons) + ", write size=" + string(mWriteData.size()) + ", sending size=" + string(mSendingPackets.size()))
          }
        } else {
          if (mAddToAvailableBurstBatonsTimer) {
            ZS_LOG_TRACE(log("cancelling add to available batons timer") + ", timer ID=" + string(mAddToAvailableBurstBatonsTimer->getID()) + ", frozen=" + (mBandwidthIncreaseFrozen ? "true" : "false") + ", available batons=" + string(mAvailableBurstBatons) + ", write size=" + string(mWriteData.size()) + ", sending size=" + string(mSendingPackets.size()))
            mAddToAvailableBurstBatonsTimer->cancel();
            mAddToAvailableBurstBatonsTimer.reset();
          }
        }

        closeOnAllDataSent();
        notifyWriteReadyAgainIfSpace();

        ZS_LOG_TRACE(log("completed send now cleanup routine") +
                            ", burst timer=" + (burstTimerRequired ? "true" : "false") +
                            ", ensure timer=" + (ensureDataHasArrivedTimer ? "true" : "false") +
                            ", force=" + (forceACKOfSentPacketsRequired ? "true" : "false") +
                            ", add timer=" + (addBatonsTimer ? "true" : "false") +
                            ", force ACK next time possible=" + (mForceACKNextTimePossible ? "true" : "false") +
                            ", force ACK ID=" + string(mForceACKOfSentPacketsRequestID) +
                            ", forced sequence number=" + sequenceToString(mForceACKOfSentPacketsAtSendingSequnceNumber))
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::handleAck(
                                        QWORD nextSequenceNumber,
                                        QWORD gsnr,
                                        QWORD gsnfr,
                                        const BYTE *externalVector,
                                        ULONG externalVectorLengthInBytes,
                                        bool vpFlag,
                                        bool pgFlag,
                                        bool xpFlag,
                                        bool dpFlag,
                                        bool ecFlag
                                        ) throw(Exceptions::IllegalACK)
      {
        // scope: handle the ACK
        {
          if (ecFlag) {
            handleECN();
          }

          // nothing to ACK?
          if (0 == mSendingPackets.size()) {
            ZS_LOG_TRACE(log("ignoring incoming ACK because there is no send data to ACK"))
            goto handleAckQuickExit;
          }

          ZS_THROW_CUSTOM_IF(Exceptions::IllegalACK, gsnfr > gsnr)  // this is illegal

          // if the next sequence number being reported is before the sequence number we have already processed then don't handle the packet
          if (nextSequenceNumber < mGSNFR) {
            ZS_LOG_WARNING(Detail, log("ignoring ACK as it was for packet already processed") + ", sequence number=" + sequenceToString(nextSequenceNumber) + ", GSNFR=" + sequenceToString(mGSNFR))
            goto handleAckQuickExit;
          }

          // if the packet is too far ahead from the greatest packet received then it can't be processed
          if (nextSequenceNumber > (mGSNR + OPENPEER_SERVICES_MAX_WINDOW_TO_NEXT_SEQUENCE_NUMBER)) {
            ZS_LOG_WARNING(Detail, log("ignoring ACK as it was for packet too far outside window") + ", sequence number=" + sequenceToString(nextSequenceNumber) + ", GSNR=" + sequenceToString(mGSNR))
            goto handleAckQuickExit;
          }

          // the remote party is claiming they received a packet we never sent
          if (gsnr > mNextSequenceNumber) {
            ZS_LOG_WARNING(Detail, log("ignoring ACK as it was for a packet that was never sent") + ", local next sequence number=" + sequenceToString(mNextSequenceNumber) + ", remote GSNR=" + sequenceToString(gsnr))
            goto handleAckQuickExit;
          }

          // we still have packets that are unacked, check to see if we can ACK them

          // find the gsnfr packet
          BufferedPacketMap::iterator gsnfrIter = mSendingPackets.find(gsnfr);
          if (gsnfrIter != mSendingPackets.end()) {
            BufferedPacketPtr gsnfrPacket = (*gsnfrIter).second;

            // the parity up to now must match or there is a problem
            if (xpFlag !=  gsnfrPacket->mXORedParityToNow) {
              ZS_THROW_CUSTOM(Exceptions::IllegalACK, log("ACK on parity bit until GSNFR is not correct") + ", GSNFR ACKed parity=" + (xpFlag ? "1" : "0") + ", GSNFR sent parity=" + (gsnfrPacket->mXORedParityToNow ? "1" : "0"))
            }
          }

          BufferedPacketMap::iterator gsnrIter = mSendingPackets.find(gsnr);
          if (gsnrIter != mSendingPackets.end()) {
            BufferedPacketPtr gsnrPacket = (*gsnrIter).second;

            if (gsnrPacket->mRUDPPacket->isFlagSet(RUDPPacket::Flag_AR_ACKRequired)) {

              // it might be possible to measure the RTT now, but only if this ACK was received from the first send attempt
              if (!(gsnrPacket->mFlaggedAsFailedToReceive)) {
                Duration oldRTT = mCalculatedRTT;

                mCalculatedRTT = zsLib::now() - gsnrPacket->mTimeSentOrReceived;

                // we have the new calculated time but we will only move halfway between the old calculation and the new one
                if (mCalculatedRTT > oldRTT) {
                  mCalculatedRTT = oldRTT + ((mCalculatedRTT - oldRTT) / 2);
                } else {
                  mCalculatedRTT = oldRTT - ((oldRTT - mCalculatedRTT) / 2);
                }
                if (mCalculatedRTT < mMinimumRTT)
                  mCalculatedRTT = mMinimumRTT;

                ZS_LOG_TRACE(log("calculating RTT") + ", RTT milliseconds=" + string(mCalculatedRTT.total_milliseconds()))

                if (mCalculatedRTT > mAddToAvailableBurstBatonsDuation) {
                  mAddToAvailableBurstBatonsDuation = (mCalculatedRTT * 2);
                  if (mAddToAvailableBurstBatonsTimer) {
                    PUID oldTimerID = mAddToAvailableBurstBatonsTimer->getID();

                    // replace existing timer with new timer
                    mAddToAvailableBurstBatonsTimer->cancel();
                    mAddToAvailableBurstBatonsTimer.reset();

                    mAddToAvailableBurstBatonsTimer = Timer::create(mThisWeak.lock(), mAddToAvailableBurstBatonsDuation);
                    ZS_LOG_TRACE(log("add to available batons timer is set too small based on calculated RTT") + ", old timer ID=" + string(oldTimerID) + ", new timer ID=" + string(mAddToAvailableBurstBatonsTimer->getID()) + ", duration milliseconds=" + string(mAddToAvailableBurstBatonsDuation.total_milliseconds()))
                  }
                }
              }
            }

            // make sure the parity bit of the gsnr matches the gsnfr's packet (if it doesn't then it's illegal)
            ZS_THROW_CUSTOM_IF(Exceptions::IllegalACK, pgFlag !=  gsnrPacket->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending))
          }

          bool hadPackets = (mSendingPackets.size() > 0);

          // we can now acknowledge and clean out all packets up-to and including the gsnfr packet
          while (0 != mSendingPackets.size()) {
            BufferedPacketPtr current = (*(mSendingPackets.begin())).second;

            // do not delete past the point of the gsnfr received
            if (current->mSequenceNumber > gsnfr) {
              break;
            }

            ZS_LOG_TRACE(log("cleaning ACKed packet") + ", sequence number=" + sequenceToString(current->mSequenceNumber) + ", GSNFR=" + sequenceToString(gsnfr))

            current->flagAsReceivedByRemoteParty(mTotalPacketsToResend, mAvailableBurstBatons);
            mSendingPackets.erase(mSendingPackets.begin());
          }

          if ((mSendingPackets.size() == 0) &&
              (hadPackets) &&
              (!ecFlag)) {
            mTotalSendingPeriodWithoutIssues = mTotalSendingPeriodWithoutIssues + (zsLib::now() - mStartedSendingAtTime);
            hadPackets = false;
          }

          // there will be no vector if these are equal
          if (gsnr == gsnfr) {
            ZS_LOG_TRACE(log("ACK packet GSNR == GSNFR thus no vector will be present") + ", gsnr/gsnfr=" + sequenceToString(gsnr))
            goto handleAckQuickExit;
          }

          QWORD vectorSequenceNumber = gsnfr+1;

          RUDPPacket::VectorDecoderState decoder;
          RUDPPacket::vectorDecoderStart(decoder, externalVector, externalVectorLengthInBytes, gsnr, gsnfr);

          bool xoredParity = xpFlag;
          bool foundECN = false;
          bool foundLoss = false;

          String vectorParityField;
          bool couldNotCalculateVectorParity = false;

          BufferedPacketMap::iterator iter = mSendingPackets.begin();

          while (true)
          {
            if (iter == mSendingPackets.end())
              break;

            BufferedPacketPtr bufferedPacket = (*iter).second;
            if (bufferedPacket->mSequenceNumber < vectorSequenceNumber) {
              ZS_LOG_TRACE(log("ignoring buffered packet because it doesn't exist in the vector"))
              // ignore the buffered packet since its not reached the vector yet...
              ++iter;
              continue;
            }

            RUDPPacket::VectorStates state = RUDPPacket::vectorDecoderGetNextPacketState(decoder);
            if (RUDPPacket::VectorState_NoMoreData == state)
              break;

            if (vectorSequenceNumber < bufferedPacket->mSequenceNumber) {
              if ((RUDPPacket::VectorState_Received == state) || (RUDPPacket::VectorState_ReceivedECNMarked == state)) {
                couldNotCalculateVectorParity = true;
              }

              ZS_LOG_TRACE(log("ignoring vectored packet because it doesn't exist as a buffered packet") + "could not calculate parity=" + (couldNotCalculateVectorParity ? "true": "false"))

              // ignore the vector since its not reached the buffered packet yet
              ++vectorSequenceNumber;
              continue;
            }

            if ((RUDPPacket::VectorState_Received == state) || (RUDPPacket::VectorState_ReceivedECNMarked == state)) {
              if (ZS_IS_LOGGING(Trace)) { vectorParityField += (bufferedPacket->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending) ? "X" : "x"); }
              xoredParity = internal::logicalXOR(xoredParity, bufferedPacket->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending));

              // mark the current packet as being received by cleaning out the original packet data (but not the packet information)
              ZS_LOG_TRACE(log("marking packet as received because of vector ACK") + ", sequence number=" + sequenceToString(bufferedPacket->mSequenceNumber))
              bufferedPacket->flagAsReceivedByRemoteParty(mTotalPacketsToResend, mAvailableBurstBatons);
            } else {
              // this packet was not received, do not remove the packet data
              if (ZS_IS_LOGGING(Trace)) { vectorParityField += "."; }

              if (!bufferedPacket->mFlaggedAsFailedToReceive) {
                bufferedPacket->mFlaggedAsFailedToReceive = true;
                bufferedPacket->flagForResending(mTotalPacketsToResend);  // since this is the first report of this packet being lost we can be sure it needs to be resent immediately
                foundLoss = true;
              }
            }

            if (RUDPPacket::VectorState_ReceivedECNMarked == state)
              foundECN = true;

            ++iter;
            ++vectorSequenceNumber;
          }

          if (gsnrIter != mSendingPackets.end()) {
            // now it is time to mark the gsnr as received
            BufferedPacketPtr gsnrPacket = (*gsnrIter).second;
            ZS_LOG_TRACE(log("marking GSNR as received in vector case") + ", sequence number=" + sequenceToString(gsnrPacket->mSequenceNumber))
            gsnrPacket->flagAsReceivedByRemoteParty(mTotalPacketsToResend, mAvailableBurstBatons);
          }

          if ((mSendingPackets.size() == 0) &&
              (hadPackets) &&
              (!ecFlag)) {
            mTotalSendingPeriodWithoutIssues = mTotalSendingPeriodWithoutIssues + (zsLib::now() - mStartedSendingAtTime);
            hadPackets = false;
          }

          if (foundECN && (!ecFlag))
            handleECN();

          if (dpFlag)
            handleDuplicate();

          if (foundLoss)
            handlePacketLoss();

          ZS_LOG_TRACE(
                       log("handling ACK with values")
                       + ", next sequence number=" + sequenceToString(nextSequenceNumber)
                       + ", GSNR=" + sequenceToString(gsnr)
                       + ", GSNFR=" + sequenceToString(gsnfr)
                       + ", vector=" + vectorParityField
                       + ", vector size=" + string(externalVectorLengthInBytes)
                       + ", vp=" + (vpFlag ? "true" : "false")
                       + ", pg=" + (pgFlag ? "true" : "false")
                       + ", xp=" + (xpFlag ? "true" : "false")
                       + ", dp=" + (dpFlag ? "true" : "false")
                       + ", ec=" + (ecFlag ? "true" : "false")
                       )

          // at this point the xoredParity must match or this ACK is bogus
          if ((!couldNotCalculateVectorParity) &&
              (xoredParity !=  vpFlag)) {
            ZS_THROW_CUSTOM(Exceptions::IllegalACK, log("ACK on parity bit until GSNFR is not correct") + ", vector ACKed parity=" + (vpFlag ? "1" : "0") + ", calculated vector parity=" + (xoredParity ? "1" : "0"))
          }
        }

      handleAckQuickExit:

        handleUnfreezing();

        if (mSendingPackets.size() < 1) {
          // cancel any forced ACK if the sending size goes down to zero (since there is no longer a need to force
          ZS_LOG_TRACE(log("cleanup cancelling forced ACK since all data is now ACKed"))
          mForceACKOfSentPacketsRequestID = 0;
        }
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::handleECN()
      {
        ZS_LOG_TRACE(log("handling ECN"))
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::handleDuplicate()
      {
        ZS_LOG_TRACE(log("handling duplicate"))
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::handlePacketLoss()
      {
        ZS_LOG_TRACE(log("handle packet loss"))

        bool wasFrozen = mBandwidthIncreaseFrozen;

        // freeze the increase to prevent an increase in the socket sending
        get(mBandwidthIncreaseFrozen) = true;
        mStartedSendingAtTime = zsLib::now();
        mTotalSendingPeriodWithoutIssues = Milliseconds(0);

        if (mAddToAvailableBurstBatonsTimer) {
          ZS_LOG_TRACE(log("cancelling add to available burst batons due to loss") + ", timer ID=" + string(mAddToAvailableBurstBatonsTimer->getID()))
          mAddToAvailableBurstBatonsTimer->cancel();
          mAddToAvailableBurstBatonsTimer.reset();
        }

        // double the time until more batons get added
        if (!wasFrozen) {
          mAddToAvailableBurstBatonsDuation = mAddToAvailableBurstBatonsDuation * 2;
          ZS_LOG_TRACE(log("increasing add to available burst batons duration") + ", duration milliseconds=" + string(mAddToAvailableBurstBatonsDuation.total_milliseconds()))
        }

        if (mPacketsPerBurst > 1) {
          // decrease the packets per burst by half
          ULONG wasPacketsPerBurst = mPacketsPerBurst;
          mPacketsPerBurst = mPacketsPerBurst / 2;
          if (mPacketsPerBurst < 1)
            mPacketsPerBurst = 1;
          ZS_LOG_TRACE(log("decreasing packets per burst") + ", old value=" + string(wasPacketsPerBurst) + ", new packets per burst=" + string(mPacketsPerBurst))
          return;
        }

        if (mAvailableBurstBatons > 1) {
          // decrease the available batons by one (to slow sending of more bursts)
          --mAvailableBurstBatons;
          ZS_LOG_TRACE(log("decreasing batons available") + ", available batons=" + string(mAvailableBurstBatons))
          return;
        }

        // we cannot destroy the last baton available
        ULONG whichBatonToDestroy = (mAvailableBurstBatons == 0 ? 1 : 0);

        // we must destroy a baton that is pending in the sending packets
        for (BufferedPacketMap::iterator iter = mSendingPackets.begin(); iter != mSendingPackets.end(); ++iter) {
          BufferedPacketPtr &packet = (*iter).second;
          if (packet->mHoldsBaton) {
            if (0 == whichBatonToDestroy) {
              packet->releaseBaton(mAvailableBurstBatons);  // release the baton from being held by the packet
              --mAvailableBurstBatons;                      // destroy the baton
              ZS_LOG_TRACE(log("destroying a baton that was being held") + ", available batons=" + string(mAvailableBurstBatons))
              return;
            }

            --whichBatonToDestroy;
          }
        }
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::handleUnfreezing()
      {
        if (mTotalSendingPeriodWithoutIssues > Seconds(OPENPEER_SERVICES_UNFREEZE_AFTER_SECONDS_OF_GOOD_TRANSMISSION)) {
          get(mBandwidthIncreaseFrozen) = false;
          mTotalSendingPeriodWithoutIssues = Milliseconds(0);

          // decrease the time between adding new batons
          mAddToAvailableBurstBatonsDuation = (mAddToAvailableBurstBatonsDuation / 2);

          // prevent the adding window from ever getting smaller than the RTT
          if (mAddToAvailableBurstBatonsDuation < mCalculatedRTT)
            mAddToAvailableBurstBatonsDuation = mCalculatedRTT;

          PUID oldTimerID = 0;

          if (mAddToAvailableBurstBatonsTimer) {
            // kill the adding timer since the duration has changed (its okay, it will be recreated later)
            oldTimerID = mAddToAvailableBurstBatonsTimer->getID();

            mAddToAvailableBurstBatonsTimer->cancel();
            mAddToAvailableBurstBatonsTimer.reset();
          }

          ZS_LOG_TRACE(log("good period of transmission without issue thus unfreezing/increasing baton adding frequency") + ", old add to batons timer ID=" + string(oldTimerID) + ", duration milliseconds=" + string(mAddToAvailableBurstBatonsDuation.total_milliseconds()))
        }
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::deliverReadPackets()
      {
        bool delivered = false;
        // see how many packets we can confirm as received
        while (mReceivedPackets.size() > 0) {
          BufferedPacketMap::iterator iter = mReceivedPackets.begin();
          BufferedPacketPtr bufferedPacket = (*iter).second;

          // can only process the next if the packet is the next in the ordered series
          if (bufferedPacket->mSequenceNumber != (mGSNFR+1))
            break;

          delivered = true;
          ZS_LOG_TRACE(log("delivering read packet") + ", sequence number=" + sequenceToString(bufferedPacket->mSequenceNumber))

          // remember when this packet arrived so we can not allow expansion of
          // the incoming buffer window to grow too big
          mLastDeliveredReadData = bufferedPacket->mTimeSentOrReceived;

          // this is the next packet in the series... process the data from the
          // packet first (but only process the data if the receive is not
          // shutdown otherwise the data will be ignored and dropped)
          if ((bufferedPacket->mRUDPPacket->mDataLengthInBytes > 0) &&
              (0 == (IRUDPChannel::Shutdown_Receive & mShutdownState))) {
            const BYTE *pos = bufferedPacket->mRUDPPacket->mData;
            ULONG bytes = bufferedPacket->mRUDPPacket->mDataLengthInBytes;

            if (mReadData.size() > 0) {
              BufferedDataPtr last = mReadData.back();
              // see how much space is available in the last packet
              ULONG available = last->mAllocSizeInBytes - last->mBufferLengthInBytes;
              available = (available > bytes ? bytes : available);
              if (0 != available) {
                memcpy(&((last->mBuffer.get())[last->mBufferLengthInBytes]), pos, available);
              }

              pos += available;
              bytes -= available;

              last->mBufferLengthInBytes += available;
            }

            if (0 != bytes) {
              // still more data available, create another data buffer in the series
              BufferedDataPtr data = BufferedData::create();

              ULONG allocSize = bytes;
              getBuffer(data->mBuffer, allocSize);
              data->mAllocSizeInBytes = allocSize;
              data->mBufferLengthInBytes = bytes;

              memcpy(data->mBuffer.get(), pos, bytes);

              // remember the read data
              mReadData.push_back(data);
            }
          }

          // recalculate the GSNFR information
          mGSNFR = bufferedPacket->mSequenceNumber;
          get(mXORedParityToGSNFR) = internal::logicalXOR(mXORedParityToGSNFR, bufferedPacket->mRUDPPacket->isFlagSet(RUDPPacket::Flag_PS_ParitySending));

          // the front packet can now be removed
          mReceivedPackets.erase(iter);
        }

        if (delivered) {
          ZS_LOG_TRACE(log("delivering notify read ready") + ", size=" + string(mReadData.size()))
          notifyReadReadyAgainIfData();
        }
      }

      //-----------------------------------------------------------------------
      ULONG RUDPChannelStream::getFromWriteBuffer(
                                                  BYTE *outBuffer,
                                                  ULONG maxFillSize
                                                  )
      {
        ZS_LOG_TRACE(log("get from write buffer") + ", max size=" + string(maxFillSize))
        ULONG readBytes = 0;

        while ((maxFillSize > 0) &&
               (mWriteData.size() > 0)) {
          BufferedDataPtr buffer = mWriteData.front();

          ULONG available = buffer->mBufferLengthInBytes - buffer->mConsumed;
          available = (available > maxFillSize ? maxFillSize : available);

          memcpy(outBuffer, &((buffer->mBuffer.get())[buffer->mConsumed]), available);
          outBuffer += available;
          maxFillSize -= available;
          readBytes += available;
          buffer->mConsumed += available;

          if (buffer->mConsumed == buffer->mBufferLengthInBytes) {
            // the entire buffer is exhasted, pop it off now
            mWriteData.pop_front();
          }
        }

        ZS_LOG_TRACE(log("get from write buffer") + ", max size=" + string(maxFillSize) + ", read size=" + string(readBytes))
        return readBytes;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::notifyReadReadyAgainIfData()
      {
        if (mReadData.size() < 1) {
          ZS_LOG_DEBUG(log("no data to read thus no need to notify about read ready"))
          return;
        }
        if (mInformedReadReady) {
          ZS_LOG_DEBUG(log("already notified that the read is ready thus no need to do it again"))
          return;
        }

        try {
          ZS_LOG_TRACE(log("notify more read data available"))
          mDelegate->onRUDPChannelStreamReadReady(mThisWeak.lock());
          get(mInformedReadReady) = true;
        } catch(IRUDPChannelStreamDelegateProxy::Exceptions::DelegateGone &) {
          setError(RUDPChannelStreamShutdownReason_DelegateGone, "delegate gone");
          cancel();
          return;
        }
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::notifyWriteReadyAgainIfSpace()
      {
        AutoRecursiveLock lock(mLock);

        if (!mDelegate) return;
        if (mInformedWriteReady) return;

        if (mWriteData.size() > 1) return;  // already have two buffers so don't advertise that there is more room

        try {
          ZS_LOG_TRACE(log("notify more write space available"))
          mDelegate->onRUDPChannelStreamWriteReady(mThisWeak.lock());
          get(mInformedWriteReady) = true;
        } catch(IRUDPChannelStreamDelegateProxy::Exceptions::DelegateGone &) {
          setError(RUDPChannelStreamShutdownReason_DelegateGone, "delegate gone");
          cancel();
          return;
        }
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::getBuffer(
                                        RecycleBuffer &outBuffer,
                                        ULONG &ioBufferAllocLengthInBytes
                                        )
      {
        if (ioBufferAllocLengthInBytes < OPENPEER_SERVICES_MINIMUM_DATA_BUFFER_LENGTH_ALLOCATED_IN_BYTES)
          ioBufferAllocLengthInBytes = OPENPEER_SERVICES_MINIMUM_DATA_BUFFER_LENGTH_ALLOCATED_IN_BYTES;

        if ((OPENPEER_SERVICES_MINIMUM_DATA_BUFFER_LENGTH_ALLOCATED_IN_BYTES != ioBufferAllocLengthInBytes) ||
            (mRecycleBuffers.size() < 1)) {
          outBuffer = RecycleBuffer(new BYTE[ioBufferAllocLengthInBytes]);
          return;
        }

        outBuffer = mRecycleBuffers.front();
        mRecycleBuffers.pop_front();
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::freeBuffer(
                                         RecycleBuffer &ioBuffer,
                                         ULONG bufferAllocLengthInBytes
                                         )
      {
        if ((OPENPEER_SERVICES_MINIMUM_DATA_BUFFER_LENGTH_ALLOCATED_IN_BYTES != bufferAllocLengthInBytes) ||
            (mRecycleBuffers.size() > OPENPEER_SERVICES_MAX_RECYCLE_BUFFERS)){
          ioBuffer.reset();
          return;
        }

        mRecycleBuffers.push_back(ioBuffer);
      }

      //-----------------------------------------------------------------------
      bool RUDPChannelStream::getRandomFlag()
      {
        ++mRandomPoolPos;
        if (mRandomPoolPos > (sizeof(mRandomPool)*8)) {
          get(mRandomPoolPos) = 0;

          CryptoPP::AutoSeededRandomPool rng;
          rng.GenerateBlock(&(mRandomPool[0]), sizeof(mRandomPool));
        }

        ULONG posByte = mRandomPoolPos / 8;
        ULONG posBit = mRandomPoolPos % 8;
        return (0 != (mRandomPool[posByte] & (1 << posBit)));
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::closeOnAllDataSent()
      {
        if (isShutdown()) return;       // already closed?
        if (!isShuttingDown()) return;  // do we want to close if all data is sent?

        if ((0 != mWriteData.size()) || (0 != mSendingPackets.size())) return;

        ZS_LOG_TRACE(log("all data sent and now will close stream"))

        // all data has already been delivered so cancel the connection now
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream::BufferedPacket
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPChannelStream::BufferedPacketPtr RUDPChannelStream::BufferedPacket::create()
      {
        BufferedPacketPtr pThis(new BufferedPacket);
        pThis->mSequenceNumber = 0;
        pThis->mTimeSentOrReceived = zsLib::now();
        pThis->mXORedParityToNow = false;
        pThis->mPacketLengthInBytes = 0;
        pThis->mHoldsBaton = false;
        pThis->mFlaggedAsFailedToReceive = false;
        pThis->mFlagForResendingInNextBurst = false;
        return pThis;
      }

      //-----------------------------------------------------------------------
      void  RUDPChannelStream::BufferedPacket::flagAsReceivedByRemoteParty(
                                                                           ULONG &ioTotalPacketsToResend,
                                                                           ULONG &ioAvailableBatons
                                                                           )
      {
        doNotResend(ioTotalPacketsToResend);
        releaseBaton(ioAvailableBatons);
        mPacket.reset();
        mPacketLengthInBytes = 0;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::BufferedPacket::flagForResending(ULONG &ioTotalPacketsToResend)
      {
        if (!mPacket) return;
        if (mFlagForResendingInNextBurst) return;
        mFlagForResendingInNextBurst = true;
        ++ioTotalPacketsToResend;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::BufferedPacket::doNotResend(ULONG &ioTotalPacketsToResend)
      {
        if (!mFlagForResendingInNextBurst) return;
        mFlagForResendingInNextBurst = false;
        ZS_THROW_BAD_STATE_IF(0 == ioTotalPacketsToResend)
        --ioTotalPacketsToResend;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::BufferedPacket::consumeBaton(ULONG &ioAvailableBatons)
      {
        if (mHoldsBaton) return;
        if (0 == ioAvailableBatons) return;
        mHoldsBaton = true;
        --ioAvailableBatons;
      }

      //-----------------------------------------------------------------------
      void RUDPChannelStream::BufferedPacket::releaseBaton(ULONG &ioAvailableBatons)
      {
        if (!mHoldsBaton) return;
        mHoldsBaton = false;
        ++ioAvailableBatons;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPChannelStream::BufferedData
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPChannelStream::BufferedDataPtr RUDPChannelStream::BufferedData::create()
      {
        BufferedDataPtr pThis(new BufferedData);
        pThis->mBufferLengthInBytes = 0;
        pThis->mConsumed = 0;
        pThis->mAllocSizeInBytes = 0;
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

    }
  }
}

#pragma warning(pop)
