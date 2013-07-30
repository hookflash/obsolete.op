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

#include <openpeer/stack/internal/types.h>

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using services::ITransportStream;
      using services::ITransportStreamPtr;
      using services::ITransportStreamReader;
      using services::ITransportStreamReaderPtr;
      using services::ITransportStreamWriter;
      using services::ITransportStreamWriterPtr;

      using services::ITransportStreamReaderDelegate;
      using services::ITransportStreamReaderDelegatePtr;
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderRelayChannel
      #pragma mark

      interaction IFinderRelayChannel
      {
        enum SessionStates
        {
          SessionState_Pending,
          SessionState_Connected,
          SessionState_Shutdown,
        };

        static const char *toString(SessionStates state);

        static String toDebugString(IFinderRelayChannelPtr channel, bool includeCommaPrefix = true);

        //---------------------------------------------------------------------
        // PURPOSE: create a connection via a remote finder
        // NOTE:    This method is called upon receiving a "Peer Location
        //          Find". This peer may not known the public peer file
        //          for the peer it's trying to connect with.
        static IFinderRelayChannelPtr connect(
                                              IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                              AccountPtr account,
                                              ITransportStreamPtr receiveStream,
                                              ITransportStreamPtr sendStream,
                                              IPAddress remoteFinderIP,
                                              const char *localContextID,
                                              const char *relayAccessToken,
                                              const char *relayAccessSecretProof,
                                              const char *encryptDataUsingEncodingPassphrase
                                              );

        //---------------------------------------------------------------------
        // PURPOSE: create a new channel that is incoming
        // NOTE:    This method is when a finder connection receives
        //          notification of a new incoming relay channel. The remote
        //          peer should be known but until context information arrives
        //          there's no way to know which peer specificially is
        //          incoming.
        static IFinderRelayChannelPtr createIncoming(
                                                     IFinderRelayChannelDelegatePtr delegate, // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                                     AccountPtr account,
                                                     ITransportStreamPtr receiveStream,
                                                     ITransportStreamPtr sendStream
                                                     );

        //---------------------------------------------------------------------
        // PURPOSE: get process unique ID for object
        virtual PUID getID() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: subscribe to the relay channel events
        // NOTE:    If IFinderRelayChannelDelegatePtr() is specified, the
        //          default subscription used during object creation will be
        //          returned (thus allowing the default delegate subscription
        //          to be cancelled if needed).
        virtual IFinderRelayChannelSubscriptionPtr subscribe(IFinderRelayChannelDelegatePtr delegate) = 0;

        //---------------------------------------------------------------------
        // PURPOSE: immediately disconnects the channel (no signaling is needed)
        virtual void cancel() = 0;

        //---------------------------------------------------------------------
        // PURPOSE: return the current statte of the connection
        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: This method provides context for the incoming stream.
        // NOTE:    Incoming streams come in anonymously until context
        //          information about the remote location arrives. This
        //          method provides the information needed to open the
        //          bidirection communication.
        //
        //          An incoming stream will always encrypt its data
        //          using a passphrase that was privded during the "Peer
        //          Location Find" request. However, until the context
        //          information arrives, it's not possible to know which
        //          passphrase was issued to encrypt the data.
        virtual void setIncomingContext(
                                        const char *contextID,
                                        const char *decryptUsingEncodingPassphrase,
                                        IPeerPtr remotePeer
                                        ) = 0;

        //---------------------------------------------------------------------
        // PURPOSE: Obtain the context ID specified in the construction of this
        //          object (and sent to the remote party).
        virtual String getLocalContextID() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: Obtain the context ID specified by the remote party.
        // NOTE:    This can be useful to pick the correct keying material
        //          when the remote party encodes keying materials using
        //          a passphrase.
        virtual String getRemoteContextID() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: This method will return the remotely referenced peer (if
        //          known).
        // NOTE:    When a stream is incoming, this will indicate which peer
        //          the incoming stream belongs.
        virtual IPeerPtr getRemotePeer() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: This method will return the the remote public key (if
        //          known).
        // NOTE:    When the connection is outgoing, the remote peer
        //          will identify itself via it's public key since the this
        //          location may not known the public key of the remote peer
        //          just yet.
        virtual IRSAPublicKeyPtr getRemotePublicKey() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderRelayChannelDelegate
      #pragma mark

      interaction IFinderRelayChannelDelegate
      {
        typedef IFinderRelayChannel::SessionStates SessionStates;

        //---------------------------------------------------------------------
        // PURPOSE: Notifies the delegate that the state of the connection
        //          has changed.
        virtual void onFinderRelayChannelStateChanged(
                                                      IFinderRelayChannelPtr channel,
                                                      SessionStates state
                                                      ) = 0;

        //---------------------------------------------------------------------
        // PURPOSE: Notifies the delegate that the stream needs context
        //          information and passphrase.
        // NOTE:    This will be called upon an incoming stream data arriving.
        virtual void onFinderRelayChannelNeedsContext(IFinderRelayChannelPtr channel) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderRelayChannelSubscription
      #pragma mark

      interaction IFinderRelayChannelSubscription
      {
        //---------------------------------------------------------------------
        // PURPOSE: get process unique ID for subscription object
        virtual PUID getID() const = 0;

        //---------------------------------------------------------------------
        // PURPOSE: Cancel a subscription to relay channel manager events
        virtual void cancel() = 0;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::internal::IFinderRelayChannelDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::IFinderRelayChannelPtr, IFinderRelayChannelPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::IFinderRelayChannel::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onFinderRelayChannelStateChanged, IFinderRelayChannelPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onFinderRelayChannelNeedsContext, IFinderRelayChannelPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_SUBSCRIPTIONS_BEGIN(openpeer::stack::internal::IFinderRelayChannelDelegate, openpeer::stack::internal::IFinderRelayChannelSubscription)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::stack::internal::IFinderRelayChannelPtr, IFinderRelayChannelPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::stack::internal::IFinderRelayChannel::SessionStates, SessionStates)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_2(onFinderRelayChannelStateChanged, IFinderRelayChannelPtr, SessionStates)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_1(onFinderRelayChannelNeedsContext, IFinderRelayChannelPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_END()
