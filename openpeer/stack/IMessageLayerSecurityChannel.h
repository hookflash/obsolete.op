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

#include <openpeer/stack/types.h>
#include <openpeer/stack/message/types.h>

namespace openpeer
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageLayerSecurityChannel
    #pragma mark

    interaction IMessageLayerSecurityChannel
    {
      static String toDebugString(IMessageLayerSecurityChannelPtr channel, bool includeCommaPrefix = true);

      enum SessionStates
      {
        SessionState_SendOnly,
        SessionState_Connected,
        SessionState_Shutdown,
      };

      enum LocalPublicKeyReferenceTypes
      {
        LocalPublicKeyReferenceType_FullPublicKey,                // in this mode, the remote party may not know anything about the local peer thus include the full public key in the channel negotiation
        LocalPublicKeyReferenceType_PeerURI,                      // in this mode the remote party is presumed to know the local peer's public peer file so the signature used in the channel negotiations will reference the local public peer file by peer URI
        LocalPublicKeyReferenceType_FingerPrint,                  // in this mode the remote party is presumed to know the local peer's public key in advance (but not the local peer file public) so reference the public key by its fingerprint
      };

      enum RemotePublicKeyReferenceTypes
      {
        RemotePublicKeyReferenceType_Unknown,                     // information about the remote party is not known yet
        RemotePublicKeyReferenceType_FullPublicKey,               // the remote party referenced a full public key
        RemotePublicKeyReferenceType_PeerURI,                     // the remote party referenced a peer URI in its signatures that was resolved into a full public key
        RemotePublicKeyReferenceType_DomainCertificatesGet,       // the remote party referenced a domain where the "certificates get" was used to resolve into a full public key
      };

      static const char *toString(SessionStates state);
      static const char *toString(LocalPublicKeyReferenceTypes type);
      static const char *toString(RemotePublicKeyReferenceTypes type);

      //-----------------------------------------------------------------------
      // PURPOSE: create a new channel to a remote connection
      static IMessageLayerSecurityChannelPtr create(
                                                    IMessageLayerSecurityChannelDelegatePtr delegate,
                                                    IPeerFilesPtr localPeerFiles,                                   // needs a public / private keer pair to operate
                                                    LocalPublicKeyReferenceTypes localPublicKeyReferenceType,       // how should the local public key be referenced in the MLS channel negotiations
                                                    IAccountPtr account = IAccountPtr()                             // if specified, the account object will be used to resolve peer URI remotely referenced public keys
                                                    );

      //-----------------------------------------------------------------------
      // PURPOSE: get process unique ID for object
      virtual PUID getID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: immediately disconnects the channel (no signaling is needed)
      virtual void cancel() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: return the current statte of the connection
      virtual SessionStates getState(
                                     WORD *outLastErrorCode = NULL,
                                     String *outLastErrorReason = NULL
                                     ) const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Send a message object over the MLS channel to the remote peer
      // NOTE:    messages are queued until a send connection is established
      virtual bool send(message::MessagePtr message) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains the next pending message received over the wire.
      // NOTE:    Messages are queued in the public key referenced in the
      //          cryptographic negotiations is resolved.
      //          Returns MessagePtr() if no message is pending.
      virtual message::MessagePtr getNextPendingIncomingMessage() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This method will return the remote public key (if known).
      virtual RemotePublicKeyReferenceTypes getRemotePublicKeyReferencedType() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This method will return the the remote public key (if known).
      // NOTE:    This method only returns the remote public key if the
      //          public key was sent by the remote party, or the remote
      //          public key was resolved by its siganture reference.
      virtual IRSAPublicKeyPtr getRemotePublicKey() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This method will return the remotely referenced
      //          peer file public.
      // NOTE:    This method returns a valid peer file public if the remote
      //          party referenced a peer URI in its cryptographic signature
      //          which was resolved to a peer file public locally.
      virtual IPeerFilePublicPtr getRemoteReferencedPeerFilePublic() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: This method will return the remotely referenced service
      //          domain referenced in the cyrptography signature.
      // NOTE:    This method returns a string with the remote domain (and
      //          optional service) attached to this connectionn if the
      //          remote party referenced a verifified domain/service as the
      //          party responsible for this connection in its cryptographic
      //          signatures.
      virtual String getRemoteReferencedDomain(String *outService = NULL) const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains the next pending data buffer that needs to be
      //          delivered over-the-wire.
      // NOTE:    Messages are queued for delivery until the wire protocol is
      //          ready to deliver the buffered data.
      virtual SecureByteBlockPtr getNextPendingBufferToSendOnWrite() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notifies of data received on-the-wire from a remote party's
      //          channel connected to this MLS channel object.
      // NOTE:    This routine will parse and extract out the "message" object
      //          or handle apply the cryptographic keying materials specified
      //          in the buffer.
      virtual void notifyReceivedFromWire(
                                          const BYTE *buffer,
                                          ULONG bufferLengthInBytes
                                          ) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageLayerSecurityChannelDelegate
    #pragma mark

    interaction IMessageLayerSecurityChannelDelegate
    {
      typedef IMessageLayerSecurityChannel::SessionStates SessionStates;

      //-----------------------------------------------------------------------
      // PURPOSE: Notifies the delegate that the state of the connection
      //          has changed.
      // NOTE:    If the cryptographic keying is discovered to be incorrect
      //          the channel will shutdown and getState() will return an
      //          error code.
      virtual void onMessageLayerSecurityChannelStateChanged(
                                                             IMessageLayerSecurityChannelPtr channel,
                                                             SessionStates state
                                                             ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notifies the delegate of an incoming message that will be
      //          queued until ready to be read.
      virtual void onMessageLayerSecurityChannelIncomingMessage(IMessageLayerSecurityChannelPtr channel) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notifies the delegate a data buffer is queued and needs to be
      //          delivered on-the-wire.
      virtual void onMessageLayerSecurityChannelBufferPendingToSendOnTheWire(IMessageLayerSecurityChannelPtr channel) = 0;

    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::IMessageLayerSecurityChannelDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::IMessageLayerSecurityChannelPtr, IMessageLayerSecurityChannelPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::IMessageLayerSecurityChannelDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onMessageLayerSecurityChannelStateChanged, IMessageLayerSecurityChannelPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onMessageLayerSecurityChannelIncomingMessage, IMessageLayerSecurityChannelPtr)
ZS_DECLARE_PROXY_METHOD_1(onMessageLayerSecurityChannelBufferPendingToSendOnTheWire, IMessageLayerSecurityChannelPtr)
ZS_DECLARE_PROXY_END()
