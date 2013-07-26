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

#include <openpeer/stack/IMessageLayerSecurityChannel.h>
#include <openpeer/stack/message/types.h>
#include <openpeer/stack/internal/types.h>

#include <list>
#include <map>

#define OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM "http://meta.openpeer.org/2012/12/14/jsonmls#aes-cfb-32-16-16-sha1-md5"

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageLayerSecurityChannelAsyncDelegate
      #pragma mark

      interaction IMessageLayerSecurityChannelAsyncDelegate
      {
        virtual void onStep() = 0;
      };

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
                                          public IMessageLayerSecurityChannelAsyncDelegate
      {
      public:
        friend interaction IMessageLayerSecurityChannelFactory;

        typedef std::list<SecureByteBlockPtr> BufferList;

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
                                    IPeerFilesPtr localPeerFiles,
                                    LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                    const char *contextID = NULL,
                                    IAccountPtr account = IAccountPtr()
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
                                                     IPeerFilesPtr localPeerFiles,
                                                     LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                     const char *contextID = NULL,
                                                     IAccountPtr account = IAccountPtr()
                                                     );

        virtual PUID getID() const {return mID;}

        virtual void cancel();

        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const;

        virtual bool send(
                          const BYTE *buffer,
                          ULONG bufferSizeInBytes
                          );

        virtual SecureByteBlockPtr getNextIncomingMessage();

        virtual void setReceiveKeyingDecoding(const char *passphrase);

        virtual void setSendKeyingEncoding(IRSAPublicKeyPtr remotePublicKey);
        virtual void setSendKeyingEncoding(const char *passphrase);

        virtual String getLocalConextID() const;
        virtual String getRemoteConextID() const;

        virtual RemotePublicKeyReferenceTypes getRemotePublicKeyReferencedType() const;

        virtual IRSAPublicKeyPtr getRemotePublicKey() const;

        virtual IPeerFilePublicPtr getRemoteReferencedPeerFilePublic() const;

        virtual SecureByteBlockPtr getNextPendingBufferToSendOnWrite();

        virtual void notifyReceivedFromWire(
                                            const BYTE *buffer,
                                            ULONG bufferLengthInBytes
                                            );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => IMessageLayerSecurityChannelAsyncDelegate
        #pragma mark

        virtual void onStep();

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

        bool verifyReceiveSignature(ElementPtr keyingEl);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        MessageLayerSecurityChannelWeakPtr mThisWeak;

        IMessageLayerSecurityChannelDelegatePtr mDelegate;

        SessionStates mCurrentState;

        WORD mLastError;
        String mLastErrorReason;

        AccountWeakPtr mAccount;

        IPeerFilesPtr mPeerFiles;

        LocalPublicKeyReferenceTypes mLocalReferenceType;

        String mLocalContextID;
        String mRemoteContextID;

        IRSAPublicKeyPtr mSendingRemotePublicKey;
        String mSendingPassphrase;

        ULONG mNextReceiveSequenceNumber;
        IRSAPublicKeyPtr mReceivingRemotePublicKey;
        IPeerFilePublicPtr mReceivingRemotePublicPeerFile;
        String mReceivingPassphrase;

        BufferList mMessagesToEncode;
        BufferList mPendingBuffersToSendOnWire;

        BufferList mMessagesReceived;
        BufferList mReceivedBuffersToDecode;

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
        typedef IMessageLayerSecurityChannel::LocalPublicKeyReferenceTypes LocalPublicKeyReferenceTypes;

        static IMessageLayerSecurityChannelFactory &singleton();

        virtual MessageLayerSecurityChannelPtr create(
                                                      IMessageLayerSecurityChannelDelegatePtr delegate,
                                                      IPeerFilesPtr localPeerFiles,
                                                      LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                      const char *contextID = NULL,
                                                      IAccountPtr account = IAccountPtr()
                                                      );
      };
      
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::internal::IMessageLayerSecurityChannelAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
