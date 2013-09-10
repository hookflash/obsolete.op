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

namespace openpeer
{
  namespace services
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
      enum SessionStates
      {
        SessionState_Pending,
        SessionState_WaitingForNeededInformation,
        SessionState_Connected,
        SessionState_Shutdown,
      };

      static const char *toString(SessionStates state);

      static String toDebugString(IMessageLayerSecurityChannelPtr channel, bool includeCommaPrefix = true);

      //-----------------------------------------------------------------------
      // PURPOSE: create a new channel to a remote connection
      static IMessageLayerSecurityChannelPtr create(
                                                    IMessageLayerSecurityChannelDelegatePtr delegate,
                                                    ITransportStreamPtr receiveStreamEncoded,
                                                    ITransportStreamPtr receiveStreamDecoded,
                                                    ITransportStreamPtr sendStreamDecoded,
                                                    ITransportStreamPtr sendStreamEncoded,
                                                    const char *localContextID = NULL                                    // the session context ID
                                                    );

      //-----------------------------------------------------------------------
      // PURPOSE: get process unique ID for object
      virtual PUID getID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: subscribe to class events
      virtual IMessageLayerSecurityChannelSubscriptionPtr subscribe(IMessageLayerSecurityChannelDelegatePtr delegate) = 0;  // passing in IMessageLayerSecurityChannelDelegatePtr() will return the default subscription

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
      // PURPOSE: Returns true if the local context ID needs to be set
      // NOTE:    Check this method when
      //          "SessionState_WaitingForNeededInformation" is
      //          notified.
      //          Call "setLocalContextID" to provide value.
      virtual bool needsLocalContextID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Returns true if a private key is needed to decode the
      //          incoming receive keying material.
      // NOTE:    Check this method when
      //          "SessionState_WaitingForNeededInformation" is
      //          notified.
      //          Call "setReceiveKeyingDecoding" with a private key / public
      //          key pair to provide the private key used for encryption.
      virtual bool needsReceiveKeyingDecodingPrivateKey(
                                                        String *outFingerprint = NULL   // optional parameter to receive the fingerprint of the public key used to encode the keying material
                                                        ) const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Returns true if the passphrase to decode the incooming
      //          keying material is needed.
      // NOTE:    Check this method when
      //          "SessionState_WaitingForNeededInformation" is
      //          notified.
      //          Call "setReceiveKeyingDecoding" to provide value.
      virtual bool needsReceiveKeyingDecodingPassphrase() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Returns true if the signature on the incoming
      //          keying material needs validation.
      // NOTE:    Check this method when
      //          "SessionState_WaitingForNeededInformation" is
      //          notified.
      //          Call "getSignedReceivingKeyingMaterial" to examine the
      //          signing signature of the receive keying materials. This
      //          can be used to resolve the public key which should be used
      //          to validate the receive keying material's signatures. Call
      //          "setReceiveKeyingMaterialSigningPublicKey" to set the
      //          remote public key used to sign the keying material.
      virtual bool needsReceiveKeyingMaterialSigningPublicKey() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Returns true if the send keying materials needs either
      //          a remote public key or a passphrase to encrypt the encoding
      //          material.
      // NOTE:    Check this method when
      //          "SessionState_WaitingForNeededInformation" is
      //          notified.
      //          Call either "setSendKeyingEncoding" and provide the
      //          remote party's public key or the encoding passphrase.
      virtual bool needsSendKeyingEncodingMaterial() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Returns true when the send keying materials needs to be
      //          signed before transmitting the key material to the remote
      //          party.
      // NOTE:    Check this method when
      //          "SessionState_WaitingForNeededInformation" is
      //          notified.
      //          Call "getSendKeyingMaterialNeedingToBeSigned" to obtain
      //          the send keying material to be signed and call
      //          "notifySendKeyingMaterialSigned" when the keying information
      //          has been signed.
      virtual bool needsSendKeyingMaterialToeBeSigned() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtain the context ID specified in the construction of this
      //          object (and sent to the remote party).
      virtual String getLocalContextID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Set the local context ID
      // NOTE:    A local context ID is reuqired before any data can be sent
      //          to the remote party.
      virtual void setLocalContextID(const char *contextID) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtain the context ID specified by the remote party.
      // NOTE:    This can be useful to pick the correct keying material
      //          when the remote party encodes keying materials using
      //          a passphrase.
      virtual String getRemoteContextID() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: If the remote party encoded their keying materials using
      //          a public key, this rountine can be called to decode that
      //          keying material by providing the associated private key.
      // NOTE:    When "needsReceiveKeyingDecodingPrivateKey" return true,
      //          call this routine to provide the keying material needed
      //          to decode the receive keying material.
      virtual void setReceiveKeyingDecoding(
                                            IRSAPrivateKeyPtr decodingPrivateKey,
                                            IRSAPublicKeyPtr decodingPublicKey
                                            ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: If the remote party encoded their keying materials using
      //          a passphrase, this rountine can be called to decode that
      //          keying material.
      // NOTE:    The "needsReceiveKeyingDecodingPassphrase" will return true
      //          when the receive keying material needs a passphrase to
      //          decode its keying materials. Call this routine to provide
      //          the passphrase used to decode the receive keying material.
      virtual void setReceiveKeyingDecoding(const char *passphrase) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Gets the signed receiving keying material.
      // NOTE:    Once the first keying material is received, the public
      //          key that should have been used to sign the package will need
      //          to be provided (if it's not contained within the package).
      //          The signature associated to this package can be examined
      //          to help reolve the public key that is to be used to validate
      //          the receive keying material.
      virtual ElementPtr getSignedReceivingKeyingMaterial() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Sets the public key to be used to validate the signature
      //          of the received keying material.
      // NOTE:    This is required to call if the public key used to sign the
      //          receive keying material cannot be automatically resolved.
      //          "needsReceiveKeyingMaterialSigningPublicKey" will return
      //          true under this condition.
      virtual void setReceiveKeyingMaterialSigningPublicKey(IRSAPublicKeyPtr remotePublicKey) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Calling this routine causes the keying material to be
      //          encoded using the public key specified.
      // NOTE:    When "needsSendKeyingEncodingMaterial" returns true, this
      //          routine can be used to provide a public key to use to
      //          encode the send keying material.
      virtual void setSendKeyingEncoding(IRSAPublicKeyPtr remotePublicKey) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Calling this routine causes the keying material to be encoded
      //          using the pass phrase specified.
      // NOTE:    When "needsSendKeyingEncodingMaterial" returns true, this
      //          routine can be used to provide a passphrase to use to encode
      //          the send keying material.
      virtual void setSendKeyingEncoding(const char *passphrase) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains the send keying material that needs to be signed.
      // NOTE:    This method needs to be called when
      //          "needsSendKeyingMaterialToeBeSigned" returns true and
      //          the element returned is the element to sign.
      //          This element needs to be modified by the caller and once
      //          the signature is applied to the element then
      //          "notifySendKeyingMaterialSigned" needs to be called to
      //          confirm the completion of the signature process.
      virtual void getSendKeyingMaterialNeedingToBeSigned(
                                                          DocumentPtr &outDocumentContainedElementToSign,
                                                          ElementPtr &outElementToSign
                                                          ) const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notified the signture has been applied to the result of
      //          "getSendKeyingMaterialNeedingToBeSigned"
      // NOTE:    This method needs to be called when
      //          "needsSendKeyingMaterialToeBeSigned" returns true after the
      //          signing signature has been applied to the result of
      //          "getSendKeyingMaterialNeedingToBeSigned".
      virtual void notifySendKeyingMaterialSigned() = 0;
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
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageLayerSecurityChannelSubscription
    #pragma mark

    interaction IMessageLayerSecurityChannelSubscription
    {
      virtual void cancel() = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::IMessageLayerSecurityChannelDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IMessageLayerSecurityChannelPtr, IMessageLayerSecurityChannelPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IMessageLayerSecurityChannelDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onMessageLayerSecurityChannelStateChanged, IMessageLayerSecurityChannelPtr, SessionStates)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_SUBSCRIPTIONS_BEGIN(openpeer::services::IMessageLayerSecurityChannelDelegate, openpeer::services::IMessageLayerSecurityChannelSubscription)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::services::IMessageLayerSecurityChannelPtr, IMessageLayerSecurityChannelPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::services::IMessageLayerSecurityChannelDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_2(onMessageLayerSecurityChannelStateChanged, IMessageLayerSecurityChannelPtr, SessionStates)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_END()
