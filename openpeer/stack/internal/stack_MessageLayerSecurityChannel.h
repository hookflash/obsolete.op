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
#include <openpeer/stack/internal/types.h>

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
      #pragma mark MessageLayerSecurityChannel
      #pragma mark

      class MessageLayerSecurityChannel : public Noop,
                                          public zsLib::MessageQueueAssociator,
                                          public IMessageLayerSecurityChannel
      {
      public:
        friend interaction IMessageLayerSecurityChannelFactory;

      protected:
        MessageLayerSecurityChannel(
                                    IMessageQueuePtr queue,
                                    IMessageLayerSecurityChannelDelegatePtr delegate,
                                    IPeerFilesPtr localPeerFiles,                                   // needs a public / private keer pair to operate
                                    LocalPublicKeyReferenceTypes localPublicKeyReferenceType,       // how should the local public key be referenced in the MLS channel negotiations
                                    IAccountPtr account = IAccountPtr()                             // if specified, the account object will be used to resolve peer URI remotely referenced public keys
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
                                                     IPeerFilesPtr localPeerFiles,                                   // needs a public / private keer pair to operate
                                                     LocalPublicKeyReferenceTypes localPublicKeyReferenceType,       // how should the local public key be referenced in the MLS channel negotiations
                                                     IAccountPtr account = IAccountPtr()                             // if specified, the account object will be used to resolve peer URI remotely referenced public keys
                                                     );

        virtual PUID getID() const {return mID;}

        virtual void cancel();

        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const;

        virtual bool send(message::MessagePtr message);

        virtual message::MessagePtr getNextPendingIncomingMessage();

        virtual RemotePublicKeyReferenceTypes getRemotePublicKeyReferencedType() const;

        virtual IRSAPublicKeyPtr getRemotePublicKey() const;

        virtual IPeerFilePublicPtr getRemoteReferencedPeerFilePublic() const;

        virtual String getRemoteReferencedDomain(String *outService = NULL) const;

        virtual SecureByteBlockPtr getNextPendingBufferToSendOnWrite();

        virtual void notifyReceivedFromWire(
                                            const BYTE *buffer,
                                            ULONG bufferLengthInBytes
                                            );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageLayerSecurityChannel => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        IMessageLayerSecurityChannelWeakPtr mThisWeak;

        IMessageLayerSecurityChannelDelegatePtr mDelegate;

        SessionStates mCurrentState;

        WORD mLastError;
        String mLastErrorReason;

        AccountWeakPtr mAccount;

        IPeerFilesPtr mPeerFiles;

        LocalPublicKeyReferenceTypes mLocalReferenceType;
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
                                                      IPeerFilesPtr localPeerFiles,                                   // needs a public / private keer pair to operate
                                                      LocalPublicKeyReferenceTypes localPublicKeyReferenceType,       // how should the local public key be referenced in the MLS channel negotiations
                                                      IAccountPtr account = IAccountPtr()                             // if specified, the account object will be used to resolve peer URI remotely referenced public keys
                                                      );
      };
      
    }
  }
}
