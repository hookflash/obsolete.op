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

#include <openpeer/services/STUNPacket.h>
#include <openpeer/services/RUDPPacket.h>
#include <zsLib/Exception.h>
#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/osrng.h>
#include <cryptopp/crc.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>

#include <algorithm>

#define OPENPEER_STUN_MAGIC_COOKIE                 (0x2112A442)
#define OPENPEER_STUN_MAGIC_XOR_FINGERPRINT_VALUE  (0x5354554e)
#define OPENPEER_STUN_HEADER_SIZE_IN_BYTES         (20)
#define OPENPEER_STUN_COMPREHENSION_REQUIRED_MIN   (0x0000)
#define OPENPEER_STUN_COMPREHENSION_REQUIRED_MAX   (0x7FFF)
#define OPENPEER_STUN_MAX_USERNAME                 (513)
#define OPENPEER_STUN_MAX_REALM                    (127)
#define OPENPEER_STUN_MAX_SERVER                   (127)
#define OPENPEER_STUN_MAX_REASON                   (127)
#define OPENPEER_STUN_MAX_NONCE                    (127)
#define OPENPEER_STUN_MAX_SOFTWARE                 (127)
#define OPENPEER_STUN_MAX_CONNECTION_INFO          (127)
#define OPENPEER_STUN_MAX_STRING                   (513)
#define OPENPEER_STUN_MAX_UTF8_UNICODE_ENCODED_CHAR (6)


namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

namespace openpeer
{
  namespace services
  {
    using zsLib::PTRNUMBER;
    using zsLib::CSTR;
    using zsLib::string;
    using zsLib::IPv6Address;

    namespace internal
    {
      String convertToHex(const BYTE *buffer, ULONG bufferLengthInBytes);

      static STUNPacket::Attributes gAttributeOrdering[] =
      {
        STUNPacket::Attribute_MappedAddress,
        STUNPacket::Attribute_Username,
        STUNPacket::Attribute_ErrorCode,
        STUNPacket::Attribute_UnknownAttribute,
        STUNPacket::Attribute_Realm,
        STUNPacket::Attribute_Nonce,
        STUNPacket::Attribute_XORMappedAddress,
        STUNPacket::Attribute_Software,
        STUNPacket::Attribute_AlternateServer,

        // RFC5766 TURN specific attributes
        STUNPacket::Attribute_ChannelNumber,
        STUNPacket::Attribute_Lifetime,
        STUNPacket::Attribute_XORPeerAddress,
        STUNPacket::Attribute_XORRelayedAddress,
        STUNPacket::Attribute_EvenPort,
        STUNPacket::Attribute_RequestedTransport,
        STUNPacket::Attribute_DontFragment,
        STUNPacket::Attribute_ReservationToken,

        // RFC5245 ICE specific attributes
        STUNPacket::Attribute_Priority,
        STUNPacket::Attribute_UseCandidate,
        STUNPacket::Attribute_ICEControlled,
        STUNPacket::Attribute_ICEControlling,

        STUNPacket::Attribute_Data,

        // RFC_draft_RUDP
        STUNPacket::Attribute_NextSequenceNumber,
        STUNPacket::Attribute_MinimumRTT,
        STUNPacket::Attribute_ConnectionInfo,
        STUNPacket::Attribute_CongestionControl,
        STUNPacket::Attribute_GSNR,
        STUNPacket::Attribute_GSNFR,
        STUNPacket::Attribute_RUDPFlags,
        STUNPacket::Attribute_ACKVector,

        // integrity and fingerprint always come last in this order
        STUNPacket::Attribute_MessageIntegrity,
        STUNPacket::Attribute_FingerPrint,

        // end of the attribute sequence
        STUNPacket::Attribute_None,
      };

      //-----------------------------------------------------------------------
      static bool isComprehensionRequired(WORD attributeType)
      {
        if ((attributeType >= OPENPEER_STUN_COMPREHENSION_REQUIRED_MIN) &&   // ignore warning this is always true
            (attributeType <= OPENPEER_STUN_COMPREHENSION_REQUIRED_MAX))
          return true;
        return false;
      }

      //-----------------------------------------------------------------------
      static bool parseSTUNString(const BYTE *data, ULONG length, ULONG maxLength, String &outValue) {
        char buffer[(OPENPEER_STUN_MAX_STRING*OPENPEER_STUN_MAX_UTF8_UNICODE_ENCODED_CHAR)+1];

        if (length > (maxLength*OPENPEER_STUN_MAX_UTF8_UNICODE_ENCODED_CHAR)) return false;

        memset(&(buffer[0]), 0, sizeof(buffer));  // ensure the buffer is NUL terminated
        memcpy(&(buffer[0]), data, length);       // copy the string "as is" into the buffer
        outValue = (const char *)(&(buffer[0]));

        if (outValue.lengthUnicodeSafe() > maxLength) return false; // this is illegal if the unicode length is too long and should be ignored
        return true;
      }

      //-----------------------------------------------------------------------
      static bool parseMappedAddress(const BYTE *dataPos, ULONG attributeLength, DWORD magicCookie, const BYTE *magicCookiePos, IPAddress &outIPAddress, bool &outHandleAsUnknownAttribute) {
        outHandleAsUnknownAttribute = false;
        outIPAddress.clear();

        if (attributeLength < sizeof(DWORD)) return false;

        BYTE family = dataPos[1];
        WORD port = ntohs(((WORD *)dataPos)[1]);

        // from RFC:
        // X-Port is computed by taking the mapped port in host byte order,
        // XOR'ing it with the most significant 16 bits of the magic cookie, and
        // then the converting the result to network byte order.
        port ^= ((magicCookie >> (sizeof(WORD)*8)) & 0xFFFF);

        switch (family) {
          case 0x01: {
            if (attributeLength < sizeof(DWORD) + (32/8)) return false;   // IPv4 has a 32 bit address

            // If the IP address family is IPv4, X-Address is computed by taking the mapped IP
            // address in host byte order, XOR'ing it with the magic cookie, and
            // converting the result to network byte order.

            DWORD ipAddress = ntohl(((DWORD *)dataPos)[1]);
            ipAddress ^= magicCookie;
            outIPAddress = IPAddress(ipAddress, port);
            break;
          }
          case 0x02: {
            if (attributeLength < sizeof(DWORD) + (128/8)) return false;  // IPv6 has a 64 bit address
            IPv6Address rawIP;
            ZS_THROW_INVALID_ASSUMPTION_IF(attributeLength < sizeof(rawIP))  // this should be impossible!!

            memset(&rawIP, 0, sizeof(rawIP));
            memcpy(&rawIP, dataPos, sizeof(rawIP));

            if (NULL != magicCookiePos) {
              BYTE *dest = ((BYTE *)(&rawIP));
              const BYTE *xorPos = magicCookiePos;

              // If the IP address family is IPv6, X-Address is computed by taking the mapped IP address
              // in host byte order, XOR'ing it with the concatenation of the magic
              // cookie and the 96-bit transaction ID, and converting the result to
              // network byte order.
              for (size_t loop = 0; loop < sizeof(DWORD) + (96/8); ++loop, ++xorPos, ++dest) {
                *dest ^= *xorPos;
              }
            }

            outIPAddress = IPAddress(rawIP, port);
            break;
          }
          default: {
            outHandleAsUnknownAttribute = true;
            break;
          }
        }
        return true;
      }

      //-----------------------------------------------------------------------
      static QWORD parseQWORD(const BYTE *pos)
      {
        QWORD value = 0;
        for (size_t size = 0; size < sizeof(QWORD); ++size) {
          value <<= 8;
          value |= pos[size];
        }
        return value;
      }

      //-----------------------------------------------------------------------
      static ULONG dwordBoundary(ULONG length)
      {
        if (0 == (length % sizeof(DWORD)))
          return length;
        return length + (sizeof(DWORD) - (length % sizeof(DWORD)));
      }

      //-----------------------------------------------------------------------
      static ULONG getActualAttributeLength(const STUNPacket &stun, STUNPacket::Attributes attribute)
      {
        switch (attribute)
        {
          case STUNPacket::Attribute_MappedAddress:       return sizeof(DWORD) + (stun.mMappedAddress.isIPv4() ? (32/8) : (128/8));
          case STUNPacket::Attribute_Username:            return stun.mUsername.length();
          case STUNPacket::Attribute_MessageIntegrity:    return sizeof(stun.mMessageIntegrity);
          case STUNPacket::Attribute_ErrorCode:           return sizeof(DWORD) + stun.mReason.length();
          case STUNPacket::Attribute_UnknownAttribute:    return stun.mUnknownAttributes.size() * sizeof(WORD);
          case STUNPacket::Attribute_Realm:               return stun.mRealm.length();
          case STUNPacket::Attribute_Nonce:               return stun.mNonce.length();
          case STUNPacket::Attribute_XORMappedAddress:    return sizeof(DWORD) + (stun.mMappedAddress.isIPv4() ? (32/8) : (128/8));
          case STUNPacket::Attribute_Software:            return stun.mSoftware.length();
          case STUNPacket::Attribute_AlternateServer:     return sizeof(DWORD) + (stun.mAlternateServer.isIPv4() ? (32/8) : (128/8));
          case STUNPacket::Attribute_FingerPrint:         return sizeof(DWORD);

          case STUNPacket::Attribute_ChannelNumber:       return sizeof(DWORD);
          case STUNPacket::Attribute_Lifetime:            return sizeof(DWORD);
          case STUNPacket::Attribute_XORPeerAddress:      {
            ZS_THROW_BAD_STATE_IF(stun.mPeerAddressList.size() < 1)

            ULONG total = 0;
            for (STUNPacket::PeerAddressList::const_iterator iter = stun.mPeerAddressList.begin(); iter != stun.mPeerAddressList.end(); ++iter) {
              total += sizeof(DWORD) + ((*iter).isIPv4() ? (32/8) : (128/8));
            }
            total += (stun.mPeerAddressList.size()-1) * sizeof(DWORD);  // need to account for the header between attributes since each one must be independently added
            return total;
          }
          case STUNPacket::Attribute_Data:                return stun.mDataLength;
          case STUNPacket::Attribute_XORRelayedAddress:   return sizeof(DWORD) + (stun.mRelayedAddress.isIPv4() ? (32/8) : (128/8));
          case STUNPacket::Attribute_EvenPort:            return sizeof(BYTE);
          case STUNPacket::Attribute_RequestedTransport:  return sizeof(DWORD);
          case STUNPacket::Attribute_DontFragment:        return 0;
          case STUNPacket::Attribute_ReservationToken:    return sizeof(stun.mReservationToken);

          case STUNPacket::Attribute_Priority:            return sizeof(DWORD);
          case STUNPacket::Attribute_UseCandidate:        return 0;
          case STUNPacket::Attribute_ICEControlled:       return sizeof(QWORD);
          case STUNPacket::Attribute_ICEControlling:      return sizeof(QWORD);


          case STUNPacket::Attribute_NextSequenceNumber:  return sizeof(QWORD);
          case STUNPacket::Attribute_MinimumRTT:          return sizeof(DWORD);
          case STUNPacket::Attribute_ConnectionInfo:      return stun.mConnectionInfo.length();
          case STUNPacket::Attribute_CongestionControl:   {
            bool bothPresent = (stun.mLocalCongestionControl.size() > 0) && (stun.mRemoteCongestionControl.size() > 0);
            ULONG lengthLocal = (stun.mLocalCongestionControl.size() > 0 ? sizeof(WORD) + (sizeof(WORD)*(stun.mLocalCongestionControl.size())) : 0);
            ULONG lengthRemote = (stun.mRemoteCongestionControl.size() > 0 ? sizeof(WORD) + (sizeof(WORD)*(stun.mRemoteCongestionControl.size())) : 0);
            // if both are present we will encode the attribute header for the second when we encode the first
            return (bothPresent ? sizeof(DWORD) : 0) + lengthLocal + lengthRemote;
            break;
          }
          case STUNPacket::Attribute_GSNR:                return sizeof(QWORD);
          case STUNPacket::Attribute_GSNFR:               return sizeof(QWORD);
          case STUNPacket::Attribute_RUDPFlags:           return sizeof(DWORD);
          case STUNPacket::Attribute_ACKVector:           return stun.mACKVectorLength;
          default:                                        break;
        }
        return 0;
      }

      //-----------------------------------------------------------------------
      static bool isLegalMethod(
                                STUNPacket::Methods method,
                                STUNPacket::Classes tClass,
                                STUNPacket::RFCs allowedRFCs
                                )
      {
        UINT rfcBits = 0;
        switch (method) {
          case STUNPacket::Method_Binding:              rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5245_ICE; break;
          case STUNPacket::Method_Allocate:             rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Method_Refresh:              rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Method_Send:                 rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Method_Data:                 rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Method_CreatePermission:     rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Method_ChannelBind:          rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Method_ReliableChannelOpen:  rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Method_ReliableChannelACK:   rfcBits = STUNPacket::RFC_draft_RUDP; break;
        }

        if (0 == (rfcBits & ((UINT)allowedRFCs)))
          return false;

        // this is now legal but not all classes are supported
        switch (tClass) {
          case STUNPacket::Class_Request:
          case STUNPacket::Class_Response:
          case STUNPacket::Class_ErrorResponse:
          {
            switch (method) {
              case STUNPacket::Method_Binding:              break; // all classes are legal for a binding request
              case STUNPacket::Method_Allocate:             break;
              case STUNPacket::Method_Refresh:              break;
              case STUNPacket::Method_Send:                 return false;
              case STUNPacket::Method_Data:                 return false;
              case STUNPacket::Method_CreatePermission:     break;
              case STUNPacket::Method_ChannelBind:          break;
              case STUNPacket::Method_ReliableChannelOpen:  break;
              case STUNPacket::Method_ReliableChannelACK:   break;
            }
            break;
          }
          case STUNPacket::Class_Indication:
          {
            switch (method) {
              case STUNPacket::Method_Binding:              break; // all classes are legal for binding request
              case STUNPacket::Method_Allocate:             return false;
              case STUNPacket::Method_Refresh:              return false;
              case STUNPacket::Method_Send:                 break;
              case STUNPacket::Method_Data:                 break;
              case STUNPacket::Method_CreatePermission:     return false;
              case STUNPacket::Method_ChannelBind:          return false;
              case STUNPacket::Method_ReliableChannelOpen:  return false;
              case STUNPacket::Method_ReliableChannelACK:   break;
            }
            break;
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      static bool isAttributeKnown(
                                   STUNPacket::RFCs allowedRFCs,
                                   STUNPacket::Attributes attribute
                                   )
      {
        UINT rfcBits = 0;
        switch (attribute)
        {
          case STUNPacket::Attribute_MappedAddress:       rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN; break;
          case STUNPacket::Attribute_Username:            rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5245_ICE | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_MessageIntegrity:    rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5245_ICE | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_ErrorCode:           rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5245_ICE | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_UnknownAttribute:    rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5245_ICE | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_Realm:               rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_Nonce:               rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_XORMappedAddress:    rfcBits = STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5245_ICE; break;
          case STUNPacket::Attribute_Software:            rfcBits = STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5245_ICE  | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_AlternateServer:     rfcBits = STUNPacket::RFC_3489_STUN | STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_FingerPrint:         rfcBits = STUNPacket::RFC_5389_STUN | STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5245_ICE  | STUNPacket::RFC_draft_RUDP; break;

          case STUNPacket::Attribute_ChannelNumber:       rfcBits = STUNPacket::RFC_5766_TURN | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_Lifetime:            rfcBits = STUNPacket::RFC_5766_TURN | STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_XORPeerAddress:      rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Attribute_Data:                rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Attribute_XORRelayedAddress:   rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Attribute_EvenPort:            rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Attribute_RequestedTransport:  rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Attribute_DontFragment:        rfcBits = STUNPacket::RFC_5766_TURN; break;
          case STUNPacket::Attribute_ReservationToken:    rfcBits = STUNPacket::RFC_5766_TURN; break;

          case STUNPacket::Attribute_Priority:            rfcBits = STUNPacket::RFC_5245_ICE; break;
          case STUNPacket::Attribute_UseCandidate:        rfcBits = STUNPacket::RFC_5245_ICE; break;
          case STUNPacket::Attribute_ICEControlled:       rfcBits = STUNPacket::RFC_5245_ICE; break;
          case STUNPacket::Attribute_ICEControlling:      rfcBits = STUNPacket::RFC_5245_ICE; break;

          case STUNPacket::Attribute_NextSequenceNumber:  rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_MinimumRTT:          rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_ConnectionInfo:      rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_CongestionControl:   rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_GSNR:                rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_GSNFR:               rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_RUDPFlags:           rfcBits = STUNPacket::RFC_draft_RUDP; break;
          case STUNPacket::Attribute_ACKVector:           rfcBits = STUNPacket::RFC_draft_RUDP; break;

          case STUNPacket::Attribute_ReservedResponseAddress:
          case STUNPacket::Attribute_ReservedChangeAddress:
          case STUNPacket::Attribute_ReservedSourceAddress:
          case STUNPacket::Attribute_ReservedChangedAddress:
          case STUNPacket::Attribute_ReservedPassword:
          case STUNPacket::Attribute_ReservedReflectedFrom: return true;  // just ignore all these attributes
          default:                                        break;
        }
        return (0 != (((UINT)allowedRFCs) & ((UINT)rfcBits)));
      }

      //-----------------------------------------------------------------------
      static bool isAttributeLegal(
                                   const STUNPacket &stun,
                                   STUNPacket::RFCs rfc,
                                   STUNPacket::Attributes attribute
                                   )
      {
        if (!isLegalMethod(stun.mMethod, stun.mClass, rfc))
          return false;

        if (!isAttributeKnown(rfc, attribute))
          return false;

        switch (attribute)
        {
          case STUNPacket::Attribute_MappedAddress:       {
            if ((STUNPacket::Class_Response == stun.mClass) &&
                (STUNPacket::Method_Binding == stun.mMethod))
              return true;                                              // this is only ever allowed to a binding response on the old RFC (but we will tolerate on a new RFC)
            return false;                                               // otherwise this attribute should never exist
          }

          case STUNPacket::Attribute_Username:            {
            switch (stun.mClass)
            {
              case STUNPacket::Class_Response:
              case STUNPacket::Class_ErrorResponse:       return false; // this can never be present on a response
              default:                                    break;
            }

            // legal every where else
            return true;
          }

          case STUNPacket::Attribute_MessageIntegrity:    {
            switch (stun.mMethod)
            {
              case STUNPacket::Method_Send:
              case STUNPacket::Method_Data: return false;               // do NOT include message integrity on SEND/DATA send indications
              default:                      break;
            }
            return true;
          }

          case STUNPacket::Attribute_ErrorCode:           {
            if (STUNPacket::Class_ErrorResponse != stun.mClass)
              return false;                                             // only allowed an error code on an error response class
            return true;
          }

          case STUNPacket::Attribute_UnknownAttribute:    {
            if (STUNPacket::Class_ErrorResponse != stun.mClass)
              return false;                                             // only allowed on an error response
            if (STUNPacket::ErrorCode_UnknownAttribute != stun.mErrorCode)
              return false;                                             // more specifically only allowed on a 420 error code

            return true;                                                // this is legal under those two conditions
          }

          case STUNPacket::Attribute_Realm:
          case STUNPacket::Attribute_Nonce:               {
            if (STUNPacket::Class_Response == stun.mClass)
              return false;                                             // do not include realm or nonce on a successful response

            return true;                                                // they are legal everywhere else
            break;
          }

          case STUNPacket::Attribute_XORMappedAddress:    {
            if (STUNPacket::Class_Response != stun.mClass)
              return false;                                             // attribute is only legal on a valid response

            switch (rfc)
            {
              case STUNPacket::RFC_3489_STUN:             return false; // old RFC cannot support this type
              case STUNPacket::RFC_5389_STUN:             return true;  // new RFC makes use of this attribute in binding response
              case STUNPacket::RFC_5766_TURN: {
                if (STUNPacket::Method_Allocate != stun.mMethod)
                  return false;                                         // it's only legal in response to the allocate method

                return true;
              }
              case STUNPacket::RFC_5245_ICE:              return true;  // legal in all ICE binding responses
              default:                                    break;
            }
            return false;                                               // shouldn't even reach this code point but mark as illegal just in case
          }

          case STUNPacket::Attribute_Software:            return true;  // legal for any message

          case STUNPacket::Attribute_AlternateServer:     {
            if (STUNPacket::Class_ErrorResponse != stun.mClass)
              return false;                                             // not legal unless this is an error response
            return true;                                                // legal in the case of an error in all other situations
          }

          case STUNPacket::Attribute_FingerPrint:         return true;  // legal for any message

          case STUNPacket::Attribute_ChannelNumber:       {
            if (STUNPacket::Method_ChannelBind == stun.mMethod) {
              if (STUNPacket::Class_Request != stun.mClass)
                return false;                                           // not legal unless this is a request on a Channel Bind
              return true;
            }
            if (STUNPacket::Method_ReliableChannelACK == stun.mMethod) {
              if ((STUNPacket::Class_Request != stun.mClass) &&
                  (STUNPacket::Class_Indication != stun.mClass) &&
                  (STUNPacket::Class_Response != stun.mClass))
                return false;                                           // not legal unless this is a request/indication on a RUDP ACK
              return true;
            }
            if (STUNPacket::Method_ReliableChannelOpen != stun.mMethod)
              return false;                                             // not legal on any other method except the RUDP open method

            return true;                                                // legal (and required) if this is a RUDP method
          }

          case STUNPacket::Attribute_Lifetime:            {
            switch (stun.mMethod) {
              case STUNPacket::Method_Allocate:
              case STUNPacket::Method_Refresh:
              case STUNPacket::Method_ReliableChannelOpen: return true; // only these methods have the LIFETIME attribtue
              default:                                     break;
            }
            return false;
          }

          case STUNPacket::Attribute_XORPeerAddress:      {
            switch (stun.mMethod) {
              case STUNPacket::Method_Allocate:
              case STUNPacket::Method_Refresh:            return false; // these two method do not use this attribute
              default:                                    break;
            }
            switch (stun.mClass) {
              case STUNPacket::Class_Response:
              case STUNPacket::Class_ErrorResponse:       return false; // never found on a response
              default:                                    break;
            }
            return true;
          }

          case STUNPacket::Attribute_Data:                {             // only every found on Send/Data indicators (where they are required)
            switch (stun.mMethod) {
              case STUNPacket::Method_Send:
              case STUNPacket::Method_Data:               return true;
              default:                                    break;
            }
            return false;
          }

          case STUNPacket::Attribute_XORRelayedAddress:   {
            if (STUNPacket::Class_Response != stun.mClass)
              return false;                                           // attribute is only legal on a valid response
            if (STUNPacket::Method_Allocate != stun.mMethod)
              return false;                                           // it's only legal in response to the allocate method
            return true;
          }

          case STUNPacket::Attribute_EvenPort:            {
            if (STUNPacket::Method_Allocate != stun.mMethod)
              return false;
            if (STUNPacket::Class_Request != stun.mClass)
              return false;
            if (stun.hasAttribute(STUNPacket::Attribute_ReservationToken))
              return false;                                           // NOT legal if reservation token is included in request
            return true;
          }

          case STUNPacket::Attribute_RequestedTransport:  {
            if (STUNPacket::Method_Allocate != stun.mMethod)          // must be contained on an allocate request
              return false;
            if (STUNPacket::Class_Request != stun.mClass)
              return false;
            return true;
          }

          case STUNPacket::Attribute_DontFragment:        {
            switch (stun.mMethod) {
              case STUNPacket::Method_Refresh:
              case STUNPacket::Method_Data:
              case STUNPacket::Method_CreatePermission:
              case STUNPacket::Method_ChannelBind:        return false;
              default:                                    break;      // optional for Allocate request and Send indication
            }
            switch (stun.mClass) {
              case STUNPacket::Class_Response:
              case STUNPacket::Class_ErrorResponse:       return false;
              default:                                    break;
            }
            return true;
          }

          case STUNPacket::Attribute_ReservationToken:    {
            if (STUNPacket::Method_Allocate != stun.mMethod)          // only found on allocate request or response
              return false;

            switch (stun.mClass) {
              case STUNPacket::Class_Indication:
              case STUNPacket::Class_ErrorResponse:       return false;
              default:                                    break;
            }
            if (stun.hasAttribute(STUNPacket::Attribute_EvenPort))    // not legal with even port attribute
              return false;

            return true;
          }

          // ICE attributes
          case STUNPacket::Attribute_Priority:            {
            if (STUNPacket::Class_Request != stun.mClass)             // madatory in binding request
              return false;
            return true;
          }

          case STUNPacket::Attribute_UseCandidate:        {
            if (STUNPacket::Class_Request != stun.mClass)           // may include in binding request (but only if controlling but no way to determine that here)
              return false;
            return true;
          }

          case STUNPacket::Attribute_ICEControlled:
          case STUNPacket::Attribute_ICEControlling:      {
            if (STUNPacket::Class_Request != stun.mClass)           // may include in binding request
              return false;

            // can have one or the other, but not both...
            if ((stun.mIceControlledIncluded) && (stun.mIceControllingIncluded))
              return false;

            return true;
          }

          case STUNPacket::Attribute_NextSequenceNumber: return true;
          case STUNPacket::Attribute_MinimumRTT:
          case STUNPacket::Attribute_ConnectionInfo:
          case STUNPacket::Attribute_CongestionControl:   {
            if (STUNPacket::Method_ReliableChannelOpen != stun.mMethod)
              return false;
            return true;
          }
          case STUNPacket::Attribute_GSNR:
          case STUNPacket::Attribute_GSNFR:
          case STUNPacket::Attribute_RUDPFlags:
          case STUNPacket::Attribute_ACKVector:           {
            if (STUNPacket::Method_ReliableChannelACK != stun.mMethod)
              return false;
            return true;
          }

          case STUNPacket::Attribute_ReservedResponseAddress:
          case STUNPacket::Attribute_ReservedChangeAddress:
          case STUNPacket::Attribute_ReservedSourceAddress:
          case STUNPacket::Attribute_ReservedChangedAddress:
          case STUNPacket::Attribute_ReservedPassword:
          case STUNPacket::Attribute_ReservedReflectedFrom: return true;  // just ignore all these attributes
          default:                                        break;
        }
        return false;
      }

      //-----------------------------------------------------------------------
      // RETURNS: returns TRUE if the message MUST have the attribute present
      //          given its current state
      static bool isAttributeRequired(
                                      const STUNPacket &stun,
                                      STUNPacket::RFCs rfc,
                                      STUNPacket::Attributes attribute
                                      )
      {
        // can only be required if the attribute is known
        if (!isAttributeKnown(rfc, attribute))
          return false;

        // can only be required if the attribute is legal
        if (!isAttributeLegal(
                              stun,
                              rfc,
                              attribute))
          return false;

        switch (attribute)
        {
          case STUNPacket::Attribute_MappedAddress:       {
            if (STUNPacket::Class_Response != stun.mClass)
              return false;                                             // not required unless this is an actual response

            switch (rfc) {
              case STUNPacket::RFC_3489_STUN:             return true;  // required on a request for old STUN RFC
              case STUNPacket::RFC_5389_STUN: {
                if (STUNPacket::Class_Response == stun.mClass)
                  return stun.isRFC3489();                              // only required if this is operating in the old RFC version on a successful response
                return false;
              }
              default:                                    break;
            }
            return false;                                               // nothing else uses this or requires this
          }

          case STUNPacket::Attribute_Username:            {
            if ((STUNPacket::Class_Request == stun.mClass) ||
                (STUNPacket::Class_Indication == stun.mClass)) {
              if (stun.hasAttribute(STUNPacket::Attribute_MessageIntegrity)) {
                return true;                                            // when there is a request and the credentials is present this attribute it required
              }
              switch (stun.mMethod) {
                case STUNPacket::Method_ReliableChannelOpen:  return false; // doesn't always require a username (i.e. in the case of a request anonymously to a server)
                case STUNPacket::Method_ReliableChannelACK:   return true;  // the reliable requests always require a username
                default:                                      break;
              }
            }
            return false;
          }

          case STUNPacket::Attribute_MessageIntegrity:    {
            if ((STUNPacket::Class_Request == stun.mClass) ||
                (STUNPacket::Class_Indication == stun.mClass)) {
              if (stun.hasAttribute(STUNPacket::Attribute_Username)) {
                return true;                                            // when there is a request or an indication and the username is present this attribute it required
              }
            }
            if (STUNPacket::RFC_5245_ICE == rfc)
              return true;                                              // ICE requires that credentials are used at all times
            return false;
          }

          case STUNPacket::Attribute_ErrorCode:           {
            if (STUNPacket::Class_ErrorResponse == stun.mClass)
              return true;                                              // must have an error code on an error response
            return false;
          }

          case STUNPacket::Attribute_UnknownAttribute:    {
            if (STUNPacket::Class_ErrorResponse != stun.mClass)
              return false;                                             // only ever used on an error response
            if (STUNPacket::ErrorCode_UnknownAttribute == stun.mErrorCode)
              return true;                                              // this particular error requires this attribute be used
            return false;
          }

          case STUNPacket::Attribute_Realm:               return false; // not required as short term credentials might be used
          case STUNPacket::Attribute_Nonce:               return false; // not required as short term credentials might be used

          case STUNPacket::Attribute_XORMappedAddress:    {
            switch (stun.mMethod) {
              case STUNPacket::Method_Binding:
              case STUNPacket::Method_Allocate:           break;
              default:                                    return false; // no other method requires this attribute
            }
            if (STUNPacket::Class_Response == stun.mClass)
              return true;                                              // required on binding and allocate responses for STUN/ICE
          }

          case STUNPacket::Attribute_Software:            return false; // never required (but recommended)

          case STUNPacket::Attribute_AlternateServer:     {
            if (STUNPacket::Class_ErrorResponse != stun.mClass)
              return false;                                             // not required when not an error

            if (STUNPacket::ErrorCode_TryAlternate == stun.mErrorCode)
              return true;                                              // this is required when specifying to try an alternative server

            return false;
          }

          case STUNPacket::Attribute_FingerPrint:         {
            if (STUNPacket::RFC_5245_ICE == rfc) {
              if (STUNPacket::Class_Indication == stun.mClass)
                return false;   // ICE does not require on indications but it is recommended
              return true;                                              // ICE requires the fingerprint attribute be used on all request/response messages
            }
            return false;
          }

          case STUNPacket::Attribute_ChannelNumber:       {
            switch (stun.mMethod) {
              case STUNPacket::Method_ChannelBind:        {
                if (STUNPacket::Class_Request == stun.mClass)
                  return true;                                          // this is required on a channel bind request but not in the response
                return false;
              }
              case STUNPacket::Method_ReliableChannelACK: {
                if ((STUNPacket::Class_Request == stun.mClass) ||
                    (STUNPacket::Class_Indication == stun.mClass) ||
                    (STUNPacket::Class_Response == stun.mClass))
                  return true;                                          // this is required on a RUDP ACK request/indication but not in the response
                return false;
              }
              case STUNPacket::Method_ReliableChannelOpen: {

                if (STUNPacket::Class_Request == stun.mClass) return true;  // always required on the open request

                if ((stun.mLifetimeIncluded) &&
                    (0 == stun.mLifetime))
                  return false; // this is required on RUDP open requests (most responses require too but not a "close" response

                if (STUNPacket::Class_Response == stun.mClass)
                  return true;

                return false;
              }
              default: break;
            }
            return false;
          }

          case STUNPacket::Attribute_Lifetime:            return false;

          case STUNPacket::Attribute_XORPeerAddress:      {
            switch (stun.mMethod) {
              case STUNPacket::Method_Send:
              case STUNPacket::Method_Data:
              case STUNPacket::Method_CreatePermission:
              case STUNPacket::Method_ChannelBind:        return true;  // require in the responses (and only legal there too)
              default:                                    break;
            }
            return false;
          }

          case STUNPacket::Attribute_Data:                {
            switch (stun.mMethod) {
              case STUNPacket::Method_Send:
              case STUNPacket::Method_Data:               return true;
              default:                                    break;
            }
            return false;
          }

          case STUNPacket::Attribute_XORRelayedAddress:   {             // must be on an allocate response
            if (STUNPacket::Method_Allocate != stun.mMethod)
              return false;
            if (STUNPacket::Class_Response != stun.mClass)
              return true;
            return false;
          }

          case STUNPacket::Attribute_EvenPort:            return false;

          case STUNPacket::Attribute_RequestedTransport:  {             // required on an allocate request
            if (STUNPacket::Method_Allocate != stun.mMethod)
              return false;
            if (STUNPacket::Class_Request == stun.mClass)
              return true;
            return false;
          }

          case STUNPacket::Attribute_DontFragment:        return false;

          case STUNPacket::Attribute_ReservationToken:    return false;

          case STUNPacket::Attribute_Priority:            {
            if (STUNPacket::Class_Request == stun.mClass)
              return true;
            return false;
          }

          case STUNPacket::Attribute_UseCandidate:        return false;

          case STUNPacket::Attribute_ICEControlled:
          case STUNPacket::Attribute_ICEControlling:      {
            if (STUNPacket::Class_Request == stun.mClass) {
              // STUN packet requires one or the other but NOT both... check for this case...

              // If the packet is correctly setup return "false" saying it's not required, otherwise return "true"
              if ((stun.mIceControlledIncluded) || (stun.mIceControllingIncluded))
                return false;

              return true;
            }
            return false;
          }

          case STUNPacket::Attribute_NextSequenceNumber:  {
            if ((STUNPacket::Method_ReliableChannelOpen != stun.mMethod) &&
                (STUNPacket::Method_ReliableChannelACK != stun.mMethod)) {
              return false;
            }

            if ((stun.mLifetimeIncluded) &&
                (0 == stun.mLifetime))
              return false;

            if ((STUNPacket::Class_Request == stun.mClass) ||
                (STUNPacket::Class_Response == stun.mClass)) {
              return true;
            }
            return false;
          }
          case STUNPacket::Attribute_MinimumRTT:          return false;         // this is optional during the request/response
          case STUNPacket::Attribute_ConnectionInfo:      return false;         // this is optional during the request/response
          case STUNPacket::Attribute_CongestionControl:   {
            if (STUNPacket::Method_ReliableChannelOpen != stun.mMethod)         // not used unless it is RUDP channel open
              return false;
            if (STUNPacket::Class_Request != stun.mClass)
              return false;
            if ((stun.mLifetimeIncluded) &&
                (0 == stun.mLifetime))
              return false;
            return true;
          }
          case STUNPacket::Attribute_GSNR:
          case STUNPacket::Attribute_GSNFR:
          case STUNPacket::Attribute_RUDPFlags:           {
            if (STUNPacket::Method_ReliableChannelACK != stun.mMethod)          //not used unless it is RUDP ACK
              return false;
            if (STUNPacket::Class_ErrorResponse == stun.mClass)                 // not required on an error response
              return false;
            return true;                                                        // the RUDP ACK method requires all of these attributes be present
          }
          case STUNPacket::Attribute_ACKVector:           return false;         // this isn't ever required

          case STUNPacket::Attribute_ReservedResponseAddress:
          case STUNPacket::Attribute_ReservedChangeAddress:
          case STUNPacket::Attribute_ReservedSourceAddress:
          case STUNPacket::Attribute_ReservedChangedAddress:
          case STUNPacket::Attribute_ReservedPassword:
          case STUNPacket::Attribute_ReservedReflectedFrom: return false;       // just ignore all these attributes
          default:                                        break;
        }
        return false;
      }

      //-----------------------------------------------------------------------
      static STUNPacket::RFCs guessRFC(const STUNPacket &stun)
      {
        switch (stun.mMethod) {
          case STUNPacket::Method_Binding:          {
            // this could be TURN or ICE

            // if these are any ICE specific attributes then it is probably ICE
            if (stun.hasAttribute(STUNPacket::Attribute_Priority)) return STUNPacket::RFC_5245_ICE;
            if (stun.hasAttribute(STUNPacket::Attribute_UseCandidate)) return STUNPacket::RFC_5245_ICE;
            if (stun.hasAttribute(STUNPacket::Attribute_ICEControlled)) return STUNPacket::RFC_5245_ICE;
            if (stun.hasAttribute(STUNPacket::Attribute_ICEControlling)) return STUNPacket::RFC_5245_ICE;

            // no ICE specific attributes found, check is message integrity is present
            if (!stun.hasAttribute(STUNPacket::Attribute_MessageIntegrity)) break;  // if there isn't any message integrity it can't be ICE

            // this is probably ICE because it has message integrity and STUN discovery typically doesn't use message integrity
            return STUNPacket::RFC_5245_ICE;
          }

          // all of the following are strictly TURN related
          case STUNPacket::Method_Allocate:
          case STUNPacket::Method_Refresh:
          case STUNPacket::Method_Send:
          case STUNPacket::Method_Data:
          case STUNPacket::Method_CreatePermission:
          case STUNPacket::Method_ChannelBind:          return STUNPacket::RFC_5766_TURN;

          // all of the following are strictly RUDP related
          case STUNPacket::Method_ReliableChannelOpen:
          case STUNPacket::Method_ReliableChannelACK:   return STUNPacket::RFC_draft_RUDP;
          default:                                      break;
        }

        return stun.isRFC5389() ? STUNPacket::RFC_5389_STUN : STUNPacket::RFC_3489_STUN;
      }

      //-----------------------------------------------------------------------
      static STUNPacket::RFCs guessRFC(const STUNPacket &stun, STUNPacket::RFCs allowedRFCs)
      {
        STUNPacket::RFCs guessedRFC = guessRFC(stun);
        if (0 != (((UINT)guessedRFC) & ((UINT)allowedRFCs)))
          return guessedRFC;

        // the guessed RFC wasn't allowed so choose one of the allowed RFCs
        if ((0 != (((UINT)STUNPacket::RFC_5389_STUN) & ((UINT)allowedRFCs))) &&
            (stun.isRFC5389()))
          return STUNPacket::RFC_5389_STUN;

        if (0 != (((UINT)STUNPacket::RFC_3489_STUN) & ((UINT)allowedRFCs)))
          return STUNPacket::RFC_3489_STUN;

        if (0 != (((UINT)STUNPacket::RFC_5245_ICE) & ((UINT)allowedRFCs)))
          return STUNPacket::RFC_5245_ICE;

        if (0 != (((UINT)STUNPacket::RFC_5766_TURN) & ((UINT)allowedRFCs)))
          return STUNPacket::RFC_5766_TURN;

        if (0 != (((UINT)STUNPacket::RFC_draft_RUDP) & ((UINT)allowedRFCs)))
          return STUNPacket::RFC_draft_RUDP;

        return stun.isRFC5389() ? STUNPacket::RFC_5389_STUN : STUNPacket::RFC_3489_STUN;
      }

      //-----------------------------------------------------------------------
      static bool shouldPacketizeAttribute(
                                           const STUNPacket &stun,
                                           STUNPacket::RFCs rfc,
                                           STUNPacket::Attributes attribute
                                           )
      {
        if (!isAttributeLegal(
                              stun,
                              rfc,
                              attribute))
          return false; // should not packetize if not legal

        if (isAttributeRequired(
                                stun,
                                rfc,
                                attribute))
          return true;  // must packetize if required

        switch (stun.mClass) {
          case STUNPacket::Class_Response: {
            switch (attribute) {
              case STUNPacket::Attribute_Username:
              case STUNPacket::Attribute_Realm:     return false;
              default:                              break;
            }
            break;
          }
          case STUNPacket::Class_ErrorResponse:     {
            switch (attribute) {
              case STUNPacket::Attribute_Username:  return false; // never include the username
              case STUNPacket::Attribute_Realm:     {
                // only include if the error code is stale nonce or unauthorized
                if ((STUNPacket::ErrorCode_Unauthorized == stun.mErrorCode) ||
                    (STUNPacket::ErrorCode_StaleNonce == stun.mErrorCode))
                  return true;
                return false;
              }
              default:                              break;
            }
            break;
          }
          default:                                  break;
        }

        return true;    // its optional here but we should packetize it
      }

      //-----------------------------------------------------------------------
      static ULONG packetizeAttributeLength(
                                            const STUNPacket &stun,
                                            STUNPacket::RFCs rfc,
                                            STUNPacket::Attributes attribute
                                            )
      {
        // do not count length for packets that should not be packetized
        if (!stun.hasAttribute(attribute)) return 0;
        if (!shouldPacketizeAttribute(
                                      stun,
                                      rfc,
                                      attribute)) return 0;

        return sizeof(DWORD) + dwordBoundary(getActualAttributeLength(stun, attribute));  // the size of one header plus the attribute value size
      }

      static void packetizeAttributeHeader(BYTE * &pos, STUNPacket::Attributes attribute, ULONG attributeLength) {
        ((WORD *)pos)[0] = htons((WORD)attribute);
        ((WORD *)pos)[1] = htons(static_cast<WORD>(attributeLength));
        pos += sizeof(DWORD);
      }

      static void packetizeAttributeString(BYTE *pos, const String &str) {
        memcpy(pos, (const BYTE *)((CSTR)str), str.length());
      }

      static void packetizeIPAddress(BYTE *pos, const IPAddress &ipAddress, DWORD xorMagicCookie = 0, const BYTE *magicCookiePos = NULL)
      {
        pos[1] = (BYTE)(ipAddress.isIPv4() ? 0x01 : 0x02);
        ((WORD *)pos)[1] = htons(ipAddress.getPort() ^ ((xorMagicCookie & 0xFFFF0000) >> (sizeof(WORD)*8)));
        pos += sizeof(DWORD);

        if (ipAddress.isIPv4()) {
          ((DWORD *)pos)[0] = htonl(ipAddress.getIPv4AddressAsDWORD() ^ xorMagicCookie);
        } else {
          BYTE *dest = (BYTE *)(&(((DWORD *)pos)[0]));
          const BYTE *sour = &(ipAddress.mIPAddress.by[0]);
          ZS_THROW_INVALID_ASSUMPTION_IF(128/8 != sizeof(ipAddress.mIPAddress))
          memcpy(dest, sour, 128/8);

          if (magicCookiePos) {
            const BYTE *xorSour = magicCookiePos;
            for (size_t loop = 0; loop < sizeof(ipAddress.mIPAddress); ++loop, ++dest, ++xorSour) {
              *dest ^= *xorSour;
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      static void packetizePeerAddresses(
                                         BYTE * &pos,
                                         const STUNPacket &stun,
                                         const BYTE *cookiePos
                                         )
      {
        for (STUNPacket::PeerAddressList::const_iterator iter = stun.mPeerAddressList.begin(); iter != stun.mPeerAddressList.end(); ++iter) {
          ULONG attributeLength = sizeof(DWORD) + ((*iter).isIPv4() ? (32/8) : (128/8));
          packetizeAttributeHeader(pos, STUNPacket::Attribute_XORPeerAddress, attributeLength);    // need a new header per alternative peer addresses

          packetizeIPAddress(pos, (*iter), stun.mMagicCookie, cookiePos);
          pos += dwordBoundary(attributeLength);
        }
      }

      static void packetizeDWORD(BYTE *pos, DWORD value) {
        ((DWORD *)pos)[0] = htonl(value);
      }

      static void packetizeQWORD(BYTE *pos, QWORD value) {
        // this is a quick way to fill the lowest to highest significance to do network byte order regardless of OS endianness
        pos = pos + sizeof(QWORD) - 1;
        for (size_t loop = 0; loop < sizeof(QWORD); ++loop, --pos) {
          *pos = (BYTE)(value & 0xFF);
          value >>= 8;
        }
      }

      static void packetizeBuffer(BYTE *pos, const BYTE *buffer, ULONG length) {
        ZS_THROW_INVALID_USAGE_IF(!buffer)
        memcpy(pos, buffer, length);
      }

      // for message integrity
//      static const char *gHexstream = "000300a42112a442f2fd4b12308af95c4c23c06300060004746f746f0014000c79616b6f6c616b6f2e636f6d001500303465333639623936303030303030303039303561346535373734636666353530353030643566616262613963626537348022002c72655455524e204173796e6320436c69656e7420302e33202d20524643353338392f7475726e2d31322020200018000180000000001900041100000000080014cd35cfedec575a0b2515cf0a5bd7c1379ab1f3ea";
      // compare fingerprint
//      static const char *gHexstream = "000100582112a442b7e7a701bc34d686fa87dfae802200105354554e207465737420636c69656e74002400046e0001ff80290008932ff9b151263b36000600096576746a3a68367659202020000800149aeaa70cbfd8cb56781ef2b5b2d3f249c1b571a2";
      // should be CRC32 value: e57a3bcf

      /*

      static void convertHexStream(BYTE *dest)
      {
        bool high = true;
        BYTE current = 0;
        for (const char *pos = &(gHexstream[0]); *pos; ++pos, high = !high)
        {
          BYTE value = 0;
          switch (*pos)
          {
            case '0': value = 0; break;
            case '1': value = 1; break;
            case '2': value = 2; break;
            case '3': value = 3; break;
            case '4': value = 4; break;
            case '5': value = 5; break;
            case '6': value = 6; break;
            case '7': value = 7; break;
            case '8': value = 8; break;
            case '9': value = 9; break;
            case 'a': value = 10; break;
            case 'b': value = 11; break;
            case 'c': value = 12; break;
            case 'd': value = 13; break;
            case 'e': value = 14; break;
            case 'f': value = 15; break;
          }
          if (high) {
            current = value << 4;
          } else {
            current |= value;
            *dest = current;
            ++dest;
          }
        }
      }
       */

      //-----------------------------------------------------------------------
      static void packetizeMessageIntegrity(
                                            BYTE *pos,
                                            const STUNPacket &stun,
                                            const BYTE *attributeStartPos
                                            )
      {
        switch (stun.mCredentialMechanism) {
          case STUNPacket::CredentialMechanisms_None:       break;
          case STUNPacket::CredentialMechanisms_ShortTerm:
          case STUNPacket::CredentialMechanisms_LongTerm:   {
            CryptoPP::Weak::MD5 md5;

            /*
            size_t len = strlen(gHexstream)/2;
            boost::shared_array<BYTE> blob(new BYTE[len*2]);
            BYTE *compare = blob.get();
            memset(compare, 0, len*2);
            convertHexStream(compare);
            */

            BYTE key[16];
            memset(&(key[0]), 0, sizeof(key));

//            String user = "bogus";
//            String pass = "bogus";
//            String realm = "yakolako.com";

            const String &user = stun.mUsername;
            const String &pass = stun.mPassword;
            const String &realm = stun.mRealm;

            if (STUNPacket::CredentialMechanisms_LongTerm == stun.mCredentialMechanism) {
              md5.Update((const BYTE *)((CSTR)user), user.length());
              md5.Update((const BYTE *)":", strlen(":"));
              md5.Update((const BYTE *)((CSTR)realm), realm.length());
              md5.Update((const BYTE *)":", strlen(":"));
            }
            md5.Update((const BYTE *)((CSTR)pass), pass.length());

            ZS_THROW_INVALID_ASSUMPTION_IF(sizeof(key) != md5.DigestSize())
            md5.Final(&(key[0]));

            BYTE result[OPENPEER_STUN_MESSAGE_INTEGRITY_LENGTH_IN_BYTES];
            memset(&(result[0]), 0, sizeof(result));

            // messageIntegrityMessageLengthInBytes is the length of the packet up to but not including the message integrity attribute
            ULONG messageIntegrityMessageLengthInBytes = ((PTRNUMBER)attributeStartPos) - ((PTRNUMBER)(stun.mOriginalPacket));

            // remember the packet's original length
            WORD originalLength = ((WORD *)stun.mOriginalPacket)[1];
            // change the length for the sake of doing the message integrity calculation
            ((WORD *)stun.mOriginalPacket)[1] = htons(static_cast<WORD>(messageIntegrityMessageLengthInBytes + sizeof(DWORD) + sizeof(stun.mMessageIntegrity) - OPENPEER_STUN_HEADER_SIZE_IN_BYTES));

            CryptoPP::HMAC<CryptoPP::SHA1> hmac(&(key[0]), sizeof(key));
            hmac.Update(stun.mOriginalPacket, messageIntegrityMessageLengthInBytes);
//            hmac.Update(compare, len - 24);
            ZS_THROW_INVALID_ASSUMPTION_IF(sizeof(result) != hmac.DigestSize())

            // put back the original length
            ((WORD *)stun.mOriginalPacket)[1] = originalLength;

            hmac.Final(&(result[0]));

            memcpy(pos, &(result[0]), sizeof(result));
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      void packetizeErrorCode(BYTE *pos, const STUNPacket &stun)
      {
        ULONG hundreds = stun.mErrorCode / 100;
        ULONG tensOnes = stun.mErrorCode % 100;

        ((WORD *)pos)[1] = htons(static_cast<WORD>((hundreds << 8) | tensOnes));
        pos += sizeof(DWORD);

        if (stun.mReason.length() > 0) {
          memcpy(pos, (const BYTE *)((CSTR)stun.mReason), stun.mReason.length());
        }
      }

      //-----------------------------------------------------------------------
      void packetizeUnknownAttributes(BYTE *pos, const STUNPacket &stun)
      {
        for (STUNPacket::UnknownAttributeList::const_iterator iter = stun.mUnknownAttributes.begin(); iter != stun.mUnknownAttributes.end(); ++iter) {
          ((WORD *)pos)[0] = htons(*iter);
          pos += sizeof(WORD);
        }
      }

      //-----------------------------------------------------------------------
      void packetizeFingerprint(BYTE *pos, const STUNPacket &stun, const BYTE *startPos)
      {
        /*
         size_t len = strlen(gHexstream)/2;
         boost::shared_array<BYTE> blob(new BYTE[len*2]);
         BYTE *compare = blob.get();
         memset(compare, 0, len*2);
         convertHexStream(compare);
         */

        CryptoPP::CRC32 crc;
        PTRNUMBER size = ((PTRNUMBER)startPos) - ((PTRNUMBER)stun.mOriginalPacket);
        crc.Update(stun.mOriginalPacket, (size_t)size);
//        crc.Update(compare, len);

        ZS_THROW_INVALID_ASSUMPTION_IF(crc.DigestSize() != sizeof(DWORD))
        DWORD crcValue = 0;
        crc.Final((BYTE *)(&crcValue));
        crcValue ^= OPENPEER_STUN_MAGIC_XOR_FINGERPRINT_VALUE;

        ((DWORD *)pos)[0] = htonl(crcValue);
      }

      //-----------------------------------------------------------------------
      void packetizeCongestionControl(BYTE * &pos, const STUNPacket::CongestionControlList &list, bool directionBit)
      {
        if (list.size() < 1) return;  // no need to encode an empty list

        packetizeAttributeHeader(pos, STUNPacket::Attribute_CongestionControl, sizeof(WORD) + (sizeof(WORD)*(list.size())));

        // packetize the directional bit
        ((WORD *)pos)[0] = htons(0);
        pos[0] = (directionBit ? (1 << 7) : (0 << 7));
        pos += sizeof(WORD);

        for (STUNPacket::CongestionControlList::const_iterator iter = list.begin(); iter != list.end(); ++iter)
        {
          ((WORD *)pos)[0] = htons(static_cast<WORD>(*iter));
          pos += sizeof(WORD);
        }
      }

      //-----------------------------------------------------------------------
      void packetizeCongestionControl(BYTE * &pos, const STUNPacket &stun)
      {
        packetizeCongestionControl(pos, stun.mLocalCongestionControl, false);
        packetizeCongestionControl(pos, stun.mRemoteCongestionControl, true);
      }

      //-----------------------------------------------------------------------
      static void packetizeAttribute(
                                     BYTE * &pos,
                                     const STUNPacket &stun,
                                     STUNPacket::RFCs rfc,
                                     STUNPacket::Attributes attribute
                                     )
      {
        if (!stun.hasAttribute(attribute)) return;
        if (!shouldPacketizeAttribute(
                                      stun,
                                      rfc,
                                      attribute)) return;

        ULONG attributeLength = getActualAttributeLength(stun, attribute);

        BYTE *startPos = pos;
        const BYTE *cookiePos = (const BYTE *)(&(((DWORD *)stun.mOriginalPacket)[1]));

        switch (attribute)
        {
          // these attributes are actually mutiple attributes at once thus don't follow the standard header/value format since it's really header/value/header/value/... format
          case STUNPacket::Attribute_XORPeerAddress:    packetizePeerAddresses(pos, stun, cookiePos); return;
          case STUNPacket::Attribute_CongestionControl: packetizeCongestionControl(pos, stun); return;

          // all other attributes can be encoded using the standard header/value technique
          default: break;
        }

        packetizeAttributeHeader(pos, attribute, attributeLength);

        switch (attribute)
        {
          case STUNPacket::Attribute_MappedAddress:       packetizeIPAddress(pos, stun.mMappedAddress); break;
          case STUNPacket::Attribute_Username:            packetizeAttributeString(pos, stun.mUsername); break;
          case STUNPacket::Attribute_MessageIntegrity:    packetizeMessageIntegrity(pos, stun, startPos); break;
          case STUNPacket::Attribute_ErrorCode:           packetizeErrorCode(pos, stun); break;
          case STUNPacket::Attribute_UnknownAttribute:    packetizeUnknownAttributes(pos, stun); break;
          case STUNPacket::Attribute_Realm:               packetizeAttributeString(pos, stun.mRealm); break;
          case STUNPacket::Attribute_Nonce:               packetizeAttributeString(pos, stun.mNonce); break;
          case STUNPacket::Attribute_XORMappedAddress:    packetizeIPAddress(pos, stun.mMappedAddress, stun.mMagicCookie, cookiePos); break;
          case STUNPacket::Attribute_Software:            packetizeAttributeString(pos, stun.mSoftware); break;
          case STUNPacket::Attribute_AlternateServer:     packetizeIPAddress(pos, stun.mAlternateServer); break;
          case STUNPacket::Attribute_FingerPrint:         packetizeFingerprint(pos, stun, startPos); break;

          case STUNPacket::Attribute_ChannelNumber:       packetizeDWORD(pos, ((DWORD)stun.mChannelNumber) << 16); break;
          case STUNPacket::Attribute_Lifetime:            packetizeDWORD(pos, stun.mLifetime); break;
          case STUNPacket::Attribute_XORPeerAddress:      break;  // already handled this case
          case STUNPacket::Attribute_Data:                packetizeBuffer(pos, stun.mData, stun.mDataLength); break;
          case STUNPacket::Attribute_XORRelayedAddress:   packetizeIPAddress(pos, stun.mRelayedAddress, stun.mMagicCookie, cookiePos); break;
          case STUNPacket::Attribute_EvenPort:            *pos = (BYTE)((stun.mEvenPort ? 1 : 0) << 7); break;
          case STUNPacket::Attribute_RequestedTransport:  *pos = stun.mRequestedTransport; break;
          case STUNPacket::Attribute_DontFragment:        break;  // it is just a flag
          case STUNPacket::Attribute_ReservationToken:    packetizeBuffer(pos, &(stun.mReservationToken[0]), sizeof(stun.mReservationToken)); break;

          case STUNPacket::Attribute_Priority:            packetizeDWORD(pos, stun.mPriority); break;
          case STUNPacket::Attribute_UseCandidate:        break;  // it is just a flag
          case STUNPacket::Attribute_ICEControlled:       packetizeQWORD(pos, stun.mIceControlled); break;
          case STUNPacket::Attribute_ICEControlling:      packetizeQWORD(pos, stun.mIceControlling); break;

          case STUNPacket::Attribute_NextSequenceNumber:  packetizeQWORD(pos, stun.mNextSequenceNumber); break;
          case STUNPacket::Attribute_MinimumRTT:          packetizeDWORD(pos, stun.mMinimumRTT); break;
          case STUNPacket::Attribute_ConnectionInfo:      packetizeAttributeString(pos, stun.mConnectionInfo); break;
          case STUNPacket::Attribute_CongestionControl:   break;  // already handled this case
          case STUNPacket::Attribute_GSNR:                packetizeQWORD(pos, stun.mGSNR); break;
          case STUNPacket::Attribute_GSNFR:               packetizeQWORD(pos, stun.mGSNFR); break;
          case STUNPacket::Attribute_RUDPFlags:           packetizeDWORD(pos, 0); *pos = stun.mReliabilityFlags; break;
          case STUNPacket::Attribute_ACKVector:           packetizeBuffer(pos, stun.mACKVector.get(), stun.mACKVectorLength); break;

          default:                                        break;
        }

        pos += dwordBoundary(attributeLength);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    const char *STUNPacket::toString(Classes value)
    {
      switch (value) {
        case Class_Request:       return "request";
        case Class_Indication:    return "indication";
        case Class_Response:      return "response";
        case Class_ErrorResponse: return "error response";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *STUNPacket::toString(Methods value)
    {
      switch (value) {
        case Method_Binding:              return "binding";

          // TURN
        case Method_Allocate:             return "allocate";
        case Method_Refresh:              return "refresh";
        case Method_Send:                 return "send";
        case Method_Data:                 return "data";
        case Method_CreatePermission:     return "permission";
        case Method_ChannelBind:          return "channel bind";

          // RUDP
        case Method_ReliableChannelOpen:  return "RUDP channel open";
        case Method_ReliableChannelACK:   return "RUDP channel ack";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *STUNPacket::toString(Attributes value)
    {
      switch (value) {
        case Attribute_None:                    return "none";

        case Attribute_MappedAddress:           return "mapped address";
        case Attribute_Username:                return "username";
        case Attribute_MessageIntegrity:        return "message integrity";
        case Attribute_ErrorCode:               return "error code";
        case Attribute_UnknownAttribute:        return "unknown attribute";
        case Attribute_Realm:                   return "realm";
        case Attribute_Nonce:                   return "nonce";
        case Attribute_XORMappedAddress:        return "xor'ed mapped address";
        case Attribute_Software:                return "software";
        case Attribute_AlternateServer:         return "alternate address";
        case Attribute_FingerPrint:             return "fingerprint";

        case Attribute_ChannelNumber:           return "channel number";
        case Attribute_Lifetime:                return "lifetime";
        case Attribute_XORPeerAddress:          return "xor'ed peer address";
        case Attribute_Data:                    return "data";
        case Attribute_XORRelayedAddress:       return "xor'ed relayed address";
        case Attribute_EvenPort:                return "even port";
        case Attribute_RequestedTransport:      return "requested transport";
        case Attribute_DontFragment:            return "don't fragment";
        case Attribute_ReservationToken:        return "reserved token";

        case Attribute_Priority:                return "priority";
        case Attribute_UseCandidate:            return "use candidate";
        case Attribute_ICEControlled:           return "ICE controlled";
        case Attribute_ICEControlling:          return "ICE controlling";

        case Attribute_NextSequenceNumber:      return "next sequence number";
        case Attribute_MinimumRTT:              return "minimum RTT";
        case Attribute_ConnectionInfo:          return "connection info";
        case Attribute_CongestionControl:       return "congestion control";
        case Attribute_GSNR:                    return "GSNR";
        case Attribute_GSNFR:                   return "GSNFR";
        case Attribute_RUDPFlags:               return "RUDP flags";
        case Attribute_ACKVector:               return "ACK vector";

        case Attribute_ReservedResponseAddress: return "obsolete response address";
        case Attribute_ReservedChangeAddress:   return "obsolete change address";
        case Attribute_ReservedSourceAddress:   return "obsolete source address";
        case Attribute_ReservedChangedAddress:  return "obsolete changed address";
        case Attribute_ReservedPassword:        return "obsolete password";
        case Attribute_ReservedReflectedFrom:   return "obsolete reflected from";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *STUNPacket::toString(CredentialMechanisms value)
    {
      switch (value) {
        case CredentialMechanisms_None:       return "none";
        case CredentialMechanisms_ShortTerm:  return "short term";
        case CredentialMechanisms_LongTerm:   return "long term";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *STUNPacket::toString(ErrorCodes value)
    {
      switch (value) {
        case ErrorCode_TryAlternate:                  return "try alternate";
        case ErrorCode_BadRequest:                    return "bad request";
        case ErrorCode_Unauthorized:                  return "unauthorized";
        case ErrorCode_UnknownAttribute:              return "unknown attribute";
        case ErrorCode_StaleNonce:                    return "stale nonce";
        case ErrroCode_ServerError:                   return "server error";

        case ErrorCode_Forbidden:                     return "forbidden";
        case ErrorCode_AllocationMismatch:            return "allocation mismatch";
        case ErrorCode_WrongCredentials:              return "wrong credentials";
        case ErrorCode_UnsupportedTransportProtocol:  return "unsupported transport protocol";
        case ErrorCode_AllocationQuoteReached:        return "allocation quota reached";
        case ErrorCode_InsufficientCapacity:          return "insufficient capacity";

        case ErrorCode_RoleConflict:                  return "role conflict";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *STUNPacket::toString(Protocols value)
    {
      switch (value) {
        case Protocol_None: return "none";
        case Protocol_UDP:  return "UDP";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    static void addMore(String &original, const char *name, STUNPacket::RFCs enumed, STUNPacket::RFCs value)
    {
      if ((enumed & value) == 0) return;

      if (!original.isEmpty()) {
        original += " | ";
      }
      original += name;
    }

    //-------------------------------------------------------------------------
    String STUNPacket::toString(RFCs value)
    {
      String result;
      addMore(result, "RFC-3489 STUN", RFC_3489_STUN, value);
      addMore(result, "RFC-5389 STUN", RFC_5389_STUN, value);
      addMore(result, "RFC-5766 TURN", RFC_5766_TURN, value);
      addMore(result, "RFC-3489 ICE", RFC_5245_ICE, value);
      addMore(result, "RFC-draft RUDP", RFC_draft_RUDP, value);
      return result;
    }

    //-------------------------------------------------------------------------
    STUNPacket::STUNPacket() :
      mLogObject("STUNPacket"),
      mLogObjectID(zsLib::createPUID()),
      mClass(Class_Request),
      mMethod(Method_Binding),
      mTotalRetries(0),
      mErrorCode(0),
      mCredentialMechanism(CredentialMechanisms_None),
      mOriginalPacket(NULL),
      mMessageIntegrityMessageLengthInBytes(0),
      mMagicCookie(OPENPEER_STUN_MAGIC_COOKIE),
      mFingerprintIncluded(true),
      mChannelNumber(0),
      mLifetimeIncluded(false),
      mLifetime(0),
      mData(NULL),
      mDataLength(0),
      mEvenPortIncluded(false),
      mEvenPort(true),
      mRequestedTransport(Protocol_None),
      mDontFragmentIncluded(false),
      mReservationTokenIncluded(false),
      mPriorityIncluded(false),
      mPriority(0),
      mUseCandidateIncluded(false),
      mIceControlledIncluded(false),
      mIceControlled(false),
      mIceControllingIncluded(false),
      mIceControlling(0),
      mNextSequenceNumber(0),
      mMinimumRTTIncluded(false),
      mMinimumRTT(0),
      mGSNR(0),
      mGSNFR(0),
      mReliabilityFlagsIncluded(false),
      mReliabilityFlags(0),
      mACKVectorLength(0)
    {
      memset(&(mTransactionID[0]), 0, sizeof(mTransactionID));  // reset it to zero
      memset(&(mMessageIntegrity[0]), 0, sizeof(mMessageIntegrity));
      memset(&(mReservationToken[0]), 0, sizeof(mReservationToken));
    }

    //-------------------------------------------------------------------------
    STUNPacketPtr STUNPacket::createRequest(
                                            Methods method,
                                            const char *software
                                            )
    {
      STUNPacketPtr stun(new STUNPacket);
      stun->mClass = Class_Request;
      stun->mMethod = method;
      if (software)
        stun->mSoftware = software;

      CryptoPP::AutoSeededRandomPool rng;
      rng.GenerateBlock(&(stun->mTransactionID[0]), sizeof(stun->mTransactionID));
      return stun;
    }

    //-------------------------------------------------------------------------
    STUNPacketPtr STUNPacket::createIndication(
                                               Methods method,
                                               const char *software
                                               )
    {
      STUNPacketPtr stun = STUNPacket::createRequest(method, software);
      stun->mClass = Class_Indication;
      if (software)
        stun->mSoftware = software;
      return stun;
    }

    //-------------------------------------------------------------------------
    STUNPacketPtr STUNPacket::createResponse(
                                             STUNPacketPtr request,
                                             const char *software
                                             )
    {
      STUNPacketPtr stun(new STUNPacket);
      stun->mClass = Class_Response;
      stun->mMethod = request->mMethod;
      stun->mMagicCookie = request->mMagicCookie;
      stun->mFingerprintIncluded = request->mFingerprintIncluded;
      if (software)
        stun->mSoftware = software;

      // copy the transaction ID
      memcpy(&(stun->mTransactionID[0]), &(request->mTransactionID[0]), sizeof(stun->mTransactionID));
      return stun;
    }

    //-------------------------------------------------------------------------
    STUNPacketPtr STUNPacket::createErrorResponse(
                                                  STUNPacketPtr request,
                                                  const char *software
                                                  )
    {
      STUNPacketPtr stun = STUNPacket::createResponse(request);
      stun->mClass = Class_ErrorResponse;
      if (software)
        stun->mSoftware = software;

      stun->mErrorCode = request->mErrorCode;
      if (0 == stun->mErrorCode)
        stun->mErrorCode = ErrroCode_ServerError; // something gone wrong with the server for this to be the error code

      const char *reason = NULL;
      switch (stun->mErrorCode) {
        case ErrorCode_TryAlternate:                  reason = "Try Alternate"; break;
        case ErrorCode_BadRequest:                    reason = "Bad Request"; break;
        case ErrorCode_Unauthorized:                  reason = "Unauthorized"; break;
        case ErrorCode_UnknownAttribute:              reason = "Unknown Attribute"; break;
        case ErrorCode_StaleNonce:                    reason = "Stale Nonce"; break;
        case ErrroCode_ServerError:                   reason = "Server Error"; break;

          // RFC 5766 TURN specific error codes
        case ErrorCode_Forbidden:                     reason = "Forbidden"; break;
        case ErrorCode_AllocationMismatch:            reason = "Allocation Mismatch"; break;
        case ErrorCode_WrongCredentials:              reason = "Wrong Credentials"; break;
        case ErrorCode_UnsupportedTransportProtocol:  reason = "Unsupported Transport Protocol"; break;
        case ErrorCode_AllocationQuoteReached:        reason = "Allocation Quota Reached"; break;
        case ErrorCode_InsufficientCapacity:          reason = "Insufficient Capacity"; break;

          // RFC 5245 ICE specific error codes
        case ErrorCode_RoleConflict:                  reason = "Role Conflict"; break;
        default:                                      reason = "Unknown"; break;
      }
      stun->mReason = reason;

      // if the reason was because of unknown attributes then add the unknown attributes
      if (ErrorCode_UnknownAttribute == stun->mErrorCode)
        stun->mUnknownAttributes = request->mUnknownAttributes;

      return stun;
    }

    //-------------------------------------------------------------------------
    STUNPacketPtr STUNPacket::clone(bool changeTransactionID) const
    {
      STUNPacketPtr dest(new STUNPacket);
      dest->mOriginalPacket = mOriginalPacket;
      dest->mClass = mClass;
      dest->mMethod = mMethod;
      dest->mTotalRetries = mTotalRetries;
      dest->mErrorCode = mErrorCode;
      dest->mReason = mReason;
      dest->mMagicCookie = mMagicCookie;
      memcpy(&(dest->mTransactionID[0]), &(mTransactionID[0]), sizeof(mTransactionID));
      dest->mUnknownAttributes = mUnknownAttributes;
      dest->mMappedAddress = mMappedAddress;
      dest->mAlternateServer = mAlternateServer;
      dest->mUsername = mUsername;
      dest->mPassword = mPassword;
      dest->mRealm = mRealm;
      dest->mNonce = mNonce;
      dest->mSoftware = mSoftware;
      dest->mCredentialMechanism = mCredentialMechanism;
      dest->mMessageIntegrityMessageLengthInBytes = mMessageIntegrityMessageLengthInBytes;
      memcpy(&(dest->mMessageIntegrity[0]), &(mMessageIntegrity[0]), sizeof(mMessageIntegrity));
      dest->mFingerprintIncluded = mFingerprintIncluded;

      // TURN attributes
      dest->mChannelNumber = mChannelNumber;
      dest->mLifetimeIncluded = mLifetimeIncluded;
      dest->mLifetime = mLifetime;
      dest->mPeerAddressList = mPeerAddressList;
      dest->mRelayedAddress = mRelayedAddress;
      dest->mData = mData;
      dest->mDataLength = mDataLength;
      dest->mEvenPortIncluded = mEvenPortIncluded;
      dest->mEvenPort = mEvenPort;
      dest->mRequestedTransport = mRequestedTransport;
      dest->mDontFragmentIncluded = mDontFragmentIncluded;
      dest->mReservationTokenIncluded = mReservationTokenIncluded;
      memcpy(&(dest->mReservationToken[0]), &(mReservationToken[0]), sizeof(mReservationToken));

      // ICE attributes
      dest->mPriorityIncluded = mPriorityIncluded;
      dest->mPriority = mPriority;
      dest->mUseCandidateIncluded = mUseCandidateIncluded;
      dest->mIceControlledIncluded = mIceControlledIncluded;
      dest->mIceControlled = mIceControlled;
      dest->mIceControllingIncluded = mIceControllingIncluded;
      dest->mIceControlling = mIceControlling;

      // RUDP attributes
      dest->mNextSequenceNumber = mNextSequenceNumber;
      dest->mMinimumRTTIncluded = mMinimumRTTIncluded;
      dest->mMinimumRTT = mMinimumRTT;
      dest->mConnectionInfo = mConnectionInfo;
      dest->mGSNR = mGSNR;
      dest->mGSNFR = mGSNFR;
      dest->mReliabilityFlagsIncluded = mReliabilityFlagsIncluded;
      dest->mReliabilityFlags = mReliabilityFlags;
      dest->mACKVector = mACKVector;
      dest->mACKVectorLength = mACKVectorLength;
      dest->mLocalCongestionControl = mLocalCongestionControl;
      dest->mRemoteCongestionControl = mRemoteCongestionControl;

      if (changeTransactionID) {
        CryptoPP::AutoSeededRandomPool rng;
        rng.GenerateBlock(&(dest->mTransactionID[0]), sizeof(dest->mTransactionID));
      }
      return dest;
    }

    //-------------------------------------------------------------------------
    STUNPacketPtr STUNPacket::parseIfSTUN(
                                          const BYTE *packet,
                                          ULONG packetLengthInBytes,
                                          RFCs allowedRFCs,
                                          bool allowRFC3489,
                                          const char *logObject,
                                          PUID logObjectID
                                          )
    {
      // 0                   1                   2                   3
      // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|0 0|     STUN Message Type     |         Message Length        |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|                         Magic Cookie                          |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|                                                               |
      //|                     Transaction ID (96 bits)                  |
      //|                                                               |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

      // 0                 1
      // 2  3  4 5 6 7 8 9 0 1 2 3 4 5
      //+--+--+-+-+-+-+-+-+-+-+-+-+-+-+
      //|M |M |M|M|M|C|M|M|M|C|M|M|M|M|
      //|11|10|9|8|7|1|6|5|4|0|3|2|1|0|
      //+--+--+-+-+-+-+-+-+-+-+-+-+-+-+

      ZS_THROW_INVALID_USAGE_IF(!packet)

      // All STUN messages MUST start with a 20-byte header followed by zero or more Attributes.
      if (packetLengthInBytes < OPENPEER_STUN_HEADER_SIZE_IN_BYTES) return STUNPacketPtr();

      //The most significant 2 bits of every STUN message MUST be zeroes.
      if (0 != (packet[0] & 0xC0)) return STUNPacketPtr();

      // The magic cookie field MUST contain the fixed value OPENPEER_STUN_MAGIC_COOKIE in network byte order.
      DWORD magicCookie = ntohl(((DWORD *)packet)[1]);
      if ((!allowRFC3489) &&
          (OPENPEER_STUN_MAGIC_COOKIE != magicCookie)) return STUNPacketPtr();

      WORD messageType = ntohs(((WORD *)packet)[0]);
      WORD messageTypeClass = ((messageType & 0x100) >> 7) | ((messageType & 0x10) >> 4);
      WORD messageTypeMethod = ((messageType & 0x3E00) >> 2) | ((messageType & 0xE0) >> 1) | (messageType & 0xF);
      WORD messageLengthInBytes = htons(((WORD *)packet)[1]);

      // The message length MUST contain the size, in bytes, of the message
      // not including the 20-byte STUN header.  Since all STUN attributes are
      // padded to a multiple of 4 bytes, the last 2 bits of this field are
      // always zero.  This provides another way to distinguish STUN packets
      // from packets of other protocols.
      if (0 != (messageLengthInBytes & 0x3)) return STUNPacketPtr();
      if (packetLengthInBytes < ((ULONG)OPENPEER_STUN_HEADER_SIZE_IN_BYTES) + messageLengthInBytes) return STUNPacketPtr();  // this is illegal since the size is larger than the actual packet received

      if (0 != (messageLengthInBytes % sizeof(DWORD))) return STUNPacketPtr(); // every attribute is aligned to a DWORD size

      ULONG availableBytes = messageLengthInBytes;
      const BYTE *pos = packet + OPENPEER_STUN_HEADER_SIZE_IN_BYTES;  //

      STUNPacketPtr stun(new STUNPacket);
      stun->mMagicCookie = magicCookie;
      stun->mClass = static_cast<Classes>(messageTypeClass);
      stun->mMethod = static_cast<Methods>(messageTypeMethod);

      memcpy(&((stun->mTransactionID)[0]), &(((DWORD *)packet)[2]), sizeof(stun->mTransactionID));

      // only process classes and types that are understood
      switch (messageTypeClass)
      {
        case Class_Request:       break;
        case Class_Indication:    break;
        case Class_Response:      break;
        case Class_ErrorResponse: break;
        default: return STUNPacketPtr();
      }

      if (!internal::isLegalMethod(stun->mMethod, stun->mClass, allowedRFCs)) return STUNPacketPtr();

      bool foundIntegrity = false;
      bool foundFingerprint = false;

      while (availableBytes > 0) {
        bool handleAsUnknownAttribute = false;

        ZS_THROW_BAD_STATE_IF(0 != (availableBytes % sizeof(DWORD)))  // every attribute is aligned to a DWORD size, so if it isn't then something is coded wrong

        ZS_THROW_BAD_STATE_IF(availableBytes < sizeof(DWORD))         // this can't be!

        WORD attributeType = ntohs(((WORD *)pos)[0]);
        WORD attributeLength = ntohs(((WORD *)pos)[1]);
        const BYTE *attributeStart = pos;

        pos += sizeof(DWORD);
        availableBytes -= sizeof(DWORD);

        ULONG fullAttributeLength = internal::dwordBoundary(attributeLength);
        if (fullAttributeLength > availableBytes) return STUNPacketPtr(); // illegal attribute length?

        const BYTE *dataPos = pos;
        pos += fullAttributeLength;
        availableBytes -= fullAttributeLength;

        if (!internal::isAttributeKnown(allowedRFCs, (Attributes)attributeType))
          handleAsUnknownAttribute = true;

        if (foundIntegrity) {
          switch (attributeType) {
            case Attribute_FingerPrint: break;    // fingerprint may come after integrity but nothing else known does come after integrity
            default: {
              handleAsUnknownAttribute = true;
              break; // sorry, not allowed to add these attributes after integrity
            }
          }
        }

        // From RFC: When present, the FINGERPRINT attribute MUST be the last attribute in the message, and thus will appear after MESSAGE-INTEGRITY.
        if (foundFingerprint)
          handleAsUnknownAttribute = true;

        if (!handleAsUnknownAttribute) {
          switch (attributeType) {

            case Attribute_AlternateServer:
            case Attribute_MappedAddress:   {
              IPAddress &useAddress = (Attribute_MappedAddress == attributeType ? stun->mMappedAddress : stun->mAlternateServer);
              if (!internal::parseMappedAddress(dataPos, attributeLength, 0, NULL, useAddress, handleAsUnknownAttribute)) return STUNPacketPtr();
              break;
            }

            case Attribute_Username:         if (!internal::parseSTUNString(dataPos, attributeLength, OPENPEER_STUN_MAX_USERNAME, stun->mUsername)) return STUNPacketPtr(); break;

            case Attribute_MessageIntegrity: {
              // found integrity but without knowing which usename and password to use there is no way to authenticate it, so that has to be done later
              foundIntegrity = true;
              if (attributeLength < sizeof(stun->mMessageIntegrity)) return STUNPacketPtr();
              memcpy(&(stun->mMessageIntegrity[0]), dataPos, sizeof(stun->mMessageIntegrity));
              stun->mOriginalPacket = packet;
              stun->mMessageIntegrityMessageLengthInBytes = ((PTRNUMBER)attributeStart) - ((PTRNUMBER)packet);
              break;
            }

            case Attribute_ErrorCode: {
              if (attributeLength < sizeof(DWORD)) return STUNPacketPtr();
              WORD code = ntohs(((WORD *)dataPos)[1]);
              WORD hundredsDigit = (0x700 & code) >> 8;
              WORD twoDigits = (0xFF & code);
              if (((hundredsDigit < 3) || (hundredsDigit > 6)) ||
                   (twoDigits > 99)) {
                handleAsUnknownAttribute = true;
                break;
              }
              stun->mErrorCode = (hundredsDigit * 100) + twoDigits;
              if (!internal::parseSTUNString(dataPos + sizeof(DWORD), attributeLength - sizeof(DWORD), OPENPEER_STUN_MAX_REASON, stun->mReason)) return STUNPacketPtr();
              break;
            }

            case Attribute_UnknownAttribute: {
              if (0 != (attributeLength % sizeof(WORD))) return STUNPacketPtr();
              while (attributeLength >= 2) {
                stun->mUnknownAttributes.push_back(htons(((WORD *)dataPos)[0]));
                dataPos += sizeof(WORD);
                attributeLength -= 2;
              }
              break;
            }

            case Attribute_Realm:            if (!internal::parseSTUNString(dataPos, attributeLength, OPENPEER_STUN_MAX_REALM, stun->mRealm)) return STUNPacketPtr(); break;

            case Attribute_Nonce:            if (!internal::parseSTUNString(dataPos, attributeLength, OPENPEER_STUN_MAX_REALM, stun->mNonce)) return STUNPacketPtr(); break;

            case Attribute_XORMappedAddress: {
              if (!internal::parseMappedAddress(dataPos, attributeLength, magicCookie, (const BYTE *)(&(((DWORD *)packet)[1])), stun->mMappedAddress, handleAsUnknownAttribute)) return STUNPacketPtr();
              break;
            }

            case Attribute_Software:        if (!internal::parseSTUNString(dataPos, attributeLength, OPENPEER_STUN_MAX_SOFTWARE, stun->mSoftware)) return STUNPacketPtr(); break;

            case Attribute_FingerPrint:         {
              if (attributeLength < sizeof(DWORD)) return STUNPacketPtr();
              foundFingerprint = true;
              CryptoPP::CRC32 crc;
              PTRNUMBER size = ((PTRNUMBER)attributeStart) - ((PTRNUMBER)packet);
              crc.Update(packet, (size_t)size);
              ZS_THROW_INVALID_ASSUMPTION_IF(crc.DigestSize() != sizeof(DWORD))
              DWORD crcValue = 0;
              crc.Final((BYTE *)(&crcValue));
              crcValue ^= OPENPEER_STUN_MAGIC_XOR_FINGERPRINT_VALUE;
              if (crcValue != ntohl(((DWORD *)dataPos)[0])) return STUNPacketPtr();
              stun->mFingerprintIncluded = true;
              break;
            }

            // RFC5766 TURN specific attributes
            case Attribute_ChannelNumber:         {
              if (attributeLength < sizeof(WORD)) return STUNPacketPtr();
              stun->mChannelNumber = ntohs(((WORD *)dataPos)[0]);
              break;
            }
            case Attribute_Lifetime:              {
              if (attributeLength < sizeof(DWORD)) return STUNPacketPtr();
              stun->mLifetimeIncluded = true;
              stun->mLifetime = ntohl(((DWORD *)dataPos)[0]);
              break;
            }
            case Attribute_XORPeerAddress:        {
              IPAddress temp;
              if (!internal::parseMappedAddress(dataPos, attributeLength, magicCookie, (const BYTE *)(&(((DWORD *)packet)[1])), temp, handleAsUnknownAttribute)) return STUNPacketPtr();

              if (!handleAsUnknownAttribute) {
                stun->mPeerAddressList.push_back(temp);
              }
              break;
            }
            case Attribute_Data:                  {
              stun->mData = dataPos;
              stun->mDataLength = attributeLength;
              break;
            }
            case Attribute_XORRelayedAddress:     {
              if (!internal::parseMappedAddress(dataPos, attributeLength, magicCookie, (const BYTE *)(&(((DWORD *)packet)[1])), stun->mRelayedAddress, handleAsUnknownAttribute)) return STUNPacketPtr();
              break;
            }
            case Attribute_EvenPort:              {
              if (attributeLength < sizeof(BYTE)) return STUNPacketPtr();
              stun->mEvenPortIncluded = true;
              stun->mEvenPort = (0 != ((dataPos[0] >> 7) & 1));
              break;
            }
            case Attribute_RequestedTransport:    {
              if (attributeLength < sizeof(BYTE)) return STUNPacketPtr();
              stun->mRequestedTransport = dataPos[0];
              break;
            }
            case Attribute_DontFragment:          stun->mDontFragmentIncluded = true; break;
            case Attribute_ReservationToken:      {
              if (attributeLength < sizeof(stun->mReservationToken)) return STUNPacketPtr();
              memcpy(&(stun->mReservationToken[0]), &(dataPos[0]), sizeof(stun->mReservationToken));
              break;
            }

            // RFC5245 ICE specific attributes
            case Attribute_Priority:              {
              if (attributeLength < sizeof(DWORD)) return STUNPacketPtr();
              stun->mPriorityIncluded = true;
              stun->mPriority = ntohl(((DWORD *)dataPos)[0]);
              break;
            }
            case Attribute_UseCandidate:          stun->mUseCandidateIncluded = true; break;

            case Attribute_ICEControlled:         {
              if (attributeLength < sizeof(QWORD)) return STUNPacketPtr();
              stun->mIceControlledIncluded = true;
              stun->mIceControlled = internal::parseQWORD(dataPos);
              break;
            }
            case Attribute_ICEControlling:        {
              if (attributeLength < sizeof(QWORD)) return STUNPacketPtr();
              stun->mIceControllingIncluded = true;
              stun->mIceControlling = internal::parseQWORD(dataPos);
              break;
            }

            // RUDP specific attributes
            case STUNPacket::Attribute_NextSequenceNumber:  {
              if (attributeLength < sizeof(QWORD)) return STUNPacketPtr();
              stun->mNextSequenceNumber = internal::parseQWORD(dataPos);
              break;
            }
            case STUNPacket::Attribute_MinimumRTT:          {
              if (attributeLength < sizeof(DWORD)) return STUNPacketPtr();
              stun->mMinimumRTTIncluded = true;
              stun->mMinimumRTT = ntohl(((DWORD *)dataPos)[0]);
              break;
            }
            case STUNPacket::Attribute_ConnectionInfo:      if (!internal::parseSTUNString(dataPos, attributeLength, OPENPEER_STUN_MAX_CONNECTION_INFO, stun->mConnectionInfo)) return STUNPacketPtr(); break;
            case STUNPacket::Attribute_CongestionControl:   {
              if (attributeLength < sizeof(DWORD)) return STUNPacketPtr();
              if (0 != (attributeLength % sizeof(WORD))) return STUNPacketPtr();

              bool direction = (0 != (dataPos[0] & (1 << 7)));
              CongestionControlList list;
              // must be at least one congestion control profile offered added otherwise it is illegal
              dataPos += sizeof(WORD);  // skip over the header
              ULONG length = ((attributeLength - sizeof(WORD)) / sizeof(WORD));
              for (; length > 0; --length) {
                list.push_back(static_cast<IRUDPChannel::CongestionAlgorithms>(ntohs(((WORD *)dataPos)[0])));
                dataPos += sizeof(WORD);
              }
              if (direction)
                stun->mRemoteCongestionControl = list;
              else
                stun->mLocalCongestionControl = list;
              break;
            }
            case STUNPacket::Attribute_GSNR:                {
              if (attributeLength < sizeof(QWORD)) return STUNPacketPtr();
              stun->mGSNR = internal::parseQWORD(dataPos);
              break;
            }
            case STUNPacket::Attribute_GSNFR:               {
              if (attributeLength < sizeof(QWORD)) return STUNPacketPtr();
              stun->mGSNFR = internal::parseQWORD(dataPos);
              break;
            }
            case STUNPacket::Attribute_RUDPFlags:           {
              if (attributeLength < sizeof(BYTE)) return STUNPacketPtr();
              stun->mReliabilityFlagsIncluded = true;
              stun->mReliabilityFlags = dataPos[0];
              break;
            }
            case STUNPacket::Attribute_ACKVector:           {
              if (attributeLength < sizeof(BYTE)) return STUNPacketPtr();
              boost::shared_array<BYTE> buffer(new BYTE[attributeLength]);
              memcpy(buffer.get(), dataPos, attributeLength);
              stun->mACKVector = buffer;
              stun->mACKVectorLength = attributeLength;
              break;
            }

            // obsolete STUN attributes should be ignored
            case STUNPacket::Attribute_ReservedResponseAddress:
            case STUNPacket::Attribute_ReservedChangeAddress:
            case STUNPacket::Attribute_ReservedSourceAddress:
            case STUNPacket::Attribute_ReservedChangedAddress:
            case STUNPacket::Attribute_ReservedPassword:
            case STUNPacket::Attribute_ReservedReflectedFrom: break;    // just ignore all these attributes

            default:                        handleAsUnknownAttribute = true; break;
          }

          if (handleAsUnknownAttribute) {
            stun->mUnknownAttributes.push_back(attributeType);  // did not understand this attribute
            if ((0 == stun->mErrorCode) &&
                (internal::isComprehensionRequired(attributeType))) {
              stun->mErrorCode = ErrorCode_UnknownAttribute;
            }
          }
        }
      }

      // Now that we have parsed the packet, we have to guess which RFC is truly belongs and restrict any attributes to only those allowed on the guessed RFC
      RFCs guessedRFC = stun->guessRFC(allowedRFCs);

      // go through all the attributes and check it they are still legal
      for (size_t loop = 0; Attribute_None != internal::gAttributeOrdering[loop]; ++loop) {
        if (stun->hasAttribute(internal::gAttributeOrdering[loop])) {
          // this attribute is found but is it legal for the RFC?
          if (!internal::isAttributeLegal(*(stun.get()), guessedRFC, internal::gAttributeOrdering[loop])) {
            if (0 == stun->mErrorCode)                                                // if there is already an error code on the request then don't put another one
              stun->mErrorCode = ErrorCode_UnknownAttribute;                          // this request has an illegal attribute on it that required understanding but is not allowed in this RFC
            stun->mUnknownAttributes.push_back(internal::gAttributeOrdering[loop]);   // this is an illegal attribute
          }
        } else {
          if (internal::isAttributeRequired(*(stun.get()), guessedRFC, internal::gAttributeOrdering[loop])) {
            // this attribute was required but it was missing!
            switch (stun->mClass) {
              case STUNPacket::Class_Request:
              case STUNPacket::Class_Indication:      {
                if (0 == stun->mErrorCode)                  // if there is already an error code on the request then don't put another one
                  stun->mErrorCode = ErrorCode_BadRequest;  // this request is clearly bad
                break;
              }
              case STUNPacket::Class_Response:
              case STUNPacket::Class_ErrorResponse:   {
                if (0 == stun->mErrorCode) {                                              // if there is already an error code on the request then don't put another one
                  stun->mErrorCode = ErrorCode_UnknownAttribute;                          // this request is clearly bad but specifically an attribute is missing
                  stun->mUnknownAttributes.push_back(internal::gAttributeOrdering[loop]); // where is this missing attribute?
                }
                break;
              }
            }
          }
        }
      }

      if (NULL != logObject)
        stun->mLogObject = logObject;
      if (0 != logObjectID)
        stun->mLogObjectID = logObjectID;
      stun->log(Log::Trace, "parse");
      return stun;
    }

    //-------------------------------------------------------------------------
    STUNPacket::ParseLookAheadStates STUNPacket::parseStreamIfSTUN(
                                                                   STUNPacketPtr &outSTUN,
                                                                   ULONG &outActualSizeInBytes,
                                                                   const BYTE *packet,
                                                                   ULONG streamDataAvailableInBytes,
                                                                   RFCs allowedRFC,
                                                                   bool allowRFC3489,
                                                                   const char *logObject,
                                                                   PUID logObjectID
                                                                   )
    {
      //0                   1                   2                   3
      //0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|0 0|     STUN Message Type     |         Message Length        |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|                         Magic Cookie                          |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|                                                               |
      //|                     Transaction ID (96 bits)                  |
      //|                                                               |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      ZS_THROW_INVALID_USAGE_IF(!packet)

      outSTUN = STUNPacketPtr();
      outActualSizeInBytes = 0;

      if (streamDataAvailableInBytes < sizeof(BYTE)) return ParseLookAheadState_InsufficientDataToDeterimine;

      //The most significant 2 bits of every STUN message MUST be zeroes.
      if (0 != (packet[0] & 0xC0)) return ParseLookAheadState_NotSTUN;

      if (streamDataAvailableInBytes < sizeof(WORD)) return ParseLookAheadState_InsufficientDataToDeterimine;

      WORD messageType = ntohs(((WORD *)packet)[0]);
      WORD messageTypeClass = ((messageType & 0x100) >> 7) | ((messageType & 0x10) >> 4);
      WORD messageTypeMethod = ((messageType & 0x3E00) >> 2) | ((messageType & 0xE0) >> 1) | (messageType & 0xF);

      switch (messageTypeClass)
      {
        case Class_Request:       break;
        case Class_Indication:    break;
        case Class_Response:      break;
        case Class_ErrorResponse: break;
        default:                  return ParseLookAheadState_NotSTUN;
      }

      // only process classes and types that are understood
      if (!internal::isLegalMethod(static_cast<STUNPacket::Methods>(messageTypeMethod), static_cast<STUNPacket::Classes>(messageTypeClass), allowedRFC)) return ParseLookAheadState_NotSTUN;

      if (streamDataAvailableInBytes < sizeof(DWORD)) return ParseLookAheadState_InsufficientDataToDeterimine;

      WORD messageLengthInBytes = htons(((WORD *)packet)[1]);
      if (0 != (messageLengthInBytes % sizeof(DWORD))) return ParseLookAheadState_NotSTUN;  // every attribute is aligned to a DWORD size

      // The message length MUST contain the size, in bytes, of the message
      // not including the 20-byte STUN header.  Since all STUN attributes are
      // padded to a multiple of 4 bytes, the last 2 bits of this field are
      // always zero.  This provides another way to distinguish STUN packets
      // from packets of other protocols.
      if (0 != (messageLengthInBytes & 0x3)) return ParseLookAheadState_NotSTUN;

      if (streamDataAvailableInBytes < (sizeof(DWORD)*2)) return ParseLookAheadState_InsufficientDataToDeterimine;

      // The magic cookie field MUST contain the fixed value OPENPEER_STUN_MAGIC_COOKIE in network byte order.
      DWORD magicCookie = ntohl(((DWORD *)packet)[1]);
      if ((!allowRFC3489) &&
          (OPENPEER_STUN_MAGIC_COOKIE != magicCookie)) return ParseLookAheadState_NotSTUN;

      outActualSizeInBytes = OPENPEER_STUN_HEADER_SIZE_IN_BYTES + messageLengthInBytes;

      // All STUN messages MUST start with a 20-byte header followed by zero or more Attributes.
      if (streamDataAvailableInBytes < OPENPEER_STUN_HEADER_SIZE_IN_BYTES) return ParseLookAheadState_AppearsSTUNButPacketNotFullyAvailable;

      if (streamDataAvailableInBytes < ((ULONG)OPENPEER_STUN_HEADER_SIZE_IN_BYTES) + messageLengthInBytes) return ParseLookAheadState_AppearsSTUNButPacketNotFullyAvailable;

      // we not have enough data available to truly determine if this is a STUN packet and decode this STUN packet
      outSTUN = parseIfSTUN(
                            packet,
                            outActualSizeInBytes,
                            allowedRFC,
                            allowRFC3489,
                            logObject,
                            logObjectID
                            );
      if (!outSTUN) {
        outActualSizeInBytes = 0;
        return ParseLookAheadState_NotSTUN;
      }

      return ParseLookAheadState_STUNPacket;
    }

    //-------------------------------------------------------------------------
    const char *STUNPacket::classAsString() const
    {
      return toString(mClass);
    }

    //-------------------------------------------------------------------------
    const char *STUNPacket::methodAsString() const
    {
      return toString(mMethod);
    }

    //-------------------------------------------------------------------------
    void STUNPacket::log(
                         Log::Level level,
                         const char *logMessage
                         ) const
    {
      if (ZS_GET_LOG_LEVEL() < level) return;

      // scope: log information
      {
        String message;

        message = String(mLogObject) + " [" + string(mLogObjectID) + "] "
                  + (logMessage ? logMessage : "<unspecified>")
                  + ", RFC=" + toString(guessRFC(RFC_AllowAll))
                  + ", class=" + classAsString()
                  + ", method=" + methodAsString()
                  + ", cookie=" + string(mMagicCookie, 16)
                  + ", ID=" + internal::convertToHex(&(mTransactionID[0]), sizeof(mTransactionID));

        if (0 != mTotalRetries)
          message += ", retries=" + string(mTotalRetries);

        if (hasAttribute(STUNPacket::Attribute_ErrorCode)) {
          message += ", error code=" + string(mErrorCode) + " (" + toString((ErrorCodes)mErrorCode) + ")";
          message += ", reason=" + mReason;
        }

        if (hasAttribute(STUNPacket::Attribute_UnknownAttribute)) {
          message += ", unknown=";
          UnknownAttributeList::const_iterator iter = mUnknownAttributes.begin();
          for (bool first = true; iter != mUnknownAttributes.end(); ++iter, first = false) {
            message += (first ? "" : ";") + string(*iter);
          }
        }
        if (hasAttribute(STUNPacket::Attribute_MappedAddress)) {
          message += ", mapped address=" + mMappedAddress.string();
        }
        if (hasAttribute(STUNPacket::Attribute_AlternateServer)) {
          message += ", alternative server=" + mAlternateServer.string();
        }
        if (hasAttribute(STUNPacket::Attribute_Username)) {
          message += ", username=" + mUsername;
        }
        if (!mPassword.isEmpty()) {
          message += ", password=" + mPassword;
        }
        if (hasAttribute(STUNPacket::Attribute_Realm)) {
          message += ", realm=" + mRealm;
        }
        if (hasAttribute(STUNPacket::Attribute_Nonce)) {
          message += ", nonce=" + mNonce;
        }
        if (hasAttribute(STUNPacket::Attribute_Software)) {
          message += ", software=" + mSoftware;
        }
        if (CredentialMechanisms_None != mCredentialMechanism) {
          message += String(", credentials=") + (toString(mCredentialMechanism));
        }
        if ((hasAttribute(STUNPacket::Attribute_MessageIntegrity)) &&
             (0 != mMessageIntegrityMessageLengthInBytes)) {
          message += ", message integrity length=" + string(mMessageIntegrityMessageLengthInBytes);
          message += ", message integrity data=" + internal::convertToHex(&(mMessageIntegrity[0]), sizeof(mMessageIntegrity));
        }
        if (hasAttribute(STUNPacket::Attribute_FingerPrint)) {
          message += ", fingerprint=true";
        }
        if (hasAttribute(STUNPacket::Attribute_ChannelNumber)) {
          message += ", channel=" + string(mChannelNumber);
        }
        if (hasAttribute(STUNPacket::Attribute_Lifetime)) {
          message += ", lifetime=" + string(mLifetime);
        }
        if (hasAttribute(STUNPacket::Attribute_XORPeerAddress)) {
          message += ", peer address=";
          PeerAddressList::const_iterator iter = mPeerAddressList.begin();
          for (bool first = true; iter != mPeerAddressList.end(); ++iter, first = false) {
            message += (first ? "" : ";") + (*iter).string();
          }
        }
        if (hasAttribute(STUNPacket::Attribute_XORRelayedAddress)) {
          message += ", relayed address=" + mRelayedAddress.string();
        }
        if (hasAttribute(STUNPacket::Attribute_Data)) {
          message += ", data length=" + string(mDataLength);
          message += String(", data=") + (mData ? "true" : "false");
        }
        if (hasAttribute(STUNPacket::Attribute_EvenPort)) {
          message += String(", even port=") + (mEvenPort ? "true" : "false");
        }
        if (hasAttribute(STUNPacket::Attribute_RequestedTransport)) {
          message += ", data length=" + string(((UINT)mRequestedTransport));
        }
        if (hasAttribute(STUNPacket::Attribute_DontFragment)) {
          message += ", don't fragment=true";
        }
        if (hasAttribute(STUNPacket::Attribute_ReservationToken)) {
          message += ", reservation token=" + internal::convertToHex(&(mReservationToken[0]), sizeof(mReservationToken));
        }
        if (hasAttribute(STUNPacket::Attribute_Priority)) {
          message += ", data length=" + string(mPriority);
        }
        if (hasAttribute(STUNPacket::Attribute_UseCandidate)) {
          message += ", use candidate=true";
        }
        if (hasAttribute(STUNPacket::Attribute_ICEControlled)) {
          message += ", ice controlled=" + string(mIceControlled);
        }
        if (hasAttribute(STUNPacket::Attribute_ICEControlling)) {
          message += ", ice controlling=" + string(mIceControlling);
        }
        if (hasAttribute(STUNPacket::Attribute_NextSequenceNumber)) {
          message += ", next sequence number=" + string(mNextSequenceNumber) + " (" +  + string(mNextSequenceNumber & 0xFFFFFF) + ")";
        }
        if (hasAttribute(STUNPacket::Attribute_NextSequenceNumber)) {
          message += ", minimum RTT=" + string(mMinimumRTT);
        }
        if (hasAttribute(STUNPacket::Attribute_ConnectionInfo)) {
          message += ", connection info=" + mConnectionInfo;
        }
        if (hasAttribute(STUNPacket::Attribute_GSNR)) {
          message += ", gsnr=" + string(mGSNR) + " (" +  + string(mGSNR & 0xFFFFFF) + ")";
        }
        if (hasAttribute(STUNPacket::Attribute_GSNFR)) {
          message += ", gsnfr=" + string(mGSNFR) + " (" +  + string(mGSNFR & 0xFFFFFF) + ")";
        }
        if (hasAttribute(STUNPacket::Attribute_RUDPFlags)) {
          message += ", flags=" + string(((UINT)mReliabilityFlags));
          message += ", flag details=";
          message += "(--)";  // (ps)
          if (0 != (RUDPPacket::Flag_PG_ParityGSNR & mReliabilityFlags)) {
            message += "(pg)";
          } else {
            message += "(--)";
          }
          if (0 != (RUDPPacket::Flag_XP_XORedParityToGSNFR & mReliabilityFlags)) {
            message += "(xp)";
          } else {
            message += "(--)";
          }
          if (0 != (RUDPPacket::Flag_DP_DuplicatePacket & mReliabilityFlags)) {
            message += "(dp)";
          } else {
            message += "(--)";
          }
          if (0 != (RUDPPacket::Flag_EC_ECNPacket & mReliabilityFlags)) {
            message += "(ec)";
          } else {
            message += "(--)";
          }
          message += "(--)";  // (eq)
          message += "(--)";  // (ar)
          if (0 != (RUDPPacket::Flag_VP_VectorParity & mReliabilityFlags)) {
            message += "(vp)";
          } else {
            message += "(--)";
          }
        }
        if (hasAttribute(STUNPacket::Attribute_ACKVector)) {
          message += ", vector length=" + string(mACKVectorLength);
          if (0 != mACKVectorLength) {
            message += ", vector=" + internal::convertToHex(mACKVector.get(), mACKVectorLength);
          }
        }
        if (hasAttribute(STUNPacket::Attribute_CongestionControl)) {
          if (mLocalCongestionControl.size() > 0)
          {
            message += ", congestion local=";
            CongestionControlList::const_iterator iter = mLocalCongestionControl.begin();
            for (bool first = true; iter != mLocalCongestionControl.end(); ++iter, first = false) {
              message += (first ? "" : ";") + string(((UINT)*iter));
            }
          }
          if (mRemoteCongestionControl.size() > 0)
          {
            message += ", congestion remote=";
            CongestionControlList::const_iterator iter = mRemoteCongestionControl.begin();
            for (bool first = true; iter != mRemoteCongestionControl.end(); ++iter, first = false) {
              message += (first ? "" : ";") + string(((UINT)*iter));
            }
          }
        }
        ZS_LOG(Basic, message)
      }
    }

    //-------------------------------------------------------------------------
    bool STUNPacket::isLegal(RFCs rfc) const
    {
      for (size_t loop = 0; Attribute_None != internal::gAttributeOrdering[loop]; ++loop) {
        if (hasAttribute(internal::gAttributeOrdering[loop])) {
          // has the attribute but is it legal?
          if (!internal::isAttributeLegal(*this, rfc, internal::gAttributeOrdering[loop])) {
            ZS_LOG_WARNING(Trace, "found an illegal attribute, RFC=" + toString(rfc) + ", attribute=" + toString(internal::gAttributeOrdering[loop]))
            return false;
          }
        } else {
          // does not have the attribute but is it required?
          if (internal::isAttributeRequired(*this, rfc, internal::gAttributeOrdering[loop])) {
            ZS_LOG_WARNING(Trace, "missing an attribute, RFC=" + toString(rfc) + ", attribute=" + toString(internal::gAttributeOrdering[loop]))
            return false;
          }
        }
      }
      return true;
    }

    //-------------------------------------------------------------------------
    STUNPacket::RFCs STUNPacket::guessRFC(RFCs allowedRFCs) const
    {
      return internal::guessRFC(*this, allowedRFCs);
    }

    //-------------------------------------------------------------------------
    void STUNPacket::packetize(
                               boost::shared_array<BYTE> &outPacket,
                               ULONG &outPacketLengthInBytes,
                               RFCs rfc
                               )
    {
      log(Log::Trace, "packetize");

      outPacketLengthInBytes = OPENPEER_STUN_HEADER_SIZE_IN_BYTES;

      // count the length of all the attributes when they are packetized
      {
        for (size_t loop = 0; Attribute_None != internal::gAttributeOrdering[loop]; ++loop) {
          if (hasAttribute(internal::gAttributeOrdering[loop])) {
            if (internal::shouldPacketizeAttribute(*this, rfc, internal::gAttributeOrdering[loop])) {
              outPacketLengthInBytes += internal::packetizeAttributeLength(*this, rfc, internal::gAttributeOrdering[loop]);
            }
          }
        }
      }


      //**********************************************************************
      // now we know the packet size, we fill it up...
      outPacket = boost::shared_array<BYTE>(new BYTE[outPacketLengthInBytes]);

      BYTE *packet = outPacket.get();
      memset(packet, 0, outPacketLengthInBytes);

      //0                   1                   2                   3
      //0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|0 0|     STUN Message Type     |         Message Length        |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|                         Magic Cookie                          |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      //|                                                               |
      //|                     Transaction ID (96 bits)                  |
      //|                                                               |
      //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

      WORD messageType = ((0x02 & ((WORD)mClass)) << 7) | ((0x01 & ((WORD)mClass)) << 4);
      messageType |= ((0xF80 & ((WORD)mMethod)) << 2) | ((0x70 & ((WORD)mMethod)) << 1) | (0xF & ((WORD)mMethod));

      ((WORD *)packet)[0] = htons(messageType);
      ((WORD *)packet)[1] = htons((WORD)(outPacketLengthInBytes - OPENPEER_STUN_HEADER_SIZE_IN_BYTES));

      ((DWORD *)packet)[1] = htonl(mMagicCookie);
      memcpy(&(((DWORD *)packet)[2]), &(mTransactionID[0]), sizeof(mTransactionID));

      BYTE *pos = packet + OPENPEER_STUN_HEADER_SIZE_IN_BYTES;
      mOriginalPacket = packet;

      // packetize all the packets now...
      {
        for (size_t loop = 0; Attribute_None != internal::gAttributeOrdering[loop]; ++loop) {
          if (hasAttribute(internal::gAttributeOrdering[loop])) {
            if (internal::shouldPacketizeAttribute(*this, rfc, internal::gAttributeOrdering[loop])) {
              internal::packetizeAttribute(pos, *this, rfc, internal::gAttributeOrdering[loop]);
            }
          }
        }
      }
    }

    //-------------------------------------------------------------------------
    bool STUNPacket::isValidResponseTo(
                                       STUNPacketPtr request,
                                       RFCs allowedRFCs
                                       )
    {
      ZS_THROW_INVALID_USAGE_IF(!request)

      if (request->mClass != Class_Request) return false; // this isn't even a request

      switch (mClass) {
        case Class_Request:       return false;
        case Class_Indication:    return false;

        case Class_Response:
        case Class_ErrorResponse: break;

        default:                  return false;
      }

      if (mMethod != request->mMethod) return false;                            // the methods must match
      if (mMagicCookie != request->mMagicCookie) return false;                  // the magic cookie must match

      if (0 != memcmp(&(mTransactionID[0]), &(request->mTransactionID[0]), sizeof(mTransactionID))) return false; // the transaction ID must match

      RFCs usingRFC = request->guessRFC(allowedRFCs);

      // now we check if the reply is legal and has all the attributes required
      if (!isLegal(usingRFC)) return false;

      if (request->hasAttribute(Attribute_MessageIntegrity)) {
        if ((mClass == Class_Response) &&
            (!hasAttribute(Attribute_MessageIntegrity)))
          return false; // if the request had message integrity then the response must as well
      }

      // this is a valid response although further checks might be required by the caller
      return true;
    }

    //-------------------------------------------------------------------------
    bool STUNPacket::isValidMessageIntegrity(
                                             const char *password,
                                             const char *username,
                                             const char *realm
                                             ) const
    {
      ZS_THROW_INVALID_USAGE_IF(!mOriginalPacket)  // this must exist or the validation can't happen

      if (NULL == password)
        password = "";

      CryptoPP::Weak::MD5 md5;

      BYTE key[16];
      memset(&(key[0]), 0, sizeof(key));

      if ((NULL != username) && (NULL != realm)) {
        md5.Update((const BYTE *)username, strlen(username));
        md5.Update((const BYTE *)":", strlen(":"));
        md5.Update((const BYTE *)realm, strlen(realm));
        md5.Update((const BYTE *)":", strlen(":"));
      }
      md5.Update((const BYTE *)password, strlen(password));

      ZS_THROW_INVALID_ASSUMPTION_IF(sizeof(key) != md5.DigestSize())
      md5.Final(&(key[0]));

      BYTE result[OPENPEER_STUN_MESSAGE_INTEGRITY_LENGTH_IN_BYTES];
      memset(&(result[0]), 0, sizeof(result));

      // we have to smash the original packet length to do calculation then put it back after...
      WORD originalLength = ((WORD *)mOriginalPacket)[1];
      // for the sake of message integrity we have to set the length to the original size up to and including the message interity attribute
      ((WORD *)mOriginalPacket)[1] = htons(static_cast<WORD>(mMessageIntegrityMessageLengthInBytes + sizeof(DWORD) + sizeof(mMessageIntegrity) - OPENPEER_STUN_HEADER_SIZE_IN_BYTES));

      CryptoPP::HMAC<CryptoPP::SHA1> hmac(&(key[0]), sizeof(key));
      hmac.Update(mOriginalPacket, mMessageIntegrityMessageLengthInBytes);
      ZS_THROW_INVALID_ASSUMPTION_IF(sizeof(result) != hmac.DigestSize())

      // put back the original value now...
      ((WORD *)mOriginalPacket)[1] = originalLength;

      hmac.Final(&(result[0]));
      return (0 == memcmp(&(mMessageIntegrity[0]), &(result[0]), sizeof(result)));
    }

    //-------------------------------------------------------------------------
    bool STUNPacket::isRFC3489() const
    {
      return OPENPEER_STUN_MAGIC_COOKIE != mMagicCookie;
    }

    //-------------------------------------------------------------------------
    bool STUNPacket::isRFC5389() const
    {
      return OPENPEER_STUN_MAGIC_COOKIE == mMagicCookie;
    }

    //-------------------------------------------------------------------------
    bool STUNPacket::hasAttribute(Attributes attribute) const
    {
      switch (attribute)
      {
        case Attribute_MappedAddress:       return (isRFC3489()) && (!mMappedAddress.isAddressEmpty());
        case Attribute_Username:            return !mUsername.isEmpty();
        case Attribute_MessageIntegrity:    return (0 != mMessageIntegrityMessageLengthInBytes) || (CredentialMechanisms_None != mCredentialMechanism);
        case Attribute_ErrorCode:           return 0 != mErrorCode;
        case Attribute_UnknownAttribute:    return mUnknownAttributes.size() > 0;
        case Attribute_Realm:               return !mRealm.isEmpty();
        case Attribute_Nonce:               return !mNonce.isEmpty();
        case Attribute_XORMappedAddress:    return (!isRFC3489()) && (!mMappedAddress.isAddressEmpty());
        case Attribute_Software:            return !mSoftware.isEmpty();
        case Attribute_AlternateServer:     return !mAlternateServer.isAddressEmpty();
        case Attribute_FingerPrint:         return mFingerprintIncluded;

        case Attribute_ChannelNumber:       return (0 != mChannelNumber);
        case Attribute_Lifetime:            return mLifetimeIncluded;
        case Attribute_XORPeerAddress:      return (mPeerAddressList.size() > 0);
        case Attribute_Data:                return ((NULL != mData) && (0 != mDataLength));
        case Attribute_XORRelayedAddress:   return !mRelayedAddress.isAddressEmpty();
        case Attribute_EvenPort:            return mEvenPortIncluded;
        case Attribute_RequestedTransport:  return Protocol_None != mRequestedTransport;
        case Attribute_DontFragment:        return mDontFragmentIncluded;
        case Attribute_ReservationToken:    return mReservationTokenIncluded;

        case Attribute_Priority:            return mPriorityIncluded;
        case Attribute_UseCandidate:        return mUseCandidateIncluded;
        case Attribute_ICEControlled:       return mIceControlledIncluded;
        case Attribute_ICEControlling:      return mIceControllingIncluded;

        case Attribute_NextSequenceNumber:  return (0 != mNextSequenceNumber);
        case Attribute_MinimumRTT:          return mMinimumRTTIncluded;
        case Attribute_ConnectionInfo:      return !mConnectionInfo.isEmpty();
        case Attribute_CongestionControl:   return (mLocalCongestionControl.size() > 0) || (mRemoteCongestionControl.size() > 0);
        case Attribute_GSNR:                return (0 != mGSNR);
        case Attribute_GSNFR:               return (0 != mGSNFR);
        case Attribute_RUDPFlags:           return mReliabilityFlagsIncluded;
        case Attribute_ACKVector:           return (0 != mACKVectorLength);

        // obsolete attributes
        case Attribute_ReservedResponseAddress:
        case Attribute_ReservedChangeAddress:
        case Attribute_ReservedSourceAddress:
        case Attribute_ReservedChangedAddress:
        case Attribute_ReservedPassword:
        case Attribute_ReservedReflectedFrom: return false;   // just ignore all these attributes

        default:                            break;
      }

      return false;
    }

    //-------------------------------------------------------------------------
    bool STUNPacket::hasUnknownAttribute(Attributes attribute)
    {
      return (mUnknownAttributes.end() != find(mUnknownAttributes.begin(), mUnknownAttributes.end(), attribute));
    }

    //-------------------------------------------------------------------------
    ULONG STUNPacket::getTotalRoomAvailableForData(
                                                          ULONG maxPacketSizeInBytes,
                                                          RFCs rfc
                                                          ) const
    {
      ULONG packetLengthInBytes = OPENPEER_STUN_HEADER_SIZE_IN_BYTES;

      // count the length of all the attributes when they are packetized
      {
        for (size_t loop = 0; Attribute_None != internal::gAttributeOrdering[loop]; ++loop) {
          if (hasAttribute(internal::gAttributeOrdering[loop])) {
            if (internal::shouldPacketizeAttribute(*this, rfc, internal::gAttributeOrdering[loop])) {
              packetLengthInBytes += internal::packetizeAttributeLength(*this, rfc, internal::gAttributeOrdering[loop]);
            }
          }
        }
      }

      // if not even enough room for the current packet then we report 0 bytes avilable
      if (packetLengthInBytes > maxPacketSizeInBytes) return 0;

      // make sure to account for the size of one attribute header and the alignment the data will require to the next DWORD
      ULONG remainder = maxPacketSizeInBytes - packetLengthInBytes;
      if (remainder < (sizeof(DWORD)*2)) return 0;

      // the amount of space available is knocked down by the size of the header
      return remainder - sizeof(DWORD);
    }
  }
}
