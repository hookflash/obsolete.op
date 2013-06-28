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
#include <openpeer/services/IRUDPChannel.h>
#include <zsLib/Proxy.h>

#include <boost/shared_array.hpp>

#include <list>

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
      #pragma mark IRUDPChannelStream
      #pragma mark

      interaction IRUDPChannelStream
      {
        enum RUDPChannelStreamStates
        {
          RUDPChannelStreamState_Connected,
          RUDPChannelStreamState_DirectionalShutdown,
          RUDPChannelStreamState_ShuttingDown,
          RUDPChannelStreamState_Shutdown,
        };

        static const char *toString(RUDPChannelStreamStates);

        enum RUDPChannelStreamShutdownReasons
        {
          RUDPChannelStreamShutdownReason_None                      = IRUDPChannel::RUDPChannelShutdownReason_None,

          RUDPChannelStreamShutdownReason_Closed                    = IRUDPChannel::RUDPChannelShutdownReason_Closed,

          RUDPChannelStreamShutdownReason_OpenFailure               = IRUDPChannel::RUDPChannelShutdownReason_OpenFailure,
          RUDPChannelStreamShutdownReason_DelegateGone              = IRUDPChannel::RUDPChannelShutdownReason_DelegateGone,
          RUDPChannelStreamShutdownReason_Timeout                   = IRUDPChannel::RUDPChannelShutdownReason_Timeout,
          RUDPChannelStreamShutdownReason_IllegalStreamState        = IRUDPChannel::RUDPChannelShutdownReason_IllegalStreamState,
        };

        static const char *toString(RUDPChannelStreamShutdownReasons reason);

        typedef IRUDPChannel::CongestionAlgorithms CongestionAlgorithms;
        typedef IRUDPChannel::Shutdown Shutdown;

        typedef std::list<CongestionAlgorithms> CongestionAlgorithmList;

        //-----------------------------------------------------------------------
        // PURPOSE: Use these values as the default for any new session created.
        static void getRecommendedStartValues(
                                              QWORD &outRecommendedNextSequenceNumberForSending,
                                              DWORD &outMinimumRecommendedRTTInMilliseconds,
                                              CongestionAlgorithmList &outLocalAlgorithms,
                                              CongestionAlgorithmList &outRemoteAlgoirthms
                                              );

        //-----------------------------------------------------------------------
        // PURPOSE: Determine which algorithms to respond to a remote party.
        // RETURNS: false if the algorithms can't be negotiated otherwise true.
        //          Returns algorithms to negotiate in out parameters. Empty
        //          lists mean that the offered/preferred algorithm was selected.
        static bool getResponseToOfferedAlgorithms(
                                                   const CongestionAlgorithmList &offeredAlgorithmsForLocal,
                                                   const CongestionAlgorithmList &offeredAlgorithmsForRemote,
                                                   CongestionAlgorithmList &outResponseAlgorithmsForLocal,
                                                   CongestionAlgorithmList &outResponseAlgorithmsForRemote
                                                   );

        static IRUDPChannelStreamPtr create(
                                            IMessageQueuePtr queue,
                                            IRUDPChannelStreamDelegatePtr delegate,
                                            QWORD nextSequenceNumberToUseForSending,      // this value must be at least "1"
                                            QWORD nextSequenberNumberExpectingToReceive,  // this value must be at least "1" and is dictated by remote party
                                            WORD sendingChannelNumber,                    // the channel number is dictated by remote party
                                            WORD receivingChannelNumber,                  // the channel number chosen locally should not conflict with any other streams on the same socket or any TURN allocations used on the same socket
                                            DWORD minimumNegotiatedRTTInMilliseconds,     // this value cannot be set lower than the offered RTT
                                            CongestionAlgorithms algorithmForLocal = IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp,
                                            CongestionAlgorithms algorithmForRemote = IRUDPChannel::CongestionAlgorithm_TCPLikeWindowWithSlowCreepUp
                                            );

        virtual PUID getID() const = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Gets the current state of the channel stream.
        virtual RUDPChannelStreamStates getState() const = 0;

        virtual RUDPChannelStreamShutdownReasons getShutdownReason() const = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Close the stream down to prevent further usage.
        // NOTE:    A stream has no capacity to time out the connection. However,
        //          a stream will require external ACKs to be delivered if the
        //          remote side is not acknowledging packets. The caller is
        //          responsible for timing out the connection should one of
        //          these external ACK requests fail to deliver.
        //
        //          If calling with shutdownOnlyOnceAllDataSent set to false then
        //          shutdown will occur immediately.
        virtual void shutdown(bool shutdownOnlyOnceAllDataSent = false) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Prevents more data from being received from the remote party
        //          or additional data from being sent to the remote party (or
        //          both).
        // NOTE:    This does not close the stream. Data that has already been
        //          put into the send buffer will be delivered but additional
        //          send data will be rejected.
        virtual void shutdownDirection(Shutdown state) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Prevents any new data from being sent out until the
        //          remote party sends a sequence number equal to or greater than
        //          to the passed-in sequence number.
        // NOTE:    Passing in a sequence number of "0" will cause the hold to be
        //          removed immediately.
        virtual void holdSendingUntilReceiveSequenceNumber(QWORD sequenceNumber) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Call to read data from the stream.
        // RETURNS: The total number of bytes read from the stream.
        // NOTE:    If the caller fails to read the entire amount from the stream
        //          an onRUDPChannelStreamReadReady(...) is notified to the delegate.
        virtual ULONG receive(
                              BYTE *outBuffer,
                              ULONG bufferSizeAvailableInBytes
                              ) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Call to obtain how much data is available for reading from
        //          the stream.
        virtual bool doesReceiveHaveMoreDataAvailable() = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Call to obtain how much data is available for reading from
        //          the stream.
        virtual ULONG getReceiveSizeAvailableInBytes() = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Call to buffer data into the stream ready to send.
        // NOTE:    All data sent in will be buffered at infinum even if the
        //          buffer is considered "full". Be careful to pace the data
        //          being sent in to prevent flooding the socket.
        //          Use getSendSize() and the onRUDPChannelStreamWriteReady(...) to
        //          help control flooding the stream.
        virtual bool send(
                          const BYTE *buffer,
                          ULONG bufferLengthInBytes
                          ) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Call to obtain how much space is available in the outgoing
        //          buffer.
        virtual ULONG getSendSize() = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Cause the RUDP stream to handle an incoming packet.
        // RETURNS: If the packet was handled it will return true. Otherwise
        //          it will return false.
        virtual bool handlePacket(
                                  RUDPPacketPtr packet,
                                  boost::shared_array<BYTE> originalBuffer,
                                  ULONG originalBufferLengthInBytes,
                                  bool ecnMarked
                                  ) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Notify the stream that the socket has notified that it is
        //          available for writing at this time.
        virtual void notifySocketWriteReady() = 0;                                    // notification that the socket is available for writing now

        //-----------------------------------------------------------------------
        // PURPOSE: Handle an ACK that did not arrive outside the RUDPChannelStream.
        virtual void handleExternalAck(
                                       PUID guarenteedDeliveryRequestID,   // if this was from a guarenteed delivery, what is the request ID that caused it?
                                       QWORD nextSequenceNumber,           // the next sequence number to be sent by remote party (which has not been sent yet)
                                       QWORD greatestSequenceNumberReceived,       // GSNR
                                       QWORD greatestSequenceNumberFullyReceived,  // GSNFR
                                       const BYTE *externalVector,         // Vector buffer is RLE encoded according to RUDP protocol specification. Pass in NULL if not specified
                                       ULONG externalVectorLengthInBytes,  // pass in 0 if not specified
                                       bool vpFlag,                               // XORed parity of all packets marked as received in the vector
                                       bool pgFlag,                               // parity of GSNR
                                       bool xpFlag,                               // XORed parity of all packets up-to-and-including GSNFR
                                       bool dpFlag,                               // received duplicate packet since last ACK
                                       bool ecFlag                                // ECN marked packet arrived since last ACK sent
                                       ) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Get the internal state of what is going on within the
        //          RUDPChannelStream (which can be used to generate an external ACK).
        virtual void getState(
                              QWORD &outNextSequenceNumber,                   // the next sequence number that will be sent (which has not been sent yet)
                              QWORD &outGreatestSequenceNumberReceived,       // GSNR
                              QWORD &outGreatestSequenceNumberFullyReceived,  // GSNFR
                              BYTE *outVector,                             // Buffer for output vector. Vector buffer is RLE encoded according to RUDP protocol specification. Pass in NULL to prevent generation of a vector.
                              ULONG &outVectorSizeInBytes,                 // how much of the vector buffer was filled
                              ULONG maxVectorSizeInBytes,                  // the maximum size in BYTEs allowed to fill inside the vector
                              bool &outVPFlag,                                    // XORed parity of all packets marked as received in the vector
                              bool &outPGFlag,                                    // parity of GSNR
                              bool &outXPFlag,                                    // XORed parity of all packets up-to-and-including GSNFR
                              bool &outDPFlag,                                    // duplicate packet was detected
                              bool &outECFlag                                     // ECN marked packet arrived since last ACK sent
                              ) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Notify that an external ACK has been sent to the remote
        //          party.
        virtual void notifyExternalACKSent(QWORD ackedSequenceNumber) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRUDPChannelStreamDelegate
      #pragma mark

      interaction IRUDPChannelStreamDelegate
      {
        typedef IRUDPChannelStream::RUDPChannelStreamStates RUDPChannelStreamStates;

        //-----------------------------------------------------------------------
        // PURPOSE: Notifies that the stream state has changed.
        virtual void onRUDPChannelStreamStateChanged(
                                                     IRUDPChannelStreamPtr stream,
                                                     RUDPChannelStreamStates state
                                                     ) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Notifies that data is available for reading in the stream.
        virtual void onRUDPChannelStreamReadReady(IRUDPChannelStreamPtr stream) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Notifies that space is available for writing to the stream.
        virtual void onRUDPChannelStreamWriteReady(IRUDPChannelStreamPtr stream) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Send a packet over the socket interface to the remote party.
        virtual bool notifyRUDPChannelStreamSendPacket(
                                                       IRUDPChannelStreamPtr stream,
                                                       const BYTE *packet,
                                                       ULONG packetLengthInBytes
                                                       ) = 0;

        //-----------------------------------------------------------------------
        // PURPOSE: Send a packet over the socket interface to the remote party.
        virtual void onRUDPChannelStreamSendExternalACKNow(
                                                           IRUDPChannelStreamPtr stream,
                                                           bool guarenteeDelivery,
                                                           PUID guarenteeDeliveryRequestID = 0
                                                           ) = 0;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::internal::IRUDPChannelStreamDelegate)
ZS_DECLARE_PROXY_METHOD_2(onRUDPChannelStreamStateChanged, openpeer::services::internal::IRUDPChannelStreamPtr, openpeer::services::internal::IRUDPChannelStreamDelegate::RUDPChannelStreamStates)
ZS_DECLARE_PROXY_METHOD_1(onRUDPChannelStreamReadReady, openpeer::services::internal::IRUDPChannelStreamPtr)
ZS_DECLARE_PROXY_METHOD_1(onRUDPChannelStreamWriteReady, openpeer::services::internal::IRUDPChannelStreamPtr)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_3(notifyRUDPChannelStreamSendPacket, bool, openpeer::services::internal::IRUDPChannelStreamPtr, const zsLib::BYTE *, zsLib::ULONG)
ZS_DECLARE_PROXY_METHOD_3(onRUDPChannelStreamSendExternalACKNow, openpeer::services::internal::IRUDPChannelStreamPtr, bool, zsLib::PUID)
ZS_DECLARE_PROXY_END()
