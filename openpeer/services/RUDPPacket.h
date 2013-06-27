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

#include <openpeer/services/types.h>
#include <boost/shared_array.hpp>

#include <zsLib/Log.h>

#define HOOKFLASH_SERVICES_RUDP_MAX_PACKET_SIZE_WHEN_PMTU_IS_NOT_KNOWN (512)

namespace hookflash
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark RUDPPacket
    #pragma mark

    struct RUDPPacket
    {
      enum Flags
      {
        Flag_PS_ParitySending =         (1 << 7),
        Flag_PG_ParityGSNR =            (1 << 6),
        Flag_XP_XORedParityToGSNFR =    (1 << 5),
        Flag_DP_DuplicatePacket =       (1 << 4),
        Flag_EC_ECNPacket =             (1 << 3),
        Flag_EQ_GSNREqualsGSNFR =       (1 << 2),
        Flag_AR_ACKRequired =           (1 << 1)
      };

      enum VectorFlags
      {
        Flag_VP_VectorParity =          (1 << 7)
      };

      enum VectorStates
      {
        VectorState_Received =           (0x00 << 6),
        VectorState_ReceivedECNMarked =  (0x01 << 6),
        VectorState_NotReceived =        (0x03 << 6),

        VectorState_Reserved =           (0x02 << 6),

        VectorState_NoMoreData =         (0xFF),
      };

      struct VectorEncoderState
      {
        VectorStates mLastState;
        bool mXORedParityToNow;
        BYTE *mVector;
        ULONG mVectorFilledLengthInBytes;
        ULONG mMaxVectorSizeInBytes;
        QWORD mGSNR;
        QWORD mGSNFR;
        QWORD mCurrentSequenceNumber;
      };

      struct VectorDecoderState
      {
        const BYTE *mVector;
        ULONG mVectorFilledLengthInBytes;
        ULONG mConsumedRLE;
        BYTE mSpecialCaseVector[2];
      };

      static RUDPPacketPtr create();
      RUDPPacketPtr clone();

      static RUDPPacketPtr parseIfRUDP(
                                       const BYTE *packet,
                                       ULONG packetLengthInBytes
                                       );

      void packetize(
                     boost::shared_array<BYTE> &outBuffer,
                     ULONG &outBufferLengthInBytes
                     ) const;

      bool isFlagSet(Flags flag) const;
      bool isFlagSet(VectorFlags flag) const;
      void setFlag(Flags flag);
      void setFlag(Flags flag, bool on);
      void setFlag(VectorFlags flag);
      void setFlag(VectorFlags flag, bool on);
      void clearFlag(Flags flag);
      void clearFlag(VectorFlags flag);

      QWORD getSequenceNumber(QWORD hintLastSequenceNumber) const;
      QWORD getGSNR(QWORD hintLastGSNR) const;
      QWORD getGSNFR(QWORD hintLastGSNFR) const;  // only can call if the Flag_EQ_GSNREqualsGSNFR is *NOT* set, otherwise an exception will be thrown

      void setSequenceNumber(QWORD sequenceNumber);
      void setGSN(
                  QWORD gsnr,  // Greatest Sequence Number Received
                  QWORD gsnfr  // Greatest Sequence Number Fully Received
                  );

      ULONG getRoomAvailableForData(ULONG maxPacketLengthInBytes) const;

      void vectorEncoderStart(
                              VectorEncoderState &outVectorState,
                              QWORD gsnr,
                              QWORD gsnfr,
                              bool xoredParityToGSNFR
                              );

      static void vectorEncoderStart(
                                     VectorEncoderState &outVectorState,
                                     QWORD gsnr,
                                     QWORD gsnfr,
                                     bool xoredParityToGSNFR,
                                     BYTE *vector,                       // if pointer specified, will encode to an external buffer
                                     ULONG vectorLengthInBytes           // if external buffer used, specify the max length of the external buffer
                                     );

      // will return false if there is no more room in the vector for more information
      static bool vectorEncoderAdd(
                                   VectorEncoderState &ioVectorState,
                                   VectorStates vectorState,
                                   bool packetParity                            // this only applicable if the packet was received, otherwise it will be ignored
                                   );

      void vectorEncoderFinalize(VectorEncoderState &ioVectorState);
      static void vectorEncoderFinalize(
                                        VectorEncoderState &ioVectorState,
                                        bool &outXORVectorParityFlag,
                                        ULONG &outVectorLengthInBytes
                                        );

      void vectorDecoderStart(VectorDecoderState &ioVectorState) const;
      static void vectorDecoderStart(
                                     VectorDecoderState &ioVectorState,
                                     const BYTE *vector,                 // if pointer specified, will encode to an external buffer
                                     ULONG vectorLengthInBytes,          // if external buffer used, specify the max length of the external buffer
                                     QWORD gsnr,
                                     QWORD gsnfr
                                     );

      static VectorStates vectorDecoderGetNextPacketState(VectorDecoderState &outVectorState);

      void log(
               Log::Level level = Log::Debug,
               const char *message = NULL
               ) const;

    public:
      WORD mChannelNumber;
      DWORD mSequenceNumber;             // only lower 24bits are valid and all upper bits are set to zero

      BYTE mFlags;
      DWORD mGSNR;                       // Greatest Sequence Number Received - only lower 24bis are valid and all upper bits are set to zero
      DWORD mGSNFR;                      // Greatest Sequence Number Fully Received - only valid if Flag_EQ_GSNREqualsGSNFR is not set and only lower 24bis are valid and all upper bits are set to zero

      BYTE mVectorFlags;                 // contains the vector flags
      BYTE mVectorLengthInBytes;         // how big is the length of the vector in bytes (only lower 7 bits are valid and upper bit is always zero)
      BYTE mVector[128];                 // contains the vector as RLE data

      const BYTE *mData;                 // pointer to data (only valid as long as buffer associated is valid)
      WORD mDataLengthInBytes;           // how big is the data buffer in bytes
    };
  }
}
