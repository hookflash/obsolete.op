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

#include <openpeer/services/IMessageLayerSecurityChannel.h>
#include <openpeer/services/internal/types.h>

#include <openpeer/services/ITransportStream.h>
#include <openpeer/services/IWakeDelegate.h>

#include <list>
#include <map>

#define OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM "http://meta.openpeer.org/2012/12/14/jsonmls#aes-cfb-32-16-16-sha1-md5"

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
      #pragma mark MessageLayerSecurityChannel
      #pragma mark

      class MessageLayerSecurityChannel : public Noop,
                                          public zsLib::MessageQueueAssociator,
                                          public IMessageLayerSecurityChannel,
                                          public IWakeDelegate,
                                          public ITransportStreamReaderDelegate,
                                          public ITransportStreamWriterDelegate
      {
      public:
        friend interaction IMessageLayerSecurityChannelFactory;
        friend interaction IMessageLayerSecurityChannel;

        typedef ITransportStream::StreamHeaderPtr StreamHeaderPtr;
        typedef std::list<SecureByteBlockPtr> BufferList;

        enum DecodingTypes
        {
          DecodingType_Unknown,
          DecodingType_PrivateKey,
          DecodingType_Passphrase,
        };

        static const char *toString(DecodingTypes decodingType);

        struct KeyInfo
        {
          String mIntegrityPassphrase;

          SecureByteBlockPtr mSendKey;
          SecureByteBlockPtr mNextIV;
          SecureByteBlockPtr mLastIntegrity;
        };
        
        typedef ULONG AlgorithmIndex;
        typedef std::map<AlgorithmIndex, KeyInfo> KeyMap;

      protected:
        MessageLayerSecurityChannel(
                                    IMessageQueuePtr queue,
                                    IMessageLayerSecurityChannelDelegatePtr delegate,
                                    ITransportStreamPtr receiveStreamEncoded,
                                    ITransportStreamPtr receiveStreamDecoded,
                                    ITransportStreamPtr sendStreamDecoded,
                                    ITransportStreamPtr sendStreamEncoded,
                                    const char *contextID = NULL
                                    );

        MessageLayerSecurityChannel(Noop) :
          Noop(true),
          zsLib::MessageQueueAssociator(IMessageQueuePtr()) {}

        void init();

      public:
        ~MessageLayerSecurityChannel();

        static MessageLayerSecurityChannelPtr convert(IMessageLayerSecurityChannelPtr channel);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => IMessageLayerSecurityChannel
        #pragma mark

        static String toDebugString(IMessageLayerSecurityChannelPtr channel, bool includeCommaPrefix = true);

        static MessageLayerSecurityChannelPtr create(
                                                     IMessageLayerSecurityChannelDelegatePtr delegate,
                                                     ITransportStreamPtr receiveStreamEncoded,
                                                     ITransportStreamPtr receiveStreamDecoded,
                                                     ITransportStreamPtr sendStreamDecoded,
                                                     ITransportStreamPtr sendStreamEncoded,
                                                     const char *contextID = NULL
                                                     );

        virtual IMessageLayerSecurityChannelSubscriptionPtr subscribe(IMessageLayerSecurityChannelDelegatePtr delegate);

        virtual PUID getID() const {return mID;}

        virtual void cancel();

        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const;

        virtual bool needsLocalContextID() const;

        virtual bool needsReceiveKeyingDecodingPrivateKey(
                                                          String *outFingerprint = NULL
                                                          ) const;

        virtual bool needsReceiveKeyingDecodingPassphrase() const;

        virtual bool needsReceiveKeyingMaterialSigningPublicKey() const;

        virtual bool needsSendKeyingEncodingMaterial() const;

        virtual bool needsSendKeyingMaterialToeBeSigned() const;

        virtual String getLocalContextID() const;

        virtual void setLocalContextID(const char *contextID);

        virtual String getRemoteContextID() const;

        virtual void setReceiveKeyingDecoding(
                                              IRSAPrivateKeyPtr decodingPrivateKey,
                                              IRSAPublicKeyPtr decodingPublicKey
                                              );

        virtual void setReceiveKeyingDecoding(const char *passphrase);

        virtual ElementPtr getSignedReceivingKeyingMaterial() const;

        virtual void setReceiveKeyingMaterialSigningPublicKey(IRSAPublicKeyPtr remotePublicKey);

        virtual void setSendKeyingEncoding(IRSAPublicKeyPtr remotePublicKey);

        virtual void setSendKeyingEncoding(const char *passphrase);

        virtual void getSendKeyingMaterialNeedingToBeSigned(
                                                            DocumentPtr &outDocumentContainedElementToSign,
                                                            ElementPtr &outElementToSign
                                                            ) const;

        virtual void notifySendKeyingMaterialSigned();
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => ITransportStreamReaderDelegate
        #pragma mark

        virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => ITransportStreamWriterDelegate
        #pragma mark

        virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => IWakeDelegate
        #pragma mark

        virtual void onWake();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => (internal)
        #pragma mark

        bool isShutdown() const {return SessionState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void setState(SessionStates state);
        void setError(WORD errorCode, const char *inReason = NULL);

        void step();
        bool stepReceive();
        bool stepSendKeying();
        bool stepSend();
        bool stepCheckConnected();

        bool stepProcessReceiveKeying(
                                      bool &outReturnResult,
                                      SecureByteBlockPtr keying = SecureByteBlockPtr()
                                      );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        MessageLayerSecurityChannelWeakPtr mThisWeak;

        IMessageLayerSecurityChannelDelegateSubscriptions mSubscriptions;
        IMessageLayerSecurityChannelSubscriptionPtr mDefaultSubscription;

        SessionStates mCurrentState;

        WORD mLastError;
        String mLastErrorReason;

        String mLocalContextID;
        String mRemoteContextID;

        IRSAPublicKeyPtr mSendingEncodingRemotePublicKey;
        String mSendingEncodingPassphrase;

        DocumentPtr mSendKeyingNeedingToSignDoc;  // temporary document containing the send keying material needing to be signed
        ElementPtr mSendKeyingNeedToSignEl;       // temporary element containing the send keying material needing to be signed (once notified it is signed, this element get set to EleemntPtr())

        ULONG mNextReceiveSequenceNumber;

        DecodingTypes mReceiveDecodingType;
        String mReceiveDecodingPublicKeyFingerprint;
        IRSAPrivateKeyPtr mReceiveDecodingPrivateKey;
        IRSAPublicKeyPtr mReceiveDecodingPublicKey;
        String mReceivingDecodingPassphrase;

        IRSAPublicKeyPtr mReceiveSigningPublicKey;
        DocumentPtr mReceiveKeyingSignedDoc;      // temporary document needed to resolve receive signing public key
        ElementPtr mReceiveKeyingSignedEl;        // temporary eleemnt needed to resolve receive signing public key

        ITransportStreamReaderPtr mReceiveStreamEncoded;  // typically connected to on-the-wire transport
        ITransportStreamWriterPtr mReceiveStreamDecoded;  // typically connected to "outer" layer
        ITransportStreamReaderPtr mSendStreamDecoded;     // typically connected to "outer" layer
        ITransportStreamWriterPtr mSendStreamEncoded;     // typically connected to on-the-wire transport

        ITransportStreamReaderSubscriptionPtr mReceiveStreamEncodedSubscription;
        ITransportStreamWriterSubscriptionPtr mReceiveStreamDecodedSubscription;
        ITransportStreamReaderSubscriptionPtr mSendStreamDecodedSubscription;
        ITransportStreamWriterSubscriptionPtr mSendStreamEncodedSubscription;

        AutoBool mReceiveStreamDecodedWriteReady;
        AutoBool mSendStreamEncodedWriteReady;

        KeyMap mReceiveKeys;
        KeyMap mSendKeys;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageLayerSecurityChannelFactory
      #pragma mark

      interaction IMessageLayerSecurityChannelFactory
      {
        static IMessageLayerSecurityChannelFactory &singleton();

        virtual MessageLayerSecurityChannelPtr create(
                                                      IMessageLayerSecurityChannelDelegatePtr delegate,
                                                      ITransportStreamPtr receiveStreamEncoded,
                                                      ITransportStreamPtr receiveStreamDecoded,
                                                      ITransportStreamPtr sendStreamDecoded,
                                                      ITransportStreamPtr sendStreamEncoded,
                                                      const char *contextID = NULL
                                                      );
      };
      
    }
  }
}
