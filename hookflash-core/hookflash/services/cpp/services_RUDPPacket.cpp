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

#include <hookflash/services/RUDPPacket.h>
#include <zsLib/Exception.h>
#include <zsLib/Stringize.h>

#ifdef _LINUX
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif //_LINUX

namespace hookflash { namespace services { ZS_DECLARE_SUBSYSTEM(hookflash_services) } }

#define HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES (12)

namespace hookflash
{
  namespace services
  {
    using zsLib::Stringize;

    namespace internal
    {
      //-----------------------------------------------------------------------
      String convertToHex(const BYTE *buffer, ULONG bufferLengthInBytes);

      //-----------------------------------------------------------------------
      static ULONG dwordBoundary(ULONG length) {
        if (0 == (length % sizeof(DWORD)))
          return length;
        return length + (sizeof(DWORD) - (length % sizeof(DWORD)));
      }

      static QWORD getNumberWithHint(DWORD number, QWORD hint) {
        number &= 0xFFFFFF; // only 24 bits are valid
#if UINT_MAX <= 0xFFFFFFFF
        QWORD temp = 1;
        temp = (temp << 48)-1;
        temp = (temp << 24);
        QWORD upper = (hint & temp);
#else
        QWORD upper = (hint & 0xFFFFFFFFFF000000);
#endif

        QWORD merged = (upper | ((QWORD)number));
        if (merged < hint) {
          // two possibe - because it is in fact a lower or because it should have wrapped
          QWORD higher = ((upper + 0x1000000) | ((QWORD)number));
          QWORD diff1 = higher - hint;
          QWORD diff2 = hint - merged;

          // which ever is closer to the hint is likely to be the correct value
          return (diff1 < diff2 ? higher : merged);
        }

        // merged is higher, but maybe it should be lower because it wrapped...

        if (upper < 0x1000000) return merged; // could not have wrapped if the upper isn't beyond 24 bits in size

        QWORD lower = ((upper - 0x1000000) | ((QWORD)number));
        QWORD diff1 = hint - lower;
        QWORD diff2 = merged - hint;

        // which ever is closer to the hint value is likely to be the correct value
        return (diff1 < diff2 ? lower : merged);
      }

      //-----------------------------------------------------------------------
      static bool logicalXOR(bool value1, bool value2) {
        return (0 != ((value1 ? 1 : 0) ^ (value2 ? 1 : 0)));
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    RUDPPacketPtr RUDPPacket::create()
    {
      RUDPPacketPtr pThis(new RUDPPacket);
      pThis->mChannelNumber = 0;
      pThis->mSequenceNumber = 0;
      pThis->mFlags = 0;
      pThis->mGSNR = 0;
      pThis->mGSNFR = 0;
      pThis->mVectorFlags = 0;
      pThis->mVectorLengthInBytes = 0;
      memset(&(pThis->mVector[0]), 0, sizeof(pThis->mVector));
      pThis->mData = NULL;
      pThis->mDataLengthInBytes = 0;
      return pThis;
    }

    //-------------------------------------------------------------------------
    RUDPPacketPtr RUDPPacket::clone()
    {
      RUDPPacketPtr pThis(new RUDPPacket);
      pThis->mChannelNumber = mChannelNumber;
      pThis->mSequenceNumber = mSequenceNumber;
      pThis->mFlags = mFlags;
      pThis->mGSNR = mGSNR;
      pThis->mGSNFR = mGSNFR;
      pThis->mVectorFlags = mVectorFlags;
      pThis->mVectorLengthInBytes = mVectorLengthInBytes;
      memcpy(&(pThis->mVector[0]), &(mVector[0]), sizeof(pThis->mVector));
      pThis->mData = mData;
      pThis->mDataLengthInBytes = mDataLengthInBytes;
      return pThis;
    }

    //-------------------------------------------------------------------------
    RUDPPacketPtr RUDPPacket::parseIfRUDP(
                                          const BYTE *packet,
                                          ULONG packetLengthInBytes
                                          )
    {
      ZS_THROW_INVALID_USAGE_IF(!packet)
      if (packetLengthInBytes < HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES) return RUDPPacketPtr();  // does not meet the minimum size expectations so it can't be RUDP

      WORD channelNumber = ntohs(((WORD *)packet)[0]);
      WORD dataLength = ntohs(((WORD *)packet)[1]);

      BYTE flags = packet[sizeof(WORD)+sizeof(WORD)];
      DWORD sequenceNumber = ntohl(((DWORD *)packet)[1]) & 0xFFFFFF;  // lower 24bits are valid only
      DWORD gsnr = ntohl(((DWORD *)packet)[2]) & 0xFFFFFF;            // lower 24bits are valid only
      DWORD gsnfr = gsnr;
      BYTE vectorSize = 0;
      BYTE vectorFlags = 0;

      bool eqFlag = (0 != (flags & Flag_EQ_GSNREqualsGSNFR));
      if (!eqFlag) {
        if (packetLengthInBytes < HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES + sizeof(DWORD)) return RUDPPacketPtr();  // does not meet the minimum size expectations so it can't be RUDP
        vectorSize = (packet[sizeof(DWORD)*3]) & (0x7F);
        vectorFlags = (packet[sizeof(DWORD)*3]) & (0x80);
        gsnfr = ntohl(((DWORD *)packet)[3]) & 0xFFFFFF;               // lower 24bits are valid only
      }

      // has to have enough room to contain vector, extended header and all data
      if (packetLengthInBytes < HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES + ((!eqFlag) ? sizeof(DWORD) : 0) + internal::dwordBoundary(vectorSize) + ((ULONG)dataLength)) return RUDPPacketPtr();  // does not meet the minimum size expectations so it can't be RUDP

      // this appears to be RUDP
      RUDPPacketPtr pThis = create();
      pThis->mChannelNumber = channelNumber;
      pThis->mSequenceNumber = sequenceNumber;
      pThis->mFlags = flags;
      pThis->mGSNR = gsnr;
      pThis->mGSNFR = gsnfr;
      pThis->mVectorFlags = vectorFlags;
      pThis->mVectorLengthInBytes = vectorSize;
      if (0 != vectorSize) {
        memcpy(&(pThis->mVector[0]), &(packet[sizeof(DWORD)*4]), vectorSize);
      }

      if (0 != dataLength) {
        pThis->mData = &(packet[HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES + ((!eqFlag) ? sizeof(DWORD) : 0) + internal::dwordBoundary(vectorSize)]);
        pThis->mDataLengthInBytes = dataLength;
      }
      pThis->log(Log::Trace, "parse");
      return pThis;
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::packetize(
                               boost::shared_array<BYTE> &outBuffer,
                               ULONG &outBufferLengthInBytes
                               ) const
    {
      log(Log::Trace, "packetize");
      ZS_THROW_BAD_STATE_IF(mDataLengthInBytes > 0xFFFF)

      bool eqFlag = (0 != (mFlags & Flag_EQ_GSNREqualsGSNFR));
      ZS_THROW_BAD_STATE_IF(eqFlag && ((mGSNR & 0xFFFFFF) != (mGSNFR & 0xFFFFFF))) // they must match if the EQ flag is set to true or this is illegal

      ULONG length = HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES + ((!eqFlag) ? sizeof(DWORD) : 0) + internal::dwordBoundary(mVectorLengthInBytes) + mDataLengthInBytes;

      outBuffer = boost::shared_array<BYTE>(new BYTE[length]);
      outBufferLengthInBytes = length;

      BYTE *packet = outBuffer.get();
      memset(packet, 0, length);  // make sure to set the entire packet to "0" so all defaults are appropriately set

      // put in channel number and length
      ((WORD *)packet)[0] = htons(mChannelNumber);
      ((WORD *)packet)[1] = htons(mDataLengthInBytes);

      ((DWORD *)packet)[1] = htonl(mSequenceNumber & 0xFFFFFF);
      (packet[sizeof(DWORD)]) = mFlags;
      ((DWORD *)packet)[2] = htonl(mGSNR & 0xFFFFFF);
      if (!eqFlag) {
        ((DWORD *)packet)[3] = htonl(mGSNFR & 0xFFFFFF);
        ZS_THROW_BAD_STATE_IF(mVectorLengthInBytes > 0x7F)  // can only have from 0..127 bytes in the vector maximum

        (packet[sizeof(DWORD)*3]) = (mVectorLengthInBytes & 0x7F);
        (packet[sizeof(DWORD)*3]) = (packet[sizeof(DWORD)*3]) | (mVectorFlags & 0x80);  // add the vector parity flag

        // copy in the vector if it is present
        if (0 != mVectorLengthInBytes) {
          ZS_THROW_BAD_STATE_IF(mVectorLengthInBytes > 0x7F)  // this is illegal
          memcpy(&(packet[HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES + sizeof(DWORD)]), &(mVector[0]), mVectorLengthInBytes);
        }
      }

      if (0 != mDataLengthInBytes) {
        ZS_THROW_BAD_STATE_IF(NULL == mData)  // cannot have set a length but forgot to specify the pointer
        memcpy(&(packet[HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES + ((!eqFlag) ? sizeof(DWORD) : 0) + internal::dwordBoundary(mVectorLengthInBytes)]), &(mData[0]), mDataLengthInBytes);
      }
    }

    //-------------------------------------------------------------------------
    bool RUDPPacket::isFlagSet(Flags flag) const
    {
      return (flag == (mFlags & flag));
    }

    //-------------------------------------------------------------------------
    bool RUDPPacket::isFlagSet(VectorFlags flag) const
    {
      return (flag == (mVectorFlags & flag));
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::setFlag(Flags flag)
    {
      mFlags |= flag;
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::setFlag(VectorFlags flag)
    {
      mVectorFlags |= flag;
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::setFlag(Flags flag, bool on)
    {
      if (on)
        setFlag(flag);
      else
        clearFlag(flag);
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::setFlag(VectorFlags flag, bool on)
    {
      if (on)
        setFlag(flag);
      else
        clearFlag(flag);
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::clearFlag(Flags flag)
    {
      mFlags = (mFlags & (0xFF ^ flag));
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::clearFlag(VectorFlags flag)
    {
      mVectorFlags = (mVectorFlags & (0xFF ^ flag));
    }

    //-------------------------------------------------------------------------
    QWORD RUDPPacket::getSequenceNumber(QWORD hintLastSequenceNumber) const
    {
      return internal::getNumberWithHint(mSequenceNumber, hintLastSequenceNumber);
    }

    //-------------------------------------------------------------------------
    QWORD RUDPPacket::getGSNR(QWORD hintLastGSNR) const
    {
      return internal::getNumberWithHint(mGSNR, hintLastGSNR);
    }

    //-------------------------------------------------------------------------
    QWORD RUDPPacket::getGSNFR(QWORD hintLastGSNFR) const
    {
      return internal::getNumberWithHint(mGSNFR, hintLastGSNFR);
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::setSequenceNumber(QWORD sequenceNumber)
    {
      mSequenceNumber = ((DWORD)(sequenceNumber & 0xFFFFFF));
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::setGSN(
                            QWORD gsnr,
                            QWORD gsnfr
                            )
    {
      // if they are equal then we need to set the EQ flag
      if (gsnr == gsnfr)
        setFlag(Flag_EQ_GSNREqualsGSNFR);
      else
        clearFlag(Flag_EQ_GSNREqualsGSNFR);

      mGSNR = ((DWORD)(gsnr & 0xFFFFFF));
      mGSNFR = ((DWORD)(gsnfr & 0xFFFFFF));
    }

    ULONG RUDPPacket::getRoomAvailableForData(ULONG maxPacketLengthInBytes) const
    {
      bool eqFlag = (0 != (mFlags & Flag_EQ_GSNREqualsGSNFR));
      ZS_THROW_BAD_STATE_IF(eqFlag & ((mGSNR & 0xFFFFFF) != (mGSNFR & 0xFFFFFF))) // they must match if the EQ flag is set to true or this is illegal

      ULONG length = HOOKFLASH_SERVICES_MINIMUM_PACKET_LENGTH_IN_BYTES + ((!eqFlag) ? sizeof(DWORD) : 0) + internal::dwordBoundary(mVectorLengthInBytes);
      if (length > maxPacketLengthInBytes) return 0;

      return maxPacketLengthInBytes - length;
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::vectorEncoderStart(
                                        VectorEncoderState &outVectorState,
                                        QWORD gsnr,
                                        QWORD gsnfr,
                                        bool xoredParityToGSNFR
                                        )
    {
      ZS_THROW_INVALID_ASSUMPTION_IF(0x7F > sizeof(mVector))
      vectorEncoderStart(outVectorState, gsnr, gsnfr, xoredParityToGSNFR, &(mVector[0]), 0x7F);
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::vectorEncoderStart(
                                        VectorEncoderState &outVectorState,
                                        QWORD gsnr,
                                        QWORD gsnfr,
                                        bool xoredParityToGSNFR,
                                        BYTE *vector,
                                        ULONG vectorLengthInBytes
                                        )
    {
      ZS_THROW_INVALID_USAGE_IF(NULL == vector)
      outVectorState.mLastState = VectorState_Received;
      outVectorState.mXORedParityToNow = xoredParityToGSNFR;
      outVectorState.mVector = vector;
      outVectorState.mVectorFilledLengthInBytes = 0;
      outVectorState.mMaxVectorSizeInBytes = vectorLengthInBytes;
      outVectorState.mGSNR = gsnr;
      outVectorState.mGSNFR = gsnfr;
      outVectorState.mCurrentSequenceNumber = gsnfr;
    }

    //-------------------------------------------------------------------------
    bool RUDPPacket::vectorEncoderAdd(
                                      VectorEncoderState &ioVectorState,
                                      VectorStates vectorState,
                                      bool packetParityIfReceived
                                      )
    {
      ZS_THROW_INVALID_ASSUMPTION_IF(NULL == ioVectorState.mVector)
      ZS_THROW_INVALID_USAGE_IF(VectorState_NoMoreData == vectorState)

      ++(ioVectorState.mCurrentSequenceNumber);
      if (ioVectorState.mCurrentSequenceNumber >= ioVectorState.mGSNR) {
        // we never encode to (or beyond) the GSNR as it is completely pointless!
        return false;
      }

      if (0 == ioVectorState.mVectorFilledLengthInBytes) {
        if (0 == ioVectorState.mMaxVectorSizeInBytes)
          return false; // why would you do this?? give us a vector size of "0"?? seems rather silly to me...

        // this is a special case where we need to allocate the first BYTE for the first time
        ioVectorState.mVector[0] = vectorState;
        ioVectorState.mVector[0] |= 1;          // now has a RLE of 1
        if (vectorState != VectorState_NotReceived)
          ioVectorState.mXORedParityToNow = internal::logicalXOR(ioVectorState.mXORedParityToNow, packetParityIfReceived);
        ioVectorState.mVectorFilledLengthInBytes = 1;
        ioVectorState.mLastState = vectorState;
        return true;
      }

      if ((ioVectorState.mLastState != vectorState) ||
          ((ioVectorState.mVector[0] & 0x3F) == 0x3F)) {  // in the RLE we only have room up to 3F for each BYTE encoded (ie. a RLE of 63)
        // we need to add another BYTE to the vector - but is there room?
        if (ioVectorState.mVectorFilledLengthInBytes >= ioVectorState.mMaxVectorSizeInBytes)
          return false; // there is no more room! sorry!

        // there is room, grab the next BYTE and use it
        ++(ioVectorState.mVector);  // point to the next position in the vector
        ioVectorState.mVector[0] = vectorState;
        ioVectorState.mVector[0] |= 1;          // now has a RLE of 1
        if (vectorState != VectorState_NotReceived)
          ioVectorState.mXORedParityToNow = internal::logicalXOR(ioVectorState.mXORedParityToNow, packetParityIfReceived);
        ++(ioVectorState.mVectorFilledLengthInBytes);
        ioVectorState.mLastState = vectorState;
        return true;
      }

      // we have room and we are not changing states...
      ioVectorState.mVector[0] = (ioVectorState.mVector[0] & 0xC0) | ((ioVectorState.mVector[0] & 0x3F) + 1); // add one to the RLE
      if (vectorState != VectorState_NotReceived)
        ioVectorState.mXORedParityToNow = internal::logicalXOR(ioVectorState.mXORedParityToNow, packetParityIfReceived);
      return true;
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::vectorEncoderFinalize(VectorEncoderState &ioVectorState)
    {
      ZS_THROW_INVALID_ASSUMPTION_IF(NULL == ioVectorState.mVector)

      bool xorParity = false;
      ULONG vectorLengthInBytes = 0;
      vectorEncoderFinalize(ioVectorState, xorParity, vectorLengthInBytes);

      setFlag(Flag_VP_VectorParity, xorParity);
      mVectorLengthInBytes = static_cast<BYTE>(vectorLengthInBytes);
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::vectorEncoderFinalize(
                                           VectorEncoderState &ioVectorState,
                                           bool &outXORVectorParityFlag,
                                           ULONG &outVectorLengthInBytes
                                           )
    {
      ZS_THROW_INVALID_ASSUMPTION_IF(NULL == ioVectorState.mVector)

      outXORVectorParityFlag = ioVectorState.mXORedParityToNow;
      outVectorLengthInBytes = 0;

      do {
        if (0 == ioVectorState.mVectorFilledLengthInBytes) {
          // you never even added any packets to the vector
          break;
        }

        if (ioVectorState.mGSNFR == ioVectorState.mGSNR) {
          // nothing was encoded since both packets have effectively been received
          break;
        }

        if (1 == ioVectorState.mVectorFilledLengthInBytes) {
          // detect a special case where all packets are missing between the GSNR and the GSNFR as we don't need a vector to encode that
          if (ioVectorState.mLastState == VectorState_NotReceived) {
            // no need for a vector when all packets from the GSNFR to the GSNR are the missing
            break;
          }
        }

        // in all other cases you wanted a vector so now you have one...
        outVectorLengthInBytes = ioVectorState.mVectorFilledLengthInBytes;
      } while (false);
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::vectorDecoderStart(VectorDecoderState &ioVectorState) const
    {
      ZS_THROW_INVALID_ASSUMPTION_IF(mVectorLengthInBytes > sizeof(mVector))
      vectorDecoderStart(ioVectorState, &(mVector[0]), mVectorLengthInBytes, mGSNR, mGSNFR);
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::vectorDecoderStart(
                                        VectorDecoderState &outVectorState,
                                        const BYTE *vector,
                                        ULONG vectorLengthInBytes,
                                        QWORD gsnr,
                                        QWORD gsnfr
                                        )
    {
      outVectorState.mVector = vector;
      outVectorState.mVectorFilledLengthInBytes = vectorLengthInBytes;
      outVectorState.mConsumedRLE = 0;
      memset(&(outVectorState.mSpecialCaseVector[0]), 0, sizeof(outVectorState.mSpecialCaseVector));

      if ((gsnr != gsnfr) &&
          (0 == vectorLengthInBytes)) {

        // this is a special case vector where we will fill the packet with not received
        outVectorState.mSpecialCaseVector[0] = VectorState_NotReceived;
        outVectorState.mSpecialCaseVector[0] = (outVectorState.mSpecialCaseVector[0] | ((gsnr - gsnfr - 1) & 0x3F));
        outVectorState.mVector = &(outVectorState.mSpecialCaseVector[0]);
        outVectorState.mVectorFilledLengthInBytes = 1;
      }
    }

    //-------------------------------------------------------------------------
    RUDPPacket::VectorStates RUDPPacket::vectorDecoderGetNextPacketState(VectorDecoderState &ioVectorState)
    {
      if (NULL == ioVectorState.mVector) return VectorState_NoMoreData;
      if (0 == ioVectorState.mVectorFilledLengthInBytes) return VectorState_NoMoreData; // filled length down to zero means no more data is available
      if (0 == ((ioVectorState.mVector[0]) & 0x3F)) return VectorState_NoMoreData;      // zero size means there are no more RLE data available (this shouldn't happen typically)

      VectorStates state = static_cast<VectorStates>(ioVectorState.mVector[0] & 0xC0);
      ++ioVectorState.mConsumedRLE;

      if (ioVectorState.mConsumedRLE >= static_cast<ULONG>(((ioVectorState.mVector[0]) & 0x3F))) {
        // pull the next BYTE off the queue...
        ++ioVectorState.mVector;
        --ioVectorState.mVectorFilledLengthInBytes;
        ioVectorState.mConsumedRLE = 0;
      }

      return state;
    }

    //-------------------------------------------------------------------------
    void RUDPPacket::log(
                         Log::Level level,
                         const char *inputMessage
                         ) const
    {
      if (ZS_GET_LOG_LEVEL() < level) return;

      // scope: log message
      {
        String message;

        message = String("RUDPPacket, message=") + (inputMessage ? inputMessage : "")
                  + ", channel=" + Stringize<WORD>(mChannelNumber).string() +
                  + ", message=" + (inputMessage ? inputMessage : "")
                  + ", sequence number=" + Stringize<DWORD>(mSequenceNumber).string() +
                  + ", flags=" + Stringize<UINT>(((UINT)mFlags), 16).string()
                  + ", GSNR=" + Stringize<DWORD>(mGSNR).string();

        if (!isFlagSet(Flag_EQ_GSNREqualsGSNFR)) {
          message += String(", GSNFR=") + Stringize<DWORD>(mGSNFR).string();
        }
        message += String(", vector flags=") + Stringize<UINT>(((UINT)mVectorFlags), 16).string();

        if (0 != mVectorLengthInBytes) {
          message += ", vector length=" + Stringize<UINT>(((UINT)mVectorLengthInBytes)).string();
          message += ", vector=" + internal::convertToHex(&(mVector[0]), mVectorLengthInBytes);
        }

        if (0 != mDataLengthInBytes) {
          message += ", data length=" + Stringize<WORD>(mDataLengthInBytes).string();
        }
        message += ", flag details=";
        if (isFlagSet(Flag_PS_ParitySending)) {
          message += "(ps)";
        } else {
          message += "(--)";
        }
        if (isFlagSet(Flag_PG_ParityGSNR)) {
          message += "(pg)";
        } else {
          message += "(--)";
        }
        if (isFlagSet(Flag_XP_XORedParityToGSNFR)) {
          message += "(xp)";
        } else {
          message += "(--)";
        }
        if (isFlagSet(Flag_DP_DuplicatePacket)) {
          message += "(dp)";
        } else {
          message += "(--)";
        }
        if (isFlagSet(Flag_EC_ECNPacket)) {
          message += "(ec)";
        } else {
          message += "(--)";
        }
        if (isFlagSet(Flag_EQ_GSNREqualsGSNFR)) {
          message += "(eq)";
        } else {
          message += "(--)";
        }
        if (isFlagSet(Flag_AR_ACKRequired)) {
          message += "(ar)";
        } else {
          message += "(--)";
        }
        if (isFlagSet(Flag_VP_VectorParity)) {
          message += "(vp)";
        } else {
          message += "(--)";
        }

        ZS_LOG(Basic, message)
      }
    }
  }
}
