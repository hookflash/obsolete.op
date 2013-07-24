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

#include <openpeer/stack/internal/stack_MessageLayerSecurityChannel.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/internal/stack_Account.h>
#include <openpeer/stack/internal/stack_Stack.h>
//#include <openpeer/stack/message/IMessageHelper.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>


namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    using zsLib::Stringize;
//    using stack::message::IMessageHelper;

    namespace internal
    {
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel
      #pragma mark

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannel::MessageLayerSecurityChannel(
                                                               IMessageQueuePtr queue,
                                                               IMessageLayerSecurityChannelDelegatePtr delegate,
                                                               IPeerFilesPtr localPeerFiles,
                                                               LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                               IAccountPtr account
                                                               ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IMessageLayerSecurityChannelDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mAccount(Account::convert(account)),
        mPeerFiles(localPeerFiles),
        mLocalReferenceType(localPublicKeyReferenceType),
        mCurrentState(SessionState_SendOnly)
      {
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::init()
      {
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannel::~MessageLayerSecurityChannel()
      {
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannelPtr MessageLayerSecurityChannel::convert(IMessageLayerSecurityChannelPtr channel)
      {
        return boost::dynamic_pointer_cast<MessageLayerSecurityChannel>(channel);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel => IMessageLayerSecurityChannel
      #pragma mark

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::toDebugString(IMessageLayerSecurityChannelPtr channel, bool includeCommaPrefix)
      {
        if (!channel) return String(includeCommaPrefix ? ", message incoming=(null)" : "message incoming=(null)");

        MessageLayerSecurityChannelPtr pThis = MessageLayerSecurityChannel::convert(channel);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannelPtr MessageLayerSecurityChannel::create(
                                                                         IMessageLayerSecurityChannelDelegatePtr delegate,
                                                                         IPeerFilesPtr localPeerFiles,
                                                                         LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                                         IAccountPtr account
                                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!localPeerFiles)

        MessageLayerSecurityChannelPtr pThis(new MessageLayerSecurityChannel(IStackForInternal::queueStack(), delegate, localPeerFiles, localPublicKeyReferenceType, account));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::cancel()
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      IMessageLayerSecurityChannel::SessionStates MessageLayerSecurityChannel::getState(
                                                                                        WORD *outLastErrorCode,
                                                                                        String *outLastErrorReason
                                                                                        ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::send(message::MessagePtr message)
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return false;
      }

      //-----------------------------------------------------------------------
      message::MessagePtr MessageLayerSecurityChannel::getNextPendingIncomingMessage()
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return MessagePtr();
      }

      //-----------------------------------------------------------------------
      IMessageLayerSecurityChannel::RemotePublicKeyReferenceTypes MessageLayerSecurityChannel::getRemotePublicKeyReferencedType() const
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return RemotePublicKeyReferenceType_Unknown;
      }
      

      //-----------------------------------------------------------------------
      IRSAPublicKeyPtr MessageLayerSecurityChannel::getRemotePublicKey() const
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return IRSAPublicKeyPtr();
      }

      //-----------------------------------------------------------------------
      IPeerFilePublicPtr MessageLayerSecurityChannel::getRemoteReferencedPeerFilePublic() const
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return IPeerFilePublicPtr();
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::getRemoteReferencedDomain(String *outService) const
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return String();
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr MessageLayerSecurityChannel::getNextPendingBufferToSendOnWrite()
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return SecureByteBlockPtr();
      }
      
      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::notifyReceivedFromWire(
                                                               const BYTE *buffer,
                                                               ULONG bufferLengthInBytes
                                                               )
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel  => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &MessageLayerSecurityChannel::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::log(const char *message) const
      {
        return String("MessageLayerSecurityChannel [" + Stringize<typeof(mID)>(mID).string() + "] " + message);
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("mls channel id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("peer files", mPeerFiles ? String("true") : String(), firstTime) +
               Helper::getDebugValue("local reference type", toString(mLocalReferenceType), firstTime);
      }
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageLayerSecurityChannel
    #pragma mark

    //-----------------------------------------------------------------------
    const char *IMessageLayerSecurityChannel::toString(SessionStates state)
    {
      switch (state)
      {
        case SessionState_SendOnly:     return "Send only";
        case SessionState_Connected:    return "Connected";
        case SessionState_Shutdown:     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-----------------------------------------------------------------------
    const char *IMessageLayerSecurityChannel::toString(LocalPublicKeyReferenceTypes type)
    {
      switch (type) {
        case LocalPublicKeyReferenceType_FullPublicKey: return "Full public key";
        case LocalPublicKeyReferenceType_PeerURI:       return "Peer URI";
        case LocalPublicKeyReferenceType_FingerPrint:   return "Fingerprint";
      }
      return "UNDEFINED";
    }

    //-----------------------------------------------------------------------
    const char *IMessageLayerSecurityChannel::toString(RemotePublicKeyReferenceTypes type)
    {
      switch (type) {
        case RemotePublicKeyReferenceType_Unknown:                return "Unknown";
        case RemotePublicKeyReferenceType_FullPublicKey:          return "Full public key";
        case RemotePublicKeyReferenceType_PeerURI:                return "Peer URI";
        case RemotePublicKeyReferenceType_DomainCertificatesGet:  return "Domain certificates get";
      }
      return "UNDEFINED";
    }

    //-----------------------------------------------------------------------
    IMessageLayerSecurityChannelPtr IMessageLayerSecurityChannel::create(
                                                                         IMessageLayerSecurityChannelDelegatePtr delegate,
                                                                         IPeerFilesPtr localPeerFiles,
                                                                         LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                                         IAccountPtr account
                                                                         )
    {
      return internal::IMessageLayerSecurityChannelFactory::singleton().create(delegate, localPeerFiles, localPublicKeyReferenceType, account);
    }
  }
}
