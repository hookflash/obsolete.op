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
#include <openpeer/services/IRUDPChannel.h>

#include <zsLib/IPAddress.h>
#include <zsLib/Log.h>
#include <zsLib/String.h>

#include <boost/shared_array.hpp>

#define OPENPEER_STUN_MESSAGE_INTEGRITY_LENGTH_IN_BYTES (20)
#define OPENPEER_SERVICES_CLIENT_SOFTARE_DECLARATION "openpeer STUN 1.0"

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark STUNPacket
    #pragma mark

    struct STUNPacket
    {
    public:
      enum Classes {
        Class_Request =       0x00,
        Class_Indication =    0x01,
        Class_Response =      0x02,
        Class_ErrorResponse = 0x03,
      };

      static const char *toString(Classes value);

      enum Methods {
        // STUN + ICE
        Method_Binding =              0x0001,

        // TURN
        Method_Allocate =             0x0003,
        Method_Refresh =              0x0004,
        Method_Send =                 0x0006,
        Method_Data =                 0x0007,
        Method_CreatePermission =     0x0008,
        Method_ChannelBind =          0x0009,

        // RUDP
        Method_ReliableChannelOpen =  0x0174,
        Method_ReliableChannelACK =   0x0175,
      };

      static const char *toString(Methods value);

      enum Attributes {
        Attribute_None =                    0x0000,

        Attribute_MappedAddress =           0x0001,
        Attribute_Username =                0x0006,
        Attribute_MessageIntegrity =        0x0008,
        Attribute_ErrorCode =               0x0009,
        Attribute_UnknownAttribute =        0x000A,
        Attribute_Realm =                   0x0014,
        Attribute_Nonce =                   0x0015,
        Attribute_XORMappedAddress =        0x0020,
        Attribute_Software =                0x8022,
        Attribute_AlternateServer =         0x8023,
        Attribute_FingerPrint =             0x8028,

        // RFC5766 TURN specific attributes
        Attribute_ChannelNumber =           0x000C,
        Attribute_Lifetime =                0x000D,
        Attribute_XORPeerAddress =          0x0012,
        Attribute_Data =                    0x0013,
        Attribute_XORRelayedAddress =       0x0016,
        Attribute_EvenPort =                0x0018,
        Attribute_RequestedTransport =      0x0019,
        Attribute_DontFragment =            0x001A,
        Attribute_ReservationToken =        0x0022,

        // RFC5245 ICE specific attributes
        Attribute_Priority =                0x0024,
        Attribute_UseCandidate =            0x0025,
        Attribute_ICEControlled =           0x8029,
        Attribute_ICEControlling =          0x802A,

        // RFC_draft_RUDP
        Attribute_NextSequenceNumber =      0x1740,
        Attribute_MinimumRTT =              0x1741,
        Attribute_ConnectionInfo =          0x1742,
        Attribute_CongestionControl =       0x1743,
        Attribute_GSNR =                    0x1744,
        Attribute_GSNFR =                   0x1745,
        Attribute_RUDPFlags =               0x1746,
        Attribute_ACKVector =               0x1747,

        // obsolete attributes that should be ignored
        Attribute_ReservedResponseAddress = 0x0002,
        Attribute_ReservedChangeAddress =   0x0003,
        Attribute_ReservedSourceAddress =   0x0004,
        Attribute_ReservedChangedAddress =  0x0005,
        Attribute_ReservedPassword =        0x0007,
        Attribute_ReservedReflectedFrom =   0x000B,
      };

      static const char *toString(Attributes value);

      enum CredentialMechanisms {
        CredentialMechanisms_None,
        CredentialMechanisms_ShortTerm,
        CredentialMechanisms_LongTerm,
      };

      static const char *toString(CredentialMechanisms value);

      enum ErrorCodes {
        ErrorCode_TryAlternate =                  300,
        ErrorCode_BadRequest =                    400,
        ErrorCode_Unauthorized =                  401,
        ErrorCode_UnknownAttribute =              420,
        ErrorCode_StaleNonce =                    438,
        ErrroCode_ServerError =                   500,

        // RFC 5766 TURN specific error codes
        ErrorCode_Forbidden =                     403,
        ErrorCode_AllocationMismatch =            437,
        ErrorCode_WrongCredentials =              441,
        ErrorCode_UnsupportedTransportProtocol =  442,
        ErrorCode_AllocationQuoteReached =        486,
        ErrorCode_InsufficientCapacity =          508,

        // RFC 5245 ICE specific error codes
        ErrorCode_RoleConflict =                  487,
      };

      static const char *toString(ErrorCodes value);

      enum Protocols {
        Protocol_None =   255,
        Protocol_UDP =     17,
      };

      static const char *toString(Protocols value);

      enum RFCs {
        RFC_3489_STUN =   0x01,   // classic STUN
        RFC_5389_STUN =   0x02,   // revised STUN
        RFC_5766_TURN =   0x04,
        RFC_5245_ICE =    0x08,
        RFC_draft_RUDP =  0x10,

        RFC_AllowAll    = RFC_3489_STUN | RFC_5389_STUN | RFC_5766_TURN | RFC_5245_ICE | RFC_draft_RUDP,
      };

      static String toString(RFCs value);

      enum ParseLookAheadStates {
        ParseLookAheadState_NotSTUN,
        ParseLookAheadState_InsufficientDataToDeterimine,
        ParseLookAheadState_AppearsSTUNButPacketNotFullyAvailable,
        ParseLookAheadState_STUNPacket
      };

    public:
      STUNPacket();

      static STUNPacketPtr createRequest(
                                         Methods method,
                                         const char *software = OPENPEER_SERVICES_CLIENT_SOFTARE_DECLARATION
                                         );
      static STUNPacketPtr createIndication(
                                            Methods method,
                                            const char *software = OPENPEER_SERVICES_CLIENT_SOFTARE_DECLARATION
                                            );

      static STUNPacketPtr createResponse(
                                          STUNPacketPtr request,
                                          const char *software = OPENPEER_SERVICES_CLIENT_SOFTARE_DECLARATION
                                          );
      static STUNPacketPtr createErrorResponse(
                                               STUNPacketPtr request,
                                               const char *software = OPENPEER_SERVICES_CLIENT_SOFTARE_DECLARATION
                                               );          // put the error code into the request

      STUNPacketPtr clone(bool changeTransactionID) const;

      static STUNPacketPtr parseIfSTUN(                                         // returns empty smart pointer if wasn't a STUN packet
                                       const BYTE *packet,               // NOTE: While this is const we have to overwrite the packet data momentarily during MESSAGE-INTEGRITY calculation
                                       ULONG packetLengthInBytes,
                                       RFCs allowedRFCs,
                                       bool allowRFC3489 = true,
                                       const char *logObject = NULL,
                                       PUID logObjectID = 0
                                       );

      static ParseLookAheadStates parseStreamIfSTUN(
                                                    STUNPacketPtr &outSTUN,
                                                    ULONG &outActualSizeInBytes,
                                                    const BYTE *packet,        // NOTE: While this is const we have to overwrite the packet data momentarily during MESSAGE-INTEGRITY calculation
                                                    ULONG streamDataAvailableInBytes,
                                                    RFCs allowedRFCs,
                                                    bool allowRFC3489 = false,
                                                    const char *logObject = NULL,
                                                    PUID logObjectID = 0
                                                    );

      const char *classAsString() const;
      const char *methodAsString() const;
      void log(
               Log::Level level = Log::Debug,
               const char *logMessage = NULL
               ) const;

      bool isLegal(RFCs rfc) const;
      RFCs guessRFC(RFCs allowedRFCs) const;

      void packetize(
                     boost::shared_array<BYTE> &outPacket,
                     ULONG &outPacketLengthInBytes,
                     RFCs rfc
                     );

      bool isValidResponseTo(
                             STUNPacketPtr stunRequest,
                             RFCs allowedRFCs
                             );

      bool isValidMessageIntegrity(
                                   const char *password,                // must be SASLprep(password)
                                   const char *username = NULL,
                                   const char *realm = NULL
                                   ) const;

      bool isRFC3489() const;
      bool isRFC5389() const;

      bool hasAttribute(Attributes attribute) const;
      bool hasUnknownAttribute(Attributes attribute);

      ULONG getTotalRoomAvailableForData(
                                         ULONG maxPacketSizeInBytes,
                                         RFCs rfc
                                         ) const;

    public:
      const char *mLogObject;                                   // when output to a log, which object was responsible for this packet (never packetized or parsed)
      PUID mLogObjectID;                                        // when output to a log, which object ID was responsible for this packet (never packetized or parsed)

      const BYTE *mOriginalPacket;                              // NOTE: This is only valid as long as the packet which it was parsed from is valid and must be valid for the validate routine
                                                                // ALSO: Because the RFC demands the length of the packet during MESSAGE-INTEGRITY be set to the value as if the MESSAGE-INTEGRITY but not including anything after we have to overwrite the length in the original packet momentarily then put back original value

      Classes mClass;
      Methods mMethod;
      ULONG mTotalRetries;                                      // this is not an attribute that is encoded, instead it can be used to determine how many retries of a request have happened

      WORD mErrorCode;                                          // if there was an error during the parse of the request, the error will be put here otherwise will be "0"
      String mReason;

      DWORD  mMagicCookie;                                      // if this is 0x2112A442 then this is the new RFC otherwise it is RFC3489
      BYTE   mTransactionID[96/8];

      // attributes

      // RFC 5389 STUN Attributes
      typedef std::list<WORD> UnknownAttributeList;
      UnknownAttributeList mUnknownAttributes;                  // if the request was valid but attributes were unknown, the list of unknown attributes will be here (and 420 in the error code if the attribute was manditory)

      IPAddress mMappedAddress;
      IPAddress mAlternateServer;

      String mUsername;                                         // NOTE: STUN responses will not contain the username
      String mPassword;                                         // NOTE: Must have had SASLprep(password) previously applied
      String mRealm;
      String mNonce;

      String mSoftware;

      CredentialMechanisms mCredentialMechanism;
      ULONG mMessageIntegrityMessageLengthInBytes;              // how big is the input into the HMAC algorithm, including 20 byte header -- it is the length of the packet up to but not including the message integrity attribute or message-integrity value
      BYTE  mMessageIntegrity[OPENPEER_STUN_MESSAGE_INTEGRITY_LENGTH_IN_BYTES];   // the message integrity of the packet

      bool mFingerprintIncluded;                                // should this packet include the fingerprint (or did include the fingerprint)


      // RFC 5766 TURN attributes
      WORD mChannelNumber;                                      // 0x0000 -> 0x3FFF = illegal, 0x4000 -> 0x7FFF = valid, 0x8000 -> 0xFFFF = reserved. The CHANNEL-NUMBER attribute contains the number of the channel.

      bool mLifetimeIncluded;                                   // set this to true if the LIFETIME attribute should be included
      DWORD mLifetime;                                          // The LIFETIME attribute represents the duration for which the server will maintain an allocation in the absence of a refresh.

      typedef std::list<IPAddress> PeerAddressList;
      PeerAddressList mPeerAddressList;                         // The XOR-PEER-ADDRESS specifies the address and port of the peer as seen from the TURN server
      IPAddress mRelayedAddress;                                // The XOR-RELAYED-ADDRESS is present in Allocate responses.

      const BYTE *mData;                                        // this is only valid as long as the packet/object the data was parsed from is valid
      ULONG mDataLength;

      bool mEvenPortIncluded;                                   // Set this to true if the "EVEN-PORT" attribute should be included
      bool mEvenPort;                                           // This attribute allows the client to request that the port in the
                                                                // relayed transport address be even, and (optionally) that the server
                                                                // reserve the next-higher port number.

      BYTE mRequestedTransport;                                 // This attribute is used by the client to request a specific transport protocol for the allocated transport address. 17 decimal = UDP

      bool mDontFragmentIncluded;                               // This attribute is used by the client to request that the server set
                                                                // the DF (Don't Fragment) bit in the IP header when relaying the application data onward to the peer.

      bool mReservationTokenIncluded;                           // set this to true if the RESERVATION-TOKEN attribute should be included
      BYTE mReservationToken[8];                                // The RESERVATION-TOKEN attribute contains a token that uniquely identifies a relayed transport address being held in reserve by the server.

      // RFC 5245 ICE attributes
      bool mPriorityIncluded;
      DWORD mPriority;                                          // The PRIORITY attribute indicates the priority that is to be associated with a peer reflexive candidate, should one be discovered by this check.

      bool mUseCandidateIncluded;                               // The USE-CANDIDATE attribute indicates that the candidate pair resulting from this check should be used for transmission of media.

      bool mIceControlledIncluded;                              // set to true if the ICE-CONTROLLED attrbute should be included
      QWORD mIceControlled;                                     // The ICE-CONTROLLED attribute is present in a Binding request and indicates that the client believes it is currently in the controlled role.

      bool mIceControllingIncluded;                             // set to true if the ICE-CONTROLLING attribute should be included
      QWORD mIceControlling;                                    // The ICE-CONTROLLING attribute is present in a Binding request and indicates that the client believes it is currently in the controlling role.


      // RFC_draft_RUDP
      QWORD mNextSequenceNumber;                                // 0 means this value is not set

      bool mMinimumRTTIncluded;
      DWORD mMinimumRTT;

      String mConnectionInfo;                                   // additional connection information in RUDP

      QWORD mGSNR;                                              // 0 means this value is not set
      QWORD mGSNFR;                                             // 0 means this value is not set

      bool mReliabilityFlagsIncluded;
      BYTE mReliabilityFlags;

      boost::shared_array<BYTE> mACKVector;                     // if set, points to the buffer containing the RLE ACK vector
      ULONG mACKVectorLength;                                   // how long is the ACK vector (if non-zero then mACKVector must be set)

      typedef std::list<IRUDPChannel::CongestionAlgorithms> CongestionControlList;
      CongestionControlList mLocalCongestionControl;
      CongestionControlList mRemoteCongestionControl;
    };
  }
}
